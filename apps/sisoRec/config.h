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
	
	// call explicitly from mainfile?
	void WriteToFile();

	
	
	string trialName;
	int numTrials;
	
	int fps;
	
	int videoWidth;
	int videoHeight;

	std::vector<CameraSettings> camSettings;

protected:
	void ReadFromFile();
	std::string configFilename = "config.cfg";

};

#endif