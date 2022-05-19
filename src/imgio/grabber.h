//
// Created by reuben on 2022-01-20.
//

#ifndef MC_GRABBER_GRABBER_H
#define MC_GRABBER_GRABBER_H

// include opencv here for typedefs
#include <cv.hpp>
using namespace cv;

typedef long int frameindex_t;


//
// I don't see anything in the SDK to ask about what grabbers
// are available. So the user will have to tell us about them.
// Probably would have to do that anyway...
//
struct SiSoBoardInfo
{
	int boardIndx;
	std::string grabberConfig;
};

class AbstractGrabber 
{

public:
	enum baseGain_t {baseGain00, baseGain06, baseGain12};
	
	virtual void PrintCameraInfo() = 0;
	virtual void StartAcquisition( int bufferFrames, int masterBoard ) = 0;
	virtual void StopAcquisition() = 0;
	virtual void StartTrigger( int masterBoard ) = 0;
	virtual void StopTrigger( int masterBoard ) = 0;
	virtual bool GetCurrentEnsureSynch( int timeout ) = 0;
	virtual frameindex_t GetSyncFrame( int timeout ) = 0;
	virtual bool GetNumberedFrame( frameindex_t frameIdx, int timeout ) = 0;
	virtual bool GetNumberedFrame( frameindex_t frameIdx, int timeout, std::vector< Mat* > dsts ) = 0;
	virtual std::vector< frameindex_t > GetFrameNumbers() = 0;
	virtual void SetOutput1StateHigh( int board ) = 0;
	virtual void SetOutput1StateLow( int board ) = 0;
	virtual unsigned GetNumCameras() = 0;
	virtual void SetFPS( int in_FPS, int masterBoard ) = 0;
	virtual void SetResolution( long int cols, long int rows ) = 0;
	virtual void SetResolution( int cam, long int cols, long int rows ) = 0;
	virtual void GetResolution( int cam, long int &cols, long int &rows ) = 0;
	
	virtual void SetExposure( long int exposure ) = 0;
	virtual void SetExposure( int cam, long int exposure ) = 0;
	
	virtual void SetGain( double gain ) = 0;
	virtual void SetGain( int cam, double gain ) = 0;
	virtual void SetBaseGain( baseGain_t value ) = 0;
	virtual void SetBaseGain( int cam, baseGain_t value ) = 0 ;
	
	bool fake = false;
	bool save_as_hdf5 = false;
};


#endif //MC_GRABBER_GRABBER_H
