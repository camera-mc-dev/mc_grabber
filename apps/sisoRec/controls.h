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
#include "config.h"

#include "renderer.h"
#include "imgio/sourceFactory.h"

class SignalHandler
{
public:
	SignalHandler() {};
	~SignalHandler() {};
	std::condition_variable cv;
	std::mutex mtx;
	bool ready=false;

};

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
		return durSpin.get_value();
	}
	
	GrabThreadData gdata;

protected:

	//
	// Widgets
	//
	
	// box to contain all the widgets
	Gtk::Box allBox;
	// previous allBox, contains everything apart from the trials stuff
	Gtk::Box vBoxLeft;
	// contains the session name, trial name editor and listview
	Gtk::Box vBoxRight;
	// box to stack vBoxLeft and vBoxRight
	Gtk::Box hBox;
	
	//
	// Dropdown menu
	//
	Glib::RefPtr<Gtk::UIManager> m_refUIManager;
	Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup;
	Glib::RefPtr<Gtk::RadioAction> m_refChoiceOne, m_refChoiceTwo;

	// signal handlers
	void MenuFileNew()
	{
  		FileChooserDialog(Gtk::FILE_CHOOSER_ACTION_CREATE_FOLDER);
	};


	void MenuFileLoad()
	{
		FileChooserDialog(Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	}

	void MenuFileMove()
	{
		fsMove = true;
		FileChooserDialog(Gtk::FILE_CHOOSER_ACTION_CREATE_FOLDER);
		
	}

	void MenuFileQuit();
	void MenuFileSave();
	void MenuFileUnimplemented();


	//
	// File Dialog
	// 
	Gtk::FileChooserDialog * dialog;
	Gtk::FileChooserAction fsAction;

	// tells the filedialog to move all files with it when it creates a new folder (for move/rename) 
	bool fsMove = false;

	// creates a new filechooser dialog
	void FileChooserDialog(Gtk::FileChooserAction action);
	
	// handles the response from file chooser dialog. 
	void FileChooserResponse(int response);

	//
	// Trial treeview
	//
	void PopulateTrialList()
	{
		PopulateTrialList(this);
	}
	void RenderTrial(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);

  	//Tree model columns:
  	class ModelColumns : public Gtk::TreeModel::ColumnRecord
	{
	public:

		ModelColumns()
		{ add(m_col_id); add(m_col_name); }

		Gtk::TreeModelColumn<int> m_col_id;
		Gtk::TreeModelColumn<Glib::ustring> m_col_name;
	};

	ModelColumns m_Columns;

	//Child widgets:
	Gtk::ScrolledWindow m_ScrolledWindow;
	Gtk::TreeView m_TreeView;
	Glib::RefPtr<Gtk::TreeStore> m_refTreeModel;

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
	Gtk::Grid        sessionGrid;
	Gtk::Frame       sessionFrame;
	Gtk::Label       xResLabel, yResLabel, fpsLabel, durLabel;
	Gtk::Entry       xResEntry, yResEntry;
	Gtk::SpinButton  fpsSpin, durSpin;
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

	//
	// For getting the text entries from xres and yres and converting them to ints (safely)
	//
	int GetResEntry(Gtk::Entry* entry)
	{
		return abs(std::stoi(entry->get_text()));	
	}

	//
	// for storing gtk settings to disk
	//
	ConfigParser * sessionConfig;


	// wrapper for static function call
	void StopGrabbing();
	void SetAllGainsAndExposures();
	void SetWidgetValues();
	//
	// for time based events.
	//
	sigc::connection fpsTimerConnection;

public:
	static gboolean PopulateTrialList(gpointer self);

	void StartGrabbing();
	
	// when called from seperate thread, needs to be called with 
	// gdk_threads_add_idle in order to avoid crashes.
	static gboolean StopGrabbing(gpointer self);
	
	void StopGrabThread();
	void ClearGtData();
	void CalibModeToggle();
	void SetGainsAndExposures();
	
	// when called from seperate thread, needs to be called with 
	// gdk_threads_add_idle in order to avoid crashes.
	static gboolean SetAllGainsAndExposures(gpointer self);

	void SetMaxExposure();
	
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
			float clockfps = fpsSpin.get_value();
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
		return fpsSpin.get_value();
	}
	
	// updates the values in session config. 
	// saves the config to disk under rootpath/sessionName if save is true.
	void UpdateSessionConfig(bool save = true);
	
	static gboolean IncrementTrialNumber(gpointer self)
	{
		ControlsWindow * window  = (ControlsWindow*) self;
		int v = window->trialNumberSpin.get_value();
		v+=1;
		window->trialNumberSpin.set_range(0, 99);
		window->trialNumberSpin.set_value(v);
		window->trialNumberSpin.set_increments(1,1);
		return FALSE;
	}

	void FinaliseCalibrationSession();
	void AutoGenCalibConfig( std::string dataRoot, std::string trialRoot );

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

	void ShowDialog();
	
};

struct GUIThreadData
{
	bool done = false;
	AbstractGrabber *grabber;
	Glib::RefPtr<Gtk::Application> app;
	
	ControlsWindow *window;
	
	std::string saveRoot0;
	std::string saveRoot1;
	SignalHandler * signalHandler;
};

void GUIThread( GUIThreadData *gtdata );

#endif
// #endif
