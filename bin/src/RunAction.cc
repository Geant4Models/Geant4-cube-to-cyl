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
RunAction::~RunAction() {}

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

  // ROOT Analysis Data
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();

  // Hit info
  analysisManager->CreateNtuple("hitsData", "Sensitive Detector Hits");
  analysisManager->CreateNtupleIColumn("run");
  analysisManager->CreateNtupleIColumn("event");
  analysisManager->CreateNtupleIColumn("hit_Id");
  analysisManager->CreateNtupleDColumn("hit_x");
  analysisManager->CreateNtupleDColumn("hit_y");
  analysisManager->CreateNtupleDColumn("hit_px");
  analysisManager->CreateNtupleDColumn("hit_py");
  analysisManager->CreateNtupleDColumn("hit_pz");
  analysisManager->FinishNtuple();

  // Create ROOT input file
  analysisManager->OpenFile("hitsData");
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

  // End data collection
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
  analysisManager->Write();
  analysisManager->CloseFile();
  delete G4AnalysisManager::Instance();

  // I/O vars
  std::ostringstream geoFileNameStream;
  std::ifstream geoFileStream;
  G4String geoFileName;

  std::ostringstream threadFileNameStream;
  std::ifstream threadFileStream;
  std::ofstream threadOutFileStream;
  G4String threadFileName;

  std::ostringstream binsFileNameStream;
  std::ifstream binsInFileStream;
  std::ofstream binsOutFileStream;
  G4String binsFileName;

  std::ostringstream productionFileNameStream;
  std::ifstream productionInFileStream;
  std::ofstream productionOutFileStream;
  G4String productionFileName;

  G4String fileVarGet;
  std::ostringstream runRmStream;
  G4String runRm;
  
  // Directory info
  G4String data_dir = "data/";
  G4String data_dir_geo = "Al";
  G4String data_dir_particle = "p";
  G4String data_dir_energy = "E";
  G4String data_dir_analysis = "Analysis";
  G4String data_dir_events = "Events";
  
  // Acqiure geometry and thread vars
  G4int Al_side, nThreads;
  geoFileNameStream << data_dir << ".state";
  geoFileName = geoFileNameStream.str();
  geoFileStream.open(geoFileName);
  geoFileStream >> fileVarGet;
  Al_side = atoi(fileVarGet);
  geoFileStream.close();
  threadFileNameStream << data_dir << ".threads";
  threadFileName = threadFileNameStream.str();
  threadFileStream.open(threadFileName);
  threadFileStream >> fileVarGet;
  nThreads = atoi(fileVarGet);
  threadFileStream.close();

  // Primary energy (run) number and total events
  G4int runID = run->GetRunID();
  G4int numEvents = run->GetNumberOfEventToBeProcessed();
  // Adjust Energy number by state var, beam particle by energy number
  G4int Al_num = 0;   if ( Al_side == 10 ) Al_num = 1; if ( Al_side == 25 ) Al_num = 2;
  G4int E_Num = runID - Al_num*90 + 1;
  if ( E_Num > 45 ) { E_Num = E_Num - 45; data_dir_particle = "n"; }

  // Final thread: any thread can finish first, so check run state file for nThreads of '#'
  // Add '#' per thread
  G4String threadString, numThreadsString = "";
  threadFileNameStream.str(""); threadFileName = "";
  threadFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/.threads";
  threadFileName = threadFileNameStream.str();
  threadOutFileStream.open(threadFileName, std::ios::app);
  threadOutFileStream << "#";
  threadOutFileStream.close();
  // Reopen, get total length
  threadFileNameStream.str("");
  threadFileStream.open(threadFileName);
  threadFileStream >> threadString;
  threadFileStream.close();

  // Acquire nThreads string of '#'s
  std::ostringstream numThreadsStream;
  for (G4int numThreads_i=0; numThreads_i < nThreads; numThreads_i++) { numThreadsStream << "#"; }
  G4String totalThreadsString = numThreadsStream.str();

  // If final worker (not workerID==#*nThreads, but the final being completed [future work: thread merge class via G4MTRunManager])
  if ( threadString == totalThreadsString ) {

    // Move ROOT analysis to geo/energy file
    std::ostringstream syscmdStream; G4String syscmd;
    syscmdStream << "mv *.root " << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num;
    syscmd = syscmdStream.str(); system(syscmd);

    // Declare analysis vars  
    G4int zBin, numBins = 1150;
    G4double numGammas = 0;
    G4double matrixProtonsNeutrons[7][7] = {{0}};
    G4double energy_bins[1150] = {0};
    G4double charge_bins[1150] = {0};
    G4String particleName;

    for ( G4int eventNum_i=0; eventNum_i<numEvents; eventNum_i++ ) {
      // Acquire energies, charges
      binsFileNameStream.str(""); binsFileName = "";
      binsFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/" << data_dir_events << "/bins" << eventNum_i << ".dat";
      binsFileName = binsFileNameStream.str();
      binsInFileStream.open(binsFileName);
      if ( binsInFileStream.good() ) {
        // Ignore first line (commented header)
        getline(binsInFileStream, fileVarGet);

        // Iteratively reconstruct energy/charge_bins, ignoring energy_val
        while ( getline(binsInFileStream, fileVarGet, ' ') ) {
          getline(binsInFileStream, fileVarGet, ' '); zBin = atoi(fileVarGet);
          getline(binsInFileStream, fileVarGet, ' '); energy_bins[zBin] += atof(fileVarGet);
          getline(binsInFileStream, fileVarGet); charge_bins[zBin] += atof(fileVarGet);
        }
        binsInFileStream.close();
      }

      // Acquire tallies
      productionFileNameStream.str(""); productionFileName = "";
      productionFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/" << data_dir_events << "/production" << eventNum_i << ".dat";
      productionFileName = productionFileNameStream.str();
      productionInFileStream.open(productionFileName);
      
      // Acquire Gamma count, ignore header (1st, rest of 2nd, 3rd)
      getline(productionInFileStream, fileVarGet);
      getline(productionInFileStream, fileVarGet, ' ');
      getline(productionInFileStream, fileVarGet, ' ');
      numGammas += atof(fileVarGet);
      getline(productionInFileStream, fileVarGet);
      getline(productionInFileStream, fileVarGet);

      // Iteratively reconstruct matrixProtonsNeutrons
      for ( G4int protonInt = 0; protonInt<7; protonInt++ ) {
        for ( G4int neutronInt = 0; neutronInt<7; neutronInt++ ) {
          getline(productionInFileStream, fileVarGet, ' ');
          matrixProtonsNeutrons[protonInt][neutronInt] += atof(fileVarGet);
        }
      }
      productionInFileStream.close();
    }
    // Remove Event files per run
    runRmStream << "rm " << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/" << data_dir_events << "/*";
    runRm = runRmStream.str(); system(runRm);
    
    // Energies for label [future work: generalize arbitrary energy list]
    G4int E_Int = (E_Num % 5); if ( E_Int == 0 ) { E_Int = 5; }
    std::ostringstream energy_stringStream;
    energy_stringStream << E_Int;
    if ( (( E_Num % 15 ) > 5) || (( E_Num % 15) == 0) ) { energy_stringStream << "0"; }
    if ( (( E_Num % 15 ) > 10) || (( E_Num % 15) == 0) ) { energy_stringStream << "0"; }
    G4String energy_valString = energy_stringStream.str();
    G4double energy_val = atof(energy_valString);
    if ( ( 15 < E_Num ) && ( E_Num < 31 ) ) { energy_stringStream << " k"; energy_val = energy_val*1000; }
    if ( ( E_Num > 30 ) && ( E_Num < 46) ) { energy_stringStream << " M"; energy_val = energy_val*1000000; }
    energy_stringStream << "eV";
    if ( data_dir_particle == "p" ) { energy_stringStream << " Protons"; }
    if ( data_dir_particle == "n" ) { energy_stringStream << " Neutrons"; }
    G4String energy_string = energy_stringStream.str();

    // Normalize energies, charges, and output to single file
    binsFileNameStream.str(""); binsFileName = "";
    binsFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/bins.dat";
    binsFileName = binsFileNameStream.str();
    binsOutFileStream.open (binsFileName);
    binsOutFileStream << "# Energy z_(mm) Energy_Dep Net_Charge\n";
    for ( G4int binNum = 0; binNum<numBins; binNum++ ) {
      binsOutFileStream << energy_val << " " << binNum << " " << energy_bins[binNum]/numEvents << " " << charge_bins[binNum]/numEvents << "\n";
    }
    binsOutFileStream.close();

    // Add content to production file
    productionFileNameStream.str(""); productionFileName = "";
    productionFileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/production.dat";
    productionFileName = productionFileNameStream.str();
    productionOutFileStream.open (productionFileName);
    productionOutFileStream << "# Gamma Count and Particle Production Probabilities \n"  << \
                               "# " << numGammas/numEvents << " gammas per event \n" << \
                               "# [ p \\ n ] production probability matrix per event \n";
    for ( G4int protonInt = 0; protonInt<7; protonInt++ ) {
      for ( G4int neutronInt = 0; neutronInt<7; neutronInt++ ) {
        productionOutFileStream << (long double)matrixProtonsNeutrons[protonInt][neutronInt]/numEvents << " ";
      }
      productionOutFileStream << "\n";
    }
    productionOutFileStream.close();
    
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

    // Remove event files
    //runRmStream << "rm " << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/" << data_dir_events << "/*";
    //runRm = runRmStream.str(); system(runRm);
  }
}
