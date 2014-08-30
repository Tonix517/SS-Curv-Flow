#include "vector.h"

#include <math.h>
#include <assert.h>

void vecCopy(float *dest, float *src)
{
	for(int i = 0; i < 3; i ++)
		dest[i] = src[i];
}

float dist3d(float *p1, float *p2)
{
	assert(p1 && p2);

	return sqrtf( pow(p1[0] - p2[0], 2) + pow(p1[1] - p2[1], 2) + pow(p1[2] - p2[2], 2));
}

void vecSub(float *p1, float *p2, float *ret)
{
	ret[0] = p2[0] - p1[0];
	ret[1] = p2[1] - p1[1];
	ret[2] = p2[2] - p1[2];
}

void vecAdd(float *p1, float *p2, float *ret)
{
	ret[0] = p2[0] + p1[0];
	ret[1] = p2[1] + p1[1];
	ret[2] = p2[2] + p1[2];
}

void vecCross(float *v1, float *v2, float *ret)
{
	ret[0] = v1[1] * v2[2] - v1[2] * v2[1];
	ret[1] = v1[2] * v2[0] - v1[0] * v2[2];
	ret[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

float vecLen(float *p)
{
	return sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
}

void vecScale(float *vOrigVec, float fScale, float *vScaledVec)
{
	for(int i = 0; i < 3; i ++)
		vScaledVec[i] = fScale * vOrigVec[i];
}
void point2point(float *vStartPoint, float *vVec, float *vEndPoint)
{
	for(int i = 0; i < 3; i ++)
		vEndPoint[i] = vStartPoint[i] + vVec[i];
}
float dot_product(float *vec1, float *vec2)
{
	return	vec1[0]*vec2[0] + 
			vec1[1]*vec2[1] + 
			vec1[2]*vec2[2];
}

void reflectVec(float *vOrigViewVec, float *vNormal, float *vReflectViewVec)
{	
	float vReverseViewVec[3] = {0};	vecScale(vOrigViewVec, -1, vReverseViewVec);
	float vDiagonalNormalVec[3] = {0};
	float fLen = dot_product(vReverseViewVec, vNormal) / vecLen(vNormal) * 2.0f;

	float vNormalizedNormal[3] = {0};
	point2point(vNormalizedNormal, vNormal, vNormalizedNormal);
	normalize(vNormalizedNormal);
	vecScale(vNormalizedNormal, fLen, vDiagonalNormalVec);
	point2point(vDiagonalNormalVec, vOrigViewVec, vReflectViewVec);	
}

void normalize(float *p1)
{
	float len = vecLen(p1);
	if(p1 != 0)
	{
		p1[0] /= len;
		p1[1] /= len;
		p1[2] /= len;
	}
}
