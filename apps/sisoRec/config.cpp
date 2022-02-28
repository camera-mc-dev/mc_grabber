#include "config.h"

ConfigParser::ConfigParser(string saveRoot)
{
	boost::filesystem::path rootPath(saveRoot);
	if (!boost::filesystem::exists(rootPath)){
		string error = "could not find path specified: " + saveRoot;
		throw std::runtime_error(error);
	}

	configPath = rootPath / boost::filesystem::path rootPath(configFileName);
	boost::filesystem::path p(ss.str());
		try
		{
			if( !boost::filesystem::exists(configPath))
			{
				// create a config file with the default settings.
				libconfig::Config cfg;
				auto &cfgRoot = cfg.getRoot();
				
				cfgRoot.add("dataRoot", libconfig::Setting::TypeString);
				cfgRoot.add("shadersRoot", libconfig::Setting::TypeString);
				cfgRoot.add("coreDataRoot", libconfig::Setting::TypeString);
				cfgRoot.add("scriptsRoot", libconfig::Setting::TypeString);

				cfgRoot.add("maxSingleWindowWidth", libconfig::Setting::TypeInt );
				cfgRoot.add("maxSingleWindowHeight", libconfig::Setting::TypeInt );
				
				cfg.lookup("dataRoot")     = userHome + "/programming/mc_dev/data";
				cfg.lookup("shadersRoot") = userHome + "/programming/mc_dev/shaders";
				cfg.lookup("coreDataRoot") = userHome + "/programming/mc_dev/data";
				cfg.lookup("scriptsRoot")  = userHome + "/programming/mc_dev/python";
				
				cfg.lookup("maxSingleWindowWidth") = 1000;
				cfg.lookup("maxSingleWindowHeight") = 800;
				
				cfg.writeFile( ss.str().c_str() );
			}
			libconfig::Config cfg;
			cfg.readFile( ss.str().c_str() );
			
			dataRoot     = (const char*) cfg.lookup("dataRoot");
			shadersRoot  = (const char*) cfg.lookup("shadersRoot");
			coreDataRoot = (const char*) cfg.lookup("coreDataRoot");
			scriptsRoot  = (const char*) cfg.lookup("scriptsRoot");
			
			maxSingleWindowWidth  = cfg.lookup("maxSingleWindowWidth");
			maxSingleWindowHeight = cfg.lookup("maxSingleWindowHeight");


	cout << saveRoot << endl;
	ReadFromFile();
}

void ConfigParser::WriteToFile()
{
	cout << "writing to file (not really)" << endl;
}

void ConfigParser::ReadFromFile()
{
	cout << "reading from file (not really)" << endl;
}