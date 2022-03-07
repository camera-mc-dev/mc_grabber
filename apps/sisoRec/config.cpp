#include "config.h"

ConfigParser::ConfigParser(string saveRoot)
{
	boost::filesystem::path rootPath(saveRoot);
	if (!boost::filesystem::exists(rootPath)){
		string error = "could not find path specified: " + saveRoot;
		throw std::runtime_error(error);
	}

	configPath = rootPath / boost::filesystem::path(configFileName);
	libconfig::Config cfg;
	try
	{

		if( !boost::filesystem::exists(configPath))
		{
			// create a config file with the default settings.
			
			auto &cfgRoot = cfg.getRoot();
			
			// create entries
			cfgRoot.add("width", libconfig::Setting::TypeInt);
			cfgRoot.add("height", libconfig::Setting::TypeInt);

			// update entries
			cfg.lookup("width") = 1920;
			cfg.lookup("height") = 1080;
			
			cfg.writeFile( configPath.string().c_str() );
		}

		libconfig::Config cfg;
		cfg.readFile( configPath.string().c_str() );
		videoWidth  = cfg.lookup("width");
		videoHeight = cfg.lookup("height");
		
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
	try
	{
		
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