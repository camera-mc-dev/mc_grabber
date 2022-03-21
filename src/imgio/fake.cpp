//
// Created by reuben on 2022-01-21.
//

#include "fake.h"
#include "imgproc/debayer/debayer.h"

//
// Fake Camera
//

FakeCamera::FakeCamera(std::string pathToSource)
{
	frameReady = false;
	source_pair = CreateSource(pathToSource);
}

void FakeCamera::Advance()
{
	frameReady = false;
	cv::Mat tmp = source_pair.source->GetCurrent();
	Bayer( tmp, currentFrame, GRBG );
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
	while (!frameReady)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return currentFrame;
}

//
// Fake Grabber
//

void RunFakeCamera(FakeGrabber *fgrabber)
{
	fgrabber->CameraLoop();
}

void FakeGrabber::CameraLoop()
{
	// make sure there's one image loaded, as that's what a real camera would have,
	// and we often pause external camera trigger before starting aquisition so that 
	// we know where all cameras are in time.
	for (unsigned i = 0; i < cameras.size(); i++)
	{
		cameras[i].Advance();
	}
	while (!done)
	{
		if( !paused )
		{
			for (unsigned i = 0; i < cameras.size(); i++)
			{
				cameras[i].Advance();
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
	}
}

FakeGrabber::FakeGrabber(string pathToSource)
{
	paused = false;
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

bool FakeGrabber::GetNumberedFrame(frameindex_t frameIdx, int timeout, std::vector< Mat * > dsts)
{
	GetCurrent();

	for (unsigned i = 0; i < GetNumCameras(); i++)
	{
		assert(dsts[i]->rows == desiredRows && dsts[i]->cols == desiredCols);
		dsts[i]->data = currentFrames[i].data;
	}

	return true;
}

frameindex_t FakeGrabber::GetSyncFrame(int timeout)
{
	return (frameindex_t)cameras[0].GetCurrentFrameIdx();
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
	chronoThread = std::thread(RunFakeCamera, this);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // in lieu of knowing the "cameras" are running.
}
