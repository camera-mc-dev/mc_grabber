#ifdef USE_SISO

#ifndef SISO_GRABBER
#define SISO_GRABBER

//
// Silicon Software SDK includes
//
#include <fgrab_struct.h>
#include <fgrab_prototyp.h>
#include <fgrab_define.h>
#include <siso_genicam.h>

//
//
// Framework includes
//
#include "grabber.h"
#include "misc/types.h"
#include <opencv2/opencv.hpp>

//
// System includes
//
#include <vector>
#include <iostream>
#include <string>
#include <chrono>
using std::cout;
using std::endl;

class SiSoGrabber: public AbstractGrabber
{
public:
	
	
	// Initialise libraries etc...
	SiSoGrabber( std::vector<SiSoBoardInfo> boardInfo );

	// Obvious really: Prints info about the discovered cameras.
	void PrintCameraInfo();

	// Sets the resolution for all cameras or individual cameras.
	void SetResolution( long int cols, long int rows );
	void SetResolution( int cam, long int cols, long int rows );
	
	void GetResolution( int cam, long int &cols, long int &rows );
	
	// Sets the exposure (for timed exposure ), in microseconds
	void SetExposure( long int exposure );
	void SetExposure( int cam, long int exposure );
	
	// Sets the digital gain
	void SetGain( double gain );
	void SetGain( int cam, double gain );
	
	// Sets the analog base gain
	void SetBaseGain( baseGain_t value ) override;
	void SetBaseGain( int cam, baseGain_t value ) override;
	

	// Sets the desired framerate
	void SetFPS( int in_FPS, int masterBoard );
	float GetFPS();


	// Start acquisition on all cameras
	// must specify the buffer size in terms of number of seconds.
	// Function will determine number of bytes based on current image size,
	// frame rate and format.
	void StartAcquisition( int bufferFrames, int masterBoard );
	void StopAcquisition();

	// Start or stop the trigger
	// If the trigger is not running, images will not be captured.
	// Needs to know which board is the master
	void StartTrigger( int masterBoard );
	void StopTrigger( int masterBoard );
	
	// As well as the pulse trigger (output 0 of the master frame grabber)
	// we can manaully change the state of output 1 of the master frame grabber.
	// Useful if we want to send a start or stop signal externally.
	void SetOutput1StateHigh( int board );
	void SetOutput1StateLow( int board );
	
	
	// Get images
	// The first approach is a live-view approach, where we just grab the 
	// most recent images from the cameras. This is not guaranteed to be
	// synchronised.
	std::vector< cv::Mat > currentFrames;
	void GetCurrent( int timeout );
	
	// The next approach is very simillar, but puts some effort in to
	// make sure that the current frames all have the same frame number,
	// and thus, should all be synchronised.
	bool GetCurrentEnsureSynch( int timeout );
	
	// This is like GetCurrentEnsureSynch, but it only works out what the most recetly available
	// frame number is from all cameras.
	frameindex_t GetSyncFrame( int timeout );
	
	// Next up we have the option of asking for a specific frame number.
	// This will be primarily useful when you are recording to RAM
	bool GetNumberedFrame( frameindex_t frameIdx, int timeout );
	
	// This is the same as above, but we provide a place to put the grabbed frames
	// instead of putting them into currentFrames.
	// This might save a bunch of big copies if we are recording.
	bool GetNumberedFrame( frameindex_t frameIdx, int timeout, std::vector< cv::Mat* > dsts );
	

	//
	// Not sure how to do this safely, but a useful function to have as a one off in some code.
	// Resets the cameras by power-cycling them.
	//
	void PowerCycle();
	
	
	// It might be useful to the user to know the frame number for each camera.
	std::vector< frameindex_t > GetFrameNumbers()
	{
		return camFrames;
	}
	
	unsigned GetNumCameras()
	{
		cout << camInfos.size() << endl;
		return camInfos.size();
	}
	
	// only valid after start of acquisition
	int GetNumFramesInBuffers()
	{
		return numImagesInBuffers;
	}
protected:

	std::vector< Fg_Struct* > fgHandles;
	std::vector< SgcBoardHandle* > boardHandles;
	std::vector< SgcCameraHandle* > camHandles;
	std::vector< SgcCameraInfo* > camInfos;
	std::vector< std::pair<int,int> > camAddrs;
	std::vector< dma_mem* > memHandles;
	std::vector< frameindex_t > camFrames;
	
	int numImagesInBuffers;
	
	int numFramesInCurrentRun;
	std::chrono::steady_clock::time_point runStart;
	
	void CleanUp();
	
	double fps; // as last set - not pulled from SISO API!
};




#endif
#endif
