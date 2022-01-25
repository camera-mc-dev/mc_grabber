#ifdef USE_SISO

#include <iomanip>
#include <thread>

#include "renderer2/basicRenderer.h"
#include "renderer2/geomTools.h"
#include "commonConfig/commonConfig.h"

#include "imgio/siso.h"
#include "imgio/loadsave.h"

//
// Very trivial renderer that tells us if we pressed the space button,
// we can then use that to stop recording and dump the buffer to disk.
//
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

struct ThreadData
{
	// pointer to the grabber
	AbstractGrabber *grabber;
	
	// the recording buffer
	int currentBufferIndx;
	int buffersNeeded;
	std::vector< std::vector< cv::Mat > > rawBuffers;
	std::vector< std::vector<unsigned long> > bufferFrameIdx;
	
	// are we done yet?
	bool done;
};

void GrabThread( ThreadData *tdata )
{
	tdata->grabber->StopTrigger(0);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	tdata->grabber->StartAcquisition(10, 0);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	tdata->grabber->StartTrigger(0);
	tdata->grabber->GetCurrentEnsureSynch(2);
	
	auto prevfnos = tdata->grabber->GetFrameNumbers();
	std::vector< cv::Mat* > dsts;
	dsts.resize( tdata->rawBuffers.size() );
	while( !tdata->done )
	{
		//
		// Grab...
		//
		//tdata->grabber->GetCurrentEnsureSynch(2);
		frameindex_t fno = tdata->grabber->GetSyncFrame(2);
		unsigned long bufIdx = fno % tdata->buffersNeeded;
		for( unsigned cc = 0; cc < tdata->rawBuffers.size(); ++cc )
		{
			dsts[cc] = &tdata->rawBuffers[cc][bufIdx];
			tdata->bufferFrameIdx[cc][bufIdx] = fno;
		}
		tdata->grabber->GetNumberedFrame( fno, 2, dsts );
		auto fnos = tdata->grabber->GetFrameNumbers();
		
		for( unsigned cc = 0; cc < tdata->rawBuffers.size(); ++cc )
		{			
			cout << cc << " : " << fnos[cc] << "    " << prevfnos[cc] << " " << fnos[cc] - prevfnos[cc] << endl;
		}
		cout << "---" << endl;
		
		
		
		prevfnos = fnos;
	}
	
	tdata->grabber->StopTrigger(0);
	tdata->grabber->StopAcquisition();
	
	return;
}


int main(int argc, char* argv[])
{
	if( argc != 7 )
	{
		cout << "Basic buffered recording of camera feeds." << endl;
		cout << "Usage: " << endl;
		cout << argv[0] << " <number of boards> <FPS> <recording time (s)> <exposure (ms)> <x-resolution> <y-resolution> " << endl;
		exit(0);
	}
	
	int numBoards      = atoi(argv[1]);
	int fps            = atoi(argv[2]);
	int recTime        = atoi(argv[3]);
	long int exposure  = atoi(argv[4]) * 1000; 
	long int imgCols   = atoi(argv[5]);
	long int imgRows   = atoi(argv[6]);
	
	CommonConfig ccfg;
	//
	// first off, create the grabber and find out how many cameras we have
	//
	
	std::vector<SiSoBoardInfo> boardInfo(numBoards);
	for( unsigned bc = 0; bc < numBoards; ++bc )
	{
		std::stringstream ss;
		boardInfo[bc].boardIndx = bc;
		if( bc == 0 )
		{
			ss << ccfg.coreDataRoot << "/SiSo/MS_Generator_Master_ME.mcf";
			boardInfo[bc].grabberConfig = ss.str();
		}
		else
		{
			ss << ccfg.coreDataRoot << "/SiSo/MS_Generator_Slave_ME.mcf";
			boardInfo[bc].grabberConfig = ss.str();
		}
	}
	
	
	cout << "Create Grabber..." << endl;
	SiSoGrabber grabber( boardInfo );
	
	grabber.PrintCameraInfo();
	
	grabber.SetFPS(fps, 0);
	grabber.SetResolution( imgCols, imgRows );
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
	std::shared_ptr<RecRenderer> renderer;
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
	// The FG library has a limit of 256 frames in its ring buffer, which is somewhat limiting.
	// (upon experimentation, it really appears to be 1048, but that is still too limiting).
	//
	// As such, we will need to manage our own in-memory buffering for recording, which
	// is obviously very annoying.
	//
	ThreadData tdata;
	tdata.done = false;
	tdata.grabber = &grabber;
	tdata.currentBufferIndx = 0;
	tdata.buffersNeeded = fps * recTime;
	tdata.rawBuffers.resize( grabber.GetNumCameras() );
	tdata.bufferFrameIdx.resize( grabber.GetNumCameras() );
	for( unsigned cc = 0; cc < tdata.rawBuffers.size(); ++cc )
	{
		tdata.rawBuffers[cc].resize( tdata.buffersNeeded );
		for( unsigned bc = 0; bc < tdata.buffersNeeded; ++bc )
		{
			tdata.rawBuffers[cc][bc] = cv::Mat( imgRows, imgCols, CV_8UC1, cv::Scalar(0) );
		}
		
		tdata.bufferFrameIdx[cc].assign( tdata.buffersNeeded, 0);
	}
	
	
	//
	// Start the thread to start filling the recording buffer.
	//
	auto gthread = std::thread(GrabThread, &tdata);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//
	// Now we should be able to grab images and display them on the renderer
	//
	std::vector< cv::Mat > bgrImgs( grabber.GetNumCameras() );
	
	

	
	
	bool record = false;
	while( !tdata.done )
	{
		
		
		
		//
		// Display. Probably too slow to do this while also grabbing!
		// So will need some threading to put the grabbing in a thread.
		//
		auto fnos = grabber.GetFrameNumbers();
		for( unsigned cc = 0; cc < bgrImgs.size(); ++cc )
		{
			long int bufIdx = fnos[cc] % tdata.buffersNeeded;
			cv::cvtColor( tdata.rawBuffers[cc][bufIdx], bgrImgs[cc], cv::COLOR_BayerGB2BGR );
// 			bgrImgs[cc] = grabber.currentFrames[cc];
			imgCards[cc]->GetTexture()->UploadImage( bgrImgs[cc] );
			
		}
		
		renderer->Step(tdata.done, record);
		
		if( record )
		{
			record = false;
			grabber.StopTrigger(0); // not so important now too be honest.
			
			auto fnos = grabber.GetFrameNumbers();
			auto earliest = fnos[0];
			for( unsigned cc = 0; cc < fnos.size(); ++cc )
			{
				earliest = std::min( earliest, fnos[cc] );
			}
			int startIdx = earliest % tdata.buffersNeeded;
			
			for( unsigned ic = 0; ic < tdata.buffersNeeded; ++ic )
			{
				int bufIndx = startIdx + ic;
				
				if( bufIndx >= tdata.buffersNeeded )
					bufIndx = bufIndx - tdata.buffersNeeded;
				
				for( unsigned cc = 0; cc < tdata.rawBuffers.size(); ++cc )
				{
					if( tdata.bufferFrameIdx[cc][ bufIndx ] == 0 )
						continue;
					std::stringstream ss;
					ss << "/data/raid0/recTest/" << std::setw(2) << std::setfill('0') << cc << "/"
					   << std::setw(12) << std::setfill('0') << tdata.bufferFrameIdx[cc][ bufIndx ] << ".charImg";
					SaveImage( tdata.rawBuffers[cc][ bufIndx ], ss.str() );
				}
			}
			
			grabber.StartTrigger(0);
		}
	}
	
}

#endif
