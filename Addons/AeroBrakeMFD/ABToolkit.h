// ==============================================================
//  MFD ToolKit for ORBITER SPACE FLIGHT SIMULATOR
//  (c) Jarmo Nikkanen 2002 - 2003
//
// ==============================================================


#ifndef __ORBIT_TOOLKIT_H
#define __ORBIT_TOOLKIT_H

#include <Orbitersdk.h>
#include <float.h>

extern unsigned int green;
extern unsigned int dgreen;	 
extern unsigned int lgreen;	 
extern unsigned int hgreen;	 
extern unsigned int yellow;  
extern unsigned int lyellow; 
extern unsigned int grey;    
extern unsigned int dgrey;  
extern unsigned int red;  
extern unsigned int white; 
extern unsigned int blue;
extern unsigned int itemcolor;


#define PI2		(PI*2)
#define PI15    (PI*1.5)
#define GMsun	1.32712440018e20
#define AU		1.49597870691e11
#define GC      6.67259e-11
#define EPOCH   51544.5
#define MIN_ECC 0.000001
#define SUM(v)  v.x+v.y+v.z
#define INVALIDV(v) if (isnan(SUM(v)) || !isfinite(SUM(v)))
#define INVALIDD(d) if (isnan(d) || !isfinite(d))
#define IS_VALIDV(v) (!isnan(SUM(v)) || isfinite(SUM(v)))
#define IS_VALIDD(d) (!isnan(d) || isfinite(d))
#define ZERO(v) v.x==0 && v.y==0 && v.z==0

// Hyperbolic orbit:
// ----------------
// Eccentric anomaly is negative before periapis passage. Maximum range used in this software
// is [-10, 10] for eccentric anomaly
// Mean anomaly is also negative before periapis passage. Maximum range is [-inf, inf]
// True anomaly and true longitude are defined [0, 2PI]
// Semi-major axis is negative.
// Parameter par is always positive


// Elliptic orbit:
// --------------
// Eccentric/Mean/True anomaly, Mean/True longiture are defined [0, 2PI]
// Passing values to this software those are out of range may cause an error



double MIN(double a,double b);
double MAX(double a,double b);
double asinh(double);
double acosh(double);

extern class Orbit Ecliptic;

struct Ship_s {
	OBJHANDLE ref;
	VECTOR3 pos, vel;
	double  mass;
	double  thr;
	double  isp;
};


inline double limit(double x)
{
	if (x>PI2) return fmod(x,PI2);
    if (x<0)   return PI2-fmod(-x,PI2);
	return x;
}


inline double dot(VECTOR3 v,VECTOR3 h)
{
	return (v.x*h.x + v.y*h.y + v.z*h.z);
}
/*
inline double length(VECTOR3 v)
{
       return(sqrt(v.x*v.x+v.y*v.y+v.z*v.z));
}

inline VECTOR3 unit(VECTOR3 v)
{
	double l=sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
	v.x/=l; v.y/=l; v.z/=l;
	return(v);
}
*/
inline VECTOR3 set_length(VECTOR3 v, double l)
{
	double x=l/sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
	v.x*=x; v.y*=x; v.z*=x;
	return(v);
}





// Text Functions

void    NameFix(char *);
void    NameUnFix(char *);


double  text2value(char *);
void	value(double real,char *buf,bool angle=false,bool au=true);
void	Text(oapi::Sketchpad *skp,int x,int y,const char *txt);
void	Text(oapi::Sketchpad *skp,int x,int y,const char *txt,const char *str);
void	Text(oapi::Sketchpad *skp,int x,int y,const char *txt,double val,const char *un=NULL);
void	TextA(oapi::Sketchpad *skp,int x,int y,const char *txt,double angle,const char *un=NULL);
void    Multiline(oapi::Sketchpad *skp,int,int,int,int,const char *);
char    *MJDToClock(double mjd, char *buf);
char    *TimeToClock(double mjd, char *buf);

double	Time2MJD(double t);
double	MJD2Time(double m);


// Conversion Functions

// base for elliptic and hyperbolic orbit
double	mna2eca(double mna,double ecc);
double	eca2mna(double eca,double ecc);
double	tra2eca(double tra,double ecc);
double	eca2tra(double eca,double ecc);

// Conversion between tangential angle and true anomaly
double  tra2tan(double tra,double ecc);
double  tan2tra(double tan,double ecc,bool pe_side);

// advanced for elliptic and hyperbolic orbit
double	tra2mna(double tra,double ecc);
double	mna2tra(double mna,double ecc);

double tandir2tra(double tandir, double ecc);
double time_between(double a, double b, double ecc, double mnm, double lpe);



// General functions
// For elliptic and hyperbolic orbit

double	difference(double a,double b);
double	angular_distance(double a,double b,bool pro=true);
double	angle(VECTOR3 v,VECTOR3 h);
double	angle(VECTOR3 p,VECTOR3 zero,VECTOR3 r);
double  nangle(VECTOR3 p,VECTOR3 zero,VECTOR3 normal);
VECTOR3 create_vector(VECTOR3 normal,VECTOR3 zero,double angle);
double  EccOfOrbit(VECTOR3 r, VECTOR3 v,double myy);
double	BurnTimeBydV(double dv,VESSEL *ship,double *mass=NULL);


VECTOR3 RotationAxis(double,double);
VECTOR3 RotationAxis(OBJHANDLE ref);
VECTOR3 AirSpeed(VECTOR3,VECTOR3,double);
VECTOR3 GroundSpeedVector(OBJHANDLE ref,double time,double lon, double lat);
VECTOR3 GroundSpeedVector(OBJHANDLE ref,VESSEL *shp);

void    PlanetAxis(double obliquity, double suntransit, double offset, double period, double since_epoch,VECTOR3 *Rot,VECTOR3 *Off);
void    PlanetAxis(double obliquity, double suntransit,VECTOR3 *Rot,VECTOR3 *Off);
void    PlanetAxis(OBJHANDLE ref,double time,VECTOR3 *Rot,VECTOR3 *Off);
 

void    LonLat(VECTOR3 in,VECTOR3 rot,VECTOR3 off,double *Lon,double *Lat);
VECTOR3 VectorByLonLat(VECTOR3 rot,VECTOR3 off,double Lon,double Lat);

double  CalculateSOI(OBJHANDLE obj, OBJHANDLE ref);

void	GetGlobalPosAfterTime(OBJHANDLE obj,VECTOR3 *pos,VECTOR3 *vel,double time,bool sys=true);
void    NewPosition(VECTOR3 *pos, VECTOR3 *vel, double myy, double stp);
double	SystemMass(OBJHANDLE obj);
bool	SystemVectors(OBJHANDLE obj,VECTOR3 *position, VECTOR3 *velocity);
bool	SystemVectors(OBJHANDLE obj,OBJHANDLE ref,VECTOR3 *position, VECTOR3 *velocity);

 
// Drawing functions

void	DrawEllipse(oapi::Sketchpad *skp,double x,double y,double xx,double yy,double w,double h);
void	DrawRectangle(oapi::Sketchpad *skp,double x,double y,double xx,double yy,double w,double h);
bool	DrawLine(oapi::Sketchpad *skp,double x,double y,double xx,double yy,double w,double h,bool ex);


// advanced
// For elliptic and hyperbolic orbit


void	DrawDirectionSelector(oapi::Sketchpad *skp,VESSEL *ship,VECTOR3 vel,int width,int height);
void	DrawLatitude(oapi::Sketchpad *skp,OBJHANDLE handle,class Orbit *prj,VECTOR3 rot,VECTOR3 off,double Lat,double zoom,double w,double h);
void	DrawLongitude(oapi::Sketchpad *skp,OBJHANDLE handle,class Orbit *prj,VECTOR3 rot,VECTOR3 off,double Lon,double zoom,double w,double h);

void	RungeKutta1(VECTOR3 *pos,VECTOR3 *vel,double *mu,int no,double step);
void	RungeKutta2(VECTOR3 *pos,VECTOR3 *vel,double *mu,int no,double step);

#endif
