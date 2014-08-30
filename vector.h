#ifndef VECTOR_H
#define VECTOR_H

float dist3d(float *p1, float *p2);
void vecCopy(float *dest, float *src);
void vecSub(float *p1, float *p2, float *ret);
void vecAdd(float *p1, float *p2, float *ret);
void vecCross(float *v1, float *v2, float *ret);
void normalize(float *p1);
float vecLen(float *p);
void reflectVec(float *vOrigViewVec, float *vNormal, float *vReflectViewVec);
void vecScale(float *vOrigVec, float fScale, float *vScaledVec);
void point2point(float *vStartPoint, float *vVec, float *vEndPoint);
float dot_product(float *vec1, float *vec2);

#endif