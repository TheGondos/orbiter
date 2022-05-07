// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#ifndef __EARTHATMNRLMSISE00
#define __EARTHATMNRLMSISE00

#include "OrbiterAPI.h"
#include "CelBodyAPI.h"

// ======================================================================
// class EarthAtmosphere_NRLMSISE00
// MSIS atmosphere model implementation
// ======================================================================

class EarthAtmosphere_NRLMSISE00: public ATMOSPHERE {
public:
	EarthAtmosphere_NRLMSISE00 (CELBODY2 *body);
	const char *clbkName () const override;
	bool clbkConstants (ATMCONST *atmc) const override;
	bool clbkParams (const PRM_IN *prm_in, PRM_OUT *prm) override;

private:
	int pmjd; // date of previous day-of-year calculation
	int doy;  // current day-of-year value
};

#endif // !__EARTHATMNRLMSISE00