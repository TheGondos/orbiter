// Copyright (c) Martin Schweiger
// Licensed under the MIT License

//-----------------------------------------------------------------------------
// File: D3DMath.cpp
//
// Desc: Shortcut macros and functions for using DX objects
//
//
// Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved
//-----------------------------------------------------------------------------

#define __D3DMATH_CPP
#define D3D_OVERLOADS
#define STRICT
#include "D3dmath.h"
#include <math.h>
#include <stdio.h>

void VMAT_rotx (glm::fmat4 &a, double r)
{
	double sinr = sin(r), cosr = cos(r);
	memset(&a, 0, sizeof (glm::fmat4));
	a[1][1] = a[2][2] = (float)cosr;
	a[1][2] = -(a[2][1] = (float)sinr);
	a[0][0] = a[3][3] = 1.0f;
}

void VMAT_roty (glm::fmat4 &a, double r)
{
	double sinr = sin(r), cosr = cos(r);
	memset (&a, 0, sizeof (glm::fmat4));
	a[0][0] = a[2][2] = (float)cosr;
	a[2][0] = -(a[0][2] = (float)sinr);
	a[1][1] = a[3][3] = 1.0f;
}

// Create a rotation matrix from a rotation axis and angle
void VMAT_rotation_from_axis (const glm::fvec3 &axis, float angle, glm::fmat4 &R)
{
	// Calculate quaternion
	angle *= 0.5f;
	float w = cosf(angle), sina = sinf(angle);
	float x = sina * axis.x;
	float y = sina * axis.y;
	float z = sina * axis.z;

	// Rotation matrix
	float xx = x*x, yy = y*y, zz = z*z;
	float xy = x*y, xz = x*z, yz = y*z;
	float wx = w*x, wy = w*y, wz = w*z;

	R[0][0] = 1 - 2 * (yy+zz);
	R[0][1] =     2 * (xy+wz);
	R[0][2] =     2 * (xz-wy);
	R[1][0] =     2 * (xy-wz);
	R[1][1] = 1 - 2 * (xx+zz);
	R[1][2] =     2 * (yz+wx);
	R[2][0] =     2 * (xz+wy);
	R[2][1] =     2 * (yz-wx);
	R[2][2] = 1 - 2 * (xx+yy);

	R[0][3] = R[1][3] = R[2][3] = R[3][0] = R[3][1] = R[3][2] = 0.0f;
	R[3][2] = 1.0f;
}

//-----------------------------------------------------------------------------
// Name: D3DMath_MatrixMultiply()
// Desc: Does the matrix operation: [Q] = [A] * [B].
//-----------------------------------------------------------------------------
void D3DMath_MatrixMultiply( glm::fmat4& q, glm::fmat4& a, glm::fmat4& b )
{
    float* pA = (float*)&a;
    float* pB = (float*)&b;
    float  pM[16];

    memset( pM, 0, sizeof(glm::fmat4) );

    for( int i=0; i<4; i++ ) 
        for( int j=0; j<4; j++ ) 
            for( int k=0; k<4; k++ ) 
                pM[4*i+j] += pA[4*k+j] * pB[4*i+k];

    memcpy( &q, pM, sizeof(glm::fmat4) );
}




//-----------------------------------------------------------------------------
// Name: D3DMath_MatrixInvert()
// Desc: Does the matrix operation: [Q] = inv[A]. Note: this function only
//       works for matrices with [0 0 0 1] for the 4th column.
//-----------------------------------------------------------------------------
bool D3DMath_MatrixInvert( glm::fmat4& q, glm::fmat4& a )
{

    if( fabs(a[3][3] - 1.0f) > .001f)
        return false;
    if( fabs(a[0][3]) > .001f || fabs(a[1][3]) > .001f || fabs(a[2][3]) > .001f )
        return false;
    q = glm::inverse(a);
/*
    float fDetInv = 1.0f / ( a._11 * ( a._22 * a._33 - a._23 * a._32 ) -
                             a._12 * ( a._21 * a._33 - a._23 * a._31 ) +
                             a._13 * ( a._21 * a._32 - a._22 * a._31 ) );

    q._11 =  fDetInv * ( a._22 * a._33 - a._23 * a._32 );
    q._12 = -fDetInv * ( a._12 * a._33 - a._13 * a._32 );
    q._13 =  fDetInv * ( a._12 * a._23 - a._13 * a._22 );
    q._14 = 0.0f;

    q._21 = -fDetInv * ( a._21 * a._33 - a._23 * a._31 );
    q._22 =  fDetInv * ( a._11 * a._33 - a._13 * a._31 );
    q._23 = -fDetInv * ( a._11 * a._23 - a._13 * a._21 );
    q._24 = 0.0f;

    q._31 =  fDetInv * ( a._21 * a._32 - a._22 * a._31 );
    q._32 = -fDetInv * ( a._11 * a._32 - a._12 * a._31 );
    q._33 =  fDetInv * ( a._11 * a._22 - a._12 * a._21 );
    q._34 = 0.0f;

    q._41 = -( a._41 * q._11 + a._42 * q._21 + a._43 * q._31 );
    q._42 = -( a._41 * q._12 + a._42 * q._22 + a._43 * q._32 );
    q._43 = -( a._41 * q._13 + a._42 * q._23 + a._43 * q._33 );
    q._44 = 1.0f;
*/
    return true;
}




//-----------------------------------------------------------------------------
// Name: D3DMath_VectorMatrixMultiply()
// Desc: Multiplies a vector by a matrix
//-----------------------------------------------------------------------------
bool D3DMath_VectorMatrixMultiply( glm::fvec3& vDest, const glm::fvec3& vSrc,
                                      const glm::fmat4& mat)
{
    float x = vSrc.x*mat[0][0] + vSrc.y*mat[1][0] + vSrc.z* mat[2][0] + mat[3][0];
    float y = vSrc.x*mat[0][1] + vSrc.y*mat[1][1] + vSrc.z* mat[2][1] + mat[3][1];
    float z = vSrc.x*mat[0][2] + vSrc.y*mat[1][2] + vSrc.z* mat[2][2] + mat[3][2];
    float w = vSrc.x*mat[0][3] + vSrc.y*mat[1][3] + vSrc.z* mat[2][3] + mat[3][3];
    
    if( fabs( w ) < g_EPSILON )
        return false;

    vDest.x = x/w;
    vDest.y = y/w;
    vDest.z = z/w;

    return true;
}




//-----------------------------------------------------------------------------
// Name: D3DMath_VectorTMatrixMultiply()
// Desc: Multiplies a vector by the transpose of a matrix
//-----------------------------------------------------------------------------
bool D3DMath_VectorTMatrixMultiply( glm::fvec3& vDest, const glm::fvec3& vSrc,
                                      const glm::fmat4& mat)
{
    float x = vSrc.x*mat[0][0] + vSrc.y*mat[0][1] + vSrc.z* mat[0][2] + mat[0][3];
    float y = vSrc.x*mat[1][0] + vSrc.y*mat[1][1] + vSrc.z* mat[1][2] + mat[1][3];
    float z = vSrc.x*mat[2][0] + vSrc.y*mat[2][1] + vSrc.z* mat[2][2] + mat[2][3];
    float w = vSrc.x*mat[3][0] + vSrc.y*mat[3][1] + vSrc.z* mat[3][2] + mat[3][3];
    
    if( fabs( w ) < g_EPSILON )
        return false;

    vDest.x = x/w;
    vDest.y = y/w;
    vDest.z = z/w;

    return true;
}

//-----------------------------------------------------------------------------
// Name: D3DMath_QuaternionFromRotation()
// Desc: Converts a normalized axis and angle to a unit quaternion.
//-----------------------------------------------------------------------------
void D3DMath_QuaternionFromRotation( float& x, float& y, float& z, float& w,
                                     glm::fvec3& v, float fTheta )
{
    x = (float)sin(fTheta/2.0f) * v.x;
    y = (float)sin(fTheta/2.0f) * v.y;
    z = (float)sin(fTheta/2.0f) * v.z;
    w = (float)cos(fTheta/2.0f);
}

//-----------------------------------------------------------------------------
// Name: D3DMath_RotationFromQuaternion()
// Desc: Converts a normalized axis and angle to a unit quaternion.
//-----------------------------------------------------------------------------
void D3DMath_RotationFromQuaternion( glm::fvec3& v, float& fTheta,
                                     float x, float y, float z, float w )
                                      
{
    fTheta = (float)( acos(w) * 2.0f );
    v.x    = (float)( x / sin(fTheta/2.0f) );
    v.y    = (float)( y / sin(fTheta/2.0f) );
    v.z    = (float)( z / sin(fTheta/2.0f) );
}




//-----------------------------------------------------------------------------
// Name: D3DMath_QuaternionFromAngles()
// Desc: Converts euler angles to a unit quaternion.
//-----------------------------------------------------------------------------
void D3DMath_QuaternionFromAngles( float& x, float& y, float& z, float& w,
                                   float fYaw, float fPitch, float fRoll )
                                        
{
    float fSinYaw   = (float)sin(fYaw/2);
    float fSinPitch = (float)sin(fPitch/2);
    float fSinRoll  = (float)sin(fRoll/2);
    float fCosYaw   = (float)cos(fYaw/2);
    float fCosPitch = (float)cos(fPitch/2);
    float fCosRoll  = (float)cos(fRoll/2);

    x = fSinRoll * fCosPitch * fCosYaw - fCosRoll * fSinPitch * fSinYaw;
    y = fCosRoll * fSinPitch * fCosYaw + fSinRoll * fCosPitch * fSinYaw;
    z = fCosRoll * fCosPitch * fSinYaw - fSinRoll * fSinPitch * fCosYaw;
    w = fCosRoll * fCosPitch * fCosYaw + fSinRoll * fSinPitch * fSinYaw;
}




//-----------------------------------------------------------------------------
// Name: D3DMath_MatrixFromQuaternion()
// Desc: Converts a unit quaternion into a rotation matrix.
//-----------------------------------------------------------------------------
void D3DMath_MatrixFromQuaternion( glm::fmat4& mat, float x, float y, float z,
                                   float w )
{
    float xx = x*x; float yy = y*y; float zz = z*z;
    float xy = x*y; float xz = x*z; float yz = y*z;
    float wx = w*x; float wy = w*y; float wz = w*z;
    
    mat[0][0] = 1 - 2 * ( yy + zz ); 
    mat[0][1] =     2 * ( xy - wz );
    mat[0][2] =     2 * ( xz + wy );

    mat[1][0] =     2 * ( xy + wz );
    mat[1][1] = 1 - 2 * ( xx + zz );
    mat[1][2] =     2 * ( yz - wx );

    mat[2][0] =     2 * ( xz - wy );
    mat[2][1] =     2 * ( yz + wx );
    mat[2][2] = 1 - 2 * ( xx + yy );

    mat[0][3] = mat[1][3] = mat[2][3] = 0.0f;
    mat[3][0] = mat[3][1] = mat[3][2] = 0.0f;
    mat[3][2] = 1.0f;
}




//-----------------------------------------------------------------------------
// Name: D3DMath_QuaternionFromMatrix()
// Desc: Converts a rotation matrix into a unit quaternion.
//-----------------------------------------------------------------------------
void D3DMath_QuaternionFromMatrix( float& x, float& y, float& z, float& w,
                                   glm::fmat4& mat )
{
    if( mat[0][0] + mat[1][1] + mat[2][2] > 0.0f )
    {
        float s = (float)sqrt( mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3] );

        x = (mat[1][2]-mat[2][1]) / (2*s);
        y = (mat[2][0]-mat[0][2]) / (2*s);
        z = (mat[0][1]-mat[1][0]) / (2*s);
        w = 0.5f * s;
    }
    else
    {


    }
    float xx = x*x; float yy = y*y; float zz = z*z;
    float xy = x*y; float xz = x*z; float yz = y*z;
    float wx = w*x; float wy = w*y; float wz = w*z;
    
    mat[0][0] = 1 - 2 * ( yy + zz ); 
    mat[0][1] =     2 * ( xy - wz );
    mat[0][2] =     2 * ( xz + wy );

    mat[1][0] =     2 * ( xy + wz );
    mat[1][1] = 1 - 2 * ( xx + zz );
    mat[1][2] =     2 * ( yz - wx );

    mat[2][0] =     2 * ( xz - wy );
    mat[2][1] =     2 * ( yz + wx );
    mat[2][2] = 1 - 2 * ( xx + yy );

    mat[0][3] = mat[1][3] = mat[2][3] = 0.0f;
    mat[3][0] = mat[3][1] = mat[3][2] = 0.0f;
    mat[3][3] = 1.0f;
}




//-----------------------------------------------------------------------------
// Name: D3DMath_QuaternionMultiply()
// Desc: Mulitples two quaternions together as in {Q} = {A} * {B}.
//-----------------------------------------------------------------------------
void D3DMath_QuaternionMultiply( float& Qx, float& Qy, float& Qz, float& Qw,
                                  float Ax, float Ay, float Az, float Aw,
                                  float Bx, float By, float Bz, float Bw )
{
    float Dx = Bw*Ax + Bx*Aw + By*Az + Bz*Ay;
    float Dy = Bw*Ay + By*Aw + Bz*Ax + Bx*Az;
    float Dz = Bw*Az + Bz*Aw + Bx*Ay + By*Ax;
    float Dw = Bw*Aw + Bx*Ax + By*Ay + Bz*Az;

    Qx = Dx; Qy = Dy; Qz = Dz; Qw = Dw;
}




//-----------------------------------------------------------------------------
// Name: D3DMath_SlerpQuaternions()
// Desc: Compute a quaternion which is the spherical linear interpolation
//       between two other quaternions by dvFraction.
//-----------------------------------------------------------------------------
void D3DMath_QuaternionSlerp( float& Qx, float& Qy, float& Qz, float& Qw,
                              float Ax, float Ay, float Az, float Aw,
                              float Bx, float By, float Bz, float Bw,
                              float fAlpha )
{
    float fScale1;
    float fScale2;

    // Compute dot product, aka cos(theta):
    float fCosTheta = Ax*Bx + Ay*By + Az*Bz + Aw*Bw;

    if( fCosTheta < 0.0f )
    {
        // Flip start quaternion
        Ax = -Ax; Ay = -Ay; Ax = -Az; Aw = -Aw;
        fCosTheta = -fCosTheta;
    }

    if( fCosTheta + 1.0f > 0.05f )
    {
        // If the quaternions are close, use linear interploation
        if( 1.0f - fCosTheta < 0.05f )
        {
            fScale1 = 1.0f - fAlpha;
            fScale2 = fAlpha;
        }
        else // Otherwise, do spherical interpolation
        {
            float fTheta    = (float)acos( fCosTheta );
            float fSinTheta = (float)sin( fTheta );
            
            fScale1 = (float)sin( fTheta * (1.0f-fAlpha) ) / fSinTheta;
            fScale2 = (float)sin( fTheta * fAlpha ) / fSinTheta;
        }
    }
    else
    {
        Bx = -Ay;
        By =  Ax;
        Bz = -Aw;
        Bw =  Az;
        fScale1 = (float)sin( g_PI * (0.5f - fAlpha) );
        fScale2 = (float)sin( g_PI * fAlpha );
    }

    Qx = fScale1 * Ax + fScale2 * Bx;
    Qy = fScale1 * Ay + fScale2 * By;
    Qz = fScale1 * Az + fScale2 * Bz;
    Qw = fScale1 * Aw + fScale2 * Bw;
}
