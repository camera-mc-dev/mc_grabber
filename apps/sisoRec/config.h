#ifndef CONFIG_PARSER
#define CONFIG_PARSER

#include "libconfig.h++"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <iomanip>
#include<vector>
#include <boost/filesystem.hpp>
#include <pwd.h>

#include <ctime>

using namespace::std;
namespace fs = boost::filesystem;

struct CameraSettings
{
	int exposure;
	float gain;
	bool displayed;
};

class ConfigParser
{
public:

	// reads the .config and .camera config files in a directory with the days date if it exists.
	// otherwise it will return the default settings
	ConfigParser(int numCameras); 

	// updates the values in the config files from memory
	void Save();

	// saves the session to the directory specified by absolutePath
	void Save(string absolutePath);

	// tries to load a config in the directory specified by rootPath/sessionName.
	// returns false if there was an error (i.e the directory doesnt exist or there is something wrong with the session config files.)
	bool Load();

	// tries to load config files in absolute path if they exist	
	bool Load(string absolutePath);

	// moves all files to the directory specified by absolutePath
	bool Move(string absolutePath);

	// entries from .config.cfg and .cameras.cfg
	string trialName;
	string sessionName;
	int trialNum;
	int fps;
	int duration;
	int videoWidth;
	int videoHeight;
	std::string sessionDate;
	std::vector<CameraSettings> camSettings;

	// sets the above variables to defaults
	void GenerateDefaultConfig();

	// checked by the controls window to open a dialog asking whether to "reload" the old session.
	// (actually the old session is always loaded if it exists, its just overwritten by generatedefaultconfig if the answer is no.)
	bool showDialog = false;

	fs::path GetRootPath()
	{
		return rootPath;
	}

	// Returns the names of the trials in the session directory
	std::vector<string> GetTrialNames();

	// returns the image directories in a trial (as strings but the parsing is done with boost::fs)
	std::vector<string> GetImageDirectories(string trialName);

	// gets the first .hdf5 file from a directory. returns an empty string if none found
	string GetHDF5(string directory);

protected:

	// where the main settings are stored (width,height,fps,duration etc)
	std::string configFileName = ".config.cfg";

	// where the arrays of camera configurations are stored (exposure, gain etc)
	std::string camerasFileName = ".cameras.cfg";
	
	// path object of saveRoot (saveRoot0 in .mcdev.grabber.cfg)
	fs::path rootPath;
	
	int numCameras;
	
	// sets the CameraSettings vector with defaults.
	void SetCameraSettings();

	// Updates the camera entries in .camera.cfg
	void SaveCameraEntries();

	// reads the camera entries from .camera.cfg 
	bool LoadCameraEntries();

	// entries from .mc.grabber.cfg
	std::string userHome;
	std::string saveRoot0;
	std::string saveRoot1;
	std::string prevSaveDir;
	std::string currentDate;

	//updates the variables above from the root config
	void LoadRootConfig();

	// adds the prevsadir entry to a config file if it doesnt exist and updates it.
	void UpdateRootConfig();
	
	//
	// will generate a default session name.
	// params:
	// 	sessionName - name of desired session directory
	// 	dirNumber   - number to be appended to end of sessionName if a directory with that name already exists.
	// 				  this will be incremented automatically until a non existent directory is found.
	//
	string GenerateSessionDirName(string sessionName, int dirNumber = 1);


};

#endif