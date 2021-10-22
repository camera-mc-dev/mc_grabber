#include "mainWindow.h"


#include <boost/filesystem.hpp>

#include <set>
#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;




MainWindow::MainWindow() : sessionsLBox(1), trialsLBox(1), camsLBox(1), debayerJobsList(1)
{
	recPaths = { "/data/raid0/recording/", "/data/raid1/recording/" };
	
	ScanForSessions();
	
	CreateInterface();
}


MainWindow::~MainWindow()
{
}


void MainWindow::ScanForSessions()
{
	//
	// We make the assumption that the recording tool creates recordings in:
	// /data/raid0/recording/<session>/<trial>
	// /data/raid1/recording/<session>/<trial>
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
	
	for(unsigned rpc = 0; rpc < recPaths.size(); ++rpc )
	{
		boost::filesystem::path p(recPaths[rpc]);
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
	// do we have a meta-file?
	
	// scan for trials.
	//
	// Directories inside of a session are assumed to be trials iff they have 
	// the format <stuff>_??
	//
	// which is to say, that the size()-3 character is '_'
	// TODO: even better test?
	
	for( unsigned rpc = 0; rpc < recPaths.size(); ++rpc )
	{
		std::stringstream ss;
		ss << recPaths[rpc] << "/" << sess.name << "/";
		
		boost::filesystem::path p(ss.str());
		if( boost::filesystem::exists(p) && boost::filesystem::is_directory(p) )
		{
			sess.paths.push_back( ss.str() );
		}
		
		ss.str("");
		ss << recPaths[rpc] << "/" << sess.name << "/raw/";
		
		boost::filesystem::path p2(ss.str());
		if( boost::filesystem::exists(p2) && boost::filesystem::is_directory(p2) )
		{
			sess.paths.push_back( ss.str() );
		}
		
	}
	
	cout << "sess: " << sess.name << endl;
	std::vector< std::string > calibFiles;
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
							ti->second.paths.push_back( s );
						}
						else
						{
							sess.trials[ trialName ].paths.push_back(s);
							sess.trials[ trialName ].name = trialName;
							sess.trials[ trialName ].sessionName = sess.name; // useful to keep with trial too.
						}
					}
				}
				else if( s.find(".calib") != std::string::npos )
				{
					calibFiles.push_back(s);
				}
			}
		}
	}
	
	std::vector< std::string > calibTrials;
	for( auto ti = sess.trials.begin(); ti != sess.trials.end(); ++ti )
	{
		if( ti->first.find("calib_") == 0 )
			calibTrials.push_back(ti->first);
		ScanTrial( ti->second );
	}
	
	//
	// Do we have an active calibration?
	// How would we know?
	// Scan the session dir for any .calib files, which should be symLinks into a specific trial.
	//
	if( calibFiles.size() > 0 )
	{
		//
		// TODO: check that these are symlinks, and if they are, figure out which 
		//       calib trial they point to.
		//
	}
	else if( calibTrials.size() > 0 )
	{
		//
		// Default to this otherwise.
		//
		sess.activeCalibTrial = calibTrials[0];
	}
	else
	{
		sess.activeCalibTrial = "N/A";
	}
	
	if( sess.activeCalibTrial.compare("N/A") != 0 )
	{
		Strial &act = sess.trials[ sess.activeCalibTrial ];
		if( act.hasAlignedCalib )
		{
			sess.calibStatus = "(3) has aligned";
		}
		else if( act.hasInitialCalib )
		{
			sess.calibStatus = "(2) has initial";
		}
		else if( act.hasGrids )
		{
			sess.calibStatus = "(1) has grids files";
		}
		else
		{
			sess.calibStatus = "(0) no calib";
		}
		
		if( act.hasMatches )
		{
			sess.matchesStatus = "(1) has matches file";
		}
		else
		{
			sess.matchesStatus = "(1) no matches file";
		}
	}
	
}


void MainWindow::ScanTrial( Strial &trial )
{
	cout << "\ttrial: " << trial.name << endl;
	
	//
	// Have we got a meta-file?
	//
	
	
	//
	// Scan for cameras.
	//
	// We assume the recording software dumps cameras to numbered directories: 00,01,02,03,..,99
	//
	for( unsigned cc = 0; cc < 99; ++cc )
	{
		ScanCamera( trial, cc );
	}
	
	//
	// Check calibration things (if this is a calib trial)
	//
	if( trial.name.find("calib_") == 0 )
	{
		//
		// Have we got:
		//
		//  1) calib files?
		//  2) matches file?
		//  3) floor-aligned calib files?
		//
		//  we assume these things are raid0
		//
		
		std::stringstream ss;
		ss << "/data/raid0/recording/" << trial.sessionName << "/rgb/" << trial.name << "/";
		std::string calibFilesPath( ss.str() );
		
		bool gotCalibs = true;
		for( unsigned cc = 0; cc < trial.cameras.size(); ++cc )
		{
			ss.str("");
			int camNum = trial.cameras[cc].id;
			ss << calibFilesPath << std::setw(2) << std::setfill('0') << camNum << ".mp4.calib";
			boost::filesystem::path p(ss.str());
			if( !boost::filesystem::exists(p) )
				gotCalibs = false;
		}
		
		trial.isCalib = true;
		if( !gotCalibs )
		{
			trial.hasInitialCalib   = false;
		}
		
		{
			ss.str("");
			ss << calibFilesPath << "/matches";
			boost::filesystem::path p(ss.str());
			if( boost::filesystem::exists(p) )
				trial.hasMatches = true;
		}
		
		
		gotCalibs = true;
		for( unsigned cc = 0; cc < trial.cameras.size(); ++cc )
		{
			ss.str("");
			int camNum = trial.cameras[cc].id;
			ss << calibFilesPath << std::setw(2) << std::setfill('0') << camNum << ".mp4.floorAligned.calib";
			boost::filesystem::path p(ss.str());
			if( !boost::filesystem::exists(p) )
				gotCalibs = false;
		}
		
		if( !gotCalibs )
		{
			trial.hasAlignedCalib  = false;
		}
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
	for( unsigned tpc = 0; tpc < trial.paths.size(); ++tpc )
	{
		std::stringstream ss;
		ss << trial.paths[tpc] << "/" << std::setw(2) << std::setfill('0') << camNum;
		
		boost::filesystem::path tp( trial.paths[tpc] );
		if( boost::filesystem::exists(tp) && boost::filesystem::is_directory(tp))
		{
			boost::filesystem::path cp( ss.str() );
			if( boost::filesystem::exists(cp) && boost::filesystem::is_directory(cp) )
			{
				if( !gotRawPath )
				{
					camInfo.rawPath = cp.string();
					
					gotRawPath = true;
				}
				else
				{
					std::stringstream msgss;
					msgss << "Warning: More than one location for raw camera image directory: " << endl;
					msgss << "\ttrial: " << trial.name << endl;
					msgss << "\t  (1): " << camInfo.rawPath << endl;
					msgss << "\t  (2): " << cp << endl;
					msgss << "\t  keeping (1) " << endl;
					
					cout << msgss.str() << endl;
					messageLog.push_back( msgss.str() );
					
				}
			}
		}
	}
	
	
	
	//
	// Check for debayered data
	//
	// We're assuming that we debayer to .mp4 videos still.
	//
	Ssession &sess = sessions[ trial.sessionName ];
	bool gotRGBPath = false;
	for( unsigned spc = 0; spc < sess.paths.size(); ++spc )
	{
		std::stringstream ss;
		ss << sess.paths[spc] << "/rgb/" << trial.name << "/" << std::setw(2) << std::setfill('0') << camNum << ".mp4";
		
		
		boost::filesystem::path cp( ss.str() );
		if( boost::filesystem::exists(cp) && !boost::filesystem::is_directory(cp))
		{
			if( !gotRGBPath )
			{
				camInfo.rgbPath = cp.string();
				gotRGBPath = true;
			}
			else
			{
				std::stringstream msgss;
				msgss << "Warning: More than one location for colour camera data: " << endl;
				msgss << "\ttrial: " << trial.name << endl;
				msgss << "\t  (1): " << camInfo.rgbPath << endl;
				msgss << "\t  (2): " << cp << endl;
				msgss << "\t  keeping (1) " << endl;
				
				cout << msgss.str() << endl;
				messageLog.push_back( msgss.str() );
			}
		}
	}
	
	if( gotRGBPath )
	{
		camInfo.isDebayered = true;
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
	sessActiveCalibLabel0.set_text(  "active calib:"  );   sessActiveCalibLabel.set_text("N/A");
	sessCalibStatusLabel0.set_text(  "calib status:"  );   sessCalibStatusLabel.set_text("N/A");
	sessMatchesStatusLabel0.set_text("matches     :"  );   sessMatchesStatusLabel.set_text("N/A");
	
	sessFrameGrid.attach( sessNameLabel0,  0, 0, 1, 1);
	sessFrameGrid.attach( sessNameLabel,  1, 0, 1, 1);
	
	
	
	sessFrameGrid.attach(   sessActiveCalibLabel0,  0, 2, 1, 1);
	sessFrameGrid.attach(   sessCalibStatusLabel0,  0, 3, 1, 1);
	sessFrameGrid.attach( sessMatchesStatusLabel0,  0, 4, 1, 1);
	
	sessFrameGrid.attach(   sessActiveCalibLabel ,  1, 2, 1, 1);
	sessFrameGrid.attach(   sessCalibStatusLabel ,  1, 3, 1, 1);
	sessFrameGrid.attach( sessMatchesStatusLabel ,  1, 4, 1, 1);
	
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
	debayerActionBtn.set_label("Process debayer jobs");
	debayerHelpBtn.set_label("Help!");
	
	debayerModeFrame.set_label( "Debayer Algorithm" );
	debayerModeRBCVEA.set_label( "OpenCV_EA" );
	debayerModeRBLED.set_label( "LED" );           debayerModeRBLED.join_group(debayerModeRBCVEA);
	debayerModeRBFCNN.set_label( "FCNN" );         debayerModeRBFCNN.join_group(debayerModeRBCVEA);
	debayerModeRBLED.set_active();
	
	debayerModeGrid.attach( debayerModeRBCVEA, 0, 0, 1, 1 );
	debayerModeGrid.attach( debayerModeRBLED,  0, 1, 1, 1 );
	debayerModeGrid.attach( debayerModeRBFCNN, 0, 2, 1, 1 );
	debayerModeFrame.add( debayerModeGrid );
	
	debayerJobsList.set_column_title(0,"jobs list");
	debayerJobsFrame.add( debayerJobsList );
	debayerJobsFrame.set_hexpand(true);
	debayerJobsFrame.set_vexpand(true);
	debayerJobsScroll.add( debayerJobsFrame );
	
	demonStatusFrame.set_label("Debayer daemon status");
	demonStatusLabel.set_label("(waiting info...)");
	demonJobsLabel.set_text("? jobs remaining");
	demonStatusGrid.attach( demonStatusLabel, 0, 0, 1, 1);
	demonStatusGrid.attach( demonJobsLabel, 0, 1, 1, 1);
	demonStatusFrame.add( demonStatusGrid );
	
	debayerFrameGrid.set_column_spacing(5);
	debayerFrameGrid.set_row_spacing(5);
	debayerFrameGrid.attach( debayerHelpBtn,   0, 0, 1, 1 );
	debayerFrameGrid.attach( debayerSessBtn,   0, 1, 1, 1 );
	debayerFrameGrid.attach( debayerTrialBtn,  0, 2, 1, 1 );
	debayerFrameGrid.attach( debayerCameraBtn, 0, 3, 1, 1 );
	debayerFrameGrid.attach( debayerRemoveBtn, 1, 1, 1, 1 );
	debayerFrameGrid.attach( debayerModeFrame, 1, 2, 1, 2 );
	debayerFrameGrid.attach( debayerJobsScroll,  2, 0, 2, 4 );
	
	debayerFrameGrid.attach( demonStatusFrame, 4, 0, 1, 2 );
	debayerFrameGrid.attach( debayerActionBtn, 4, 2, 1, 2 );
	
	
	
	
	debayerFrame.set_label("Debayering");
	debayerFrame.add( debayerFrameGrid );
	
	debayerSep.set_hexpand(true);
	debayerSep.set_vexpand(true);
	
	debayerFrame.set_sensitive(false);
	
	CheckDemonStatus();
	
	
	debayerHelpBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DebayerHelpClick ) );
	debayerSessBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DebayerAddSessionClick ) );
	debayerTrialBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DebayerAddTrialClick ) );
	debayerCameraBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DebayerAddCameraClick ) );
	debayerRemoveBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DebayerRemoveJobClick ) );
	debayerActionBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::DebayerProcessJobsClick ) );
	
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &MainWindow::CheckDemonStatus), 5000);
	
	
	//
	// Calibration
	//
	calibRunCalibBtn.set_label("Run calibration");
	calibRunPointMatcherBtn.set_label("Run point matcher");
	calibRunAlignToolBtn.set_label("Run alignment");
	calibRunCheckToolBtn.set_label("Run check");
	calibSetActiveBtn.set_label("Set active");
	calibHelpBtn.set_label("Help!");
	
	calibUseMatchesCheck.set_label("use matches");
	calibUseExGridsCheck.set_label("use existing grids");
	calibUseBundleCheck.set_label("use bundle adjust");
	
	
	calibInnerFrameGrid.attach(     calibRunCalibBtn, 0, 0, 1, 1 );
	calibInnerFrameGrid.attach( calibUseMatchesCheck, 0, 1, 1, 1 );
	calibInnerFrameGrid.attach( calibUseExGridsCheck, 0, 2, 1, 1 );
	calibInnerFrameGrid.attach(  calibUseBundleCheck, 0, 3, 1, 1 );
	calibInnerFrame.add( calibInnerFrameGrid );
	
	calibVisFrameScroll.add( calibVisFrameGrid );
	calibVisFrameFrame.add( calibVisFrameScroll );
	calibVisFrameFrame.set_label("vis frame (for point matcher etc) " );
	
	calibFrameGrid.set_column_spacing(5);
	calibFrameGrid.set_row_spacing(5);
	calibFrameGrid.attach(         calibInnerFrame, 0, 0, 1, 4 );
	calibFrameGrid.attach( calibRunPointMatcherBtn, 1, 0, 1, 1 );
	calibFrameGrid.attach(    calibRunAlignToolBtn, 1, 1, 1, 1 );
	calibFrameGrid.attach(    calibRunCheckToolBtn, 1, 2, 1, 1 );
	calibFrameGrid.attach(       calibSetActiveBtn, 1, 3, 1, 1 );
	calibFrameGrid.attach(      calibVisFrameFrame, 3, 0, 2, 4 );
	calibFrameGrid.attach(            calibHelpBtn, 6, 0, 1, 1 );
	
	calibFrame.set_label("Calibration");
	calibFrame.set_hexpand(true);
	calibFrame.set_vexpand(true);
	calibFrame.add( calibFrameGrid );
	
	
	
	calibRunCalibBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::CalibRunClick ) );
	calibRunPointMatcherBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::CalibPMRunClick ) );
	calibRunAlignToolBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::CalibAlignRunClick ) );
	calibRunCheckToolBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::CalibCheckRunClick ) );
	calibHelpBtn.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::CalibHelpClick ) );
	
	calibFrame.set_sensitive(false);
	
	//
	// Put everything in place.
	//
	mainGrid.set_column_spacing(5);
	mainGrid.set_row_spacing(10);
	
	
	mainGrid.attach( sessTrialCamGrid,   0, 0, 3, 2);
	mainGrid.attach(     debayerFrame,   0, 2, 3, 1);
	mainGrid.attach(       calibFrame,   0, 3, 3, 1);
	
	allBox.pack_start( mainGrid );
	
	
	
	
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
		
		
		sessActiveCalibLabel.set_text(sessions[sn].activeCalibTrial);
		sessCalibStatusLabel.set_text(sessions[sn].calibStatus);
		sessMatchesStatusLabel.set_text(sessions[sn].matchesStatus);
		
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
	auto ssel = sessionsLBox.get_selected();
	auto tsel = trialsLBox.get_selected();
	camsLBox.clear_items();
	
	if( ssel.size() > 0 && tsel.size() > 0 )
	{
		std::string sn = sessionsLBox.get_text( ssel[0] );
		std::string tn = trialsLBox.get_text( tsel[0] );
		
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
		
		
		if( sessions[sn].trials[tn].isCalib )
		{
			calibFrame.set_sensitive(true);
		}
		else
		{
			calibFrame.set_sensitive(false);
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
		camRawPathLabel.set_text( ci.rawPath );
		if( ci.isDebayered )
		{
			camDebayerPathLabel.set_text( ci.rgbPath );
		}
		else
		{
			camDebayerPathLabel.set_text( "N/A" );
		}
	}
}



