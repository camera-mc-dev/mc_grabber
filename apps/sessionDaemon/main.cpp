#include <iostream>
using std::cout;
using std::endl;

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include <boost/filesystem.hpp>
#include <fstream>
#include <sstream>
#include <pwd.h>
#include "libconfig.h++"

using std::endl;

int main(void)
{
	//
	// We need to check a couple things from the grabber config.
	//
	// 1) what is the debayer binary?
	//
	
	std::string debayerBin;
	
	std::string userHome;
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
		throw std::runtime_error("No grabber config found at: " + p.string());	
	}
	try
	{
		libconfig::Config cfg;
		cfg.readFile( ss.str().c_str() );
		
		debayerBin = (const char*)cfg.lookup("debayerBinary");
		
	}
	catch( libconfig::SettingException &e)
	{
		cout << "Setting error: " << endl;
		cout << e.what() << endl;
		cout << e.getPath() << endl;
		exit(0);
	}
	
	
	//
	// The sessionDaemon needs to be a daemon process, i.e.
	// it runs in the background without stopping.
	//
	// To do that, we fork()...
	//
	pid_t pid;
	pid = fork();
	
	if( pid < 0 )
	{
		cout << "error forking daemon" << endl;
		exit(-1);
	}
	
	// now there's two versions of this process running.
	// The first one just ends here.
	if (pid > 0)
		exit(0);
	
	// the new one carries on, but is now in the background and just running.
	
	// I think this will stop it being killed when the terminal that created it closes.
	signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
	
	std::ofstream log("/opt/sessionDaemon/log", std::ios::app );
	
	while(1)
	{
		//
		// Look in the directory /opt/sessionDaemon/todo for a job
		//
		boost::filesystem::path p( "/opt/sessionDaemon/todo" );
		if( boost::filesystem::exists(p) && boost::filesystem::is_directory(p))
		{
			// we don't know what order the filesystem will give us files. 
			// but we know that the grabber will give each file a name such that 
			// there is a logical order to them. Even more though, it _might_ send a
			// special control system file, which _has_ to be done next. 
			// so we sort the filenames to find the next job.
			std::vector< boost::filesystem::path > paths;
			boost::filesystem::directory_iterator di(p);
			while( di != boost::filesystem::directory_iterator{} )
			{
				cout << di->path() << endl;
				if(boost::filesystem::is_regular_file( di->path() ) )
					paths.push_back( di->path() );
				++di;
			}
			
			if( paths.size() > 0 )
			{
				std::sort( paths.begin(), paths.end() );
				
				
				std::string nextJobFn = paths[0].string();
				
				//
				// process the job
				//
				std::ifstream infi(nextJobFn);
				std::string command;
				infi >> command;
				if( command.compare("debayer") == 0 )
				{
					std::string rawSource;
					infi >> rawSource;
					
					std::string outTarget;
					infi >> outTarget;
					
					std::string alg;
					infi >> alg;
					
					std::string fps;
					infi >> fps;
					
					
					// make sure the directories above the output target exist.
					boost::filesystem::path outp( outTarget );
					auto outDir = outp.parent_path();
					boost::filesystem::create_directories(outDir);
					
					log << "od: " << outDir << endl;
					
					
					// 
					// We'll assume debayering to h265 with a crf of 15 - which is quite high quality
					// in a way I'd rather use yuv444p but, well, the debayering is probably so poor that anything more that yuv420p 
					// is probably pointless anyway.
					//
					// We'll pipe the output to null to keep things quiet
					// TODO: upgrade from the "system" call to something that will give us a return value.
					std::stringstream cmd;
					cmd << debayerBin << " " << rawSource << " GRBG " << outTarget << " " << alg << " " << fps << " h265 15 yuv420p >> /dev/null 2>&1" << endl;
					
					// some simplistic checks.
					if( boost::filesystem::exists(outp) )
					{
						log << "debayer " << rawSource << " -> " << outTarget << " successfully created file" << endl;
					}
					else
					{
						log << "ERROR: " << "debayer " << rawSource << " -> " << outTarget << " failed to create file. Command was: " << cmd.str() << endl;
					}
					
					std::system(cmd.str().c_str());
					
				}
				else if( command.compare("stop") == 0 )
				{
					boost::filesystem::remove(paths[0]);
					log << "got -stop- command. Quitting demon." << endl;
					exit(0);
				}
				else
				{
					std::vector< std::string > params;
					while(infi)
					{
						std::string x;
						infi >> x;
						if( infi )
							params.push_back(x);
					}
					log << "did not understand command: " << command;
					for( unsigned pc = 0; pc < params.size(); ++pc )
					{
						log << " (" << params[pc] << ")";
					}
					log << " -> ignoring and deleting job file" << endl;
				}
				
				
				//
				// delete the job file
				//
				boost::filesystem::remove(paths[0]);
			}
		}
		else
		{
			log << "no todo directory!" << endl;
			exit(-1);
		}
		
		
		
		
		
	}
}
