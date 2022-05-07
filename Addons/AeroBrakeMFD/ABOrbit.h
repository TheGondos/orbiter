// ==============================================================
//  MFD ToolKit/Orbit class for ORBITER SPACE FLIGHT SIMULATOR
//  (c) Jarmo Nikkanen 2002 - 2004
//
// ==============================================================

#ifndef __ORBIT_ORBIT_H
#define __ORBIT_ORBIT_H

#include "ABToolkit.h"
#include "Orbitersdk.h"




// Hyperbolic orbit:
// ----------------
// Eccentric anomaly is negative before periapis passage. Maximum range used in this software
// is [-10, 10] for eccentric anomaly
// Mean anomaly is also negative before periapis passage. Maximum range is [-inf, inf]
// True anomaly and true longitude are defined [0, 2*PI]
// Semi-major axis is negative.


// Elliptic orbit:
// --------------
// Eccentric/Mean/True anomaly, Mean/True longiture are defined [0, 2*PI]
// Passing values to this software those are out of range may cause an error

#define ORBIT_ECLIPTIC


class Orbit {

public:

			Orbit();
			Orbit(OBJHANDLE body,OBJHANDLE ref);		
			Orbit(char *body, char *ref);				
			Orbit(VECTOR3 pos,VECTOR3 vel,double myy);	

			~Orbit();

			// Describe new orbit
	void	Elements(VECTOR3 r, VECTOR3 v,double myy,bool reset_proj=true);
	void	Elements(char *body,char *ref);	
	void	Elements(OBJHANDLE body,OBJHANDLE ref);	

	void	GEO(OBJHANDLE ref);	 // Geostationary orbit
	void    LEO(OBJHANDLE ref);  // Local equatorial orbit for reference body
	void    Ecliptic();          // Ecliptic orbit around the sun


	
	// Define an orbit with these parameters
	bool    DefineOrbit(OBJHANDLE ref, VECTOR3 pos, VECTOR3 normal,double tra,double ecc);

	// Create new circular orbit at orbital plane of orbit "plane" with radius of "rad"
	void    CreateNewCircularOrbit(Orbit *plane,double rad);

	

	void	Reset();						
	void	SetProjection(class Orbit *);  // One orbit is used as projection plane for drawing purposes.	
	void    CreateProjectionPlane(VECTOR3 normal,VECTOR3 zero);

	void	SetTime(double);			
	double	GetTime();					
	
	void    SetOrbitPosition(double trl);

	double	TimeToPeriapis();			
	double  PeriapisVelocity();		
	double  PeriapisDistance();			
	double  ApoapisDistance();			
	

	double	Radius(double trl);			
	double	Velocity(double trl);		
	double  VelocityByRadius(double rad);   


	double	TimeToNode(Orbit *,double *trl_of_node=NULL);				
	double  TrlOfNode(Orbit *);	
	double  TrlOfNode(VECTOR3 normal);	
	
    
	double  TrlByRadius(double);
	double	TrlByTime(double);	
	double	TimeTo(double);	
	double  TimeToPoint(double longitude);
	double	SemiMinor();		
	double	MeanMotion();		
	double	Period();	// Orbit period
	double  Energy();	// Orbital Energy

    // Receive some vector information
	VECTOR3 Velocity();	
	VECTOR3 Position();	
	VECTOR3 Position(double trl);				
	VECTOR3 Tangent(double trl);
	VECTOR3 Asymptote();
	VECTOR3 EscapeAsymptote();
    VECTOR3 Projection(VECTOR3 pr,VECTOR3 dir);
	VECTOR3 Projection(VECTOR3 pr);
	
	void	Longitude(VECTOR3 pr,double *rad,double *height,double *angle);		
	double	Longitude(VECTOR3 pr);												
    double  TangentialAngle(double longitude);
	double  MaxTra();	

	

    //Draw
	void    Point(double xp,double yp,double l,double *x,double *y,double s);
	void    Point(VECTOR3 pr,double xp,double yp,double s,double *x,double *y);

	void    DrawPlaneIntersection(oapi::Sketchpad *skp,double lan,double cx,double cy,double zoom,int color,bool box=true);
	void    DrawPlaneIntersection(oapi::Sketchpad *skp,Orbit *t,double cx,double cy,double zoom,int color,bool box=true);
	void	Draw(oapi::Sketchpad *skp,int color,double x,double y,double siz,OBJHANDLE ref=NULL,bool pe=true,bool ra=true,bool la=true);
	void	Displacement(VECTOR3);

	// Special function used with hyperbolas only.
	void    SetTimeToPeriapis(double);
	double	Translate(Orbit *,double longitude);
    void    SetNULL();
	bool    Defined();
	bool    IsValid(double trl);

	// READ ONLY

	double	sma;		// Semi-major axis
	double	ecc;		// Eccentricity
	double	lan;		// Longitude of ascenting node
	double	agp;		// Argument of periapis
	double	inc;		// Inclination
	double	tra;		// True anomaly
	double	trl;		// True longitude;
	double	lpe;		// Longitude of periapis
	double	rad;		// Radius
	double  vel;		// Velocity
	double	myy;		// Reduced mass = G * "mass of central body"
	double	mna;		// Mean anomaly
	double	par;		// Parameter = a * fabs(1-e*e)
	double  ang;        // Angular momentum
	double	mnm;		// MeanMotion

	VECTOR3 norv;		// Unit length ! Normal Vector
	VECTOR3 majv;		// Unit length ! Major axis, Pointing periapis 
	VECTOR3 minv;		// Unit length ! Minor axis, (norv X majv)

	VECTOR3 zero;		// Projection vectors	
	VECTOR3 nine;

	VECTOR3 Xmajv;		
	VECTOR3 Xminv;	
	VECTOR3 Xcv;

private:

	void	CreateProjection();							
				

	bool	IsVisible(double);
	bool    Limits(double,double,double *,double *,double *);
	bool	IsOut(double,double);
	bool    IsSmall(double,double);
	void    DrawOrbitSegment(oapi::Sketchpad *skp,double xp,double yp,double w,double h,double siz,double s,double e);
	void	XDrawHyperbola(oapi::Sketchpad *skp,double xp,double yp,double,double,double siz);				
	void	XDrawEllipse(oapi::Sketchpad *skp,double,double,double siz,double start=0,double end=PI2);		
	
	bool	DrawAll(double,double);
	
	VECTOR3 PositionByEcA(double);


	double	time;		// MJD of current orbit position

	bool    EclipticPlane;
	OBJHANDLE LeoRef, GeoRef;
	VECTOR3 displacement;			//Zero, if not in use. Clear in reset
	VECTOR3 vv;			// Current velocity vector
	VECTOR3 rv;			// Current position vector

	class	Orbit *intersection;	// Clear in reset
	class	Orbit *node_trl_o;		// Clear in reset

	double	first,second,node_trl,timeto;

	double  xcenter,ycenter,scale,width,height;

};

#endif

