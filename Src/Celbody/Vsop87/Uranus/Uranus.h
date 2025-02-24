// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#ifndef __VSOP87_URANUS
#define __VSOP87_URANUS

#include "Vsop87.h"

// ======================================================================
// class Uranus: interface
// ======================================================================

class Uranus: public VSOPOBJ {
public:
	Uranus (OBJHANDLE hCBody);
	void clbkInit (FILEHANDLE cfg) override;
	int clbkEphemeris (double mjd, int req, double *ret) override;
	int clbkFastEphemeris (double simt, int req, double *ret) override;
};

#endif // !__VSOP87_URANUS