#ifndef THRUSTTORQUE_H
#define THRUSTTORQUE_H

enum THRUST_TYPE {	
	TH_PIT, TH_ROL, TH_YAW, 
	TH_LAT, TH_LNG, TH_VRT, 
	EN_MAIN, EN_RETRO, EN_HOVER
};

extern double GetMaxForce(THRUST_TYPE);
extern double GetMaxTorque(THRUST_TYPE);
extern void InitThrusters(VESSEL*);

#endif // def THRUSTTORQUE_H