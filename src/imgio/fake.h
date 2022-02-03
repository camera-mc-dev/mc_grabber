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


class FakeGrabber: public AbstractGrabber 
{
public:

    FakeGrabber(std::vector<SiSoBoardInfo> boardInfo );
    FakeGrabber(int foo);

    void PrintCameraInfo() override 
    {
        cout << "num active source_pairs " << source_pairs.size() << endl;
    };

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

    /////////////////////////////////////////////////////////////////////////
    // unimplemented functions
    /////////////////////////////////////////////////////////////////////////
    void SetResolution( long int cols, long int rows ){};
    void SetResolution( int cam, long int cols, long int rows ){};
    void SetExposure( long int exposure ){};
    void SetExposure( int cam, long int exposure ){};
    void SetFPS( int in_FPS, int masterBoard ){};

    //overrides
    void StartAcquisition( int bufferFrames, int masterBoard ) override {};
    void StopAcquisition() override {};
    void StartTrigger( int masterBoard ) override {};
    void StopTrigger( int masterBoard ) override {};
    bool GetCurrentEnsureSynch( int timeout ) override {return 0;};
    
    bool GetNumberedFrame( frameindex_t frameIdx, int timeout ) override {return 0;};
    
    


private:
   std::vector <SourcePair> source_pairs;
   std::vector< frameindex_t > camFrames;

};


#endif //MC_GRABBER_FAKE_H
