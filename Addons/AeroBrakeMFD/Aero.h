#ifndef __AEROCAL_H
#define __AEROCAL_H


#include "OrbiterAPI.h"
#include "Orbitersdk.h"
#include "ABReference.h"


#define ATT_REALTIMEBANK 0
#define ATT_REALTIMEUP 1
#define ATT_PROGRADE 2
#define ATT_RETROGRADE 3

#define MAX_INTEGRATION_STEP 1024


class AeroCal {

public:

	AeroCal(VESSEL* ship);
	~AeroCal();

	void	Timestep();
	void	Start(double time);
	void    Stop(bool isTouchDown);


	VECTOR3		GetFinalD(); 
	VECTOR3		GetFinalV(); 

	VECTOR3		GetReP(); 
	VECTOR3		GetPeD(); // Periapis Distance
	VECTOR3		GetPeV(); // Periapis Velocity
	double		GetPeT(); // Time To Periapis 
	double		GetStartMJD();

	double		GetStepping();
	double		GetDeAcc(); 
	double		GetdVLose();

	VECTOR3		GetTrajectoryPoint(int index); // Get Plot position
  double    GetTrajectoryAccel(int index);
  double    GetTrajectoryVel(int index);
  double    GetTrajectoryAlt(int index);
  double    GetTrajectoryQFlux(int index);
  double    GetTrajectoryMach(int index);
  double    GetTrajectoryTime(int index);
	int			  GetLastPoint();

	bool		GetBodyName(int i,char *s,int max);

	void		SetAtmosphere(OBJHANDLE);
	void		SetShipMass(double);
	void		SetShipRho(double);
	void		SetShipHandle(OBJHANDLE);
	
  void    SetMach(double);
	void		SetSCl(double);
	void		SetSCd(double);
  void    SetAoA(double);
  double	GetSCl() {return SCl;};
  double	GetSCd() {return SCd;};
  double  GetAoA() {return AoA;}
  void    SetAttitude(int);
  void    SetBank(double);

  void    SetHDV(double v);
  void    SetTDA(double a);

  OBJHANDLE   GetReference();

	bool		IsProgress();
	//bool		IsPeriapisPassed();

	//void		SetAccuracy(double a);

	void		SetLoops(int outer,int inner);

	double  GetTime();
  double  TotTime;

  bool    isTouchDown(){return isTchDn;}
private:
  void		Segment();
	// DATA
	VECTOR3    TrajPathMap[MAX_INTEGRATION_STEP];
  double     TrajPathAccel[MAX_INTEGRATION_STEP];
  double     TrajPathAlt[MAX_INTEGRATION_STEP];
  double     TrajPathQFlux[MAX_INTEGRATION_STEP];
  double     TrajPathVel[MAX_INTEGRATION_STEP];
  double     TrajPathTime[MAX_INTEGRATION_STEP];
  double     TrajPathMach[MAX_INTEGRATION_STEP];

	bool		Cal_progress;
	int			OuterLoop;
	int			InnerLoop;
  int     Attitude;
 
	OBJHANDLE	RefHandle;
	OBJHANDLE   refhandle;

	int         MAX_SAMPLES;

	VECTOR3     RotAxis;
	double      RotPeriod;

	double		  AltLimit,Accuracy,Rho,SCl,SCd,AoA,Radius,TouchDownAlt,Mass,BankAngle,MachNumber;
	double      maxdeacc,MaxDeAcc,dvlose,dVLose;
  double      segmentdecel,segmentqflux,segmentmach;
	double      stop;
	double      step;
	double      StartMJD;
	
  double      hdv;
  
	double		Traj_myy[4];
	VECTOR3		Traj_position[4];
	VECTOR3		Traj_velocity[4];
	OBJHANDLE	Traj_handle[4];

	//FinalData

	VECTOR3		Traj_PeD,Traj_PeV,FinalV,FinalD,Traj_ReP;
	double		Traj_PeT;
	int			  Traj_PeIndex,Traj_Pos;
  bool      isTchDn;
   
	// RuntimeData

	VECTOR3   Old_Vel,traj_rep;
  VECTOR3		traj_ped,traj_pev; // Periapis distance and velocity at runtime
	int			  traj_peindex, traj_pos;
	double		traj_time; // Trajectory flight time at runtime	
	double		traj_pet; // Periapis time at runtime
	double		PeD; // Periapis distance at runtime
  VESSEL*   Ship;

  double    ComputeStep(double alt,double decel);
  int fso; // per debug
 
};


#endif