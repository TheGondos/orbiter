//////////////////////////////////////////////////////////////////////////////////////////////
//
//	Prediction.h - Code to solve the prediction problem - given the position and velocity
//					 vectors at some initial time, what will the position and velocity vectors
//					 be at specified time in the future.  The method implemented here is
//					 described in Chapter 4 of "Fundementals of Astrodynamics" by Bate, Mueller,
//					 and White.
//
//	Copyright Chris Knestrick, 2003
//
//	For a full description of the copyright, please see the file Copyright.txt
///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef PREDICTION_H
#define PREDICTION_H
#include "Orbitersdk.h"
#include "Elements.h"

void PredictPosVelVectors(const VECTOR3 &Pos, const VECTOR3 &Vel, double a, double Mu, 
						  double Time, VECTOR3 &NewPos, VECTOR3 &NewVel);

#endif