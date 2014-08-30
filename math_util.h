#ifndef MATH_H
#define MATH_H

#include "particle.h"

#define PI 3.1415926

extern float mPoly6Kern;
extern float mSpikyKern;			// Laplacian of viscocity (denominator): PI h^6
extern float mLapKern;
//

extern float fCFThreshold;
extern float fCFFactor;
extern unsigned nIterCount;
extern unsigned nNormalsLerpStep;
extern int nAvgNormRad;

//
extern float *depth_buffer;
extern float *depth_buffer1;
extern float *thick_buffer;
extern float *normals;

extern int width;
extern int height;
//

extern double modelview[16];					
extern double projection[16]; 
extern int viewport[4];

void math_init();
void math_destroy();

//	ss-normal
void calcAllNormals(float *normals, float *depthBuf, int width, int height, unsigned nSampleStep = 4);

//	thickness
void projectAllParticles(std::vector<Particle> &pars3d, std::vector<Particle> &pars2d);
void calcAllThickness(float *thickBuf, float *depthBuf, int width, int height, std::vector<Particle> &pars2d);

void genDepthPic(float *depBuf0, float *depBuf1, float *depthPic, int width, int height);
void applyDepthToTex(float *depth, unsigned char *texData, int width, int height);

//	curvature flow
extern float *cf_buf;
void curvature_flow(float *depthBuf, float fCVFactor, int width, int height, unsigned nIterCount);

#endif