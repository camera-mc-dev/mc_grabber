#ifdef USE_SISO

#include "libconfig.h++"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <iomanip>
#include<vector>
#include <boost/filesystem.hpp>

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
	ConfigParser(std::string saveRoot, int numCameras); 

	// updates the values in the config files from memory
	void Save();
	
	
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
	
	boost::filesystem::path rootPath;
	int numCameras;
	
	void SetCameraSettings();
	void UpdateCameraEntries();
	void ReadCameraEntries();

};

#endif