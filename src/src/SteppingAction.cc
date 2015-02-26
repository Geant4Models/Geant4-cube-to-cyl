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
  // 1. Total charge transfer between birth and death bins
  // 2. Total energy deposited per bin
  // 3a. e- created per primary
  // 3b. p+ created per primary
  // 3c. n created per primary
  
  // At final step
  if ( step->GetTrack()->GetTrackStatus() != fAlive ) {
	// Directory info
	std::ostringstream fileNameStream; G4String fileName;
	std::ofstream fileStream; G4String fileVarGet;
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
    
    // Primary event number and energy number
	G4int eventNum = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
	G4double E_Num = G4RunManager::GetRunManager()->GetCurrentRun()->GetRunID();
	// Adjust event number by state var
	G4int Al_num = 0;   if ( Al_side == 10 ) Al_num = 1; if ( Al_side == 25 ) Al_num = 2;
	E_Num = E_Num - Al_num*45 + 1;
		
    // particle name, charge, and energy deposited
	G4String particleName = step->GetTrack()->GetDefinition()->GetParticleName();
    G4double particleCharge = step->GetTrack()->GetDefinition()->GetPDGCharge();
    G4double particleEDep = step->GetTotalEnergyDeposit();
	
	// also E_num goes screwy high
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
    
    // 1_Charge_Transfer
    if ( particleCharge != 0 ) {
	  // - init bin
	  fileNameStream.str(""); fileName = "";
      fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/" << data_dir_analysis << "1/" << zBin_init << ".txt";
      fileName = fileNameStream.str();
      fileStream.open (fileName, std::ios::app);
      fileStream << -particleCharge << "\n";
      fileStream.close();
      
	  // + final_bin
	  fileNameStream.str(""); fileName = "";
      fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/" << data_dir_analysis << "1/" << zBin_final << ".txt";
      fileName = fileNameStream.str();
      fileStream.open (fileName, std::ios::app);
      fileStream << particleCharge << "\n";
      fileStream.close();
    }
    
    // 2_Energy_Deposition
    // + final bin
	fileNameStream.str(""); fileName = "";
    fileNameStream << data_dir << data_dir_geo << Al_side << "/" << data_dir_energy << E_Num << "/" << data_dir_analysis << "2/" << zBin_final;
    fileName = fileNameStream.str();
    fileStream.open (fileName, std::ios::app);
    fileStream << particleEDep << "\n";
    fileStream.close();
  }
}
