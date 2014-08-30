#include "math_util.h"
#include "consts.h"
#include "particle.h"
#include "vector.h"

#include <math.h>
#include <vector>
#include <algorithm>
#include <assert.h>

float mPoly6Kern;
float mSpikyKern;
float mLapKern;
///

float fCFThreshold = 1.f;
float fCFFactor = 0.01;
unsigned nIterCount = 20;
unsigned nNormalsLerpStep = 4;
int nAvgNormRad = 4;

///

float *depth_buffer;
float *depth_buffer1;
float *thick_buffer;
float *normals;

int width;
int height;

//

double modelview[16] = {0};					
double projection[16] = {0}; 
int viewport[4] = {0};

void math_init()
{
	depth_buffer = (float*)malloc(sizeof(float) * WinWidth * WinHeight);
	depth_buffer1 = (float*)malloc(sizeof(float) * WinWidth * WinHeight);
	thick_buffer  = (float*)malloc(sizeof(float) * WinWidth * WinHeight);
	cf_buf  = (float*)malloc(sizeof(float) * WinWidth * WinHeight);
	normals = (float *)malloc(sizeof(float) * 3 * WinWidth * WinHeight);

	width = WinWidth;
	height = WinHeight;

	//	N-S Equation
	mPoly6Kern = 315.0f / (64.0f * 3.141592 * pow( KernelRad, 9) );
	mSpikyKern = -45.0f / (PI * pow( KernelRad, 6) );			// Laplacian of viscocity (denominator): PI h^6
	mLapKern = 45.0f / (PI * pow( KernelRad, 6) );
}

void math_destroy()
{
	if(depth_buffer != NULL)
	{
      free(depth_buffer);
	}

	if(depth_buffer1 != NULL)
	{
      free(depth_buffer1);
	}

	if(thick_buffer)
	{
		free(thick_buffer);
		thick_buffer = NULL;
	}

	if(normals)
	{
		free(normals);
		normals = NULL;
	}

	if(cf_buf)
	{
		free(cf_buf);
		cf_buf = NULL;
	}

}

////
////
////
#define max(x,y) ( ((x) > (y)) ? (x) : (y))
float fThickKernRad = 30;
float thickKern(int x, int y, float *pos, float fDepth)
{
	float fProjectedSize = ParRad/* * fDepth*/;	// TODO: I know...

	float p1 = fabs(x - pos[0]);
	float p2 = fabs(y - pos[1]);

	if(p1 > fThickKernRad || p2 > fThickKernRad)
	{
		return 0;
	}

	//	Linear here
	return max(0.f, 1.f - fabs(p1/fProjectedSize)/fThickKernRad) * max(0.f, 1.f - fabs(p2/fProjectedSize)/fThickKernRad);
}

///
///	http://blog.csdn.net/kaizitop/archive/2009/05/07/4158521.aspx
///
static void transform_point(double out[4], const double m[16], const double in[4])   
{   
#define M(row,col) m[col*4+row]   
    out[0] =    
        M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0, 3) * in[3];   
    out[1] =   
        M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1, 3) * in[3];   
    out[2] =   
        M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2, 3) * in[3];   
    out[3] =   
        M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3, 3) * in[3];   
#undef M   
}   

bool myProject(GLdouble objx, GLdouble objy, GLdouble objz, 
			   const GLdouble  modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], 
			   GLdouble *winx, GLdouble *winy, GLdouble *winz)   
{   
    // matrice transformation   
    double in[4], out[4];   
    //initialize matrice and column vector as a transformer   
    in[0] = objx;   
    in[1] = objy;   
    in[2] = objz;   
    in[3] = 1.0;   
    transform_point(out, modelMatrix, in);  //乘以模型视图矩阵   
    transform_point(in, projMatrix, out);   //乘以投影矩阵   
    //齐次向量的第四项不能为0   
    if(in[3] == 0.0)   
        return false;   
    //向量齐次化标准化   
    in[0] /= in[3];   
    in[1] /= in[3];   
    in[2] /= in[3];   
    //视口向量的作用   
    *winx = viewport[0] + (1 + in[0]) * viewport[2] / 2;   
    *winy = viewport[1] + (1 + in[1]) * viewport[3] / 2;   
    *winz = (1 + in[2]) / 2;   
    return true;   
}  

///
///		CUDA-lizable
///
void projectAllParticles(std::vector<Particle> &pars3d, std::vector<Particle> &pars2d)
{
	pars2d.clear();
	for(int i = 0; i < pars3d.size(); i++)
	{
		double vPrjPos[3] = {0};

		if(!myProject( pars3d[i]._pos[0], pars3d[i]._pos[1], pars3d[i]._pos[2],
						modelview, projection, viewport, 
						vPrjPos + 0, vPrjPos + 1, vPrjPos + 2) )
		{
			printf("gluProject fail.. %d\n", i);
		}

		float vFvPrjPos[3] = {vPrjPos[0], vPrjPos[1], vPrjPos[2]};
		float vel[3] = {0};
		pars2d.push_back(Particle(pars3d[i].nId, vFvPrjPos, vel, pars3d[i]._color));
	}
}

///		CUDA-lizable
void calcAllThickness(float *thickBuf, float *depthBuf, int width, int height, std::vector<Particle> &pars2d)
{
	for(int j = 0; j < height; j ++)
	for(int i = 0; i < width; i ++)
	{
		float fDepth = *(depthBuf + (i + j * width));
		if(fDepth == 0)
		{
			*(thickBuf + (i + j * width)) = 0;
		}
		else
		{
			float fThick = 0;
			
			for(int n = 0; n < pars2d.size(); n ++)
			{
				fThick += thickKern(i, j, pars2d[n]._pos, fDepth) * 0.04;
			}

			*(thickBuf + (i + j * width)) = fThick;
		}
	}
}

//	dz/dx
float dz2x(int x, int y, int width, int height, float *depthBuf)
{
	if( x < 0 || y < 0 || x >= width || y >= height)
	{
		return 0;
	}

	float v0 = (x <= 0) ? 0 :         *(depthBuf + (x - 1 + y * width));
	float v1 =                        *(depthBuf + (x + y * width));
	float v2 = (x >= width - 1) ? 0 : *(depthBuf + (x + 1 + y * width));

	float ret = 0;

	if( (v0 == 0 && v2 != 0) )
	{
		ret = (v2 - v1);
	}
	else if( (v2 == 0 && v0 != 0) )
	{
		ret = (v1 - v0);
	}
	else
	{
		ret = (  v2 - v0) / 2.f;	
	}

	//if( ret >  fDepthThreshold ) ret = fDepthThreshold;
	//if( ret < -fDepthThreshold ) ret = -fDepthThreshold;
	return ret;
}

//	dz/dy
float dz2y(int x, int y, int width, int height, float *depthBuf)
{
	if( x < 0 || y < 0 || x >= width || y >= height)
	{
		return 0;
	}

	float v0 = (y <= 0) ? 0 :         *(depthBuf + (x + (y - 1) * width));
	float v1 =                        *(depthBuf + (x + y * width));
	float v2 = (y >= height - 1) ? 0 : *(depthBuf + (x + (y + 1) * width));

	float ret = 0;

	if( (v0 == 0 && v2 != 0) )
	{
		ret = (v2 - v1);
	}
	else if( (v2 == 0 && v0 != 0) )
	{
		ret = (v1 - v0);
	}
	else
	{
		ret = ( v2 - v0) / 2.f;	//	TODO
	}

	//if( ret >  fDepthThreshold ) ret = fDepthThreshold;
	//if( ret < -fDepthThreshold ) ret = -fDepthThreshold;
	return ret;
}

void getAvgNormal(float *retNorm, float *depthBuf, int x, int y, int width, int height, int nSampleRad)
{
	float tmp[3] = {0};
	unsigned nAvailableCount = 0;

	for(int j = -nSampleRad; j <= nSampleRad; j ++)
	for(int i = -nSampleRad; i <= nSampleRad; i ++)
	{
		int currX = x + i;
		int currY = y + j;
		if( currX >=0 && currX < width &&
			currY >=0 && currY < height )
		{
			float fd = *(depthBuf + (currX + currY * width));
			if(fd > 0)
			{
				float currNorm[3];

				//	The sampled normal should get an average value ...
				float dz_x = dz2x(currX, currY, width, height, depthBuf);
				float dz_y = dz2y(currX, currY, width, height, depthBuf);

				float Cx = i == 0 ? 0 : 2.f / (width * currX);	
				float Cy = j == 0 ? 0 : 2.f / (height * currY);	

				float D = Cy * Cy * dz_x * dz_x + Cx * Cx * dz_y * dz_y + Cx * Cx * Cy * Cy * (1 - fd) * (1 - fd);
				if(D == 0)
				{
					continue;
				}
				float rv_sqrtD = 1.f / sqrt(D);

				currNorm[0] = - Cy * dz_x * rv_sqrtD;
				currNorm[1] = - Cx * dz_y * rv_sqrtD;
				currNorm[2] = Cx * Cy * (1 - fd) * rv_sqrtD;

				vecAdd(tmp, currNorm, tmp);
				nAvailableCount ++;
			}
		}
	}

	if(nAvailableCount > 0)
	{
		vecScale(tmp, 1.f/nAvailableCount, tmp);
		normalize(tmp);
		vecCopy(retNorm, tmp);
	}
}

void calcAllNormals(float *normals, float *depthBuf, int width, int height, unsigned nSampleStep)
{

	//	Grid Points
	for(int j = 0; j < height; j += nSampleStep)
	for(int i = 0; i <  width; i += nSampleStep)
	{
		float fd = *(depthBuf + (i + j * width));
		if(fd > 0)
		{
			getAvgNormal(normals + 3 * (i + j * width), depthBuf, i, j, width, height, nAvgNormRad);
		}
	}

	if(nSampleStep == 1) 
	{
		return;
	}

	//	Interpolation other pixels
	for(int j = 0; j < height; j += 1)
	for(int i = 0; i <  width; i += 1)
	{
		if( (i % nSampleStep == 0) && (j % nSampleStep == 0))
		{
			continue;
		}

		float fd = *(depthBuf + (i + j * width));
		if(fd > 0)
		{
			//	1. find grid
			int nGridX0 = (i / nSampleStep) * nSampleStep;
			int nGridY0 = (j / nSampleStep) * nSampleStep;

			float fDistPercX = (i - nGridX0) * 1.f / nSampleStep;
			float fDistPercY = (j - nGridY0) * 1.f / nSampleStep;
			
			//	2. Bi-linear Interpolation
			float NullNorm[3] = {0};
			float *currNormals[4];
			currNormals[0] = normals + 3 * (nGridX0 + nGridY0 * width);

			if((i + 1) < width && (nGridX0 + nSampleStep) < width ) 
			{
				currNormals[1] = normals + 3 * (nGridX0 + nSampleStep + nGridY0 * width);
			}
			else
			{
				currNormals[1] = NullNorm;
			}

			if((j + 1)< height && (nGridY0 + nSampleStep) < height)
			{
				currNormals[2] = normals + 3 * (nGridX0 + (nGridY0 + nSampleStep) * width);
			}
			else
			{
				currNormals[2] = NullNorm;
			}

			if(	(i + 1) < width && (j + 1)< height && 
				(nGridY0 + nSampleStep) < height && 
				(nGridX0 + nSampleStep) < width )
			{
				currNormals[3] = normals + 3 * (nGridX0 + nSampleStep + (nGridY0 + nSampleStep) * width);
			}
			else
			{
				currNormals[3] = NullNorm;
			}

			float tmp[3], tmp1[3];

			float n01[3] = {0}; 
			vecScale(currNormals[0], (1 - fDistPercX), tmp);
			vecScale(currNormals[1], fDistPercX, tmp1);
			vecAdd(tmp, tmp1, n01);

			float n23[3] = {0};
			vecScale(currNormals[2], (1 - fDistPercX), tmp);
			vecScale(currNormals[3], fDistPercX, tmp1);
			vecAdd(tmp, tmp1, n23);

			float *pCurrNorm = normals + 3 * (i + j * width);
			vecScale(n01, (1 - fDistPercY), tmp);
			vecScale(n23, fDistPercY, tmp1);
			vecAdd(tmp, tmp1, pCurrNorm);
		}
	}//	for
}

///	CUDA-lizable
void genDepthPic(float *depth_buffer0, float *depth_buffer1, float *depth_pic, int width, int height)
{
	std::vector<float> vSet;
	for(int j = 0; j < height; j ++)
	for(int i = 0; i < width; i ++)
	{
		float refV = *(depth_buffer1 + (i + j * width)) = 1.f - *(depth_buffer1 + (i + j * width));
		float value = *(depth_buffer0 + (i + j * width)) = 1.f - *(depth_buffer0 + (i + j * width));
		if(value != refV)
		{
			value = 0;
			*(depth_buffer0 + (i + j * width)) = 0;
		}

		if(value > 0)
		{
			vSet.push_back(value);
		}
	}

	float min = *std::min_element(vSet.begin(), vSet.end());
	float max = *std::max_element(vSet.begin(), vSet.end());

	for(int j = 0; j < height; j ++)
	{
		 for(int i = 0; i < width; i ++)
		 {
			 float value = *(depth_buffer0 + (i + j * width));
			 float cValue = value == 0.f ? 0 : (value - min) / (max - min);
			 *(depth_pic + (i + j * width)) = cValue;
		 }
	}
}
///	CUDA-lizable
void applyDepthToTex(float *depth, unsigned char *texData, int width, int height)
{
	for(unsigned i = 0; i < height; i ++)
	for(unsigned j = 0; j < width ; j ++)
	{
		unsigned offset = i * WinWidth + j;
		float v = *(depth + offset);
		if(v >= 0)
		{
			*((texData + offset * 4) + 0) = v * 255;
			*((texData + offset * 4) + 1) = 0;
			*((texData + offset * 4) + 2) = 0;
			*((texData + offset * 4) + 3) = 0;
		}
		else if(v < 0)
		{
			*((texData + offset * 4) + 2) = -v * 255;
			*((texData + offset * 4) + 1) = 0;
			*((texData + offset * 4) + 0) = 0;
			*((texData + offset * 4) + 3) = 0;
		}
	}
}

float *cf_buf = NULL;

///
void curvature_flow_step(float *depthBuf, int width, int height)
{
	unsigned nSampleCount = 2;

	for(unsigned j = 0; j < height; j ++)
	for(unsigned i = 0; i < width ; i ++)
	{
		unsigned offset = i + j * width;
		float fd = *(depthBuf + offset);
		if(fd >= 0)
		{
			float dz_x = dz2x(i, j, width, height, depthBuf);
			float dz_x0 = dz2x(i-1, j, width, height, depthBuf);
			float dz_x2 = dz2x(i+1, j, width, height, depthBuf);
			float dz2x2 = (dz_x2 - dz_x0) / 2.f;

			float dz_y = dz2y(i, j, width, height, depthBuf);
			float dz_y0 = dz2y(i, j-1, width, height, depthBuf);
			float dz_y2 = dz2y(i, j+1, width, height, depthBuf);
			float dz2y2 = (dz_y2 - dz_y0) / 2.f;

			float Cx = i == 0 ? 0 : 2.f / (width * i);	//	TODO ?
			float Cy = j == 0 ? 0 : 2.f / (height * j);	//	TODO ?
			float D = Cy * Cy * dz_x * dz_x + Cx * Cx * dz_y * dz_y + Cx * Cx * Cy * Cy * (1 - fd) * (1 - fd);
			float inv_D32 = 1.f / powf(D, 1.5);

			float ky = 4.f / height / height;
			float kx = 4.f / width / width;
			float dD_x = ky * pow(j, -2.f) * 2 * dz_x * dz2x2 + 
						 kx * dz_y * dz_y * -2 * pow(i, -3.f) + 
						 ky * pow(j, -2.f) * kx * (-2 * pow(i, -3.f) * fd * fd + pow(i, -2.f) * 2 * fd * dz_x);

			float dD_y = kx * pow(i, -2.f) * 2 * dz_y * dz2y2 + 
						 ky * dz_x * dz_x * -2 * pow(j, -3.f) + 
						 kx * pow(i, -2.f) * ky * (-2 * pow(j, -3.f) * fd * fd + pow(j, -2.f) * 2 * fd * dz_y);

			float Ex = 0.5 * dz_x * dD_x - dz2x2 * D;
			float Ey = 0.5 * dz_y * dD_y - dz2y2 * D;

			*(cf_buf + offset) = (Cy * Ex + Cx * Ey) * inv_D32 / 2;
		}
	}
}

void applyCurvFlow(float *depthBuf, float *flowBuf, float fCVFactor, int width, int height)
{
	for(unsigned i = 0; i < height; i ++)
	for(unsigned j = 0; j < width ; j ++)
	{
		unsigned offset = i * width + j;
		float fd = *(depthBuf + offset);
		if(fd > 0)
		{
			float fFlow = *(flowBuf + offset);
			fFlow = fFlow > fCFThreshold ? fCFThreshold : fFlow;
			*(depthBuf + offset) -= fFlow * fCVFactor;

			if(*(depthBuf + offset) < 0) *(depthBuf + offset) = 0;
			if(*(depthBuf + offset) > 1) *(depthBuf + offset) = 1;
		}
	
	}
}

void curvature_flow(float *depthBuf, float fCVFactor, int width, int height, unsigned nIterCount)
{
	for(unsigned i = 0; i < nIterCount; i ++)
	{
		curvature_flow_step(depthBuf, width, height);
		applyCurvFlow(depthBuf, cf_buf, fCVFactor, width, height);
		printf("\b\b\b\b\b\b%.2f", i * 1.f/ nIterCount);
	}
}
