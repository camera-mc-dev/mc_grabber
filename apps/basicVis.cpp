#ifdef USE_SISO

#include "renderer2/basicRenderer.h"
#include "renderer2/geomTools.h"
#include "commonConfig/commonConfig.h"

#include "imgio/siso.h"
#include "imgio/loadsave.h"

int main(int argc, char* argv[])
{
	if( argc != 3 )
	{
		cout << "Basic visualisation of camera feeds using OpenCV Debayering." << endl;
		cout << "Usage: " << endl;
		cout << argv[0] << " <x-resolution> <y-resolution> " << endl;
		exit(0);
	}
	
	long int imgCols = atoi(argv[1]);
	long int imgRows = atoi(argv[2]);
	
	CommonConfig ccfg;
	//
	// first off, create the grabber and find out how many cameras we have
	//
	std::vector<SiSoBoardInfo> boardInfo(1);
	
	boardInfo[0].boardIndx = 0;
	boardInfo[0].grabberConfig = "/home/me475/docs/Stemmer/MS_Generator_Master_ME.mcf";
	
// 	boardInfo[1].boardIndx = 1;
// 	boardInfo[1].grabberConfig = "/home/me475/docs/Stemmer/MS_Generator_Slave_ME.mcf";
	
	cout << "Create Grabber..." << endl;
	SiSoGrabber grabber( boardInfo );
	
	grabber.PrintCameraInfo();
	
	grabber.SetFPS(30, 0);
	grabber.SetResolution( imgCols, imgRows );
	long int exposure = 20000;
	grabber.SetExposure( exposure );
	
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
	float winW = ccfg.maxSingleWindowWidth;
	float winH = winW * (imgRows * numRows)/(imgCols * numCols);
	if( winH > ccfg.maxSingleWindowHeight )
	{
		winH = ccfg.maxSingleWindowHeight;
		winW = winH * (imgCols * numCols) / (imgRows * numRows);
	}
	
	float rat = winH / winW;
	
	if( winW > ccfg.maxSingleWindowWidth )
	{
		winW = ccfg.maxSingleWindowWidth;
		winH = rat * winW / rat;
	}
	
	cout << "win W, H: " << winW << ", " << winH << endl;
	
	// create renderer
	std::shared_ptr<Rendering::BasicRenderer> renderer;
	Rendering::RendererFactory::Create( renderer, winW,winH, "SISO/JAI camera viewer" );
	
	float renW, renH;
	renW = numCols * imgCols;
	renH = numRows * imgRows;
	
	renderer->Get2dBgCamera()->SetOrthoProjection(0, renW, 0, renH, -10, 10 );
	
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
	
	//
	// Now we should be able to grab images and display them on the renderer
	//
	std::vector< cv::Mat > bgrImgs( grabber.GetNumCameras() );
	
	
	grabber.StartAcquisition(1, 0);
	grabber.StartTrigger(0);
	
	bool done = false;
	while( !done )
	{
		grabber.GetCurrent(2);
		auto fnos = grabber.GetFrameNumbers();
		
		for( unsigned cc = 0; cc < bgrImgs.size(); ++cc )
		{
			cv::cvtColor( grabber.currentFrames[cc], bgrImgs[cc], cv::COLOR_BayerGB2BGR );
// 			bgrImgs[cc] = grabber.currentFrames[cc];
			imgCards[cc]->GetTexture()->UploadImage( bgrImgs[cc] );
		}
		
		done = renderer->StepEventLoop();
	}
	
	grabber.StopTrigger(0);
	grabber.StopAcquisition();
	
}

#endif
