//
// Created by reuben on 2022-01-21.
//

#include "fake.h"

FakeGrabber::FakeGrabber(int numCams)
{
    auto sp = CreateSource(std::string("/media/reuben/HDD/Work/test.mp4"));
}

// void FakeGrabber::SetFPS(int in_FPS)
// {
    
// }