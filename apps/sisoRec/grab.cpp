#ifdef USE_SISO

#include "grab.h"


#include <thread>
#include <chrono>

void GrabThread( GrabThreadData *tdata )
{
	tdata->grabTime = std::chrono::steady_clock::now();
	tdata->grabber->StopTrigger(0);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	tdata->grabber->StartAcquisition(10, 0);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	tdata->grabber->StartTrigger(0);
	tdata->grabber->GetCurrentEnsureSynch(2);
	
	auto prevfnos = tdata->grabber->GetFrameNumbers();
	auto initfnos = prevfnos;
	std::vector< cv::Mat* > dsts;
	dsts.resize( tdata->rawBuffers.size() );
	frameindex_t prevfno = 0;
	int grabCount = 0;
	while( !tdata->done )
	{
		//
		// Grab...
		//
		frameindex_t fno = tdata->grabber->GetSyncFrame(2);
// 		cout << "sync frame: " << fno << endl;
		if( prevfno > 0 && (fno > prevfno + 1) )
		{
			cout << "warning, frame missed, relying on camera internal buffers" << endl;
			fno = prevfno + 1;
		}
		unsigned long bufIdx = fno % tdata->buffersNeeded;
		for( unsigned cc = 0; cc < tdata->rawBuffers.size(); ++cc )
		{
			dsts[cc] = &tdata->rawBuffers[cc][bufIdx];
			tdata->bufferFrameIdx[cc][bufIdx] = fno;
		}
		tdata->grabber->GetNumberedFrame( fno, 2, dsts );
		auto fnos = tdata->grabber->GetFrameNumbers();
		if( grabCount == 0 )
		{
			initfnos = fnos;
			tdata->totalGrabTime = std::chrono::steady_clock::now();;
		}
		
// 		for( unsigned cc = 0; cc < tdata->rawBuffers.size(); ++cc )
// 		{			
// 			cout << cc << " : " << fnos[cc] << "    " << prevfnos[cc] << " " << fnos[cc] - prevfnos[cc] << endl;
// 		}
		auto t1 = std::chrono::steady_clock::now();
		auto d = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - tdata->grabTime);
		
// 		cout << "grab time: " << d.count() << " ms ( " << 1000.0f / d.count() << " fps(ish) " << endl;
		tdata->grabTime = t1;
		tdata->lastGrabDuration = d;
// 		cout << "---" << endl;

		
		
		
		prevfnos = fnos;
		prevfno = fno;
		++grabCount;
	}

	auto t1 = std::chrono::steady_clock::now();	

	tdata->grabber->StopTrigger(0);
	tdata->grabber->StopAcquisition();
	

	auto d = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - tdata->totalGrabTime);
	
	cout << "grabCount : " << grabCount << endl;
	cout << "camera fnos at start and end of grabbing: " << endl;
	for( unsigned cc = 0; cc < initfnos.size(); ++cc )
	{
		cout << cc << " : " << initfnos[cc] << "     " << prevfnos[cc] << "      diff: " << prevfnos[cc] - initfnos[cc];
		cout << " -> " << (prevfnos[cc] - initfnos[cc]) / (d.count()/1000.0f) << endl;
	}
	
	return;
}

#endif
