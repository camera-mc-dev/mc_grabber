#ifndef MC_DEV_SESSION_MANAGER_MAIN_WINDOW
#define MC_DEV_SESSION_MANAGER_MAIN_WINDOW


#include <gtkmm.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/listviewtext.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/separator.h>
#include <gtkmm/checkbutton.h>

class MainWindow : public Gtk::Window
{
public:
	MainWindow();
	~MainWindow();
	
	
	
	//
	// Useful structures
	//
	struct ScameraInfo
	{
		unsigned id;
		bool isDebayered;
		
		std::string rawPath;
		std::string rgbPath;
		
	};
	
	struct Strial
	{
		std::string name;
		std::string sessionName;
		std::vector< std::string > paths;
		
		std::vector< ScameraInfo > cameras;
		
		bool isCalib;
		bool hasInitialCalib;
		bool hasMatches;
		bool hasAlignedCalib;
		bool hasGrids;
	};
	
	
	struct Ssession
	{
		std::string name;
		std::vector< std::string > paths;
		
		std::map< std::string, Strial > trials;
		
		std::string activeCalibTrial;
		std::string calibStatus;
		std::string matchesStatus;
		std::string alignStatus;
	};
	
protected:
	
	//
	// General data and vars.
	//
	std::vector< std::string > recPaths;
	
	std::vector< std::string > messageLog;
	
	
	//
	// GUI elements (widgets)
	//
	void CreateInterface();
	
	void SessionBoxRowActivated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
	void TrialBoxRowActivated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
	void CamBoxRowActivated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
	
	// box and grid to contain all display frames
	Gtk::Box            allBox;
	Gtk::Grid           mainGrid;
	
	// Our first couple things display the sessions that we found
	// and the trials within the selected session.
	Gtk::Frame          sessLBFrame;
	Gtk::ScrolledWindow sessLBScroll;
	Gtk::ListViewText   sessionsLBox;
	
	Gtk::Frame          trialLBFrame;
	Gtk::ScrolledWindow trialLBScroll;
	Gtk::ListViewText   trialsLBox;
	
	Gtk::Frame          camLBFrame;
	Gtk::ScrolledWindow camLBScroll;
	Gtk::ListViewText   camsLBox;
	
	// The next things that we need are:
	//   - Display meta information about the selected session (and allow modification of that)
	//   - Display meta information about the selected trial
	//   - Buttons for the tools to:
	//     - Add session/trial/camera to a debayering session
	//     - Run point matcher
	//     - Run calibration checking tool
	//     - Run calibration tool (TODO! do we have existing grids?)
	Gtk::Frame           sessFrame;
	Gtk::Grid            sessFrameGrid0, sessFrameGrid;
	Gtk::Label           sessNameLabel0, sessNameLabel;
	Gtk::Label           sessActiveCalibLabel0,    sessActiveCalibLabel;
	Gtk::Label           sessCalibStatusLabel0,    sessCalibStatusLabel;
	Gtk::Label           sessMatchesStatusLabel0,  sessMatchesStatusLabel;
	
	
	
	Gtk::Frame           trialFrame;
	Gtk::Grid            trialFrameGrid;
	Gtk::Label           trialNameLabel0,          trialNameLabel;
	Gtk::Label           trialDebayerStatusLabel0, trialDebayerStatusLabel;
	
	
	Gtk::Frame           camFrame;
	Gtk::Grid            camFrameGrid;
	Gtk::Label           camNumLabel0,          camNumLabel;
	Gtk::Label           camRawPathLabel0,      camRawPathLabel;
	Gtk::Label           camDebayerPathLabel0,  camDebayerPathLabel;
	
	Gtk::Grid            sessTrialCamGrid;
	
	
	
	
	//
	// Now we need frames for the various tools the user can make use of.
	//
	Gtk::Separator       debayerSep;
	//
	// Debayering
	//
	Gtk::Frame           debayerFrame;
	Gtk::Grid            debayerFrameGrid;
	Gtk::Button          debayerSessBtn;
	Gtk::Button          debayerTrialBtn;
	Gtk::Button          debayerCameraBtn;
	Gtk::Button          debayerRemoveBtn;
	Gtk::Button          debayerActionBtn;
	Gtk::Button          debayerHelpBtn;
	Gtk::Frame           debayerJobsFrame;
	Gtk::ScrolledWindow  debayerJobsScroll;
	Gtk::ListViewText    debayerJobsList;
	Gtk::Frame           debayerModeFrame;
	Gtk::Grid            debayerModeGrid;
	Gtk::RadioButton     debayerModeRBLED, debayerModeRBCVEA, debayerModeRBFCNN;
	
	Gtk::Frame           demonStatusFrame;
	Gtk::Grid            demonStatusGrid;
	Gtk::Label           demonStatusLabel;
	Gtk::Label           demonJobsLabel;
	
	//
	// Calibration
	//
	Gtk::Separator                  calibSep;
	Gtk::Frame                      calibFrame;
	Gtk::Grid                       calibFrameGrid;
	Gtk::Frame                      calibInnerFrame;
	Gtk::Grid                       calibInnerFrameGrid;
	Gtk::Button                     calibRunCalibBtn;
	Gtk::Button                     calibRunPointMatcherBtn;
	Gtk::Button                     calibRunAlignToolBtn;
	Gtk::Button                     calibRunCheckToolBtn;
	Gtk::Button                     calibSetActiveBtn;
	Gtk::Button                     calibHelpBtn;
	                                
	Gtk::CheckButton                calibUseMatchesCheck;
	Gtk::CheckButton                calibUseExGridsCheck;
	Gtk::CheckButton                calibUseBundleCheck;
	                                
	Gtk::Frame                      calibVisFrameFrame;
	Gtk::ScrolledWindow             calibVisFrameScroll;
	Gtk::Grid                       calibVisFrameGrid;
	std::vector< Gtk::Label >       calibVisFrameLabels;
	std::vector< Gtk::SpinButton >  calibVisFrameSpins;
	
	
	//
	// Exporting
	//
	Gtk::Frame           exportFrame;
	Gtk::Grid            exportFrameGrid;
	
	
	
// 	Gtk::Frame           trialFrame;
// 	Gtk::Grid            trialFrameGrid;
// 	Gtk::Label           trialNameLabel0,          trialNameLabel;
// 	Gtk::Label           trialDebayerStatusLabel0, trialDebayerStatusLabel;
// 	Gtk::Label           trialCalibStatusLabel0,   trialCalibStatusLabel;
// 	Gtk::Label           trialMatchesStatusLabel0, trialMatchesStatusLabel;
// 	Gtk::Label           trialAlignStatusLabel0,   trialAlignStatusLabel;
	
	
	
	//
	// Utility methods
	//
	std::map< std::string, Ssession > sessions;
	void ScanForSessions();
	void ScanSession( Ssession &sess );
	void ScanTrial( Strial &trial );
	void ScanCamera( Strial &trial, unsigned camNum );
	
	void CalibHelpClick();
	void CalibRunClick();
	void CalibPMRunClick();
	void CalibAlignRunClick();
	void CalibCheckRunClick();
	
	void DebayerAddSessionClick();
	void DebayerAddTrialClick();
	void DebayerAddCameraClick();
	void DebayerRemoveJobClick();
	void DebayerProcessJobsClick();
	void DebayerHelpClick();
	
	bool CheckDemonStatus();
	
	bool CreateCalibConfig( bool forPointMatcher, std::string &out_filename );
	
	
	
	
};




#endif
