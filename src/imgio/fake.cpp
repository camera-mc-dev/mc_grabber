//
// Created by reuben on 2022-01-21.
//

#include "fake.h"

// tried tying this function in to a class but i get a "static function" error.
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


//
// Fake Camera
//

FakeCamera::FakeCamera(std::string pathToSource)
{
    source_pair = CreateSource(pathToSource);
}

void FakeCamera::Advance()
{
    frameReady = false;
    currentFrame = source_pair.source->GetCurrent();
    if (!source_pair.source->Advance())
    {
        source_pair.source->JumpToFrame(0);
    }
    currentFrameIdx = source_pair.source->GetCurrentFrameID();
    frameReady = true;

}
int FakeCamera::GetCurrentFrameIdx()
{
    return currentFrameIdx;
}
Mat FakeCamera::GetCurrentFrame()
{
    while(!frameReady)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return currentFrame;
}

//
// Fake Grabber
//

FakeGrabber::FakeGrabber(string pathToSource)
{
    for (unsigned i = 0; i < GetNumCameras(); i++)
    {
        cameras.push_back(FakeCamera(pathToSource));
        camFrames.push_back(0);
        fake = true;
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
        assert( dsts[i]->rows == desiredRows && dsts[i]->cols == desiredCols);
        dsts[i]->data = currentFrames[i].data;
    }

    return true;
}

frameindex_t FakeGrabber::GetSyncFrame(int timeout)
{
    return (frameindex_t) cameras[0].GetCurrentFrameIdx();
}

std::vector< frameindex_t > FakeGrabber::GetFrameNumbers()
{
    for (unsigned i = 0; i < GetNumCameras(); i++)
        {
             camFrames[i] = cameras[i].GetCurrentFrameIdx();
        }
    return camFrames;
}

void FakeGrabber::StartAcquisition()
{
    done = false;
    chronoThread = std::thread(RunFakeCamera, &cameras, &done, fps);

}