//
// Created by reuben on 2022-01-21.
//

#ifndef MC_GRABBER_FAKE_H
#define MC_GRABBER_FAKE_H

#include "grabber.h"

class FakeGrabber: public AbstractGrabber 
{
public:

    FakeGrabber(int value);

    void PrintCameraInfo() {};

    unsigned GetNumCameras()
    {
        // alwayus just return 4 for now.
        return 4;
    }
//    void StartAcquisition( int bufferFrames, int masterBoard ) {};
//    void StopAcquisition() {};
//    void StartTrigger( int masterBoard ) {};
//    void StopTrigger( int masterBoard ) {};
//    bool GetCurrentEnsureSynch( int timeout ) = 0;
//    frameindex_t GetSyncFrame( int timeout ) {};
//    bool GetNumberedFrame( frameindex_t frameIdx, int timeout ) = 0;
//    bool GetNumberedFrame( frameindex_t frameIdx, int timeout, std::vector< cv::Mat* > dsts ) = 0;
//    std::vector< frameindex_t > GetFrameNumbers(){};
};


#endif //MC_GRABBER_FAKE_H
