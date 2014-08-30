#include "consts.h"
#include <math.h>

unsigned WinWidth  = 512;
unsigned WinHeight = 512;
unsigned WinLeft   = 200;
unsigned WinTop    = 100;

char * WinTitle = "Curvature Flow - Tony Zhang";

unsigned ZoomStep = 1;
unsigned MaxDist = 300;

//	Rotation
bool bRotation = false;
float RotPerSec = 0.05;

//	Camera params
float CamPos[3]    = {52, 42, 52};
float CamTarget[3] = {0, 0, 0};
float CamUp[3] = {-1.5, 4, -1.5};
unsigned InitDist = 20;

//	Particle Parameters
unsigned ParCount = 500;
float ParRad = 1;

int bGPUEnabled = 0;

//	Physical param
float InitPos[3] = {-10, 0, -10};

//	Grid Param
float SceneHeight = -5;
float SceneDim = 30;


//	Physical param
float fVelFactor = 0.02;
float g = -9.8;
float fGravFactor = 9 * fVelFactor;
float fVelDissByWall = 0.6;
float InitVel[3] = {30, 0, 10};

//	Rigid Param
float fSpringCoe = 0.6f;
float fDampingCoe = 0.06;

//	Grid Param
unsigned GridCountPerDim = 25;
float GridDim = SceneDim / GridCountPerDim;

//	fluid param
//
float fStdFluidDensity = 1.0f;
float fFluidDensityFactor = 1.f;
float fMassPerPar = 1.f;
float KernelRad = GridDim;
float kPress = 0.05f;
float kVisco = 0.2f; // buggy

//	DevIL
//
ILuint nCurrImg = 1;
unsigned nCurrImgCount = 0;