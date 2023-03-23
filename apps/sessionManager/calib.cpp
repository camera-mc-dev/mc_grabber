#include "mainWindow.h"


#include <boost/filesystem.hpp>

#include <set>
#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;

#include "commonConfig/commonConfig.h"
#include "libconfig.h++"


void MainWindow::CalibHelpClick()
{
	//
	// I think the easiest thing we can do here is launch an external web browser
	// to open a simple html guide of calibration.
	//
	// A basic "system" call should be safe here as we don't need to block.
	//
	std::system( "firefox /opt/software/mc_dev/mc_grabber/docs/html/0003.html &");
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
	ss << "konsole --workdir ~/ -e " << calibBinariesDir << "/circleGridCamNetwork " << filename;
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
	ss << "konsole --workdir ~/ -e  " << calibBinariesDir << "/pointMatcher " << filename;
	cout << ss.str() << endl;
	std::system( ss.str().c_str() );
	
	//
	// TODO: Update the session/trial once this completes.
	//
}


void MainWindow::CalibAlignRunClick()
{
	//
	// Create the calibration config file
	//
	std::string filename;
	if( !CreateCalibConfig(true, filename) )
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
	ss << "konsole --workdir ~/ -e  " << calibBinariesDir << "/manualAlignNetwork " << filename;
	cout << ss.str() << endl;
	std::system( ss.str().c_str() );
	
	//
	// TODO: Update the session/trial once this completes.
	//
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
	ss << "konsole --workdir ~/ -e  " << calibBinariesDir << "/calibCheck " << filename;
	cout << ss.str() << endl;
	std::system( ss.str().c_str() );
	
	//
	// TODO: Update the session/trial once this completes.
	//
}

void MainWindow::CalibMGPClick()
{
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
	// TODO: options for how much of the ground plane to view.
	std::stringstream ss;
	ss << "konsole --workdir ~/ -e  " << calibBinariesDir << "/makeGroundPlaneImage " << filename << " -3000 -3000 6000 1000 ";
	cout << ss.str() << endl;
	std::system( ss.str().c_str() );
	
}

bool MainWindow::CreateCalibConfig( bool forPointMatcher, std::string &out_filename )
{
	
	//
	// Check what the currently selected session and trial are, and make sure 
	// that it is a "calib_" trial.
	//
	auto ssel = sessionsLBox.get_selected();
	auto tsel = trialsLBox.get_selected();
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
	
	
	std::string calibConfigFile;
	
	
	//
	// This is where things get a little bit tricky.
	//
	// If a person has done a live grid detection, then they already have debayered images in
	// the "raw" calib directory, and can already start to work on a calibration.
	//
	// However, it could also be that there is a "debayered" calib video as well as this raw data,
	// so how do we know what we should be using?
	//
	// The answer is that we have to ask the user, so we now have that option.
	//
	
	
	//
	// There are 3 possible sources for the calibration file we open up:
	//
	CommonConfig ccfg;
	std::stringstream pss, rss, bss;
	pss << processedSessionsRoot << "/" << sn << "/" << tn << "/calib.cfg";
	rss << recPaths[0] << "/" << sn << "/" << tn << "/calib.cfg";
	bss << ccfg.coreDataRoot << "/baseConfigs/calib.cfg";
	
	// but which do we use?
	boost::filesystem::path cfgPth;
	boost::filesystem::path rpth( rss.str() ), ppth( pss.str() ), bpth( bss.str() );
	if( calibProcRadioBtn.get_active() )
	{
		cfgPth = ppth;
	}
	else if( calibRawRadioBtn.get_active() )
	{
		cfgPth = rpth;
	}
	
	try
	{
		libconfig::Config cfg;
		
		if( boost::filesystem::exists( cfgPth ) )
		{
			cfg.readFile( boost::filesystem::canonical( cfgPth ).string().c_str() );
		}
		else
		{
			// need to use the base config and initialise parts of it.
			cfg.readFile( boost::filesystem::canonical( bpth ).string().c_str() );
			
			if( calibProcRadioBtn.get_active() )
			{
				//
				// Set the data root and trial root.
				//
				std::stringstream tss;
				tss << sn << "/" << tn << "/";
				cfg.lookup("dataRoot")  = processedSessionsRoot;
				cfg.lookup("testRoot")  = tss.str();
				
				//
				// Set the image dirs.
				//
				libconfig::Setting &idirs = cfg.lookup("imgDirs");
				while( idirs.getLength() > 0 )
					idirs.remove(0u);
				for( unsigned cc = 0; cc < sessions[sn].trials[tn].cameras.size(); ++cc )
				{
					std::stringstream css;
					css << std::setw(2) << std::setfill('0') << cc << ".mp4";
					idirs.add( libconfig::Setting::TypeString );
					idirs[cc] = css.str().c_str();
				}
				cout << "check idirs: " << sessions[sn].trials[tn].cameras.size() << " " << idirs.getLength() << endl;
				assert( idirs.getLength() == sessions[sn].trials[tn].cameras.size() );
				
				
				//
				// Just a few sensible values a little different from the defaults
				//
				cfg.lookup("noDistortionOnInitial") = true;
				cfg.lookup("useExistingIntrinsics") = false;
				cfg.lookup("forceOneCam")           = false;
				cfg.lookup("minSharedGrids")        =    40;
				
				
				cfg.writeFile( cfgPth.string().c_str() );
				
				cfg.readFile( cfgPth.string().c_str() );
			}
			else
			{
				cout << "not initialising calib for raw data. It would already exist if used liveGridDetect." << endl;
				return false;
			}
		}
		
		
		cfg.lookup("targetDepth")      = atof( calibOriginHeightEntry.get_text().c_str() );
		cfg.lookup("alignXisNegative") = calibXAxisIsNegCheck.get_active();
		cfg.lookup("alignYisNegative") = calibYAxisIsNegCheck.get_active();
		
		cfg.lookup("useExistingGrids") = calibUseExGridsCheck.get_active();
		cfg.lookup("useSBA") = calibUseBundleCheck.get_active();
		
		auto &cfgRoot = cfg.getRoot();
		if( cfg.exists("matchesFile") )
		{
			cfgRoot.remove("matchesFile");
			cfgRoot.remove("frameInds");
			cfgRoot.remove("winX");
			cfgRoot.remove("winY");
		}
		
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
			
		}
		
		cfg.writeFile( cfgPth.string().c_str() );
		
		out_filename = cfgPth.string();
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


void MainWindow::Raw2ProcClick()
{
	// TODO: copy calib config file to new location, updating the dataroot
	//       copy grid files to new location
	//       copy calibration files to new location.
	
	
	auto ssel = sessionsLBox.get_selected();
	auto tsel = trialsLBox.get_selected();
	if( ssel.size() == 0 || tsel.size() == 0 )
	{
		return;
	}
	
	std::string sn = sessionsLBox.get_text( ssel[0] );
	std::string tn = trialsLBox.get_text( tsel[0] );
	Strial &trial = sessions[sn].trials[tn];
	if( !trial.isCalib )
	{
		cout << "trial is not calib, aborting calib file write: " << sn << " " << tn << endl;
		return;
	}
	
	
	
	std::stringstream pss, rss;
	pss << processedSessionsRoot << "/" << sn << "/" << tn << "/calib.cfg";
	rss << recPaths[0] << "/" << sn << "/" << tn << "/calib.cfg";
	
	
	
	try
	{
		
		libconfig::Config cfg;
		
		if( boost::filesystem::exists( rss.str() ) )
		{
			cfg.readFile( rss.str().c_str() );
			
			std::stringstream tss;
			tss << sn << "/" << tn << "/";
			cfg.lookup("dataRoot")  = processedSessionsRoot;
			cfg.lookup("testRoot")  = tss.str();
			
			// we know that it currently points to directories, we need it to point to videos.
			libconfig::Setting &idirs = cfg.lookup("imgDirs");
			for( unsigned dc = 0; dc < idirs.getLength(); ++dc )
			{
				std::string dn = (const char*) idirs[dc];
				std::stringstream ss;
				for( unsigned c = 0; c < dn.size(); ++c )
				{
					if( dn[c] != '/' )
						ss << dn[c];
				}
				ss << ".mp4";
				idirs[dc] = ss.str().c_str();
			}
			
			cfg.writeFile( pss.str().c_str() );
		}
	}
	catch( libconfig::SettingException &e)
	{
		cout << "Setting error: " << endl;
		cout << e.what() << endl;
		cout << e.getPath() << endl;
		return;
	}
	catch( libconfig::ParseException &e )
	{
		cout << "Parse error:" << endl;
		cout << e.what() << endl;
		cout << e.getError() << endl;
		cout << e.getFile() << endl;
		cout << e.getLine() << endl;
		return;
	}
	
	for( unsigned cc = 0; cc < trial.cameras.size(); ++cc )
	{
		ScameraInfo &ci = trial.cameras[cc];
		
		std::stringstream cfss;
		cfss << ci.rawPath << "/calibFile";
		if( boost::filesystem::exists( cfss.str() ) )
		{
			std::stringstream cfss2;
			cfss2 << ci.rgbPath << ".calib";
			boost::filesystem::copy_file( cfss.str(), cfss2.str(), boost::filesystem::copy_option::overwrite_if_exists);
		}
		
		
		std::stringstream gfss;
		gfss << ci.rawPath << "/grids";
		if( boost::filesystem::exists( gfss.str() ) )
		{
			std::stringstream gfss2;
			gfss2 << ci.rgbPath << ".grids";
			boost::filesystem::copy_file( gfss.str(), gfss2.str(), boost::filesystem::copy_option::overwrite_if_exists);
		}
	}
	
	TrialBoxRowActivatedImpl();
}
