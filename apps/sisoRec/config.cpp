#include "config.h"

ConfigParser::ConfigParser(string saveRoot, int numCameras)
{
	rootPath = fs::path(saveRoot);
	this->numCameras = numCameras;
	time_t rawNow;
	time(&rawNow);
	auto now = localtime(&rawNow);
	std::stringstream defSessionName;
	defSessionName << now->tm_year + 1900 << "-"
	               << std::setw(2) << std::setfill('0') << now->tm_mon+1 << "-"
	               << std::setw(2) << std::setfill('0') << now->tm_mday;
	
	sessionName = defSessionName.str();
	
	fs::path configPath = rootPath / fs::path(sessionName) / fs::path(configFileName);
	
	libconfig::Config cfg;
	
	try
	{
		if (fs::exists(configPath))
		{
			libconfig::Config cfg;
			cfg.readFile( configPath.string().c_str() );
			videoWidth  = cfg.lookup("width");
			videoHeight = cfg.lookup("height");
			fps         = cfg.lookup("fps");
			duration    = cfg.lookup("duration");
			trialName   = (const char*) cfg.lookup("trialname");
			trialNum    = cfg.lookup("trialnum");
			ReadCameraEntries();
		}
		else
		{
			videoWidth  = 480;
			videoHeight = 360;
			fps         = 200;
			duration    = 10;
			trialName   = "test";
			trialNum    = 0;
			SetCameraSettings();
		}

		
	}
	
	catch( libconfig::SettingException &e)
	{
		cout << "Setting error: " << endl;
		cout << e.what() << endl;
		cout << e.getPath() << endl;
		exit(0);
	}
	
}


void ConfigParser::Save()
{
	libconfig::Config cfg;
	fs::path p = rootPath / fs::path(sessionName);
	if (!fs::exists(p)){
		string error = "could not find path specified: " + p.string();
		throw std::runtime_error(error);
	}

	fs::path configPath = p / fs::path(configFileName);
	
	try
	{
		if(!fs::exists(configPath))
		{
			// create a config file.
			
			auto &cfgRoot = cfg.getRoot();
			
			// create entries
			cfgRoot.add("width", libconfig::Setting::TypeInt);
			cfgRoot.add("height", libconfig::Setting::TypeInt);
			cfgRoot.add("fps", libconfig::Setting::TypeInt);
			cfgRoot.add("duration", libconfig::Setting::TypeInt);
			cfgRoot.add("trialname", libconfig::Setting::TypeString);
			cfgRoot.add("trialnum", libconfig::Setting::TypeInt);
			
			cfg.writeFile( configPath.string().c_str() );
		}

		cfg.readFile( configPath.string().c_str() );

		// update entries
		cfg.lookup("width")     = videoWidth;
		cfg.lookup("height")    = videoHeight;
		cfg.lookup("fps")       = fps;
		cfg.lookup("duration")  = duration;
		cfg.lookup("trialname") = trialName;
		cfg.lookup("trialnum")  = trialNum;
		cfg.writeFile( configPath.string().c_str() );
		
		UpdateCameraEntries();
	}

	catch( libconfig::SettingException &e)
	{
		cout << "Setting error: " << endl;
		cout << e.what() << endl;
		cout << e.getPath() << endl;
		exit(0);
	}
}

void ConfigParser::SetCameraSettings()
{
	for (unsigned i=0; i < numCameras; i++)
	{
		CameraSettings camSetting;
		camSetting.exposure = 250;
		camSetting.gain = 1;
		camSetting.displayed = true;
		camSettings.push_back(camSetting);
	}

}

void ConfigParser::UpdateCameraEntries()
{
	// there might be a better way of doing this
	// the way libconfig seems to want to update arrays, it seems safer to overwrite the entire file
	// For this, its better to have a discrete file for the camera settings so the other settings are kept untouched.

	libconfig::Config cfg;
	auto &cfgRoot = cfg.getRoot();

	libconfig::Setting &camExposures = cfgRoot.add("camexposures", libconfig::Setting::TypeArray);
	libconfig::Setting &camGains = cfgRoot.add("camgains", libconfig::Setting::TypeArray);
	libconfig::Setting &camsDisplayed = cfgRoot.add("camsdisplayed", libconfig::Setting::TypeArray);
	
	for (unsigned i=0; i < numCameras; i++)
	{
		camExposures.add(libconfig::Setting::TypeInt) = camSettings[i].exposure;
		camGains.add(libconfig::Setting::TypeFloat) = camSettings[i].gain;
		camsDisplayed.add(libconfig::Setting::TypeBoolean) = camSettings[i].displayed;
	}
	
	fs::path camerasConfigPath = rootPath / fs::path(sessionName) / fs::path(camerasFileName);
	cfg.writeFile( camerasConfigPath.string().c_str() );


}

void ConfigParser::ReadCameraEntries()
{
	fs::path camerasConfigPath = rootPath / fs::path(sessionName) / fs::path(camerasFileName);
	
	SetCameraSettings();
	
	// adding this here to prevent segfaults if there was an error when writing
	if (!fs::exists(camerasConfigPath))
	{
		cout << camerasConfigPath << " not found, using default settings for the cameras." << endl;
		return;
	}
	
	libconfig::Config cfg;
	cfg.readFile( camerasConfigPath.string().c_str() );
	try
	{
		libconfig::Setting &camExposures = cfg.lookup("camexposures"); 
		libconfig::Setting &camGains = cfg.lookup("camgains");
		libconfig::Setting &camsDisplayed = cfg.lookup("camsdisplayed");
		
		for (unsigned i = 0; i < numCameras; i++)
		{
			camSettings[i].exposure = camExposures[i];
			camSettings[i].gain = camGains[i];
			camSettings[i].displayed = camsDisplayed[i];
		}

	}

	catch( libconfig::SettingException &e)
	{
		cout << "Setting error: " << endl;
		cout << e.what() << endl;
		cout << e.getPath() << endl;
		exit(0);
	}
}