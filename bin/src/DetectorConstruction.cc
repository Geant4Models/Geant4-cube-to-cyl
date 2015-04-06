#include "DetectorConstruction.hh"

#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Transform3D.hh"
#include "G4RotationMatrix.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4PVReplica.hh"
#include "G4GlobalMagFieldMessenger.hh"
#include "G4AutoDelete.hh"
#include "G4GeometryManager.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SolidStore.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"

G4ThreadLocal 
G4GlobalMagFieldMessenger* DetectorConstruction::fMagFieldMessenger = 0; 

DetectorConstruction::DetectorConstruction()
 : G4VUserDetectorConstruction(),
   fDetector(0),
   fCheckOverlaps(true) {}

DetectorConstruction::~DetectorConstruction() {}

G4VPhysicalVolume* DetectorConstruction::Construct() {
  // Define materials and volumes
  DefineMaterials();  
  return DefineVolumes();
}

void DetectorConstruction::DefineMaterials() { 
  // Materials defined using NIST Manager
  G4NistManager* nistManager = G4NistManager::Instance();
  nistManager->FindOrBuildMaterial("G4_He");
  nistManager->FindOrBuildMaterial("G4_Al");
  
  // Geant4 conventional definition of a vacuum
  G4double density     = universe_mean_density;  //from PhysicalConstants.h
  G4double pressure    = 1.e-19*pascal;
  G4double temperature = 0.1*kelvin;
  new G4Material("Vacuum", 1., 1.01*g/mole, density,
                   kStateGas,temperature,pressure);

  // Print materials
  G4cout << *(G4Material::GetMaterialTable()) << G4endl;
}

//// Geometry parameters
G4VPhysicalVolume* DetectorConstruction::DefineVolumes() {

  // World box parameters
  G4double world_cube_X = 50*cm;
  G4double world_cube_Y = 50*cm;
  G4double world_cube_Z = 115*cm;
  
  // Al box parameters (5cm side initially)
  G4double Al_cube_X = 5*cm;
  G4double Al_cube_Y = 5*cm;
  G4double Al_cube_Z = 5*cm;
  // 50 cm from source (-7.5 cm from world center)
  G4ThreeVector Al_pos = G4ThreeVector(0*cm, 0*cm,-7.5*cm);
  G4RotationMatrix Al_box_rot = G4RotationMatrix(G4ThreeVector(), G4ThreeVector(), G4ThreeVector());
  G4Transform3D Al_transform = G4Transform3D(Al_box_rot, Al_pos);

  // Planar Detector geometry (thin cube)
  G4double detector_cube_X = 15*cm;
  G4double detector_cube_Y = 15*cm;
  G4double detector_cube_Z = 0.5*mm;
  // del_z from Al box (k cm from box center, [k - 7.5*cm] from world center)
  G4double detector_k_pos = 20*cm;
  G4ThreeVector detector_pos = G4ThreeVector(0*cm, 0*cm, detector_k_pos);
  G4RotationMatrix detector_rot = G4RotationMatrix(G4ThreeVector(), G4ThreeVector(), G4ThreeVector());
  G4Transform3D detector_transform = G4Transform3D(detector_rot, detector_pos);
  
  // He cylinder parameters
  G4double He_cyl_innerRadius = 0*cm;
  G4double He_cyl_outerRadius = 15*cm;
  G4double He_cyl_height = 50*cm;
  G4double He_cyl_startAngle = 0*deg;
  G4double He_cyl_spanningAngle = 360*deg;
  // Rotate about x-axis 90 degrees
  G4double phi = 90*deg;
  G4RotationMatrix He_cyl_rot = G4RotationMatrix(G4ThreeVector(1, 0, 0),
                                                 G4ThreeVector(0, std::cos(phi), -std::sin(phi)),
                                                 G4ThreeVector(0, std::sin(phi), std::cos(phi)));
  // 100 cm from source (+42.5 cm from world center)
  G4ThreeVector He_pos = G4ThreeVector(0*cm, 0*cm, 42.5*cm);
  G4Transform3D He_transform = G4Transform3D(He_cyl_rot, He_pos);
  
  // Get materials
  G4Material* defaultMaterial = G4Material::GetMaterial("Vacuum");
  G4Material* heliumMaterial = G4Material::GetMaterial("G4_He");
  G4Material* aluminumMaterial = G4Material::GetMaterial("G4_Al");

  // Throw exception to ensure material usability
  if ( ! defaultMaterial || ! heliumMaterial || ! aluminumMaterial ) {
    G4ExceptionDescription msg;
    msg << "Cannot retrieve materials already defined."; 
    G4Exception("DetectorConstruction::DefineVolumes()",
      "MyCode0001", FatalException, msg);
  }  
   
  // World Volume
  G4VSolid* worldS 
    = new G4Box("World",            // its name
                 world_cube_X/2,    // its half sideX
                 world_cube_Y/2,    // its half sideY
                 world_cube_Z/2);   // its half sideZ
                         
  G4LogicalVolume* worldLV
    = new G4LogicalVolume(
                 worldS,            // its solid
                 defaultMaterial,   // its material
                 "World");          // its name
                                   
  G4VPhysicalVolume* worldPV
    = new G4PVPlacement(
                 0,                 // no rotation
                 G4ThreeVector(),   // At origin
                 worldLV,           // its logical volume                         
                 "World",           // its name
                 0,                 // its mother  volume
                 false,             // no boolean operation
                 0,                 // copy number
                 fCheckOverlaps);   // checking overlaps 

  // Aluminum Box Volume
  G4VSolid* Al_boxS 
    = new G4Box("AlBox",            // its name
                 Al_cube_X/2,       // its half sideX
                 Al_cube_Y/2,       // its half sideY
                 Al_cube_Z/2);      // its half sideZ
                         
  G4LogicalVolume* Al_boxLV
    = new G4LogicalVolume(
                 Al_boxS,           // its solid
                 aluminumMaterial,  // its material
                 "Al_box");         // its name
                                   
  fDetector
    = new G4PVPlacement(
                 Al_transform,             // its transformation
                 Al_boxLV,                 // its logical volume                         
                 "Al_box",                 // its name
                 worldLV,                  // its mother  volume
                 false,                    // no boolean operation
                 0,                        // copy number
                 fCheckOverlaps);          // checking overlaps

  // Planar Detector Volume
  G4VSolid* Detector_boxS 
    = new G4Box("DetectorBox",             // its name
                 detector_cube_X/2,        // its half sideX
                 detector_cube_Y/2,        // its half sideY
                 detector_cube_Z/2);       // its half sideZ
                         
  G4LogicalVolume* Detector_boxLV
    = new G4LogicalVolume(
                 Detector_boxS,            // its solid
                 defaultMaterial,          // its material
                 "Detector_box");          // its name
                                   
  fDetector
    = new G4PVPlacement(
                 detector_transform,       // its transformation
                 Detector_boxLV,           // its logical volume                         
                 "Detector_box",           // its name
                 worldLV,                  // its mother  volume
                 false,                    // no boolean operation
                 0,                        // copy number
                 fCheckOverlaps);          // checking overlaps

  // Helium Cylinder Volume
  G4VSolid* He_cylS 
    = new G4Tubs("He_cyl",                 // its name
                 He_cyl_innerRadius,       // its inner radius
                 He_cyl_outerRadius,       // its outer radius
                 He_cyl_height/2,          // its half-height
                 He_cyl_startAngle,        // its init angle
                 He_cyl_spanningAngle);    // its end angle
                         
  G4LogicalVolume* He_cylLV
    = new G4LogicalVolume(
                 He_cylS,                  // its solid
                 heliumMaterial,           // its material
                 "He_cyl");                // its name
                                   
  fDetector
    = new G4PVPlacement(
                 He_transform,              // its transformation
                 He_cylLV,                  // its logical volume                         
                 "He_cyl",                  // its name
                 worldLV,                   // its mother volume
                 false,                     // no boolean operation
                 0,                         // copy number
                 fCheckOverlaps);           // checking overlaps

  // Visualization attributes
  worldLV->SetVisAttributes (G4VisAttributes::Invisible);
  G4VisAttributes* simpleBoxVisAtt= new G4VisAttributes(G4Colour(1.0, 1.0, 1.0));
  simpleBoxVisAtt->SetVisibility(true);
  Al_boxLV->SetVisAttributes(simpleBoxVisAtt);
  Detector_boxLV->SetVisAttributes(simpleBoxVisAtt);
  He_cylLV->SetVisAttributes(simpleBoxVisAtt);

  // Always return the physical World
  return worldPV;
}

void DetectorConstruction::AlSideIteration(G4int side_i) {

  // Variable dimensions
  G4double Al_side[3] = {5*cm, 10*cm, 25*cm};
  
  // Acquire logical and physical volumes
  G4LogicalVolume* worldLV = G4LogicalVolumeStore::GetInstance()->GetVolume("World");
  G4VPhysicalVolume* worldPV = worldLV->GetDaughter(0);
  G4LogicalVolume* Al_boxLV = G4LogicalVolumeStore::GetInstance()->GetVolume("Al_box");
  
  // Unlock geometry layer Al_box, redefine and lock
  G4GeometryManager* geomManager = G4GeometryManager::GetInstance();
  geomManager->OpenGeometry(worldPV);

  Al_boxLV->SetSolid(new G4Box("World",
                 Al_side[side_i]/2,
                 Al_side[side_i]/2,
                 Al_side[side_i]/2));
                 
  geomManager->CloseGeometry(worldPV);
}
