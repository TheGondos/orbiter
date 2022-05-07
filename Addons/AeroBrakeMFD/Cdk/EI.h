//////////////////////////////////////////////////////////////////////////////////////////////
//
//	EI.h - Code to calculate the entry interface paramters
//
//	Copyright Chris Knestrick, 2003
//
//	For a full description of the copyright, please see the file Copyright.txt
///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef EI_H
#define EI_H

struct ENTRY_INTERFACE {
	bool InterfaceDefined;	// Indicates if the interface is defined.  This is used to
							// indicate the return status of CalcEntryInterface() 

	double Angle;			// Angle between the velocity vector and the local horizon
	double TimeToGo;		// Time until entry interface
	VECTOR3 Pos, Vel;		// The predicted position and velocity vectors at entry interface
};

void CalcEntryInterface(ENTRY_INTERFACE &Interface);

#endif