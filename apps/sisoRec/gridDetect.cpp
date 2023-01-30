#ifdef USE_SISO

#include "controls.h"
#include "grab.h"
#include "gridDetect.h"

#ifdef HAVE_MC_NETS
#include "deepNets/calibGrid/calibGrid.h"
#include <chrono>
#endif


#include <chrono>

void GridDetectThread( SGridDetectData *data )
{
	std::vector< cv::Mat > imgs;
	int grows, gcols;
	bool lightOnDark;
	
	
#ifdef HAVE_MC_NETS
	CommonConfig ccfg;
	
	std::string netPath = ccfg.netsRoot + "/gridDetNets/";
	std::string netName = "cgd-hm-pts-1";
	auto ctx = mx::Context::gpu(0);
	cout << "bs: " << data->numCameras << endl;
	CalibGridDetectNet cgdNet( netPath, netName, data->numCameras, ctx );
	cv::Mat tmpf;
	std::ofstream cgdbg("cgdDebug");
	int dbgImCnt = 0;
#endif
	std::vector< std::shared_ptr<CircleGridDetector> > cgDetectors;
	
	while( !data->done )
	{
		auto t0 = std::chrono::steady_clock::now();
		// lock data lock
		data->gridMutex.lock();
		
		auto t1 = std::chrono::steady_clock::now();
		
		// get images to process
		imgs.resize( data->inBGR.size() );
		for( unsigned ic = 0; ic < imgs.size(); ++ic )
		{
			imgs[ic] = data->inBGR[ic].clone();
		}
		grows = data->inGridRows;
		gcols = data->inGridCols;
		lightOnDark = data->inLightOnDark;
		cgDetectors  = data->inCgDetectors;
		
		// unlock data lock
		data->gridMutex.unlock();
		
		if( imgs.size() == 0 )
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}
		
		// try to get grids
		unsigned maxDetections = 0;
		unsigned goodCount = 0;
		std::vector< std::vector< CircleGridDetector::GridPoint > > gridPoints( imgs.size() );
#ifdef HAVE_MC_NETS
		
		// At the moment, the circleGridNet is only accurate enough to give us a detection and 
		// approximate location of the outside corners of the grid. But that's not a bad thing,
		// because we can still make use of that info to massively reduce our normal use of 
		// the circle grid detector.
		
		auto t2 = std::chrono::steady_clock::now();
		
		//
		// First off, get small images to feed the network.
		// Right now, the network is expecting images of 480x270
		//
		std::vector< cv::Mat > smallImgs( imgs.size() );
		for( unsigned ic = 0; ic < imgs.size(); ++ic )
		{
			cv::Mat rawf, input;
			imgs[ic].convertTo( tmpf, CV_32FC3 );
			tmpf /= 255.0f;
			cv::resize( tmpf, smallImgs[ic], cv::Size( 480, 270 ) );
		}
		
		
		//
		// Next we can run the detector on those images.
		//
		std::vector< std::vector<hVec2D> > detections( imgs.size() );
		std::vector< std::vector< cv::Mat > > hmaps;
		cgdNet.Detect( smallImgs, detections, hmaps );
		
		auto t3 = std::chrono::steady_clock::now();
		
		cgdbg << imgs.size() << endl;
		cgdbg << "cgd time: " << std::chrono::duration <double, std::milli> (t3-t2).count() << endl;
		
		
		//
		// Having done that, we now want to run the normal circle grid detector
		// but only on the images that had a grid detection, and only over the 
		// small region of the image where the grid was detected. We do one more
		// speed/accuracy trade off and resize the grid detection window to half 
		// its original size, just because the accuracy cost is small enough (probably)
		//
		#pragma omp parallel for
		for( unsigned ic = 0; ic < detections.size(); ++ic )
		{
			cv::Mat &raw = imgs[ic];
			
			// is there a grid, and how big is it in the image?
			hVec2D m, M;
			m << raw.cols, raw.rows, 1.0f;
			M << 0, 0, 1.0f;
			int got = 0;
			for( unsigned pc = 0; pc < detections[ic].size(); ++pc )
			{
				if( detections[ic][pc](2) > 0.2 )
				{
					m(0) = std::min( m(0), detections[ic][pc](0) * raw.cols );
					m(1) = std::min( m(1), detections[ic][pc](1) * raw.rows );
					
					M(0) = std::max( M(0), detections[ic][pc](0) * raw.cols );
					M(1) = std::max( M(1), detections[ic][pc](1) * raw.rows );
					
					++got;
				}
			}
			
			
			cv::Rect win;
			if( got > 3 )
			{
				// make our grid detection a bit bigger.
				int xmod = 0.1* (M(0)-m(0));
				m(0) = std::max((int)m(0)-xmod, 0);
				M(0) = std::min((int)M(0)+xmod, raw.cols-1);
				int ymod = 0.1* (M(1)-m(1));
				m(1) = std::max((int)m(1)-ymod, 0);
				M(1) = std::min((int)M(1)+ymod, raw.rows-1);
				
				// get greyscale version of image for MSER grid detect.
				cv::Mat grey, greyW;
				cv::cvtColor(  raw,  grey, cv::COLOR_BGR2GRAY );
				
				// get the window.
				win = cv::Rect( m(0), m(1), M(0)-m(0), M(1)-m(1) );
				greyW = grey( win ).clone(); 
				
				// is the window still quite ... big?
				bool halved = false;
				//if( win.width > 0.25 * raw.cols )
				{
					cv::resize( greyW, greyW, cv::Size( ), 0.5, 0.5 );
					halved = true;
				}
				
				// run the normal grid detector on that window
				cgDetectors[ic]->FindGrid( greyW, 9, 10, false, false, gridPoints[ic] );
				
				// adjust the grid points for our window.
				for( unsigned gpc = 0; gpc < gridPoints[ic].size(); ++gpc )
				{
					if( halved )
					{
						gridPoints[ic][gpc].pi(0) *= 2.0f;
						gridPoints[ic][gpc].pi(1) *= 2.0f;
					}
					
					gridPoints[ic][gpc].pi(0) += int(m(0));
					gridPoints[ic][gpc].pi(1) += int(m(1));
				}
				maxDetections = std::max( maxDetections, (unsigned)gridPoints[ic].size() );
				
				cgdbg << "cgdNet -- " << ic << " --" << endl;
				cgdbg << "\t" << got << " : " << gridPoints[ic].size() << endl;
			}
		}
		
		
		auto t4 = std::chrono::steady_clock::now();
		
		
		
		auto r = data->renderer.lock();
		// have we got grids?
		if( maxDetections >= 4 )
		{
			// lock data lock
			data->gridMutex.lock();
			
			for( unsigned cc = 0; cc < imgs.size(); ++cc )
			{
				data->grids->at(cc).push_back( gridPoints[cc] );
				
				
				std::stringstream ss;
				ss << data->outDir << "/" << std::setw(2) << std::setfill('0') << cc << "/"
				<< std::setw(12) << std::setfill('0') << *data->gridNo << ".charImg";
				SaveImage( imgs[cc], ss.str() );
			}
			
			++(*data->gridNo );
			
			// unlock data lock
			data->gridMutex.unlock();
			
			auto t5 = std::chrono::steady_clock::now();
			
			cgdbg << "cgd time 0 -> 1: " << std::chrono::duration <double, std::milli> (t1-t0).count() << endl;
			cgdbg << "cgd time 1 -> 2: " << std::chrono::duration <double, std::milli> (t2-t1).count() << endl;
			cgdbg << "cgd time 2 -> 3: " << std::chrono::duration <double, std::milli> (t3-t2).count() << endl;
			cgdbg << "cgd time 3 -> 4: " << std::chrono::duration <double, std::milli> (t4-t3).count() << endl;
			cgdbg << "cgd time 4 -> 5: " << std::chrono::duration <double, std::milli> (t5-t4).count() << endl;
			cgdbg << " ==== " << endl << endl;
		}
		
		
#else
		for( unsigned cc = 0; cc < imgs.size(); ++cc )
		{	
			cgDetectors[cc]->FindGrid( imgs[cc], grows, gcols, true, false, gridPoints[cc] );
			maxDetections = std::max( maxDetections, (unsigned)gridPoints[cc].size() );
			if( gridPoints[cc].size() == grows * gcols )
				++goodCount;
		}
		
		auto r = data->renderer.lock();
		// have we got grids?
		if( maxDetections == grows * gcols )
		{
			// lock data lock
			data->gridMutex.lock();
			
			for( unsigned cc = 0; cc < imgs.size(); ++cc )
			{
				data->grids->at(cc).push_back( gridPoints[cc] );
				
				
				std::stringstream ss;
				ss << data->outDir << "/" << std::setw(2) << std::setfill('0') << cc << "/"
				<< std::setw(12) << std::setfill('0') << *data->gridNo << ".jpg";
                
                cout << "Saving (?): " << ss.str() << endl;
				SaveImage( imgs[cc], ss.str() );
			}
			
			++(*data->gridNo );
			
			// unlock data lock
			data->gridMutex.unlock();
		}
#endif
		
		
		
		// lock data lock
		data->gridMutex.lock();
		
		// empty data source to say "I'm ready!"
		data->inBGR.clear();
		
		// unlock data lock
		data->gridMutex.unlock();
	}
	cout << "done with current grid detector" << endl;
}






#endif
