//
// Created by reuben on 2022-01-21.
//

#include "fake.h"

FakeGrabber::FakeGrabber(int numCams)
{
    for (unsigned i = 0; i < GetNumCameras(); i++)
    {
        //NOTE: If the source does not exist, CreateSource does not throw an error.
        sps.push_back(CreateSource(std::string("/home/rjl67/futurama_s01e01.mp4")));
    }


    // for (unsigned cc=0; cc < sps[0].source->GetNumImages(); cc++)
    // {
    //     Mat frame = sps[0].source->GetCurrent();
    //     sps[0].source->Advance();
    //     imshow("this image", frame);
    //     if( waitKey(10) == 27 ) break;
    // }

}

void FakeGrabber::GetCurrent()
{
    if (currentFrames.size() < GetNumCameras())
    {
        for (unsigned i = 0; i < GetNumCameras(); i++)
        {
             currentFrames.push_back(sps[i].source->GetCurrent());
        }

    }
    else
    {
        for (unsigned i = 0; i < GetNumCameras(); i++)
        {
             currentFrames[i] = sps[i].source->GetCurrent();
        }
    }

    for (unsigned i = 0; i < GetNumCameras(); i++)
    {
        sps[i].source->Advance();
    }
}

// void FakeGrabber::SetFPS(int in_FPS)
// {
    
// }