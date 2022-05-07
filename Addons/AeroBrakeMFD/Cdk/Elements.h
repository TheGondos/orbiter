//////////////////////////////////////////////////////////////////////////////////////////////
//
//	Elements.h - Routines to provide orbital elements in vector form
//
//	Copyright Chris Knestrick, 2003
//
//	For a full description of the copyright, please see the file Copyright.txt
///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef ELEMENTS_H
#define ELEMENTS_H

#include "Orbitersdk.h"
#include "GravBodyData.h"

// Orbital elements in vector form, since Vessel->GetElements() only provides the scalars. Also
// includes several scalar elements, including the magnitude of each vector.  The general 
// convention is that if R is the vector, then r is the magnitude of that vector
struct ELEMENTS_VEC {
	GRAVBODY Body;		// The reference body of the orbit
	double Energy;		// The specific mechanic energy of the spacecraft		

	// Vectors
	VECTOR3 R;			// Position
	VECTOR3 V;			// Velocity
	VECTOR3 H;			// Angular Momentum
	VECTOR3 N;			// Node Vector
	VECTOR3 Ecc;		// Eccentricity

	// Vector magnitude
	double r;			// Position
	double v;			// Velocity
	double h;			// Angular Momentum
	double n;			// Node
	double e;			// Eccentricity
	double p;			// Semi-latus rectum
	double a;			// Semi-major axis

	// Angles
	double Inc;			// Inclination
	double LAN;			// Longitude of Ascending Node
	double AOP;			// Arguement of Periapsis
	double TrueAnomaly;	// True Anonaly (Duh!)
};

const VECTOR3 UNIT_I = { 1, 0, 0 };
const VECTOR3 UNIT_J = { 0, 1, 0 };
const VECTOR3 UNIT_K = { 0, 0, 1 };

// Prototypes
double CalcEccAnomaly(double TrueAnomaly, double Ecc);
double CalcTimeOfFlightEcc(double EccAnomaly1, double EccAnomaly2, int K,
						   const ELEMENTS_VEC &Elements);
double CalcTimeOfFlight(double TrueAnomaly1, double TrueAnomaly2, int K,
						const ELEMENTS_VEC &Elements);
void GetElements(ELEMENTS_VEC &Elements);

#endif