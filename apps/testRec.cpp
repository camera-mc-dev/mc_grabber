#include "imgio/fake.h"

#include "renderer2/basicRenderer.h"
#include "renderer2/geomTools.h"
#include "commonConfig/commonConfig.h"

class RecRenderer : public Rendering::BasicRenderer
{
public:
	friend class Rendering::RendererFactory;

	void Step(bool &done, bool &record)
	{
		done = false;
		win.setActive();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		Render();

		sf::Event ev;
		while( win.pollEvent(ev) )
		{
			if( ev.type == sf::Event::Closed )
				done = true;

			if( ev.type == sf::Event::KeyReleased )
			{
				if (ev.key.code == sf::Keyboard::Space )
				{
					record = true;
// 					record = !record;
				}
			}

		}

	}
protected:
	RecRenderer(unsigned width, unsigned height, std::string title) : BasicRenderer(width,height,title)
	{

	}
};

int main(int argc, char* argv[])
{

	long int imgCols = 480;
	long int imgRows = 360;
	long int maxSingleWindowWidth = 1000;
	long int maxSingleWindowHeight = 800;

	FakeGrabber grabber("/media/reuben/HDD/Work/test.mp4");
	grabber.PrintCameraInfo();
	//
	// Now use a basic renderer to display all of the cameras.
	//

	// decide how many rows and columns we want to show.
	unsigned numRows = 1;
	unsigned numCols = 1;
	if( grabber.GetNumCameras() > 16)
		throw std::runtime_error("I'm just refusing to show that many images!");
	else
	{
		numRows = numCols = (int)ceil( sqrt(grabber.GetNumCameras()) );
	}
	cout << "num rows: " << numRows << endl;

	// find out how big a window to make
	float winW = maxSingleWindowWidth;
	float winH = winW * (imgRows * numRows)/(imgCols * numCols);
	if( winH > maxSingleWindowHeight )
	{
		winH = maxSingleWindowHeight;
		winW = winH * (imgCols * numCols) / (imgRows * numRows);
	}

	float rat = winH / winW;

	if( winW > maxSingleWindowWidth )
	{
		winW = maxSingleWindowWidth;
		winH = rat * winW / rat;
	}

	cout << "win W, H: " << winW << ", " << winH << endl;

	// create renderer
	std::shared_ptr<RecRenderer> renderer;
	Rendering::RendererFactory::Create( renderer, winW,winH, "SISO/JAI camera viewer" );

	float renW, renH;
	renW = numCols * imgCols;
	renH = numRows * imgRows;

	renderer->Get2dBgCamera()->SetOrthoProjection(0, renW, 0, renH, -10, 10 );

	std::vector< cv::Mat > bgrImgs( grabber.GetNumCameras() );

	std::vector< std::shared_ptr<Rendering::MeshNode> > imgCards;
	for( unsigned cc = 0; cc < grabber.GetNumCameras(); ++cc )
	{
		std::shared_ptr<Rendering::MeshNode> mn;
		cv::Mat img(imgRows, imgCols, CV_8UC1, cv::Scalar(0) );
		std::stringstream ss; ss << "image-" << cc;

		unsigned r = cc/numCols;
		unsigned c = cc%numCols;

		float x,y;
		x = c * imgCols;
		y = r * imgRows;

		mn = Rendering::GenerateImageNode( x, y, img.cols, img, ss.str(), renderer );
		mn->GetTexture()->SetFilters(GL_NEAREST, GL_NEAREST);
		imgCards.push_back(mn);
		renderer->Get2dBgRoot()->AddChild( mn );
	}

	//render loop
	bool done = false;
	bool record = false;
	while(!done)
	{
		//
		// Display. Going to try without threading for now.
		//
		//auto fnos = grabber.GetFrameNumbers();
		for( unsigned cc = 0; cc < bgrImgs.size(); ++cc )
		{
			// long int bufIdx = fnos[cc] % tdata.buffersNeeded;
			// cv::cvtColor( tdata.rawBuffers[cc][bufIdx], bgrImgs[cc], cv::COLOR_BayerGB2BGR );
			grabber.GetCurrent();
			bgrImgs[cc] = grabber.currentFrames[cc];
			imgCards[cc]->GetTexture()->UploadImage( bgrImgs[cc] );
		}
		renderer->Step(done,record);
	}

}