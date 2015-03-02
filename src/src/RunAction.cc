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

void RunAction::BeginOfRunAction(const G4Run* run) {
  // Directory info
  G4String fileVarGet;
  G4String data_dir = "data/";

  // I/O vars
  std::ostringstream fileNameStream;
  std::ofstream fileStream;
  G4String fileName;

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
  // Adjust Energy number by state var
  G4int Al_num = 0;   if ( Al_side == 10 ) Al_num = 1; if ( Al_side == 25 ) Al_num = 2;
  E_Num = E_Num - Al_num*45 + 1;

  G4cout << "Running energy " << E_Num << " for Al=" << Al_side << G4endl;
}

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
  std::ostringstream energyFileNameStream;
  G4String energyFileName;
  std::ostringstream chargeFileNameStream;
  G4String chargeFileName;
  std::ostringstream tallyFileNameStream;
  G4String tallyFileName;
  
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
  
  // Bin and tally files
  energyFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/energy.txt";
  energyFileName = energyFileNameStream.str();
  std::ifstream energyFile;
  energyFile.open(energyFileName);
  chargeFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/charge.txt";
  chargeFileName = chargeFileNameStream.str();
  std::ifstream chargeFile;
  chargeFile.open(chargeFileName);
  tallyFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/tally.txt";
  tallyFileName = tallyFileNameStream.str();
  std::ifstream tallyFile;
  tallyFile.open(tallyFileName);
  
  // 1 Energy Deposition
  // Continuously append to energy bins
  while ( getline(energyFile, fileVarGet, ' ') ) {
    zBin = atoi(fileVarGet);
    getline(energyFile, fileVarGet);
    particleEDep = atof(fileVarGet);
    energy_bins[zBin] += particleEDep;
  }
    
  // 2 Charge Transfer
  // Continuously append to charge bins
  while ( getline(chargeFile, fileVarGet, ' ') ) {
    zBin = atoi(fileVarGet);
    getline(chargeFile, fileVarGet);
    particleCharge = atof(fileVarGet);
    charge_bins[zBin] += particleCharge;
  }

  // 3 Particle Tallies
  // Continuously append to particle bins
  while ( getline(tallyFile, fileVarGet, ' ') ) {
    numProtons = atof(fileVarGet);
    getline(tallyFile, fileVarGet, ' '); numNeutrons = atof(fileVarGet);
    getline(tallyFile, fileVarGet); numGammas += atof(fileVarGet);
    
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
    fileStream << binNum << " " << energy_bins[binNum]/numEvents << " " << charge_bins[binNum]/numEvents << "\n";
  }
  fileStream.close();
  // Summarize normalized results for production probabilities
  fileNameStream.str(""); fileName = "";
  fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/production.dat";
  fileName = fileNameStream.str();

  // Add content
  fileStream.open (fileName, std::ios::app);
  fileStream << "# Gamma Count and Particle Production Probabilities \n"  << \
                "# " << numGammas/numEvents << " gammas per event \n" << \
                "# [ p \\ n ] production probability matrix per event \n";
  for ( G4int protonInt = 0; protonInt<7; protonInt++ ) {
    for ( G4int neutronInt = 0; neutronInt<7; neutronInt++ ) {
      fileStream << (long double)matrixProtonsNeutrons[protonInt][neutronInt]/numEvents << " ";
    }
    fileStream << "\n";
  }

  // Construct gnuplot file for run
  std::ostringstream gnuFileNameStream;
  std::ofstream gnuFileStream;

  G4String gnuFileName;
  gnuFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/graphs.gplot";
  gnuFileName = gnuFileNameStream.str();

  // Add content
  gnuFileStream.open (gnuFileName, std::ios::app);
  gnuFileStream << "set term png\n" << \
                   "set output \"fig_EDep.png\"\n" << \
                   "set key samplen 2 spacing 0.9 font \",8\" below\n" << \
                   "set title \"Energy Deposition by Millimeter\"\n" << \
                   "set xlabel \"Position (mm)\"\n" << \
                   "set ylabel \"Energy Deposition\"\n" << \
                   "plot \"bins.dat\" u 1:2 t \"Energy_Dep\"\n" << \
                   "set output \"fig_ChargeDep.png\"\n" << \
                   "set key samplen 2 spacing 0.9 font \",8\" below\n" << \
                   "set title \"Charge Transfer by Millimeter\"\n" << \
                   "set xlabel \"Position (mm)\"\n" << \
                   "set ylabel \"Net Charge\"\n" << \
                   "plot \"bins.dat\" u 1:3 t \"Net_Charge\"";
  gnuFileStream.close();
}
