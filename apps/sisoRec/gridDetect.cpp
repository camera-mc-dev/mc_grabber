#ifdef USE_SISO

#include "controls.h"
#include "grab.h"
#include "gridDetect.h"
#include <chrono>

void GridDetectThread( SGridDetectData *data )
{
	std::vector< cv::Mat > imgs;
	int grows, gcols;
	bool lightOnDark;
	
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
		std::vector< std::vector< CircleGridDetector::GridPoint > > gridPoints( imgs.size() );
		unsigned maxDetections = 0;
		unsigned goodCount = 0;
		for( unsigned cc = 0; cc < imgs.size(); ++cc )
		{	
			cgDetector->FindGrid( imgs[cc], grows, gcols, true, false, gridPoints[cc] );
			maxDetections = std::max( maxDetections, (unsigned)gridPoints[cc].size() );
			if( gridPoints[cc].size() == grows * gcols )
				++goodCount;
		}
		
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
