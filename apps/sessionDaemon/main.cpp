#include <iostream>
using std::cout;
using std::endl;

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include <boost/filesystem.hpp>
#include <fstream>
#include <sstream>

using std::endl;

int main(void)
{
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
			boost::filesystem::directory_iterator di(p), endi;
			if( di != endi )
			{
				std::string nextJobFn = di->path().string();
				
				//
				// process the job
				//
				std::ifstream infi(nextJobFn);
				std::string rawSource;
				infi >> rawSource;
				
				std::string outTarget;
				infi >> outTarget;
				
				std::string alg;
				infi >> alg;
				
				
				// make sure the directories above the output target exist.
				boost::filesystem::path outp( outTarget );
				auto outDir = outp.parent_path();
				boost::filesystem::create_directories(outDir);
				
				log << "od: " << outDir << endl;
				
				
				// more use of std::system. I mean, why not right?
				// The biggest problem here is maybe the presumption of 200 Hz.
				// The crf of 10 may be good or may be too high quality. <shrug>.
				std::stringstream cmd;
				cmd << "/opt/mc_bin/debayer " << rawSource << " " << outTarget << " " << alg << " 200 10 " << endl;
				log << cmd.str() << endl;
				std::system(cmd.str().c_str());
				
				//
				// delete the job file
				//
				boost::filesystem::remove(di->path());
				
			}
		}
		else
		{
			log << "no todo directory!" << endl;
			exit(-1);
		}
		
		
		
		
		
	}
}
