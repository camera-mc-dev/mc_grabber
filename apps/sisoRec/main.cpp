#ifdef USE_SISO

// #define GRABBER_SAVE_AS_HDF5


#include <iostream>
#include <thread>

using std::cout;
using std::endl;

#include "controls.h"
#include "renderer.h"
#include "gridDetect.h"

#include "misc/imgSender.h"
#include "imgproc/debayer/debayer.h"

#include "imgio/hdf5source.h"
#include "imgio/fake.h"

#include <boost/filesystem.hpp>
namespace fs  = boost::filesystem;
#include <iomanip>
#include "gdk/gdk.h"

#include "config.h"
#include "commonConfig/commonConfig.h"

#include <chrono>

void GetSaveRoots( GUIThreadData &gtdata );

void PrepSaveDirectories( const std::vector<std::string> outDir, unsigned numCameras, GUIThreadData gtdata );




std::shared_ptr< Rendering::MeshNode > CreateGridNode( 
                                                       unsigned gridRows, 
                                                       unsigned gridCols,
                                                       std::shared_ptr<RecRenderer> ren,
                                                       std::vector< CircleGridDetector::GridPoint > &grid
                                                     );
void CreateGridNodes( 
                      unsigned grows, 
                      unsigned gcols,
                      unsigned numVis,
                      std::shared_ptr<RecRenderer> ren,
                      std::vector< std::vector< std::vector< CircleGridDetector::GridPoint > > > &grids
                    );


void SaveEven(GrabThreadData *data);
void SaveOdd(GrabThreadData *data);


int main(int argc, char* argv[])
{

	if( argc < 2 )
	{
		cout << "Usage: " << endl;
		cout << argv[0] << " <number of framegrabbers> <video path (optional)>" << endl;
		return 0;
	}
	
	CommonConfig ccfg;
	
	boost::asio::io_service ioService;
	
	int numBoards      = atoi(argv[1]);
	
	// optional parameters
	std::vector< std::string > videoPaths;
	if (argc > 2)
	{
		for( unsigned c = 2; c < argc; ++c )
			videoPaths.push_back( argv[c] );
	}
	bool fpsWarningActive = false;
	
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
	AbstractGrabber* grabber;
	if (videoPaths.empty())
	{
		grabber = new SiSoGrabber(boardInfo);
	}
	else
	{
		grabber = new FakeGrabber(videoPaths);
	}
	
	grabber->PrintCameraInfo();
// 	grabber->SetOutput1StateLow(0);
	grabber->SetOutput1StateHigh(0); // default to being high instead.
	
	// 1) Launch the GTKMM GUI thread for controls
	GUIThreadData gtdata;
	gtdata.done = false;
	gtdata.grabber = grabber;

	// Find out where we're saving images to.
	GetSaveRoots( gtdata );
	
	auto guiThread = std::thread(GUIThread, &gtdata);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	
	std::unique_lock<std::mutex> gtlock(gtdata.signalHandler->mtx);
	while(!gtdata.signalHandler->ready) gtdata.signalHandler->cv.wait(gtlock);
	
	
	
	
	// create the image sender
	ImageSender isender(ioService, 90210);
	
	std::vector<cv::Mat> sendImgs(1);
	sendImgs[0] = cv::Mat(100, 100, CV_8UC1, cv::Scalar(128) );
	isender.SetImages("SiSo Rec image", sendImgs );
	
	//gdk_threads_add_idle(ControlsWindow::SetAllGainsAndExposures, gtdata.window);
	
	// loop infinitely, responding to requests from the controls thread.
	while( !gtdata.done )
	{
		// wait for the grabber's signal...
		std::unique_lock<std::mutex> lock( gtdata.window->grabbingMutex );
		gtdata.window->grabbingCV.wait( lock );
		
		if( gtdata.window->grabbing )
		{
			GrabThreadData &tdata = gtdata.window->gdata;
			
			
			cv::Mat sharesGrid;
			std::shared_ptr<Rendering::BasicRenderer> sharesGridRen;
			int numCameras = tdata.rawBuffers.size();
			if( gtdata.window->IsLiveGridDetectEnabled() )
			{
				sharesGrid = cv::Mat( numCameras, numCameras, CV_32FC1, cv::Scalar(0) );
				Rendering::RendererFactory::Create( sharesGridRen, 200,200 , "grid shares" );
				sharesGridRen->Get2dBgCamera()->SetOrthoProjection(0,numCameras,0,numCameras,-10,10);
				auto bgn = sharesGridRen->SetBGImage(sharesGrid);
				bgn->GetTexture()->SetFilters(GL_NEAREST, GL_NEAREST);
			}
			
			
			
			// Create renderer
			unsigned winW, winH;
			PrepRenderWindow( grabber, winW, winH );
			
			cout << winW << " " << winH << endl;
			
			
			std::shared_ptr<RecRenderer> renderer;
			Rendering::RendererFactory::Create( renderer, winW,winH, "SISO/JAI camera recorder" );
			renderer->Prep( grabber );
			
			// display something if we are live recording
			renderer->Get2dFgCamera()->SetOrthoProjection(0, 1, 0, 1, -10, 10 );
			hVec2D cent; cent << 0.5, 0.5, 1.0f;
			auto liveRecCircle = Rendering::GenerateCircleNode2D( cent, 0.02, 0.01, "live rec circle", renderer ); 
			Eigen::Vector4f rcol; rcol << 1.0, 0.0, 0.0, 0.1;
			transMatrix3D T = transMatrix3D::Identity();
			T(0,3) = -5;
			liveRecCircle->SetTransformation(T);
			liveRecCircle->SetTexture( renderer->GetBlankTexture() );
			liveRecCircle->SetBaseColour(rcol);
			renderer->Get2dFgRoot()->AddChild( liveRecCircle );
			
			
			// Launch the grid detection thread which will probably spend 
			// most of its time idling.
			SGridDetectData gdtdata; // hmmm... to many g*datas!
			std::shared_ptr< std::thread > gridThread;
			if( gtdata.window->IsLiveGridDetectEnabled() )
			{
				gdtdata.done = false;
				gdtdata.renderer = renderer;
				gdtdata.gridNo = &tdata.calibGrabNo;
				gdtdata.grids  = &tdata.grids;
				gdtdata.outDir = gtdata.saveRoot0;
				gdtdata.numCameras = grabber->GetNumCameras();
				
				gridThread.reset( new std::thread(GridDetectThread, &gdtdata) );
			}
			unsigned numVis = 0;
			
			// Launch the batch image saving threads
			//
			tdata.savingOdd = false;
			tdata.savingEven = false;
// 			tdata.mutOdd.unlock();
// 			tdata.mutEven.unlock();
			tdata.saveProgress.assign( grabber->GetNumCameras(), 0.0f );
			tdata.needSavingThreads = true;
			std::thread saveEvenThread( SaveEven, &tdata );
			std::thread saveOddThread(  SaveOdd,  &tdata );
			
			
			bool liveRecord = false;
			
			std::vector< cv::Mat > bgrImgs( grabber->GetNumCameras() );
			
			std::map<int, bool> dispCams;
			tdata.meanfps = -1.0f;
			bool new_liverecord = false;
			// run renderer
			while( !gtdata.done && gtdata.window->grabbing )
			{
				// guess at framerate...
				int val = (int)floor(1000.0 / gtdata.window->gdata.lastGrabDuration.count());
				if( tdata.meanfps > 0 )
				{
					float rat = 0.05;
					tdata.meanfps = rat * val + (1.0-rat) * tdata.meanfps;
				}
				else
				{
					tdata.meanfps = val;
				}
				if( tdata.meanfps < 0.999 * gtdata.window->GetDesiredFPS() )
				{
					//
					// Put in highly visible FPS warning error ! ! ! 
					//
					
					// 1) Create warning error text nodes elsewhere in advance.
					// 2) Make sure they are attached to the render tree here.
					if( !fpsWarningActive )
					{
						renderer->RaiseFPSWarning();
						fpsWarningActive = true;
					}
				}
				else
				{
					// Make sure the warning text nodes are not attached to the render tree.
					if( fpsWarningActive )
					{
						renderer->StopFPSWarning();
						fpsWarningActive = false;
					}
				}
				
				gtdata.window->GetCameraDisplayInfo( dispCams );
				
				auto fnos = grabber->GetFrameNumbers();
				GrabThreadData &gdata = gtdata.window->gdata;
				for( unsigned cc = 0; cc < bgrImgs.size(); ++cc )
				{
					// if we display fno-1 then we shouldn't run into the situation where we
					// grab an image at the same time as the system is writing an image.
					long int bufIdx = (fnos[cc]-1) % gdata.buffersNeeded;
					cv::cvtColor( gdata.rawBuffers[cc][bufIdx], bgrImgs[cc], cv::COLOR_BayerGB2BGR_EA );
					//DebayerLED( gdata.rawBuffers[cc][bufIdx], bgrImgs[cc], GRBG );
				}
				
				renderer->Update( bgrImgs, dispCams );
				
				bool buffRecord = false;
				if( renderer->Step( buffRecord, liveRecord ) )
				{
					gdk_threads_add_idle(ControlsWindow::StopGrabbing, gtdata.window);
				}
				
				
				sendImgs[0] = bgrImgs[ gtdata.window->GetSendImageIndx() ];
				isender.SetImages("SiSo Rec image", sendImgs );
				
				
				if( buffRecord )
				{
					// just so we can see the progress bars for cameras that have not started.
					tdata.saveProgress.assign( grabber->GetNumCameras(), 0.01f );
					
					buffRecord = false;
					liveRecord = false;
					grabber->StopTrigger(0);
// 					grabber->SetOutput1StateHigh(0);
					grabber->SetOutput1StateLow(0);
				
					auto t0 = std::chrono::steady_clock::now();
	
					renderer->Get2dFgCamera()->SetOrthoProjection(0, 1, 0, 1, -10, 10 );
					hVec3D cent; cent << 0.5, 0.5, 0.0, 1.0f;
					
					std::vector< std::shared_ptr<Rendering::MeshNode> > progRects;
					Eigen::Vector4f rcol; rcol << 1.0, 0.0, 0.0, 1.0;
					progRects.resize( grabber->GetNumCameras() );
					float barHeight = 0.25 / progRects.size();
					transMatrix3D T = transMatrix3D::Identity();
					T(0,3) = -0.5;
					for( unsigned cc = 0; cc < progRects.size(); ++cc )
					{
						hVec3D pos; cent << 0.5, 0.25 + barHeight*cc, 0.0, 1.0f;
						std::stringstream ss;
						ss << "progRect-" << cc;
						progRects[cc] = GenerateRectNode(cent, 1, barHeight, 1, ss.str(), renderer );
						
						progRects[cc]->SetTransformation(T);
						progRects[cc]->SetTexture( renderer->GetBlankTexture() );
						progRects[cc]->SetBaseColour(rcol);
						renderer->Get2dFgRoot()->AddChild( progRects[cc] );
					}
					
					
					
					//
					// Get directory... to even things out on the disks, we'll
					// alternate which disk gets even or odd cameras.
					//
					
					fs::path outDir0, outDir1;
					
					outDir0 = fs::path(gtdata.saveRoot0) / fs::path(gtdata.window->GetSaveDirectory());
					outDir1 = fs::path(gtdata.saveRoot1) / fs::path(gtdata.window->GetSaveDirectory());

					if( gtdata.window->GetTrialNumber() % 2 == 0 )
					{
						tdata.outDir0 = outDir0.string();
						tdata.outDir1 = outDir1.string();
					}
					else
					{
						tdata.outDir0 = outDir1.string();
						tdata.outDir1 = outDir0.string();
					}
					
					
					
					unsigned numCams = tdata.rawBuffers.size();
					PrepSaveDirectories( {tdata.outDir0, tdata.outDir1}, numCams, gtdata );
					
					
					
					auto fnos = grabber->GetFrameNumbers();
					auto earliest = fnos[0];
					for( unsigned cc = 0; cc < fnos.size(); ++cc )
					{
						earliest = std::min( earliest, fnos[cc] );
					}
					tdata.startIdx = earliest % tdata.buffersNeeded;
					
					
					// tell the two save threads to get to work.
					
// 					cout << endl << endl << "even lock " << endl << endl << endl;
					std::unique_lock<std::mutex> evenlock( tdata.mutEven );
					
// 					cout << endl << endl << "odd lock " << endl << endl << endl;
					std::unique_lock<std::mutex> oddlock( tdata.mutOdd );
					tdata.savingOdd = true;
					tdata.savingEven = true;
					
					auto t1 = std::chrono::steady_clock::now();
					evenlock.unlock();
					oddlock.unlock();
					
// 					cout << endl << endl << "notify even " << endl << endl << endl;
					tdata.condVarEven.notify_one();
					
// 					cout << endl << endl << "notify odd " << endl << endl << endl;
					tdata.condVarOdd.notify_one();
					
					// wait for the two save threads,
					// and while we wait, update the progress bar.
					while( tdata.savingEven || tdata.savingOdd )
					{
						for( unsigned cc = 0; cc < tdata.saveProgress.size(); ++cc )
						{
							T(0,3) = -1.0 + tdata.saveProgress[cc];
							rcol << 1.0-tdata.saveProgress[cc], tdata.saveProgress[cc], 0.0, 1.0;
							progRects[cc]->SetTransformation(T);
							progRects[cc]->SetBaseColour(rcol);
						}
						renderer->Step( buffRecord, liveRecord );
					}
					
					
					buffRecord = false;
					liveRecord = false;
					
					// update the trial list from here so we dont need to wait for the window to close or poll
					// the grabber window from the gtk thread
					gdk_threads_add_idle(ControlsWindow::PopulateTrialList, gtdata.window);

					// update the trial list from here so we dont need to wait for the window to close or poll
					// the grabber window from the gtk thread
					gdk_threads_add_idle(ControlsWindow::PopulateTrialList, gtdata.window);

					auto t2 = std::chrono::steady_clock::now();
					std::stringstream clss;
					clss << outDir0.string()<< "/capTime.log";
					std::ofstream caplog( clss.str() );
					std::string trialName = gtdata.window->GetSaveDirectory();
					float capTime = gtdata.window->GetRecDuration();
					float saveTime = std::chrono::duration <double> (t2-t1).count();
					caplog << trialName << " " << capTime << " " << " " << saveTime << endl;
					caplog.close(); 					
					gdk_threads_add_idle(ControlsWindow::IncrementTrialNumber, gtdata.window);
					
					for( unsigned cc = 0; cc < tdata.saveProgress.size(); ++cc )
						progRects[cc]->RemoveFromParent();
					grabber->StartTrigger(0);
					//grabber->SetOutput1StateLow(0);
					grabber->SetOutput1StateHigh(0);
					tdata.saveProgress.assign( grabber->GetNumCameras(), 0.0f );
				}
				
				if( liveRecord )
				{
					new_liverecord = true;

					
					GrabThreadData &tdata = gtdata.window->gdata;
					buffRecord = false;
					
					fs::path outDir = fs::path(gtdata.saveRoot0) / fs::path(gtdata.window->GetSaveDirectory());
					
					unsigned numCams = tdata.rawBuffers.size();
					PrepSaveDirectories( {outDir.string()}, numCams, gtdata );
					gdtdata.outDir = outDir.c_str();

					auto fnos = grabber->GetFrameNumbers();
					auto earliest = fnos[0];
					for( unsigned cc = 0; cc < fnos.size(); ++cc )
					{
						earliest = std::min( earliest, fnos[cc] );
					}
					int bufIndx = earliest % tdata.buffersNeeded;
					std::vector<cv::Mat> grabs( tdata.rawBuffers.size() );
					for( unsigned cc = 0; cc < tdata.rawBuffers.size(); ++cc )
					{
						if( tdata.bufferFrameIdx[cc][ bufIndx ] == 0 )
							continue;
						grabs[cc] = tdata.rawBuffers[cc][ bufIndx ];
					}
					
					if( gtdata.window->IsLiveGridDetectEnabled() )
					{
						cout << "Calib mode!" << endl;
						int grows = gtdata.window->GetCalibGridRows();
						int gcols = gtdata.window->GetCalibGridCols();
						bool glightOnDark = gtdata.window->GetCalibGridLightOnDark();
						
						// is the grid detect thread available?
						if( gdtdata.inBGR.size() == 0 )
						{
							gdtdata.gridMutex.lock();
							
							
							
							gdtdata.inBGR.resize( grabs.size() );
							for( unsigned ic = 0; ic < grabs.size(); ++ic )
							{
								cv::cvtColor( grabs[ic], gdtdata.inBGR[ic], cv::COLOR_BayerGB2BGR_EA );
							}
							gdtdata.inGridRows = grows;
							gdtdata.inGridCols = gcols;
							gdtdata.inLightOnDark = glightOnDark;
							
							gdtdata.inCgDetectors = tdata.cgDetectors;
							
							gdtdata.outDir = outDir.c_str();
							
							gdtdata.gridMutex.unlock();
						}
						
						
						gdtdata.gridMutex.lock();
						// that should now do the grids in the background...
						CreateGridNodes(grows, gcols, numVis, renderer, tdata.grids );
						
						// we also want to know the sharing of grids between views.
						float sharesGridMax    = 25.0f;
						float sharesGridCurMax = 0.0f;
						sharesGrid = cv::Mat( numCameras, numCameras, CV_32FC1, cv::Scalar(0) );
						for( unsigned gc = 0; gc < tdata.grids[0].size(); ++gc )
						{
							for( unsigned cc0 = 0; cc0 < tdata.grids.size(); ++cc0 )
							{
								for( unsigned cc1 = cc0; cc1 < tdata.grids.size(); ++cc1 )
								{
									if( tdata.grids[cc0][gc].size() > 0 &&
									    tdata.grids[cc1][gc].size() > 0   )
									{
										float &f = sharesGrid.at<float>(cc0,cc1);
										f = std::min(f+1.0f, sharesGridMax);
										sharesGridCurMax = std::max( f, sharesGridCurMax );
									}
								}
							}
						}
						
						gdtdata.gridMutex.unlock();
						sharesGrid /= sharesGridCurMax;
						sharesGridRen->SetBGImage( sharesGrid );
						sharesGridRen->StepEventLoop();
					}
					else
					{
						for( unsigned cc = 0; cc < tdata.rawBuffers.size(); ++cc )
						{
							std::stringstream ss;
							ss << outDir.c_str() << "/" << std::setw(2) << std::setfill('0') << cc << "/"
							<< std::setw(12) << std::setfill('0') << tdata.bufferFrameIdx[cc][ bufIndx ] << ".charImg";
							SaveImage( grabs[cc], ss.str() );
						}
					} 
					
					T = transMatrix3D::Identity();
					T(0,3) = 0.05f;
					T(1,3) = 0.05f;
					liveRecCircle->SetTransformation(T);
					int v = fnos[0] % 10;
					float v2 = v / 30.0f;
					Eigen::Vector4f rcol; rcol << v2, 1.0-v2, 0.0, 0.8;
					liveRecCircle->SetBaseColour(rcol);
					
					
				}
				else
				{
					T = transMatrix3D::Identity();
					T(0,3) = -5.0f;
					T(1,3) = 0.5f;
					liveRecCircle->SetTransformation(T);
				}
				
			}
			
			if( gtdata.window->IsLiveGridDetectEnabled() )
			{
				cout << "wait for grid detect thread..." << endl;
				gdtdata.done = true;
				gridThread->join();
			}
			
			cout << "wait for odd/even save threads..." << endl;
			tdata.needSavingThreads = false;
			tdata.condVarEven.notify_one();
			tdata.condVarOdd.notify_one();
			saveEvenThread.join();
			saveOddThread.join();
			
			// clean up renderer
			cout << "done with current renderer" << endl;
			sendImgs[0] = cv::Mat(100, 100, CV_8UC1, cv::Scalar(128) );

			if (new_liverecord)
			{
				gdk_threads_add_idle(ControlsWindow::IncrementTrialNumber, gtdata.window);
				gdk_threads_add_idle(ControlsWindow::PopulateTrialList, gtdata.window);
				new_liverecord = false;
			}
		}
	}
	
	guiThread.join();
	
}



void PrepSaveDirectories( const std::vector<std::string> outDirs, unsigned numCameras, GUIThreadData gtdata )
{
	assert( outDirs.size() <= 2 );
	for( unsigned dc = 0; dc < outDirs.size(); ++dc )
	{
		std::string outDir = outDirs[dc];
		boost::filesystem::path p(outDir);
		if( !boost::filesystem::exists(p) )
		{
			boost::filesystem::create_directories(p);
		}
		else
		{
			if( !boost::filesystem::is_directory( p ))
			{
				cout << "Error! save location exists but is not a directory!" << endl;
				cout << "Saving to " << gtdata.saveRoot0 << "/rescued/" << endl;
				fs::path p = fs::path(gtdata.saveRoot0) / fs::path("rescued");
				boost::filesystem::create_directories(p);
			}
		}
	}
	
	for( unsigned cc = 0; cc < numCameras; ++cc )
	{
		std::string outDir;
		if( outDirs.size() == 2 )
		{
			outDir = outDirs[ cc%2 ];
		}
		else
		{
			outDir = outDirs[0];
		}
		std::stringstream ss;
		ss << std::setw(2) << std::setfill('0') << cc;
		fs::path p = fs::path(outDir) / fs::path(ss.str());
		std::string camDir = p.string();
		
		boost::filesystem::path cp(camDir);
		if( !boost::filesystem::exists(cp) )
		{
			boost::filesystem::create_directories(cp);
		}
	}
}


std::shared_ptr< Rendering::MeshNode > CreateGridNode( 
                                                       unsigned gridRows, 
                                                       unsigned gridCols,
                                                       std::shared_ptr<RecRenderer> ren,
                                                       std::vector< CircleGridDetector::GridPoint > &grid
                                                     )
{
// 	cout << "CGN: " << grid.size() << endl;
	// where are our corners?
	int p00, p01, p10, p11;
	p00 = p01 = p10 = p11 = -1;
	for( unsigned pc = 0; pc < grid.size(); ++pc )
	{
		if( grid[pc].row == 0          && grid[pc].col == 0          ) p00 = pc;
		if( grid[pc].row == 0          && grid[pc].col == gridCols-1 ) p01 = pc;
		if( grid[pc].row == gridRows-1 && grid[pc].col == 0          ) p10 = pc;
		if( grid[pc].row == gridRows-1 && grid[pc].col == gridCols-1 ) p11 = pc;
	}
	
	if( p00 >= 0 && p01 >= 0 && p10 >= 0 && p11 >= 0 )
	{
// 		cout << "CGN: " << grid.size() << " MESH" << endl;
		// create the mesh.
		std::shared_ptr<Rendering::Mesh> msh( new Rendering::Mesh(4,4,2) ); // 4 verts, 4 faces (lines)
		msh->vertices << grid[p00].pi(0), grid[p01].pi(0), grid[p11].pi(0), grid[p10].pi(0),
		                 grid[p00].pi(1), grid[p01].pi(1), grid[p11].pi(1), grid[p10].pi(1),
		                              -1,              -1,              -1,              -1,
		                               1,               1,               1,               1;
		
		msh->normals   << 0 ,  0,  0,  0,
		                  0 ,  0,  0,  0,
		                 -1 , -1, -1, -1,
		                  0 ,  0,  0,  0;
		
		msh->texCoords << 0, 1, 1, 0,
		                  0, 0, 1, 1;
		
		msh->faces << 0, 1, 2, 3, 
		              1, 2, 3, 0;
		
		msh->UploadToRenderer(ren);
		
		// create the node.
		std::shared_ptr<Rendering::MeshNode> mn;
		int id = rand();
		std::stringstream ss; ss << grid[p00].pi.transpose() << id; // should be unique enough.
		Rendering::NodeFactory::Create(mn, ss.str());
		
		mn->SetTexture( ren->GetBlankTexture() );
		mn->SetMesh( msh );
		mn->SetShader( ren->GetShaderProg("basicShader") );
		
		return mn;
		
	}
	else
	{
// 		throw std::runtime_error( "Got a grid with no corners. That shouldn't happen?" );
		// actually, it can happen, and doesn't really matter.
	}
}


void CreateGridNodes( 
                      unsigned grows, 
                      unsigned gcols,
                      unsigned numVis,
                      std::shared_ptr<RecRenderer> ren,
                      std::vector< std::vector< std::vector< CircleGridDetector::GridPoint > > > &grids
                    )  
{
	if( grids.size() == 0 )
		return;
	
	// technically all grids shouls be the same size, but I guess there's a chance of
	// them not being due to us doing things mid thread, so...
	int s = grids[0].size(); 
	for( unsigned gc = 1; gc < grids.size(); ++gc )
	{
		s = std::min( (int)grids[gc].size(), s );
	}
	if( numVis < s )
	{
		// we have grids to draw.
		for( unsigned gc = numVis; gc < s; ++gc )
		{
			unsigned maxDetections = 0;
			unsigned goodCount = 0;
			for( unsigned cc = 0; cc < grids.size(); ++cc )
			{
				if( grids[cc][gc].size() == grows * gcols || grids[cc][gc].size() == 4)
					++goodCount;
			}
			
			Eigen::Vector4f bc; 
			if( goodCount == 1 )
				bc << 0.6f, 0.1f, 0.1f, 1.0f;
			else if( goodCount == 2 )
				bc << 0.8f, 0.1f, 0.8f, 1.0f;
			else if( goodCount > 2 )
				bc << 0.1f, 0.9f, 0.1f, 1.0f;
			
			for( unsigned cc = 0; cc < grids.size(); ++cc )
			{
				if( grids[cc][gc].size() == grows * gcols || grids[cc][gc].size() == 4 )
				{
					auto n = CreateGridNode( grows, gcols, ren, grids[cc][gc] );
					n->SetBaseColour(bc);
					ren->AddImgChild(cc, n);
				}
			}
			++numVis;
		}
		assert( numVis == s ); 
	}
}


//
// We'll have two threads waiting for the command to save image 
// data to disk. One will handle the even cameras, the other will
// handle the odd cameras. We do this because we can then easily
// write one half(ish) of the data to one RAID array, and the other
// to the other array, helping to minimise write times.
//
void SaveEven(GrabThreadData *tdata)
{
	std::ofstream tfi("even.thread.log");
	
	tfi << "enter" << endl;
	while( tdata->needSavingThreads )
	{
		// wait for signal to start saving.
		tfi << "lock" << endl;
		std::unique_lock<std::mutex> lock( tdata->mutEven );
		while( !tdata->savingEven && tdata->needSavingThreads )
		{
			tfi << "wait" << endl;
			if( !tdata->needSavingThreads )
				continue;
			tdata->condVarEven.wait( lock );
		}
		if( !tdata->needSavingThreads )
			continue;
		
		tfi << "run" << endl;
		
#ifdef GRABBER_SAVE_AS_HDF5		
		for( unsigned cc = 0; cc < tdata->rawBuffers.size(); cc+=2 )
		{
			// open an hdf5 file for saving.
			std::stringstream ss;
			ss << tdata->outDir0 << "/" << std::setw(2) << std::setfill('0') << cc << "/"
		   	   << std::setw(12) << std::setfill('0') << tdata->bufferFrameIdx[cc][ tdata->startIdx ] 
		   	   << ".hdf5";
			HDF5ImageWriter writer( ss.str() );

			for( unsigned ic = 0; ic < tdata->buffersNeeded; ++ic )
			{
				int bufIndx = tdata->startIdx + ic;
				
				if( bufIndx >= tdata->buffersNeeded )
					bufIndx = bufIndx - tdata->buffersNeeded;
				
				writer.AddImage( tdata->rawBuffers[cc][ bufIndx ], tdata->bufferFrameIdx[cc][ bufIndx ] );
				
				tdata->saveProgress[cc] = ic / (float)tdata->buffersNeeded;
			}
			writer.Flush();
		}
		
#else
		for( unsigned cc = 0; cc < tdata->rawBuffers.size(); cc+=2 )
		{
			for( unsigned ic = 0; ic < tdata->buffersNeeded; ++ic )
			{
				int bufIndx = tdata->startIdx + ic;
				
				if( bufIndx >= tdata->buffersNeeded )
					bufIndx = bufIndx - tdata->buffersNeeded;
				std::stringstream ss;
				ss << tdata->outDir0 << "/" 
				   << std::setw(2) << std::setfill('0') << cc << "/"
				   << std::setw(12) << std::setfill('0') << tdata->bufferFrameIdx[cc][ bufIndx ] << ".charImg";
				
				SaveImage( tdata->rawBuffers[cc][ bufIndx ], ss.str() );
				
				tdata->saveProgress[cc] = ic / (float)tdata->buffersNeeded;
			}
		}
#endif

		
		tfi << "done" << endl;
		
		// signal done with saving
		tdata->savingEven = false;
		lock.unlock();
	}
	tfi << "exit" << endl;
}

void SaveOdd(GrabThreadData *tdata)
{
	std::ofstream tfi("odd.thread.log");
	
	tfi << "enter" << endl;
	while( tdata->needSavingThreads )
	{
		// wait for signal to start saving.
		tfi << "lock" << endl;
		std::unique_lock<std::mutex> lock( tdata->mutOdd );
		while( !tdata->savingOdd && tdata->needSavingThreads)
		{
			if( !tdata->needSavingThreads )
				continue;
			tfi << "wait" << endl;
			tdata->condVarOdd.wait( lock );
		}
		
		if( !tdata->needSavingThreads )
			continue;
		
		tfi << "run" << endl;
		
		
#ifdef GRABBER_SAVE_AS_HDF5
		for( unsigned cc = 1; cc < tdata->rawBuffers.size(); cc+=2 )
		{
			// open an hdf5 file for saving.
			std::stringstream ss;
			ss << tdata->outDir1 << "/" << std::setw(2) << std::setfill('0') << cc << "/"
			   << std::setw(12) << std::setfill('0') << tdata->bufferFrameIdx[cc][ tdata->startIdx ] 
			   << ".hdf5";
			
			HDF5ImageWriter writer( ss.str() );
			for( unsigned ic = 0; ic < tdata->buffersNeeded; ++ic )
			{
				int bufIndx = tdata->startIdx + ic;
				
				if( bufIndx >= tdata->buffersNeeded )
					bufIndx = bufIndx - tdata->buffersNeeded;
				
				writer.AddImage( tdata->rawBuffers[cc][ bufIndx ], tdata->bufferFrameIdx[cc][ bufIndx ] );
				
				tdata->saveProgress[cc] = ic / (float)tdata->buffersNeeded;
			}
			writer.Flush();
		}
		
#else
		for( unsigned cc = 1; cc < tdata->rawBuffers.size(); cc+=2 )
		{
			for( unsigned ic = 0; ic < tdata->buffersNeeded; ++ic )
			{
				int bufIndx = tdata->startIdx + ic;
				
				if( bufIndx >= tdata->buffersNeeded )
					bufIndx = bufIndx - tdata->buffersNeeded;
				std::stringstream ss;
				ss << tdata->outDir1 << "/" 
				   << std::setw(2) << std::setfill('0') << cc << "/"
				   << std::setw(12) << std::setfill('0') << tdata->bufferFrameIdx[cc][ bufIndx ] << ".charImg";
				
				SaveImage( tdata->rawBuffers[cc][ bufIndx ], ss.str() );
				
				tdata->saveProgress[cc] = ic / (float)tdata->buffersNeeded;
			}
		}
#endif
		
		tfi << "done" << endl;
		
		// signal done with saving
		tdata->savingOdd = false;
		lock.unlock();
	}
	tfi << "exit" << endl;
}



void GetSaveRoots( GUIThreadData &gtdata )
{
	std::string userHome;
#if defined(__APPLE__) || defined( __gnu_linux__ )
	struct passwd* pwd = getpwuid(getuid());
	if (pwd)
	{
		userHome = pwd->pw_dir;
	}
	else
	{
		// try the $HOME environment variable
		userHome = getenv("HOME");
	}
#else
	throw std::runtime_error("yeah, I've not done this for Windows or unknown unix!");
#endif
	
	std::stringstream ss;
	ss << userHome << "/.mc_dev.grabber.cfg";
	boost::filesystem::path p(ss.str());
	try
	{
		if( !boost::filesystem::exists(p) )
		{
			// create a default config file with conservative guesses.
			libconfig::Config cfg;
			auto &cfgRoot = cfg.getRoot();
			
			cfgRoot.add("saveRoot0", libconfig::Setting::TypeString);
			cfgRoot.add("saveRoot1", libconfig::Setting::TypeString);
			cfg.writeFile( ss.str().c_str() );
			cout << "The saveroots were not set. Please set them to absolute paths under " << fs::path(userHome) / fs::path(".mc_dev.grabber.cfg") << endl;
			exit(0);
		}
		
		libconfig::Config cfg;
		cfg.readFile( ss.str().c_str() );
		
		gtdata.saveRoot0 = (const char*) cfg.lookup("saveRoot0");
		gtdata.saveRoot1 = (const char*) cfg.lookup("saveRoot1");
		std::vector <string> saveroots = {gtdata.saveRoot0, gtdata.saveRoot1};
		for (unsigned i = 0; i < 2; i++)
		{
			fs::path sp(saveroots[i]);
			if (!fs::exists(sp) || !sp.is_absolute())
			{
				std::stringstream ss;
				ss << "error from reading .mc_dev.grabber.cfg: " << "\n" << sp << " is not a valid path"; 
				throw std::runtime_error(ss.str());
			}
		}	
	}
	catch( libconfig::SettingException &e)
	{
		cout << "Setting error: " << endl;
		cout << e.what() << endl;
		cout << e.getPath() << endl;
		exit(0);
	}
	catch( libconfig::ParseException &e )
	{
		cout << "Parse error:" << endl;
		cout << e.what() << endl;
		cout << e.getError() << endl;
		cout << e.getFile() << endl;
		cout << e.getLine() << endl;
		exit(0);
	}
	
	
	
}


#endif
