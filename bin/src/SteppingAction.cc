#include "SteppingAction.hh"
#include "EventAction.hh"
#include "DetectorConstruction.hh"
#include "Analysis.hh"

#include "G4SystemOfUnits.hh"
#include "G4Step.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <math.h>
#include <stdio.h>

// Initialize Step Procedure
SteppingAction::SteppingAction(
                const DetectorConstruction* detectorConstruction,
                EventAction* eventAction)
               : G4UserSteppingAction(),
                 fDetConstruction(detectorConstruction),
                 fEventAction(eventAction) {}
SteppingAction::~SteppingAction() {}

// Step Procedure (for every step...)
void SteppingAction::UserSteppingAction(const G4Step* step) {

  // Tracking info
  //
  // Source propagates along z-axis
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
  G4String data_dir_particle = "p";
  G4String data_dir_energy = "E";
  G4String data_dir_analysis = "Analysis";
  
  // Acqiure state vars
  G4int Al_side;
  fileNameStream << data_dir << ".state";
  fileName = fileNameStream.str();
  std::ifstream stateFile;
  stateFile.open(fileName);
  stateFile >> fileVarGet;
  Al_side = atoi(fileVarGet);
    
  // Primary event number and energy (run) number
  G4int eventNum = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
  G4int runID = G4RunManager::GetRunManager()->GetCurrentRun()->GetRunID();
  // Adjust Energy number by state var, beam particle by energy number
  G4int Al_num = 0;   if ( Al_side == 10 ) Al_num = 1; if ( Al_side == 25 ) Al_num = 2;
  G4int E_Num = runID - Al_num*90 + 1;
  if ( E_Num > 45 ) { E_Num = E_Num - 45; data_dir_particle = "n"; }
  
  // Get half-length of world to fix z counting [(-L/2, L/2) -> (0, L)]
  G4double worldZHalfLength = 0; G4Box* worldBox = 0;
  G4LogicalVolume* worldLV = G4LogicalVolumeStore::GetInstance()->GetVolume("World");
  if ( worldLV) worldBox = dynamic_cast< G4Box*>(worldLV->GetSolid()); 
  if ( worldBox ) { worldZHalfLength = worldBox->GetZHalfLength(); }
	
  // initial bin
  G4ThreeVector xyzVertex = step->GetTrack()->GetVertexPosition();
  G4double zVertex = xyzVertex[2] + worldZHalfLength;
  G4int zBin_init = floor(zVertex);
  // final bin
  G4ThreeVector xyz = step->GetPostStepPoint()->GetPosition();
  G4double z = xyz[2] + worldZHalfLength;
  G4int zBin_final = floor(z); if ( zBin_final >= 1149 ) zBin_final = 1149;
  
  // 1 Energy Deposition
  G4double particleEDep = step->GetTotalEnergyDeposit();
  // + final bin
  fileNameStream.str(""); fileName = "";
  fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/" << data_dir_analysis << "1/event" << eventNum << ".txt";
  fileName = fileNameStream.str();
  fileStream.open (fileName, std::ios::app);
  fileStream << zBin_final << " " << particleEDep << "\n";
  fileStream.close();
  
  // At final step
  if ( step->GetTrack()->GetTrackStatus() != fAlive ) {
    // particle name, charge, and parentID
    G4int particleParentID = step->GetTrack()->GetParentID();
    G4String particleName = step->GetTrack()->GetDefinition()->GetParticleName();
    G4double particleCharge = step->GetTrack()->GetDefinition()->GetPDGCharge();
	
    // 2 Charge Transfer
    if ( particleCharge != 0 ) {
      // - init bin
      fileNameStream.str(""); fileName = "";
      fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/" << data_dir_analysis << "2/event" << eventNum << ".txt";
      fileName = fileNameStream.str();
      fileStream.open (fileName, std::ios::app);
      fileStream << zBin_init << " init " << particleCharge << "\n";
      fileStream.close();
      
      // + final_bin
      fileNameStream.str(""); fileName = "";
      fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/" << data_dir_analysis << "2/event" << eventNum << ".txt";
      fileName = fileNameStream.str();
      fileStream.open (fileName, std::ios::app);
      fileStream << zBin_final << " final " << particleCharge << "\n";
      fileStream.close();
    }
    
    // 3a Electron backscatter via Gamma ray counting (method to come)
    // 3b,c proton and neutron production;
    // Segfault may be here
    if ( particleParentID != 0 && ( particleName == "gamma" || particleName == "proton" || particleName == "neutron" )  ) {
      // Add to tally
      fileNameStream.str(""); fileName = "";
      fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_particle << data_dir_energy << E_Num << "/" << data_dir_analysis << "3/event" << eventNum << ".txt";
      fileName = fileNameStream.str();
      fileStream.open (fileName, std::ios::app);
      fileStream << particleName << "\n";
      fileStream.close();
    }
  }

  // Planar Detector Hit data
  G4String volumeName = step->GetTrack()->GetVolume()->GetName();
  G4String volumeNameVertex = step->GetTrack()->GetLogicalVolumeAtVertex()->GetName();

  if ( volumeName == "Detector_Box" && volumeNameVertex != "Detector_Box" ) {

    // Hit properties
    G4ThreeVector stepXYZ = step->GetPostStepPoint()->GetPosition();
    G4double stepX = stepXYZ[0]; G4double stepY = stepXYZ[1];
    G4ThreeVector stepPxPyPz = step->GetTrack()->GetMomentum();
    G4double stepPx = stepPxPyPz[0]; G4double stepPy = stepPxPyPz[1]; G4double stepPz = stepPxPyPz[2];
    G4String particleName = step->GetTrack()->GetDefinition()->GetParticleName();
    G4int particleID = 5;

    // Digitize particle name
    if ( particleName == "e-" ) { particleID = 0; }
    if ( particleName == "proton" ) { particleID = 1; }
    if ( particleName == "neutron" ) { particleID = 2; }
    if ( particleName == "gamma" ) { particleID = 4; }

    // Fill ROOT ntuple
    G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetVerboseLevel(10);
    analysisManager->FillNtupleIColumn(0, runID);       // run
    analysisManager->FillNtupleIColumn(1, eventNum);    // event
    analysisManager->FillNtupleIColumn(2, particleID);  // particle name id
    analysisManager->FillNtupleDColumn(3, stepX);       // hit_x
    analysisManager->FillNtupleDColumn(4, stepY);       // hit_y
    analysisManager->FillNtupleDColumn(5, stepPx);      // hit_px
    analysisManager->FillNtupleDColumn(6, stepPy);      // hit_py
    analysisManager->FillNtupleDColumn(7, stepPz);      // hit_pz
    analysisManager->AddNtupleRow();
    
  }
}
