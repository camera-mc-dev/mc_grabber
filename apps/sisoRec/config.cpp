#include "config.h"

ConfigParser::ConfigParser(string saveRoot)
{
	rootPath = fs::path(saveRoot);
	
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
		}
		else
		{
			videoWidth = 480;
			videoHeight = 360;
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
			// create a config file with the default settings.
			
			auto &cfgRoot = cfg.getRoot();
			
			// create entries
			cfgRoot.add("width", libconfig::Setting::TypeInt);
			cfgRoot.add("height", libconfig::Setting::TypeInt);
			
			cfg.writeFile( configPath.string().c_str() );
		}

		cfg.readFile( configPath.string().c_str() );
		
		// update entries
		cfg.lookup("width") = videoWidth;
		cfg.lookup("height") = videoHeight;
		
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