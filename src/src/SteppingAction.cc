#include "SteppingAction.hh"
#include "EventAction.hh"
#include "DetectorConstruction.hh"

#include "G4Step.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"

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
void SteppingAction::UserSteppingAction(const G4Step* /*step*/) {
  /*
  // Tracking info
  //
  // Source propagates along z-axis
  bin_size = 1*mm;
  //
  // 1. Total charge transfer between birth and death bins
  // 2. Total energy deposited per bin
  // 3a. e- created per primary
  // 3b. p+ created per primary
  // 3c. n created per primary
  

  G4double netSignal = 0;
  // At final step
  if ( step->GetTrack()->GetTrackStatus() != fAlive ) {
	// primary event number
	G4int eventNum = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
	
	// particle name and charge
	G4String stepParticle = step->GetTrack()->GetDefinition()->GetParticleName();
    G4double stepCharge = step->GetTrack()->GetDefinition()->GetPDGCharge();
	
	// initial bin
	G4ThreeVector xyzVertex = step->GetTrack()->GetVertexPosition();
    G4double zVertex = xyzVertex[2];
    G4int zBin_init = floor(zVertex + 0.5*mm + 57.5*mm);
    // final bin
    G4ThreeVector xyz = step->GetPostStepPoint()->GetPosition();
    G4double z = xyz[2];
    G4int zBin_final = floor(z + 0.5*mm + 57.5*mm);
    
    // 1_Charge_Transfer
    if ( stepCharge != 0 ) {
      // Add to eventID's dataset
      G4String data_dir = "data/";
      
      std::ostringstream rawEventFileName;
      rawEventFileName << data_dir << "event" << eventID << "signals.txt";
      G4String eventFileName = rawEventFileName.str();
      std::ofstream eventFile;
      eventFile.open (eventFileName, std::ios::app);
      eventFile << netSignal << "\n";
      eventFile.close();
    }
    
      G4double chargeProp = ((percentRVertex>percentZVertex)?percentRVertex:percentZVertex); // concise maximum function
      netSignal -= stepCharge*(1-chargeProp);
    }
    
    // DESTINATION
    // particle enters cylinder, +q_i
    if ( volumeName == "Cu_cyl" ) { netSignal += stepCharge; }
        
    // particle enters Kapton, +q_i*max(del_r/(r_KA - r_Cu), del_z/(h_KA - h_Cu))
    if ( volumeName == "Kapton_cyl1" ) {
      G4double percentR = 0, percentZ = 0;
      // Radial edge of Kapton
      if ( stepR >= r_Cu ) { percentR = (stepR - r_Cu)/(r_KA - r_Cu); }
      // Z edges of Kapton (cylinders are upside-down)
      if ( stepZ <= -half_Cu ) { percentZ = (stepZ - (-half_Cu))/((-half_KA) - (-half_Cu)); }
      if ( stepZ >= half_Cu ) { percentZ = (stepZ - half_Cu)/(half_KA - half_Cu); }
      
      G4double chargeProp = ((percentR>percentZ)?percentR:percentZ); // concise maximum function
      netSignal += stepCharge*(1-chargeProp);
    }
  }

  if ( netSignal != 0 ) { // Zeros already counted
    G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
    
    // Add to eventID's dataset
    std::ostringstream rawEventFileName;
    rawEventFileName << data_dir << "event" << eventID << "signals.txt";
    G4String eventFileName = rawEventFileName.str();
    std::ofstream eventFile;
    eventFile.open (eventFileName, std::ios::app);
    eventFile << netSignal << "\n";
    eventFile.close();
  }
  */
}
