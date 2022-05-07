// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#ifndef __EARTHATM2006_H
#define __EARTHATM2006_H

#include "OrbiterAPI.h"
#include "CelBodyAPI.h"

// ======================================================================
// class EarthAtmosphere_2006
// Legacy atmospheric model
// ======================================================================

class EarthAtmosphere_2006: public ATMOSPHERE {
public:
	EarthAtmosphere_2006 (CELBODY2 *body): ATMOSPHERE (body) {}
	const char *clbkName () const override;
	bool clbkConstants (ATMCONST *atmc) const override;
	bool clbkParams (const PRM_IN *prm_in, PRM_OUT *prm) override;
};

#endif // !__EARTHATM2006_H