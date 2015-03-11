# $Id: GNUmakefile $
# -----------------------------------------------------------------------
# GNUmakefile for "Geant4Models/cube-to-cyl".  Shaun Marshall, 2015-02-21
# -----------------------------------------------------------------------

name := cube-to-cyl
G4TARGET := $(name)
G4EXLIB := true

ifndef G4INSTALL
  G4INSTALL = ../../..
endif

.PHONY: all
all: lib bin

include $(G4INSTALL)/config/binmake.gmk

visclean:
	rm -f g4*.prim g4*.eps g4*.wrl
	rm -f .DAWN_*

