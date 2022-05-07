//////////////////////////////////////////////////////////////////////////////////////////////
//
//	AttCtrl.cpp - Routines to provide spacecraft control
//
//	Copyright Chris Knestrick, 2002
//
//	For a full description of the copyright, please see the file Copyright.txt
//
// 15-Jun-05 rbd	For Visual C++ 7 (VS.NET 2005) removed iomanip.h. Changed name to AttCtrl
//					to avoid name clash with standard include file control.h. Cache the 
//					vessel pointer to avoid needless calls to get it. Add new function
//					ControlFocusChanged() to trigger initialization here. Change the method
//					of getting thruster forces to the supported calls (see ThrustTorque.cpp)
//					and call the intiializer there on vessel focus changes and at startup.
///////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "CDK.h"

// CCK
// These are magnitudes, sign can be + or -.
//const double DEADBAND_MID = Radians(2);		
//const double DEADBAND_HIGH = Radians(10);
//const double RATE_NULL = Radians(0.05);
//const double RATE_LOW = Radians(0.1);		
//const double RATE_MID = Radians(1.0);
//const double RATE_HIGH = Radians(2.0);

 
const double RATE_MAX = Radians(2.0);
const double DEADBAND_MAX = Radians(10.0);
const double RATE_HIGH = Radians(1.0);
const double DEADBAND_HIGH = Radians(3.0);
const double RATE_MID = Radians(0.5);
const double DEADBAND_MID = Radians(1.0);
const double RATE_LOW = Radians(0.25);
const double DEADBAND_LOW = Radians(0.15);
const double RATE_FINE = Radians(0.01);

const double RATE_NULL = Radians(0.0001);
// CCK End

static VESSEL *Vessel = NULL;
static VECTOR3 PMI;									// Principal moments of inertia (mass-normalized)
static double TimeStep = 1.0;

#pragma warning (push)
#pragma warning (disable : 4715)
static inline THRUST_TYPE AxisToThruster(AXIS Axis)
{
	switch(Axis)
	{
	case PITCH:	return(TH_PIT);
	case YAW:	return(TH_YAW);
	case ROLL:	return(TH_ROL);
	}
// warning C4715: 'AxisToThruster' : not all control paths return a value
}
#pragma warning (pop)

//
// Called from client at startup and if vessel changes during operation
//
void InitAttControl(VESSEL *V, double dT)
{
	if(dT > 0.01)
		TimeStep = dT;								// Needed for required-torque calculations
	if (V) {
		Vessel = V;
		InitThrusters(Vessel);
		Vessel->GetPMI(PMI);
	}
}


// Generates rotational commands in order to achieve a target attitude in a given axis.
// Note that the user is responsible for providing an accurate current attitude information.
// This allows for greater flexability but also can be a source of problems if you pass
// incorrect data to the function.  For example, if you invert signs, then the spacecraft will
// fly out of control because as it rotates in the correct direction, the DeltaAngle will
// increase!  Please be careful :-)  The function returns true when the desired attitude
// has been reached (within the given deadband) and the velocity has been nulled; otherwise, 
// it returns false. 
bool SetAttitude(double TargetAttitude, double CurrentAttitude, double RotRate, 
				 AXIS Axis, DEADBAND DeadbandLow)
{	
	double Rate;			// Depends on magnitude of DeltaAngle
	double RateDeadband;	// Depends on the magnitude of Rate
	double TorqueReq;		// Torque required to achieve rate in 1 ttime step (N.B.)
	double Level;			// Required thrust level [0 - 1]
	double Mass;			// Spacecraft mass

	double DeltaRate;		// The difference between the desired and the actual rate
	double DeltaAngle = TargetAttitude - CurrentAttitude;

	// Get State
	Mass = Vessel->GetMass();

	// Let's take care of the good case first :-)
	if ((fabs(DeltaAngle) < DeadbandLow)) {
		//sprintf(oapiDebugString(), "NULL");
		if (fabs(RotRate) < RATE_NULL) {
			Vessel->SetAttitudeRotLevel(Axis, 0);
			return true;
		} 
	
		return (NullRate(Axis));
	}

	// CCK 
	// Now, we actually have to DO something! ;-)  Well divide it up into two cases, once
	// for each direction.  There's probably a better way, but not right now :-)
	//if (fabs(DeltaAngle) < DEADBAND_MID) {
	//	Rate = RATE_LOW;
	//	sprintf(oapiDebugString(), "LOW");
	//} else if(fabs(DeltaAngle) < DEADBAND_HIGH) {
	//	Rate = RATE_MID;	
	//	sprintf(oapiDebugString(), "MID");
	//} else {
	//	Rate = RATE_HIGH;
	//	sprintf(oapiDebugString(), "HIGH");
	//}
	if (fabs(DeltaAngle) < DEADBAND_LOW) {
		Rate = RATE_FINE;
		//sprintf(oapiDebugString(), "FINE");
	} else if (fabs(DeltaAngle) < DEADBAND_MID) {
		Rate = RATE_LOW;
		//sprintf(oapiDebugString(), "LOW");
	} else if (fabs(DeltaAngle) < DEADBAND_HIGH) {
		Rate = RATE_MID;	
		//sprintf(oapiDebugString(), "MID");
	} else if (fabs(DeltaAngle) < DEADBAND_MAX) {
		Rate = RATE_HIGH;
		//sprintf(oapiDebugString(), "HIGH");
	} else {
		Rate = RATE_MAX;
		//sprintf(oapiDebugString(), "MAX");
	}
	// CCK End

	RateDeadband = min(Rate / 2, Radians(0.01/*2*/));
	
	if (DeltaAngle < 0 ) {
		Rate = -Rate;
		RateDeadband = -RateDeadband;
	}

	DeltaRate = Rate - RotRate;
	
	if (DeltaAngle > 0) {
		if (DeltaRate > RateDeadband) {
			TorqueReq = (Mass * PMI.data[Axis] * DeltaRate) / TimeStep;
			Level = min((TorqueReq/GetMaxTorque(AxisToThruster(Axis))), 1);
			Vessel->SetAttitudeRotLevel(Axis, Level);
		} else if (DeltaRate < -RateDeadband) { 
			TorqueReq = (Mass * PMI.data[Axis] * DeltaRate) / TimeStep;
			Level = max((TorqueReq/GetMaxTorque(AxisToThruster(Axis))), -1);
			Vessel->SetAttitudeRotLevel(Axis, Level);
		} else {
			Vessel->SetAttitudeRotLevel(Axis, 0);
		}
	} else {
		if (DeltaRate < RateDeadband) {
			TorqueReq = (Mass * PMI.data[Axis] * DeltaRate) / TimeStep;
			Level = max((TorqueReq/GetMaxTorque(AxisToThruster(Axis))), -1);
			Vessel->SetAttitudeRotLevel(Axis, Level);
		} else if (DeltaRate > -RateDeadband) { 
			TorqueReq = (Mass * PMI.data[Axis] * DeltaRate) / TimeStep;
			Level = min((TorqueReq/GetMaxTorque(AxisToThruster(Axis))), 1);
			Vessel->SetAttitudeRotLevel(Axis, Level);
		} else {
			Vessel->SetAttitudeRotLevel(Axis, 0);
		}
	}



	return false;
}


// Wrapper for the above function that assumes that the rotation rate is vrot.
// Calling the function directly allows more control but will probably be used less
bool SetAttitude(double TargetAttitude, double CurrentAttitude, AXIS Axis, DEADBAND DeadbandLow)
{	
	VESSELSTATUS Status;	// Spacecraft status

	Vessel->GetStatus(Status);
	
	return SetAttitude(TargetAttitude, CurrentAttitude, Status.vrot.data[Axis], Axis, DeadbandLow);
}


// Basically NAVMODE_KILLROT in a single dimension.  Returns true when the rate has been nulled
// out, otherwise it returns false.
bool NullRate(AXIS Axis)
{
	VESSELSTATUS Status;
	Vessel->GetStatus(Status);

	double Rate = Status.vrot.data[Axis];

	//
	// Apply nulling torque for as long as it takes. Calculate the torque
	// needed over one time step. Then apply either that or the max torque
	// available from the thruster group, whichever is smaller. Next time step
	// we do the same thing, Eventually, we'll get to where the torque needed
	// over a time step is less than max torque. At the time step after that,
	// we should find the rate nulled.
	//
	if (fabs(Rate) < RATE_NULL) {										// Already nulled?
		Vessel->SetAttitudeRotLevel(Axis, 0.0);
		return true;													// (yes)
	}
	double Mass = Vessel->GetMass();
	double TorqueReq = -(Mass * PMI.data[Axis] * Rate) / TimeStep;
	double Level = TorqueReq/GetMaxTorque(AxisToThruster(Axis));
	Level = min(Level, 1);
	Level = max(Level, -1);
	Vessel->SetAttitudeRotLevel(Axis, Level);
	return false;

}