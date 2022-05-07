// Auto ReferenceClass For Orbiter

#ifndef __ORBIT_ReferenceClass_H
#define __ORBIT_ReferenceClass_H

#include "Orbitersdk.h"
#include "ABMFD.h"

#define TOTAL_COUNT 512

class ReferenceClass {
	
public:

	ReferenceClass();
  ~ReferenceClass();

	OBJHANDLE  FindStar();
	OBJHANDLE  Get2ndReferenceForShip(VESSEL*);
	OBJHANDLE	 GetReference(OBJHANDLE);
	OBJHANDLE	 GetReferenceByName(char *);
	double		 GetSOI(OBJHANDLE obj);
	OBJHANDLE* GetSystemList(OBJHANDLE x);
	int			   GetSystemCount(OBJHANDLE x);
  bool		   IsGbody(OBJHANDLE x);

	OBJHANDLE	 StarHandle;

private:

	OBJHANDLE	FindGravityReference(OBJHANDLE);
	void		CreateDatabase();

	struct ReferenceClass_info {	
		OBJHANDLE	handle;
		OBJHANDLE	grf_handle;
		double		soi;
		double		dist;  // Distance where this ReferenceClass is active
		OBJHANDLE   system[128];
		int         sys_index;
	} References[TOTAL_COUNT];
};

extern class ReferenceClass *Refer;

#endif

