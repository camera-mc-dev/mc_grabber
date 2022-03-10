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

void ConfigParser::UpdateCameraEntries(fs::path configPath)
{
	libconfig::Config cfg;
	auto &cfgRoot = cfg.getRoot();

	libconfig::Setting &camExposures = cfgRoot.add("camexposures", libconfig::Setting::TypeArray);
	libconfig::Setting &camGains = cfgRoot.add("camgains", libconfig::Setting::TypeArray);
	libconfig::Setting &camsDisplayed = cfgRoot.add("camsdisplayed", libconfig::Setting::TypeArray);
	
	for (unsigned i=0; i < numCameras; i++)
	{
		camExposures.add(libconfig::Setting::TypeInt) = camSettings[i].exposure;
		camGains.add(libconfig::Setting::TypeInt) = camSettings[i].gain;
		camsDisplayed.add(libconfig::Setting::TypeBoolean) = camSettings[i].displayed;
	}
	

}

void ConfigParser::ReadCameraEntries(libconfig::Config cfg)
{

}