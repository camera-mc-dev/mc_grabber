#ifdef USE_SISO
#ifndef SISO_REC_GRAB
#define SISO_REC_GRAB

#include <vector>
#include <chrono>

#include "imgio/siso.h"
#include "imgio/loadsave.h"

#include "calib/circleGridTools.h"

struct GrabThreadData
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

	// useful to know timing info
	std::chrono::time_point<std::chrono::steady_clock> grabTime;
	std::chrono::time_point<std::chrono::steady_clock> totalGrabTime;
	std::chrono::milliseconds lastGrabDuration;
	float meanfps;
	
	// I'm not entirely happy with this being here, semantically
	// it doesn't really suit. But it is accessable from both places that need it.
	std::shared_ptr<CircleGridDetector> cgDetector;
	
	// for the current calibration session
	// number of the next grab
	unsigned calibGrabNo;
	
	// per-camera, per-grab, set of grid points
	std::vector< std::vector< std::vector< CircleGridDetector::GridPoint > > > grids;
	
	// for the image saving threads
	int startIdx;
	bool needSavingThreads;
	std::vector<float> saveProgress;
	std::condition_variable condVarOdd, condVarEven;
	std::mutex mutOdd, mutEven;
	bool savingOdd, savingEven;
	std::string outDir0, outDir1;
};
void GrabThread( GrabThreadData *tdata );

#endif
#endif
