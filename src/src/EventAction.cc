#include "EventAction.hh"
#include "RunAction.hh"
#include "Analysis.hh"
#include "Randomize.hh"

#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4UnitsTable.hh"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <sys/types.h>

// Useful module inserts
EventAction::EventAction() : G4UserEventAction() {}
EventAction::~EventAction() {}

void EventAction::BeginOfEventAction(const G4Event* /*event*/) {}

void EventAction::EndOfEventAction(const G4Event* event) {
  // Energy scoring
  //
  // Acquire statistics normalized per event for each energy
  //
  // 1. Total energy deposited per bin
  // 2. Total charge transfer between bins
  // 3a. e- backscatter per primary
  // 3b. p+ created per primary
  // 3c. n created per primary
  
  // I/O vars
  std::ostringstream fileNameStream;
  std::ofstream fileStream;
  G4String fileName;
  std::ostringstream energyFileNameStream;
  G4String energyFileName;
  std::ostringstream chargeFileNameStream;
  G4String chargeFileName;
  std::ostringstream tallyFileNameStream;
  G4String tallyFileName;
  G4String runRm;
  
  // Directory info
  G4String fileVarGet;
  G4String data_dir = "data/";
  G4String data_dir_geo = "Al";
  G4String data_dir_energy = "E";
  G4String data_dir_analysis = "Analysis";
  
  // Acqiure state vars
  G4int Al_side;
  fileNameStream.str(""); fileName = "";
  fileNameStream << data_dir << ".state";
  fileName = fileNameStream.str();
  std::ifstream stateFile;
  stateFile.open(fileName);
  stateFile >> fileVarGet;
  Al_side = atoi(fileVarGet);
    
  // Primary energy (run) number and event number
  G4int E_Num = G4RunManager::GetRunManager()->GetCurrentRun()->GetRunID();
  G4int eventNum = event->GetEventID();
  // Adjust Energy number by state var
  G4int Al_num = 0;   if ( Al_side == 10 ) Al_num = 1; if ( Al_side == 25 ) Al_num = 2;
  E_Num = E_Num - Al_num*45 + 1;

  // Invoke stat vars  
  G4int zBin;
  G4double numProtons = 0, numNeutrons = 0, numGammas = 0;
  G4double particleEDep, particleCharge;
  G4String particleLocation, particleName;
  
  // Bin and tally files
  energyFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/energy.txt";
  energyFileName = energyFileNameStream.str();
  std::ofstream energyFile;
  energyFile.open(energyFileName, std::ios::app);
  chargeFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/charge.txt";
  chargeFileName = chargeFileNameStream.str();
  std::ofstream chargeFile;
  chargeFile.open(chargeFileName, std::ios::app);
  tallyFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/tally.txt";
  tallyFileName = tallyFileNameStream.str();
  std::ofstream tallyFile;
  tallyFile.open(tallyFileName, std::ios::app);
  
  // 1 Energy Deposition
  fileNameStream.str(""); fileName = "";
  fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/" << data_dir_analysis << "1/event" << eventNum << ".txt";
  fileName = fileNameStream.str();
  std::ifstream eventFile1;
  eventFile1.open(fileName);
  // Append event to energy bins file
  while ( getline(eventFile1, fileVarGet, ' ') ) {
    zBin = atoi(fileVarGet);
    getline(eventFile1, fileVarGet);
    particleEDep = atof(fileVarGet);
    energyFile << zBin << " " << particleEDep << "\n";
  }
  // Remove event file
  runRm = "rm " + fileName;
  system(runRm);
  
  // 2 Charge Transfer
  fileNameStream.str(""); fileName = "";
  fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/" << data_dir_analysis << "2/event" << eventNum << ".txt";
  fileName = fileNameStream.str();
  std::ifstream eventFile2; eventFile2.open(fileName);
  // Continuously append to charge bins
  while ( getline(eventFile2, fileVarGet, ' ') ) {
    zBin = atoi(fileVarGet);
    getline(eventFile2, particleLocation, ' ');
    getline(eventFile2, fileVarGet);
    particleCharge = atof(fileVarGet);
    if ( particleLocation == "init" ) chargeFile << zBin << " " << -particleCharge << "\n";
    if ( particleLocation == "final" ) chargeFile << zBin << " " << particleCharge << "\n";
  }
  // Remove event file
  runRm = "rm " + fileName;
  system(runRm);
  
  // 3 Particle Tallies
  fileNameStream.str(""); fileName = "";
  fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/" << data_dir_analysis << "3/event" << eventNum << ".txt";
  fileName = fileNameStream.str();
  std::ifstream eventFile3;
  // Check if particles were produced in event
  if ( eventFile3.good() ) {
    eventFile3.open(fileName);
    // Continuously append to particle bins
    while ( getline(eventFile3, particleName) ) {
      if ( particleName == "proton" ) { numProtons++; }
      if ( particleName == "neutron" ) { numNeutrons++; }
      if ( particleName == "gamma" ) { numGammas++; }
    }
    tallyFile << numProtons << " " << numNeutrons << " " << numGammas << "\n";
    
    // Remove event file
    runRm = "rm " + fileName;
    system(runRm);
  }
}
