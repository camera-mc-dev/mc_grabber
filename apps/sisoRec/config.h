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

	void Load();
	
	bool showDialogue = false;
	
	string trialName;
	string sessionName;

	int trialNum;
	
	int fps;
	int duration;
	int videoWidth;
	int videoHeight;

	std::vector<CameraSettings> camSettings;

protected:
	// where the main settings are stored (width,height,fps,duration etc)
	std::string configFileName = ".config.cfg";

	// where the arrays of camera configurations are stored (exposure, gain etc)
	std::string camerasFileName = ".cameras.cfg";
	
	// path object of saveRoot (saveRoot0 in .mcdev.grabber.cfg)
	boost::filesystem::path rootPath;
	
	int numCameras;
	
	// sets the CameraSettings vector with defaults.
	void SetCameraSettings();

	// Updates the camera entries in .camera.cfg
	void UpdateCameraEntries();

	// reads the camera entries from .camera.cfg 
	void ReadCameraEntries();

	// entries from .mc.grabber.cfg
	std::string userHome;
	std::string saveRoot0;
	std::string saveRoot1;
	std::string prevSaveDir; 

	//updates the variables above from the root config
	void GetRootConfig();

	// overwrites the root config, adding the prevsavedir setting
	void UpdateRootConfig();


};

#endif