#ifdef USE_SISO

#include "imgio/siso.h"

SiSoGrabber::SiSoGrabber(std::vector<SiSoBoardInfo> boardInfo)
{
	for( unsigned bc = 0; bc < boardInfo.size(); ++bc )
	{
		//
		// initialise handle to the fg library
		//
		cout << "init board " << bc << " with config: " << endl;
		cout << "\t" << boardInfo[bc].grabberConfig.c_str() << endl;
		Fg_Struct *fg = Fg_InitConfig( boardInfo[bc].grabberConfig.c_str(), boardInfo[bc].boardIndx );

		if( fg == NULL )
		{
			cout << "Error Fg_Init for board: " << boardInfo[bc].boardIndx << endl;
			CleanUp();
			exit(0);
		}
		fgHandles.push_back( fg );

		//
		// Initialise genicam handle
		//
	    SgcBoardHandle *boardHandle;
    	int err = Sgc_initBoard( fg, 0, &boardHandle );
	    if( err != 0 )
    	{
        	cout << "Sgc_initBoard error for board " << boardInfo[bc].boardIndx << " : " << err << endl;
	        CleanUp();
    	    exit(0);
    	}
		boardHandles.push_back( boardHandle );



		//
		// Scan the board for cameras.
		//
		cout << "scan board" << endl;
		err = Sgc_scanPorts( boardHandle, 0xFFFF, 2, 0 );
		if( err != 0 )
		{
			 cout << "Sgc_scanPorts error for board " << boardInfo[bc].boardIndx << " : " << err << endl;
			 CleanUp();
			 exit(0);
		}
	
		//
		// Initialise each camera.
		//
		int numCameras = Sgc_getCameraCount( boardHandle );
		for( unsigned cc = 0; cc < numCameras; ++cc )
		{
			SgcCameraHandle *camHandle;
			err = Sgc_getCameraByIndex( boardHandle, cc, &camHandle );
			if( err != 0 )
			{
				cout << "Sgc_getCameraByIndex error (board,camera) (" << boardInfo[bc].boardIndx << "," << cc << ") : " << err << endl;
				CleanUp();
				exit(0);
			}

			SgcCameraInfo *camInfo;
			camInfo = Sgc_getCameraInfo( camHandle );

			if( camInfo != NULL )
			{
				err = Sgc_connectCamera( camHandle );
				if( err != NULL )
				{
					cout << "Error connecting camera: " << err << " " << Sgc_getErrorDescription(err) << endl;
					CleanUp();
					exit(0);
				}

				camInfos.push_back( camInfo );
				camHandles.push_back( camHandle );
				camAddrs.push_back( std::pair<int,int>(bc,cc) );
				memHandles.push_back(NULL);
				camFrames.push_back(0);
			}
		}
	}	
}

void SiSoGrabber::CleanUp()
{
	//
	//TODO: Cleanup cameras, stop grabbing, etc...
	//

	for( unsigned bc = 0; bc < boardHandles.size(); ++bc )
	{
		Sgc_freeBoard( boardHandles[bc] );
	}
	boardHandles.clear();

	for( unsigned fc = 0; fc < fgHandles.size(); ++fc )
	{
		Fg_FreeGrabber( fgHandles[fc] );
	}
	fgHandles.clear();
}



void SiSoGrabber::PrintCameraInfo()
{
	for( unsigned cc = 0; cc < camInfos.size(); ++cc )
	{
		cout << " ======== " << endl;
		cout << "Camera " << cc << " : (" << camAddrs[cc].first << ", " << camAddrs[cc].second << ")" << endl;
		cout << "\tfw version:  " << camInfos[cc]->deviceFirmwareVersion << endl;
		cout << "\tmfer info:   " << camInfos[cc]->deviceManufacturerInfo << endl;
		cout << "\tmodel name:  " << camInfos[cc]->deviceModelName << endl;
		cout << "\tserial num:  " << camInfos[cc]->deviceSerialNumber << endl;
		cout << "\tuser id:     " << camInfos[cc]->deviceUserId << endl;
		cout << "\tvendor name: " << camInfos[cc]->deviceVendorName << endl;
		cout << "\tversion:     " << camInfos[cc]->deviceVersion << endl;
	}
}

void SiSoGrabber::StartTrigger( int masterBoard )
{
	int ts = TS_ACTIVE;
	Fg_setParameter(fgHandles[masterBoard], FG_TRIGGERSTATE, &ts, 0);
}

void SiSoGrabber::StopTrigger( int masterBoard )
{
	int ts = TS_SYNC_STOP;
	Fg_setParameter(fgHandles[masterBoard], FG_TRIGGERSTATE, &ts, 0);
}

void SiSoGrabber::SetOutput1StateHigh( int masterBoard )
{
	int val = VCC;
	Fg_setParameter( fgHandles[ masterBoard ], FG_TRIGGEROUT_SELECT1, &val, 0);
}
void SiSoGrabber::SetOutput1StateLow( int masterBoard )
{
	int val = GND;
	Fg_setParameter( fgHandles[ masterBoard ], FG_TRIGGEROUT_SELECT1, &val, 0);
}

void SiSoGrabber::StartAcquisition(int bufferFrames, int masterBoard )
{
	numFramesInCurrentRun = 0;
	double fps;
	Fg_getParameter(fgHandles[ masterBoard ], FG_TRIGGER_FRAMESPERSECOND, &fps, 0);
	for( unsigned cc = 0; cc < camAddrs.size(); ++cc )
	{
		//
		// Need to allocate the grab buffer
		//
		int fc = camAddrs[cc].first;
		int port = camAddrs[cc].second;
		
		cout << "start acq: " << cc << " " << fc << " " << port << endl;
		
		int bytesPerPixel = 1;
// 		numImagesInBuffers = (fps * milliseconds)/1000;
// 		if( numImagesInBuffers > 256 )
// 		{
// 			cout << "It looks like the FG library's internal ring buffer has a maximum of 1048 frames." << endl;
// 			cout << "It is documented as only having a maximum of 256." << endl;
// 			cout << "As such, to buffer more than that to memory you will need to create your own buffering." << endl;
// 			if( numImagesInBuffers > 1048 )
// 			{
// 				exit(0);
// 			}
// 		}
		numImagesInBuffers = bufferFrames;
		
		
		int camHeight = -1;
		int camWidth = -1;
		Fg_getParameter(fgHandles[fc], FG_HEIGHT, &camHeight, port );
		Fg_getParameter(fgHandles[fc], FG_WIDTH,  &camWidth, port );
		
		cout << "fps,w,h: " << fps << " " << camWidth << " " << camHeight << endl;
		
		int bufferSize = camHeight * camWidth * bytesPerPixel * numImagesInBuffers;
		
		if( memHandles[cc] != NULL )
			Fg_FreeMemEx( fgHandles[fc], memHandles[cc] );
		
		memHandles[cc] = Fg_AllocMemEx(fgHandles[fc], bufferSize, numImagesInBuffers);
		if (memHandles[cc] == NULL)
    	{
        	cout << "Error in Fg_AllocMemEx: " << Fg_getLastErrorDescription(fgHandles[fc]) << endl;
	        CleanUp();
			exit(0);
    	}

		//
		// Fg level first
		//
		int res = Fg_AcquireEx( fgHandles[fc], port, GRAB_INFINITE, ACQ_STANDARD, memHandles[cc] );
		if( res != FG_OK )
		{
			cout << "Error in Fg_AcquireEx() " << Fg_getLastErrorDescription(fgHandles[fc]) << endl;
			CleanUp();
		}
		//
		// Then genicam
		//
		int err = Sgc_startAcquisition( camHandles[cc], 0 );
		if( err != 0 )
		{
			cout << "Error in Sgc_startAcquisition: " << err << " " << Sgc_getErrorDescription(err) << endl;
			CleanUp();
			exit(0);
		}
	}
	runStart = std::chrono::steady_clock::now();
}


void SiSoGrabber::StopAcquisition( )
{
	for( unsigned cc = 0; cc < camAddrs.size(); ++cc )
	{
		//
		// Need to allocate the grab buffer
		//
		int fc = camAddrs[cc].first;
		int port = camAddrs[cc].second;
		
		//
		// genicam first
		//
		int err = Sgc_stopAcquisition( camHandles[cc], 0 );
		if( err != 0 )
		{
			cout << "Error in Sgc_stopAcquisition: " << err << " " << Sgc_getErrorDescription(err) << endl;
			CleanUp();
			exit(0);
		}

		//
		// Then FG level
		//
		int res = Fg_stopAcquireEx( fgHandles[fc], port, memHandles[cc], STOP_ASYNC );
		if( res != 0 )
		{
			cout << "Error stopping camera " << cc << " : ( " << res  << ") " << Fg_getLastErrorDescription(fgHandles[fc]) << endl;
		}
	}
	
	auto t1 = std::chrono::steady_clock::now();
	auto d = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - runStart);
	
	cout << "got " << numFramesInCurrentRun << " frames in " << d.count() << " ms" << endl;
	cout << "that is: " << numFramesInCurrentRun / (d.count()/1000.0f) << endl;
}


void SiSoGrabber::SetFPS(int inFPS, int masterBoard )
{
	double fps = inFPS;
	Fg_setParameter(fgHandles[masterBoard], FG_TRIGGER_FRAMESPERSECOND, &fps, 0);
}

void SiSoGrabber::SetResolution( long int cols, long int rows )
{
	for( unsigned cc = 0; cc < camInfos.size(); ++cc )
	{
		SetResolution(cc, cols, rows);
	}
}

void SiSoGrabber::SetResolution( int cam, long int cols, long int rows )
{
	int fc = camAddrs[cam].first;
	int port = camAddrs[cam].second;
	
	// If we ever have different cameras than the JAIs we will need to put 
	// more effort into where we get this information from.
	long int maxWidth = 2560;
	long int maxHeight = 2048;
	
	cols = std::max(64l, std::min( cols, maxWidth ) );
	rows = std::max(2l, std::min( rows, maxHeight) );
	
	//
	// the width (i.e. cols) must be a multiple of 64
	// the height (i.e. rows) must be a multiple of 2
	//
	while( cols > 64 && cols % 64 != 0 )
		--cols;
	
	while( rows > 2 && rows % 2 != 0 )
		--rows;
	
	
	//
	// Try to set the width and height first.
	//
	cout << "set cols rows  : " << cols << " " << rows << endl;
	auto err = Sgc_setIntegerValue( camHandles[cam], "Width", cols );
	if( err != 0 )
	{
		cout << "Error setting camera image width: ( " << err << " ) " << Sgc_getErrorDescription(err) << endl;
	}
	err = Sgc_setIntegerValue( camHandles[cam], "Height", rows );
	if( err != 0 )
	{
		cout << "Error setting camera image height: ( " << err << " ) " << Sgc_getErrorDescription(err) << endl;
	}
	
	Sgc_getIntegerValue( camHandles[cam], "Width", &cols );
	Sgc_getIntegerValue( camHandles[cam], "Height", &rows );
	cout << "get cols rows  : " << cols << " " << rows << endl;
	
	//
	// Decide our offset based on the actual image size we've got.
	//
	// We want to make sure we are always taking the crop from the centre of the image.
	//
	long int offx = maxWidth/2 - cols/2;
	long int offy = maxHeight/2 - rows/2;
	
	// but the offx must be a multiple of 64
	while( offx > 0 && offx % 64 != 0 )
		--offx;
	
	// and offy must be a multiple of 2.
	while( offy > 0 && offy % 2 != 0 )
		--offy;
	
	cout << "set offx offy  : " << offx << " " << offy << endl;
	err = Sgc_setIntegerValue( camHandles[cam], "OffsetX", offx );
	if( err != 0 )
	{
		cout << "Error setting camera image offset x: " << Sgc_getErrorDescription(err) << endl;
	}
	err = Sgc_setIntegerValue( camHandles[cam], "OffsetY", offy );
	if( err != 0 )
	{
		cout << "Error setting camera image offset y: " << Sgc_getErrorDescription(err) << endl;
	}
	
	Sgc_getIntegerValue( camHandles[cam], "OffsetX", &offx );
	Sgc_getIntegerValue( camHandles[cam], "OffsetY", &offy );
	cout << "get offx offy  : " << offx << " " << offy << endl;

	
	
	
	//
	// Now that we've set this information on the camera, what do we need to set on the grabber?
	//
	unsigned fgCols = cols;
	unsigned fgRows = rows;
	unsigned fgOffx = 0;//offx;
	unsigned fgOffy = 0;//offy;
	cout << Fg_setParameter(fgHandles[fc], FG_WIDTH, &fgCols, port)  << endl;
	cout << Fg_setParameter(fgHandles[fc], FG_HEIGHT, &fgRows, port) << endl;
	cout << Fg_setParameter(fgHandles[fc], FG_XOFFSET, &fgOffx, port)<< endl;
	cout << Fg_setParameter(fgHandles[fc], FG_YOFFSET, &fgOffy, port)<< endl;
	cout << "set fg : " << fgOffx << " " << fgOffy << " " << fgCols << " " << fgRows << endl;
	
	Fg_getParameter(fgHandles[fc], FG_WIDTH,   &fgCols, port);
	Fg_getParameter(fgHandles[fc], FG_HEIGHT,  &fgRows, port);
	Fg_getParameter(fgHandles[fc], FG_XOFFSET, &fgOffx, port);
	Fg_getParameter(fgHandles[fc], FG_YOFFSET, &fgOffy, port);
	cout << "get fg: " << fgOffx << " " << fgOffy << " " << fgCols << " " << fgRows << endl;
}

void SiSoGrabber::GetResolution(int cam, long int &cols, long int &rows )
{
	Sgc_getIntegerValue( camHandles[cam], "Width", &cols );
	Sgc_getIntegerValue( camHandles[cam], "Height", &rows );
}

void SiSoGrabber::SetExposure( long int exposure )
{
	for( unsigned cc = 0; cc < camInfos.size(); ++cc )
	{
		SetExposure(cc, exposure);
	}
}
void SiSoGrabber::SetExposure( int cam, long int exposure )
{
	long int val;
// 	cout << Sgc_getEnumerationValue( camHandles[cam], "ExposureMode", &val ) << endl;
// 	cout << "exp mode " << cam << " : " << val << endl;
// 	
// 	cout << Sgc_getEnumerationValue( camHandles[cam], "TriggerMode", &val ) << endl;
// 	cout << "trig mode " << cam << " : " << val << endl;
// 	
// 	cout << Sgc_getEnumerationValue( camHandles[cam], "TriggerOption", &val ) << endl;
// 	cout << "trig opt " << cam << " : " << val << endl;
// 	
// 	cout << Sgc_getIntegerValue( camHandles[cam], "ExposureTimeRaw", &val ) << endl;
// 	cout << "exp time " << cam << " : " << val << endl;
	
	cout << Sgc_setIntegerValue( camHandles[cam], "ExposureTimeRaw", exposure ) << endl;
	cout << "exp time " << cam << " : " << val << endl;
	
	cout << Sgc_getIntegerValue( camHandles[cam], "ExposureTimeRaw", &val ) << endl;
	cout << "exp time " << cam << " : " << val << endl;
	
// 	Sgc_setEnumerationValue( camHandles[cam], "ExposureMode", "Timed" );
// 	
// 	Sgc_setIntegerValue( camHandles[cam], "ExposureTime", exposure );
// 	int fc = camAddrs[cam].first;
// 	int port = camAddrs[cam].second;
// 	
// 	cout << "exp set: " << exposure << endl;
// 	Fg_getParameter(fgHandles[fc], FG_EXPOSURE, &exposure, port );
// 	
// 	
// 	Fg_getParameter(fgHandles[fc], FG_EXPOSURE, &exposure, port );
// 	cout << "exp get: " << exposure << endl;
}


void SiSoGrabber::SetGain( double gain )
{
	for( unsigned cc = 0; cc < camInfos.size(); ++cc )
	{
		SetGain(cc, gain);
	}
}
void SiSoGrabber::SetGain( int cam, double gain )
{
	cout << Sgc_setFloatValue( camHandles[cam], "Gain", gain ) << endl;
}


void SiSoGrabber::SetBaseGain( baseGain_t value )
{
	for( unsigned cc = 0; cc < camInfos.size(); ++cc )
	{
		SetBaseGain(cc, value);
	}
}

void SiSoGrabber::SetBaseGain( int cam, baseGain_t value )
{
	int err;
	switch( value )
	{
		case baseGain00:
			err = Sgc_setEnumerationValue(camHandles[cam], "AnalogBaseGain", "AnalogBaseGain0dB");
			break;
		case baseGain06:
			err = Sgc_setEnumerationValue(camHandles[cam], "AnalogBaseGain", "AnalogBaseGain6dB");
			break;
		case baseGain12:
			err = Sgc_setEnumerationValue(camHandles[cam], "AnalogBaseGain", "AnalogBaseGain12dB");
			break;
	}
	
	if( err != 0 )
	{
		cout << "Error in Sgc_startAcquisition: " << err << " " << Sgc_getErrorDescription(err) << endl;
		CleanUp();
		exit(0);
	}
}


void SiSoGrabber::GetCurrent( int timeout )
{
	if( currentFrames.size() != camInfos.size() )
		currentFrames.resize( camInfos.size() );
	
	for( unsigned cc = 0; cc < camInfos.size(); ++cc )
	{
		// find out what the most recent image number is.
		int fc = camAddrs[cc].first;
		int port = camAddrs[cc].second;
		
		int camHeight = -1;
		int camWidth = -1;
		Fg_getParameter(fgHandles[fc], FG_HEIGHT, &camHeight, port );
		Fg_getParameter(fgHandles[fc], FG_WIDTH,  &camWidth, port );
		
		frameindex_t nfNum = Fg_getLastPicNumberBlockingEx(fgHandles[fc], camFrames[cc]+1, port, timeout, memHandles[cc] );
		if( nfNum > 0 )
		{
			camFrames[cc] = nfNum;
			unsigned char *data = (unsigned char*)Fg_getImagePtrEx(fgHandles[fc], nfNum, port, memHandles[cc]);
			
			if( currentFrames[cc].rows != camHeight || currentFrames[cc].cols != camWidth )
			{
				currentFrames[cc] = cv::Mat( camHeight, camWidth, CV_8UC1, cv::Scalar(0) );
			}
			memcpy( currentFrames[cc].data, data, camHeight*camWidth );
		}
		else
		{
			//TODO: handle errors
		}
		
	}
}

bool SiSoGrabber::GetCurrentEnsureSynch( int timeout )
{
	bool retVal = true;
	
	if( currentFrames.size() != camInfos.size() )
		currentFrames.resize( camInfos.size() );
	
	// the camera who's most recent frame number is smallest should
	// indicate the frame number that is available on all cameras.
	frameindex_t earliest = 0;
	for( unsigned cc = 0; cc < camInfos.size(); ++cc )
	{
		// find out what the most recent image number is.
		int fc = camAddrs[cc].first;
		int port = camAddrs[cc].second;
		
		frameindex_t nfNum = Fg_getLastPicNumberBlockingEx(fgHandles[fc], camFrames[cc]+1, port, timeout, memHandles[cc] );
		if( nfNum > 0 )
		{
			if( cc != 0 )
			{
				earliest = std::min( earliest, nfNum );
			}
			else
			{
				earliest = nfNum;
			}
		}
	}
	
	for( unsigned cc = 0; cc < camInfos.size(); ++cc )
	{
		camFrames[cc] = earliest;
		
		int fc = camAddrs[cc].first;
		int port = camAddrs[cc].second;
		
		int camHeight = -1;
		int camWidth = -1;
		Fg_getParameter(fgHandles[fc], FG_HEIGHT, &camHeight, port );
		Fg_getParameter(fgHandles[fc], FG_WIDTH,  &camWidth, port );
		
		unsigned char *data = (unsigned char*)Fg_getImagePtrEx(fgHandles[fc], earliest, port, memHandles[cc]);
		
		if( data != NULL )
		{
			if( currentFrames[cc].rows != camHeight || currentFrames[cc].cols != camWidth )
			{
				currentFrames[cc] = cv::Mat( camHeight, camWidth, CV_8UC1, cv::Scalar(0) );
			}
			memcpy( currentFrames[cc].data, data, camHeight*camWidth );
		}
		else
			retVal = false;
	}
}


frameindex_t SiSoGrabber::GetSyncFrame( int timeout )
{
	// the camera who's most recent frame number is smallest should
	// indicate the frame number that is available on all cameras.
	frameindex_t earliest = 0;
	for( unsigned cc = 0; cc < camInfos.size(); ++cc )
	{
		// find out what the most recent image number is.
		int fc = camAddrs[cc].first;
		int port = camAddrs[cc].second;
		
		frameindex_t nfNum = Fg_getLastPicNumberBlockingEx(fgHandles[fc], camFrames[cc]+1, port, timeout, memHandles[cc] );
// 		cout << "\t " << cc << " <> " << nfNum << endl;
		if( nfNum > 0 )
		{
			if( cc != 0 )
			{
				earliest = std::min( earliest, nfNum );
			}
			else
			{
				earliest = nfNum;
			}
		}
	}
	return earliest;
}


//
// NOTE: I think the way that Fg_getImagePtrEx works is that, given the picture number,
//       it will figure out which image buffer it would be in and returns whatever is
//       in that buffer.  That, unfortunately, does not tell us if that picture number 
//       actually exists. Rather, you might find you have to ask for the most recent frame,
//       and then work backwards until you've used up all the buffer. Thus you must know
//       the buffer size.
//
bool SiSoGrabber::GetNumberedFrame( frameindex_t frameIdx, int timeout )
{
	if( currentFrames.size() != camInfos.size() )
		currentFrames.resize( camInfos.size() );
	
	for( unsigned cc = 0; cc < camInfos.size(); ++cc )
	{
		// find out what the most recent image number is.
		int fc = camAddrs[cc].first;
		int port = camAddrs[cc].second;
		
		int camHeight = -1;
		int camWidth = -1;
		Fg_getParameter(fgHandles[fc], FG_HEIGHT, &camHeight, port );
		Fg_getParameter(fgHandles[fc], FG_WIDTH,  &camWidth, port );
		
		auto bufNo =  Fg_getImageEx(fgHandles[fc], SEL_NUMBER, frameIdx, port, timeout, memHandles[cc] ); 
		unsigned char *data = (unsigned char*)Fg_getImagePtrEx(fgHandles[fc], bufNo, port, memHandles[cc]);
		if( data == NULL )
			return false;
		if( currentFrames[cc].rows != camHeight || currentFrames[cc].cols != camWidth )
		{
			currentFrames[cc] = cv::Mat( camHeight, camWidth, CV_8UC1, cv::Scalar(0) );
		}
		memcpy( currentFrames[cc].data, data, camHeight*camWidth );
	}
}

bool SiSoGrabber::GetNumberedFrame( frameindex_t frameIdx, int timeout, std::vector< cv::Mat* > dsts )
{
	assert( dsts.size() == camInfos.size() );
	
	for( unsigned cc = 0; cc < camInfos.size(); ++cc )
	{
		// find out what the most recent image number is.
		int fc = camAddrs[cc].first;
		int port = camAddrs[cc].second;
		
		int camHeight = -1;
		int camWidth = -1;
		Fg_getParameter(fgHandles[fc], FG_HEIGHT, &camHeight, port );
		Fg_getParameter(fgHandles[fc], FG_WIDTH,  &camWidth, port );
		
		unsigned char *data = (unsigned char*)Fg_getImagePtrEx(fgHandles[fc], frameIdx, port, memHandles[cc]);
		if( data != NULL )
		{
			camFrames[cc] = frameIdx;
			
			assert( dsts[cc]->rows == camHeight && dsts[cc]->cols == camWidth );
			memcpy( dsts[cc]->data, data, camHeight*camWidth );
		}
		else
		{
			cout << cc << " not got " << frameIdx << endl;
		}
	}
	
	++numFramesInCurrentRun;
}

void SiSoGrabber::PowerCycle()
{
	for( unsigned bc = 0; bc < boardHandles.size(); ++bc )
	{
		Sgc_powerSwitch( boardHandles[bc], 0xFFFFFFFF );
	}
}

#endif
