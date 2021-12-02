#ifdef USE_SISO

#include "imgio/siso.h"
#include "imgio/loadsave.h"

#include <chrono>
#include <thread>

int main(void)
{
	std::vector<SiSoBoardInfo> boardInfo(1);
	
	boardInfo[0].boardIndx = 0;
	boardInfo[0].grabberConfig = "/home/me475/docs/Stemmer/MS_Generator_Master_ME.mcf";
	
	cout << "Create Grabber..." << endl;
	SiSoGrabber grabber( boardInfo );
	
	cout << "wait a second or two..." << endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	
	grabber.PrintCameraInfo();
	
	
	cout << "grab a few images." << endl;
	grabber.SetFPS(100, 0);
	grabber.SetResolution( 800, 600 );
	
	grabber.StartAcquisition(1, 0);
	grabber.StartTrigger(0);
	
	for( int ic = 0; ic < 100; ++ic )
	{
		cout << ic << endl;
		grabber.GetCurrent(2);
		auto fnos = grabber.GetFrameNumbers();
		for( unsigned cc = 0; cc < grabber.currentFrames.size(); ++cc )
		{
			cout << cc << " " << fnos[cc] << " " << grabber.currentFrames[cc].cols << " " << grabber.currentFrames[cc].rows << endl;
		}
		//SaveImage( grabber.currentFrames[0], "siso.png" );
	}
	
	grabber.StopTrigger(0);
	grabber.StopAcquisition();
	
}

#endif
