//tabs=4
//
// Code to get the force and torques (as applicable) for the
// thrusters (including the "big" ones!). And yes, this could
// be made more efficient by looping through the thruster
// groups just once for both thrust and torque. Also, the 
// MaxForce/MaxTorque arrays could be accessed directly, but
// I wanted to provide type safety on the indexes.
//

#include "CDK.h"

// These are initialized once at startup and any change in Ship class name
static double MaxForce[9] = {0,0,0,0,0,0,0,0,0};		// Max force ratings (Newtons) for thrusters in each axis
static double MaxTorque[9] = {0,0,0,0,0,0,0,0,0};		// Max torque ratings (Newton-meters) for thrusters in each axis

static double GetThrusterGroupMaxForce(THGROUP_TYPE thgt, VESSEL *Ship)
{
	int num;
	THRUSTER_HANDLE th;
	double max = 0.0;

	if(Ship == NULL) return 0.0;						// Safety
	num = Ship->GetGroupThrusterCount(thgt);
	for (int i = 0; i < num; i++)
	{
		th = Ship->GetGroupThruster(thgt, i);
		max = max + Ship->GetThrusterMax(th);
	}
	return max;
}

//
// This assumes that there is no cross-coupling of thrusters, and that 
// they are all aimed exactly along the spacecraft principal axes.
//
static double GetThrusterGroupMaxTorque(THGROUP_TYPE thgt, VESSEL *Ship)
{
	int num;
	THRUSTER_HANDLE th;
	double max = 0.0;

	if(Ship == NULL) return 0.0;						// Safety
	num = Ship->GetGroupThrusterCount(thgt);
	for (int i = 0; i < num; i++)
	{
		VECTOR3 R;
		th = Ship->GetGroupThruster(thgt, i);
		Ship->GetThrusterRef(th, R);
		max = max + (Ship->GetThrusterMax(th) * length(R));
	}
	return max;
}	

// ================
// PUBLIC INTERFACE
// ================

double GetMaxForce(THRUST_TYPE t)	{ return MaxForce[t];  }
double GetMaxTorque(THRUST_TYPE t)	{ return MaxTorque[t]; }

//
// Initialize thruster force and torque ratings once per startup of 
// Orbiter and the client MFD, and each time the vessel is changed. 
// These things should almost certainly NOT be global like this... 
// take care to call this when a focus vessel change is done!
//
// Assumption: Thrusters for left vs right, fore vs aft, up vs down
// have the same forces. We don't distinguish between left and right
// fore and aft, or up and down. This may have to change. For now, 
// we return the average force of the pair of groups when used for 
// translation.
//
void InitThrusters(VESSEL *S)
{
	MaxForce[TH_PIT]	= (GetThrusterGroupMaxForce(THGROUP_ATT_PITCHUP,S) + 
							GetThrusterGroupMaxForce(THGROUP_ATT_PITCHDOWN,S));
	MaxForce[TH_ROL]	= (GetThrusterGroupMaxForce(THGROUP_ATT_BANKLEFT,S) + 
							GetThrusterGroupMaxForce(THGROUP_ATT_BANKRIGHT,S));
	MaxForce[TH_YAW]	= (GetThrusterGroupMaxForce(THGROUP_ATT_YAWLEFT,S) + 
							GetThrusterGroupMaxForce(THGROUP_ATT_YAWRIGHT,S));

	MaxForce[TH_LAT]	= (GetThrusterGroupMaxForce(THGROUP_ATT_LEFT,S) + 
							GetThrusterGroupMaxForce(THGROUP_ATT_RIGHT,S))/2.0;
	MaxForce[TH_LNG]	= (GetThrusterGroupMaxForce(THGROUP_ATT_FORWARD,S) + 
							GetThrusterGroupMaxForce(THGROUP_ATT_BACK,S))/2.0;
	MaxForce[TH_VRT]	= (GetThrusterGroupMaxForce(THGROUP_ATT_UP,S) + 
							GetThrusterGroupMaxForce(THGROUP_ATT_DOWN,S))/2.0;

	MaxForce[EN_MAIN]	= GetThrusterGroupMaxForce(THGROUP_MAIN,S);
	MaxForce[EN_RETRO]	= GetThrusterGroupMaxForce(THGROUP_RETRO,S);
	MaxForce[EN_HOVER]	= GetThrusterGroupMaxForce(THGROUP_HOVER,S);

	MaxTorque[TH_PIT]	= (GetThrusterGroupMaxTorque(THGROUP_ATT_PITCHUP,S) + 
							GetThrusterGroupMaxTorque(THGROUP_ATT_PITCHDOWN,S));
	MaxTorque[TH_ROL]	= (GetThrusterGroupMaxTorque(THGROUP_ATT_BANKLEFT,S) + 
							GetThrusterGroupMaxTorque(THGROUP_ATT_BANKRIGHT,S));
	MaxTorque[TH_YAW]	= (GetThrusterGroupMaxTorque(THGROUP_ATT_YAWLEFT,S) + 
							GetThrusterGroupMaxTorque(THGROUP_ATT_YAWRIGHT,S));

	MaxTorque[TH_LAT]	= 0.0;											// No torques applicable
	MaxTorque[TH_LNG]	= 0.0;
	MaxTorque[TH_VRT]	= 0.0;

	MaxTorque[EN_MAIN]	= 0.0;
	MaxTorque[EN_RETRO] = 0.0;
	MaxTorque[EN_HOVER] = 0.0;

}

