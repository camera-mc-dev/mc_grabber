//
// Created by reuben on 2022-01-21.
//

#include "fake.h"

void RunFakeCamera(std::vector <FakeCamera> * cameras, bool* done, int fps)
{
    while(!*done)
    {
        for (unsigned i = 0 ; i < cameras[0].size(); i++)
        {
            cameras[0][i].Advance();
        }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000/fps));
    }
}

FakeCamera::FakeCamera(std::string pathToSource)
{
    source_pair = CreateSource(pathToSource);
}

void FakeCamera::Advance()
{
    currentFrame = source_pair.source->GetCurrent();
    if (!source_pair.source->Advance())
    {
        source_pair.source->JumpToFrame(0);
    }
    currentFrameIdx = source_pair.source->GetCurrentFrameID();

}

Mat FakeCamera::GetCurrentFrame()
{
    return currentFrame;
}

FakeGrabber::FakeGrabber(string pathToSource)
{
    for (unsigned i = 0; i < GetNumCameras(); i++)
    {
        cameras.push_back(FakeCamera(pathToSource));
        //NOTE: If the source does not exist, CreateSource does not throw an error.
        source_pairs.push_back(CreateSource(pathToSource));
        camFrames.push_back(0);
        this->fake = true;
    }
}

void FakeGrabber::GetCurrent()
{
    if (currentFrames.size() < GetNumCameras())
    {
        for (unsigned i = 0; i < GetNumCameras(); i++)
        {
             currentFrames.push_back(cameras[i].GetCurrentFrame());
        }

    }
    else
    {
        for (unsigned i = 0; i < GetNumCameras(); i++)
        {
             currentFrames[i] = cameras[i].GetCurrentFrame();
        }
    }
}


bool FakeGrabber::GetNumberedFrame( frameindex_t frameIdx, int timeout, std::vector< Mat* > dsts ) 
{
    GetCurrent();
    
    for (unsigned i = 0; i < GetNumCameras(); i++)
    {
        int camHeight = currentFrames[i].rows;
        int camWidth = currentFrames[i].cols;
        assert( dsts[i]->rows == camHeight && dsts[i]->cols == camWidth);
        dsts[i]->data = currentFrames[i].data;
    }

    return true;
}

frameindex_t FakeGrabber::GetSyncFrame(int timeout)
{
    return (frameindex_t) source_pairs[0].source->GetCurrentFrameID();
}

std::vector< frameindex_t > FakeGrabber::GetFrameNumbers()
{
    for (unsigned i = 0; i < GetNumCameras(); i++)
        {
             camFrames[i] = source_pairs[i].source->GetCurrentFrameID();
        }
    return camFrames;
}

void FakeGrabber::StartAcquisition()
{
    chronoThread = std::thread(RunFakeCamera, &cameras, &done, fps);
}

// Some code for testing the camera thread. 

// }