#include "config.h"

ConfigParser::ConfigParser(int numCameras)
{
	// the root config stores the saveroot as well as the name of the last directory written to by this class. 
	LoadRootConfig();
	sessionName = prevSaveDir;
	rootPath = fs::path(saveRoot0);
	this->numCameras = numCameras;
	
	// sets currentDate to todays date in order to check the validity of the prevsavedir
	time_t rawNow;
	time(&rawNow);
	auto now = localtime(&rawNow);
	std::stringstream date;
	date << now->tm_year + 1900 << "-"
         << std::setw(2) << std::setfill('0') << now->tm_mon+1 << "-"
         << std::setw(2) << std::setfill('0') << now->tm_mday;
	currentDate = date.str();
	
	// tries to load the configuration files
	bool loaded = Load();
	
	// checks if the load was successful.
	// if it was, also checks the session date that has now been updated by the loading procedure
	// the date of the prevsavedir is stored in the .config.cfg file 
	if (loaded && (sessionDate == currentDate))
	{	
		showDialog=true;
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
	sessionName = GenerateSessionDirName(currentDate); 
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
			if (!ReadCameraEntries())
			{
				return false;
			}
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
		cout << configPath << " not found" << endl;
		return false;
	}

}

bool ConfigParser::Load(string absolutePath)
{
	fs::path newPath(absolutePath);
	fs::path relativePath = fs::relative(newPath,rootPath);
	sessionName = relativePath.c_str();
	return Load();	
}

void ConfigParser::Save()
{
	libconfig::Config cfg;
	fs::path p = rootPath / fs::path(sessionName);

	// creates the top level directory if it doesnt exist
	// the validity of rootPath and sessionName should already have been checked at this point.
	if (!fs::exists(p)){
		fs::create_directory(p);
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
	    cfg.lookup("sessionDate") = currentDate;
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
void ConfigParser::Save(string absolutePath)
{
	fs::path newPath(absolutePath);
	fs::path relativePath = fs::relative(newPath,rootPath);
	sessionName = relativePath.c_str();
	Save();
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

bool ConfigParser::ReadCameraEntries()
{
	fs::path camerasConfigPath = rootPath / fs::path(sessionName) / fs::path(camerasFileName);
	
	SetCameraSettings();
	
	// adding this here to prevent segfaults if there was an error when writing
	if (!fs::exists(camerasConfigPath))
	{
		cout << camerasConfigPath << " not found." << endl;
		return false;
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
		return false;
	}
return true;
}

void ConfigParser::LoadRootConfig()
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
		cfg.readFile( ss.str().c_str() );
		
		if (!cfg.exists("prevSaveDir"))
		{
			auto &cfgRoot = cfg.getRoot();
			cfgRoot.add("prevSaveDir", libconfig::Setting::TypeString);
		}
		
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

string ConfigParser::GenerateSessionDirName(string sessionName, int dirNumber)
{
	if (fs::exists(rootPath / fs::path(sessionName)))
	{
		
		stringstream ss;
		ss << currentDate << "_" << dirNumber;
		sessionName = ss.str();
		dirNumber += 1;
		// recurse function until it finds a session name
		// that doesnt exist yet.
		return GenerateSessionDirName(sessionName,dirNumber);

	}
	else
	{
		return sessionName;
	}
}
