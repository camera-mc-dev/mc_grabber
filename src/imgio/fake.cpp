//
// Created by reuben on 2022-01-21.
//

#include "fake.h"

void RunFakeCamera(FakeCamera* camera, std::string pathToSource)
{
    camera->run();
}

FakeCamera::FakeCamera(std::string pathToSource)
{
    source_pair = CreateSource(pathToSource);
}

void FakeCamera::run()
{
     while(!this->kill)
    {
        currentFrame = source_pair.source->GetCurrent();
        if (!source_pair.source->Advance()){
            source_pair.source->JumpToFrame(0);
        }
        currentFrameIdx = source_pair.source->GetCurrentFrameID();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000/fps));
    }
}

FakeGrabber::FakeGrabber(string pathToSource)
{

    std::vector<std::thread> threads;
    std::vector<FakeCamera> cameras;
    for (unsigned i = 0; i < GetNumCameras(); i++)
    {
        //FakeCamera camera(pathToSource);
        cameras.push_back(FakeCamera(pathToSource));
        threads.push_back(std::thread(RunFakeCamera, &cameras[i], pathToSource));
        //NOTE: If the source does not exist, CreateSource does not throw an error.
        source_pairs.push_back(CreateSource(pathToSource));
        camFrames.push_back(0);
        this->fake = true;
    }
    while(1)
    {
        for (unsigned i = 0; i < GetNumCameras(); i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            cout << "cam num" << i  << " " << cameras[i].GetCurrentFrame() << endl;
        }

    }
    for (unsigned i = 0; i < GetNumCameras(); i++)
    {
        threads[i].join();
    }
    


}

void FakeGrabber::GetCurrent()
{
    if (currentFrames.size() < GetNumCameras())
    {
        for (unsigned i = 0; i < GetNumCameras(); i++)
        {
             currentFrames.push_back(source_pairs[i].source->GetCurrent());
        }

    }
    else
    {
        for (unsigned i = 0; i < GetNumCameras(); i++)
        {
             currentFrames[i] = source_pairs[i].source->GetCurrent();
        }
    }

    for (unsigned i = 0; i < GetNumCameras(); i++)
    {
        if (!source_pairs[i].source->Advance()){source_pairs[i].source->JumpToFrame(0);};
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
    //cout << "current frame id " << source_pairs[0].source->GetCurrentFrameID() << endl;
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
