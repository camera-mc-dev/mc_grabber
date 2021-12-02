#ifdef USE_SISO

#include <iomanip>
#include <thread>

#include "renderer2/basicRenderer.h"
#include "renderer2/geomTools.h"
#include "commonConfig/commonConfig.h"

#include "imgio/siso.h"
#include "imgio/loadsave.h"

int main(int argc, char* argv[])
{
	if( argc != 2 )
	{
		cout << "Basic buffered recording of camera feeds." << endl;
		cout << "Usage: " << endl;
		cout << argv[0] << " <number of boards> " << endl;
		exit(0);
	}
	
	int numBoards      = atoi(argv[1]);
	
	CommonConfig ccfg;
	//
	// first off, create the grabber and find out how many cameras we have
	//
	
	std::vector<SiSoBoardInfo> boardInfo(numBoards);
	for( unsigned bc = 0; bc < numBoards; ++bc )
	{
		std::stringstream ss;
		boardInfo[bc].boardIndx = bc;
		if( bc == 0 )
		{
			ss << ccfg.coreDataRoot << "/SiSo/MS_Generator_Master_ME.mcf";
			boardInfo[bc].grabberConfig = ss.str();
		}
		else
		{
			ss << ccfg.coreDataRoot << "/SiSo/MS_Generator_Slave_ME.mcf";
			boardInfo[bc].grabberConfig = ss.str();
		}
	}
	
	
	cout << "Create Grabber..." << endl;
	SiSoGrabber grabber( boardInfo );
	
	grabber.PrintCameraInfo();
	
	grabber.PowerCycle();
	
	
}

#endif
