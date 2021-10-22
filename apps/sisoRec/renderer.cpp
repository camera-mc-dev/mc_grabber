#ifdef USE_SISO

#include "controls.h"
#include "grab.h"
#include "renderer.h"


#include "renderer2/basicRenderer.h"
#include "renderer2/geomTools.h"
#include "renderer2/sdfText.h"

#include "commonConfig/commonConfig.h"

void PrepRenderWindow(SiSoGrabber *grabber, unsigned &width, unsigned &height)
{
	long int imgCols, imgRows;
	grabber->GetResolution( 0, imgCols, imgRows );
	
	CommonConfig ccfg;
	
	// find out how big a window to make
	float winW = ccfg.maxSingleWindowWidth;
	float winH = winW * imgRows / (float)imgCols;
	if( winH > ccfg.maxSingleWindowHeight )
	{
		winH = ccfg.maxSingleWindowHeight;
		winW = winH * imgCols / (float)imgRows;
	}
	
	float rat = winH / winW;
	
	if( winW > ccfg.maxSingleWindowWidth )
	{
		winW = ccfg.maxSingleWindowWidth;
		winH = rat * winW / rat;
	}
	
	cout << "win W, H: " << winW << ", " << winH << endl;
	width = winW;
	height = winH;
	
	return;
}


RecRenderer::RecRenderer(unsigned width, unsigned height, std::string title) : BasicRenderer(width,height,title)
{
	CommonConfig ccfg;
	
	Rendering::NodeFactory::Create( fpsWarnNode, "fpsWarnNode" );
	
}


void RecRenderer::Prep( SiSoGrabber *grabber )
{
	grabber->GetResolution( 0, imgCols, imgRows );
	for( unsigned cc = 0; cc < grabber->GetNumCameras(); ++cc )
	{
		std::shared_ptr<Rendering::MeshNode> mn;
		cv::Mat img(imgRows, imgCols, CV_8UC1, cv::Scalar(0) );
		std::stringstream ss; ss << "image-" << cc;
		
		mn = Rendering::GenerateImageNode( 0, 0, img.cols, img, ss.str(), smartThis );
		mn->GetTexture()->SetFilters(GL_NEAREST, GL_NEAREST);
		imgCards.push_back(mn);
		
		Get2dBgRoot()->AddChild( imgCards[cc] );
	}
	
	std::stringstream fntss;
	fntss << ccfg.coreDataRoot << "/NotoMono-Regular.ttf";
	Rendering::SDFText textMaker(fntss.str(), smartThis);
	
	textMaker.RenderString("! Observed FPS low !", 0.1, 1.0, 0.1, 0.1, fpsWarnNode);
	transMatrix3D T = transMatrix3D::Identity();
	T(0,3) = 0.02;
	T(1,3) = 0.4;
	fpsWarnNode->SetTransformation(T);
	
	
	
}

void RecRenderer::Update( std::vector< cv::Mat > &bgrImgs, std::map<int, bool> &dispCams )
{
	int numCams = 0;
	for( auto ci = dispCams.begin(); ci != dispCams.end(); ++ci )
	{
		if( ci->second == true )
			++numCams;
	}
	
	unsigned numRows = 1;
	unsigned numCols = 1;
	if( numCams > 16)
		throw std::runtime_error("I'm just refusing to show that many images!");
	else
	{
		numRows = numCols = (int)ceil( sqrt(numCams) );
	}
	
	// shove all images off-screen (we'll position the visible ones in a moment)
	for( unsigned cc = 0; cc < bgrImgs.size(); ++cc )
	{
		transMatrix3D T = transMatrix3D::Identity();
		T(0,3) = 100000;
		T(1,3) = 0;
		imgCards[ cc ]->SetTransformation(T);
	}

	Get2dBgCamera()->SetOrthoProjection(0, numCols * imgCols, 0, numRows * imgRows, -10, 10 );
	
	int cc = 0;
	for( auto ci = dispCams.begin(); ci != dispCams.end(); ++ci )
	{
		if( ci->second )
		{
			unsigned r = cc/numCols;
			unsigned c = cc%numCols;
			
			float x,y;
			x = c * imgCols;
			y = r * imgRows;
			
			transMatrix3D T = transMatrix3D::Identity();
			T(0,3) = x;
			T(1,3) = y;
			imgCards[ ci->first ]->SetTransformation(T);
			
			imgCards[ci->first]->GetTexture()->UploadImage( bgrImgs[ ci->first ] );
			
			++cc;
		}
	}
	
	
	
	
}

bool RecRenderer::Step( bool &buffRecord, bool &liveRecord )
{
	win.setActive();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Render();

	sf::Event ev;
	bool close = false;
	while( win.pollEvent(ev) )
	{
		if( ev.type == sf::Event::Closed )
			close = true;
		
		if( ev.type == sf::Event::KeyReleased )
		{			
			if (!liveRecord && ev.key.code == sf::Keyboard::Space )
			{
				buffRecord = true;
			}
			if (ev.key.code == sf::Keyboard::R )
			{
				liveRecord = !liveRecord;
			}
		}
	}
	return close;
}


void RecRenderer::AddImgChild( unsigned img, std::shared_ptr<Rendering::MeshNode> node )
{
	imgCards[img]->AddChild( node );
}

void RecRenderer::ClearImgChildren( unsigned img )
{
	imgCards[img]->Clean();
}


#endif 
