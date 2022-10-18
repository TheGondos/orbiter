// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//              ORBITER MODULE: Scenario Editor
//                  Part of the ORBITER SDK
//
// ScnEditor.cpp
//
// A plugin module to edit a scenario during the simulation.
// This allows creation, deleting and configuration of vessels.
// ==============================================================

#define STRICT 1
#define ORBITER_MODULE
#include "Orbitersdk.h"
//#include "resource.h"
#include "Editor.h"
//#include "DlgCtrl.h"

// ==============================================================
// Global variables and constants
// ==============================================================

ScnEditor *g_editor = 0;   // scenario editor instance pointer

// ==============================================================
// API interface
// ==============================================================

// ==============================================================
// Initialise module

DLLCLBK void InitModule (MODULEHANDLE hDLL)
{
	// Create editor instance
	g_editor = new ScnEditor ();
}

// ==============================================================
// Clean up module

DLLCLBK void ExitModule (MODULEHANDLE hDLL)
{
	// Delete editor instance
	delete g_editor;
	g_editor = 0;
}

// ==============================================================
// Vessel destruction notification

DLLCLBK void opcDeleteVessel (OBJHANDLE hVessel)
{
	g_editor->VesselDeleted (hVessel);
}

// ==============================================================
// Pause state change notification

DLLCLBK void opcPause (bool pause)
{
	g_editor->Pause (pause);
}