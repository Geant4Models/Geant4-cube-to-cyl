#include "SteppingAction.hh"
#include "EventAction.hh"
#include "DetectorConstruction.hh"

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
  G4int E_Num = G4RunManager::GetRunManager()->GetCurrentRun()->GetRunID();
  // Adjust Energy number by state var
  G4int Al_num = 0;   if ( Al_side == 10 ) Al_num = 1; if ( Al_side == 25 ) Al_num = 2;
  E_Num = E_Num - Al_num*45 + 1;
  
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
  G4int zBin_final = floor(z);
  
  // 1 Energy Deposition
  G4double particleEDep = step->GetTotalEnergyDeposit();
  // + final bin
  fileNameStream.str(""); fileName = "";
  fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/" << data_dir_analysis << "1/event" << eventNum << ".txt";
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
      fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/" << data_dir_analysis << "2/event" << eventNum << ".txt";
      fileName = fileNameStream.str();
      fileStream.open (fileName, std::ios::app);
      fileStream << zBin_init << " init " << -particleCharge << "\n";
      fileStream.close();
      
	  // + final_bin
	  fileNameStream.str(""); fileName = "";
      fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/" << data_dir_analysis << "2/event" << eventNum << ".txt";
      fileName = fileNameStream.str();
      fileStream.open (fileName, std::ios::app);
      fileStream << zBin_final << " final " << particleCharge << "\n";
      fileStream.close();
    }
    
    // 3a Electron backscatter via Gamma ray counting (method to come)
    // 3b,c proton and neutron production;
    // Currently broken
    if ( particleParentID != 0 && ( particleName == "gamma" || particleName == "proton" || particleName == "neutron" )  ) {
      // Add to tally
	  fileNameStream.str(""); fileName = "";
      fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/" << data_dir_analysis << "3/event" << eventNum << ".txt";
      fileName = fileNameStream.str();
      fileStream.open (fileName, std::ios::app);
      fileStream << particleName << "\n";
      fileStream.close();
    }
  }
}
