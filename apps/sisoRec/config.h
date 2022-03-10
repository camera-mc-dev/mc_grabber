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
	int gain;
	bool displayed;
};

class ConfigParser
{
public:

	// read if it exists
	ConfigParser(std::string saveRoot, int numCameras); 

	// update the values in the config from memory
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
	std::string configFileName = ".config.cfg";
	std::string camerasFileName = ".cameras.cfg";
	boost::filesystem::path rootPath;
	int numCameras;
	
	void SetCameraSettings();
	void UpdateCameraEntries();
	void ReadCameraEntries();

};

#endif