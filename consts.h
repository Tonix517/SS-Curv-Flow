#ifndef CONSTS_H
#define CONSTS_H

#include "IL/ilut.h"

//	Windows Params
extern unsigned WinWidth;
extern unsigned WinHeight;
extern unsigned WinLeft;
extern unsigned WinTop;

extern char * WinTitle;

//	Zoom params
extern unsigned ZoomStep;
extern unsigned MaxDist;

//	Camera params
extern float CamPos[3];
extern float CamTarget[3];
extern float CamUp[3];
extern unsigned InitDist;

//	Rotation params
extern bool bRotation;
extern float RotPerSec;

//	Particle Param
extern unsigned ParCount;
extern float ParRad;
extern float InitPos[3];
extern float InitVel[3];

//	Scene Param
extern float SceneHeight;
extern float SceneDim;

extern int bGPUEnabled;

//	Physical Param
extern float fVelFactor;		// velocity to position k
extern float g;
extern float fGravFactor;	// to tune the g
extern float fVelDissByWall;

//	Rigid Param
extern float fSpringCoe;
extern float fDampingCoe;

//	Grid Param
extern float SceneHeight;
extern float SceneDim;
//	NOTE: The Kernel dim is the same as GridDim.
extern unsigned GridCountPerDim;
extern float GridDim;

//	Fluid param
extern float fStdFluidDensity;
extern float fFluidDensityFactor;
extern float fMassPerPar;

extern float KernelRad;

extern float kPress;
extern float kVisco;


//	DevIL
extern ILuint nCurrImg;
extern unsigned nCurrImgCount;

#endif