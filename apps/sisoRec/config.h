#ifdef USE_SISO

#include "libconfig.h++"
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
	float exposure;
	float gain;
};

class ConfigParser
{
public:

	// read if it exists
	ConfigParser(std::string saveRoot); 

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
	boost::filesystem::path rootPath;

};

#endif