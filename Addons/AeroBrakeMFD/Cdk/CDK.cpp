#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "CDK.h"


// Print out a vector to the debug string, with a precision of two decimal places
void PrintVector(VECTOR3 Vec)
{
	sprintf(oapiDebugString(), "%.3f  %.3f  %.3f", Vec.x, Vec.y, Vec.z);
}

// Same as above, except the vector contains angles
void PrintAngleVector(VECTOR3 Vec)
{
	sprintf(oapiDebugString(), "%.3f  %.3f  %.3f", Degree(Vec.x), Degree(Vec.y), Degree(Vec.z));
}

// Print an integer
void PrintInt(int x)
{
	sprintf(oapiDebugString(), "%d", x);	
}

// Print a real (double)
void PrintReal(double x)
{
	sprintf(oapiDebugString(), "%.3f", x);	
}

void PrintReal(double x, double y)
{
	sprintf(oapiDebugString(), "%.3f  %.3f", x, y);	
}

void PrintReal(double x, double y, double z)
{
	sprintf(oapiDebugString(), "%.3f  %.3f  %.3f", x, y, z);	
}

// Prints a string
void PrintString(char *str)
{
	sprintf(oapiDebugString(), "%s", str);	
}
