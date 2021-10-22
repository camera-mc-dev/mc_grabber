#include "mainWindow.h"
#include "misc/splitLine.h"

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
	std::system( "firefox /opt/sessionManager/DebayerGuide.html");
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
	
	if( 0 == std::system("pidof -x sessionDaemon > /dev/null") )
	{
		cout << "daemon is running" << endl;
	}
	else
	{
		cout << "daemon not running -> starting daemon" << endl;
		//
		// sessionDaemon is not running, so start that process in the background.
		// I'm sure this is bad form and will be frowned upon
		//
		std::system("/opt/mc_bin/sessionDaemon");
		
		if( 0 == std::system("pidof -x sessionDaemon > /dev/null") )
		{
			cout << "daemon is running" << endl;
		}
		else
		{
			throw std::runtime_error("Can't start debayer sessionDaemon :(" );
		}
	}
	
	
	
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
			// We will create a string which are the arguments for the debayer tool.
			// We put those job strings in a set so that we only have one of them!
			// (Note: We still might have multiple algorithms on a single session/trial/camera
			//  in which case only the first will get processed because after that the output already exists)
			//
			
			std::stringstream ss;
			
			if( ci.rawPath.find("/data/raid0") == 0 )
			{
				ss << ci.rawPath 
				<< " /data/raid0/recording/" << sn << "/rgb/" << tn << "/" << std::setw(2) << std::setfill('0') << ci.id
				<< ".mp4 " << alg << endl;
			}
			else if( ci.rawPath.find("/data/raid1") == 0 )
			{
				ss << ci.rawPath 
				<< " /data/raid1/recording/" << sn << "/rgb/" << tn << "/" << std::setw(2) << std::setfill('0') << ci.id
				<< ".mp4 " << alg << endl;
			}
			else
			{
				throw std::runtime_error("Expected camera rawPath to be /data/raid0/... or /data/raid1/..., but got: " + ci.rawPath );
			}
			
			jobsSet.insert( ss.str() );
		}
	}
	
	cout << "making job files..." << endl;
	for( auto ji = jobsSet.begin(); ji != jobsSet.end(); ++ji )
	{
		//
		// Create a temporary file and write to it.
		//
		
		// for a unique job id, get the time and a random number:
		auto n = std::chrono::steady_clock::now().time_since_epoch().count();
		unsigned r = rand()%1000;
		
		std::stringstream ss;
		ss << "/tmp/" << n << "-" << r << "-" << std::setw(2) << std::setfill('0');
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
	}
	
	cout << "done..." << endl;
	
	mainGrid.set_sensitive(true);
	debayerJobsList.clear_items();
}



bool MainWindow::CheckDemonStatus()
{
	if( 0 == std::system("pidof -x sessionDaemon > /dev/null") )
	{
		demonStatusLabel.set_text("running");
	}
	else
	{
		cout << "daemon not running -> starting daemon" << endl;
		//
		// sessionDaemon is not running, so start that process in the background.
		// I'm sure this is bad form and will be frowned upon
		//
		std::system("/opt/mc_bin/sessionDaemon");
		
		if( 0 == std::system("pidof -x sessionDaemon > /dev/null") )
		{
			demonStatusLabel.set_text("running");
		}
		else
		{
			throw std::runtime_error("Can't start debayer sessionDaemon :(" );
		}
	}
	
	boost::filesystem::path p("/opt/sessionDaemon/todo/");
	int cnt = 0;
	if( boost::filesystem::exists(p) && boost::filesystem::is_directory(p))
	{
		boost::filesystem::directory_iterator di(p), endi;
		for( ; di != endi; ++di )
		{
			++cnt;
		}
	}
	
	std::stringstream ss;
	ss << cnt << " jobs remaining" << endl;
	demonJobsLabel.set_text(ss.str() );
	
	return true;
}
