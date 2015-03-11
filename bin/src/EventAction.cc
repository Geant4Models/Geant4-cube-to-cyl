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
  std::ostringstream geoFileNameStream;
  std::ifstream geoFileStream;
  G4String geoFileName;

  std::ostringstream eventFileNameStream;
  std::ifstream eventFileStream;
  G4String eventFileName;

  std::ostringstream binsFileNameStream;
  std::ofstream binsOutFileStream;
  std::ifstream binsInFileStream;
  G4String binsFileName;

  std::ostringstream productionFileNameStream;
  std::ofstream productionOutFileStream;
  std::ifstream productionInFileStream;
  G4String productionFileName;

  G4String fileVarGet;
  G4String runRm;
  
  // Directory info vars
  G4String data_dir = "data/";
  G4String data_dir_geo = "Al";
  G4String data_dir_particle = "p";
  G4String data_dir_energy = "E";
  G4String data_dir_analysis = "Analysis";
  G4String data_dir_events = "Events";
  
  // Acqiure geometry state vars
  G4int Al_side;
  geoFileNameStream << data_dir << ".state";
  geoFileName = geoFileNameStream.str();
  geoFileStream.open(geoFileName);
  geoFileStream >> fileVarGet;
  Al_side = atoi(fileVarGet);
    
  // Primary energy (run) number, event number, and total events
  G4int E_Num = G4RunManager::GetRunManager()->GetCurrentRun()->GetRunID();
  G4int eventNum = event->GetEventID();
  // Adjust Energy number by state var, beam particle by energy number
  G4int Al_num = 0;   if ( Al_side == 10 ) Al_num = 1; if ( Al_side == 25 ) Al_num = 2;
  E_Num = E_Num - Al_num*90 + 1;
  if ( E_Num > 45 ) { E_Num = E_Num - 45; data_dir_particle = "n"; }

  // Energies for label [future work: generalize arbitrary energy list]
  G4int E_Int = (E_Num % 5); if ( E_Int == 0 ) { E_Int = 5; }
  std::ostringstream energy_stringStream;
  energy_stringStream << E_Int;
  if ( (( E_Num % 15 ) > 5) || (( E_Num % 15) == 0) ) { energy_stringStream << "0"; }
  if ( (( E_Num % 15 ) > 10) || (( E_Num % 15) == 0) ) { energy_stringStream << "0"; }
  G4String energy_valString = energy_stringStream.str();
  G4int energy_val = atoi(energy_valString);

  // Analysis vars  
  G4int zBin, numBins = 1150;
  G4double numProtons = 0, numNeutrons = 0, numGammas = 0;
  G4double matrixProtonsNeutrons[7][7] = {{0}};
  G4double energy_bins[1150] = {0};
  G4double charge_bins[1150] = {0};
  G4double particleEDep, particleCharge;
  G4String particleLocation, particleName;
  
  // Bins and production output
  binsFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/" << data_dir_events << "/bins" << eventNum << ".dat";
  binsFileName = binsFileNameStream.str();
  productionFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/" << data_dir_events << "/production" << eventNum << ".dat";
  productionFileName = productionFileNameStream.str();

  // 1+2 Energy and Charge Deposition
  // Get event energy/charge deposition and write to total events file
  eventFileNameStream.str(""); eventFileName = "";
  eventFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/" << data_dir_analysis << "1/event" << eventNum << ".txt";
  eventFileName = eventFileNameStream.str();
  eventFileStream.open(eventFileName);
  while ( getline(eventFileStream, fileVarGet, ' ') ) {
    zBin = atoi(fileVarGet);
    getline(eventFileStream, fileVarGet);
    particleEDep = atof(fileVarGet);
    energy_bins[zBin] += particleEDep;
  }
  eventFileStream.close();
  // Remove event file
  runRm = "rm " + eventFileName;
  system(runRm);

  // Charge Deposition File
  eventFileNameStream.str(""); eventFileName = "";
  eventFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/" << data_dir_analysis << "2/event" << eventNum << ".txt";
  eventFileName = eventFileNameStream.str();
  eventFileStream.open(eventFileName);
  // Check if charge transfer occured (may be necessary for neutron events)
  if ( eventFileStream.good() ) {
    while ( getline(eventFileStream, fileVarGet, ' ') ) {
      zBin = atoi(fileVarGet);
      getline(eventFileStream, particleLocation, ' ');
      getline(eventFileStream, fileVarGet);
      particleCharge = atof(fileVarGet);
      if ( particleLocation == "init" ) { charge_bins[zBin] -= particleCharge; }
      if ( particleLocation == "final" ) { charge_bins[zBin] += particleCharge; }
    }
    eventFileStream.close();

    // Remove event file
    runRm = "rm " + eventFileName;
    system(runRm);
  }

  // 3 Particle Tallies
  // Acquire current normalized analytical variables
  productionInFileStream.open(productionFileName);
  if ( productionInFileStream.good() ) {
    // Acquire Gamma count, ignore header (1st, rest of 2nd, 3rd)
    getline(productionInFileStream, fileVarGet);
    getline(productionInFileStream, fileVarGet, ' ');
    getline(productionInFileStream, fileVarGet, ' ');
    numGammas = atof(fileVarGet);
    getline(productionInFileStream, fileVarGet);
    getline(productionInFileStream, fileVarGet);

    // Iteratively reconstruct matrixProtonsNeutrons
    for ( G4int protonInt = 0; protonInt<6; protonInt++ ) {
      for ( G4int neutronInt = 0; neutronInt<6; neutronInt++ ) {
        getline(productionInFileStream, fileVarGet, ' ');
        matrixProtonsNeutrons[protonInt][neutronInt] = atof(fileVarGet);
      }
      // final value in line uses no delimeter
      getline(productionInFileStream, fileVarGet);
      matrixProtonsNeutrons[protonInt][6] = atof(fileVarGet);
    }
    productionInFileStream.close();
  }

  // Get normalized event production tallies continuously append to productionFileName
  eventFileNameStream.str(""); eventFileName = "";
  eventFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/" << data_dir_analysis << "3/event" << eventNum << ".txt";
  eventFileName = eventFileNameStream.str();
  eventFileStream.open(eventFileName);
  // Check if particles were produced in event
  if ( eventFileStream.good() ) {
    // Continuously append to particle bins
    while ( getline(eventFileStream, particleName) ) {
      if ( particleName == "proton" ) { numProtons++; }
      if ( particleName == "neutron" ) { numNeutrons++; }
      if ( particleName == "gamma" ) { numGammas++;; }

      // Construct normalized production probability matrix
      if ( numProtons > 6 ) numProtons = 6;
      if ( numNeutrons > 6 ) numNeutrons = 6;
      matrixProtonsNeutrons[int(numProtons)][int(numNeutrons)] += 1;
    }
    eventFileStream.close();

    /*// Remove event file
    runRm = "rm " + eventFileName;
    system(runRm);*/
  } else { 
    // Add zeroed count to matrix
    matrixProtonsNeutrons[0][0] += 1;
  }

  // Add content to bins file
  binsOutFileStream.open (binsFileName);
  binsOutFileStream << "# Energy z_(mm) Energy_Dep Net_Charge\n";
  for ( G4int binNum = 0; binNum<numBins; binNum++ ) {
    binsOutFileStream << energy_val << " " << binNum << " " << energy_bins[binNum] << " " << charge_bins[binNum] << "\n";
  }
  binsOutFileStream.close();

  // Add content to production file
  productionOutFileStream.open (productionFileName);
  productionOutFileStream << "# Gamma Count and Particle Production Probabilities \n"  << \
                "# " << numGammas << " gammas per event \n" << \
                "# [ p \\ n ] production probability matrix per event \n";
  for ( G4int protonInt = 0; protonInt<7; protonInt++ ) {
    for ( G4int neutronInt = 0; neutronInt<7; neutronInt++ ) {
      productionOutFileStream << (long double)matrixProtonsNeutrons[protonInt][neutronInt] << " ";
    }
    productionOutFileStream << "\n";
  }
  productionOutFileStream.close();
}
