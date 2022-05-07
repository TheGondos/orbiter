// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#ifndef __VSOP87_SUN
#define __VSOP87_SUN

#include "Vsop87.h"

// ======================================================================
// class Sun: interface
// ======================================================================

class Sun: public VSOPOBJ {
public:
	Sun (OBJHANDLE hCBody);
	void clbkInit (FILEHANDLE cfg) override;
	int clbkEphemeris (double mjd, int req, double *ret) override;
	int clbkFastEphemeris (double simt, int req, double *ret) override;
};

#endif // !__VSOP87_SUN