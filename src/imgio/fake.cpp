//
// Created by reuben on 2022-01-21.
//

#include "fake.h"
FakeGrabber::FakeGrabber(string pathToSource)
{
    for (unsigned i = 0; i < GetNumCameras(); i++)
    {
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