#include "config.h"

ConfigParser::ConfigParser(int numCameras)
{
	GetRootConfig();
	sessionName = prevSaveDir;
	rootPath = fs::path(saveRoot0);
	this->numCameras = numCameras;
	time_t rawNow;
	time(&rawNow);
	auto now = localtime(&rawNow);
	std::stringstream date;
	date << now->tm_year + 1900 << "-"
         << std::setw(2) << std::setfill('0') << now->tm_mon+1 << "-"
         << std::setw(2) << std::setfill('0') << now->tm_mday;
	currentDate = date.str();
	
	if (Load() && (date.str() == sessionDate))
	{	
		cout << "date found in file" << sessionDate << endl;
		showDialogue=true;
	}
	else
	{
		GenerateDefaultConfig();
	}
		
}
void ConfigParser::GenerateDefaultConfig()
{

	cout << "Generating default config" << endl;
	sessionDate = currentDate;
	sessionName = currentDate; 
	videoWidth  = 480;
	videoHeight = 360;
	fps         = 200;
	duration    = 10;
	trialName   = "test";
	trialNum    = 0;
	SetCameraSettings();	
}
bool ConfigParser::Load()
{
	fs::path configPath = rootPath / fs::path(sessionName) / fs::path(configFileName);
	libconfig::Config cfg;
	if (fs::exists(configPath))
	{
		try
		{
			cfg.readFile( configPath.string().c_str() );
			videoWidth  = cfg.lookup("width");
			videoHeight = cfg.lookup("height");
			fps         = cfg.lookup("fps");
			duration    = cfg.lookup("duration");
			trialName   = (const char*) cfg.lookup("trialname");
			trialNum    = cfg.lookup("trialnum");
			sessionDate = (const char*) cfg.lookup("sessionDate");
			ReadCameraEntries();
		}
		catch(libconfig::SettingException &e)
		{
			cout << "Setting error: " << endl;
			cout << e.what() << endl;
			cout << e.getPath() << endl;
			return false;
		}
		return true;

	}
	else
	{
		return false;
	}

}

void ConfigParser::Save()
{
	libconfig::Config cfg;
	fs::path p = rootPath / fs::path(sessionName);
	if (!fs::exists(p)){
		cout << "could not find path specified: " << p.string() << endl;
		return;
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
			cfgRoot.add("sessionDate", libconfig::Setting::TypeString);
			
			cfg.writeFile( configPath.string().c_str() );
		}

		cfg.readFile( configPath.string().c_str() );

		// update entries
		cfg.lookup("width")       = videoWidth;
		cfg.lookup("height")      = videoHeight;
		cfg.lookup("fps")         = fps;
		cfg.lookup("duration")    = duration;
		cfg.lookup("trialname")   = trialName;
		cfg.lookup("trialnum")    = trialNum;
	    cfg.lookup("sessionDate") = sessionDate;
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

	UpdateRootConfig();
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

void ConfigParser::GetRootConfig()
{
#if defined(__APPLE__) || defined( __gnu_linux__ )
	struct passwd* pwd = getpwuid(getuid());
	if (pwd)
	{
		userHome = pwd->pw_dir;
	}
	else
	{
		// try the $HOME environment variable
		userHome = getenv("HOME");
	}
#else
	throw std::runtime_error("yeah, I've not done this for Windows or unknown unix!");
#endif
	
	std::stringstream ss;
	ss << userHome << "/.mc_dev.grabber.cfg";
	boost::filesystem::path p(ss.str());
	
	if( !boost::filesystem::exists(p) )
	{
		throw std::runtime_error("No root config found at: " + p.string());	
	}
	try
	{
		libconfig::Config cfg;
		cfg.readFile( ss.str().c_str() );
		
		saveRoot0 = (const char*) cfg.lookup("saveRoot0");
		saveRoot1 = (const char*) cfg.lookup("saveRoot1");

		// adding this for backwards compatibility
		if (cfg.exists("prevSaveDir"))
		{
			prevSaveDir = (const char*) cfg.lookup("prevSaveDir");
		}
		else
		{
			prevSaveDir = "";
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

void ConfigParser::UpdateRootConfig()
{
	std::stringstream ss;
	ss << userHome << "/.mc_dev.grabber.cfg";
	boost::filesystem::path p(ss.str());

	try
	{
		libconfig::Config cfg;
		auto &cfgRoot = cfg.getRoot();
		
		cfgRoot.add("saveRoot0", libconfig::Setting::TypeString);
		cfgRoot.add("saveRoot1", libconfig::Setting::TypeString);
		cfgRoot.add("prevSaveDir", libconfig::Setting::TypeString);
		cfg.writeFile( ss.str().c_str() );
		
		cfg.readFile( ss.str().c_str() );
		cfg.lookup("saveRoot0") = saveRoot0;
		cfg.lookup("saveRoot1") = saveRoot1;
		cfg.lookup("prevSaveDir") = sessionName;
		cfg.writeFile( ss.str().c_str() );



	}
	catch( libconfig::SettingException &e)
	{
		cout << "Setting error: " << endl;
		cout << e.what() << endl;
		cout << e.getPath() << endl;
		exit(0);
	}


}
