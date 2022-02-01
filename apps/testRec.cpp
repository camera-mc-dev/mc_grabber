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
	
	FakeGrabber grabber(0);

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


}