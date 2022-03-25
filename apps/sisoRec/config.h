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

	void Save(string absolutePath);
	
	// updates the values in the config files from memory
	void Save();

	// tries to load a config in the directory specified by rootPath/sessionName.
	// returns false if there was an error (i.e the directory doesnt exist or there is something wrong with the session config files.)
	bool Load();

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

	
	
	// checked by the controls window to open a dialogue asking whether to "reload" the old session.
	// (actually the old session is always loaded if it exists, its just overwritten by generatedefaultconfig if the answer is no.)
	bool showDialog = false;

protected:

	// where the main settings are stored (width,height,fps,duration etc)
	std::string configFileName = ".config.cfg";

	// where the arrays of camera configurations are stored (exposure, gain etc)
	std::string camerasFileName = ".cameras.cfg";
	
	// path object of saveRoot (saveRoot0 in .mcdev.grabber.cfg)
	boost::filesystem::path rootPath;
	fs::path savePath;
	
	int numCameras;
	
	// sets the CameraSettings vector with defaults.
	void SetCameraSettings();

	// Updates the camera entries in .camera.cfg
	void UpdateCameraEntries();

	// reads the camera entries from .camera.cfg 
	bool ReadCameraEntries();

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
	
	/**
	 * will generate a default session name.
	 * params:
	 * 	sessionName - name of desired session directory
	 * 	dirNumber   - number to be appended to end of sessionName if a directory with that name already exists.
	 * 				  this will be incremented automatically until a non existent directory is found.
	 **/
	string GenerateSessionDirName(string sessionName, int dirNumber = 1);


};

#endif