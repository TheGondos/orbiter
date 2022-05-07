//////////////////////////////////////////////////////////////////////////////////////////////
//
//	AttCtrl.h.h - Header file for spacecraft control 
//
//	Copyright Chris Knestrick, 2002
//
//	For a full description of the copyright, please see the file Copyright.txt
//
// 15-Jun-05 rbd	For Orbiter 2005 and Visual C++ 7 (VS.NET 2005), renamed this H
//					file to AttCtrl.h to avoid conflict with standard control.h file.
//					Add void return type to NormalizeThrustLevel() to avoid warning.
// 16-Jun-05 rbd	New call for control focus changes to init forces/torques/PMI
///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CONTROL_H
#define CONTROL_H
#include <math.h>
#include "Orbitermath.h"

// The deadband for attitude adjustments.  These are just the "standard" deadband limits - a user
// can pass any deadband value to SetAttitude().  For example, a 5 degree deadband: 
// SetAttitude(TargetYaw, CurrentYaw, YAW, DEADBAND(Radians(5)))
typedef double DEADBAND;

// CCK
//const DEADBAND DB_FINE = Radians(0.2), 
//			   DB_NORMAL = Radians(0.5), 
//			   DB_COARSE = Radians(1);

const DEADBAND DB_FINE = Radians(0.1), 
			   DB_NORMAL = Radians(0.5), 
			   DB_COARSE = Radians(1);
//CCK End

// Provide ease of access to vectors (e.g. Status.vrot.data[PITCH])
enum AXIS {PITCH, YAW, ROLL};

// Prototypes
void InitAttControl(VESSEL* V, double TimeStep);
bool SetAttitude(double TargetAttitude, double CurrentAttitude, double RotRate, 
					AXIS Axis, DEADBAND DeadbandLow);
bool SetAttitude(double TargetAttitude, double CurrentAttitude, AXIS Axis, DEADBAND DeadbandLow);
bool NullRate(AXIS Axis);

static inline void NormalizeThrustLevel(double& Level)
{
	if (Level > 1.0) {
		Level = 1.0;
	} else if (Level < -1.0) {
		Level = -1.0;
	}
}

#endif