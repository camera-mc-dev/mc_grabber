#include "mainWindow.h"


#include <boost/filesystem.hpp>

#include <set>
#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;

#include "libconfig.h++"


void MainWindow::CalibHelpClick()
{
	//
	// I think the easiest thing we can do here is launch an external web browser
	// to open a simple html guide of calibration.
	//
	// A basic "system" call should be safe here as we don't need to block.
	//
	std::system( "firefox /opt/sessionManager/CalibGuide.html");
}


void MainWindow::CalibRunClick()
{
	//
	// Create the calibration config file
	//
	std::string filename;
	if( !CreateCalibConfig(false, filename) )
	{
		cout << "aborting calib due to previous error" << endl;
		return;
	}
	
	
	//
	// Run the calibration. We start a process that can run in the background
	// without blocking the session manager.
	// TODO: Run a process in a way we know that it has completed!
	//
	std::stringstream ss;
	ss << "konsole --workdir ~/ -e /opt/mc_bin/circleGridCamNetwork " << filename;
	cout << ss.str() << endl;
	std::system( ss.str().c_str() );
	
	//
	// TODO: Update the session/trial once this completes.
	//
}


void MainWindow::CalibPMRunClick()
{
	//
	// Create the calibration config file
	//
	std::string filename;
	if( !CreateCalibConfig(true, filename) )
	{
		cout << "aborting point matcher run due to previous error" << endl;
		return;
	}
	
	
	//
	// Run the point matcher. We start a process that can run in the background
	// without blocking the session manager.
	// TODO: Run a process in a way we know that it has completed!
	//
	std::stringstream ss;
	ss << "konsole --workdir ~/ -e /opt/mc_bin/pointMatcher " << filename;
	cout << ss.str() << endl;
	std::system( ss.str().c_str() );
	
	//
	// TODO: Update the session/trial once this completes.
	//
}


void MainWindow::CalibAlignRunClick()
{
}

void MainWindow::CalibCheckRunClick()
{
	//
	// Create the calibration config file
	//
	std::string filename;
	if( !CreateCalibConfig(false, filename) )
	{
		cout << "aborting calib check run due to previous error" << endl;
		return;
	}
	
	
	//
	// Run the point matcher. We start a process that can run in the background
	// without blocking the session manager.
	// TODO: Run a process in a way we know that it has completed!
	//
	std::stringstream ss;
	ss << "konsole --workdir ~/ -e /opt/mc_bin/calibCheck " << filename;
	cout << ss.str() << endl;
	std::system( ss.str().c_str() );
	
	//
	// TODO: Update the session/trial once this completes.
	//
}



bool MainWindow::CreateCalibConfig( bool forPointMatcher, std::string &out_filename )
{
	try
	{
		auto ssel = sessionsLBox.get_selected();
		auto tsel = trialsLBox.get_selected();
		auto csel = camsLBox.get_selected();
		if( ssel.size() == 0 || tsel.size() == 0 )
		{
			return false;
		}
		
		std::string sn = sessionsLBox.get_text( ssel[0] );
		std::string tn = trialsLBox.get_text( tsel[0] );
		
		Strial &trial = sessions[sn].trials[tn];
		if( !trial.isCalib )
		{
			cout << "trial is not calib, aborting calib file write: " << sn << " " << tn << endl;
			return false;
		}
		
		std::stringstream ss;
		ss << "/data/raid0/recording/";
		
		std::string dataRoot = ss.str();
		
		ss.str("");
		ss << sn << "/rgb/" << tn << "/";
		std::string testRoot = ss.str();
		
		ss.str("");
		ss << dataRoot << testRoot << "/sm-calib.cfg"; // sm because this is the file written by the session manager
		out_filename = ss.str();
		
		libconfig::Config cfg;
		auto &cfgRoot = cfg.getRoot();
		
		cfgRoot.add("dataRoot", libconfig::Setting::TypeString) = dataRoot;
		cfgRoot.add("testRoot", libconfig::Setting::TypeString) = testRoot;
		
		cfgRoot.add("imgDirs", libconfig::Setting::TypeList );
		auto &id = cfg.lookup("imgDirs");
		for( unsigned cc = 0; cc < trial.cameras.size(); ++cc )
		{
			if( !trial.cameras[cc].isDebayered )
			{
				cout << "camera not debayered, aborting calib file write: " << sn << " " << tn << " " << cc << endl;
				return false;
			}
			auto i = trial.cameras[cc].rgbPath.rfind("/")+1;
			std::string tmp( trial.cameras[cc].rgbPath.begin()+i, trial.cameras[cc].rgbPath.end() );
			id.add( libconfig::Setting::TypeString ) = tmp;
		}
		cfgRoot.add("maxGridsForInitial", libconfig::Setting::TypeInt ) = 50;
		
		auto &g = cfgRoot.add("grid", libconfig::Setting::TypeGroup );
		g.add("rows", libconfig::Setting::TypeInt ) =  9;
		g.add("cols", libconfig::Setting::TypeInt ) = 10;
		g.add("rspacing", libconfig::Setting::TypeFloat ) = 78.5;
		g.add("cspacing", libconfig::Setting::TypeFloat ) = 78.5;
		g.add("isLightOnDark", libconfig::Setting::TypeBoolean ) = false;
		g.add("useHypothesis", libconfig::Setting::TypeBoolean ) = false;
		g.add("hasAlignmentDots", libconfig::Setting::TypeBoolean ) = true;
		
		
		cfgRoot.add("useExistingGrids", libconfig::Setting::TypeBoolean ) = calibUseExGridsCheck.get_active();
		cfgRoot.add("useExistingIntrinsics", libconfig::Setting::TypeBoolean ) = false;
		cfgRoot.add("noDistortionOnInitial", libconfig::Setting::TypeBoolean ) = true;
		cfgRoot.add("forceOneCam", libconfig::Setting::TypeBoolean ) = true;
		cfgRoot.add("numDistortionToSolve", libconfig::Setting::TypeInt ) =  2;
		cfgRoot.add("numIntrinsicsToSolve", libconfig::Setting::TypeInt ) =  3;
		cfgRoot.add("useSBA", libconfig::Setting::TypeBoolean ) = calibUseBundleCheck.get_active();
		cfgRoot.add("SBAVerbosity", libconfig::Setting::TypeInt ) =  3;
		cfgRoot.add("minSharedGrids", libconfig::Setting::TypeInt ) =  30;
		
		
		
		cfgRoot.add("originFrame", libconfig::Setting::TypeInt ) =  1;
		cfgRoot.add("targetDepth", libconfig::Setting::TypeFloat ) =  0.0;
		cfgRoot.add("alignXisNegative", libconfig::Setting::TypeBoolean ) = false;
		cfgRoot.add("alignYisNegative", libconfig::Setting::TypeBoolean ) = false;
		cfgRoot.add("visualise", libconfig::Setting::TypeInt ) =  4;
		
		if( forPointMatcher || calibUseMatchesCheck.get_active() )
		{
			cfgRoot.add("matchesFile", libconfig::Setting::TypeString) = "matches";
			auto &fi = cfgRoot.add("frameInds", libconfig::Setting::TypeList );
			for( unsigned cc = 0; cc < trial.cameras.size(); ++cc )
			{
				int i = calibVisFrameSpins[cc].get_value();
				fi.add(libconfig::Setting::TypeInt) = i;
			}
			cfgRoot.add("winX", libconfig::Setting::TypeInt ) =  1800;
			cfgRoot.add("winY", libconfig::Setting::TypeInt ) =  1000;
			
			auto &ap = cfgRoot.add("axisPoints", libconfig::Setting::TypeList );
			ap.add( libconfig::Setting::TypeInt ) = 0;
			ap.add( libconfig::Setting::TypeInt ) = 1;
			ap.add( libconfig::Setting::TypeInt ) = 2;
		}
		
		cout << "writing: " << out_filename << endl;
		cfg.writeFile( out_filename.c_str() );
	}
	catch( libconfig::SettingException &e)
	{
		cout << "Setting error: " << endl;
		cout << e.what() << endl;
		cout << e.getPath() << endl;
		return false;
	}
	catch( libconfig::ParseException &e )
	{
		cout << "Parse error:" << endl;
		cout << e.what() << endl;
		cout << e.getError() << endl;
		cout << e.getFile() << endl;
		cout << e.getLine() << endl;
		return false;
	}
	
	
	
	return true;
}
