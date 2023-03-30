#include "mainWindow.h"


#include <boost/filesystem.hpp>

#include <set>
#include <iostream>
#include <iomanip>
#include <pwd.h>
#include "libconfig.h++"
using std::cout;
using std::endl;




MainWindow::MainWindow() : sessionsLBox(1), trialsLBox(1), camsLBox(1), debayerJobsList(1)
{
	//
	// We need three things from the grabber config:
	//  1) recording paths
	//  2) where the sessionDaemon binary is
	//
	
	std::string userHome;
#if defined(__APPLE__) || defined( __gnu_linux__ )
	struct passwd* pwd = getpwuid(getuid());
	if (pwd)
	{
		userHome = pwd->pw_dir;
	}
	else
	{
		// try the $HOME environment variable
		userHome = getenv("HOME");
	}
#else
	throw std::runtime_error("yeah, I've not done this for Windows or unknown unix!");
#endif
	
	std::stringstream ss;
	ss << userHome << "/.mc_dev.grabber.cfg";
	boost::filesystem::path p(ss.str());
	
	if( !boost::filesystem::exists(p) )
	{
		throw std::runtime_error("No grabber config found at: " + p.string());	
	}
	try
	{
		libconfig::Config cfg;
		cfg.readFile( ss.str().c_str() );
		
		std::string saveRoot0 = (const char*) cfg.lookup("saveRoot0");
		std::string saveRoot1 = (const char*) cfg.lookup("saveRoot1");
		recPaths = {saveRoot0, saveRoot1};

		// we need to know where the session daemon binary is.
		if(!cfg.exists("daemonBinary"))
		{
			auto &cfgRoot = cfg.getRoot();
			cfgRoot.add("daemonBinary", libconfig::Setting::TypeString);
			cfg.lookup("daemonBinary") = "/opt/mc_bin/mc_grabber/sessionDaemon";
		}
		daemonBinary = (const char*) cfg.lookup("daemonBinary");
		
		// we need to know where the calibration binaries are.
		if(!cfg.exists("calibBinariesDir"))
		{
			auto &cfgRoot = cfg.getRoot();
			cfgRoot.add("calibBinariesDir", libconfig::Setting::TypeString);
			cfg.lookup("calibBinariesDir") = "/opt/mc_bin/mc_core/";
		}
		calibBinariesDir = (const char*) cfg.lookup("calibBinariesDir");
		
		
		// we need to know where we're putting processed (debayered) videos
		if(!cfg.exists("processedSessionsRoot"))
		{
			auto &cfgRoot = cfg.getRoot();
			cfgRoot.add("processedSessionsRoot", libconfig::Setting::TypeString);
			cfg.lookup("processedSessionsRoot") = "/data/processedSessions/";
		}
		processedSessionsRoot = (const char*) cfg.lookup("processedSessionsRoot");
		
		// and we'd best make sure this exists as well.
		if(!cfg.exists("debayerBinary"))
		{
			auto &cfgRoot = cfg.getRoot();
			cfgRoot.add("debayerBinary", libconfig::Setting::TypeString);
			cfg.lookup("debayerBinary") = "/opt/mc_bin/mc_imgproc/debayer";
		}
		
		cfg.writeFile( ss.str().c_str() );
		
	}
	catch( libconfig::SettingException &e)
	{
		cout << "Setting error: " << endl;
		cout << e.what() << endl;
		cout << e.getPath() << endl;
		exit(0);
	}
	
	
	ScanForSessions();
	
	CreateInterface();
}


MainWindow::~MainWindow()
{
}

bool MainWindow::RescanSessionsTimer()
{
	ScanForSessions();
	return true;
}

void MainWindow::ScanForSessions()
{
	//
	// We make the assumption that the recording tool creates recordings in:
	// recPaths[0]/<session>/<trial>
	// recPaths[1]/<session>/<trial>
	//
	// We also assume that all processed (debayered etc... videos, calibs) are in:
	// processedSessionsRoot/<session>/<trial>
	//
	//
	// We don't assume that there is already a meta-file.
	//
	// Our job:
	// - find sessions and trials and cameras
	// - make best guess about status of calibration and debayering
	// - create meta-file if it doesn't already exist.
	//
	
	
	
	
	// scan for sessions.
	std::set<std::string> sessionNames;
	std::vector< std::string > scanPaths = recPaths;
	scanPaths.push_back( processedSessionsRoot );
	
	for(unsigned pc = 0; pc < scanPaths.size(); ++pc )
	{
		boost::filesystem::path p(scanPaths[pc]);
		if( boost::filesystem::exists(p) && boost::filesystem::is_directory(p))
		{
			boost::filesystem::directory_iterator di(p), endi;
			for( ; di != endi; ++di )
			{
				if( boost::filesystem::is_directory(di->path()) )
				{
					std::string s = di->path().string();
					
					auto i = s.rfind("/");
					sessionNames.insert( std::string( s.begin()+i+1, s.end() ) );
					
				}
			}
		}
	}
	
	for( auto ni = sessionNames.begin(); ni != sessionNames.end(); ++ni )
	{
		sessions[ *ni ].name = *ni;
		
		ScanSession( sessions[ *ni ] );
	}
}


void MainWindow::ScanSession( Ssession &sess )
{
	// TODO: Meta-file information.
	
	//
	// First job is to (re)discover where this session's files are located.
	// There's a reason to re-do this - and that is so that we can easily re-scan a session
	// without doing a global re-scan.
	//
	std::vector< std::string > scanPaths = recPaths;
	scanPaths.push_back( processedSessionsRoot );
	
	sess.paths.clear();
	for( unsigned pc = 0; pc < scanPaths.size(); ++pc )
	{
		std::stringstream ss;
		ss << scanPaths[pc] << "/" << sess.name << "/";
		
		boost::filesystem::path p(ss.str());
		if( boost::filesystem::exists(p) && boost::filesystem::is_directory(p) )
		{
			sess.paths.push_back( ss.str() );
		}
	}
	
	//
	// Now we can search in those locations for the trials.
	//
	
	
// 	cout << "sess: " << sess.name << endl;
	for( unsigned spc = 0; spc < sess.paths.size(); ++spc )
	{
		boost::filesystem::path p( sess.paths[spc] );
		if( boost::filesystem::exists(p) && boost::filesystem::is_directory(p))
		{
			boost::filesystem::directory_iterator di(p), endi;
			for( ; di != endi; ++di )
			{
				std::string s = di->path().string();
				if( boost::filesystem::is_directory(di->path()) )
				{
					
					
					auto i = s.rfind("/");
					std::string trialName( s.begin()+i+1, s.end() );
					
					if( trialName[ trialName.size()-3 ] == '_' )
					{
						auto ti = sess.trials.find( trialName );
						if( ti != sess.trials.end() )
						{
							ti->second.paths.insert( s );
						}
						else
						{
							sess.trials[ trialName ].paths.insert(s);
							sess.trials[ trialName ].name = trialName;
							sess.trials[ trialName ].sessionName = sess.name; // useful to keep with trial too.
						}
					}
				}
			}
		}
	}
	
	for( auto ti = sess.trials.begin(); ti != sess.trials.end(); ++ti )
	{
		ScanTrial( ti->second );
	}
	
	
}


void MainWindow::ScanTrial( Strial &trial )
{
// 	cout << "\ttrial: " << trial.name << endl;
	
	//
	// Have we got a meta-file?
	//
	
	
	//
	// Scan for cameras.
	//
	// We assume the recording software dumps cameras to numbered directories: 00,01,02,03,..,99
	//
	trial.cameras.clear();
	for( unsigned cc = 0; cc < 99; ++cc )
	{
		ScanCamera( trial, cc );
	}
	
	//
	// Check calibration things (if this is a calib trial)
	//
	if( trial.name.find("calib_") == 0 )
	{
		trial.isCalib = true;
	}
}


void MainWindow::ScanCamera( Strial &trial, unsigned camNum )
{
	ScameraInfo camInfo;
	camInfo.id = camNum;
	
	//
	// Check for raw Data
	//
	bool gotRawPath = false;
	bool gotRGBPath = false;
	for( auto tpi = trial.paths.begin(); tpi != trial.paths.end(); ++tpi )
	{
		// first off, check that this trial path actually exists...
		boost::filesystem::path tp( *tpi );
		if( boost::filesystem::exists(tp) && boost::filesystem::is_directory(tp))
		{
			// now look for:
			//  1) "??/"    : two-digit camera directory, we assume full of raw unprocessed images.
			//  2) "??.mp4" : two-digit camera video file
			std::stringstream rss, vss;
			rss << *tpi << "/" << std::setw(2) << std::setfill('0') << camNum;
			vss << *tpi << "/" << std::setw(2) << std::setfill('0') << camNum << ".mp4";
			
			boost::filesystem::path rp( rss.str() );
			if( boost::filesystem::exists(rp) && boost::filesystem::is_directory(rp) )
			{
				// found a possible raw path. Make sure it is the _only_ raw path we've found.
				if( !gotRawPath )
				{
					camInfo.rawPath = rp.string();
					
					gotRawPath = true;
				}
				else
				{
					std::stringstream msgss;
					msgss << "Warning: More than one location for raw camera image directory: " << endl;
					msgss << "\ttrial: " << trial.name << endl;
					msgss << "\t  (1): " << camInfo.rawPath << endl;
					msgss << "\t  (2): " << rp << endl;
					msgss << "\t  keeping (1) " << endl;
					
					cout << msgss.str() << endl;
					messageLog.push_back( msgss.str() );
					
				}
			}
			
			boost::filesystem::path vp( vss.str() );
			if( boost::filesystem::exists(vp) )
			{
				// found a possible raw path. Make sure it is the _only_ raw path we've found.
				if( !gotRGBPath )
				{
					camInfo.rgbPath = vp.string();
					
					gotRGBPath = true;
				}
				else
				{
					std::stringstream msgss;
					msgss << "Warning: More than one location for rgb camera video: " << endl;
					msgss << "\ttrial: " << trial.name << endl;
					msgss << "\t  (1): " << camInfo.rgbPath << endl;
					msgss << "\t  (2): " << vp << endl;
					msgss << "\t  keeping (1) " << endl;
					
					cout << msgss.str() << endl;
					messageLog.push_back( msgss.str() );
					
				}
			}
		}
	}
	
	
	if( gotRGBPath )
	{
		camInfo.isDebayered = true;
	}
	else
	{
		camInfo.isDebayered = false;
	}
	if( gotRawPath || gotRGBPath )
	{
		trial.cameras.push_back( camInfo );
	}
	
}




void MainWindow::CreateInterface()
{
	//
	// First off, we need a simple list of sessions.
	//
	for( auto i = sessions.begin(); i != sessions.end(); ++i )
	{
		sessionsLBox.prepend( i->first );
	}
	sessionsLBox.set_column_title(0,"sessions");
	sessionsLBox.set_activate_on_single_click(true);
	sessLBScroll.add( sessionsLBox );
	sessLBFrame.add( sessLBScroll );
	
	sessLBFrame.set_hexpand(true);
	sessLBFrame.set_vexpand(true);
	
	//
	// And of the trials of that session
	//
	trialsLBox.set_column_title(0,"trials");
	trialsLBox.set_activate_on_single_click(true);
	trialLBScroll.add( trialsLBox );
	trialLBFrame.add( trialLBScroll );
	trialLBFrame.set_hexpand(true);
	trialLBFrame.set_vexpand(true);
	
	
	//
	// And the cameras of the trial.
	//
	camsLBox.set_column_title(0,"cameras");
	camsLBox.set_activate_on_single_click(true);
	camLBScroll.add( camsLBox );
	camLBFrame.add( camLBScroll );
	camLBFrame.set_hexpand(true);
	camLBFrame.set_vexpand(true);
	
	
	
	
	//
	// Now we need to display session information in a frame
	//
	sessNameLabel0.set_text("Session:");
	sessNameLabel.set_text("(none selected)");
	
	
	sessFrameGrid.attach( sessNameLabel0,  0, 0, 1, 1);
	sessFrameGrid.attach( sessNameLabel,  1, 0, 1, 1);
	
	
	sessFrame.add( sessFrameGrid );
	
	
	//
	// And trial information
	//
	
	trialNameLabel0.set_text(         "Trial:"  );   trialNameLabel.set_text("(none selected)");
	trialDebayerStatusLabel0.set_text("debayer:");   trialDebayerStatusLabel.set_text("N/A");
	
	
	trialFrameGrid.attach(          trialNameLabel0,  0, 0, 1, 1);
	trialFrameGrid.attach( trialDebayerStatusLabel0,  0, 1, 1, 1);
	
	trialFrameGrid.attach(          trialNameLabel ,  1, 0, 1, 1);
	trialFrameGrid.attach( trialDebayerStatusLabel ,  1, 1, 1, 1);
	
	
	trialFrame.add( trialFrameGrid );
	
	
	//
	// And camera information
	//
	camNumLabel0.set_text( "Camera:" );          camNumLabel.set_text( "N/A" );
	camRawPathLabel0.set_text("raw path:");      camRawPathLabel.set_text("N/A");
	camDebayerPathLabel0.set_text("colour path:");  camDebayerPathLabel.set_text("N/A");
	
	camFrameGrid.attach( camNumLabel0,         0, 0, 1, 1 );
	camFrameGrid.attach( camRawPathLabel0,     0, 1, 1, 1 );
	camFrameGrid.attach( camDebayerPathLabel0, 0, 2, 1, 1 );
	
	camFrameGrid.attach( camNumLabel,         1, 0, 1, 1 );
	camFrameGrid.attach( camRawPathLabel,     1, 1, 1, 1 );
	camFrameGrid.attach( camDebayerPathLabel, 1, 2, 1, 1 );
	
	camFrame.add( camFrameGrid );
	
	
	sessTrialCamGrid.attach(  sessLBFrame,   0, 0, 1, 3);
	sessTrialCamGrid.attach( trialLBFrame,   1, 0, 1, 3);
	sessTrialCamGrid.attach(   camLBFrame,   2, 0, 1, 3);
	sessTrialCamGrid.attach(    sessFrame,   0, 3, 1, 1);
	sessTrialCamGrid.attach(   trialFrame,   1, 3, 1, 1);
	sessTrialCamGrid.attach(     camFrame,   2, 3, 1, 1);
	
	sourceFrame.set_label("data");
	sourceAlign.set( 0.5, 0.5, 0.95, 0.95 );
	sourceAlign.add( sessTrialCamGrid );
	sourceFrame.add( sourceAlign );
	
	//
	// vis Frame
	//
	visTrialBtn.set_label("Visualise Trial");
	visCameraBtn.set_label("Visualise Camera");
	visGrid.attach(  visTrialBtn,  0, 0, 1, 1 );
	visGrid.attach( visCameraBtn,  1, 0, 1, 1 );
	
	visAlign.set( 0.5, 0.5, 0.95, 0.95 );
	visAlign.add( visGrid );
	
	visFrame.add( visAlign );
	visFrame.set_label("Visualise");
	
	visTrialBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::VisTrialClick ) );
	visCameraBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::VisCameraClick ) );
	
	
	//
	// Demon Frame
	//
	demonFrame.set_label("Processing 'demon':");
	demonGrid.set_column_spacing(5);
	demonGrid.set_row_spacing(5);
	
	demonStatusLabel.set_label("status: (waiting info...)");
	demonJobsLabel.set_text(   "jobs  : (waiting info...)");
	demonErrLabel.set_text(    "errors: (waiting info...)");
	demonStartBtn.set_label("Start Demon");
	demonStopBtn.set_label("Stop Demon");
	demonClearBtn.set_label("Clear jobs");
	demonLogBtn.set_label("Check log");
	demonErrBtn.set_label("Clear err");
	demonGrid.attach(    demonStartBtn, 0, 0, 1, 1);
	demonGrid.attach(     demonStopBtn, 1, 0, 1, 1);
	demonGrid.attach(   demonStatusSep, 2, 0, 1, 1);
	demonGrid.attach( demonStatusLabel, 3, 0, 1, 1);
	demonGrid.attach(     demonJobsSep, 4, 0, 1, 1);
	demonGrid.attach(   demonJobsLabel, 5, 0, 1, 1);
	demonGrid.attach(    demonClearBtn, 6, 0, 1, 1);
	demonGrid.attach(      demonErrSep, 7, 0, 1, 1);
	demonGrid.attach(    demonErrLabel, 8, 0, 1, 1);
	demonGrid.attach(      demonLogBtn, 9, 0, 1, 1);
	demonGrid.attach(      demonErrBtn,10, 0, 1, 1);
	
	demonAlign.set( 0.5, 0.5, 0.95, 0.95 );
	demonAlign.add( demonGrid );
	demonFrame.add( demonAlign );
	
	
	
	demonStopBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DemonStopClick ) );
	demonStartBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DemonStartClick ) );
	demonClearBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DemonClearClick ) );
	demonLogBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DemonLogClick ) );
	
	//
	// The bottom half of the window is two tabs, one for calibration, 
	// one for debayering. We put those in a notebook.
	//
	botBook.set_name( "Processing:" );
	procAlign.set( 0.5, 0.5, 0.95, 0.95 );
	procAlign.add( botBook );
	procFrame.add( procAlign );
	procFrame.set_label("Processing");
	
	//
	// Add tools
	//
	
	
	
	
	
	//
	// Debayering
	//
	debayerSessBtn.set_label("Queue Session");
	debayerTrialBtn.set_label("Queue Trial");
	debayerCameraBtn.set_label("Queue Camera");
	debayerRemoveBtn.set_label("Remove job");
	debayerActionBtn.set_label("Launch jobs");
	debayerHelpBtn.set_label("Help!");
	
	debayerModeFrame.set_label( "Debayer Algorithm" );
	debayerModeRBCVEA.set_label( "OpenCV_EA" );
	debayerModeRBLED.set_label( "LED" );           debayerModeRBLED.join_group(debayerModeRBCVEA);
	debayerModeRBFCNN.set_label( "FCNN" );         debayerModeRBFCNN.join_group(debayerModeRBCVEA);
	debayerModeRBCVEA.set_active();
	
	debayerModeGrid.attach( debayerModeRBCVEA, 0, 0, 1, 1 );
	debayerModeGrid.attach( debayerModeRBLED,  0, 1, 1, 1 );
	debayerModeGrid.attach( debayerModeRBFCNN, 0, 2, 1, 1 );
	debayerModeFrame.add( debayerModeGrid );
	
	debayerJobsList.set_column_title(0,"jobs list");
	debayerJobsFrame.add( debayerJobsList );
	debayerJobsFrame.set_hexpand(true);
	debayerJobsFrame.set_vexpand(true);
	debayerJobsScroll.add( debayerJobsFrame );
	
	
	
	debayerFrameGrid.set_column_spacing(5);
	debayerFrameGrid.set_row_spacing(5);
	debayerFrameGrid.attach( debayerHelpBtn,   0, 0, 1, 1 );
	debayerFrameGrid.attach( debayerSessBtn,   0, 1, 1, 1 );
	debayerFrameGrid.attach( debayerTrialBtn,  0, 2, 1, 1 );
	debayerFrameGrid.attach( debayerCameraBtn, 0, 3, 1, 1 );
	debayerFrameGrid.attach( debayerActionBtn, 0, 4, 2, 2 );
	
	debayerFrameGrid.attach( debayerRemoveBtn, 1, 1, 1, 1 );
	debayerFrameGrid.attach( debayerModeFrame, 1, 2, 1, 2 );
	debayerFrameGrid.attach( debayerJobsScroll,  2, 0, 2, 6 );
	
	
	
	
	
	debayerFrame.set_label("Debayering");
	debayerFrame.add( debayerFrameGrid );
	
	
	
	debayerFrame.set_sensitive(false);
	
	CheckDemonStatus();
	
	
	debayerHelpBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DebayerHelpClick ) );
	debayerSessBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DebayerAddSessionClick ) );
	debayerTrialBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DebayerAddTrialClick ) );
	debayerCameraBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DebayerAddCameraClick ) );
	debayerRemoveBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DebayerRemoveJobClick ) );
	debayerActionBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DebayerProcessJobsClick ) );
	
	
	
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &MainWindow::CheckDemonStatusTimer), 100);
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &MainWindow::RescanSessionsTimer), 1000);
	
	
	//
	// Calibration
	//
	calibInitConfigBtn.set_label("Initialise config");
	calibRunCalibBtn.set_label("Run calibration");
	calibRunPointMatcherBtn.set_label("Run point matcher");
	calibRunAlignToolBtn.set_label("Run alignment");
	calibRunCheckToolBtn.set_label("Run check");
	calibRunMGPBtn.set_label("Run Make Ground Plane Image");
	calibHelpBtn.set_label("Help!");
	
	calibUseMatchesCheck.set_label("use matches");
	calibUseExGridsCheck.set_label("use existing grids");
	calibUseBundleCheck.set_label("use bundle adjust");
	calibXAxisIsNegCheck.set_label("align: X-Axis is negative");
	calibYAxisIsNegCheck.set_label("align: Y-Axis is negative");
	calibOriginHeightLabel.set_label("align: origin target height");
	calibOriginHeightEntry.set_text("50");
	
	calibVisFrameScroll.add( calibVisFrameGrid );
	calibVisFrameFrame.add( calibVisFrameScroll );
	calibVisFrameFrame.set_label("vis frame (for point matcher etc)..." );
	calibVisFrameFrame.set_hexpand(false);
	calibVisFrameFrame.set_vexpand(true);
	
// 	calibInnerFrameGrid.attach( calibUseMatchesCheck, 0, 0, 1, 1 );
// 	calibInnerFrameGrid.attach( calibUseExGridsCheck, 0, 1, 1, 1 );
// 	calibInnerFrameGrid.attach(  calibUseBundleCheck, 0, 2, 1, 1 );
// 	calibInnerFrameGrid.attach(   calibVisFrameFrame, 1, 0, 2, 5 );
// 	calibInnerFrame.set_label("config");
// 	calibInnerFrame.add( calibInnerFrameGrid );
	
	calibRawRadioBtn.set_label(  "raw dir" );        
	calibProcRadioBtn.set_label( "rgb dir" );         calibProcRadioBtn.join_group(calibRawRadioBtn);
	calibProcRadioBtn.set_active();
	calibRaw2ProcBtn.set_label("Raw -> RGB");
	
	
	
	calibConfigGrid.attach(      calibRawRadioBtn, 0, 0, 1, 1 );
	calibConfigGrid.attach(     calibProcRadioBtn, 1, 0, 1, 1 );
	calibConfigGrid.attach(      calibRaw2ProcBtn, 2, 0, 1, 1 );
	calibConfigGrid.attach(      calibRaw2ProcBtn, 0, 1, 1, 1 );
	calibConfigGrid.attach(  calibUseMatchesCheck, 0, 2, 1, 1 );
	calibConfigGrid.attach(  calibUseExGridsCheck, 0, 3, 1, 1 );
	calibConfigGrid.attach(   calibUseBundleCheck, 0, 4, 1, 1 );
	calibConfigGrid.attach(  calibXAxisIsNegCheck, 0, 5, 1, 1 );
	calibConfigGrid.attach(  calibYAxisIsNegCheck, 0, 6, 1, 1 );
	calibConfigGrid.attach(calibOriginHeightLabel, 0, 7, 1, 1 );
	calibConfigGrid.attach(calibOriginHeightEntry, 1, 7, 1, 1 );
	calibConfigGrid.attach(    calibVisFrameFrame, 0, 8, 1, 1 );
	calibConfigFrame.set_label("Config");
	calibConfigFrame.add( calibConfigGrid );
	
	
	calibToolsGrid.attach(        calibRunCalibBtn, 0, 0, 1, 1 );
	calibToolsGrid.attach( calibRunPointMatcherBtn, 0, 1, 1, 1 );
	calibToolsGrid.attach(    calibRunAlignToolBtn, 0, 2, 1, 1 );
	calibToolsGrid.attach(    calibRunCheckToolBtn, 0, 3, 1, 1 );
	calibToolsGrid.attach(          calibRunMGPBtn, 0, 4, 1, 1 );
	calibToolsGrid.attach(            calibHelpBtn, 0, 5, 1, 1 );
	calibToolsFrame.set_label("Tools");
	calibToolsFrame.add( calibToolsGrid );
	
	calibPageFrameGrid.set_column_spacing(5);
	calibPageFrameGrid.set_row_spacing(5);
	calibPageFrameGrid.attach(         calibConfigFrame, 0, 0, 1, 5 );
	calibPageFrameGrid.attach(          calibToolsFrame, 1, 0, 1, 5 );
	
	
	
	

	
	calibPageFrame.set_label("Calibration");
	calibPageFrame.set_hexpand(true);
	calibPageFrame.set_vexpand(true);
	calibPageFrame.add( calibPageFrameGrid );
	
	
	calibRaw2ProcBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::Raw2ProcClick ) );
	calibRunCalibBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::CalibRunClick ) );
	calibRunPointMatcherBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::CalibPMRunClick ) );
	calibRunAlignToolBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::CalibAlignRunClick ) );
	calibRunCheckToolBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::CalibCheckRunClick ) );
	calibRunMGPBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::CalibMGPClick ) );
	calibHelpBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::CalibHelpClick ) );
	
	calibPageFrame.set_sensitive(false);
	
	
	exportFrame.set_label("Upload");
	exportFrameGrid.set_column_spacing(5);
	mirrorToRaidCheck.set_label("Mirror to RAID");
	raidHostLabel.set_label("RAID host:");
	raidHostEntry.set_text("cssv-camera.bath.ac.uk");
	raidUserLabel.set_label("RAID user:");
	raidUserEntry.set_text("ftpResearcher");
	raidDirLabel.set_label("RAID dir:");
	raidDirEntry.set_text("data/");
	
	raidAddBookmark.set_label("Bookmark raid login");
	raidDelBookmark.set_label("Delete login bookmark");
	raidAddBookmark.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::AddBookmarkClick ) );
	raidDelBookmark.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DelBookmarkClick ) );
	
	exportFrameGrid.attach( mirrorToRaidCheck, 0, 0, 1, 1 );
	exportFrameGrid.attach(     raidHostLabel, 1, 0, 1, 1 );
	exportFrameGrid.attach(     raidHostEntry, 2, 0, 1, 1 );
	exportFrameGrid.attach(     raidUserLabel, 1, 1, 1, 1 );
	exportFrameGrid.attach(     raidUserLabel, 1, 1, 1, 1 );
	exportFrameGrid.attach(     raidUserEntry, 2, 1, 1, 1 );
// 	exportFrameGrid.attach(         exportSep, 3, 0, 1, 1 );
	exportFrameGrid.attach(      raidDirLabel, 1, 2, 1, 1 );
	exportFrameGrid.attach(      raidDirEntry, 2, 2, 1, 1 );
	exportFrameGrid.attach(   raidAddBookmark, 3, 1, 1, 1 );
	exportFrameGrid.attach(   raidDelBookmark, 3, 2, 1, 1 );
	exportFrame.add( exportFrameGrid );
	
	procPageGrid.attach( debayerFrame,  0, 0, 1, 6 );
	procPageGrid.attach(  exportFrame,  0, 7, 1, 1 );
	procPageFrame.add( procPageGrid );
	
	//
	// Put everything in place.
	//
	mainGrid.set_column_spacing(5);
	mainGrid.set_row_spacing(10);
	
	
	botBook.append_page(    procPageFrame,  "Processing" );
	botBook.append_page(   calibPageFrame, "Calibration" );
	
	
	mainGrid.attach(      sourceFrame,   0, 0, 3, 2);
	mainGrid.attach(         visFrame,   0, 2, 3, 1);
	mainGrid.attach(       demonFrame,   0, 3, 3, 1);
	mainGrid.attach(        procFrame,   0, 4, 3, 2);
	
	allAlign.set( 0.5, 0.5, 0.9, 0.9 );
	allAlign.add( mainGrid );
	allBox.pack_start( allAlign );
	
	
	
	
	//
	// Select a session and show the trials, cameras etc...
	//
	sessionsLBox.signal_row_activated().connect( sigc::mem_fun(*this, &MainWindow::SessionBoxRowActivated ) );
	trialsLBox.signal_row_activated().connect( sigc::mem_fun(*this, &MainWindow::TrialBoxRowActivated ) );
	camsLBox.signal_row_activated().connect( sigc::mem_fun(*this, &MainWindow::CamBoxRowActivated ) );
	
	
	
	
	
	
	
	add( allBox );
	show_all_children();
}


void MainWindow::SessionBoxRowActivated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column)
{
	auto selection = sessionsLBox.get_selected();
	if( selection.size() > 0 )
	{
		trialsLBox.clear_items();
		const auto row = selection[0];
		std::string sn = sessionsLBox.get_text(row);
		
		for( auto ti = sessions[sn].trials.begin(); ti != sessions[sn].trials.end(); ++ti )
		{
			trialsLBox.prepend( ti->first );
		}
		
		sessNameLabel.set_text( sn );
		
		
		trialNameLabel.set_text( "(none selected)" );
		trialDebayerStatusLabel.set_text("N/A");
		
		
		for( unsigned c = 0; c < calibVisFrameSpins.size(); ++c )
		{
			calibVisFrameGrid.remove( calibVisFrameSpins[c] );
			calibVisFrameGrid.remove( calibVisFrameLabels[c] );
		}
		calibVisFrameSpins.clear();
		calibVisFrameLabels.clear();
		
		debayerFrame.set_sensitive(true);
	}
}


void MainWindow::TrialBoxRowActivated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column)
{
	TrialBoxRowActivatedImpl();
}

void MainWindow::TrialBoxRowActivatedImpl()
{
	// find the selected session and trial
	auto ssel = sessionsLBox.get_selected();
	auto tsel = trialsLBox.get_selected();
	
	// clear the camera list
	camsLBox.clear_items();
	
	if( ssel.size() > 0 && tsel.size() > 0 )
	{
		// get session and trial names.
		std::string sn = sessionsLBox.get_text( ssel[0] );
		std::string tn = trialsLBox.get_text( tsel[0] );
		
		// re-scan the trial while we're here.
		ScanTrial( sessions[sn].trials[tn] );
		
		
		// update basic bits of the interface
		trialNameLabel.set_text( tn );
		
		int numDebayered = 0;
		for( unsigned cc = 0; cc < sessions[sn].trials[tn].cameras.size(); ++cc)
		{
			ScameraInfo &ci = sessions[sn].trials[tn].cameras[cc];
			std::stringstream ss;
			ss << ci.id;
			camsLBox.append( ss.str() );
			
			if( ci.isDebayered )
				++numDebayered;
		}
		
		// is it a calibration trial?
		bool updateCalib = false;
		std::string cfPth;
		if( sessions[sn].trials[tn].isCalib )
		{
			calibPageFrame.set_sensitive(true);
			
			// do we have "raw" calibration config / grids?
			std::stringstream rss, pss;
			rss << recPaths[0] << "/" << sn << "/" << tn << "/calib.cfg";
			pss << processedSessionsRoot << "/" << sn << "/" << tn << "/calib.cfg";
			boost::filesystem::path rpth( rss.str() ), ppth( pss.str() );
			cout << "rpth: " << boost::filesystem::exists( rpth ) << endl;
			cout << "ppth: " << boost::filesystem::exists( ppth ) << endl;
			if( boost::filesystem::exists( rpth ) )
			{
				if( boost::filesystem::exists( ppth ) && numDebayered == sessions[sn].trials[tn].cameras.size())
				{
					cfPth = ppth.string();
					updateCalib = true;
					calibProcRadioBtn.set_active();
					calibProcRadioBtn.set_sensitive(true);
					calibRawRadioBtn.set_sensitive(true);
					calibRaw2ProcBtn.set_sensitive(true);
					cout << "set a" << endl;
				}
				else if( numDebayered == sessions[sn].trials[tn].cameras.size() )
				{
					cfPth = rpth.string();
					updateCalib = true;
					calibRawRadioBtn.set_active();
					calibProcRadioBtn.set_sensitive(false);
					calibRawRadioBtn.set_sensitive(false);
					calibRaw2ProcBtn.set_sensitive(true);
					cout << "set b" << endl;
				}
				else
				{
					calibRawRadioBtn.set_active();
					calibProcRadioBtn.set_sensitive(false);
					calibRawRadioBtn.set_sensitive(false);
					calibRaw2ProcBtn.set_sensitive(false);
					cout << "set c" << endl;
				}
				
			}
			else if( numDebayered == sessions[sn].trials[tn].cameras.size() )
			{
				calibProcRadioBtn.set_active();
				calibProcRadioBtn.set_sensitive(false);
				calibRawRadioBtn.set_sensitive(false);
				calibRaw2ProcBtn.set_sensitive(false);
			}
			
			
		}
		else
		{
			calibPageFrame.set_sensitive(false);
		}
		
		debayerFrame.set_sensitive(true);
		
		
		for( unsigned c = 0; c < calibVisFrameSpins.size(); ++c )
		{
			calibVisFrameGrid.remove( calibVisFrameSpins[c] );
			calibVisFrameGrid.remove( calibVisFrameLabels[c] );
		}
		calibVisFrameLabels.clear();
		calibVisFrameSpins.clear();
		
		calibVisFrameSpins.resize( sessions[sn].trials[tn].cameras.size() );
		calibVisFrameLabels.resize( sessions[sn].trials[tn].cameras.size() );
		for( unsigned cc = 0; cc < sessions[sn].trials[tn].cameras.size(); ++cc)
		{
			calibVisFrameSpins[cc].set_range(0, 2000);
			calibVisFrameSpins[cc].set_value(10);
			calibVisFrameSpins[cc].set_increments(1,1);
			calibVisFrameGrid.attach(calibVisFrameSpins[cc], 1, cc, 1, 1 );
			
			std::stringstream ss;
			ss << "cam " << std::setw(2) << std::setfill('0') << sessions[sn].trials[tn].cameras[cc].id;
			calibVisFrameLabels[cc].set_text( ss.str() );
			calibVisFrameGrid.attach(calibVisFrameLabels[cc], 0, cc, 1, 1 );
		}
		calibVisFrameGrid.show_all_children();
		
		std::stringstream ss;
		ss << numDebayered << " of " << sessions[sn].trials[tn].cameras.size();
		trialDebayerStatusLabel.set_text( ss.str() );
		
		if( updateCalib )
		{
			UpdateCalibSettingsFrom( cfPth );
		}
	}
}


void MainWindow::CamBoxRowActivated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column)
{
	auto ssel = sessionsLBox.get_selected();
	auto tsel = trialsLBox.get_selected();
	auto csel = camsLBox.get_selected();
	if( ssel.size() > 0 && tsel.size() > 0 && csel.size() > 0)
	{
		std::string sn = sessionsLBox.get_text( ssel[0] );
		std::string tn = trialsLBox.get_text( tsel[0] );
		
		cout << " -- Select a camera! -- " << endl;
		
		int idx = csel[0];
		
		ScameraInfo &ci = sessions[sn].trials[tn].cameras[idx];
		
		std::stringstream ss;
		ss << std::setw(2) << std::setfill('0') << ci.id;
		camNumLabel.set_text( ss.str() );
		
		camRawPathLabel.set_text( SimplifyPath( ci.rawPath ) );
		if( ci.isDebayered )
		{
			camDebayerPathLabel.set_text( SimplifyPath( ci.rgbPath ) );
		}
		else
		{
			camDebayerPathLabel.set_text( "N/A" );
		}
	}
}


std::string MainWindow::SimplifyPath( std::string inpth )
{
	bool got = false;
	std::stringstream ss;
	for( unsigned rcp = 0; rcp < recPaths.size(); ++rcp )
	{
		if( inpth.find( recPaths[rcp] ) == 0 && !got)
		{
			got = true;
			ss << "<recPath" << rcp << ">/";
			ss << inpth.substr( recPaths[rcp].size() );
		}
	}
	
	if( !got && inpth.find( processedSessionsRoot ) == 0)
	{
		got = true;
		ss << "<procPath>/";
		ss << inpth.substr( processedSessionsRoot.size() );
	}
	
	return ss.str();
	
}



void MainWindow::VisTrialClick()
{
	cout << "vis trial" << endl;
	
	auto ssel = sessionsLBox.get_selected();
	auto tsel = trialsLBox.get_selected();
	if( ssel.size() == 0 || tsel.size() == 0 )
	{
		return;
	}
	
	std::string sn = sessionsLBox.get_text( ssel[0] );
	std::string tn = trialsLBox.get_text( tsel[0] );
	
	Strial &trial = sessions[sn].trials[tn];
	
	std::stringstream ss;
	ss << "konsole --workdir ~/ -e " << calibBinariesDir << "/renderSyncedSources ";
	for( unsigned c = 0; c < trial.cameras.size(); ++c )
	{
		if( trial.cameras[c].isDebayered )
		{
			ss << trial.cameras[c].rgbPath << " ";
		}
		else
		{
			ss << trial.cameras[c].rawPath << " ";
		}
	}
	cout << ss.str() << endl;
	std::system( ss.str().c_str() );
	
	
}

void MainWindow::VisCameraClick()
{
	cout << "vis camera" << endl;
	
	auto ssel = sessionsLBox.get_selected();
	auto tsel = trialsLBox.get_selected();
	auto csel = camsLBox.get_selected();
	if( ssel.size() == 0 || tsel.size() == 0 || csel.size() == 0)
	{
		cout << "one of session, trial or camera not selected. Can't vis camera." << endl;
		return;
	}
	
	std::string sn = sessionsLBox.get_text( ssel[0] );
	std::string tn = trialsLBox.get_text( tsel[0] );
	int idx = csel[0];
	
	Strial &trial = sessions[sn].trials[tn];
	
	cout << idx << " | " << trial.cameras[idx].rgbPath << " | " << trial.cameras[idx].rawPath << " |" << endl;
	
	std::stringstream ss;
	ss << "konsole --workdir ~/ -e " << calibBinariesDir << "/renderSyncedSources ";
	if( trial.cameras[idx].isDebayered )
	{
		ss << trial.cameras[idx].rgbPath << " ";
	}
	else
	{
		ss << trial.cameras[idx].rawPath << " ";
	}
	
	cout << ss.str() << endl;
	std::system( ss.str().c_str() );
}

void MainWindow::AddBookmarkClick()
{
	std::string username = raidUserEntry.get_text();
// 	if( username.compare("ftpResearcher") == 0 )
// 	{
// 		cout << "ftpResearcher bookmark should already exist, not adding" << endl;
// 		return;
// 	}
// 	else
	{
		std::stringstream ss;
		ss << "konsole --workdir ~/ -e lftp -u " << username 
		                                         << " ftps://" 
		                                         << raidHostEntry.get_text() 
		                                         << " -e \"bookmark add " << username << "-" << raidHostEntry.get_text() << "\"";
		std::system( ss.str().c_str() );
	}
}

void MainWindow::DelBookmarkClick()
{
	std::string username = raidUserEntry.get_text();
	if( username.compare("ftpResearcher") == 0 )
	{
		cout << "Not deleting ftpResearcher login bookmark" << endl;
		return;
	}
	else
	{
		std::stringstream ss;
		ss << "konsole -e lftp -e bookmark del " << username << "-" << raidHostEntry.get_text();
		std::system( ss.str().c_str() );
	}
}


void MainWindow::UpdateCalibSettingsFrom( std::string cfgPath)
{
	try
	{
		libconfig::Config cfg;
		cfg.readFile( cfgPath.c_str() );
		
		if( cfg.exists("alignXisNegative" ) )
		{
			calibXAxisIsNegCheck.set_active(cfg.lookup("alignXisNegative"));
		}
		if( cfg.exists("alignYisNegative" ) )
		{
			calibYAxisIsNegCheck.set_active(cfg.lookup("alignYisNegative"));
		}
		if( cfg.exists("targetDepth") )
		{
			std::stringstream ss;
			ss << (float)cfg.lookup("targetDepth");
			calibOriginHeightEntry.set_text( ss.str() );
		}
		
		calibUseMatchesCheck.set_active(  cfg.exists("matchesFile")  );
		if( cfg.exists("useExistingGrids") )
		{
			calibUseExGridsCheck.set_active( cfg.lookup("useExistingGrids") );
		}
		if( cfg.exists("useSBA") )
		{
			calibUseBundleCheck.set_active( cfg.lookup("useSBA") );
		}
		if( cfg.exists("frameInds") )
		{
			auto &fi = cfg.lookup("frameInds");
			for( int cc = 0; cc < fi.getLength(); ++cc )
			{
				calibVisFrameSpins[cc].set_value( (int)fi[cc] );
			}
		}
		
	}
	catch( libconfig::SettingException &e)
	{
		cout << "Setting error: " << endl;
		cout << e.what() << endl;
		cout << e.getPath() << endl;
		exit(0);
	}
}
