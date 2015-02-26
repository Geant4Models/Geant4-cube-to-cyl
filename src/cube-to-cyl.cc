#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

#ifdef G4MULTITHREADED
#include "G4MTRunManager.hh"
#else
#include "G4RunManager.hh"
#endif

#include "G4UImanager.hh"
#include "G4UIcommand.hh"

#include "FTFP_BERT.hh"

#include "Randomize.hh"

#ifdef G4VIS_USE
#include "G4VisExecutive.hh"
#endif

#ifdef G4UI_USE
#include "G4UIExecutive.hh"
#endif

namespace {
  void PrintUsage() {
    G4cerr << " Usage: " << G4endl;
    G4cerr << " ./cube-to-cyl [-m macro ] [-u UIsession] [-t nThreads]" << G4endl;
    G4cerr << "   note: -t option is available only for multi-threaded mode."
           << G4endl;
  }
}

int main(int argc,char** argv) {
  // Evaluate arguments
  if ( argc > 7 ) {
    PrintUsage();
    return 1;
  }  
  G4String macro;
  G4String session;
#ifdef G4MULTITHREADED
  G4int nThreads = 0;
#endif
  for ( G4int i=1; i<argc; i=i+2 ) {
    if      ( G4String(argv[i]) == "-m" ) macro = argv[i+1];
    else if ( G4String(argv[i]) == "-u" ) session = argv[i+1];
#ifdef G4MULTITHREADED
    else if ( G4String(argv[i]) == "-t" ) {
      nThreads = G4UIcommand::ConvertToInt(argv[i+1]);
    }
#endif
    else {
      PrintUsage();
      return 1;
    }
  }  
  
  // Choose the Random engine
  G4Random::setTheEngine(new CLHEP::RanecuEngine);
  
  // Construct the default run manager
#ifdef G4MULTITHREADED
  G4MTRunManager * runManager = new G4MTRunManager;
  if ( nThreads > 0 ) { 
    runManager->SetNumberOfThreads(nThreads);
  }  
#else
  G4RunManager * runManager = new G4RunManager;
#endif

  // Set mandatory initialization classes
  DetectorConstruction* detConstruction = new DetectorConstruction();
  runManager->SetUserInitialization(detConstruction);

  G4VModularPhysicsList* physicsList = new FTFP_BERT;
  runManager->SetUserInitialization(physicsList);
    
  ActionInitialization* actionInitialization
     = new ActionInitialization(detConstruction);
  runManager->SetUserInitialization(actionInitialization);

  // Initialize G4 kernel
  runManager->Initialize();
  
#ifdef G4VIS_USE
  // Initialize visualization
  G4VisManager* visManager = new G4VisExecutive;
  // G4VisExecutive can take a verbosity argument - see /vis/verbose guidance.
  // G4VisManager* visManager = new G4VisExecutive("Quiet");
  visManager->Initialize();
#endif

  // Get the pointer to the User Interface manager
  G4UImanager* UImanager = G4UImanager::GetUIpointer();

  // batch mode  
  if ( macro.size() ) {
	// 10 micrometer particle track cuts
    G4String cutCommand = "/run/setCut 0.01 mm";
    UImanager->ApplyCommand(cutCommand);
    
    // Experimental Parameters
	G4int Al_side[3] = {5, 10, 25};
	
	// Create data analysis directory tree
	G4cout << "Creating data analysis tree..." << G4endl;
	std::ostringstream dirCommandStream;
	G4String data_dir = "data/";
	G4String data_dir_geo = "Al";
	G4String data_dir_energy = "E";
	G4String data_dir_analysis = "Analysis";
	// Make base dir
	dirCommandStream << "mkdir -p " << data_dir;
	G4String dirCommand = dirCommandStream.str();
    system(dirCommand);
    for (G4int al_num=0; al_num<3; al_num++) {
	  // Geometry dir iteration
	  dirCommandStream.str(""); dirCommand = "";
      dirCommandStream << "mkdir -p " << data_dir << data_dir_geo << Al_side[al_num] << "/";
	  dirCommand = dirCommandStream.str();
      system(dirCommand);
      
      for (G4int E_num=1; E_num<=45; E_num++) {
        // Energy dir iteration
        dirCommandStream.str(""); dirCommand = "";
        dirCommandStream << "mkdir -p " << data_dir << data_dir_geo << Al_side[al_num] << "/" << data_dir_energy << E_num << "/";
	    dirCommand = dirCommandStream.str();
        system(dirCommand);
        
        for (G4int analysis_num=1; analysis_num<5; analysis_num++) {
          // Analysis dir iteration
          dirCommandStream.str(""); dirCommand = "";
          dirCommandStream << "mkdir -p " << data_dir << data_dir_geo << Al_side[al_num] << "/" << data_dir_energy << E_num << "/" << data_dir_analysis << analysis_num << "/";
	      dirCommand = dirCommandStream.str();
          system(dirCommand);
        }
      }
	}
    
    for ( G4int side_i=0; side_i<3; side_i++ ) {
	  // Assign thickness
	  detConstruction->AlSideIteration(side_i);
	  runManager->GeometryHasBeenModified();
	
	  // Create state flag
	  dirCommandStream.str(""); dirCommand = "";
      dirCommandStream << "echo " << Al_side[side_i] << " > " << data_dir << ".state";
      dirCommand = dirCommandStream.str();
      system(dirCommand);
	  
	  // Run experimental beam energies
	  G4String command = "/control/execute ";
      UImanager->ApplyCommand(command+macro);
    }
    
    // Remove state flag
    G4String runRm = "rm " + data_dir + ".state";
    system(runRm);
  }
  else {
    // interactive mode : define UI session
#ifdef G4UI_USE
    G4UIExecutive* ui = new G4UIExecutive(argc, argv, session);
#ifdef G4VIS_USE
    UImanager->ApplyCommand("/control/execute init_vis.mac"); 
#else
    UImanager->ApplyCommand("/control/execute init.mac"); 
#endif
    if (ui->IsGUI())
      UImanager->ApplyCommand("/control/execute gui.mac");
    ui->SessionStart();
    delete ui;
#endif
  }

#ifdef G4VIS_USE
  delete visManager;
#endif
  delete runManager;

  return 0;
}
