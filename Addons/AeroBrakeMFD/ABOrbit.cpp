// ==============================================================
//  MFD ToolKit / OrbitClass for ORBITER SPACE FLIGHT SIMULATOR
//  (c) Jarmo Nikkanen 2002 - 2003
//
// ==============================================================

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
#include "Orbitersdk.h"
#include "ABToolkit.h"
#include "ABReference.h"
#include "ABOrbit.h"


double Bug,Bug2;
Orbit DebugOrbit;
VECTOR3 BugVector;

// CONSTRUCTORS

Orbit::Orbit()
{
	Reset();
	Ecliptic();
}

Orbit::Orbit(OBJHANDLE body,OBJHANDLE ref) 
{
	Elements(body,ref);
}


Orbit::Orbit(char *bo,char *re) 
{
	Elements(bo,re);
}



Orbit::Orbit(VECTOR3 r, VECTOR3 v,double myy)
{
	Elements(r,v,myy);
}


Orbit::~Orbit()
{	
}


void Orbit::Reset()
{
	LeoRef=NULL;
	GeoRef=NULL;
	EclipticPlane=false;
	displacement=_V(0,0,0);
	intersection=NULL;	
	node_trl_o=NULL;
	SetTime(0);
}


void Orbit::Displacement(VECTOR3 d)
{
	displacement=d;
}


void Orbit::Elements(OBJHANDLE body,OBJHANDLE ref) 
{
	VECTOR3 r,v;

    if (body==NULL || ref==NULL) return;

	double myy = oapiGetMass(ref) * GC;

	oapiGetRelativePos(body,ref,&r);
    oapiGetRelativeVel(body,ref,&v);

	Elements(r,v,myy);
}


void Orbit::Elements(char *bo,char *re) 
{
	VECTOR3 r,v;

	OBJHANDLE body=oapiGetObjectByName(bo);
	OBJHANDLE ref=oapiGetObjectByName(re);

    if (body==NULL || ref==NULL) return;

	double myy = oapiGetMass(ref) * GC;

	oapiGetRelativePos(body,ref,&r);
    oapiGetRelativeVel(body,ref,&v);

	Elements(r,v,myy);
}


void Orbit::Elements(VECTOR3 r, VECTOR3 v,double m,bool reset_proj)
{	
	
	Reset();

	if (fabs(r.x)<0.01) r.x=0;
	if (fabs(r.y)<0.01) r.y=0;
	if (fabs(r.z)<0.01) r.z=0;

	if (fabs(v.x)<0.01) v.x=0;
	if (fabs(v.y)<0.01) v.y=0;
	if (fabs(v.z)<0.01) v.z=0;

	vv=v;
	rv=r;

	double vel2;
	VECTOR3 z; z.x=0; z.z=0; z.y=1;
   
	vel2 = dotp(v,v);
	vel  = sqrt(vel2);
    rad  = length(r);
    myy  = m;

    // Semi-major axis, negative on hyperbola
	//sma  = -m / ( vel2 - 2 * m / rad); 


    // Computer normal vector and vector pointing ascenting node
    norv	= crossp(r,v);
	ang		= length(norv);
	norv	= norv*(1.0/ang);

    VECTOR3 ANv = unit(crossp(z,norv));
    
	
	 // Inclination
    inc = acos(-norv.y);
    par = ang*ang/myy; 
    majv =( r * (vel2-myy/rad) ) - (v * dotp(r,v));

    double ml=length(majv);

    ecc  = ml/myy;
	
    if (ecc<1e-6) ecc=0;
	if (inc<1e-6) inc=0;

	sma = par / (1-ecc*ecc);


	r=r*(1.0/rad);					// Make the radius vector to unit size, After computing ecc-vector

	if (inc==0) ANv=_V(1,0,0);		// Place ANv to vernal equinox
	if (ecc!=0) majv=majv*(1.0/ml); // Make the major vector to unit size	
	else majv=ANv;					// Place major vector to ascenting node
	


	// Longitude of ascenting node
	if (inc!=0) {
		double  x=ANv.x;
		if      (x>=1)  lan=0;
		else if (x<=-1) lan=PI;
		else {		
			lan=acos(x);		
			if (ANv.z<0) lan=PI2-lan;		
		}
	}
	else lan=0.0;
	

	// Argument of periapis
	if (inc!=0 && ecc!=0) {
		double x=dot(ANv,majv);
		if (x>1) x=1; // Avoid some precision problems
		else if (x<-1) x=-1;
		agp=acos(x);
		if (majv.y<0) agp=PI2-agp;
	}
	else 
	if (ecc!=0) {
		agp=acos(majv.x);
		if (majv.z<0) agp=PI2-agp;
	}
	else agp=0.0;



    // True anomaly
	if (ecc!=0) {
		double x=dot(majv,r);
		if      (x>=1)  tra=0;
		else if (x<=-1) tra=PI;
		else {
			tra=acos(x);
			x=dot(r,v);
			if (fabs(x)<1e-6) x=0; // Avoid some precision problems
			if (x<0) tra=PI2-tra;
		}
    }
	else 
	if (inc!=0) {
		tra=acos(dot(ANv,r));
		if (dot(ANv,v)>0) tra=PI2-tra;
	}
	else {
		tra=acos(r.x);
		if (v.x>0) tra=PI2-tra;
	}


    // Longitude of periapis
	lpe=limit(agp+lan);

    // True longitude
	trl=limit(lpe+tra);
	minv=unit(crossp(norv,majv));
	mna=tra2mna(tra,ecc);

	mnm=MeanMotion();

	Xmajv = majv * sma;
	Xminv = minv * sqrt(fabs(sma*par));
	Xcv	  = majv * (sma*ecc);

	if (reset_proj) SetProjection(this);
}





void Orbit::SetOrbitPosition(double t)
{
	VECTOR3 x=Position(t), xx=Tangent(t);
	rv=x, vv=xx;
	trl = t;
	tra = limit(trl-lpe);
	mna = tra2mna(tra,ecc);
}




void Orbit::Ecliptic()
{	

	if (EclipticPlane) return;

	EclipticPlane=true;

	Reset();

    myy = 1.327e20;

    ecc = 0;
	sma = AU;
	rad = AU;
    inc = 0;
    lan = 0;
	agp = 0;
    tra = 0;

	par = sma;
    mna = 0;
	mnm = MeanMotion();
	ang = sqrt(myy*par);

	lpe=0;
	trl=0;

	majv=_V(1,0,0);
	minv=_V(0,0,1);
	norv=_V(0,-1,0);

	Xmajv = majv * sma;
	Xminv = minv * sma;
	Xcv	  = _V(0,0,0);

	SetProjection(this);
}

void Orbit::SetNULL()
{
	sma=0;
}

double Orbit::Energy()
{
	return -myy/(2*sma);
}

bool Orbit::Defined()
{
	if (sma!=0) return true;
	return false;
}


void Orbit::LEO(OBJHANDLE ref)
{	
	if (!ref) {
		LeoRef=NULL;
		return;
	}

	//if (LeoRef=ref) return;

	double obli  = oapiGetPlanetObliquity(ref);
	double peri  = oapiGetPlanetPeriod(ref);
	double trans = oapiGetPlanetTheta(ref);

	VECTOR3 rota, refd;
	PlanetAxis(obli,trans,&rota,&refd);

	VECTOR3 velo = crossp(rota,refd);
	double  myy  = oapiGetMass(ref)*GC;
	double  rad  = oapiGetSize(ref);
	double  vel  = sqrt(myy/rad);

	refd=set_length(refd,rad);
	velo=set_length(velo,vel);
	if (peri<0) velo=_V(0,0,0)-velo;

	Elements(refd,velo,myy);
	LeoRef=ref;
}


void Orbit::GEO(OBJHANDLE ref)
{	
	if (!ref) {
		GeoRef=NULL;
		return;
	}

	//if (GeoRef=ref) return;

	double obli  = oapiGetPlanetObliquity(ref);
	double peri  = oapiGetPlanetPeriod(ref);
	double trans = oapiGetPlanetTheta(ref);

	VECTOR3 rota, refd;
	PlanetAxis(obli,trans,&rota,&refd);

	VECTOR3 velo = crossp(rota,refd);
	double  myy  = oapiGetMass(ref)*GC;
	double  rad  = pow(fabs(peri)*sqrt(myy)/PI2, 2.0 / 3.0);
	double  vel  = sqrt(myy/rad);

	refd=set_length(refd,rad);
	velo=set_length(velo,vel);
	if (peri<0) velo=_V(0,0,0)-velo;

	Elements(refd,velo,myy);
	GeoRef=ref;
}


// Radius vector will point in vernal equinox
void Orbit::CreateNewCircularOrbit(Orbit *plane,double rad)
{
	double mu=plane->myy;
	double vcir=sqrt(mu/rad);
	VECTOR3 rv=plane->Position(0);
	VECTOR3 vv=crossp(plane->norv,rv);
	Elements(set_length(rv,rad),set_length(vv,vcir),mu,false);
}








bool Orbit::DefineOrbit(OBJHANDLE ref, VECTOR3 pos, VECTOR3 normal,double tra,double ecc)
{

	VECTOR3 ma  = create_vector(normal,pos,PI2-tra);
	VECTOR3 mi  = unit(crossp(normal,ma));

	double x,y;
    double myy = oapiGetMass(ref) * GC;
	double rad = length(pos);

    double par = rad * (1+ecc*cos(tra));

	double sma = par / (1-ecc*ecc);

	double vel = sqrt(2*myy/rad - myy/sma); // Ship's velocity	
	double smi = sqrt( fabs(sma) * par );
	double eca = tra2eca(tra,ecc);	

	INVALIDD(par*sma*vel) return false;

	if (ecc<1) { 
		x = -sma * sin(eca);
		y =  smi * cos(eca);
	}
	else {
		x = sma * sinh(eca);
		y = smi * cosh(eca);
	}
  
	double l = sqrt(x*x + y*y);

	VECTOR3 an = ma * (vel * x / l);
	VECTOR3 di = mi * (vel * y / l);
	
	Elements(pos,an+di,myy,false);

	return true;
}



void Orbit::CreateProjectionPlane(VECTOR3 normal,VECTOR3 zero)
{	
	
	Reset();

    myy = 1.327e20;

    ecc = 0;
	sma = AU;
	rad = AU;
    inc = 0;
    lan = 0;
	agp = 0;
    tra = 0;

	par = sma;
    mna = 0;
	mnm = MeanMotion();
	ang = sqrt(myy*par);

	lpe=0;
	trl=0;

	if (normal.y>0) normal*=(-1);

	majv=unit(crossp(crossp(normal,zero),normal));
	norv=unit(normal);
    minv=unit(crossp(norv,majv));

	Xmajv = majv * sma;
	Xminv = minv * sma;
	Xcv	  = _V(0,0,0);

	SetProjection(this);
}




// Hyberbolic use only
// ReCalculate orbit position (mna,tra,trl,rad,vel)
void Orbit::SetTimeToPeriapis(double t)
{
	if (Defined()) {
		mna = -t * MeanMotion();
		tra = mna2tra(mna,ecc);
		trl = limit(tra+lpe);
		vv  = Tangent(trl);
		rv  = Position(trl);
		rad = length(rv);
		vel = length(vv);
	}
}




void Orbit::SetTime(double t)
{
	if (Defined()) {
		timeto=0;
		time=oapiGetSimMJD()+(t/86400);
	}
}



double Orbit::GetTime()
{
	if (Defined()) {
		if (timeto>0) return timeto;
		timeto=(time-oapiGetSimMJD())*86400;
		if (timeto<0) timeto=0;
		return timeto;
	}
	else return 0;
}



double Orbit::MeanMotion()
{
	if (Defined()) return(sqrt(myy/(fabs(sma*sma*sma))));
	return 0;
}


double Orbit::Period()
{
	if (Defined()) return (PI2*sqrt(fabs(sma*sma*sma)/myy));
	return 0;
}



double Orbit::TimeToPeriapis()
{
	if (Defined()) {
		// For ellipse
		if (ecc<1) return( (PI2-mna) / mnm );

		// For hyperbola
		return( (-mna) / mnm );	
	}
	return 0;
}



double Orbit::TimeToNode(Orbit *tgt,double *trl_of_node)
{
	if (Defined()) {
		double hpa=TrlOfNode(tgt);
		double hpd=limit(hpa+PI);

		// Avoid oscilation when the ship is at the node.
		if (fabs(hpa-trl)<1e-6) hpa=hpd;
		if (fabs(hpd-trl)<1e-6) hpd=hpa;

		double tn;
		double a=TimeTo(hpa);
		double d=TimeTo(hpd);
		
		if (ecc>1) {
			if (a>0 && d>0) tn=MIN(a,d);
			else tn=MAX(a,d);
		}
		else tn=MIN(a,d);
		
		
		if (trl_of_node) {
			if (tn==a) *trl_of_node=hpa;
			else if (tn==d) *trl_of_node=hpd;
			else *trl_of_node=0;
		}
		return(tn);
	}
	return 0;
}




double Orbit::PeriapisVelocity()
{
	if (Defined()) return( sqrt( myy * (1+ecc) / (sma * (1-ecc)) ) );
	return 0;
}

double Orbit::TangentialAngle(double longitude)
{
	if (ecc==0) return PI05;
	return( tra2tan( limit(longitude-lpe), ecc) );
}



double Orbit::PeriapisDistance()
{
	return(sma*(1-ecc));
}


// There are no apoapis on hyperbola but
// return value anyway

double Orbit::ApoapisDistance()
{
	return(sma*(1+ecc));
}



double Orbit::TrlOfNode(Orbit *tgt)
{
	return TrlOfNode(tgt->norv);
}



double Orbit::TrlOfNode(VECTOR3 tgt_norv)
{
	if (Defined()) {
		//VECTOR3 anv=crossp(norv, tgt_norv);
		//if (dotp(anv,anv)==0) return 0;
		//return Longitude(anv);

		
		VECTOR3 z; z.x=0; z.z=0; z.y=1;
		VECTOR3 vlan,vect;

		if (length(crossp(z,norv))==0) vlan=majv, vect=minv;
		else {
			vlan = unit(crossp(z,norv));
			vect = unit(crossp(norv,vlan));
		}

		VECTOR3 lv = crossp(norv, tgt_norv);
		
		double xf=dot(lv,vect);                
		double xl=dot(lv,vlan); 
			
		return limit(atan2(xf,xl)+lan);
		
	}
	return 0;
}



// Tangent vector is also velocity vector
VECTOR3 Orbit::Tangent(double longitude)
{	
	double tr=limit(longitude-lpe); // Must be limited  
	double up=sqrt(myy/par);
	return majv*(-sin(tr)*up) + minv*((ecc+cos(tr))*up);
}



VECTOR3 Orbit::Position(double longitude)
{
    double n=limit(longitude-lpe);
	double cos_n=cos(n); 
	double r = par / ( 1 + ecc * cos_n ); 
	
	// This is wrong. 
	if (r<0) r=-r;

	return(majv * (r * cos_n) + minv * (r * sin(n)));
}


VECTOR3 Orbit::EscapeAsymptote()
{
	if (ecc<1) return _V(0,0,0);
	double n=limit(PI-acos(1/ecc));
	return(majv * cos(n) + minv * sin(n));
}


VECTOR3 Orbit::Asymptote()
{
	if (ecc<1) return _V(0,0,0);
	double n=limit(PI+acos(1/ecc));
	return(majv * cos(n) + minv * sin(n));	
}


double Orbit::TimeTo(double longitude)
{   
	if (longitude==trl) return GetTime();

    double tr=limit( longitude - lpe );

	// For Hyperbola
	if (ecc>1) {
		double max=MaxTra();
		if (tr>max && tr<(PI2-max)) return(-10e20);
		double ang = tra2mna(tr,ecc) - mna;	
		return(GetTime() + (ang / mnm) );
	}

    // For Ellipse
	double ang=angular_distance(mna, tra2mna(tr,ecc));	
	return( GetTime() + (ang / mnm) );
}
  


// Return negative value after point passage
double Orbit::TimeToPoint(double longitude)
{   
    double tr=limit( longitude - lpe );

	// For Hyperbola
	if (ecc>1) {
		double max=MaxTra();
		if (tr>max && tr<(PI2-max)) return(-10e20);
		double ang = tra2mna(tr,ecc) - mna;	
		return(GetTime() + (ang / mnm) );
	}


    // For Ellipse
	double ang = angular_distance(mna, tra2mna(tr,ecc));
	if (ang>PI) return -(PI2-ang)/mnm;
	return( ang/mnm );
}
  


bool Orbit::IsValid(double longitude)
{   
	if (ecc>1) {
		double tr=limit( longitude - lpe );
		double max=MaxTra();
		if (tr>max && tr<(PI2-max)) return false;
	    return true;
	}
    return true;
}
    


double Orbit::TrlByRadius(double r)
{
	double z = (par-r) / (r*ecc);
	if (z>1) z=1;
	if (z<-1) z=-1;
	
	return limit( acos(z) + lpe );
}



double Orbit::TrlByTime(double ti)
{
	if (ti==0) return trl;
	if (ecc<1) 
	return ( limit( mna2tra(limit(mna + ti * mnm), ecc) + lpe ) );
	return ( limit( mna2tra(      mna + ti * mnm , ecc) + lpe ) ); // Can not limit with hyperbola
}



double Orbit::Radius(double longitude)
{  
	if (ecc==0) return par;
	double v = par / ( 1 + (ecc * cos( limit(longitude - lpe) ) ) ); 
	if (v<0.0) return -v;
	return v;
}



double Orbit::Velocity(double longitude)
{
	double r = par / ( 1 + (ecc * cos( limit(longitude - lpe) ) ) ); 
	return( sqrt( ( 2 * myy / r) - (myy / sma)) ); 		
}



VECTOR3 Orbit::Velocity()
{
	return vv;
}


VECTOR3 Orbit::Position()
{
	return rv;
}





double Orbit::VelocityByRadius(double r)
{
	return( sqrt( ( 2 * myy / r) - (myy / sma)) );
}




double Orbit::SemiMinor()
{
	if (ecc<1) return sqrt( sma * par );
	return sqrt(-sma * par);	
}



// Private
double Orbit::MaxTra()
{
	double r=50*AU;
	return(acos( (-sma*(ecc*ecc-1)) / (ecc*r) - (1.0/ecc) ));
}



void Orbit::Longitude(VECTOR3 pr,double *rad,double *height,double *angle)
{	
	double	xf=dotp(pr,minv); 
	double	xl=dotp(pr,majv);
    
	if (rad)	*rad   = sqrt((xl*xl) + (xf*xf));  	    
    if (height) *height= dotp(pr,norv);
	if (angle)	*angle = limit(atan2(xf,xl)+lpe);
}


double Orbit::Longitude(VECTOR3 pr)
{	
	double	xf=dotp(pr,minv); 
	double	xl=dotp(pr,majv);
	return limit(atan2(xf,xl)+lpe);
}



double Orbit::Translate(Orbit *tgt,double longitude)
{
	VECTOR3 lv=Position(longitude);	

	double xf=dot(lv,tgt->minv); 
	double xl=dot(lv,tgt->majv); 

	return limit(atan2(xf,xl)+tgt->lpe);
}



void Orbit::SetProjection(Orbit *p)
{	
	zero = unit(p->Position(0));
	nine = unit(p->Position(90*RAD));
}


//Private
bool Orbit::IsOut(double cx,double z)
{
	if (!Defined()) return true;

	double d=(3*cx)/z;
	double f=length(displacement)-(ApoapisDistance()*3);
	if (d>f) return false;
	return true;
}


//Private
bool Orbit::DrawAll(double w,double z)
{
	if ((sma*z)<w) return true;
	return false;
}


//Private
bool Orbit::IsSmall(double w,double z)
{
	if ((ApoapisDistance()*z)<3) return true;
	return false;
}






// This will draw an orbit with all the spices
//op = orbit to draw, t = projection orbit,  cx,cy = MFD Center, siz = scale factor
//ref = reference planet or NULL, dsp = displacement vector or NULL
//What To Draw:  pe = periapis, rad = radius line, lan = line of nodes

void Orbit::Draw(oapi::Sketchpad *skp,int color,double cx,double cy,double siz,OBJHANDLE ref,
				bool pe,bool ra,bool la) 
{	
	if (!Defined()) return;

    double w=cx*2;
	double h=cy*2;
    double x,y;
	  
    double s=3;
	double r;

//	LOGBRUSH br;
//	br.lbStyle=BS_HOLLOW;

//	HBRUSH none=CreateBrushIndirect(&br);
	oapi::Brush *none=oapiCreateBrush(0x00c0c0c0);    //FIXME : HOLLOW???
	oapi::Pen *dash=oapiCreatePen(1,1,color);     //FIXME : dotted
    oapi::Pen *pen=oapiCreatePen(1,1,color);  
    oapi::Brush *brush=oapiCreateBrush(color);
	oapi::Brush *brushb=oapiCreateBrush(0);
	oapi::Pen *greypen=oapiCreatePen(1,1,dgrey); 
   

    // HYPERBOLIC ORBIT

    if (ecc>1) {

	//angle=lpe;

	if (length(displacement)>0) {
		Point(displacement,w/2,h/2,siz,&cx,&cy);
		cx=ceil(cx);
		cy=ceil(cy);
	}

    if (ref) {
			skp->SetPen(greypen);
			skp->SetBrush(none);
			r=oapiGetSize(ref)*siz;
			DrawEllipse(skp, cx-r,cy-r,cx+r,cy+r, w, h);		
	}

    skp->SetPen(pen);

    XDrawHyperbola(skp,cx,cy,w,h,siz);

	skp->SetBrush(brush);
	
	// Periapis
	if (pe) {
		Point(cx,cy,lpe,&x,&y,siz);
		DrawEllipse(skp,x-s,y-s,x+s,y+s,w,h);
	}

	// Radius
	if (ra) {
		Point(cx,cy,trl,&x,&y,siz);
		skp->Line(cx,cy,x,y);
	}


	// Lan
    if (la) {
		Point(cx,cy,lan+PI,&x,&y,siz);
		skp->SetBrush(none);
		DrawRectangle(skp,x-s,y-s,x+s,y+s,w,h);
   
		double xx=x;
		double yy=y;	

		skp->SetBrush(brush);
		Point(cx,cy,lan,&x,&y,siz);
		DrawRectangle(skp,x-s,y-s,x+s,y+s,w,h);

		skp->SetPen(dash);
		DrawLine(skp,xx,yy,x,y,w,h,false);
    }

	}





	// ELLIPTIC ORBIT

    if (ecc<1 && !IsOut(cx,siz) && !IsSmall(cx,siz)) { 
      
	if (length(displacement)>0) {
		Point(displacement,w/2,h/2,siz,&cx,&cy);
		cx=ceil(cx);
		cy=ceil(cy);
	}

	if (ref) {
		skp->SetPen(greypen);
		skp->SetBrush(none);
		r=oapiGetSize(ref)*siz;
		DrawEllipse(skp,cx-r,cy-r,cx+r,cy+r,w,h);	
	}


    skp->SetPen(pen);	
    XDrawEllipse(skp,w,h,siz);


    // Radius
	if (ra) {
		Point(cx,cy,trl,&x,&y,siz);
		DrawLine(skp,cx,cy,x,y,w,h,false);
	}
    
    // Periapis
	if (pe) {
		skp->SetBrush(brush);
		Point(cx,cy,lpe,&x,&y,siz);
		DrawEllipse(skp,x-s,y-s,x+s,y+s,w,h);
	}


	// LAN
	if (la) {
		Point(cx,cy,lan+PI,&x,&y,siz);
		skp->SetBrush(none);
		DrawRectangle(skp,x-s,y-s,x+s,y+s,w,h);
   
		double xx=x;
		double yy=y;	


		skp->SetBrush(brush);

		Point(cx,cy,lan,&x,&y,siz);
		DrawRectangle(skp,x-s,y-s,x+s,y+s,w,h);
		skp->SetPen(dash);
		DrawLine(skp,x,y,xx,yy,w,h,false);	
    }


    // Apoapis
	if (pe) {
		skp->SetBrush(brushb);
		skp->SetPen(pen);	
		Point(cx,cy,lpe+PI,&x,&y,siz);	
		DrawEllipse(skp,x-s,y-s,x+s,y+s,w,h);
	}

    }

//    SelectObject(hDC,GetStockObject(NULL_BRUSH));
//	SelectObject(hDC,GetStockObject(WHITE_PEN));

	oapiReleaseBrush(none);
	oapiReleasePen(dash);     
    oapiReleasePen(pen);  
    oapiReleaseBrush(brush);
	oapiReleaseBrush(brushb);
	oapiReleasePen(greypen);
}





void Orbit::DrawPlaneIntersection(oapi::Sketchpad *skp,Orbit *t,double cx,double cy,double zoom,int color,bool box)
{

	if (!Defined()) return;

	double x,y,x2,y2,hpa,s,hpd;
	double w=cx*2;
	double h=cy*2;

	if (t==NULL) return;

    oapi::Pen *pen=oapiCreatePen(1,1,color);  //FIXME: DOTTED
	oapi::Pen *spen=oapiCreatePen(1,1,color); 
    oapi::Brush *brush=oapiCreateBrush(color);
	oapi::Brush *brushb=oapiCreateBrush(0);

    hpa=TrlOfNode(t);
	hpd=limit(hpa+PI);

	if (hpa!=-1 && hpd!=-1) {
        
		s=3.0;

		if (length(displacement)>0) Point(displacement,cx,cy,zoom,&cx,&cy);
		
		Point(cx,cy,hpd,&x,&y,zoom);
		Point(cx,cy,hpa,&x2,&y2,zoom);

		skp->SetPen(pen);
		DrawLine(skp,x,y,x2,y2,w,h,false);

		skp->SetPen(spen);
		skp->SetBrush(brushb);
		if (box) DrawRectangle(skp,x-s,y-s,x+s,y+s,w,h);
	
		skp->SetBrush(brush);
		if (box) DrawRectangle(skp,x2-s,y2-s,x2+s,y2+s,w,h);	
	}

//	SelectObject(hDC,GetStockObject(NULL_BRUSH));
//	SelectObject(hDC,GetStockObject(WHITE_PEN));
  
    oapiReleasePen(pen); 
    oapiReleasePen(spen);  
    oapiReleaseBrush(brush);
	oapiReleaseBrush(brushb);
}



void Orbit::DrawPlaneIntersection(oapi::Sketchpad *skp,double hpa,double cx,double cy,double zoom,int color,bool box)
{
    if (!Defined()) return;

	double x,y,x2,y2,s,hpd;
	double w=cx*2;
	double h=cy*2;


    oapi::Pen *pen=oapiCreatePen(1,1,color);  //FIXME : DOTTED
	oapi::Pen *spen=oapiCreatePen(1,1,color); 
    oapi::Brush *brush=oapiCreateBrush(color);
	oapi::Brush *brushb=oapiCreateBrush(0);

	hpd=limit(hpa+PI);

	if (hpa!=-1 && hpd!=-1) {
        
		s=3.0;

		if (length(displacement)>0) Point(displacement,cx,cy,zoom,&cx,&cy);
		
		Point(cx,cy,hpd,&x,&y,zoom);
		Point(cx,cy,hpa,&x2,&y2,zoom);

		skp->SetPen(pen);
		DrawLine(skp,x,y,x2,y2,w,h,false);

		skp->SetPen(spen);
		skp->SetBrush(brushb);
		if (box) DrawRectangle(skp,x-s,y-s,x+s,y+s,w,h);
	
		skp->SetBrush(brush);
		if (box) DrawRectangle(skp,x2-s,y2-s,x2+s,y2+s,w,h);	
	}

//	SelectObject(hDC,GetStockObject(NULL_BRUSH));
//	SelectObject(hDC,GetStockObject(WHITE_PEN));
  
    oapiReleasePen(pen); 
    oapiReleasePen(spen);  
    oapiReleaseBrush(brush);
	oapiReleaseBrush(brushb);
}


void Orbit::Point(VECTOR3 pr,double xp,double yp,double s,double *x,double *y)
{
	*x = xp + (dot(pr,zero) * s);
	*y = yp - (dot(pr,nine) * s);	
}


void Orbit::Point(double xp,double yp,double l,double *x,double *y,double s)
{
	Point(Position(l),xp,yp,s,x,y);
}


VECTOR3 Orbit::PositionByEcA(double t)
{		
	if (ecc<1)	return Xmajv*cos(t) - Xcv + Xminv*sin(t);
	else return Xmajv*cosh(t) - Xcv + Xminv*sinh(t);
}


bool Orbit::IsVisible(double t)
{
	
    VECTOR3 pr = PositionByEcA(t);

	double x = xcenter + (dot(pr,zero) * scale);
	double y = ycenter - (dot(pr,nine) * scale);
	
	if (x>0 && x<width && y>0 && y<height) return true;
	return false;
}



// Vector projection to orbital plane, This is NOT perdictular projection. (dir) is projection direction
VECTOR3 Orbit::Projection(VECTOR3 pr,VECTOR3 dir)
{
	VECTOR3 lv = crossp(dir,norv);  // direction does not matter
	double  a  = angle(pr,lv);
	double  l  = length(pr)*fabs(sin(a));
	double  ri = angle(dir,norv);  // relative inclination
	double  le = l*tan(ri);

	return (set_length(dir,-le)+pr);
}

VECTOR3 Orbit::Projection(VECTOR3 pr)
{
	return pr-(norv*dotp(pr,norv));
}


bool Orbit::Limits(double beg,double trl,double *a,double *b,double *c)
{
	double start,p;
	int i;
    bool w;

	double xx;
    
	// Select starting point
	if (IsVisible(beg)) start=beg; // Closest to center
	else if (IsVisible(trl)) start=trl;
	else if (IsVisible(0)) start=0;
	else if (IsVisible(PI)) start=PI;
	else {
		VECTOR3 nor=crossp(zero,nine);
		xx=Longitude(Projection(_V(0,0,0)-displacement,nor));     
        start=tra2eca(limit(xx-lpe),ecc);
		if (!IsVisible(start)) return false;
	}

	*c=start;
    

	// Interpolate boundries

	double step=PI05;
	p = start - step;
    for (i=0;i<8;i++) {
		p=limit(p);
		step/=2;
		w=IsVisible(p);
		if (w) p-=step;
		else p+=step;
	}
	*b=p-step;

	step=PI05;
	p = start + step;
    for (i=0;i<8;i++) {
		p=limit(p);
		step/=2;
		w=IsVisible(p);
		if (w) p+=step;
		else p-=step;
	}
	*a=p+step;
	return true;
}







// Private
void Orbit::XDrawEllipse(oapi::Sketchpad *skp,double w,double h,double siz,double s,double e)
{
    double t=0,x,y,nx,ny,beg,etra,a,b,c;
	int i;
    double qq;

	VECTOR3 v;

	xcenter=ceil(w/2);
	ycenter=ceil(h/2);

	if (length(displacement)>0) {
		Point(displacement,xcenter,ycenter,siz,&xcenter,&ycenter);
		xcenter=ceil(xcenter);
		ycenter=ceil(ycenter);
		beg=Longitude(displacement);
		beg=limit(beg+PI);
	}
	else {
		beg=trl;
	}

	width=w;
	height=h;
	scale=siz;

	beg=tra2eca(limit(beg-lpe),ecc);
	etra=tra2eca(tra,ecc);
	
	if (DrawAll(w,siz))  {
		a=beg+PI, b=beg-PI, c=beg;
	}
	else if (!Limits(beg,etra,&a,&b,&c)) return;
		
	t=c;
	qq=difference(c,a)/20;

	Point(PositionByEcA(t),xcenter,ycenter,scale,&x,&y);
    t+=qq;

	for(i=0;i<20;i++) {	

		if (t>PI2) t-=PI2;		
		v=PositionByEcA(t);
		
		nx = xcenter + (dot(v,zero) * siz);
		ny = ycenter - (dot(v,nine) * siz);
		t+=qq;
				
		DrawLine(skp,x,y,nx,ny,w,h,false);		
		x=nx, y=ny;	
	}

	t=c;
	qq=difference(c,b)/20;

	Point(PositionByEcA(t),xcenter,ycenter,scale,&x,&y);
    t-=qq;

	for(i=0;i<20;i++) {	

		if (t<0) t+=PI2;		
		v=PositionByEcA(t);
		
		nx = xcenter + (dot(v,zero) * siz);
		ny = ycenter - (dot(v,nine) * siz);
		t-=qq;
				
		DrawLine(skp,x,y,nx,ny,w,h,false);		
		x=nx, y=ny;	
	}
}

void Orbit::XDrawHyperbola(oapi::Sketchpad *skp,double xp,double yp,double w,double h,double siz)
{

	bool ex;
	int c;
    double t,x,y,nx,ny;
	double q=MaxTra()/60.0;

    Point(xp,yp,lpe,&x,&y,siz);
	
    t=lpe+q;
    for (c=0;c<60;c++) {
	    Point(xp,yp,t,&nx,&ny,siz);
		t+=q;
		if (c==59) ex=true;
		else ex=false;
		DrawLine(skp,x,y,nx,ny,w,h,ex);
		x=nx,y=ny;
	}

	Point(xp,yp,lpe,&x,&y,siz);
	
    t=lpe-q;
    for (c=0;c<60;c++) {
		Point(xp,yp,t,&nx,&ny,siz);
		t-=q;
		if (c==59) ex=true;
		else ex=false;
		DrawLine(skp,x,y,nx,ny,w,h,ex);
		x=nx,y=ny;
	}	
}


