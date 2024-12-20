// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#ifndef __VSOP87_VENUS
#define __VSOP87_VENUS

#include "Vsop87.h"

// ======================================================================
// class Venus: interface
// ======================================================================

class Venus: public VSOPOBJ {
public:
	Venus (OBJHANDLE hCBody);
	void clbkInit (FILEHANDLE cfg) override;
	int clbkEphemeris (double mjd, int req, double *ret) override;
	int clbkFastEphemeris (double simt, int req, double *ret) override;
};

#endif // !__VSOP87_VENUS