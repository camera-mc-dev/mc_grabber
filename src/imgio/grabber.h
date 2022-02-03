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
};


#endif //MC_GRABBER_GRABBER_H
