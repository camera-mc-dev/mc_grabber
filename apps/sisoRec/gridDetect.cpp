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
	//TODO: load from common config or grabber config
	std::string netPath = "/data2/gridDet/";
	std::string netName = "cgd-bxs03-bst";
	auto ctx = mx::Context::gpu(0);
	cout << "bs: " << data->numCameras << endl;
	CalibGridDetectNet cgdNet( netPath, netName, data->numCameras, ctx );
	cv::Mat tmpf;
	std::ofstream cgdbg("cgdDebug");
	int dbgImCnt = 0;
#endif
	std::shared_ptr<CircleGridDetector> cgDetector;
	
	while( !data->done )
	{
		// lock data lock
		data->gridMutex.lock();
		
		// get images to process
		imgs.resize( data->inBGR.size() );
		for( unsigned ic = 0; ic < imgs.size(); ++ic )
		{
			imgs[ic] = data->inBGR[ic].clone();
		}
		grows = data->inGridRows;
		gcols = data->inGridCols;
		lightOnDark = data->inLightOnDark;
		cgDetector  = data->inCgDetector;
		
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
		// If we've got one of our grid detector networks, then we can at least try it.
		// we probably don't expect it to find the grid circles well enough for calibration,
		// but at the very least, we can get a good idea of whether the grid is visible, and 
		// where it is in the image.
		auto t0 = std::chrono::steady_clock::now();
		std::vector< cv::Mat > smallImgs( imgs.size() );
		for( unsigned ic = 0; ic < imgs.size(); ++ic )
		{
			cv::Mat rawf, input;
			imgs[ic].convertTo( tmpf, CV_32FC3 );
			tmpf /= 255.0f;
			cv::resize( tmpf, smallImgs[ic], cv::Size( 480, 270 ) );
		}
		
		std::vector< std::vector<hVec2D> > detections( imgs.size() );
		cgdNet.Detect( smallImgs, detections );
		auto t1 = std::chrono::steady_clock::now();
		cgdbg << imgs.size() << endl;
		cgdbg << "cgd time: " << std::chrono::duration <double, std::milli> (t1-t0).count() << endl;
		for( unsigned ic = 0; ic < detections.size(); ++ic )
		{
			cgdbg << "cgdNet -- " << ic << " --" << endl;
			for( unsigned pc = 0; pc < detections[ic].size(); ++pc )
			{
				cgdbg << detections[ic][pc].transpose() << endl;
				float v = detections[ic][pc](2);
				cv::circle( imgs[0], cv::Point( detections[ic][pc](0) * imgs[0].cols, detections[ic][pc](1) * imgs[0].rows ), 7, cv::Scalar(0,v*255,255 - v*255), 3 );
				cv::circle( smallImgs[0], cv::Point( detections[ic][pc](0) * smallImgs[0].cols, detections[ic][pc](1) * smallImgs[0].rows ), 7, cv::Scalar(0,v,1-v), 3 );
			}
		}
		
		
		std::stringstream ss;
		ss << "/data2/gridDet/liveTst/full/" << std::setw(8) << std::setfill('0') << dbgImCnt << ".jpg";
		SaveImage( imgs[0], ss.str() );
		
		ss.str("");
		ss << "/data2/gridDet/liveTst/small/" << std::setw(8) << std::setfill('0') << dbgImCnt << ".jpg";
		SaveImage( smallImgs[0], ss.str() );
		++dbgImCnt;
		
		
#else
		for( unsigned cc = 0; cc < imgs.size(); ++cc )
		{	
			cgDetector->FindGrid( imgs[cc], grows, gcols, true, false, gridPoints[cc] );
			maxDetections = std::max( maxDetections, (unsigned)gridPoints[cc].size() );
			if( gridPoints[cc].size() == grows * gcols )
				++goodCount;
		}
#endif
		
		auto r = data->renderer.lock();
		// have we got grids?
		if( maxDetections == grows * gcols )
		{
			for( unsigned cc = 0; cc < imgs.size(); ++cc )
			{
				data->grids->at(cc).push_back( gridPoints[cc] );
				
				
				std::stringstream ss;
				ss << data->outDir << "/" << std::setw(2) << std::setfill('0') << cc << "/"
				<< std::setw(12) << std::setfill('0') << *data->gridNo << ".charImg";
				SaveImage( imgs[cc], ss.str() );
			}
			
			++(*data->gridNo );
		}
		
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
