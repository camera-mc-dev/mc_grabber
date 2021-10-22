#ifdef USE_SISO
#ifndef SISO_REC_GRID
#define SISO_REC_GRID

#include "renderer.h"

struct SGridDetectData
{
	std::mutex gridMutex;
	
	std::vector< cv::Mat > inBGR;
	int inGridRows, inGridCols;
	bool inLightOnDark;
	
	std::weak_ptr<RecRenderer> renderer;
	
	std::shared_ptr<CircleGridDetector> inCgDetector;
	
	// pointer to where we keep track of grid numbers, which are variables 
	// in the grabThreadData structure.
	unsigned *gridNo; 
	std::vector< std::vector< std::vector< CircleGridDetector::GridPoint > > > *grids;
	
	std::string outDir;
	
	bool done;
};
void GridDetectThread( SGridDetectData *data );





#endif
#endif
