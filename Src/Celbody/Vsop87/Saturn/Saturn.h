// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#ifndef __VSOP87_SATURN
#define __VSOP87_SATURN

#include "Vsop87.h"

// ======================================================================
// class Saturn: interface
// ======================================================================

class Saturn: public VSOPOBJ {
public:
	Saturn (OBJHANDLE hCBody);
	void clbkInit (FILEHANDLE cfg) override;
	int clbkEphemeris (double mjd, int req, double *ret) override;
	int clbkFastEphemeris (double simt, int req, double *ret) override;
};

#endif // !__VSOP87_SATURN