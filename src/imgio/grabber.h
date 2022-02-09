//
// Created by reuben on 2022-01-20.
//

#ifndef MC_GRABBER_GRABBER_H
#define MC_GRABBER_GRABBER_H

// include opencv here for typedefs
#include <cv.hpp>
using namespace cv;

typedef long int frameindex_t;
typedef long int baseGain_t;

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
    virtual void PrintCameraInfo(){};
    virtual void StartAcquisition( int bufferFrames, int masterBoard ) {};
    virtual void StopAcquisition() {};
    virtual void StartTrigger( int masterBoard ) {};
    virtual void StopTrigger( int masterBoard ) {};
    virtual bool GetCurrentEnsureSynch( int timeout ) = 0;
    virtual frameindex_t GetSyncFrame( int timeout ) {};
    virtual bool GetNumberedFrame( frameindex_t frameIdx, int timeout ) = 0;
    virtual bool GetNumberedFrame( frameindex_t frameIdx, int timeout, std::vector< Mat* > dsts ) = 0;
    virtual std::vector< frameindex_t > GetFrameNumbers(){};
    virtual void SetOutput1StateHigh( int board ){};
    virtual void SetOutput1StateLow( int board ){};
    virtual unsigned GetNumCameras() = 0;
    virtual void SetFPS( int in_FPS, int masterBoard ) {};
    virtual void SetResolution( long int cols, long int rows ) {};
    virtual void SetResolution( int cam, long int cols, long int rows ) {};
    virtual void GetResolution( int cam, long int &cols, long int &rows ) {};
    
    virtual void SetExposure( long int exposure ){};
    virtual void SetExposure( int cam, long int exposure ){};
    
    virtual void SetGain( double gain ) {};
    virtual void SetGain( int cam, double gain ) {};
    virtual void SetBaseGain( baseGain_t value ) {};
    virtual void SetBaseGain( int cam, baseGain_t value ) {};

    bool fake = false;
};


#endif //MC_GRABBER_GRABBER_H
