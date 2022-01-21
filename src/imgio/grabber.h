//
// Created by reuben on 2022-01-20.
//

#ifndef MC_GRABBER_GRABBER_H
#define MC_GRABBER_GRABBER_H

//
// Silicon Software SDK includes
//
#include <fgrab_struct.h>
#include <fgrab_prototyp.h>
#include <fgrab_define.h>
#include <siso_genicam.h>

//
// Framework includes
//
#include "misc/types.h"
//#include "grabber.h"
#include <cv.hpp>


//
// System includes
//
#include <vector>
#include <iostream>
#include <string>
#include <chrono>
using std::cout;
using std::endl;



class AbstractGrabber {
public:

    enum baseGain_t {baseGain00, baseGain06, baseGain12};

    // Initialise libraries etc...
    // SiSoGrabber( std::vector<SiSoBoardInfo> boardInfo );

    // Obvious really: Prints info about the discovered cameras.
    virtual void PrintCameraInfo();

    // Sets the resolution for all cameras or individual cameras.
    virtual void SetResolution( long int cols, long int rows );
    virtual void SetResolution( int cam, long int cols, long int rows );

    virtual void GetResolution( int cam, long int &cols, long int &rows );

    // Sets the exposure (for timed exposure ), in microseconds
    virtual void SetExposure( long int exposure );
    virtual void SetExposure( int cam, long int exposure );

    // Sets the digital gain
    virtual void SetGain( double gain );
    virtual void SetGain( int cam, double gain );

    // Sets the analog base gain
    virtual void SetBaseGain( baseGain_t value );
    virtual void SetBaseGain( int cam, baseGain_t value );


    // Sets the desired framerate
    virtual void SetFPS( int in_FPS, int masterBoard );


    // Start acquisition on all cameras
    // must specify the buffer size in terms of number of seconds.
    // Function will determine number of bytes based on current image size,
    // frame rate and format.
    virtual void StartAcquisition( int bufferFrames, int masterBoard );
    virtual void StopAcquisition();

    // Start or stop the trigger
    // If the trigger is not running, images will not be captured.
    // Needs to know which board is the master
    virtual void StartTrigger( int masterBoard );
    virtual void StopTrigger( int masterBoard );

    // As well as the pulse trigger (output 0 of the master frame grabber)
    // we can manaully change the state of output 1 of the master frame grabber.
    // Useful if we want to send a start or stop signal externally.
    virtual void SetOutput1StateHigh( int board );
    virtual void SetOutput1StateLow( int board );


    // Get images
    // The first approach is a live-view approach, where we just grab the
    // most recent images from the cameras. This is not guaranteed to be
    // synchronised.
    // std::vector< cv::Mat > currentFrames;
    virtual void GetCurrent( int timeout );

    // The next approach is very simillar, but puts some effort in to
    // make sure that the current frames all have the same frame number,
    // and thus, should all be synchronised.
    virtual bool GetCurrentEnsureSynch( int timeout );

    // This is like GetCurrentEnsureSynch, but it only works out what the most recetly available
    // frame number is from all cameras.
    virtual frameindex_t GetSyncFrame( int timeout );

    // Next up we have the option of asking for a specific frame number.
    // This will be primarily useful when you are recording to RAM
    virtual bool GetNumberedFrame( frameindex_t frameIdx, int timeout );

    // This is the same as above, but we provide a place to put the grabbed frames
    // instead of putting them into currentFrames.
    // This might save a bunch of big copies if we are recording.
    virtual bool GetNumberedFrame( frameindex_t frameIdx, int timeout, std::vector< cv::Mat* > dsts );


    //
    // Not sure how to do this safely, but a useful function to have as a one off in some code.
    // Resets the cameras by power-cycling them.
    //
    virtual void PowerCycle();


    // It might be useful to the user to know the frame number for each camera.
    std::vector< frameindex_t > GetFrameNumbers()
    {
        return camFrames;
    }

    virtual unsigned GetNumCameras()
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


};


#endif //MC_GRABBER_GRABBER_H
