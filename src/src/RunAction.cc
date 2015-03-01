#include "RunAction.hh"
#include "PrimaryGeneratorAction.hh"
#include "Analysis.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

RunAction::RunAction() : G4UserRunAction() {}
RunAction::~RunAction() { delete G4AnalysisManager::Instance(); }

void RunAction::BeginOfRunAction(const G4Run* /*run*/) {}

void RunAction::EndOfRunAction(const G4Run* run) {
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
    
  // Primary energy (run) number and total events
  G4int E_Num = run->GetRunID();
  G4int numEvents = run->GetNumberOfEventToBeProcessed();
  // Adjust Energy number by state var
  G4int Al_num = 0;   if ( Al_side == 10 ) Al_num = 1; if ( Al_side == 25 ) Al_num = 2;
  E_Num = E_Num - Al_num*45 + 1;

  // Invoke stat vars  
  G4int zBin, numBins = 1150;
  G4double numProtons = 0, numNeutrons = 0, numGammas = 0;
  G4double matrixProtonsNeutrons[7][7] = {0};
  G4double energy_bins[1150] = {0};
  G4double charge_bins[1150] = {0};
  G4double particleEDep, particleCharge;
  G4String particleLocation, particleName;
  
  for ( G4int eventNum = 0; eventNum<numEvents; eventNum++ ) {
    // 1 Energy Deposition
    fileNameStream.str(""); fileName = "";
    fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/" << data_dir_analysis << "1/event" << eventNum << ".txt";
    fileName = fileNameStream.str();
    std::ifstream eventFile1;
    // Wait for run to finish to begin analysis
    while ( ! eventFile1.good() ) { sleep(1); }
    eventFile1.open(fileName);
    // Continuously append to energy bins
    while ( getline(eventFile1, fileVarGet, ' ') ) {
      zBin = atoi(fileVarGet);
      getline(eventFile1, fileVarGet);
      particleEDep = atof(fileVarGet);
      energy_bins[zBin] += particleEDep;
    }
    
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
      if ( particleLocation == "init" ) charge_bins[zBin] -= particleCharge/numEvents;
      if ( particleLocation == "final" ) charge_bins[zBin] += particleCharge/numEvents;
    }

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
	if ( particleName == "gamma" ) { numGammas = numGammas+1; }
      }
    }
    // Construct normalized production probability matrix
    if ( numProtons > 6 ) numProtons = 6;
    if ( numNeutrons > 6 ) numNeutrons = 6;
    matrixProtonsNeutrons[int(numProtons)][int(numNeutrons)] += 1;
  }
  
  // Summarize normalized results for energy and charge bins
  fileNameStream.str(""); fileName = "";
  fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/bins.dat";
  fileName = fileNameStream.str();
  fileStream.open (fileName, std::ios::app);
  fileStream << "#  z (mm)   Energy_Dep  Net_Charge\n";
  for ( G4int binNum = 0; binNum<numBins; binNum++ ) {
    fileStream << binNum << " " << energy_bins[binNum] << " " << charge_bins[binNum] << "\n";
  }
  fileStream.close();
  // Summarize normalized results for production probabilities
  fileNameStream.str(""); fileName = "";
  fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/production.dat";
  fileName = fileNameStream.str();
  fileStream.open (fileName, std::ios::app);
  fileStream << "# Gamma Count and Particle Production Probabilities \n";
  fileStream << "# " << numGammas/numEvents << " gammas per event \n";
  fileStream << "# production probability matrix per event \n";
  for ( G4int protonInt = 0; protonInt<7; protonInt++ ) {
	for ( G4int neutronInt = 0; neutronInt<7; neutronInt++ ) {
      fileStream << (long double)matrixProtonsNeutrons[protonInt][neutronInt]/numEvents << " ";
    }
    fileStream << "\n";
  }
}
