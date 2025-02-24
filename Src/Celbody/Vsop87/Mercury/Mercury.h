// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#ifndef __VSOP87_MERCURY
#define __VSOP87_MERCURY

#include "Vsop87.h"

// ======================================================================
// class Mercury: interface
// ======================================================================

class Mercury: public VSOPOBJ {
public:
	Mercury (OBJHANDLE hCBody);
	void clbkInit (FILEHANDLE cfg) override;
	int clbkEphemeris (double mjd, int req, double *ret) override;
	int clbkFastEphemeris (double simt, int req, double *ret) override;
};

#endif // !__VSOP87_MERCURY