#ifdef USE_SISO

#include "controls.h"
#include "grab.h"

#include <ctime>
#include <iomanip>
#include <boost/filesystem.hpp>
#include <giomm/file.h>

void GUIThread( GUIThreadData *gtdata )
{
	int argc=0; char** argv = NULL;

	// tell the main thread to pause until this thread is ready
	SignalHandler * handler = new SignalHandler();
	gtdata->signalHandler = handler;
	
	auto app = Gtk::Application::create(argc, argv, "recording controls");

	ControlsWindow window( gtdata->grabber);
	window.set_default_size(400, 200);

	gtdata->window = &window;

	gtdata->signalHandler->ready = true;
	gtdata->signalHandler->cv.notify_all();

	app->run(window);

	gtdata->done = true;
	gtdata->window->UpdateSessionConfig();

}


ControlsWindow::ControlsWindow(AbstractGrabber *in_grabber)
{
	// Generate the config parser
	sessionConfig = new ConfigParser(in_grabber->GetNumCameras());
	// check if the config has found a pre-existing session for today
	if (sessionConfig->showDialog)
	{
		ShowDialog();
	}

	// TODO: Get input from the grabber class.
	grabber = in_grabber;
	numCameras = grabber->GetNumCameras();
	cout << "controls, numCameras: " << numCameras << endl;
	
	//
	// Configure the overall container
	//
	allBox.set_border_width(5);
	allBox.set_orientation(Gtk::ORIENTATION_VERTICAL);
	
	//
	// Dropdown menu
	//

	//Create actions for menus and toolbars:
	m_refActionGroup = Gtk::ActionGroup::create();

	//File menu:
	m_refActionGroup->add(Gtk::Action::create("FileMenu", "File"));
	m_refActionGroup->add(Gtk::Action::create("FileNew",
	          Gtk::Stock::NEW, "_New", "Create a new session"),
	      sigc::mem_fun(*this, &ControlsWindow::MenuFileNew));
	
	m_refActionGroup->add(Gtk::Action::create("FileLoad", 
		Gtk::Stock::OPEN, "Load", "Load a pre-existing session"),
	      sigc::mem_fun(*this, &ControlsWindow::MenuFileLoad));

	m_refActionGroup->add(Gtk::Action::create("FileMove", 
		Gtk::Stock::DIRECTORY, "Move/Rename", "Move existing session and all trials to different directory"),
	      sigc::mem_fun(*this, &ControlsWindow::MenuFileMove));

	m_refActionGroup->add(Gtk::Action::create("FileSave", 
		Gtk::Stock::SAVE, "Save", "Save existing session"),
	      sigc::mem_fun(*this, &ControlsWindow::MenuFileSave));
	
	m_refActionGroup->add(Gtk::Action::create("FileQuit", Gtk::Stock::QUIT),
	      sigc::mem_fun(*this, &ControlsWindow::MenuFileQuit));

	//Help menu:
	m_refActionGroup->add( Gtk::Action::create("HelpMenu", "Help") );
	m_refActionGroup->add( Gtk::Action::create("HelpAbout", Gtk::Stock::HELP),
	      sigc::mem_fun(*this, &ControlsWindow::MenuFileUnimplemented) );

	m_refUIManager = Gtk::UIManager::create();
	m_refUIManager->insert_action_group(m_refActionGroup);

	add_accel_group(m_refUIManager->get_accel_group());

	//Layout the actions in a menubar:
	Glib::ustring ui_info = 
	    "<ui>"
        "  <menubar name='MenuBar'>"
        "    <menu action='FileMenu'>"
        "      <menuitem action='FileNew'/>"
		"      <menuitem action='FileLoad'/>"
		"      <menuitem action='FileMove'/>"
		"      <menuitem action='FileSave'/>"
        "      <menuitem action='FileQuit'/>"
        "    </menu>"
        "    <menu action='HelpMenu'>"
        "      <menuitem action='HelpAbout'/>"
        "    </menu>"
        "  </menubar>"
        "</ui>";

	try
	{
		m_refUIManager->add_ui_from_string(ui_info);
	}
	catch(const Glib::Error& ex)
	{
		std::cerr << "building menus failed: " <<  ex.what();
	}

	//Get the menubar widgets and add them to a container widget:
	Gtk::Widget* pMenubar = m_refUIManager->get_widget("/MenuBar");
	if(pMenubar)
		allBox.pack_start(*pMenubar, Gtk::PACK_SHRINK);

	//
	// Create start/stop controls
	//
	xResLabel.set_text("width");
	xResScale.set_range(64, 2560);
	xResScale.set_value(sessionConfig->videoWidth);
	xResScale.set_hexpand(true);
	
	yResLabel.set_text("height");
	yResScale.set_range(2,2048);
	yResScale.set_value(sessionConfig->videoHeight);
	yResScale.set_hexpand(true);
	
	fpsLabel.set_text("fps");
	fpsScale.set_range(1,200);
	fpsScale.set_value(sessionConfig->fps);
	fpsScale.set_hexpand(true);

	obsFpsA.set_text("observed fps:");
	obsFpsA.set_hexpand(true);
	obsFpsB.set_text("(start grabbing)");
	obsFpsB.set_hexpand(true);
	
	durLabel.set_text("duration (s)");
	durScale.set_range(1,20);
	durScale.set_value(sessionConfig->duration);
	durScale.set_hexpand(true);
	
	sessionNameLabel.set_text("Session:");
	trialNameLabel.set_text("Trial:");
	
	sessionNameEntry.set_text(sessionConfig->sessionName);
	trialNameEntry.set_text(sessionConfig->trialName);
	
	trialNumberSpin.set_range(0, 99);
	trialNumberSpin.set_value(sessionConfig->trialNum);
	trialNumberSpin.set_increments(1,1);
	
	
	sessionNameEntry.set_hexpand(true);
	trialNameEntry.set_hexpand(true);
	trialNumberSpin.set_hexpand(true);
	
	startGrabButton.set_label("Start Grabbing");
	stopGrabButton.set_label("Stop Grabbing");
	
	calibModeCheckBtn.set_label("Calibration mode");
	calibModeCheckBtn.set_active(false);
	
	gridDetectCheckBtn.set_label("Live Detect Grid");
	gridDetectCheckBtn.set_active(false);
	
	gridLabel.set_text("grid rows/cols");
	gridRowsSpin.set_range(1, 99);
	gridRowsSpin.set_value(9);
	gridRowsSpin.set_increments(1,1);
	gridColsSpin.set_range(1, 99);
	gridColsSpin.set_value(10);
	gridColsSpin.set_increments(1,1);
	gridLightOnDarkCheck.set_label("light on dark");
	gridLightOnDarkCheck.set_active(false);
	
	ssGrid.attach( xResLabel, 0, 0, 1, 1); ssGrid.attach( xResScale, 1, 0, 3, 1 );
	ssGrid.attach( yResLabel, 0, 1, 1, 1); ssGrid.attach( yResScale, 1, 1, 3, 1 );
	ssGrid.attach( fpsLabel,  0, 2, 1, 1); ssGrid.attach( fpsScale,  1, 2, 3, 1 );
	ssGrid.attach( durLabel,  0, 3, 1, 1); ssGrid.attach( durScale,  1, 3, 3, 1 );
	ssGrid.attach( obsFpsA,   0, 4, 1, 1); ssGrid.attach( obsFpsB,   1, 4, 1, 1 );
	ssGrid.attach(    sessionNameLabel, 0, 5, 1, 1 );
	ssGrid.attach(    sessionNameEntry, 1, 5, 4, 1 );
	ssGrid.attach(      trialNameLabel, 0, 6, 1, 1 );
	ssGrid.attach(      trialNameEntry, 1, 6, 2, 1 );
	ssGrid.attach(     trialNumberSpin, 3, 6, 1, 1 );
	ssGrid.attach(     startGrabButton, 0, 7, 2, 1 );
	ssGrid.attach(     stopGrabButton,  2, 7, 2, 1 );
	ssGrid.attach(   calibModeCheckBtn, 0, 8, 2, 1 );
	ssGrid.attach(  gridDetectCheckBtn, 2, 8, 4, 1 );
	ssGrid.attach(           gridLabel, 0, 9, 1, 1 );
	ssGrid.attach(        gridRowsSpin, 1, 9, 1, 1 );
	ssGrid.attach(        gridColsSpin, 2, 9, 1, 1 );
	ssGrid.attach(gridLightOnDarkCheck, 3, 9, 1, 1 );
	
	ssFrame.set_label("acquisition");
	ssFrame.add( ssGrid );
	
	allBox.pack_start( ssFrame );
	
	stopGrabButton.set_sensitive(false);
	
	//
	// Create per-camera gain and exposure controls.
	//
	camControlGrid.set_column_spacing(4);
	camControlGrid.set_row_spacing(4);
	camFrames.resize( numCameras );
	camGrids.resize( numCameras );
	camExpLabels.resize(numCameras);
	camExpScales.resize(numCameras);
	camGainLabels.resize(numCameras);
	camGainScales.resize(numCameras);
	camDisplayedChecks.resize( numCameras );
	for( unsigned cc = 0; cc < numCameras; ++cc )
	{
		std::stringstream ss; ss << "camera " << cc;
		camFrames[cc].set_label( ss.str() );
		camExpLabels[cc].set_label("exp 1/x (s)");
		camGainLabels[cc].set_label("gain");
		camExpScales[cc].set_range(1,1000);
		camGainScales[cc].set_range(1,16);
		
		camFrames[cc].set_hexpand(true);
		camExpLabels[cc].set_hexpand(false);
		camGainLabels[cc].set_hexpand(false);
		camExpScales[cc].set_hexpand(true);
		camGainScales[cc].set_hexpand(true);
		camExpScales[cc].set_value(sessionConfig->camSettings[cc].exposure);
		camGainScales[cc].set_value(sessionConfig->camSettings[cc].gain);
		
		camDisplayedChecks[cc].set_label("displayed");
		camDisplayedChecks[cc].set_active(sessionConfig->camSettings[cc].displayed);
		
		camGrids[cc].set_hexpand(true);
		camGrids[cc].attach( camExpLabels[cc] , 0, 0, 1, 1 );
		camGrids[cc].attach( camExpScales[cc] , 1, 0, 1, 1 );
		camGrids[cc].attach( camGainLabels[cc], 0, 1, 1, 1 );
		camGrids[cc].attach( camGainScales[cc], 1, 1, 1, 1 );
		camGrids[cc].attach( camDisplayedChecks[cc], 0, 2, 2, 1 );
		camFrames[cc].add( camGrids[cc] );
		
		camControlGrid.attach( camFrames[cc], cc%3, cc/3, 1, 1);
	}
	camControlSetButton.set_label("Set Exp/Gain");
	int cc = numCameras;
	camControlGrid.attach( camControlSetButton, cc%3, cc/3, 1, 1);
	
	allBox.pack_start( camControlGrid );
	
	
	
	allCamExpFrame.set_label("Set all cameras");
	allCamExpSetButton.set_label("Set all");
	allCamExpLabel.set_label("exposure");
	allCamExpScale.set_range(1,1000);
	allCamGainLabel.set_label("gain");
	allCamGainScale.set_range(1,16);

	baseGainFrame.set_label("analog base gain");	
	baseGainButton.set_label("Set All");
	baseGainLabel.set_label("base gain");
	baseGain00RB.set_label("0db");
	baseGain06RB.set_label("6db");
	baseGain12RB.set_label("12db");
	baseGain06RB.join_group( baseGain00RB );
	baseGain12RB.join_group( baseGain00RB );
	baseGain00RB.set_active();
	
	allCamGainScale.set_value(1);
	allCamExpScale.set_value(250);
	
	allCamExpGrid.set_hexpand(true);
	allCamExpLabel.set_hexpand(false);
	allCamExpScale.set_hexpand(true);
	allCamGainLabel.set_hexpand(false);
	allCamGainScale.set_hexpand(true);
	allCamExpGrid.attach( allCamExpLabel,     0, 0, 1, 1 );
	allCamExpGrid.attach( allCamExpScale,     1, 0, 1, 1 );
	allCamExpGrid.attach( allCamGainLabel,    0, 1, 1, 1 );
	allCamExpGrid.attach( allCamGainScale,    1, 1, 1, 1 );
	allCamExpGrid.attach( allCamExpSetButton, 2, 0, 1, 2 );
	allCamExpFrame.add( allCamExpGrid );
	
	allBox.pack_start( allCamExpFrame );
	
	
	baseGainGrid.attach( baseGainLabel,      0, 0, 1, 1 );
	baseGainGrid.attach( baseGain00RB,       1, 0, 1, 1 );
	baseGainGrid.attach( baseGain06RB,       2, 0, 1, 1 );
	baseGainGrid.attach( baseGain12RB,       3, 0, 1, 1 );
	baseGainGrid.attach( baseGainButton,     4, 0, 1, 1 );
	baseGainFrame.add( baseGainGrid );
	allBox.pack_start( baseGainFrame );

	
	
	
	
	startGrabButton.signal_clicked().connect( sigc::mem_fun(*this, &ControlsWindow::StartGrabbing ) );
	stopGrabButton.signal_clicked().connect( sigc::mem_fun(*this, &ControlsWindow::StopGrabbing ) );
	calibModeCheckBtn.signal_toggled().connect( sigc::mem_fun(*this, &ControlsWindow::CalibModeToggle ) );
	camControlSetButton.signal_clicked().connect( sigc::mem_fun(*this, &ControlsWindow::SetGainsAndExposures ) );
	allCamExpSetButton.signal_clicked().connect( sigc::mem_fun(*this, &ControlsWindow::SetAllGainsAndExposures ) );
	baseGainButton.signal_clicked().connect( sigc::mem_fun(*this, &ControlsWindow::SetAllBaseGains ) );
	
	
	
	//
	// Image sender controls.
	//
	shareLabel.set_text("camera to send:");
	shareLabel.set_hexpand(true);
	shareSpinner.set_hexpand(true);
	shareSpinner.set_range(0, grabber->GetNumCameras()-1);
	shareSpinner.set_value(0);
	shareSpinner.set_increments(1,1);
	shareGrid.attach( shareLabel,   0, 0, 1, 1);
	shareGrid.attach( shareSpinner, 1, 0, 1, 1);
	shareFrame.add( shareGrid );
	shareFrame.set_label("sharing");
	allBox.pack_start( shareFrame );
	
	
	add( allBox );
	show_all_children();
}

ControlsWindow::~ControlsWindow()
{
}

void ControlsWindow::SetWidgetValues()
{
	xResScale.set_value(sessionConfig->videoWidth);
	yResScale.set_value(sessionConfig->videoHeight);
	fpsScale.set_value(sessionConfig->fps);
	durScale.set_value(sessionConfig->duration);
	sessionNameEntry.set_text(sessionConfig->sessionName);
	trialNameEntry.set_text(sessionConfig->trialName);
	trialNumberSpin.set_value(sessionConfig->trialNum);
	for( unsigned cc = 0; cc < numCameras; ++cc )
	{
		camExpScales[cc].set_value(sessionConfig->camSettings[cc].exposure);
		camGainScales[cc].set_value(sessionConfig->camSettings[cc].gain);
		camDisplayedChecks[cc].set_active(sessionConfig->camSettings[cc].displayed);
	}

}
void ControlsWindow::UpdateSessionConfig(bool save)
{
	// if in calibmode, dont store the gtk settings.
	if (calibModeCheckBtn.get_active())
	{
		return;
	}
	sessionConfig->sessionName = sessionNameEntry.get_text();
	sessionConfig->videoWidth = xResScale.get_value();
	sessionConfig->videoHeight = yResScale.get_value();
	sessionConfig->fps = fpsScale.get_value();
	sessionConfig->duration = durScale.get_value();
	sessionConfig->trialNum = trialNumberSpin.get_value();
	sessionConfig->trialName = trialNameEntry.get_text();
	
	for( unsigned cc = 0; cc < numCameras; ++cc )
	{
		sessionConfig->camSettings[cc].exposure = camExpScales[cc].get_value();
		sessionConfig->camSettings[cc].gain = camGainScales[cc].get_value();
		sessionConfig->camSettings[cc].displayed = camDisplayedChecks[cc].get_active();	
	}
	if (save)
	{
		sessionConfig->Save();	
	}
	
}

void ControlsWindow::StartGrabbing()
{
	cout << "Start grabbing" << endl;

	// save the session config so we can reload incase of crash
	UpdateSessionConfig();

	meanfps = -1.0;
	
	startGrabButton.set_sensitive(false);
	fpsScale.set_sensitive(false);
	xResScale.set_sensitive(false);
	yResScale.set_sensitive(false);
	durScale.set_sensitive(false);
	calibModeCheckBtn.set_sensitive(false);
	
	stopGrabButton.set_sensitive(true);
	baseGainButton.set_sensitive(false);
	
	fpsTimerConnection = Glib::signal_timeout().connect( sigc::mem_fun(*this, &ControlsWindow::SetObservedFPS), 500 );
	
	
	if( calibModeCheckBtn.get_active() )
	{
		InitialiseCalibrationSession();
	}
	grabbing = true;
	
	
	// hard coded memory limit.
	// we know we have 128GB RAM on the system, so we'll set a 120GB limit on recording space.
	// I know this looks dopey, but it is clearer than having lots of uncountable 0s
	//             
	unsigned long memLimit = 120 * 1000 * 1000 * 1000;
	
	unsigned long fps = fpsScale.get_value();
	long int resX = xResScale.get_value();
	long int resY = yResScale.get_value();
	long int recDuration = durScale.get_value();
	
	grabber->SetFPS( fps, 0 );
	grabber->SetResolution( resX, resY );
	xResScale.set_value( resX );
	yResScale.set_value( resY );
	fpsScale.set_value( fps );
	
	unsigned long memUse = resX * resY * fps * recDuration * grabber->GetNumCameras();
	while( memUse > memLimit )
	{
		--recDuration;
		memUse = resX * resY * fps * recDuration * grabber->GetNumCameras();
	}
	cout << "Calculated memory use: " << memUse << endl;
	durScale.set_value( recDuration );
	
	//
	// Prepare grab thread
	//
	gdata.done = false;
	gdata.grabber = grabber;
	gdata.currentBufferIndx = 0;
	gdata.buffersNeeded = fps * recDuration;
	gdata.rawBuffers.resize( grabber->GetNumCameras() );
	gdata.bufferFrameIdx.resize( grabber->GetNumCameras() );
	for( unsigned cc = 0; cc < gdata.rawBuffers.size(); ++cc )
	{
		gdata.rawBuffers[cc].resize( gdata.buffersNeeded );
		for( unsigned bc = 0; bc < gdata.buffersNeeded; ++bc )
		{
			gdata.rawBuffers[cc][bc] = cv::Mat( resY, resX, CV_8UC1, cv::Scalar(0) );
		}
		
		gdata.bufferFrameIdx[cc].assign( gdata.buffersNeeded, 0);
	}
	
	//
	// Launch grab thread.
	//
	gthread = std::thread(GrabThread, &gdata);
	
	// tell the main thread it can start displaying
	{
		std::lock_guard<std::mutex> lock( grabbingMutex );
		grabbing = true;
	}
	grabbingCV.notify_one();
	
}

void ControlsWindow::StopGrabbing()
{
	StopGrabbing(this);
}

gboolean ControlsWindow::StopGrabbing(gpointer self)
	{
		ControlsWindow * window  = (ControlsWindow*) self;
		window->UpdateSessionConfig(true);
		window->startGrabButton.set_sensitive(true);
		window->fpsScale.set_sensitive(true);
		window->xResScale.set_sensitive(true);
		window->yResScale.set_sensitive(true);
		window->durScale.set_sensitive(true);
		window->baseGainButton.set_sensitive(true);
		window->calibModeCheckBtn.set_sensitive(true);
	
		window->obsFpsB.set_text("(not grabbing)");	
	
		window->stopGrabButton.set_sensitive(false);

		window->StopGrabThread();


		if( window->calibModeCheckBtn.get_active() )
		{
			window->FinaliseCalibrationSession();
		}
		window->ClearGtData();
		return FALSE;
	}

void ControlsWindow::StopGrabThread()
{
	grabbing = false;
	
	gdata.done = true;
	
	gthread.join();
}

void ControlsWindow::ClearGtData()
{
	gdata.rawBuffers.clear();
	gdata.bufferFrameIdx.clear();
}

void ControlsWindow::CalibModeToggle()
{
	bool active = calibModeCheckBtn.get_active();
	
	if(active)
	{
		fpsPreCalibToggle         = fpsScale.get_value();
		trialNamePreCalibToggle   = trialNameEntry.get_text();
		trialNumPreCalibToggle = trialNumberSpin.get_value();
		
		trialNameEntry.set_text("calib");
		trialNameEntry.set_sensitive(false);
		trialNumberSpin.set_range(0, 99);
		trialNumberSpin.set_value(0);
		trialNumberSpin.set_increments(1,1);
		
		cout << "fps pre calib toggle: " << fpsPreCalibToggle << endl;
		fpsScale.set_value(5);
		fpsScale.set_sensitive(false);
		
		
		// Create the circle grid detector
		unsigned w,h;
		w = xResScale.get_value();
		h = yResScale.get_value();
		cout << w << " " << h << endl;
		gdata.cgDetector.reset( new CircleGridDetector( w, h, false, false, CircleGridDetector::CIRCD_t ) );
	}
	else
	{
		fpsScale.set_value(fpsPreCalibToggle);
		trialNameEntry.set_text( trialNamePreCalibToggle );
		trialNumberSpin.set_range(0, 99);
		trialNumberSpin.set_value(trialNumPreCalibToggle);
		trialNumberSpin.set_increments(1,1);
		trialNameEntry.set_sensitive(true);
		fpsScale.set_sensitive(true);
	}
	
}

void ControlsWindow::SetGainsAndExposures()
{
	cout << "Set gains and exposures..." << endl;
	
	for( unsigned cc = 0; cc < grabber->GetNumCameras(); ++cc )
	{
		long int divisor = camExpScales[cc].get_value();
		
		long int exp = 1000000 * (1.0 / divisor);
		
// 		// make sure the value is smaller than the framerate.
// 		long int frameDurationMicroSec = 1000000 * (1.0f / fpsScale.get_value());
// 		exp = std::min( frameDurationMicroSec - 10, exp );
		
		grabber->SetExposure(cc, exp );
// 		camExpScales[cc].set_value(exp);
		
		double gain = camGainScales[cc].get_value();
		grabber->SetGain(cc, gain);
	}
}
void ControlsWindow::SetAllGainsAndExposures()
{
	SetAllGainsAndExposures(this);
}
gboolean ControlsWindow::SetAllGainsAndExposures(gpointer self)
{
	cout << "Set all gains and exposures..." << endl;
	ControlsWindow * window  = (ControlsWindow*) self;
	long int exp = window->allCamExpScale.get_value();
	double gain  = window->allCamGainScale.get_value();
	
	for( unsigned cc = 0; cc < window->grabber->GetNumCameras(); ++cc )
	{
		window->camExpScales[cc].set_value(exp);
		window->camGainScales[cc].set_value(gain);
	}
	
	window->SetGainsAndExposures();
	return FALSE;
}

void ControlsWindow::SetAllBaseGains()
{
	cout << "Setting all base gains..." << endl;
	
	SiSoGrabber::baseGain_t baseGain;
	if     ( baseGain00RB.get_active() ) baseGain = SiSoGrabber::baseGain00;
	else if( baseGain06RB.get_active() ) baseGain = SiSoGrabber::baseGain06;
	else if( baseGain12RB.get_active() ) baseGain = SiSoGrabber::baseGain12;
	grabber->SetBaseGain( baseGain );
}


void ControlsWindow::InitialiseCalibrationSession()
{
	trialNumberSpin.set_sensitive(false);
	
	// backup previous grids files for calib trial
	// Load existing grids
	// set number for next grab
	
	gdata.grids.resize( numCameras );
	
	for( unsigned cc = 0; cc < numCameras; ++cc )
	{
		// what's the grid file for this camera
		std::stringstream ss;
		ss << "/data/raid0/recording/" 
		   << GetSaveDirectory() 
		   << "/grids-"
		   << std::setw(2) << std::setfill('0') << cc;
		boost::filesystem::path p( ss.str() );
		
		// does it exist?
		if( boost::filesystem::exists(p) )
		{
			// back it up
			ss << ".backup";
			boost::filesystem::path pb( ss.str() );
			boost::filesystem::copy_file( p, pb, boost::filesystem::copy_option::overwrite_if_exists);
			
			// then read the grids
			LoadGrids( boost::filesystem::canonical(p).string(), gdata.grids[cc] );
		}
	}
	
	// pick the next grid ID
	int numGridGrabs = gdata.grids[0].size();
	for( unsigned cc = 0; cc < numCameras; ++cc )
	{
		assert( gdata.grids[cc].size() == numGridGrabs );
	}
	gdata.calibGrabNo = numGridGrabs;
}


void ControlsWindow::FinaliseCalibrationSession()
{
	// Save grids
	for( unsigned cc = 0; cc < numCameras; ++cc )
	{
		// what's the grid file for this camera
		std::stringstream ss;
		ss << "/data/raid0/recording/" 
		   << GetSaveDirectory() 
		   << "/grids-"
		   << std::setw(2) << std::setfill('0') << cc;
		   
		SaveGrids( ss.str(), gdata.grids[cc] );
	}
	gdata.grids.clear();
	trialNumberSpin.set_sensitive(true);
	
}


void ControlsWindow::LoadGrids( std::string fn, std::vector< std::vector< CircleGridDetector::GridPoint > > &grids )
{
	std::ifstream infi(fn);
	while( infi )
	{
		unsigned id;
		unsigned num;
		
		infi >> id;
		infi >> num;
		if( !infi )
			continue;
		
		std::vector< CircleGridDetector::GridPoint > newGrid;
		newGrid.resize( num );
		for( unsigned pc = 0; pc < num; ++pc )
		{
			unsigned r, c;
			float x, y;
			
			infi >> r;
			infi >> c;
			infi >> x;
			infi >> y;
			
			newGrid[pc].row = r;
			newGrid[pc].col = c;
			newGrid[pc].pi << x,y,1.0;
		}
		std::sort( newGrid.begin(), newGrid.end() );
		
		grids.push_back( newGrid );
	}
}

void ControlsWindow::SaveGrids( std::string fn, std::vector< std::vector< CircleGridDetector::GridPoint > > &grids )
{
	std::ofstream outfi(fn);
	for( unsigned gc = 0; gc < grids.size(); ++gc )
	{
		outfi << gc << " " << grids[gc].size() << endl;
		for( unsigned pc = 0; pc < grids[gc].size(); ++pc )
		{
			outfi << "\t"
			      << grids[gc][pc].row << " "
			      << grids[gc][pc].col << " "
			      << grids[gc][pc].pi(0) << " "
			      << grids[gc][pc].pi(1) << endl;
		}
	}
}


void ControlsWindow::ShowDialog()
{
	Gtk::MessageDialog dialog(*this, "Previous session with todays date was found. Should this be reloaded?",
	      false /* use_markup */, Gtk::MESSAGE_QUESTION,
	      Gtk::BUTTONS_OK_CANCEL);
	dialog.set_default_response(Gtk::RESPONSE_OK);
	int result = dialog.run();

	//Handle the response:
	switch(result)
	{
		case(Gtk::RESPONSE_OK):
		{
		  break;
		}
		case(Gtk::RESPONSE_CANCEL):
		{
		  sessionConfig->GenerateDefaultConfig();
		  break;
		}
		default:
		{
		  std::cout << "Unexpected button clicked." << std::endl;
		  break;
		}
	}
}

void ControlsWindow::MenuFileQuit()
{
  hide(); //Closes the main window to stop the Gtk::Main::run().
}

void ControlsWindow::MenuFileUnimplemented()
{
  std::cout << "A menu item was selected." << std::endl;
}

void ControlsWindow::FileChooserResponse(int response)
{
	switch(response)
	{
		case(Gtk::RESPONSE_OK):
		{
			if(fsAction == Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER)
			{
				if (sessionConfig->Load(dialog->get_filename()))
				{
					SetWidgetValues();
					dialog->hide();
				}
			}
			if (fsAction == Gtk::FILE_CHOOSER_ACTION_CREATE_FOLDER)
			{
				if (fsMove)
				{
					UpdateSessionConfig();
					sessionConfig->Move(dialog->get_filename());
					SetWidgetValues();
					dialog->hide();
				}
				else
				{
					sessionConfig->GenerateDefaultConfig();
					sessionConfig->Save(dialog->get_filename());
					SetWidgetValues();
					dialog->hide();	
				}
				
			}
		break;
		}
				
		case(Gtk::RESPONSE_CANCEL):
		{
			dialog->hide();
			break;
		}
		default:
		{
			cout << "unexpected button pressed" << endl;
		}
	}
	
}

void ControlsWindow::MenuFileSave()
{
	UpdateSessionConfig(true);
}

void ControlsWindow::FileChooserDialog(Gtk::FileChooserAction action)
{
  fsAction = action;
  dialog = new Gtk::FileChooserDialog("Please choose a folder",
          action);
  dialog->set_transient_for(*this);

  //Add response buttons the the dialog:
  dialog->add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog->add_button("Select", Gtk::RESPONSE_OK);
  dialog->signal_response().connect(sigc::mem_fun(*this, &ControlsWindow::FileChooserResponse));
  dialog->set_current_folder(sessionConfig->GetRootPath().c_str());
  dialog->run();

}

#endif