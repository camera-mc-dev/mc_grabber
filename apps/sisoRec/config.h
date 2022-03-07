#ifdef USE_SISO

#include "libconfig.h++"
#include <iostream>
#include<vector>
#include <boost/filesystem.hpp>

using namespace::std;

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
	int numTrials;
	
	int fps;
	
	int videoWidth;
	int videoHeight;

	std::vector<CameraSettings> camSettings;

protected:
	std::string configFileName = ".config.cfg";
	boost::filesystem::path configPath;

};

#endif