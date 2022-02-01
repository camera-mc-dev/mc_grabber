//
// Created by reuben on 2022-01-21.
//

#include "fake.h"


FakeGrabber::FakeGrabber(int numCams)
{
    auto sp = CreateSource(std::string("/dev/video0"));
    
    
    for (unsigned cc=0; cc < sp.source->GetNumImages(); cc++)   
    {
        Mat frame = sp.source->GetCurrent();
        sp.source->Advance();    
        imshow("this image", frame);
        if( waitKey(10) == 27 ) break;
    }
    
}

// void FakeGrabber::SetFPS(int in_FPS)
// {
    
// }