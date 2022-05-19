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
			if (!LoadCameraEntries())
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
		
		SaveCameraEntries();
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

void ConfigParser::SaveCameraEntries()
{
	// there might be a better way of doing this
	// the way libconfig seems to want to update arrays, it seems safer to overwrite the entire file
	// For this, its better to have a discrete file for the camera settings so the other settings are kept untouched.

	libconfig::Config cfg;
	auto &cfgRoot = cfg.getRoot();
	libconfig::Setting &numCamerasConf = cfgRoot.add("numCameras", libconfig::Setting::TypeInt);
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
	
	cfg.readFile(camerasConfigPath.string().c_str());
	cfg.lookup("numCameras") = numCameras;
	cfg.writeFile( camerasConfigPath.string().c_str() );


}

bool ConfigParser::LoadCameraEntries()
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
	int numCamerasConf;
	try
	{
		numCamerasConf = cfg.lookup("numCameras");
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
if (numCamerasConf != numCameras)
{
	cout << "Error: will not load session with " << numCamerasConf << " cameras with " << numCameras << " cameras connected." << endl;
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

bool ConfigParser::Move(string absolutePath)
{
	// all the config stuff is saved under saveroot0 so we need to make sure the changes
	// are mirrored on the other drive. Also need to make sure the dual drive system is 
	// preserved. 

	// generate the src and dst for saveroot0
	fs::path src0 = rootPath / fs::path(sessionName);
	fs::path dst0 = fs::path(absolutePath);
	
	
	// do the same for saveroot1
	fs::path src1 = fs::path(saveRoot1) / fs::path(sessionName);
	fs::path new_sessionName = fs::relative(fs::path(absolutePath), rootPath);
	fs::path dst1 = fs::path(saveRoot1) / new_sessionName;
	
	// move the first directory
	fs::rename(src0,dst0);
	
	// add check condition incase we only have one drive or only one camera is being used.
	if (fs::exists(src1))
	{
		// if this condition triggers it might mean we have moved out of the drives and we would have to merge
		// the two directories (which we dont want to do at the capture phase)
		if (fs::exists(dst1))
		{
			cout << "Error when moving " << src1 << " to " << dst1 << endl;
			cout << dst1 << " already exists." << endl;

			// undo move of first directory.
			fs::rename(dst0, src0);
			return false;
		}
		fs::rename(src1,dst1);
	}
	
	// load the session to update the sessionName.
	return Load(absolutePath);

}

std::vector<string> ConfigParser::GetTrialNames()
{
	std::vector<string> trials;
	fs::path p = rootPath / fs::path(sessionName);
	if (!fs::exists(p))
	{
		return trials;
	}
	for (const auto & entry : fs::directory_iterator(p))
		if (fs::is_directory(entry.path()))
		{
        	trials.push_back(fs::relative(entry.path(),p).string());
		}
	std::sort(trials.begin(),trials.end());

    return trials;
}

std::vector<string> ConfigParser::GetImageDirectories(string trialName)
{
	std::vector<string> directories;
	fs::path tp0 = rootPath / fs::path(sessionName) / fs::path(trialName);
	fs::path tp1 = fs::path(saveRoot1) / fs::path(sessionName) / fs::path(trialName);

	std::vector<fs::path> paths;
	// add this check so we dont render twice as many cameras if were not on a dual directory system
	if (!fs::equivalent(tp0,tp1))
	{
		paths.push_back(tp0);
		paths.push_back(tp1);	
	}
	else
	{
		paths.push_back(tp0);
	}
	
	for (fs::path tp : paths)
	{
		if (!fs::exists(tp))
		{
			// return empty vector (to be checked by calling function)
			return directories;
		}
		for (const auto & entry : fs::directory_iterator(tp))
			if (fs::is_directory(entry.path()))
			{
				stringstream ss;
				ss << entry.path().string() << "/";
	        	directories.push_back(ss.str());
			}
	}

	std::sort(directories.begin(),directories.end());
	return directories;
}

string ConfigParser::GetHDF5(string directory)
{
	fs::path p(directory);
	for (const auto & entry : fs::directory_iterator(p)){
		if(entry.path().extension() == ".hdf5")
		{
			return entry.path().string();
		}
	}
	return "";

}