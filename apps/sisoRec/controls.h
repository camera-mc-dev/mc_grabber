// #ifdef USE_SISO
#ifndef SISO_REC_CONTROLS
#define SISO_REC_CONTROLS

#include <gtkmm.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>

#include <thread>
#include <condition_variable>
#include <map>
#include <iomanip>

#include "imgio/siso.h"

#include "grab.h"

class ControlsWindow : public Gtk::Window
{
public:
	ControlsWindow(AbstractGrabber *in_grabber);
	~ControlsWindow();
	
	// we use this to signal to the main thread that we 
	// are grabbing, and thus, it should create a rendering window
	// and display the requested cameras.
	bool grabbing = false;
	std::mutex grabbingMutex;
	std::condition_variable grabbingCV;
	
	void GetCameraDisplayInfo( std::map<int, bool> &dispCams )
	{
		dispCams.clear();
		for( unsigned cc = 0; cc < camDisplayedChecks.size(); ++cc )
		{
			dispCams[cc] = camDisplayedChecks[cc].get_active();
		}
	}
	
	std::string GetSaveDirectory()
	{
		std::stringstream ss;
		ss << sessionNameEntry.get_text() << "/" 
		   << trialNameEntry.get_text()   << "_"
		   << std::setw(2) << std::setfill('0') << trialNumberSpin.get_value()  << "/";
		return ss.str();
	}
	
	int GetTrialNumber()
	{
		return trialNumberSpin.get_value();
	}
	
	int GetSendImageIndx()
	{
		return shareSpinner.get_value();
	}

	int GetRecDuration()
	{
		return durScale.get_value();
	}
	
	GrabThreadData gdata;
	
	//
	// Widgets
	//
	
	// box to contain all control frames
	Gtk::Box allBox;
	
	//
	// At the top of our controls window we have:
	//    =====
	// image resolution x       (scale/slider - greyed out when grabbing)
	// image resolution y       (scale/slider - greyed out when grabbing)
	// frame rate               (scale/slider - greyed out when grabbing)
	// observed frame rate      (label)
	// Start grabbing           (Button)
	// Stop  grabbing           (Button)
	//    =====
	//
	Gtk::Frame       ssFrame;
	Gtk::Grid        ssGrid;
	Gtk::Label       xResLabel, yResLabel, fpsLabel, durLabel;
	Gtk::HScale      xResScale, yResScale, fpsScale, durScale;
	Gtk::Label       obsFpsA, obsFpsB;
	Gtk::Label       sessionNameLabel, trialNameLabel;
	Gtk::Entry       sessionNameEntry;
	Gtk::Entry       trialNameEntry;
	Gtk::SpinButton  trialNumberSpin;
	Gtk::Button      stopGrabButton;
	Gtk::CheckButton calibModeCheckBtn;
	Gtk::CheckButton gridDetectCheckBtn;
	Gtk::Button      startGrabButton;
	Gtk::Label       gridLabel;
	Gtk::SpinButton  gridRowsSpin, gridColsSpin;
	Gtk::CheckButton gridLightOnDarkCheck;
	
	//
	// Next we have the per-camera controls,
	// which really just amounts to exposure and gain.
	//
	Gtk::Frame  camControlFrame;
	Gtk::Grid   camControlGrid;
	Gtk::Button camControlSetButton;
	std::vector< Gtk::Frame > camFrames;
	std::vector< Gtk::Grid >  camGrids;
	std::vector< Gtk::Label > camExpLabels;
	std::vector< Gtk::Scale > camExpScales;
	std::vector< Gtk::Label > camGainLabels;
	std::vector< Gtk::Scale > camGainScales;
	std::vector< Gtk::CheckButton > camDisplayedChecks;
	
	//
	// It is useful to have one control to set exposure and gain of all cameras.
	//
	Gtk::Frame  allCamExpFrame;
	Gtk::Grid   allCamExpGrid;
	Gtk::Button allCamExpSetButton;
	Gtk::Scale  allCamExpScale;
	Gtk::Scale  allCamGainScale;
	Gtk::Label  allCamExpLabel;
	Gtk::Label  allCamGainLabel;
	
	Gtk::Frame  baseGainFrame;
	Gtk::Grid   baseGainGrid;
	Gtk::Button baseGainButton;
	Gtk::Label  baseGainLabel;
	Gtk::RadioButton baseGain00RB, baseGain06RB, baseGain12RB;
	
	//
	// Control which camera we share via the image sender
	//
	Gtk::Frame      shareFrame;
	Gtk::Grid       shareGrid;
	Gtk::Label      shareLabel;
	Gtk::SpinButton shareSpinner;

protected:
	void StopGrabbing();
	//
	// for time based events.
	//
	sigc::connection fpsTimerConnection;
	
	//
	// Signal handlers
	//
public:
	void StartGrabbing();
	static gboolean StopGrabbing(gpointer data);
	void StopGrabThread();
	void ClearGtData();
	void CalibModeToggle();
	void SetGainsAndExposures();
	void SetAllGainsAndExposures();
	void SetAllBaseGains();
	
	bool IsCalibMode()
	{
		return calibModeCheckBtn.get_active();
	}
	bool IsLiveGridDetectEnabled()
	{
		return gridDetectCheckBtn.get_active();
	}
	
	int GetCalibGridRows()
	{
		return gridRowsSpin.get_value();
	}
	int GetCalibGridCols()
	{
		return gridColsSpin.get_value();
	}
	bool GetCalibGridLightOnDark()
	{
		return gridLightOnDarkCheck.get_active();
	}
	
	bool SetObservedFPS()
	{
		if( grabbing )
		{
			//
			// What we've measured is not actually the fps, it is the grab time.
			// The important thing is that the grab-time is less than the fps of
			// the sync pulses - that means that meanfps is hopefully going to be
			// some number larger than the sync fps.
			//
			// It would be confusing to the user to see meanfps larger than 
			// the clock fps, so we cap the displayed value.
			//
			float clockfps = fpsScale.get_value();
			obsFpsB.set_text( std::to_string( std::min(gdata.meanfps, clockfps) ) );
		}
		else
		{
			obsFpsB.set_text( "not grabbing" );
		}
		return true;
	}
	
	int GetDesiredFPS()
	{
		return fpsScale.get_value();
	}
	
	void IncrementTrialNumber()
	{
		int v = trialNumberSpin.get_value();
		v+=1;
		trialNumberSpin.set_range(0, 99);
		trialNumberSpin.set_value(v);
		trialNumberSpin.set_increments(1,1);
	}

	void FinaliseCalibrationSession();

protected:
	//
	// Access to the grabber class.
	//
	AbstractGrabber *grabber;
	int numCameras;
	
	
	//
	// State monitors
	//
	std::thread gthread;
	
	float fpsPreCalibToggle;
	std::string trialNamePreCalibToggle;
	int trialNumPreCalibToggle;
	
	float meanfps;
	
	
	void InitialiseCalibrationSession();
	
	void LoadGrids( std::string fn, std::vector< std::vector< CircleGridDetector::GridPoint > > &grids );
	void SaveGrids( std::string fn, std::vector< std::vector< CircleGridDetector::GridPoint > > &grids );
};

struct GUIThreadData
{
	bool done = false;
	AbstractGrabber *grabber;
	Glib::RefPtr<Gtk::Application> app;
	
	ControlsWindow *window;
	
	std::string saveRoot0;
	std::string saveRoot1;
};

void GUIThread( GUIThreadData *gtdata );

#endif
// #endif
