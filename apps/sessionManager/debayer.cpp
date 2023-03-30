#include "mainWindow.h"
#include "misc/tokeniser.h"

#include <boost/filesystem.hpp>

#include <set>
#include <queue>
#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;



void MainWindow::DebayerHelpClick()
{
	//
	// I think the easiest thing we can do here is launch an external web browser
	// to open a simple html guide of calibration.
	//
	// A basic "system" call should be safe here as we don't need to block.
	//
	std::system( "firefox /opt/software/mc_dev/mc_grabber/docs/html/index.html");
}



void MainWindow::DebayerAddSessionClick()
{
	auto ssel = sessionsLBox.get_selected();
	if( ssel.size() > 0 )
	{
		std::string sn = sessionsLBox.get_text( ssel[0] );
		std::stringstream ss;
		ss << "(session) " << sn;
		
		if( debayerModeRBLED.get_active() )
			ss << " LED ";
		if( debayerModeRBCVEA.get_active() )
			ss << " OpenCV_EA ";
		if( debayerModeRBFCNN.get_active() )
			ss << " FCNN ";
		
		debayerJobsList.append( ss.str() );
		
		if( mirrorToRaidCheck.get_active() )
		{
			cout << "add session mirror job" << endl;
			ss.str("");
			ss << "(mirror) session " << sn;
			debayerJobsList.append( ss.str() );
		}
	}
}


void MainWindow::DebayerAddTrialClick()
{
	auto ssel = sessionsLBox.get_selected();
	auto tsel = trialsLBox.get_selected();
	if( ssel.size() > 0 && tsel.size() > 0)
	{
		std::string sn = sessionsLBox.get_text( ssel[0] );
		std::string tn = trialsLBox.get_text( tsel[0] );
		
		std::stringstream ss;
		ss << "(trial) " << sn << " " << tn;
		
		if( debayerModeRBLED.get_active() )
			ss << " LED ";
		if( debayerModeRBCVEA.get_active() )
			ss << " OpenCV_EA ";
		if( debayerModeRBFCNN.get_active() )
			ss << " FCNN ";
		
		debayerJobsList.append( ss.str() );
		
		if( mirrorToRaidCheck.get_active() )
		{
			cout << "add trial mirror job" << endl;
			ss.str("");
			ss << "(mirror) trial " << sn << " " << tn;
			debayerJobsList.append( ss.str() );
		}
	}
}


void MainWindow::DebayerAddCameraClick()
{
	auto ssel = sessionsLBox.get_selected();
	auto tsel = trialsLBox.get_selected();
	auto csel = camsLBox.get_selected();
	if( ssel.size() > 0 && tsel.size() > 0 && csel.size() > 0)
	{
		std::string sn = sessionsLBox.get_text( ssel[0] );
		std::string tn = trialsLBox.get_text( tsel[0] );
		
		int idx = csel[0];
		
		std::stringstream ss;
		ss << "(camera) " << sn << " " << tn << " " << idx;
		
		if( debayerModeRBLED.get_active() )
			ss << " LED ";
		if( debayerModeRBCVEA.get_active() )
			ss << " OpenCV_EA ";
		if( debayerModeRBFCNN.get_active() )
			ss << " FCNN ";
		
		debayerJobsList.append( ss.str() );
		
		if( mirrorToRaidCheck.get_active() )
		{
			cout << "add camera mirror job" << endl;
			ss.str("");
			ss << "(mirror) camera " << sn << " " << tn << " " << idx;
			debayerJobsList.append( ss.str() );
		}
	}
}


void MainWindow::DebayerRemoveJobClick()
{
	//
	// This all seems perversely round-about - but it works, so thank the internet for 
	// someone else figuring it out.
	//
	
	Gtk::ListViewText::SelectionList dsel = debayerJobsList.get_selected();
	if( dsel.empty() )
		return;
	
	Glib::RefPtr<Gtk::TreeSelection> ts = debayerJobsList.get_selection();
	Gtk::TreeModel::iterator sel_it = ts->get_selected();
	
	Glib::RefPtr<Gtk::TreeModel> reftm = debayerJobsList.get_model();
	Glib::RefPtr<Gtk::ListStore> refLStore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(reftm);
	if(refLStore)
	{
		refLStore->erase(sel_it);
	}
}


void MainWindow::DebayerProcessJobsClick()
{
	mainGrid.set_sensitive(false);
	cout << "processing debayer jobs" << endl;
	
	
	//
	// Create jobs files for the sessionDaemon.
	//
	cout << "getting jobs..." << endl;
	std::set< std::string > jobsSet;
	for( unsigned jc = 0; jc < debayerJobsList.size(); ++jc )
	{
		jobsSet.insert( debayerJobsList.get_text( jc ) );
	}
	
	std::queue< std::string > jobsQ;
	
	for( auto ji = jobsSet.begin(); ji != jobsSet.end(); ++ji )
	{
		jobsQ.push( *ji );
	}
	
	jobsSet.clear();
	
	while( jobsQ.size() > 0 )
	{
		auto job = jobsQ.front();
		jobsQ.pop();
		
		// tokenise the string
		auto tokens = SplitLine( job, " " );
		
		// first token is (session) (trial) or (camera)
		if( tokens[0].compare("(session)") == 0 )
		{
			// We need jobs for all trials of this session
			std::string sn  = tokens[1];
			std::string alg = tokens[2];
			for( auto ti = sessions[sn].trials.begin(); ti != sessions[sn].trials.end(); ++ti )
			{
				std::stringstream ss;
				ss << "(trial) " << sn << " " << ti->first << " " << alg;
				jobsQ.push( ss.str() );
			}
		}
		else if( tokens[0].compare("(trial)") == 0 )
		{
			// we need jobs for all cameras of this trial.
			std::string sn = tokens[1];
			std::string tn = tokens[2];
			std::string alg = tokens[3];
			for( unsigned cc = 0; cc < sessions[sn].trials[tn].cameras.size(); ++cc )
			{
				std::stringstream ss;
				ss << "(camera) " << sn << " " << tn << " " << cc << " " << alg;
				jobsQ.push( ss.str() );
			}
		}
		else if( tokens[0].compare("(camera)") == 0 )
		{
			std::string sn  = tokens[1];
			std::string tn  = tokens[2];
			unsigned idx    = atoi( tokens[3].c_str() );
			std::string alg = tokens[4];
			
			ScameraInfo &ci = sessions[sn].trials[tn].cameras[idx];
			
			//
			// We don't do any processing in the manager. Rather, the job of the manager is to feed the 
			// damon with jobs. 
			//
			// We can't just send through the command to be run - that would be a security risk....
			//
			std::string fps; // TODO: Where to get this from? Session config file which we've not processed yet.
			fps = "200";
			if( tn.find("calib") != std::string::npos )
				fps = "5";
			std::stringstream ss;
			ss << "debayer " << ci.rawPath << " " 
			                 << processedSessionsRoot << "/" << sn << "/" << tn << "/" << std::setw(2) << std::setfill('0') << ci.id << ".mp4"
			                 << " " << alg
			                 << " " << fps
			                 << endl;
			
			jobsSet.insert( ss.str() );
		}
		else if( tokens[0].compare("(mirror)") == 0 )
		{
			cout << job << endl;
			std::stringstream ss;
			ss << "mirror " << processedSessionsRoot << "/ ";
			if( tokens[1].compare("session") == 0 )
			{
				std::string sn  = tokens[2];
				
				ss << sn << "/ ";
			}
			else if( tokens[1].compare("trial") == 0 )
			{
				std::string sn  = tokens[2];
				std::string tn  = tokens[3];
				ss << sn << "/" << tn << "/";
			}
			else if( tokens[1].compare("camera") == 0 )
			{
				std::string sn  = tokens[2];
				std::string tn  = tokens[3];
				unsigned idx    = atoi( tokens[4].c_str() );
				
				ss << sn << "/" << tn << "/" << std::setw(2) << std::setfill('0') << idx << ".mp4 ";
			}
			
			ss << " " << raidHostEntry.get_text() << " " << raidUserEntry.get_text() << " " << raidDirEntry.get_text();
			cout << ss.str() << endl;
			jobsSet.insert( ss.str() );
		}
	}
	
	cout << "making job files..." << endl;
	int i = 0;
	for( auto ji = jobsSet.begin(); ji != jobsSet.end(); ++ji )
	{
		//
		// Create a temporary file and write to it.
		//
		
		// for a unique job id, get the time and a random number:
		auto n = std::chrono::steady_clock::now().time_since_epoch().count();
		unsigned r = rand()%1000;
		
		std::stringstream ss;
		ss << "/tmp/" << n << "-" << std::setw(4) << std::setfill('0') << i << "-" << std::setw(4) << std::setfill('0') << r;
		std::string fn = ss.str();
		
		std::ofstream outfi(fn);
		if( !outfi )
		{
			throw std::runtime_error("Couldn't open temporary job file: " + fn );
		}
		
		outfi << *ji;
		outfi.close();
		
		// move the job file where the demon will see it
		ss.str("");
		ss << "mv " << fn << " /opt/sessionDaemon/todo/";
		std::system( ss.str().c_str() );
		
		++i;
	}
	
	cout << "done..." << endl;
	
	mainGrid.set_sensitive(true);
	debayerJobsList.clear_items();
}



bool MainWindow::CheckDemonStatus()
{
	bool retVal;
	
	auto checkVal = std::system("pidof -x sessionDaemon > /dev/null");
// 	cout << "checkVal " <<  checkVal << endl;
	if( 0 == checkVal )
	{
		demonStatusLabel.set_text("status: running");
		demonStatusLabel.override_color (Gdk::RGBA("green"), Gtk::STATE_FLAG_NORMAL);
		retVal = true;
	}
	else
	{
		retVal = false;
		demonStatusLabel.set_text("status: stopped");
		demonStatusLabel.override_color (Gdk::RGBA("red"), Gtk::STATE_FLAG_NORMAL);
	}
	
	int cnt = 0;
	boost::filesystem::path p("/opt/sessionDaemon/todo/");
	if( boost::filesystem::exists(p) && boost::filesystem::is_directory(p))
	{
		boost::filesystem::directory_iterator di(p), endi;
		for( ; di != endi; ++di )
		{
			std::string fn = di->path().string();
			if( fn.find("000-stopSessionDemon") != std::string::npos && retVal == true)
			{
				demonStatusLabel.set_text("status: stopping");
				demonStatusLabel.override_color (Gdk::RGBA("orange"), Gtk::STATE_FLAG_NORMAL);
			}
			++cnt;
		}
	}
	
	std::stringstream ss;
	ss << "  jobs:   " << cnt;
	demonJobsLabel.set_text(ss.str() );
	
	
	boost::filesystem::path ep("/opt/sessionDaemon/errorFlag");
	if( boost::filesystem::exists(ep) )
	{
		// open file. it should have a single line on it, which says the last time there was an error.
		std::ifstream effi( ep.str() );
		std::string errStr = std::get_line( effi );
		std::stringstream ss;
		ss << "Last Error: " << errStr << endl; 
		demonErrLabel.set_text( ss.str() );
		demonErrLabel.override_color (Gdk::RGBA("red"), Gtk::STATE_FLAG_NORMAL);
	}
	else
	{
		demonErrLabel.set_text( "No errors" );
		demonErrLabel.override_color (Gdk::RGBA("green"), Gtk::STATE_FLAG_NORMAL);
	}
	
	return retVal;
}

void MainWindow::DemonClearClick()
{
	boost::filesystem::path ep("/opt/sessionDaemon/errorFlag");
	if( boost::filesystem::exists(ep) )
	{
		boost::filesystem::remove(ep);
	}
}

bool MainWindow::CheckDemonStatusAndStart()
{
	if( !CheckDemonStatus() )
	{
		
		boost::filesystem::path p("/opt/sessionDaemon/todo/");
		if( !boost::filesystem::exists(p) )
		{
			boost::filesystem::create_directories(p);
		}
		
		cout << "daemon not running -> starting daemon: " << daemonBinary << endl;
		
		//
		// sessionDaemon is not running, so start that process in the background.
		// I'm sure this is bad form and will be frowned upon
		//
		std::system(daemonBinary.c_str());
		
		sleep(1);
	}
	
	return CheckDemonStatus();
}


bool MainWindow::CheckDemonStatusTimer()
{
	CheckDemonStatus();
	return true;
}

void MainWindow::DemonStopClick()
{
	std::string fn = "/tmp/000-stopSessionDemon";
	std::ofstream outfi(fn);
	if( !outfi )
	{
		throw std::runtime_error("Couldn't open temporary job file: " + fn );
	}
	
	outfi << "stop";
	outfi.close();
	
	// move the job file where the demon will see it
	std::stringstream ss;
	ss << "mv " << fn << " /opt/sessionDaemon/todo/";
	std::system( ss.str().c_str() );
}

void MainWindow::DemonStartClick()
{
	CheckDemonStatusAndStart();
}

void MainWindow::DemonClearClick()
{
	system( "rm /opt/sessionDaemon/todo/*" );
}


void MainWindow::DemonLogClick()
{
	//
	// Perversely simplistic way of viewing the demon log
	//
	ss.str("");
	ss << "konsole --workdir ~/ -e kate /opt/sessionDaemon/log &";
	cout << ss.str() << endl;
	std::system( ss.str().c_str() );
}
