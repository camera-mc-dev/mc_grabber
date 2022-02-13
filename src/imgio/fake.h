//
// Created by reuben on 2022-01-21.
//

#ifndef MC_GRABBER_FAKE_H
#define MC_GRABBER_FAKE_H

//
//
// Framework includes
//
#include "grabber.h"
#include "imgio/sourceFactory.h"

//
// System includes
//
#include <vector>
#include <iostream>
#include <string>
#include <chrono>
using std::cout;
using std::endl;

//
// Want a fake camera s.th we can launch it in a thread. Pausing the grabber thread seems to cause issues with GTK.
//
class FakeCamera
{
public:

	FakeCamera(std::string pathToSource);

	int GetCurrentFrameIdx()
	{
		return currentFrameIdx;
	}

	Mat GetCurrentFrame();

	// issues with threading. having a thread for each camera seems a little too unreliable.
	// This function is called from the chrono thread. 

	// I've now said the word thread too many times...

	void Advance();


protected:
	Mat currentFrame;
	int currentFrameIdx;
	int fps = 100;
	bool kill = false;
	SourcePair source_pair;

};

class FakeGrabber: public AbstractGrabber 
{
public:
	FakeGrabber(string pathToSource);

	~FakeGrabber()
	{
		done = true;
		chronoThread.join();
	}

	void PrintCameraInfo() override 
	{
		for (unsigned j = 0; j < GetNumCameras(); j++)
        {
            cout << cameras[j].GetCurrentFrameIdx() << endl;
        }
    
	}
	
	unsigned GetNumCameras()
	{
	    // always just return 4 for now.
	    return 4;
	}
	
	std::vector< Mat > currentFrames;
	
	void GetCurrent();
	
	bool GetNumberedFrame( frameindex_t frameIdx, int timeout, std::vector< Mat* > dsts );
	
	frameindex_t GetSyncFrame( int timeout );
	
	std::vector< frameindex_t > GetFrameNumbers();
	
	//
	// overrides
	//
	void SetExposure( long int exposure ) override
	{
		// we have no physical camera, so no exposure to set.
	};
	
	void SetExposure( int cam, long int exposure )override
	{
		// we have no physical camera, so no exposure to set.
	};
	
	
	void SetResolution( long int cols, long int rows ) override
	{
		// we have no physical camera, but we have a single image source 
		// faked to be some number of cameras, and we can thus 
		// scale that source to some desired image size.
		desiredCols = cols;
		desiredRows = rows;
	}
	
	void SetResolution( int cam, long int cols, long int rows ) override
	{
		desiredCols = cols;
		desiredRows = rows;
	}
	
	void SetFPS( int in_FPS, int masterBoard )override
	{
		fps = in_FPS;
	}
	
	void StartAcquisition( int bufferFrames, int masterBoard ) override 
	{
		// Calls this here so we dont need to change too many function calls in the mainfiles. 
		StartAcquisition();
	}
	
	// 
	// Starts the chronothread which runs the Advance() method for FakeCameras.
	//
	void StartAcquisition();
	void StopAcquisition() override 
	{
		// ending chronoThread handled by destructor so dont do anything here.
	}
	
	void StartTrigger( int masterBoard ) override
	{
		// nothing to do unless we go down the route of having a seperate grabbing thread.
	}
	
	void StopTrigger( int masterBoard ) override
	{
		// nothing to do unless we go down the route of having a seperate grabbing thread.
	}
	
	bool GetCurrentEnsureSynch( int timeout ) override
	{
		// not currently used.
		return true;
	}
	
	bool GetNumberedFrame( frameindex_t frameIdx, int timeout ) override
	{
		// not currently used.
		return true;
	}
	
	
	void SetOutput1StateHigh( int board ) override
	{
		// nothing to do.
	}
	
	void SetOutput1StateLow( int board ) override
	{
		// nothing to do.
	}
	
	void GetResolution( int cam, long int &cols, long int &rows )
	{
		cols = desiredCols;
		rows = desiredRows;
	}
	
	void SetGain( double gain ) override
	{
		// nothing to do.
	}
	
	void SetGain( int cam, double gain ) override
	{
		// nothing to do.
	}
	
	void SetBaseGain( baseGain_t value ) override
	{
		// nothing to do.
	}
	
	void SetBaseGain( int cam, baseGain_t value ) override
	{
		// nothing to do
	}
	
private:
	std::vector <SourcePair> source_pairs;
	std::vector< frameindex_t > camFrames;
	std::vector<FakeCamera> cameras;
	std::thread chronoThread;
	long int desiredRows, desiredCols;
	int fps = 24;
	bool done = false;
};

#endif //MC_GRABBER_FAKE_H
