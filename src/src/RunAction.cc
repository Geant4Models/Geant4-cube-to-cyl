#include "RunAction.hh"
#include "PrimaryGeneratorAction.hh"
#include "Analysis.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <algorithm>

RunAction::RunAction() : G4UserRunAction() {}
RunAction::~RunAction() { delete G4AnalysisManager::Instance(); }

void RunAction::BeginOfRunAction(const G4Run* run) {
  // Primary thread
  if ( G4Threading::G4GetThreadId() == 0 ) {
    // Directory info
    G4String fileVarGet;
    G4String data_dir = "data/";
    G4String data_dir_particle = "p";

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
    // Adjust Energy number by state var, beam particle by energy number
    G4int Al_num = 0;   if ( Al_side == 10 ) Al_num = 1; if ( Al_side == 25 ) Al_num = 2;
    E_Num = E_Num - Al_num*90 + 1;
    if ( E_Num > 45 ) { E_Num = E_Num - 45; data_dir_particle = "n"; }

    G4cout << "Running " << data_dir_particle << " energy #" << E_Num << " for Al=" << Al_side << G4endl;
  }
}

void RunAction::EndOfRunAction(const G4Run* run) {
  // Energy scoring
  //
  // Acquire statistics normalized per event for each energy
  //
  // 1. Total energy deposited per bin
  // 2. Total charge transfer between bins
  // 3a. gamma created per primary
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
  G4String data_dir_particle = "p";
  G4String data_dir_energy = "E";
  G4String data_dir_analysis = "Analysis";
  
  // Acqiure state and thread vars
  G4int Al_side, nThreads;
  fileNameStream.str(""); fileName = "";
  fileNameStream << data_dir << ".state";
  fileName = fileNameStream.str();
  std::ifstream stateFile;
  stateFile.open(fileName);
  stateFile >> fileVarGet;
  Al_side = atoi(fileVarGet);
  stateFile.close();
  fileNameStream.str(""); fileVarGet = ""; fileName = "";
  fileNameStream << data_dir << ".threads";
  fileName = fileNameStream.str();
  std::ifstream threadFile;
  threadFile.open(fileName);
  threadFile >> fileVarGet;
  nThreads = atoi(fileVarGet);
  threadFile.close();
    
  // Primary energy (run) number and total events
  G4int E_Num = run->GetRunID();
  G4int numEvents = run->GetNumberOfEventToBeProcessed();
  // Adjust Energy number by state var, beam particle by energy number
  G4int Al_num = 0;   if ( Al_side == 10 ) Al_num = 1; if ( Al_side == 25 ) Al_num = 2;
  E_Num = E_Num - Al_num*90 + 1;
  if ( E_Num > 45 ) { E_Num = E_Num - 45; data_dir_particle = "n"; }

  // Invoke stat vars  
  G4int zBin, numBins = 1150;
  G4double numProtons = 0, numNeutrons = 0, numGammas = 0;
  G4double matrixProtonsNeutrons[7][7] = {{0}};
  G4double energy_bins[1150] = {0};
  G4double charge_bins[1150] = {0};
  G4double particleEDep, particleCharge;
  G4String particleLocation, particleName;
  
  // Bin and tally files
  energyFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/energy.txt";
  energyFileName = energyFileNameStream.str();
  std::ifstream energyFile;
  energyFile.open(energyFileName);
  chargeFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/charge.txt";
  chargeFileName = chargeFileNameStream.str();
  std::ifstream chargeFile;
  chargeFile.open(chargeFileName);
  tallyFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/tally.txt";
  tallyFileName = tallyFileNameStream.str();
  std::ifstream tallyFile;
  tallyFile.open(tallyFileName);

  // Final thread: any thread can finish first, so check run state file for nThreads of '#'
  // Add '#' per run
  G4String threadString, numThreadsString = "";
  fileNameStream.str(""); fileName = "";
  fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/.threads";
  fileName = fileNameStream.str();
  std::ofstream threadNumFile;
  threadNumFile.open(fileName, std::ios::app);
  threadNumFile << "#";
  threadNumFile.close();
  // Reopen, get total length
  std::ifstream threadsFile;
  threadsFile.open(fileName);
  threadsFile >> threadString;
  threadsFile.close();

  // Acquire nThreads string of '#'s
  std::ostringstream numThreadsStream;
  for (G4int numThreads_i=1; numThreads_i <= nThreads; numThreads_i++) { numThreadsStream << "#"; }
  G4String totalThreadsString = numThreadsStream.str();

  // If fourth run (not ID==4, but the fourth being completed)
  if ( threadString == totalThreadsString ) {
  
    // Energies for label
    G4int E_Int = (E_Num % 5); if ( E_Int == 0 ) { E_Int = 5; }
    std::ostringstream energy_stringStream;
    energy_stringStream << E_Int;
    if ( (( E_Num % 15 ) > 5) || (( E_Num % 15) == 0) ) { energy_stringStream << "0"; }
    if ( (( E_Num % 15 ) > 10) || (( E_Num % 15) == 0) ) { energy_stringStream << "0"; }
    G4String energy_valString = energy_stringStream.str();
    G4int energy_val = atoi(energy_valString);
    if ( ( 15 < E_Num ) && ( E_Num < 31 ) ) { energy_stringStream << " k"; energy_val = energy_val*1000; }
    if ( ( E_Num > 30 ) && ( E_Num < 46) ) { energy_stringStream << " M"; energy_val = energy_val*1000000; }
    energy_stringStream << "eV";
    if ( data_dir_particle == "p" ) { energy_stringStream << " Protons"; }
    if ( data_dir_particle == "n" ) { energy_stringStream << " Neutrons"; }
    G4String energy_string = energy_stringStream.str();
    
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
    fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/bins.dat";
    fileName = fileNameStream.str();
    fileStream.open (fileName, std::ios::app);
    fileStream << "# Energy z_(mm) Energy_Dep Net_Charge\n";
    for ( G4int binNum = 0; binNum<numBins; binNum++ ) {
      fileStream << energy_val << " " << binNum << " " << energy_bins[binNum]/numEvents << " " << charge_bins[binNum]/numEvents << "\n";
    }
    fileStream.close();
    // Summarize normalized results for production probabilities
    fileNameStream.str(""); fileName = "";
    fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/production.dat";
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
    gnuFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/graphs.gplot";
    gnuFileName = gnuFileNameStream.str();

    // Add content
    gnuFileStream.open (gnuFileName);
    gnuFileStream << "set term png\n" << \
                     "set output \"fig_EDep.png\"\n" << \
                     "set key samplen 2 spacing 0.9 font \",8\" below\n" << \
                     "set title \"Net Energy Deposition per " << energy_string << "\n" << \
                     "set xlabel \"Position (mm)\"\n" << \
                     "set ylabel \"Energy (eV/" << data_dir_particle << ")\"\n" << \
                     "plot \"bins.dat\" u 2:3 t \"Energy_Dep\"\n" << \
                     "set output \"fig_ChargeDep.png\"\n" << \
                     "set key samplen 2 spacing 0.9 font \",8\" below\n" << \
                     "set title \"Net Charge per " << energy_string << "\n" << \
                     "set xlabel \"Position (mm)\"\n" << \
                     "set ylabel \"Charge (e/" << data_dir_particle << ")\"\n" << \
                     "set yrange [-1:1]\n" << \
                     "plot \"bins.dat\" u 2:4 t \"Net_Charge\"\n";
    gnuFileStream.close();

    // Run gnuplot script to create graphs as PNGs
    G4cout << "Populating gnuplot graph..." << G4endl;
    std::ostringstream runGnuStream;
    runGnuStream << "cd " << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "; gnuplot graphs.gplot; cd ../../..";
    G4String runGnu = runGnuStream.str();
    system(runGnu);
  }
}
