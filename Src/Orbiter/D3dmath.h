//-----------------------------------------------------------------------------
// File: D3DMath.h
//
// Desc: Math functions and shortcuts for Direct3D programming.
//
//
// Copyright (C) 1997 Microsoft Corporation. All rights reserved
//-----------------------------------------------------------------------------

#ifndef D3DMATH_H
#define D3DMATH_H

// ============================================================================
// Begin stuff added by MS
// ============================================================================
#include "Vecmat.h"

typedef struct {
	float x, y, z;
	float tu, tv;
} POSTEXVERTEX;
//const int POSTEXVERTEXFLAG = D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);

// ============================================================================
// SetD3DRotation()
// Convert a logical rotation matrix into a D3D transformation matrix, taking into
// account the change in direction (counter-clockwise in the logical interface,
// clockwise in D3D)

inline void SetD3DRotation (glm::fmat4 &a, const Matrix &r)
{
	a[0][0] = (float)r.m11;
	a[0][1] = (float)r.m12;
	a[0][2] = (float)r.m13;
	a[1][0] = (float)r.m21;
	a[1][1] = (float)r.m22;
	a[1][2] = (float)r.m23;
	a[2][0] = (float)r.m31;
	a[2][1] = (float)r.m32;
	a[2][2] = (float)r.m33;
}

// ============================================================================
// SetInvD3DRotation()
// Copy the transpose of a matrix as rotation of a D3D transformation matrix

inline void SetInvD3DRotation (glm::fmat4 &a, const Matrix &r)
{
	a[0][0] = (float)r.m11;
	a[0][1] = (float)r.m21;
	a[0][2] = (float)r.m31;
	a[1][0] = (float)r.m12;
	a[1][1] = (float)r.m22;
	a[1][2] = (float)r.m32;
	a[2][0] = (float)r.m13;
	a[2][1] = (float)r.m23;
	a[2][2] = (float)r.m33;
}

// ============================================================================
// SetD3DTranslation()
// Assemble a logical translation vector into a D3D transformation matrix

inline void SetD3DTranslation (glm::fmat4 &a, const Vector &t)
{
	a[3][0] = (float)t.x;
	a[3][1] = (float)t.y;
	a[3][2] = (float)t.z;
}

// ============================================================================
// End stuff added by MS
// ============================================================================


//-----------------------------------------------------------------------------
// Useful Math constants
//-----------------------------------------------------------------------------
const float g_PI       =  3.14159265358979323846f; // Pi
const float g_2_PI     =  6.28318530717958623200f; // 2 * Pi
const float g_PI_DIV_2 =  1.57079632679489655800f; // Pi / 2
const float g_PI_DIV_4 =  0.78539816339744827900f; // Pi / 4
const float g_INV_PI   =  0.31830988618379069122f; // 1 / Pi
const float g_DEGTORAD =  0.01745329251994329547f; // Degrees to Radians
const float g_RADTODEG = 57.29577951308232286465f; // Radians to Degrees
const float g_HUGE     =  1.0e+38f;                // Huge number for float
const float g_EPSILON  =  1.0e-5f;                 // Tolerance for floats

//-----------------------------------------------------------------------------
// Matrix functions
//-----------------------------------------------------------------------------

// Set up a as mirror transformation at xz-plane
inline void VMAT_flipy (glm::fmat4 &a)
{
	memset (&a, 0, sizeof (glm::fmat4));
	a[0][0] = a[2][2] = a[3][3] = 1.0f;
	a[1][1] = -1.0f;
}

// Set up a as matrix for ANTICLOCKWISE rotation r around x/y/z-axis
void VMAT_rotx (glm::fmat4 &a, double r);
void VMAT_roty (glm::fmat4 &a, double r);

// Create a rotation matrix from a rotation axis and angle
void VMAT_rotation_from_axis (const glm::fvec3 &axis, float angle, glm::fmat4 &R);

void    D3DMath_MatrixMultiply( glm::fmat4& q, glm::fmat4& a, glm::fmat4& b );
bool D3DMath_MatrixInvert( glm::fmat4& q, glm::fmat4& a );


//-----------------------------------------------------------------------------
// Vector functions
//-----------------------------------------------------------------------------

inline void D3DMath_SetVector (glm::fvec3 &vec, float x, float y, float z)
{ vec.x = x, vec.y = y, vec.z = z; }

inline void D3DMath_SetVector (glm::fvec3 &vec, double x, double y, double z)
{ D3DMath_SetVector (vec, (float)x, (float)y, (float)z); }

inline glm::fvec3 D3DMath_Vector (double x, double y, double z)
{ glm::fvec3 vec; D3DMath_SetVector (vec, x, y, z); return vec; }

bool D3DMath_VectorMatrixMultiply( glm::fvec3& vDest, const glm::fvec3& vSrc,
                                      const glm::fmat4& mat);
bool D3DMath_VectorTMatrixMultiply( glm::fvec3& vDest, const glm::fvec3& vSrc,
                                      const glm::fmat4& mat);


//-----------------------------------------------------------------------------
// Quaternion functions
//-----------------------------------------------------------------------------
void D3DMath_QuaternionFromRotation( float& x, float& y, float& z, float& w,
                                     glm::fvec3& v, float fTheta );
void D3DMath_RotationFromQuaternion( glm::fvec3& v, float& fTheta,
                                     float x, float y, float z, float w );
void D3DMath_QuaternionFromAngles( float& x, float& y, float& z, float& w,
                                   float fYaw, float fPitch, float fRoll );
void D3DMath_MatrixFromQuaternion( glm::fmat4& mat, float x, float y, float z,
                                   float w );
void D3DMath_QuaternionFromMatrix( float &x, float &y, float &z, float &w,
                                   glm::fmat4& mat );
void D3DMath_QuaternionMultiply( float& Qx, float& Qy, float& Qz, float& Qw,
                                 float Ax, float Ay, float Az, float Aw,
                                 float Bx, float By, float Bz, float Bw );
void D3DMath_QuaternionSlerp( float& Qx, float& Qy, float& Qz, float& Qw,
                              float Ax, float Ay, float Az, float Aw,
                              float Bx, float By, float Bz, float Bw,
                              float fAlpha );


#endif // D3DMATH_H
