
// ==============================================================
//  MFD ToolKit for ORBITER SPACE FLIGHT SIMULATOR
//  (c) Jarmo Nikkanen 2002 - 2003
//
// ==============================================================

//�����


#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOSOUND
#define NOKANJI
#define NOIMAGE
#define NOTAPE



#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <cstring>
#include "Orbitersdk.h"
#include "ABToolkit.h"
#include "ABReference.h"
#include "ABOrbit.h"

unsigned int green	   = 0x00009900; // Ship orbit
unsigned int dgreen	   = 0x00006600; // Dark green, map lines
unsigned int lgreen	   = 0x0000CC00; // Text color
unsigned int hgreen	   = 0x0000EEAA; // Hilighted text
unsigned int yellow    = 0x0000BBBB; // Target orbit
unsigned int lyellow   = 0x0000DDDD; // Yellow text
unsigned int grey      = 0x00999999; // Light grey lines, intersection
unsigned int dgrey     = 0x00666666; // Dark Grey, planet, actual ship orbit in HTO
unsigned int red       = 0x000000DD; // Warning lights
unsigned int white     = 0x00EEEEEE; // White
unsigned int blue      = 0x00FF0000; // Blue
unsigned int itemcolor = 0x00FF8888; //

class Orbit Ecliptic;


double acosh(double x)
{
	return( log( x + sqrt( x*x - 1)) );
}

double asinh(double x)
{
	return( log( x + sqrt( x*x + 1)) );
}

double MIN(double a,double b)
{
	if (a<b) return(a);
	return(b);
}

double MAX(double a,double b)
{
	if (a<b) return(b);
	return(a);
}


void NameFix(char *name)
{
    unsigned int i;
	if (name==NULL) return;
	for (i=0;i<strlen(name);i++) if (name[i]==' ') name[i]='_';
}

void NameUnFix(char *name)
{
    unsigned int i;
	if (name==NULL) return;
	for (i=0;i<strlen(name);i++) if (name[i]=='_') name[i]=' ';
}


double text2value(char *txt)
{
	double m=1;
	int l=strlen(txt);
	char c=txt[l-1];

	if (c=='d' || c=='D') m=86400;
	if (c=='h' || c=='H') m=3600;
	if (c=='k' || c=='K') m=1000;
	if (c=='M')			  m=1e6;
	if (c=='g' || c=='G') m=1e9;
	if (c=='a' || c=='A') m=AU;
	if (c=='b' || c=='B') m=1e5;
	if (c=='u' || c=='U') m=1e-6;
	if (c=='m')			  m=1e-3;

	if (m!=1) txt[l-1]=0;
	return(atof(txt)*m);
}


// Convert Value to string

void value(double real,char *buf,bool angle,bool au)
{
	 char buffer[512];

     double v=fabs(real);
     int n;
     memset(buffer,0,12);
	 
	 
	 const char *c={""};
	 const char *k={"k"};
	 const char *k2={"M"};
	 const char *k3={"G"};
	 const char *k4={"AU"};
	 const char *k5={"m"};
	 const char *k6={"�"};
	 const char *k7={"T"};

     n=(int)floor(log10(v))+1;
     
     if (angle==false) {
	 if (n>15) {
     sprintf(buffer,"%1.3e\n",real); 
	 strncpy(buf,buffer,10); 
	 return;
	 }
     else if (n>12 && !au) real/=1e12,c=k7;
     else if (n>11 && au) real/=149.5979e9,c=k4;
	 else if (n>9) real/=1e9,c=k3; 
     else if (n>6) real/=1e6,c=k2; 
     else if (n>3) real/=1e3,c=k; 
	 else if (n<(-4)) real*=1e6,c=k6; 
	 else if (n<(-1)) real*=1e3,c=k5; 

	 }
     
	 if (angle) { 
		 sprintf(buffer,"%1.2f",real); 
		 strncpy(buf,buffer,6);
		 strcat(buf,"�");
	 }
	 else {
		 sprintf(buffer,"%1.3f",real); 
		 strncpy(buf,buffer,6);
		 strcat(buf,c);
	 }
}


char *MJDToClock(double mjd, char *buf)
{
	char b[32];
	double sec=fabs((mjd-floor(mjd))*86400);

	double h=floor(sec/3600);
	double m=floor((sec-(h*3600))/60);
	double s=floor(sec - (h*3600) - (m*60) );
    
	
	sprintf(b,"%0.0f",h); 
	if (h<10) strcpy(buf,"0"), strcat(buf,b), strcat(buf,":");
	else strcpy(buf,b), strcat(buf,":");

	sprintf(b,"%0.0f",m);
	if (m<10) strcat(buf,"0"), strcat(buf,b), strcat(buf,":");
	else strcat(buf,b), strcat(buf,":");


	sprintf(b,"%0.0f",s); 
	if (s<10) strcat(buf,"0"), strcat(buf,b);
	else strcat(buf,b);

	return buf;
}

char *TimeToClock(double sec, char *buf)
{
	char b[32];

	double h=floor(sec/3600);
	double m=floor((sec-(h*3600))/60);
	double s=sec - (h*3600) - (m*60);
    
	
	sprintf(b,"%0.0f",h); 
	if (h<10) strcpy(buf,"0"), strcat(buf,b), strcat(buf,":");
	else strcpy(buf,b), strcat(buf,":");

	sprintf(b,"%0.0f",m);
	if (m<10) strcat(buf,"0"), strcat(buf,b), strcat(buf,":");
	else strcat(buf,b), strcat(buf,":");


	sprintf(b,"%0.2f",s); 
	if (s<10) strcat(buf,"0"), strcat(buf,b);
	else strcat(buf,b);

	return buf;
}


// Text output functions

void Text(oapi::Sketchpad *skp,int x,int y,const char *txt)
{
	skp->Text(x,y,txt,strlen(txt));
}


void Text(oapi::Sketchpad *skp,int x,int y,const char *txt,const char *str)
{
	char buf[64];
	strcpy(buf,txt);
	strcat(buf,str);
	skp->Text(x,y,buf,strlen(buf));
}

void Text(oapi::Sketchpad *skp,int x,int y,const char *txt,double val,const char *un)
{
	char buf[64];
	char buf2[64];
	memset(buf2,0,15);
	strcpy(buf,txt);
	value(val,buf2,0);
	strcat(buf,buf2);
	if (un) strcat(buf,un);
	skp->Text(x,y,buf,strlen(buf));
}

void Multiline(oapi::Sketchpad *skp,int x,int y,int width,int height,const char *text)
{
	int tot=strlen(text);
	int start=0;
	int end=0;
	
	while (end<tot) {
		end=start+width;
		if (end>tot) end=tot;
		
		if (end<tot) while (text[end]!=' ' && end>0) end--;

		int wi=end-start;
		skp->Text(x,y,text+start,wi);
		start+=(wi+1);
		y+=height;
	}
}


// For Angles

void TextA(oapi::Sketchpad *skp,int x,int y,const char *txt,double val,const char *un)
{
	char buf[64];
	char buf2[64];
	memset(buf2,0,10);
	strcpy(buf,txt);
	value(val,buf2,1);
	strcat(buf,buf2);
	if (un) strcat(buf,un);
	skp->Text(x,y,buf,strlen(buf));
}



double Time2MJD(double t)
{
	if (t<0) t=0;
	return(oapiGetSimMJD()+(t/86400));
}



double MJD2Time(double m)
{
	double x=(m-oapiGetSimMJD())*86400;
	if (x<0) x=0;
	return x;	
}


double difference(double a,double b)
{	
	double n;
	a=limit(a);
	b=limit(b);
	
	if (a>b) n=(a-b);
	else n=(b-a);

	if (n>PI) n=PI2-n;

	return(n);
}

double angular_distance(double a,double b,bool pro)
{	
	double n;
    if (a<b) n=b-a;
	else n=PI2-(a-b);
	if (!pro) n=PI2-n;
	return(n);
}


double angle(VECTOR3 v,VECTOR3 h)
{
	double x = dot(v,h) / (length(h)*length(v));
	if (x>1)  x=1; // Avoid precision problems
	else if (x<-1) x=-1;
	return( acos( x ) );
}



double angle(VECTOR3 p,VECTOR3 z,VECTOR3 r)
{
	double y=dot(p,z)/length(z);
    double x=dot(p,r)/length(r);
	return ( limit( atan2(x,y) ) );
}


double nangle(VECTOR3 p,VECTOR3 zero,VECTOR3 normal)
{
	VECTOR3 nine=crossp(normal,zero);
	double y=dot(p,zero)/length(zero);
    double x=dot(p,nine)/length(nine);
	return ( limit( atan2(x,y) ) );
}

VECTOR3 create_vector(VECTOR3 normal,VECTOR3 zero,double angle)
{
	zero=unit(zero);
	normal=unit(normal);
	zero=unit(zero-(zero*dotp(zero,normal)));
	VECTOR3 per=unit(crossp(normal,zero));
	VECTOR3 vec=zero*cos(angle) + per*sin(angle);
	return vec;
}


double tandir2tra(double td, double e)
{
	double v=PI;
	double step=PI05;
	for (int i=0;i<12;i++) {
		double x=e*cos(v)+1;
		double tau=asin(sqrt(x*x/(e*e+2*e*cos(v)+1)));
		if (v>PI) tau=PI-tau;
		double t=(tau+v);

		if (t>td) v-=step;
		else v+=step;

		step/=2.0;
	}
	return v;
}


double tra2tan(double tra,double e)
{ 
	if (e==0) return PI05;

	double co = cos(tra); 
	double x  = 2*co*e+1;
	double ta = asin( sqrt(co*co*e*e+x) / sqrt(e*e+x) );
	if (tra>PI) return PI-ta;
	return ta;
}



double tan2tra(double tan,double e,bool pe_side)
{ 
	double s  = sin(tan); 
	double ta;
	if (pe_side) {
		ta = acos( (s*s + s*sqrt(e*e+s*s-1) - 1) / e );
		if (tan>PI05) return PI2-ta;
	}	
	else {
		ta = acos( (s*s - s*sqrt(e*e+s*s-1) - 1) / e );
		if (tan>PI05) return PI2-ta;
	}
	return ta;
}



double tra2eca(double tra,double e)
{ 
	double eca; 
	double cos_tra = cos(tra);
    	
	if (e>1) {
        eca = acosh( (e+cos_tra) / (1+e*cos_tra) );	
		if (tra>PI) eca=-eca;
		return(eca);
	}

	if (e>0) {
		eca = acos( (e+cos_tra) / (1+e*cos_tra) );
	    if (tra>PI) eca = PI2 - eca;
		return(eca);
	}

	return(tra);
}




double tra2mna(double tra,double ecc)
{	
    if (ecc==0) return(tra);
	double eca = tra2eca(tra,ecc);	
	return(eca2mna(eca,ecc));
}




double eca2mna(double eca,double ecc)
{
	double mna;

	if (ecc==0) return(eca);

	if (ecc<1) {
	    mna = eca - ( ecc * sin( eca ) ); 
		return (limit(mna));
	}
	
    return ( ( ecc * sinh(eca) ) - eca );	
}

	

// Avoid calling this very often
double mna2eca(double mna,double ecc)
{
    int i,c;
	double delta,M,eca,mnamp;
  

	if (ecc==0) return(mna);

	// For Elliptic orbit

    if (ecc<1) { 

		mnamp=mna;
		if (mna>PI) mnamp=PI2-mna;

        if (ecc<0.9) { // Solve using newton's method
	
			if (ecc<0.4)	c=4;
			else 
			if (ecc<0.7)	c=5;
			else			c=7;
		
			eca=mnamp;
			for (i=0;i<c;i++) eca = eca + (mnamp + ecc*sin(eca) - eca) / (1 - ecc*cos(eca));		
			

			if (mna>PI) eca=PI2-eca;
			return eca;
		}


	   	eca=PI05; delta=eca/2;
   
		for (i=0;i<32;i++) {   // Must be 28
			M=eca-ecc*sin(eca);
			if (M>mnamp) eca-=delta;
			else		 eca+=delta;
			delta/=2;
		}

		if (mna>PI) eca=PI2-eca;

		return(eca);
	} 
	


    // For Hyperbolic orbit
    
	eca=10; delta=eca/2;
    mnamp=fabs(mna);

	for (i=0;i<32;i++) {	
		M=ecc*sinh(eca)-eca;
		if (M>mnamp) eca-=delta;
		else		 eca+=delta;
		delta/=2;
	}

	if (mna<0) eca=-eca;
	return(eca);
}



double eca2tra(double eca,double ecc)
{
	if (ecc==0) return(eca);

	if (ecc<1) {
		double cos_ea=cos(eca);
		double tra=acos( ( cos_ea - ecc ) / ( 1 - ecc * cos_ea ) );     
		if (eca > PI) tra=PI2-tra;
		return(limit(tra));
	}

    double cos_ea=cosh(eca);
	double tra=acos( ( ecc - cos_ea ) / (  ecc * cos_ea - 1 ) ); 
	if (eca < 0) tra=PI2-tra;
	return(tra);
}



double mna2tra(double mna,double ecc)
{
	double eca=mna2eca(mna,ecc);
	double tra=eca2tra(eca,ecc);
    return(tra);
}



double time_between(double a, double b, double ecc, double mnm, double lpe)
{
	a=limit(a-lpe);
	b=limit(b-lpe);

	if (ecc<1.0) {
		
		double ang = angular_distance(tra2mna(a,ecc), tra2mna(b,ecc));
		
		if (ang>PI) ang-=PI2;

		return (ang/mnm);
	}
	
	double max = acos(1/ecc)*0.99999;

	if (a>max && a<(PI2-max)) return(0);
	if (b>max && b<(PI2-max)) return(0);

	double ang = tra2mna(b,ecc) - tra2mna(a,ecc);	
	
	return(ang / mnm);
}




// Checked OK
// Input: Obliquity of ecliptic, longitude of Sun�s transit		=angle(ecliptic normal , rot-axis)
// Warning: "Obliquity of orbit" is not the same ! ! !			=angle(planet orbit normal , rot-axis)

VECTOR3 RotationAxis(double obliquity, double suntransit)
{
	VECTOR3 majv=_V(1,0,0);
	VECTOR3 minv=_V(0,0,1);
	VECTOR3 norv=_V(0,-1,0);

	VECTOR3 lan = majv * cos(suntransit) + minv * sin(suntransit);
    VECTOR3 per = crossp(norv,lan);
	VECTOR3 rot = norv * cos(obliquity) - per * ( sin(obliquity) / sqrt(dotp(per,per)) );

	return (rot);
}



void PlanetAxis(double obliquity, double suntransit, double offset, double period, double since_epoch,VECTOR3 *Rot,VECTOR3 *Off)
{
	VECTOR3 majv=_V(1,0,0);
	VECTOR3 minv=_V(0,0,1);
	VECTOR3 norv=_V(0,-1,0);

	VECTOR3 lan = majv * cos(suntransit) + minv * sin(suntransit);
    VECTOR3 per = crossp(norv,lan);
	VECTOR3 rot = norv * cos(obliquity) - per * ( sin(obliquity) / sqrt(dotp(per,per)) );

	if (Rot) *Rot=rot;

    double posit = limit((since_epoch*86400.0/period)*PI2 + offset); 

	        per = crossp(rot,lan);
	VECTOR3 off = lan * cos(posit) + per * (sin(posit) / sqrt(dotp(per,per)) );
 
    if (Off) *Off=off;
}



// Warning Offset is not a rotation offset
void PlanetAxis(double obliquity, double suntransit,VECTOR3 *Rot,VECTOR3 *Off)
{
	VECTOR3 majv=_V(1,0,0);
	VECTOR3 minv=_V(0,0,1);
	VECTOR3 norv=_V(0,-1,0);

	VECTOR3 lan = majv * cos(suntransit) + minv * sin(suntransit);
    VECTOR3 per = crossp(norv,lan);
	VECTOR3 rot = norv * cos(obliquity) - per * ( sin(obliquity) / sqrt(dotp(per,per)) );

	if (Rot) *Rot=rot;
    if (Off) *Off=lan;
}


void PlanetAxis(OBJHANDLE ref,double time,VECTOR3 *Rot,VECTOR3 *Off)
{
	VECTOR3 majv=_V(1,0,0);
	VECTOR3 minv=_V(0,0,1);
	VECTOR3 norv=_V(0,-1,0);

	double suntransit = oapiGetPlanetTheta(ref);
	double obliquity  = oapiGetPlanetObliquity(ref);
	double offset     = oapiGetPlanetCurrentRotation(ref);
	double period     = oapiGetPlanetPeriod(ref);

	VECTOR3 lan = majv * cos(suntransit) + minv * sin(suntransit);
  VECTOR3 per = crossp(norv,lan);
	VECTOR3 rot = norv * cos(obliquity) - per * ( sin(obliquity) / sqrt(dotp(per,per)) );

	if (Rot) *Rot=rot;

	if (Off) {
		double posit = limit((time*86400.0/period)*PI2 + offset); 
		per = crossp(rot,lan);
		VECTOR3  off = lan * cos(posit) + per * (sin(posit) / sqrt(dotp(per,per)) );
		*Off=off;
	}
}

VECTOR3 RotationAxis(OBJHANDLE ref)
{
	VECTOR3 majv=_V(1,0,0);
	VECTOR3 minv=_V(0,0,1);
	VECTOR3 norv=_V(0,-1,0);

	double suntransit = oapiGetPlanetTheta(ref);
	double obliquity  = oapiGetPlanetObliquity(ref);
	
	VECTOR3 lan = majv * cos(suntransit) + minv * sin(suntransit);
    VECTOR3 per = crossp(norv,lan);
	VECTOR3 rot = norv * cos(obliquity) - per * ( sin(obliquity) / sqrt(dotp(per,per)) );

	return rot;
}



VECTOR3 SunTransitAxis(OBJHANDLE ref)
{
	VECTOR3 majv=_V(1,0,0);
	VECTOR3 minv=_V(0,0,1);
	double suntransit = oapiGetPlanetTheta(ref);
	VECTOR3 lan = majv * cos(suntransit) + minv * sin(suntransit);
	return lan;
}





void LonLat(VECTOR3 in,VECTOR3 rot,VECTOR3 off,double *Lon,double *Lat)
{	
	double l=nangle(in,off,rot);
	if (l>PI) 
    l=l-PI2;
	
  double h=dotp(in,rot)/length(rot);
	double t=asin(h/length(in));

	if (Lon) 
    *Lon=l;
	if (Lat) 
    *Lat=-t;	
}




VECTOR3 VectorByLonLat(VECTOR3 rot,VECTOR3 off,double Lon,double Lat)
{
	return unit( unit(rot)*sin(-Lat) + 
		   unit(off)*(cos(Lon)*cos(Lat)) +
		   unit(crossp(rot,off))*(sin(Lon)*cos(Lat)) );
}



// Ship relative Air Speed = Rel.Velocity - AirSpeed
// Rotation velocity of atmosphere

VECTOR3 AirSpeed(VECTOR3 pos,VECTOR3 rot_axis,double rotper)
{
	double p = dot(pos, rot_axis) / length(rot_axis);
	double l = length(pos);
	double r = sqrt(l*l - p*p);
	double v = PI2 * r / rotper;

	return ( unit( crossp(rot_axis, pos) ) * v );
}



VECTOR3 GroundSpeedVector(OBJHANDLE ref,double time,double lon, double lat)
{
	VECTOR3 rot,off;
	
	if (time==0) time=oapiGetSimMJD();
	PlanetAxis(ref,time,&rot,&off);

	VECTOR3 gpv = VectorByLonLat(rot,off,lon,lat);   
    double v = PI2 * cos(lat) * oapiGetSize(ref) / oapiGetPlanetPeriod(ref);
	return ( unit( crossp(rot, gpv) ) * v );
}	


VECTOR3 GroundSpeedVector(OBJHANDLE ref,VESSEL *shp)
{
	VECTOR3 rot,off;	
	PlanetAxis(ref,0,&rot,&off);

	VECTOR3 gpv,vel;

	shp->GetRelativePos(ref,gpv);
	shp->GetRelativeVel(ref,vel);

	vel=vel*(oapiGetSize(ref)/length(gpv));

	double lat = angle(gpv,rot)-PI05;
    double v = PI2 * cos(lat) * oapiGetSize(ref) / oapiGetPlanetPeriod(ref);
	
	VECTOR3 GSp = unit( crossp(rot, gpv) ) * v;
	
	return (GSp-vel);
}



double BurnTimeBydV(double dv,VESSEL *ship,double *mass)
{
	double isp=ship->GetISP();
	double ev = 9.81 * isp;
	double mas=ship->GetMass();
	double th=ship->GetMaxThrust(ENGINE_MAIN);

	double time = (mas * ev / th) * ( 1 - exp(-dv/ev) );
    double eff=1;

    if (mass) *mass = ( th / ( isp * eff) ) * time;

	return(time);
}


double SystemMass(OBJHANDLE obj)
{
	int i;
	if (obj==NULL) return 0;
    if (obj==Refer->StarHandle) return oapiGetMass(obj);

	double sys_mass=oapiGetMass(obj);
	int c=Refer->GetSystemCount(obj);

	if (c>0) {		
		OBJHANDLE *list=Refer->GetSystemList(obj);     
		for (i=0;i<c;i++) sys_mass+=oapiGetMass(list[i]);	
	}

	return sys_mass;
}




bool SystemVectors(OBJHANDLE obj,VECTOR3 *position, VECTOR3 *velocity)
{
	VECTOR3 pos,vel,gp,gv;
    int i;

    if (obj==NULL || position==NULL || velocity==NULL) return false;

	oapiGetGlobalPos(obj,&pos);
	oapiGetGlobalVel(obj,&vel);

	*position=pos;
	*velocity=vel;

	if (obj==Refer->StarHandle) return true;

	int c=Refer->GetSystemCount(obj);

	if (c>0) {
		
		OBJHANDLE *list=Refer->GetSystemList(obj);
	   
		double sys_mass = oapiGetMass(obj);
		double cm=sys_mass;

		for (i=0;i<c;i++) sys_mass+=oapiGetMass(list[i]);

		vel=vel*(cm/sys_mass);
		pos=pos*(cm/sys_mass);

		for (i=0;i<c;i++) {

		    double mps=oapiGetMass(list[i])/sys_mass;

			oapiGetGlobalPos(list[i],&gp);
			oapiGetGlobalVel(list[i],&gv);

			pos+=(gp*mps);		
			vel+=(gv*mps);
		}

		*position=pos;
		*velocity=vel;

		return true;
	}
	return false;
}


bool SystemVectors(OBJHANDLE obj, OBJHANDLE ref, VECTOR3 *position, VECTOR3 *velocity)
{
	VECTOR3 pos,vel,gp,gv;
    int i;

    if (obj==NULL || position==NULL || velocity==NULL) return false;

	oapiGetRelativePos(obj,ref,&pos);
	oapiGetRelativeVel(obj,ref,&vel);

	*position=pos;
	*velocity=vel;

	if (obj==Refer->StarHandle) return true;

	int c=Refer->GetSystemCount(obj);

	if (c>0) {
		
		OBJHANDLE *list=Refer->GetSystemList(obj);
	   
		double sys_mass = oapiGetMass(obj);
		double cm=sys_mass;

		for (i=0;i<c;i++) sys_mass+=oapiGetMass(list[i]);

		vel=vel*(cm/sys_mass);
		pos=pos*(cm/sys_mass);

		for (i=0;i<c;i++) {

		    double mps=oapiGetMass(list[i])/sys_mass;

			//OBJHANDLE r=Refer->GetReference(list[i]);
			oapiGetRelativePos(list[i],ref,&gp);
			oapiGetRelativeVel(list[i],ref,&gv);

			pos+=(gp*mps);		
			vel+=(gv*mps);
		}

		*position=pos;
		*velocity=vel;

		return true;
	}
	return false;
}


// WARNING: This function requires Orbit class and Reference class

// TODO: FIX The System Issue

void GetGlobalPosAfterTime(OBJHANDLE obj,VECTOR3 *pos,VECTOR3 *vel,double time,bool sys)
{	
	class Orbit O;

	if (obj==NULL || pos==NULL || vel==NULL) return;

    *pos = *vel = _V(0,0,0);

	OBJHANDLE ref;
	OBJHANDLE sun=Refer->StarHandle;

	while(obj!=sun && obj!=NULL) {

		ref=Refer->GetReference(obj);

		if (ref==sun && sys && Refer->GetSystemCount(obj)>0) {
			VECTOR3 p,v;
			double myy=oapiGetMass(sun) * GC;
			SystemVectors(obj,&p,&v);
			O.Elements(p,v,myy);
		}
		else O.Elements(obj,ref);


		double trl=O.TrlByTime(time);

		*vel += O.Tangent(trl);
		*pos += O.Position(trl);

		obj=ref;	 	
	}
}


double EccOfOrbit(VECTOR3 r, VECTOR3 v,double myy)
{	
	
	double vel2 = dotp(v,v);
    double rad  = length(r);
    VECTOR3 majv =( r * (vel2-myy/rad) ) - (v * dotp(r,v));
    return length(majv)/myy;
}



// Compute new position and velocity vectors after a time stp

void NewPosition(VECTOR3 *pos, VECTOR3 *vel, double myy, double stp)
{ 
	VECTOR3 r=*pos, v=*vel;

	// angular momentum
	VECTOR3 n   = crossp(r,v); 
    double  h   = sqrt(dotp(n,n));

	double vel2 = dotp(v,v);
    double rad  = sqrt(dotp(r,r));
    double mpr  = myy/rad; 

    // Semi-major axis, negative on hyperbola
	double sma  = -myy / ( vel2 - 2 * mpr); 
    double mnm  = sqrt(myy/fabs(sma*sma*sma));

	 // parameter
	double par = (h*h) / myy;

	// eccentricity
	double ecc  = sqrt( (-par / sma) + 1 );

	double drv  = dotp(r,v);

	VECTOR3 ma  = (r * (vel2-mpr)  -  v * drv) * ( 1 / (myy*ecc) );
    VECTOR3 mi  = crossp(n*(1/h),ma);
    
	double ac = (par - rad) / ( rad * ecc );
	if (ac<-1) ac =-1;
	if (ac>1)  ac = 1;

	double tra = acos( ac );
	if (drv<0) tra=PI2-tra;

	double mna = tra2mna(tra,ecc);


    // END POINT
	double nxt;
    if (ecc<1) nxt = mna2tra(limit(mna + stp * mnm), ecc);
	else       nxt = mna2tra(      mna + stp * mnm , ecc); 
	
	double cos_nxt = cos(nxt);
	double sin_nxt = sin(nxt);
      
	double nr = par / (1 + ecc * cos_nxt );
	double nv = sqrt( 2*myy / nr - myy / sma);

	*pos = ma * (nr*cos_nxt) + mi * (nr*sin_nxt); 

    double as = h / (nr*nv);
	if (as<-1) as =-1;
	if (as>1)  as = 1;

	double tg = asin( as );
	if (nxt>PI) tg=PI-tg;

    double tga=limit( nxt + tg );

    *vel = ma * ( nv * cos(tga) ) + mi * ( nv * sin(tga) ); 
}

void DrawEllipse(oapi::Sketchpad *skp,double x,double y,double xx,double yy,double w,double h)
{
	if (xx<0 || x>w || y>h || yy<0) return;
	if (x<-3000 || xx>3000 || yy>3000 || y<-3000) return;
	skp->Ellipse((int)x,(int)y,(int)xx,(int)yy);
}


void DrawRectangle(oapi::Sketchpad *skp,double x,double y,double xx,double yy,double w,double h)
{
	if (xx<0 || x>w || y>h || yy<0) return;
	skp->Rectangle((int)x,(int)y,(int)xx,(int)yy);
}

bool DrawLine(oapi::Sketchpad *skp,double x,double y,double xx,double yy,double w,double h,bool ex)
{
	
	double x0,y0,x1,y1;
	bool a=true,b=true;

	double dx=xx-x;
	double dy=yy-y;

    double left=1;
	double right=w-2;
	double top=1;
	double bottom=h-2;


    // Extend endpoint
	if (ex) xx=xx+dx*400, yy=yy+dy*400;

    if (x<left  || x>right  || y<top  || y>bottom)  a=false;
	if (xx<left || xx>right || yy<top || yy>bottom) b=false;


    if (a && b) {   // Entirely inside
		skp->MoveTo((int)x, (int)y);
		skp->LineTo((int)xx,(int)yy);
		return true;
	}


    if (!a && !b) {  // Totaly Outside
		if (x<left   && xx<left)   return false;
		if (x>right  && xx>right)  return false;
		if (y<top    && yy<top)    return false;
		if (y>bottom && yy>bottom) return false;
	}

	// Boundry line cross sections

    double xa=(top-y)   * dx/dy + x;
	double xw=(bottom-y)* dx/dy + x;

	double ya=(left-x)  * dy/dx + y;
	double yh=(right-x) * dy/dx + y;

	
    // First point
	if      (xa>=left && xa<=right)  x0=xa,   y0=top; 
	else if (xw>=left && xw<=right)  x0=xw,   y0=bottom;
	else if (ya>=top  && ya<=bottom) x0=left, y0=ya; 
    else if (yh>=top  && yh<=bottom) x0=right,y0=yh; 
	else return false; // Line not visible


	// Second point
	if      (yh>=top  && yh<=bottom) x1=right,y1=yh; 	
	else if (ya>=top  && ya<=bottom) x1=left, y1=ya; 
	else if (xw>=left && xw<=right)  x1=xw,   y1=bottom;
	else if (xa>=left && xa<=right)  x1=xa,   y1=top;
	else return false;
    
	if (!a && !b) x=x0,y=y0,xx=x1,yy=y1;	
    
	else
	if (!a) {  // Select closest point for starting
		if (((x-x0)*(x-x0) + (y-y0)*(y-y0)) < ((x-x1)*(x-x1) + (y-y1)*(y-y1))) x=x0,y=y0;
		else x=x1,y=y1;
	}

	else 
	if (!b) {  // Select closest point for end
		if (((xx-x0)*(xx-x0) + (yy-y0)*(yy-y0)) < ((xx-x1)*(xx-x1) + (yy-y1)*(yy-y1))) xx=x0,yy=y0;
		else xx=x1,yy=y1;
	}	
    

    skp->MoveTo((int)x, (int)y);
	skp->LineTo((int)xx,(int)yy);

	return true;
}



double CalculateSOI(OBJHANDLE obj, OBJHANDLE ref)  
{
	if (obj==NULL || ref==NULL) return 0;
	if (obj==ref) return 100*AU;

	double myyref = oapiGetMass(ref) * GC;
	double myy    = oapiGetMass(obj) * GC;

	VECTOR3 pos;
    oapiGetRelativePos(obj,ref,&pos);

	double rad = ( length(pos) * sqrt(myy) ) / ( sqrt(myy) + sqrt(myyref) );
	
	return(rad);
}





void DrawDirectionSelector(oapi::Sketchpad *skp,VESSEL *ship,VECTOR3 vel,int width,int height)
{
        oapi::Pen *pen1=oapiCreatePen(1,1,dgreen); 
		oapi::Pen *pen2=oapiCreatePen(1,1,green); 
		oapi::Pen *pen3=oapiCreatePen(1,1,grey); 

		VECTOR3 thrustdir=_V(0,0,1);
		VECTOR3 accel_loc=_V(0,0,0)-thrustdir;
		VECTOR3 upwards_loc=_V(0,-1,0);
		VECTOR3 side_loc=_V(1,0,0);
      
		VECTOR3 accel,upwards,side;

		ship->GlobalRot(accel_loc,accel);
		ship->GlobalRot(upwards_loc,upwards);
		ship->GlobalRot(side_loc,side);

	
		double line=dot(vel,accel)/length(accel);

		double vert=dot(vel,upwards)/(length(upwards)*length(vel));
		double horiz=dot(vel,side)/(length(side)*length(vel));

        skp->SetPen(pen1);

		skp->MoveTo(20,height/2);
		skp->LineTo(width/2 - 20, height/2);

		skp->MoveTo(width/2 + 20 ,height/2);
		skp->LineTo(width - 20, height/2);

		skp->MoveTo(width/2, 20);
		skp->LineTo(width/2, height/2 - 20);

		skp->MoveTo(width/2, height/2 + 20);
		skp->LineTo(width/2, height - 20);

		skp->Ellipse(width/2 - 90,height/2 - 90,width/2 + 90,height/2 + 90);
		skp->Ellipse(width/2 - 20,height/2 - 20,width/2 + 20,height/2 + 20);


        int x=(int)(horiz * width/2);
		int y=(int)(vert * height/2);

		skp->SetPen(pen2);

		skp->MoveTo(width/2, height/2);
		skp->LineTo(width/2 + x , height/2 + y);

        if (line<0) {
		
			if (abs(x)<4 && abs(y)<4) skp->SetPen(pen3);

			skp->MoveTo(width/2 + x - 16, height/2 + y);
			skp->LineTo(width/2 + x + 16, height/2 + y);

			skp->MoveTo(width/2 + x , height/2 + y - 16);
			skp->LineTo(width/2 + x , height/2 + y + 16);

		}

		//SelectObject(hDC,GetStockObject(WHITE_PEN));
  
		oapiReleasePen(pen1); 
		oapiReleasePen(pen2);  
		oapiReleasePen(pen3);
}


void DrawLatitude(oapi::Sketchpad *skp,OBJHANDLE handle,Orbit *prj,VECTOR3 rot,VECTOR3 off,double Lat,double zoom,double w,double h)
{

    rot=unit(rot);
	off=unit(off);
	
	VECTOR3 per = unit(crossp(rot,off));
    VECTOR3 pos;
	double  rad = oapiGetSize(handle);
    double  stp = 10*RAD;
    double  xc=w/2;
	double  yc=h/2;
	double  Lon=0;
	double  sin_lat=sin(-Lat);
	double  cos_lat=cos(Lat);
	double  x,y,ox,oy,dot,old_dot;

	pos=unit( rot*sin_lat + off*(cos(Lon)*cos_lat) + per*(sin(Lon)*cos_lat) )*rad;
	prj->Point(pos,xc,yc,zoom,&ox,&oy);
	old_dot = dotp(pos,prj->norv);

	for (Lon=stp;Lon<6.29;Lon+=stp) {

		pos=unit( rot*sin_lat + off*(cos(Lon)*cos_lat) + per*(sin(Lon)*cos_lat) )*rad;
		prj->Point(pos,xc,yc,zoom,&x,&y);
		dot = dotp(pos,prj->norv);
		if (dot<0 && old_dot<0) {
			DrawLine(skp,x,y,ox,oy,w,h,false);
		}
		old_dot=dot;
		ox=x, oy=y;
	}	
}

void DrawLongitude(oapi::Sketchpad *skp,OBJHANDLE handle,Orbit *prj,VECTOR3 rot,VECTOR3 off,double Lon,double zoom,double w,double h)
{

    rot=unit(rot);
	off=unit(off);
	
	VECTOR3 per = unit(crossp(rot,off));
    VECTOR3 pos;
	double  rad = oapiGetSize(handle);
    double  stp = 5*RAD;
    double  xc=w/2;
	double  yc=h/2;
	double  Lat=0;
	double  sin_lon=sin(Lon);
	double  cos_lon=cos(Lon);
	double  x,y,ox,oy,dot,old_dot;
   
	pos=unit( rot*sin(-Lat) + off*(cos_lon*cos(Lat)) + per*(sin_lon*cos(Lat)) )*rad;
	prj->Point(pos,xc,yc,zoom,&ox,&oy);
	old_dot = dotp(pos,prj->norv);
    
	for (Lat=stp;Lat<6.29;Lat+=stp) {

        pos=unit( rot*sin(-Lat) + off*(cos_lon*cos(Lat)) + per*(sin_lon*cos(Lat)) )*rad;

		prj->Point(pos,xc,yc,zoom,&x,&y);
		dot = dotp(pos,prj->norv);
		if (dot<0 && old_dot<0) {
			DrawLine(skp,x,y,ox,oy,w,h,false);
		}
		old_dot=dot;
		ox=x, oy=y;
	}	
}

// NUMERICAL METHODS
VECTOR3 Accel(VECTOR3 *pos,double *mu,int no,int for_p)
{
    int j;
	double l;
	VECTOR3 dist,accel=_V(0,0,0);

	for(j=0;j<no;j++) { 

		// Exclude the planet to witch we are calculating acc-vector
		// Also exclude "planets" (ships) those have no gravity 

		if (j!=for_p && mu[j]>0) {  	
			
			// Compute distance vector
			dist = pos[j] - pos[for_p];

			// Compute distance
			l = sqrt(dotp(dist,dist));
					
			accel += dist * ( mu[j] / (l*l*l) );
		}
	}
	return accel;
}




void RungeKutta2(VECTOR3 *pos,VECTOR3 *vel,double *mu,int no,double step){
  int i;
  // non piu' di 15 oggetti
  if (no>15) 
	return;

  VECTOR3 a1[16],a2[16];
  VECTOR3 v1[16],v2[16];
  VECTOR3 pp[16];

  // Velocities and accelerations for planets
  for (i=0;i<no;i++){
    v1[i] = vel[i]; 
    a1[i] = Accel(pos,mu,no,i);
  }

  // Update planet position
  for (i=0;i<no;i++)
    pp[i] = pos[i] + v1[i] * (step/2);

  // Compute new Velocities and accelerations for planets
  for (i=0;i<no;i++){
    v2[i] = vel[i] + a1[i] * (step/2); 
    a2[i] = Accel(pp,mu,no,i);
  }

  for (i=0;i<no;i++) 
  	pos[i] += ( v2[i] * step );
  for (i=0;i<no;i++) 
	  vel[i] += ( a2[i] * step );
}


void RungeKutta1(VECTOR3 *pos,VECTOR3 *vel,double *mu,int no,double step){
	int x;
	if (no>15) 
    return;

  VECTOR3 a1[16];

  double step2p2 = step*step/2.0;
	
	// Velocities and accelerations for planets
	for (x=0;x<no;x++) 
    a1[x] = Accel(pos,mu,no,x);
  for (x=0;x<no;x++) 
    pos[x] += ( vel[x]*step + a1[x]*step2p2 );
	for (x=0;x<no;x++) 
    vel[x] += a1[x]*step;	
}




