// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#define STRICT 1
#define OAPI_IMPLEMENTATION

#include "Orbiter.h"
#include "Psys.h"
#include "Camera.h"
#include "Pane.h"
#include "Defpanel.h"
#include "Panel2D.h"
#include "Vessel.h"
#include "Select.h"
#include "Config.h"
#include "cmdline.h"
#include "Script.h"
#include "Util.h"
#include "Log.h"
#include "Mesh.h"
#include "MenuInfoBar.h"
#include "DlgLaunchpad.h"

#include "zlib.h"

#include "Orbitersdk.h"
#include <errno.h>
#include <sstream>
#include <filesystem>

using namespace std;

extern Orbiter *g_pOrbiter;
extern TimeData td;
extern PlanetarySystem *g_psys;
extern Camera *g_camera;
extern Pane *g_pane;
extern Vessel *g_focusobj;
extern bool g_bShowGrapple;
extern char DBG_MSG[256];

// ------------------------------------------------------------------------------
// API interface functions
// ------------------------------------------------------------------------------

DLLEXPORT void FormatValue (char *cbuf, int n, double f, int precision)
{
	char *s = FloatStr (f, precision);
	strncpy (cbuf, s, n);
}

DLLEXPORT int oapiGetOrbiterVersion ()
{
	return g_pOrbiter->GetVersion();
}

DLLEXPORT const char *oapiGetCmdLine ()
{
	return orbiter::CommandLine::Instance().CmdLine();
}

DLLEXPORT void oapiGetViewportSize (int *w, int *h, int *bpp)
{
	*w = g_pOrbiter->ViewW();
	*h = g_pOrbiter->ViewH();
	if (bpp)
		*bpp = g_pOrbiter->ViewBPP();
}

DLLEXPORT double oapiGetPanelScale ()
{
	return g_pOrbiter->Cfg()->CfgLogicPrm.PanelScale;
}

DLLEXPORT double oapiGetPanel2DScale ()
{
	Panel2D *p = g_pane->GetPanel2D();
	if (p) return p->GetActiveScale();
	else   return 1.0;
}

DLLEXPORT void oapiRegisterModule (oapi::Module *module)
{
	g_pOrbiter->register_module = module;
}

DLLEXPORT OBJHANDLE oapiGetObjectByName (const char *name)
{
	return (OBJHANDLE)(g_psys ? g_psys->GetObj (name, true) : 0);
}

DLLEXPORT OBJHANDLE oapiGetObjectByIndex (int index)
{
	return (OBJHANDLE)(g_psys ? g_psys->GetObj (index) : 0);
}

DLLEXPORT int oapiGetObjectCount ()
{
	return (g_psys ? g_psys->nObj() : 0);
}

DLLEXPORT int oapiGetObjectType (OBJHANDLE hObj)
{
	int type = hObj->Type();
	if(type == OBJTP_VESSEL) {
		// Try for vessel. Here we go through the list of current
		// vessels, to avoid deleted vessels (which may still cast ok)
		for (int i = 0; i < g_psys->nVessel(); i++) {
			if (hObj == g_psys->GetVessel(i))
				return OBJTP_VESSEL;
		}
		return OBJTP_INVALID;
	}
	return type;
}

DLLEXPORT const void *oapiGetObjectParam (OBJHANDLE hObj, int paramtype)
{
	switch (hObj->Type()) {
	case OBJTP_VESSEL:
		return ((Vessel*)hObj)->GetParam (paramtype);
	case OBJTP_PLANET:
		return ((Planet*)hObj)->GetParam (paramtype);
	default:
		return ((Body*)hObj)->GetParam (paramtype);
	}
}

DLLEXPORT OBJHANDLE oapiGetVesselByName (const char *name)
{
	return (OBJHANDLE)(g_psys ? g_psys->GetVessel (name, true) : 0);
}

DLLEXPORT OBJHANDLE oapiGetVesselByIndex (int index)
{
	if (!g_psys || index < 0 || index >= g_psys->nVessel()) return NULL;
	return (OBJHANDLE)g_psys->GetVessel (index);
}

DLLEXPORT int oapiGetVesselCount ()
{
	return (g_psys ? g_psys->nVessel() : 0);
}

DLLEXPORT bool oapiIsVessel (OBJHANDLE hVessel)
{
	return (g_psys ? g_psys->isVessel ((const Vessel*)hVessel) : false);
}

DLLEXPORT OBJHANDLE oapiGetStationByName (char *name)
{
	static bool bWarning = true;
	if (bWarning) {
		LogOut_Obsolete(__FUNCTION__);
		bWarning = false;
	}
	return 0;
}

DLLEXPORT OBJHANDLE oapiGetStationByIndex (int index)
{
	static bool bWarning = true;
	if (bWarning) {
		LogOut_Obsolete(__FUNCTION__);
		bWarning = false;
	}
	return 0;
}

DLLEXPORT int oapiGetStationCount ()
{
	static bool bWarning = true;
	if (bWarning) {
		LogOut_Obsolete(__FUNCTION__);
		bWarning = false;
	}
	return 0;
}

DLLEXPORT OBJHANDLE oapiGetGbodyByName (const char *name)
{
	return (g_psys ? g_psys->GetGravObj (name, true) : 0);
}

DLLEXPORT OBJHANDLE oapiGetGbodyByIndex (int index)
{
	return (g_psys ? g_psys->GetGravObj (index) : 0);
}

DLLEXPORT OBJHANDLE oapiGetGbodyParent (OBJHANDLE hBody)
{
	return (OBJHANDLE)((CelestialBody*)hBody)->Primary();
}

DLLEXPORT OBJHANDLE oapiGetGbodyChild (OBJHANDLE hBody, int index)
{
	CelestialBody* cbody = (CelestialBody*)hBody;
	if (index < cbody->nSecondary())
		return (OBJHANDLE)cbody->Secondary(index);
	else
		return NULL;
}

DLLEXPORT int oapiGetGbodyCount ()
{
	return (g_psys ? g_psys->nGrav() : 0);
}

DLLEXPORT OBJHANDLE oapiGetBaseByName (OBJHANDLE hPlanet, const char *name)
{
	Body *body = (Body*)hPlanet;
	return (OBJHANDLE)(body->Type() == OBJTP_PLANET ? ((Planet*)hPlanet)->GetBase (name, true) : 0);
}

DLLEXPORT OBJHANDLE oapiGetBaseByIndex (OBJHANDLE hPlanet, int index)
{
	Planet *p = (Planet*)hPlanet;
	if (index < 0 || index >= p->nBase()) return 0;
	return (OBJHANDLE)p->GetBase (index);
}

DLLEXPORT int oapiGetBaseCount (OBJHANDLE hPlanet)
{
	Body *body = (Body*)hPlanet;
	return (body->Type() == OBJTP_PLANET ? ((Planet*)body)->nBase() : 0);
}

DLLEXPORT OBJHANDLE oapiGetBasePlanet (OBJHANDLE hBase)
{
	return (OBJHANDLE)((Base*)hBase)->RefPlanet();
}

DLLEXPORT void oapiGetObjectName (OBJHANDLE hObj, char *name, int n)
{
	strncpy (name, ((Body*)hObj)->Name(), n);
}

DLLEXPORT OBJHANDLE oapiGetFocusObject ()
{
	return (OBJHANDLE)g_focusobj;
}

DLLEXPORT OBJHANDLE oapiSetFocusObject (OBJHANDLE hVessel)
{
	Body *body = (Body*)hVessel;
	if (body->Type() != OBJTP_VESSEL) return 0;
	return (OBJHANDLE)g_pOrbiter->SetFocusObject ((Vessel*)hVessel);
}

DLLEXPORT VESSEL *oapiGetVesselInterface (OBJHANDLE hVessel)
{
	if (((Body*)hVessel)->Type() != OBJTP_VESSEL) return 0;
	else return ((Vessel*)hVessel)->GetModuleInterface();
}

DLLEXPORT CELBODY *oapiGetCelbodyInterface (OBJHANDLE hBody)
{
	return ((CelestialBody*)hBody)->GetModuleInterface();
}

DLLEXPORT VESSEL *oapiGetFocusInterface ()
{
	return g_focusobj->GetModuleInterface();
}

DLLEXPORT OBJHANDLE oapiCreateVessel (const char *name, const char *classname, const VESSELSTATUS &status)
{
	Vessel *vessel = new Vessel (g_psys, name, classname, status); TRACENEW
	g_pOrbiter->InsertVessel (vessel);
	return (OBJHANDLE)vessel;
}

DLLEXPORT OBJHANDLE oapiCreateVesselEx (const char *name, const char *classname, const void *status)
{
	Vessel *vessel = new Vessel (g_psys, name, classname, status); TRACENEW
	g_pOrbiter->InsertVessel (vessel);
	return (OBJHANDLE)vessel;
}

DLLEXPORT bool oapiDeleteVessel (OBJHANDLE hVessel, OBJHANDLE hAlternativeCameraTarget)
{
	Vessel *vessel = (Vessel*)hVessel;
	if (g_camera->Target() == vessel && hAlternativeCameraTarget)
		g_pOrbiter->SetView ((Body*)hAlternativeCameraTarget, 1);
	vessel->RequestDestruct();
	return true;
}

DLLEXPORT double oapiGetSize (OBJHANDLE hObj)
{
	return ((Body*)hObj)->Size();
}

DLLEXPORT double oapiGetMass (OBJHANDLE hObj)
{
	return ((Body*)hObj)->Mass();
}

DLLEXPORT double oapiGetEmptyMass (OBJHANDLE hVessel)
{
	return ((Vessel*)hVessel)->EmptyMass();
}

DLLEXPORT double oapiGetFuelMass (OBJHANDLE hVessel)
{
	return ((Vessel*)hVessel)->FuelMass();
}

DLLEXPORT double oapiGetMaxFuelMass (OBJHANDLE hVessel)
{
	return (((Vessel*)hVessel)->ntank ? ((Vessel*)hVessel)->tank[0]->maxmass : 0.0);
}

DLLEXPORT double oapiGetPropellantMaxMass (PROPELLANT_HANDLE ph)
{
	return ((TankSpec*)ph)->maxmass;
}

DLLEXPORT double oapiGetPropellantMass (PROPELLANT_HANDLE ph)
{
	return ((TankSpec*)ph)->mass;
}

DLLEXPORT PROPELLANT_HANDLE oapiGetPropellantHandle (OBJHANDLE hVessel, int idx)
{
	Vessel *vessel = (Vessel*)hVessel;
	return (idx < vessel->ntank ? (PROPELLANT_HANDLE)vessel->tank[idx] : 0);
}

DLLEXPORT DOCKHANDLE oapiGetDockHandle (OBJHANDLE hVessel, unsigned int n)
{
	Vessel *vessel = (Vessel*)hVessel;
	if (n < vessel->nDock()) return (DOCKHANDLE)vessel->GetDockParams(n);
	else return 0;
}

DLLEXPORT OBJHANDLE oapiGetDockStatus (DOCKHANDLE dock)
{
	return (OBJHANDLE)((PortSpec*)dock)->mate;
}

DLLEXPORT void oapiSetEmptyMass (OBJHANDLE hVessel, double mass)
{
	((Vessel*)hVessel)->SetEmptyMass (mass);
}

DLLEXPORT void oapiGetGlobalPos (OBJHANDLE hObj, VECTOR3 *pos)
{
	Vector gp (((Body*)hObj)->GPos());
	pos->x = gp.x, pos->y = gp.y, pos->z = gp.z;
}

DLLEXPORT void oapiGetGlobalVel (OBJHANDLE hObj, VECTOR3 *vel)
{
	Vector gv (((Body*)hObj)->GVel());
	vel->x = gv.x, vel->y = gv.y, vel->z = gv.z;
}

DLLEXPORT void oapiGetFocusGlobalPos (VECTOR3 *pos)
{
	Vector gp (g_focusobj->GPos());
	pos->x = gp.x, pos->y = gp.y, pos->z = gp.z;
}

DLLEXPORT void oapiGetFocusGlobalVel (VECTOR3 *vel)
{
	Vector gv (g_focusobj->GVel());
	vel->x = gv.x, vel->y = gv.y, vel->z = gv.z;
}

DLLEXPORT void oapiGetRelativePos (OBJHANDLE hObj, OBJHANDLE hRef, VECTOR3 *pos)
{
	Vector dp (((Body*)hObj)->GPos()-((Body*)hRef)->GPos());
	pos->x = dp.x, pos->y = dp.y, pos->z = dp.z;
}

DLLEXPORT void oapiGetRelativeVel (OBJHANDLE hObj, OBJHANDLE hRef, VECTOR3 *vel)
{
	Vector dv (((Body*)hObj)->GVel()-((Body*)hRef)->GVel());
	vel->x = dv.x, vel->y = dv.y, vel->z = dv.z;
}

DLLEXPORT void oapiGetFocusRelativePos (OBJHANDLE hRef, VECTOR3 *pos)
{
	Vector dp (g_focusobj->GPos()-((Body*)hRef)->GPos());
	pos->x = dp.x, pos->y = dp.y, pos->z = dp.z;
}

DLLEXPORT void oapiGetFocusRelativeVel (OBJHANDLE hRef, VECTOR3 *vel)
{
	Vector dv (g_focusobj->GVel()-((Body*)hRef)->GVel());
	vel->x = dv.x, vel->y = dv.y, vel->z = dv.z;
}

DLLEXPORT void oapiGetBarycentre (OBJHANDLE hObj, VECTOR3 *bary)
{
	Vector b(((CelestialBody*)hObj)->Barycentre());
	bary->x = b.x, bary->y = b.y, bary->z = b.z;
}

DLLEXPORT void oapiGetRotationMatrix (OBJHANDLE hObj, MATRIX3 *mat)
{
	const Matrix &m = ((Body*)hObj)->GRot();
	for (int i = 0; i < 9; i++) mat->data[i] = m.data[i];
}

DLLEXPORT void oapiGlobalToLocal (OBJHANDLE hObj, const VECTOR3 *glob, VECTOR3 *loc)
{
	Vector vloc;
	((Body*)hObj)->GlobalToLocal (Vector(glob->x, glob->y, glob->z), vloc);
	loc->x = vloc.x, loc->y = vloc.y, loc->z = vloc.z;
}

DLLEXPORT void oapiLocalToGlobal (OBJHANDLE hObj, const VECTOR3 *loc, VECTOR3 *glob)
{
	Vector vglob;
	((Body*)hObj)->LocalToGlobal (Vector(loc->x, loc->y, loc->z), vglob);
	glob->x = vglob.x, glob->y = vglob.y, glob->z = vglob.z;
}

DLLEXPORT void oapiEquToLocal (OBJHANDLE hObj, double lng, double lat, double rad, VECTOR3 *loc)
{
	Vector vloc;
	((Body*)hObj)->EquatorialToLocal (lng, lat, rad, vloc);
	loc->x = vloc.x, loc->y = vloc.y, loc->z = vloc.z;
}

DLLEXPORT void oapiLocalToEqu (OBJHANDLE hObj, const VECTOR3 &loc, double *lng, double *lat, double *rad)
{
	Vector vloc (loc.x, loc.y, loc.z);
	((Body*)hObj)->LocalToEquatorial (vloc, *lng, *lat, *rad);
}

DLLEXPORT void oapiEquToGlobal (OBJHANDLE hObj, double lng, double lat, double rad, VECTOR3 *glob)
{
	Vector vglob;
	((Body*)hObj)->EquatorialToGlobal (lng, lat, rad, vglob);
	glob->x = vglob.x, glob->y = vglob.y, glob->z = vglob.z;
}

DLLEXPORT void oapiGlobalToEqu (OBJHANDLE hObj, const VECTOR3 &glob, double *lng, double *lat, double *rad)
{
	Vector vglob (glob.x, glob.y, glob.z);
	((Body*)hObj)->GlobalToEquatorial (vglob, *lng, *lat, *rad);
}

DLLEXPORT double oapiOrthodome (double lng1, double lat1, double lng2, double lat2)
{
	return Orthodome (lng1, lat1, lng2, lat2);
}

DLLEXPORT bool oapiGetAltitude (OBJHANDLE hVessel, double *alt)
{
	const SurfParam *sp = ((Vessel*)hVessel)->GetSurfParam();
	if (sp) {
		*alt = sp->alt0;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetAltitude (OBJHANDLE hVessel, AltitudeMode mode, double *alt)
{
	const SurfParam *sp = ((Vessel*)hVessel)->GetSurfParam();
	if (sp) {
		*alt = (mode == ALTMODE_MEANRAD ? sp->alt0 : sp->alt);
		return true;
	} else {
		return false;
	}	
}

DLLEXPORT bool oapiGetPitch (OBJHANDLE hVessel, double *pitch)
{
	const SurfParam *sp = ((Vessel*)hVessel)->GetSurfParam();
	if (sp) {
		*pitch = sp->pitch;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetBank (OBJHANDLE hVessel, double *bank)
{
	const SurfParam *sp = ((Vessel*)hVessel)->GetSurfParam();
	if (sp) {
		*bank = sp->bank;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetHeading (OBJHANDLE hVessel, double *heading)
{
	const SurfParam *sp = ((Vessel*)hVessel)->GetSurfParam();
	if (sp) {
		*heading = sp->dir;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetEquPos (OBJHANDLE hVessel, double *longitude, double *latitude, double *radius)
{
	const SurfParam *sp = ((Vessel*)hVessel)->GetSurfParam();
	if (sp) {
		*longitude = sp->lng;
		*latitude  = sp->lat;
		*radius    = sp->rad;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetFocusAltitude (double *alt)
{
	const SurfParam *sp = g_focusobj->GetSurfParam();
	if (sp) {
		*alt = sp->alt;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetFocusPitch (double *pitch)
{
	const SurfParam *sp = g_focusobj->GetSurfParam();
	if (sp) {
		*pitch = sp->pitch;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetFocusBank (double *bank)
{
	const SurfParam *sp = g_focusobj->GetSurfParam();
	if (sp) {
		*bank = sp->bank;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetFocusHeading (double *heading)
{
	const SurfParam *sp = g_focusobj->GetSurfParam();
	if (sp) {
		*heading = sp->dir;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetFocusEquPos (double *longitude, double *latitude, double *radius)
{
	const SurfParam *sp = g_focusobj->GetSurfParam();
	if (sp) {
		*longitude = sp->lng;
		*latitude  = sp->lat;
		*radius    = sp->rad;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetGroundspeed (OBJHANDLE hVessel, double *groundspeed)
{
	Vessel *v = (hVessel ? (Vessel*)hVessel : g_focusobj);
	const SurfParam *sp = v->GetSurfParam();
	if (sp) {
		*groundspeed = sp->groundspd;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetGroundspeedVector (OBJHANDLE hVessel, REFFRAME frame, VECTOR3 *vel)
{
	Vessel *v = (hVessel ? (Vessel*)hVessel : g_focusobj);
	const SurfParam *sp = v->GetSurfParam();
	if (sp) {
		switch (frame) {
		case FRAME_GLOBAL:
			vel->x = sp->groundvel_glob.x, vel->y = sp->groundvel_glob.y, vel->z = sp->groundvel_glob.z;
			return true;
		case FRAME_LOCAL:
			vel->x = sp->groundvel_ship.x, vel->y = sp->groundvel_ship.y, vel->z = sp->groundvel_ship.z;
			return true;
		case FRAME_REFLOCAL: {
			Vector hvel (tmul (sp->ref->GRot(), sp->groundvel_glob));
			vel->x = hvel.x, vel->y = hvel.y, vel->z = hvel.z;
			} return true;
		case FRAME_HORIZON: {
			Vector hvel (tmul (sp->ref->GRot(), sp->groundvel_glob));
			hvel.Set (mul (sp->L2H, hvel));
			vel->x = hvel.x, vel->y = hvel.y, vel->z = hvel.z;
			} return true;
		default:
			vel->x = vel->y = vel->z = 0.0;
			return false;
		}
	} else {
		vel->x = vel->y = vel->z = 0.0;
		return false;
	}
}

DLLEXPORT bool oapiGetAirspeed (OBJHANDLE hVessel, double *airspeed)
{
	Vessel *v = (hVessel ? (Vessel*)hVessel : g_focusobj);
	const SurfParam *sp = v->GetSurfParam();
	if (sp) {
		*airspeed = sp->airspd;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetAirspeedVector (OBJHANDLE hVessel, REFFRAME frame, VECTOR3 *vel)
{
	Vessel *v = (hVessel ? (Vessel*)hVessel : g_focusobj);
	const SurfParam *sp = v->GetSurfParam();
	if (sp) {
		switch (frame) {
		case FRAME_GLOBAL:
			vel->x = sp->airvel_glob.x, vel->y = sp->airvel_glob.y, vel->z = sp->airvel_glob.z;
			return true;
		case FRAME_LOCAL:
			vel->x = sp->airvel_ship.x, vel->y = sp->airvel_ship.y, vel->z = sp->airvel_ship.z;
			return true;
		case FRAME_REFLOCAL: {
			Vector hvel (tmul (sp->ref->GRot(), sp->airvel_glob));
			vel->x = hvel.x, vel->y = hvel.y, vel->z = hvel.z;
			} return true;
		case FRAME_HORIZON: {
			Vector hvel (tmul (sp->ref->GRot(), sp->airvel_glob));
			hvel.Set (mul (sp->L2H, hvel));
			vel->x = hvel.x, vel->y = hvel.y, vel->z = hvel.z;
			} return true;
		default:
			vel->x = vel->y = vel->z = 0.0;
			return false;
		}
	} else {
		vel->x = vel->y = vel->z = 0.0;
		return false;
	}
}

DLLEXPORT bool oapiGetAirspeedVector (OBJHANDLE hVessel, VECTOR3 *speedvec)
{
	LOGOUT_OBSOLETE;
	const SurfParam *sp = ((Vessel*)hVessel)->GetSurfParam();
	if (sp) {
		Vector hvel (tmul (sp->ref->GRot(), sp->airvel_glob));
		hvel.Set (mul (sp->L2H, hvel));
		speedvec->x = hvel.x, speedvec->y = hvel.y, speedvec->z = hvel.z;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetShipAirspeedVector (OBJHANDLE hVessel, VECTOR3 *speedvec)
{
	LOGOUT_OBSOLETE;
	const SurfParam *sp = ((Vessel*)hVessel)->GetSurfParam();
	if (sp) {
		speedvec->x = sp->airvel_ship.x, speedvec->y = sp->airvel_ship.y, speedvec->z = sp->airvel_ship.z;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetFocusAirspeed (double *airspeed)
{
	LOGOUT_OBSOLETE;
	const SurfParam *sp = g_focusobj->GetSurfParam();
	if (sp) {
		*airspeed = sp->airspd;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetFocusAirspeedVector (VECTOR3 *speedvec)
{
	LOGOUT_OBSOLETE;
	const SurfParam *sp = g_focusobj->GetSurfParam();
	if (sp) {
		Vector hvel (tmul (sp->ref->GRot(), sp->airvel_glob));
		hvel.Set (mul (sp->L2H, hvel));
		speedvec->x = hvel.x, speedvec->y = hvel.y, speedvec->z = hvel.z;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT bool oapiGetFocusShipAirspeedVector (VECTOR3 *speedvec)
{
	LOGOUT_OBSOLETE;
	const SurfParam *sp = g_focusobj->GetSurfParam();
	if (sp) {
		speedvec->x = sp->airvel_ship.x, speedvec->y = sp->airvel_ship.y, speedvec->z = sp->airvel_ship.z;
		return true;
	} else {
		return false;
	}
}

DLLEXPORT void oapiGetAtm (OBJHANDLE hVessel, ATMPARAM *prm, OBJHANDLE *hAtmRef)
{
	Vessel *v = (hVessel ? (Vessel*)hVessel : g_focusobj);
	bool bAtm;

	if (!(bAtm = v->AtmPressureAndDensity (prm->p, prm->rho)))
		prm->p = prm->rho = 0.0;
	if (!v->AtmTemperature (prm->T))
		prm->T = 0.0;

	if (hAtmRef)
		*hAtmRef = (bAtm ? (OBJHANDLE)v->ProxyBody() : NULL);
}

DLLEXPORT void oapiGetAtmPressureDensity (OBJHANDLE hVessel, double *pressure, double *density)
{
	static bool bWarning = true;
	if (bWarning) {
		LogOut_Obsolete(__FUNCTION__);
		bWarning = false;
	}
	if (!((Vessel*)hVessel)->AtmPressureAndDensity (*pressure, *density))
		*pressure = *density = 0.0;
}

DLLEXPORT void oapiGetFocusAtmPressureDensity (double *pressure, double *density)
{
	static bool bWarning = true;
	if (bWarning) {
		LogOut_Obsolete(__FUNCTION__);
		bWarning = false;
	}
	if (!g_focusobj->AtmPressureAndDensity (*pressure, *density))
		*pressure = *density = 0.0;
}

DLLEXPORT VECTOR3 oapiGetGroundVector (OBJHANDLE hPlanet, double lng, double lat, int frame)
{
	return MakeVECTOR3 (((Planet*)hPlanet)->GroundVelocity (lng, lat, 0, frame));
}

DLLEXPORT VECTOR3 oapiGetWindVector (OBJHANDLE hPlanet, double lng, double lat, double alt, int frame, double *windspeed)
{
	return MakeVECTOR3 (((Planet*)hPlanet)->WindVelocity (lng, lat, alt, frame, 0, windspeed));
}

// ===========================================================================
// Aerodynamics helper functions

DLLEXPORT double oapiGetInducedDrag (double cl, double A, double eps)
{
	return (cl*cl)/(Pi*A*eps);
}

DLLEXPORT double oapiGetWaveDrag (double M, double M1, double M2, double M3, double cmax)
{
	if (M < M1) return 0.0;
	if (M < M2) return cmax * (M-M1)/(M2-M1);
	if (M < M3) return cmax;
	return cmax * sqrt ((M3*M3-1.0)/(M*M-1.0));
}

// ===========================================================================

DLLEXPORT void oapiGetEngineStatus (OBJHANDLE hVessel, ENGINESTATUS *es)
{
	((Vessel*)hVessel)->EngineStatus (es);
}

DLLEXPORT void oapiGetFocusEngineStatus (ENGINESTATUS *es)
{
	g_focusobj->EngineStatus (es);
}

DLLEXPORT void oapiSetEngineLevel (OBJHANDLE hVessel, ENGINETYPE engine, double level)
{
	if (((Body*)hVessel)->Type() == OBJTP_VESSEL) {
		Vessel *vessel = (Vessel*)hVessel;
		switch (engine) {
		case ENGINE_MAIN:
			//vessel->SetThrustMain (level);
			vessel->SetThrusterGroupLevel (THGROUP_MAIN, level);
			break;
		case ENGINE_RETRO:
			//vessel->SetThrustMain (-level);
			vessel->SetThrusterGroupLevel (THGROUP_RETRO, level);
			break;
		case ENGINE_HOVER:
			//vessel->SetThrustHover (level);
			vessel->SetThrusterGroupLevel (THGROUP_HOVER, level);
			break;
		case ENGINE_ATTITUDE: break;
		}
	}
}

DLLEXPORT SURFHANDLE oapiRegisterExhaustTexture (const char *name)
{
	return g_pOrbiter->RegisterExhaustTexture (name);
}

DLLEXPORT SURFHANDLE oapiRegisterReentryTexture (const char *name)
{
	return g_pOrbiter->RegisterExhaustTexture (name);
}

DLLEXPORT SURFHANDLE oapiRegisterParticleTexture (const char *name)
{
	return g_pOrbiter->RegisterExhaustTexture (name);
}

DLLEXPORT int oapiGetAttitudeMode (OBJHANDLE hVessel)
{
	return ((Vessel*)hVessel)->AttMode();
}

DLLEXPORT int oapiToggleAttitudeMode (OBJHANDLE hVessel)
{
	return ((Vessel*)hVessel)->ToggleAttMode();
}

DLLEXPORT bool oapiSetAttitudeMode (OBJHANDLE hVessel, int mode)
{
	return ((Vessel*)hVessel)->SetAttMode (mode);
}

DLLEXPORT int oapiGetFocusAttitudeMode ()
{
	return g_focusobj->AttMode();
}

DLLEXPORT int oapiToggleFocusAttitudeMode ()
{
	return g_focusobj->ToggleAttMode();
}

DLLEXPORT bool oapiSetFocusAttitudeMode (int mode)
{
	return g_focusobj->SetAttMode (mode);
}

DLLEXPORT void oapiSetShowGrapplePoints (bool show)
{
	g_bShowGrapple = show;
}

DLLEXPORT bool oapiGetShowGrapplePoints ()
{
	return g_bShowGrapple;
}

DLLEXPORT double oapiGetSimTime ()
{
	return td.SimT0;
}

DLLEXPORT double oapiGetSimStep ()
{
	return td.SimDT;
}

DLLEXPORT double oapiGetSysTime ()
{
	return td.SysT0;
}

DLLEXPORT double oapiGetSysStep ()
{
	return td.SysDT;
}

DLLEXPORT double oapiGetSimMJD ()
{
	return td.MJD0;
}

DLLEXPORT double oapiGetSysMJD ()
{
	return MJD (time (NULL)) + UTC_CT_diff*day;
}

DLLEXPORT bool oapiSetSimMJD (double mjd, int pmode)
{
	return g_pOrbiter->Timejump (mjd, pmode);
}

DLLEXPORT double oapiGetFrameRate ()
{
	return td.FPS();
}

DLLEXPORT double oapiTime2MJD (double t)
{
	return td.MJD_ref + Day(t);
}

DLLEXPORT double oapiGetTimeAcceleration ()
{
	return td.Warp();
}

DLLEXPORT void oapiSetTimeAcceleration (double warp)
{
	g_pOrbiter->SetWarpFactor (warp);
}

DLLEXPORT bool oapiGetPause ()
{
	return !g_pOrbiter->IsRunning();
}

DLLEXPORT void oapiSetPause (bool pause)
{
	g_pOrbiter->Pause (pause == true);
}

// Camera functions

DLLEXPORT bool oapiCameraInternal ()
{
	return g_camera->IsInternal();
}

DLLEXPORT OBJHANDLE oapiCameraTarget ()
{
	return (OBJHANDLE)g_camera->Target();
}

DLLEXPORT OBJHANDLE oapiCameraProxyGbody ()
{
	return (OBJHANDLE)g_camera->ProxyPlanet();
}

DLLEXPORT void oapiCameraGlobalPos (VECTOR3 *gpos)
{
	const Vector *gp = g_camera->GPosPtr();
	gpos->x = gp->x;
	gpos->y = gp->y;
	gpos->z = gp->z;
}

DLLEXPORT void oapiCameraGlobalDir (VECTOR3 *gdir)
{
	const Vector gd = g_camera->Direction();
	gdir->x = gd.x;
	gdir->y = gd.y;
	gdir->z = gd.z;
}

DLLEXPORT void oapiCameraRotationMatrix (MATRIX3 *rmat)
{
	const Matrix &m = g_camera->GRot();
	for (int i = 0; i < 9; i++) rmat->data[i] = m.data[i];

}

DLLEXPORT double oapiCameraTargetDist ()
{
	return g_camera->Distance ();
}

DLLEXPORT double oapiCameraAzimuth ()
{
	return g_camera->Phi();
}

DLLEXPORT double oapiCameraPolar ()
{
	return g_camera->Theta();
}

DLLEXPORT double oapiCameraAperture ()
{
	return g_camera->Aperture();
}

DLLEXPORT void oapiCameraSetAperture (double aperture)
{
	g_pOrbiter->SetFOV (min (max (aperture, RAD*0.1), RAD*80.0), false);
}

DLLEXPORT void oapiCameraScaleDist (double dscale)
{
	g_camera->ChangeDist (dscale);
}

DLLEXPORT void oapiCameraRotAzimuth (double dazimuth)
{
	g_camera->AddPhi (dazimuth);
}

DLLEXPORT void oapiCameraRotPolar (double dpolar)
{
	g_camera->AddTheta (dpolar);
}

DLLEXPORT void oapiCameraSetCockpitDir (double polar, double azimuth, bool transition)
{
	g_camera->SetCockpitDir (polar, azimuth);
	// this is always set relative to current default camera direction
}

DLLEXPORT void oapiCameraAttach (OBJHANDLE hObj, int mode)
{
	g_pOrbiter->SetView ((Body*)hObj, mode);
}

DLLEXPORT bool oapiSetCameraMode (const CameraMode &mode)
{
	g_camera->SetCMode (&mode);
	return true;
}

DLLEXPORT bool oapiMoveGroundCamera (double forward, double right, double up)
{
	if (g_camera->GetMode() != CAM_GROUNDOBSERVER) return false;
	g_camera->GroundObserverShift(right,forward,up);
	return true;
}

DLLEXPORT int oapiCameraMode ()
{
	return g_camera->GetMode ();
}

DLLEXPORT int oapiCockpitMode ()
{
	return g_pane->GetPanelMode();
}

// Functions for planetary bodies

DLLEXPORT double oapiGetPlanetPeriod (OBJHANDLE hPlanet)
{
	return ((Planet*)hPlanet)->RotT();
}

DLLEXPORT double oapiGetPlanetObliquity (OBJHANDLE hPlanet)
{
	return ((Planet*)hPlanet)->Obliquity();
}

DLLEXPORT double oapiGetPlanetTheta (OBJHANDLE hPlanet)
{
	return ((Planet*)hPlanet)->EqLng();
}

DLLEXPORT void oapiGetPlanetObliquityMatrix (OBJHANDLE hPlanet, MATRIX3 *mat)
{
	const Matrix m = ((CelestialBody*)hPlanet)->RotObliq();
	for (int i = 0; i < 9; i++) mat->data[i] = m.data[i];
}

DLLEXPORT double oapiGetPlanetCurrentRotation (OBJHANDLE hPlanet)
{
	return ((CelestialBody*)hPlanet)->Rotation();
}

DLLEXPORT bool oapiPlanetHasAtmosphere (OBJHANDLE hPlanet)
{
	return ((Planet*)hPlanet)->HasAtmosphere();
}

DLLEXPORT void oapiGetPlanetAtmParams (OBJHANDLE hPlanet, double rad, ATMPARAM *prm)
{
	// This should operate directly on altitude
	Planet *planet = (Planet*)hPlanet;
	planet->GetAtmParam (rad - planet->Size(), 0, 0, prm);
}

DLLEXPORT void oapiGetPlanetAtmParams (OBJHANDLE hPlanet, double alt, double lng, double lat, ATMPARAM *prm)
{
	Planet *planet = (Planet*)hPlanet;
	planet->GetAtmParam (alt, lng, lat, prm);
}

DLLEXPORT const ATMCONST *oapiGetPlanetAtmConstants (OBJHANDLE hPlanet)
{
	return ((Planet*)hPlanet)->AtmParams ();
}

DLLEXPORT int oapiGetPlanetJCoeffCount (OBJHANDLE hPlanet)
{
	return ((CelestialBody*)hPlanet)->nJcoeff();
}

DLLEXPORT double oapiGetPlanetJCoeff (OBJHANDLE hPlanet, int n)
{
	CelestialBody *cb = (CelestialBody*)hPlanet;
	return (n < cb->nJcoeff() ? cb->Jcoeff(n) : 0.0);
}

// Elevation support interface
DLLEXPORT ELEVHANDLE oapiElevationManager (OBJHANDLE hPlanet)
{
	Body *body = (Body*)hPlanet;
	if (body->Type() != OBJTP_PLANET) return 0;
	Planet *planet = (Planet*)body;
	return (ELEVHANDLE)planet->ElevMgr();
}

DLLEXPORT double oapiSurfaceElevation (OBJHANDLE hPlanet, double lng, double lat)
{
	Body *body = (Body*)hPlanet;
	if (body->Type() != OBJTP_PLANET) return 0.0;
	Planet *planet = (Planet*)body;
	ElevationManager *emgr = planet->ElevMgr();
	return (emgr ? emgr->Elevation (lat, lng) : 0.0);
}

DLLEXPORT double oapiSurfaceElevationEx(OBJHANDLE hPlanet, double lng, double lat, int tgtlvl, std::vector<ElevationTile> *tilecache, VECTOR3 *nml, int *lvl)
{
	Body *body = (Body*)hPlanet;
	if (body->Type() != OBJTP_PLANET) return 0.0;
	Planet *planet = (Planet*)body;
	ElevationManager *emgr = planet->ElevMgr();
	Vector normal;
	if (nml)
		normal = MakeVector(*nml);
	double elev = (emgr ? emgr->Elevation(lat, lng, tgtlvl, tilecache, nml ? &normal : 0, lvl) : 0.0);
	if (nml)
		*nml = MakeVECTOR3(normal);
	return elev;
}

DLLEXPORT std::vector<ElevationTile> *InitTileCache(int size)
{
	return new std::vector<ElevationTile>(size);
}

DLLEXPORT void ReleaseTileCache(std::vector<ElevationTile> *tilecache)
{
	delete tilecache;
}

// Surface base interface

DLLEXPORT void oapiGetBaseEquPos (OBJHANDLE hBase, double *lng, double *lat, double *rad)
{
	Base *base = (Base*)hBase;
	base->EquPos (*lng, *lat);
	if (rad) *rad = base->RefPlanet()->Size(); // for now
}

DLLEXPORT int oapiGetBasePadCount (OBJHANDLE hBase)
{
	return ((Base*)hBase)->nPad();
}

DLLEXPORT int oapiGetBaseRwyCount (OBJHANDLE hBase)
{
	return ((Base*)hBase)->nRwy();
}

DLLEXPORT bool oapiGetBaseRwyEquPos (OBJHANDLE hBase, int rwy, int endpoint, double *dir, double *lng, double *lat, double *rad) {
	Base *base = (Base*)hBase;
	if (rwy >= base->nRwy()) return false;
	base->Rwy_EquPos (rwy, endpoint, *lng, *lat, *dir);
	if (rad) *rad = base->RefPlanet()->Size(); // for now
	return true;
}

DLLEXPORT bool oapiGetBasePadEquPos (OBJHANDLE hBase, int pad, double *lng, double *lat, double *rad)
{
	Base *base = (Base*)hBase;
	if (pad >= base->nPad()) return false;
	base->Pad_EquPos (pad, *lng, *lat);
	if (rad) *rad = base->RefPlanet()->Size(); // for now
	return true;
}

DLLEXPORT bool oapiGetBasePadStatus (OBJHANDLE hBase, int pad, int *status)
{
	Base *base = (Base*)hBase;
	if (pad >= base->nPad()) return false;
	*status = base->PadStatus (pad)->status;
	return true;
}

DLLEXPORT NAVHANDLE oapiGetBasePadNav (OBJHANDLE hBase, int pad)
{
	Base *base = (Base*)hBase;
	if (pad >= base->nPad()) return 0;
	return (NAVHANDLE)base->PadStatus (pad)->nav;
}

// Navigation radio transmitter functions

DLLEXPORT void oapiGetNavPos (NAVHANDLE hNav, VECTOR3 *gpos)
{
	Vector p;
	((Nav*)hNav)->GPos (p);
	gpos->x = p.x;
	gpos->y = p.y;
	gpos->z = p.z;
}

DLLEXPORT int oapiGetNavChannel (NAVHANDLE hNav)
{
	return ((Nav*)hNav)->GetStep();
}

DLLEXPORT float oapiGetNavFreq (NAVHANDLE hNav)
{
	return ((Nav*)hNav)->GetFreq();
}

DLLEXPORT double oapiGetNavSignal (NAVHANDLE hNav, const VECTOR3 &gpos)
{
	return ((Nav*)hNav)->FieldStrength (MakeVector(gpos));
}

DLLEXPORT float oapiGetNavRange (NAVHANDLE hNav)
{
	return ((Nav*)hNav)->GetRange();
}

DLLEXPORT int oapiGetNavType (NAVHANDLE hNav)
{
	return (int)((Nav*)hNav)->Type();
}

DLLEXPORT int oapiGetNavData (NAVHANDLE hNav, NAVDATA *data)
{
	Nav *nav = (Nav*)hNav;
	nav->GetData (data);
	return 0;
}

DLLEXPORT int oapiGetNavDescr (NAVHANDLE hNav, char *descr, int maxlen)
{
	return ((Nav*)hNav)->IdString (descr, maxlen);
}

DLLEXPORT bool oapiNavInRange (NAVHANDLE hNav, const VECTOR3 &gpos)
{
	Vector p(gpos.x, gpos.y, gpos.z);
	return ((Nav*)hNav)->InRange (p);
}

DLLEXPORT INTERPRETERHANDLE oapiCreateInterpreter ()
{
	return g_pOrbiter->Script()->NewInterpreter();
}

DLLEXPORT int oapiDelInterpreter (INTERPRETERHANDLE hInterp)
{
	return g_pOrbiter->Script()->DelInterpreter (hInterp);
}

DLLEXPORT bool oapiExecScriptCmd (INTERPRETERHANDLE hInterp, const char *cmd)
{
	return g_pOrbiter->Script()->ExecScriptCmd (hInterp, cmd);
}

DLLEXPORT bool oapiAsyncScriptCmd (INTERPRETERHANDLE hInterp, const char *cmd)
{
	return g_pOrbiter->Script()->AsyncScriptCmd (hInterp, cmd);
}

DLLEXPORT lua_State *oapiGetLua (INTERPRETERHANDLE hInterp)
{
	return g_pOrbiter->Script()->GetLua (hInterp);
}

DLLEXPORT VISHANDLE *oapiObjectVisualPtr (OBJHANDLE hObject)
{
	return (VISHANDLE*)((Body*)hObject)->GetVishandlePtr();
}

DLLEXPORT MESHHANDLE oapiLoadMesh (const char *fname)
{
	ifstream ifs(g_pOrbiter->MeshPath(fname));
	Mesh *mesh = new Mesh; TRACENEW
	ifs >> *mesh;
	return (MESHHANDLE)mesh;
}

DLLEXPORT const MESHHANDLE oapiLoadMeshGlobal (const char *fname)
{
	return (const MESHHANDLE)g_pOrbiter->LoadMeshGlobal (fname);
}

DLLEXPORT const MESHHANDLE oapiLoadMeshGlobal (const char *fname, LoadMeshClbkFunc fClbk)
{
	return (const MESHHANDLE)g_pOrbiter->LoadMeshGlobal (fname, fClbk);
}

DLLEXPORT MESHHANDLE oapiCreateMesh (int ngrp, MESHGROUP *grp)
{
	Mesh *mesh = new Mesh;
	for (int i = 0; i < ngrp; i++) {
		mesh->AddGroup (grp[i].Vtx, grp[i].nVtx, grp[i].Idx, grp[i].nIdx, grp[i].MtrlIdx, grp[i].TexIdx, grp[i].zBias, 0, true);
	}
	return (MESHHANDLE)mesh;
}

DLLEXPORT void oapiDeleteMesh (MESHHANDLE hMesh)
{
	Mesh *mesh = (Mesh*)hMesh;
	delete mesh;
}

DLLEXPORT void oapiParticleSetLevelRef (PSTREAM_HANDLE ph, double *lvl)
{
	((oapi::ParticleStream*)ph)->SetLevelPtr (lvl);
}

DLLEXPORT int oapiMeshTextureCount (MESHHANDLE hMesh)
{
	return ((Mesh*)hMesh)->nTexture();
}

DLLEXPORT SURFHANDLE oapiGetTextureHandle (MESHHANDLE hMesh, int texidx)
{
	Mesh *mesh = (Mesh*)hMesh;
	return (mesh ? mesh->GetTexture (texidx-1) : 0);
}

DLLEXPORT SURFHANDLE oapiLoadTexture (const char *fname, bool dynamic)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) return gc->clbkLoadTexture (fname, 8 & (dynamic ? 3:0));
	else return NULL;
}

DLLEXPORT bool oapiGetTextureSize (SURFHANDLE hTex, int *w, int *h)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) return gc->clbkGetSurfaceSize (hTex, w, h);
	return false;
}

DLLEXPORT void oapiReleaseTexture (SURFHANDLE hTex)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) gc->clbkReleaseTexture (hTex);
}

DLLEXPORT bool oapiSetTexture (MESHHANDLE hMesh, int texidx, SURFHANDLE tex)
{
	Mesh *mesh = (Mesh*)hMesh;
	return mesh->SetTexture (texidx-1, tex);
}

DLLEXPORT bool oapiSetTexture (DEVMESHHANDLE hMesh, int texidx, SURFHANDLE tex)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	return (gc ? gc->clbkSetMeshTexture (hMesh, texidx, tex) : false);
}

DLLEXPORT void oapiIncrTextureRef (SURFHANDLE hTex)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) gc->clbkIncrSurfaceRef (hTex);
}


DLLEXPORT int oapiMeshGroupCount (MESHHANDLE hMesh)
{
	if (!hMesh) {
		static bool errecho = true;
		if (errecho) {
			LOGOUT_ERR("Invalid mesh handle. Returned 0. (Continuing)");
			errecho = false;
		}
		return 0;
	}
	return ((Mesh*)hMesh)->nGroup();
}

DLLEXPORT int oapiGetMeshFlags (MESHHANDLE hMesh)
{
	if (!hMesh) {
		static bool errecho = true;
		if (errecho) {
			LOGOUT_ERR("Invalid mesh handle. Returned NULL. (Continuing)");
			errecho = false;
		}
		return 0;
	}
	return ((Mesh*)hMesh)->GetFlags();
}

DLLEXPORT MESHGROUP *oapiMeshGroup (MESHHANDLE hMesh, int idx)
{
	if (!hMesh) {
		static bool errecho = true;
		if (errecho) {
			LOGOUT_ERR("Invalid mesh handle. Returned NULL. (Continuing)");
			errecho = false;
		}
		return 0;
	}
	return (MESHGROUP*)(((Mesh*)hMesh)->GetGroup (idx));
}

DLLEXPORT MESHGROUP *oapiMeshGroup (DEVMESHHANDLE hMesh, int idx)
{
	// Device-specific meshes don't allow direct access to group data
	return NULL;
}

DLLEXPORT MESHGROUPEX *oapiMeshGroupEx (MESHHANDLE hMesh, int idx)
{
	return (MESHGROUPEX*)(((Mesh*)hMesh)->GetGroup (idx));
}

DLLEXPORT int oapiAddMeshGroup (MESHHANDLE hMesh, MESHGROUP *grp)
{
	return (int)((Mesh*)hMesh)->AddGroup (grp->Vtx, grp->nVtx, grp->Idx, grp->nIdx,
		grp->MtrlIdx, grp->TexIdx, grp->zBias, 0, true);
}

DLLEXPORT bool oapiAddMeshGroupBlock (MESHHANDLE hMesh, int grpidx,
	const NTVERTEX *vtx, int nvtx, const uint16_t *idx, int nidx)
{
	return ((Mesh*)hMesh)->AddGroupBlock (grpidx, vtx, nvtx, idx, nidx);
}

DLLEXPORT int oapiGetMeshGroup (DEVMESHHANDLE hMesh, int grpidx, GROUPREQUESTSPEC *grs)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	return (gc ? gc->clbkGetMeshGroup (hMesh, grpidx, grs) : -1);
}

DLLEXPORT int oapiEditMeshGroup (MESHHANDLE hMesh, int grpidx, GROUPEDITSPEC *ges)
{
	return ((Mesh*)hMesh)->EditGroup (grpidx, ges);
}

DLLEXPORT int oapiEditMeshGroup (DEVMESHHANDLE hMesh, int grpidx, GROUPEDITSPEC *ges)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	return (gc ? gc->clbkEditMeshGroup (hMesh, grpidx, ges) : -1);
}

DLLEXPORT int oapiMeshMaterialCount (MESHHANDLE hMesh)
{
	return ((Mesh*)hMesh)->nMaterial();
}

DLLEXPORT MATERIAL *oapiMeshMaterial (MESHHANDLE hMesh, int idx)
{
	return (MATERIAL*)(((Mesh*)hMesh)->GetMaterial (idx));
}

DLLEXPORT int oapiMeshMaterial (DEVMESHHANDLE hMesh, int matidx, MATERIAL *mat)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	return (gc ? gc->clbkMeshMaterial (hMesh, matidx, mat) : 1);
}

DLLEXPORT int oapiSetMaterial (DEVMESHHANDLE hMesh, int matidx, const MATERIAL *mat)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	return (gc ? gc->clbkSetMeshMaterial (hMesh, matidx, mat) : 1);
}

DLLEXPORT int oapiAddMaterial (MESHHANDLE hMesh, MATERIAL *mat)
{
	MATERIAL *m = (MATERIAL*)mat;
	return ((Mesh*)hMesh)->AddMaterial (*m);
}

DLLEXPORT bool oapiDeleteMaterial (MESHHANDLE hMesh, int idx)
{
	return ((Mesh*)hMesh)->DeleteMaterial (idx);
}

DLLEXPORT bool oapiSetMeshProperty (MESHHANDLE hMesh, int property, int value)
{
	Mesh *mesh = (Mesh*)hMesh;
	switch (property) {
	case MESHPROPERTY_MODULATEMATALPHA:
		mesh->EnableMatAlpha (value != 0);
		return true;
	}
	return false;
}

DLLEXPORT bool oapiSetMeshProperty (DEVMESHHANDLE hMesh, int property, int value)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	return (gc ? gc->clbkSetMeshProperty (hMesh, property, value) : false);
}

DLLEXPORT bool oapiSetHUDMode (int mode)
{
	return (g_pane ? g_pane->SetHUDMode (mode) : false);
}

DLLEXPORT bool oapiSetHUDMode (int mode, const HUDPARAM *prm)
{
	return (g_pane ? g_pane->SetHUDMode (mode, prm) : false);
}

DLLEXPORT int oapiGetHUDMode ()
{
	return (g_pane ? g_pane->GetHUDMode () : 0);
}

DLLEXPORT int oapiGetHUDMode (HUDPARAM *prm)
{
	return (g_pane ? g_pane->GetHUDMode (prm) : 0);
}

DLLEXPORT void oapiToggleHUDColour ()
{
	if (g_pane) g_pane->ToggleHUDColour ();
}

DLLEXPORT double oapiGetHUDIntensity ()
{
	return (g_pane ? g_pane->HudIntens() : 0.0);
}

DLLEXPORT void oapiSetHUDIntensity (double val)
{
	if (g_pane) g_pane->SetHUDIntens (val);
}

DLLEXPORT void oapiIncHUDIntensity ()
{
	if (g_pane) g_pane->IncHUDIntens ();
}

DLLEXPORT void oapiDecHUDIntensity ()
{
	if (g_pane) g_pane->DecHUDIntens ();
}

DLLEXPORT void oapiRenderHUD (MESHHANDLE hMesh, SURFHANDLE *hTex)
{
	if (g_pane) g_pane->RenderCustomHUD (hMesh, hTex);
}

DLLEXPORT void oapiOpenMFD (int mode, MfdId mfd)
{
	if (g_pane) g_pane->OpenMFD (mfd, mode);
}

DLLEXPORT void oapiToggleMFD_on (MfdId mfd)
{
	if (g_pane) g_pane->ToggleMFD_on (mfd);
}

DLLEXPORT int oapiRegisterMFDMode (MFDMODESPEC &spec)
{
	LOGOUT_OBSOLETE;
	return Instrument::RegisterUserMode (&spec);
}

DLLEXPORT int oapiRegisterMFDMode (MFDMODESPECEX &spec)
{
	return Instrument::RegisterUserMode (&spec);
}

DLLEXPORT bool oapiUnregisterMFDMode (int mode)
{
	return Instrument::UnregisterUserMode (mode);
}

DLLEXPORT int oapiGetMFDMode (MfdId mfd)
{
	return (g_pane->MFD(mfd) ? g_pane->MFD(mfd)->Type() : MFD_NONE);
}

DLLEXPORT double oapiSetMFDRefreshIntervalMultiplier (int mfd, double multiplier)
{
	return g_pane->SetMFDRefreshIntervalMultiplier (mfd, multiplier);
}

DLLEXPORT int oapiBroadcastMFDMessage (int mfdmode, int msg, void *data)
{
	return g_pane->BroadcastMFDMessage (mfdmode, msg, data);
}

DLLEXPORT int oapiSendMFDKey (MfdId mfd, int key)
{
	return g_pane->MFDConsumeKeyBuffered (mfd, key);
}

DLLEXPORT void oapiRefreshMFDButtons (int mfd, OBJHANDLE hVessel)
{
	if (!hVessel || (Vessel*)hVessel == g_focusobj)
		g_focusobj->MFDchanged (mfd, MFD_REFRESHBUTTONS);
}

DLLEXPORT bool oapiProcessMFDButton (MfdId mfd, int bt, int event)
{
	Instrument *MFD = g_pane->MFD(mfd);
	if (MFD) return MFD->ConsumeButton (bt, event);
	else     return false;
}

DLLEXPORT const char *oapiMFDButtonLabel (MfdId mfd, int bt)
{
	return (g_pane->MFD(mfd) ? g_pane->MFD(mfd)->ButtonLabel(bt) : 0);
}

DLLEXPORT void oapiRegisterMFD (int mfd, const MFDSPEC &spec)
{
	g_pane->RegisterMFD (mfd, spec);
}

DLLEXPORT void oapiRegisterMFD (int mfd, const EXTMFDSPEC *spec)
{
	g_pane->RegisterMFD (mfd, spec);
}

DLLEXPORT void oapiRegisterExternMFD (ExternMFD *emfd, const MFDSPEC &spec)
{
	if (g_pane)
		g_pane->RegisterExternMFD (emfd, spec);
}

DLLEXPORT bool oapiUnregisterExternMFD (ExternMFD *emfd)
{
	return (g_pane ? g_pane->UnregisterExternMFD (emfd) : false);
}

DLLEXPORT void oapiDisableMFDMode (int mode)
{
	Instrument::DisableMode (mode);
	if (g_pane)
		g_pane->MFDModeDisabled (mode);
}

DLLEXPORT int oapiGetMFDModeSpecEx (char *name, MFDMODESPECEX **spec)
{
	return Instrument::ModeFromName (name, spec);
}

DLLEXPORT int oapiGetMFDModeSpec (char *name, MFDMODESPEC **spec)
{
	return Instrument::ModeFromNameOld (name, spec);
}

DLLEXPORT void oapiVCRegisterMFD (int mfd, const VCMFDSPEC *spec)
{
	g_pane->RegisterVCMFD (mfd, spec);
}

DLLEXPORT void oapiVCRegisterHUD (const VCHUDSPEC *spec)
{
	if (g_pane)
		g_pane->RegisterVCHUD (spec);
}

DLLEXPORT void oapiRegisterPanelBackground (SURFHANDLE surf, int flag, uint32_t ck)
{
	g_pane->RegisterPanelBackground (surf, flag, ck);
}

DLLEXPORT void oapiRegisterPanelArea (int id, const RECT &pos, int draw_event, int mouse_event, int bkmode)
{
	g_pane->RegisterPanelArea (id, pos, draw_event, mouse_event, bkmode);
}

DLLEXPORT void oapiSetPanelNeighbours (int left, int right, int top, int bottom)
{
	g_pane->SetPanelNeighbours (left, right, top, bottom);
}

DLLEXPORT void oapiVCSetNeighbours (int left, int right, int top, int bottom)
{
	g_pane->SetVCNeighbours (left, right, top, bottom);
}

DLLEXPORT void oapiTriggerRedrawArea (int panel_id, int vc_id, int area_id)
{
	static bool bwarn = true;
	if (bwarn) {
		LogOut_Obsolete (__FUNCTION__, "Replaced by VESSEL::TriggerRedrawArea");
		bwarn = false;
	}
	if (g_pane)
		g_pane->TriggerRedrawArea (panel_id, vc_id, area_id);
}

DLLEXPORT void oapiTriggerPanelRedrawArea (int panel_id, int area_id)
{
	static bool bwarn = true;
	if (bwarn) {
		LogOut_Obsolete (__FUNCTION__, "Replaced by VESSEL::TriggerPanelRedrawArea");
		bwarn = false;
	}
	if (g_pane)
		g_pane->TriggerPanelRedrawArea (panel_id, area_id);
}

DLLEXPORT bool oapiBltPanelAreaBackground (int area_id, SURFHANDLE surf)
{
	if (g_pane)
		return g_pane->BltPanelAreaBackground (area_id, surf);
	else
		return false;
}

DLLEXPORT void oapiSetDefNavDisplay (int mode)
{
	DefaultPanel *defpanel = g_pane->GetDefaultPanel();
	if (defpanel) defpanel->SetNavDisplayMode (mode);
}

DLLEXPORT void oapiSetDefRCSDisplay (int mode)
{
	DefaultPanel *defpanel = g_pane->GetDefaultPanel();
	if (defpanel) defpanel->SetRCSDisplayMode (mode);
}

DLLEXPORT int oapiSwitchPanel (int direction)
{
	return g_pane->SwitchPanel (direction);
}

DLLEXPORT int oapiSetPanel (int panel_id)
{
	return g_pane->SelectPanel (panel_id);
}

DLLEXPORT void oapiSetPanelBlink (VECTOR3 v[4])
{
	g_pane->SetPanel2DBlink (v);
}

DLLEXPORT void oapiVCTriggerRedrawArea (int vc_id, int area_id)
{
	g_pane->TriggerVCRedrawArea (vc_id, area_id);
}

DLLEXPORT void oapiVCRegisterArea (int id, const RECT &tgtrect, int draw_event, int mouse_event, int bkmode, SURFHANDLE tgt)
{
	g_pane->RegisterVCArea (id, tgtrect, draw_event, mouse_event, bkmode, tgt);
}

DLLEXPORT void oapiVCRegisterArea (int id, int draw_event, int mouse_event)
{
	g_pane->RegisterVCArea (id, _R(0,0,0,0), draw_event, mouse_event, PANEL_MAP_NONE, NULL);
}

DLLEXPORT void oapiVCSetAreaClickmode_Spherical (int id, const VECTOR3 &cnt, double rad)
{
	g_pane->SetVCAreaClickmode_Spherical (id, Vector(cnt.x, cnt.y, cnt.z), rad);
}

DLLEXPORT void oapiVCSetAreaClickmode_Quadrilateral (int id, const VECTOR3 &p1, const VECTOR3 &p2, const VECTOR3 &p3, const VECTOR3 &p4)
{
	g_pane->SetVCAreaClickmode_Quadrilateral (id, Vector(p1.x, p1.y, p1.z), Vector(p2.x,p2.y,p2.z), Vector(p3.x,p3.y,p3.z), Vector(p4.x,p4.y,p4.z));
}

DLLEXPORT oapi::Sketchpad *oapiGetSketchpad (SURFHANDLE surf, bool antialiased)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	oapi::Sketchpad *skp = NULL;
	if (gc) skp = gc->clbkGetSketchpad (surf, antialiased);
	return skp;
}

DLLEXPORT void oapiReleaseSketchpad (oapi::Sketchpad *skp)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc && skp) gc->clbkReleaseSketchpad (skp);
}

DLLEXPORT oapi::Font *oapiCreateFont (int height, bool prop, const char *face, FontStyle style, bool antialiased)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	oapi::Font *font = NULL;
	if (gc) font = gc->clbkCreateFont (height, prop, face, (oapi::Font::Style)style, 0, antialiased);
	return font;
}

DLLEXPORT oapi::Font *oapiCreateFont (int height, bool prop, const char *face, FontStyle style, int orientation, bool antialiased)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	oapi::Font *font = NULL;
	if (gc) font = gc->clbkCreateFont (height, prop, face, (oapi::Font::Style)style, orientation, antialiased);
	return font;
}

DLLEXPORT void oapiReleaseFont (oapi::Font *font)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) gc->clbkReleaseFont (font);
}

DLLEXPORT oapi::Pen *oapiCreatePen (int style, int width, uint32_t col)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) return gc->clbkCreatePen (style, width, col);
	else    return NULL;
}

DLLEXPORT void oapiReleasePen (oapi::Pen *pen)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) gc->clbkReleasePen (pen);
}

DLLEXPORT oapi::Brush *oapiCreateBrush (uint32_t col)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) return gc->clbkCreateBrush (col);
	else    return NULL;
}

DLLEXPORT void oapiReleaseBrush (oapi::Brush *brush)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) gc->clbkReleaseBrush (brush);
}

DLLEXPORT SURFHANDLE oapiCreateSurface (int width, int height)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	return (gc ? gc->clbkCreateSurface (width, height) : NULL);
}

DLLEXPORT void oapiUpdateSurface (SURFHANDLE surf, int x, int y, int w, int h, const unsigned char* data)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if(gc) gc->clbkUpdateSurface (surf, x, y, w, h, data);
}

DLLEXPORT void oapiSetSurfaceParam (SURFHANDLE surf, int param, int val)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if(gc) gc->clbkSetSurfaceParam (surf, param, val);
}

DLLEXPORT SURFHANDLE oapiCreateSurfaceEx (int width, int height, int attrib)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	SURFHANDLE surf = NULL;
	if (gc) {
		//int attrib = OAPISURFACE_RENDERTARGET | OAPISURFACE_GDI | OAPISURFACE_SKETCHPAD;
		surf = gc->clbkCreateSurfaceEx (width, height, attrib);
	}
	return surf;
}

DLLEXPORT SURFHANDLE oapiCreateTextureSurface (int width, int height)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	return (gc ? gc->clbkCreateTexture (width, height) : NULL);
}

DLLEXPORT void oapiDestroySurface (SURFHANDLE surf)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) gc->clbkReleaseSurface (surf);
}

DLLEXPORT void oapiClearSurface (SURFHANDLE surf, uint32_t col)
{
	if (!surf) return;
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) gc->clbkFillSurface (surf, col);
}

DLLEXPORT void oapiSetSurfaceColourKey (SURFHANDLE surf, uint32_t ck)
{
	if (!surf) return;
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) gc->clbkSetSurfaceColourKey (surf, ck);
}

DLLEXPORT void oapiClearSurfaceColourKey (SURFHANDLE surf)
{
	if (!surf) return;
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	//if (gc) gc->clbClearSurfaceColourKey (surf); // TODO
	fprintf(stderr, "oapiClearSurfaceColourKey: unimpl\n");
	assert(false);
	exit(-1);
//	((LPDIRECTDRAWSURFACE7)surf)->SetColorKey (DDCKEY_SRCBLT, 0);
}

DLLEXPORT uint32_t oapiGetColour (uint32_t red, uint32_t green, uint32_t blue)
{
	return GetSurfColour (red, green, blue);
}

DLLEXPORT void oapiBlt (SURFHANDLE tgt, SURFHANDLE src, int tgtx, int tgty, int srcx, int srcy, int w, int h, uint32_t ck)
{
	if (!src || !tgt) return;

	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) {

		if (ck != SURF_NO_CK) {
			static bool bWarnCK = true;
			if (bWarnCK) {
				printf ("%s %s\n",__FUNCTION__, "Colour key argument not supported by graphics client");
				bWarnCK = false;
			}
		}

		dASSERT (gc->clbkBlt (tgt, tgtx, tgty, src, srcx, srcy, w, h), "GraphicsClient::clbkBlt failed");
	}
}

DLLEXPORT void oapiBlt (SURFHANDLE tgt, SURFHANDLE src, RECT *tgtr, RECT *srcr, uint32_t ck, int rotation)
{
	if (!src || !tgt) return;

	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) {

		static bool bWarnRot = true;
		if (rotation != SURF_NO_ROTATION) {
			printf ("%s %s\n",__FUNCTION__, "Rotation argument no longer supported");
			bWarnRot = false;
		}

		if (ck != SURF_NO_CK) {
			static bool bWarnCK = true;
			if (bWarnCK) {
				printf ("%s %s\n",__FUNCTION__, "Colour key argument not supported by graphics client");
				bWarnCK = false;
			}
		}
		gc->clbkScaleBlt (tgt, tgtr->left, tgtr->top, tgtr->right-tgtr->left, tgtr->bottom-tgtr->top,
			src, srcr->left, srcr->top, srcr->right-srcr->left, srcr->bottom-srcr->top);
	}
}

DLLEXPORT int oapiBeginBltGroup (SURFHANDLE tgt)
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	return (gc ? gc->clbkBeginBltGroup (tgt) : -1);
}

DLLEXPORT int oapiEndBltGroup ()
{
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	return (gc ? gc->clbkEndBltGroup () : -1);
}

DLLEXPORT void oapiColourFill (SURFHANDLE tgt, uint32_t fillcolor, int tgtx, int tgty, int w, int h)
{
	if (!tgt) return;
	oapi::GraphicsClient *gc = g_pOrbiter->GetGraphicsClient();
	if (gc) {
		if (w && h)
			gc->clbkFillSurface (tgt, tgtx, tgty, w, h, fillcolor);
		else
			gc->clbkFillSurface (tgt, fillcolor);
	}
}

DLLEXPORT bool oapiAcceptDelayedKey (char key, double interval)
{
	static char lastkey = (char)255;
	static double lastkeytime = 0.0;

	if ((key == lastkey) && td.SysT0-lastkeytime < interval && td.SysT0 > lastkeytime)
		return false;
	lastkey = key;
	lastkeytime = td.SysT0;
	return true;
}

DLLEXPORT bool oapiRegisterLaunchpadItem (LaunchpadItem *item, const char *category) 
{
	return g_pOrbiter->m_DlgLaunchpad->RegisterExtraItem(item, category);
}

DLLEXPORT bool oapiUnregisterLaunchpadItem (LaunchpadItem *item)
{
	return g_pOrbiter->m_DlgLaunchpad->UnregisterExtraItem(item);
}

DLLEXPORT int oapiRegisterCustomCmd (const char *label, const char *desc, CustomFunc func, void *context)
{
	return g_pOrbiter->RegisterCustomCmd (label, desc, func, context);
}

DLLEXPORT bool oapiUnregisterCustomCmd (int cmdId)
{
	return g_pOrbiter->UnregisterCustomCmd (cmdId);
}

DLLEXPORT void oapiOpenDialog (GUIElement *e)
{
	g_pOrbiter->RegisterGUIElement (e);
	e->show = true;
}

DLLEXPORT void oapiCloseDialog (GUIElement *e)
{
	g_pOrbiter->UnregisterGUIElement (e);
}

DLLEXPORT void oapiAddNotification(int type, const char *title, const char *content) {
	enum GUIManager::NotifType t;
	switch(type) {
		case OAPINOTIF_SUCCESS:
			t = GUIManager::Success;
			break;
		case OAPINOTIF_WARNING:
			t = GUIManager::Warning;
			break;
		case OAPINOTIF_ERROR:
			t = GUIManager::Error;
			break;
		case OAPINOTIF_INFO:
		default:
			t = GUIManager::Info;
			break;
	}

	g_pOrbiter->m_pGUIManager->Notify(t, title, content);
}

DLLEXPORT int oapiGetMainMenuVisibilityMode()
{
	return g_pane->MIBar()->GetMenuMode();
}

DLLEXPORT void oapiSetMainMenuVisibilityMode (int mode)
{
	g_pane->MIBar()->SetMenuMode (mode);
}

DLLEXPORT int oapiGetMainInfoVisibilityMode()
{
	return g_pane->MIBar()->GetInfoMode();
}

DLLEXPORT void oapiSetMainInfoVisibilityMode (int mode)
{
	g_pane->MIBar()->SetInfoMode (mode);
}

DLLEXPORT FILEHANDLE oapiOpenFile (const char *fname, FileAccessMode mode, PathRoot root)
{
	char cbuf[512];
	switch (root) {
	case CONFIG:
		strcpy (cbuf, g_pOrbiter->Cfg()->ConfigPathNoext (fname));
		break;
	case SCENARIOS:
		strcpy (cbuf, g_pOrbiter->ScnPath (fname));
		break;
	case TEXTURES:
		strcpy (cbuf, g_pOrbiter->TexPath (fname));
		break;
	case TEXTURES2:
		strcpy (cbuf, g_pOrbiter->HTexPath (fname));
		break;
	case MESHES:
		strcpy (cbuf, g_pOrbiter->MeshPath (fname));
		break;
	default:
		strcpy (cbuf, fname);
		break;
	}

	switch (mode) {
	case FILE_IN:
		return (FILEHANDLE)(new ifstream (cbuf));
	case FILE_IN_ZEROONFAIL: {
		ifstream *ifs = new ifstream (cbuf);
		if (ifs->fail()) {
			delete ifs;
			ifs = 0;
		}
		return (FILEHANDLE)ifs;
		}
	case FILE_OUT:
		TRACENEW; return (FILEHANDLE)(new ofstream (cbuf));
	case FILE_APP:
		TRACENEW; return (FILEHANDLE)(new ofstream (cbuf, ios::app));
	}
	return 0;
}

DLLEXPORT void oapiCloseFile (FILEHANDLE file, FileAccessMode mode)
{
	if (file) {
		switch (mode) {
		case FILE_IN:
		case FILE_IN_ZEROONFAIL:
			delete (ifstream*)file;
			break;
		case FILE_OUT:
		case FILE_APP:
			delete (ofstream*)file;
			break;
		}
	}
}

DLLEXPORT bool oapiSaveScenario (const char *fname, const char *desc)
{
	return g_pOrbiter->SaveScenario (fname, desc);
}

DLLEXPORT void oapiWriteLine (FILEHANDLE file, const char *line)
{
	ofstream &ofs = *(ofstream*)file;
	ofs << line << endl;
}

DLLEXPORT void oapiWriteLog (const char *line)
{
	LOGOUT (line);
}

DLLEXPORT void oapiExitOrbiter(int code)
{
	printf("oapiExitOrbiter\n");
	exit(code);
}

DLLEXPORT void oapiWriteLogV (const char *format, ...)
{
#ifdef GENERATE_LOG
	va_list ap;
	va_start(ap, format);
	LogOutVA(format, ap);
	va_end(ap);
#endif
}

DLLEXPORT void __writeLogError(const char *func, const char *file, int line, const char *format, ...)
{
#ifdef GENERATE_LOG
	va_list ap;
	va_start(ap, format);
	LogOut_ErrorVA(func, file, line, format, ap);
	va_end(ap);
#endif
}

DLLEXPORT void oapiWriteScenario_string (FILEHANDLE file, const char *item, const char *string)
{
	ofstream &ofs = *(ofstream*)file;
	ofs << "  " << item << ' ' << string << endl;
}

DLLEXPORT void oapiWriteScenario_int (FILEHANDLE file, const char *item, int i)
{
	ofstream &ofs = *(ofstream*)file;
	ofs << "  " << item << ' ' << i << endl;
}

DLLEXPORT void oapiWriteScenario_float (FILEHANDLE file, const char *item, double d)
{
	ofstream &ofs = *(ofstream*)file;
	ofs << "  " << item << ' ' << d << endl;
}

DLLEXPORT void oapiWriteScenario_vec (FILEHANDLE file, const char *item, const VECTOR3 &vec)
{
	ofstream &ofs = *(ofstream*)file;
	ofs << "  " << item << ' ' << vec.x << ' ' << vec.y << ' ' << vec.z << endl;
}

DLLEXPORT bool oapiReadScenario_nextline (FILEHANDLE file, char *&line)
{
	ifstream &ifs = *(ifstream*)file;
	char *cbuf = readline(ifs);
	if (!cbuf) return false;
	line = trim_string (cbuf);
	if (!_stricmp (line, "END")) return false;
	return true;
}

DLLEXPORT void oapiWriteItem_string (FILEHANDLE file, const char *item, const char *string)
{
	ofstream &ofs = *(ofstream*)file;
	ofs << item << " = " << string << endl;
}

DLLEXPORT void oapiWriteItem_float (FILEHANDLE file, const char *item, double d)
{
	ofstream &ofs = *(ofstream*)file;
	ofs << item << " = " << d << endl;
}

DLLEXPORT void oapiWriteItem_int (FILEHANDLE file, const char *item, int i)
{
	ofstream &ofs = *(ofstream*)file;
	ofs << item << " = " << i << endl;
}

DLLEXPORT void oapiWriteItem_bool (FILEHANDLE file, const char *item, bool b)
{
	ofstream &ofs = *(ofstream*)file;
	ofs << item << " = " << (b ? "TRUE":"FALSE") << endl;
}

DLLEXPORT void oapiWriteItem_vec (FILEHANDLE file, const char *item, const VECTOR3 &vec)
{
	ofstream &ofs = *(ofstream*)file;
	ofs << item << " = " << vec.x << ' ' << vec.y << ' ' << vec.z << endl;
}

DLLEXPORT bool oapiReadItem_string (FILEHANDLE f, const char *item, char *string)
{
	return GetItemString (*(ifstream*)f, item, string);
}

DLLEXPORT bool oapiReadItem_float (FILEHANDLE f, const char *item, double &val)
{
	return GetItemReal (*(ifstream*)f, item, val);
}

DLLEXPORT bool oapiReadItem_int (FILEHANDLE f, const char *item, int &val)
{
	return GetItemInt (*(ifstream*)f, item, val);
}

DLLEXPORT bool oapiReadItem_bool (FILEHANDLE f, const char *item, bool &val)
{
	return GetItemBool (*(ifstream*)f, item, val);
}

DLLEXPORT bool oapiReadItem_vec (FILEHANDLE f, const char *item, VECTOR3 &val)
{
	Vector vec;
	bool res = GetItemVector (*(ifstream*)f, item, vec);
	val.x = vec.x;
	val.y = vec.y;
	val.z = vec.z;
	return res;
}

DLLEXPORT void oapiOpenInputBox (const char *title, bool (*Clbk)(void*,const char*,void*), const char *buf, int vislen, void *usrdata)
{
	InputBox *input = (InputBox *)g_pOrbiter->m_pGUIManager->GetCtrl<InputBox>();
	input->Open (title, buf, vislen, (InputBox::Callbk)Clbk, usrdata);
}

DLLEXPORT void oapiOpenInputBoxEx (const char *title, bool (*Clbk_enter)(void*,const char*,void*), bool (*Clbk_cancel)(void*,const char*,void*),
	char *buf, int vislen, void *usrdata, int flags)
{
	InputBox *input = (InputBox *)g_pOrbiter->m_pGUIManager->GetCtrl<InputBox>();
	input->OpenEx (title, buf, vislen, (InputBox::Callbk)Clbk_enter, (InputBox::Callbk)Clbk_cancel, usrdata, flags);
}

DLLEXPORT bool oapiSimulateBufferedKey (int key, int *mod, int nmod, bool onRunningOnly)
{
	return g_pOrbiter->SendKbdBuffered (key, mod, nmod, onRunningOnly);
}

DLLEXPORT bool oapiSimulateImmediateKey (char kstate[256], bool onRunningOnly)
{
	return g_pOrbiter->SendKbdImmediate (kstate, onRunningOnly);
}

DLLEXPORT NOTEHANDLE oapiCreateAnnotation (bool exclusive, double size, const VECTOR3 &col)
{
	COLORREF c = ((int)(col.z*255.99) << 16) + ((int)(col.y*255.99) << 8) + (int)(col.x*255.99);
	return (NOTEHANDLE)g_pOrbiter->CreateAnnotation (exclusive, size, c);
}

DLLEXPORT void oapiAnnotationSetPos (NOTEHANDLE hNote, double x1, double y1, double x2, double y2)
{
	if (hNote) {
		oapi::ScreenAnnotation *sn = (oapi::ScreenAnnotation*)hNote;
		sn->SetPosition (x1, y1, x2, y2);
	}
}

DLLEXPORT void oapiAnnotationSetSize (NOTEHANDLE hNote, double size)
{
	if (hNote) {
		oapi::ScreenAnnotation *sn = (oapi::ScreenAnnotation*)hNote;
		sn->SetSize (size);
	}
}

DLLEXPORT void oapiAnnotationSetColour (NOTEHANDLE hNote, const VECTOR3 &col)
{
	if (hNote) {
		oapi::ScreenAnnotation *sn = (oapi::ScreenAnnotation*)hNote;
		sn->SetColour (col);
	}
}

DLLEXPORT void oapiAnnotationSetText (NOTEHANDLE hNote, const char *note)
{
	if (hNote) {
		oapi::ScreenAnnotation *sn = (oapi::ScreenAnnotation*)hNote;
		if (note) sn->SetText (note);
		else sn->ClearText();
	}
}

DLLEXPORT bool oapiDelAnnotation (NOTEHANDLE hNote)
{
	if (hNote)
		return g_pOrbiter->DeleteAnnotation ((oapi::ScreenAnnotation*)hNote);
	else
		return false;
}

DLLEXPORT char *oapiDebugString ()
{
	return DBG_MSG;
}

DLLEXPORT double oapiRand ()
{
	static double irmax = 1.0/(double)RAND_MAX;
	return (double)rand() * irmax;
}

DLLEXPORT int oapiDeflate (const uint8_t *inp, int ninp, uint8_t *outp, int noutp)
{
	int ret, ndata;
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);

	strm.avail_in = ninp;
	strm.next_in = (uint8_t*)inp;

	strm.avail_out = noutp;
	strm.next_out = outp;
	ret = deflate(&strm, Z_FINISH);
	ndata = (ret == Z_STREAM_END ? noutp - strm.avail_out : 0);

	deflateEnd(&strm);
	return ndata;
}

DLLEXPORT int oapiInflate (const uint8_t *inp, int ninp, uint8_t *outp, int noutp)
{
	unsigned long ndata = noutp;
	if (uncompress (outp, &ndata, inp, ninp) != Z_OK)
		return 0;
	return (int)ndata;
}

// ------------------------------------------------------------------------------
// Undocumented interface functions
// ------------------------------------------------------------------------------

DLLEXPORT void InitLib (MODULEHANDLE hModule)
{
	typedef void (*OPC_DLLInit)(MODULEHANDLE hDLL);
	OPC_DLLInit DLLInit;
	char cbuf[280], mname[256]="xxxmodule", *mp;
	int i, len;

	if (td.SimT0 < 1) {
		// don't write during simulation, since unnecessary file access
		// can cause time waste
		//GetModuleFileName (hModule, mname, 256);
		for (i = 0, mp = mname; mname[i]; i++)
			if (mname[i] == '\\') mp = mname+i+1;
		sprintf (cbuf, "Module %s ", mp);
		if ((len = strlen(cbuf)) < 30) {
			for (i = len; i < 30; i++) cbuf[i] = '.';
			cbuf[i] = '\0';
		}

		char *(*mdate)() = (char*(*)())oapiModuleGetProcAddress(hModule, "ModuleDate");
		if (mdate) {
			int Date2Int (const char *date);
			sprintf (cbuf+strlen(cbuf), " [Build %06d", Date2Int(mdate()));
		} else {
			strcat (cbuf, " [Build ******");
		}

		int (*fversion)() = (int(*)())oapiModuleGetProcAddress(hModule, "GetModuleVersion");
		if (fversion) {
			sprintf (cbuf+strlen(cbuf), ", API %06d]", fversion());
		} else {
			strcat (cbuf, ", API ******]");
		}

		LOGOUT (cbuf);
	}

	DLLInit = (OPC_DLLInit)oapiModuleGetProcAddress(hModule, "InitModule");
	if (!DLLInit) DLLInit = (OPC_DLLInit)oapiModuleGetProcAddress(hModule, "opcDLLInit");
	if (DLLInit) (*DLLInit)(hModule);
}

DLLEXPORT void ExitLib (MODULEHANDLE hModule)
{
	typedef void (*OPC_DLLExit)(MODULEHANDLE hDLL);
	OPC_DLLExit DLLExit;
	DLLExit = (OPC_DLLExit)oapiModuleGetProcAddress(hModule, "ExitModule");
	if (!DLLExit) DLLExit = (OPC_DLLExit)oapiModuleGetProcAddress(hModule, "opcDLLExit");
	if (DLLExit) (*DLLExit)(hModule);
}

DLLEXPORT int Date2Int (const char *date)
{
	static const char *mstr[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	char ms[32];
	int day, month, year, v;
	sscanf (date, "%s%d%d", ms, &day, &year);
	for (month = 0; month < 12; month++)
		if (!_strnicmp (ms, mstr[month], 3)) break;
	v = (year%100)*10000 + (month+1)*100 + day;
	return v;
}

DLLEXPORT void WriteScenario_state (FILEHANDLE f, const char *tag, const AnimState &s)
{
	char cbuf[256];
	sprintf (cbuf, "%d %0.4f", s.action-1, s.pos);
	oapiWriteScenario_string (f, tag, cbuf);
}

DLLEXPORT void sscan_state (char *str, AnimState &s)
{
	int a;
	double p;
	sscanf (str, "%d%lf", &a, &p);
	s.action = (AnimState::Action)(a+1);
	s.pos = p;
}

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#define SOCK2HANDLE(s) ((SOCKETHANDLE)(ptrdiff_t)(s))
#define HANDLE2SOCK(h) ((int)(ptrdiff_t)(h))
DLLEXPORT SOCKETHANDLE oapiSocketCreate() {
	int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s != -1) {
		fcntl (s, F_SETFL, O_NONBLOCK);
	}
	// Be nonblocking
	int iMode = 1; // 0 = BLOCKING, 1 = NONBLOCKING
	if (ioctl(s, FIONBIO, (u_long*) &iMode) != 0) {
		printf("ioctl(FIONBIO) failed\n");
		close(s);
		return OAPI_INVALID_SOCKET;
	}
	return SOCK2HANDLE(s);
}

DLLEXPORT void oapiSocketClose (SOCKETHANDLE h) {
	int s = HANDLE2SOCK(h);
	shutdown(s, SHUT_RDWR);
	close(s);
}

DLLEXPORT bool oapiSocketConnect (SOCKETHANDLE h, const char *addr, int port) {
	int s = HANDLE2SOCK(h);
	struct sockaddr_in sa;

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(addr);
	sa.sin_port = htons(port);
	bool status = connect(s, (struct sockaddr*)&sa, sizeof(sa)) == 0;
	return status;
}

DLLEXPORT bool oapiSocketBind (SOCKETHANDLE h, const char *addr, int port) {
	int s = HANDLE2SOCK(h);
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(addr);
	sa.sin_port = htons( port );
	bool status = bind( s, (struct sockaddr*)&sa, sizeof(sa) ) == 0;
	return status;
}

DLLEXPORT bool oapiSocketListen (SOCKETHANDLE h, int backlog) {
	int s = HANDLE2SOCK(h);
	bool status = listen( s, backlog ) == 0;
	return status;
}

DLLEXPORT SOCKETHANDLE oapiSocketAccept (SOCKETHANDLE h) {
	int s = HANDLE2SOCK(h);
	int a = accept( s, NULL, NULL );
	return SOCK2HANDLE(a);
}

DLLEXPORT ssize_t oapiSocketSend (SOCKETHANDLE h, const void *buf, size_t len) {
	int s = HANDLE2SOCK(h);
	ssize_t sent = send(s, buf, len, MSG_DONTWAIT );
	if(sent < 0) {
		switch(errno) {
			case EINTR:
			case EAGAIN:
			case EBUSY:
				return OAPI_SOCKET_ERROR_RETRY;
			default:
				return OAPI_SOCKET_ERROR;
		}
	}
	return sent;
}
DLLEXPORT ssize_t oapiSocketRecv (SOCKETHANDLE h, void *buf, size_t len) {
	int s = HANDLE2SOCK(h);
	ssize_t received = recv( s, buf, len, MSG_DONTWAIT );
	if(received < 0) {
		switch(errno) {
			case EINTR:
			case EAGAIN:
			case EBUSY:
				return OAPI_SOCKET_ERROR_RETRY;
			default:
				return OAPI_SOCKET_ERROR;
		}
	}
	return received;
}

// Check "dir" for an entry named "name", ignoring case
// If no entry is found, returns the queried name
static std::string GetDirEntry(std::string &&dir, std::string name) {
    std::error_code ec;

    // if dir is empty, we want to look in the current directory
    if(dir == "") dir = "./";

    // if dir does not exist, bail out
    if(!std::filesystem::exists(dir, ec)) {
        return name;
    }

    // fast check if the file exists with the expected name
    std::filesystem::path path(dir + name);
    if(std::filesystem::exists(path, ec)) {
        return name;
    }

    // slow way: we iterate over every entry in the directory
    // and do a case insensitive comparison
    for (auto const& dir_entry : std::filesystem::directory_iterator{dir}) 
    {
        if(!strcasecmp(name.c_str(), dir_entry.path().filename().c_str())) {
            return dir_entry.path().filename();
        }
    }

    // we found nothing -> returns the queried name
    return name;
}

DLLEXPORT std::string oapiGetFilePath(const char *path) {
    size_t pos_start = 0, pos_end;
    std::string s(path);
    std::stringstream ss;

    while ((pos_end = s.find_first_of ("\\/", pos_start)) != std::string::npos) {
        std::string token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + 1;

        std::string entry = GetDirEntry(ss.str(), token);
        ss<<entry<<"/";
    }

    std::string entry = GetDirEntry(ss.str(), s.substr(pos_start));
    ss<<entry;

    return ss.str();
}

DLLEXPORT std::istream& oapiGetLine(std::istream& is, std::string& line, char delim)
{
    line.clear();
    std::istream::sentry sentry(is, true);
    std::streambuf* sbuf = is.rdbuf();
	std::stringstream ss;

    for(;;) {
        int chr = sbuf->sbumpc();
        switch (chr) {
        case '\r':
            if(sbuf->sgetc() == '\n')
                sbuf->sbumpc();
			//fall through
        case '\n':
			goto done;
        case std::streambuf::traits_type::eof():
            is.setstate(std::ios::eofbit);
            if(line.empty())
                is.setstate(std::ios::failbit);
			goto done;
        default:
			if((char)chr == delim)
				goto done;
			ss<<(char)chr;
            break;
        }
    }
done:
	line = ss.str();
	return is;
}

