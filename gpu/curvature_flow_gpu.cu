#ifndef CURVATURE_FLOW_GPU_CU
#define CURVATURE_FLOW_GPU_CU

#include "vector_gpu.cu"
#include "particle_gpu.cu"

__device__ __constant__ int width  = 0;
__device__ __constant__ int height = 0;
__device__ __constant__ float fCFThreshold  = 0;
__device__ __constant__ float fCVFactor = 0;
__device__ __constant__ float fThickKernRad = 30;
__device__ __constant__ float ParRad = 30;

__device__	float *depth_buf0_gpu = NULL;
__device__  float *avg_normals_gpu;
__device__  float *pixle_normals_gpu;
__device__	Particle_gpu *pars_gpu;
__device__  float *thick_buf_gpu;

void sendConstant2GPU(int iWidth, int iHeight, float ffCFThreshold, float ffCVFactor, float ffThickKernRad, float fParRad)
{
	cudaMemcpyToSymbol(width, &iWidth, sizeof(int), cudaMemcpyHostToDevice);
	cudaMemcpyToSymbol(height, &iHeight, sizeof(int), cudaMemcpyHostToDevice);
	cudaMemcpyToSymbol(fCFThreshold, &ffCFThreshold, sizeof(float), cudaMemcpyHostToDevice);
	cudaMemcpyToSymbol(fCVFactor, &ffCVFactor, sizeof(float), cudaMemcpyHostToDevice);
	cudaMemcpyToSymbol(fThickKernRad, &ffThickKernRad, sizeof(float), cudaMemcpyHostToDevice);
	cudaMemcpyToSymbol(ParRad, &fParRad, sizeof(float), cudaMemcpyHostToDevice);
}

__global__
void gpu_setup(float *pDepBuf0, float *pAvgNorm, float *pPixNorm, Particle_gpu *pParsBuf, float *pThickBuf)
{
	depth_buf0_gpu = pDepBuf0;
	avg_normals_gpu = pAvgNorm;
	pixle_normals_gpu = pPixNorm;
	pars_gpu = pParsBuf;
	thick_buf_gpu = pThickBuf;
}

////

__device__
float dz2x(int x, int y)
{
	if( x < 0 || y < 0 || x >= width || y >= height)
	{
		return 0;
	}

	float v0 = (x <= 0) ? 0 :         *(depth_buf0_gpu + (x - 1 + y * width));
	float v1 =                        *(depth_buf0_gpu + (x + y * width));
	float v2 = (x >= width - 1) ? 0 : *(depth_buf0_gpu + (x + 1 + y * width));

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

	return ret;
}

__device__
float dz2y(int x, int y)
{
	if( x < 0 || y < 0 || x >= width || y >= height)
	{
		return 0;
	}

	float v0 = (y <= 0) ? 0 :         *(depth_buf0_gpu + (x + (y - 1) * width));
	float v1 =                        *(depth_buf0_gpu + (x + y * width));
	float v2 = (y >= height - 1) ? 0 : *(depth_buf0_gpu + (x + (y + 1) * width));

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

	return ret;
}

__global__
void curvature_flow_step_gpu()
{
	int tid = 0; //	TODO
	int i, j;	//TODO

	float fd = *(depth_buf0_gpu + tid);
	if(fd >= 0)
	{
		float dz_x = dz2x(i, j);
		float dz_x0 = dz2x(i-1, j);
		float dz_x2 = dz2x(i+1, j);
		float dz2x2 = (dz_x2 - dz_x0) / 2.f;

		float dz_y = dz2y(i, j);
		float dz_y0 = dz2y(i, j-1);
		float dz_y2 = dz2y(i, j+1);
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

		float fCF = (Cy * Ex + Cx * Ey) * inv_D32 / 2;

		//	Apply
		//
		fCF = fCF > fCFThreshold ? fCFThreshold : fCF;
		*(depth_buf0_gpu + tid) -= fCF * fCVFactor;

		if(*(depth_buf0_gpu + tid) < 0) *(depth_buf0_gpu + tid) = 0;
		if(*(depth_buf0_gpu + tid) > 1) *(depth_buf0_gpu + tid) = 1;
	}
}

////
__device__
void getAvgNormal(float *retNorm, int x, int y, int nSampleRad)
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
			float fd = *(depth_buf0_gpu + (currX + currY * width));
			if(fd > 0)
			{
				float currNorm[3];

				//	The sampled normal should get an average value ...
				float dz_x = dz2x(currX, currY);
				float dz_y = dz2y(currX, currY);

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

__global__
void calcAvgNormals_gpu(unsigned nSampleStep, unsigned nAvgNormRad)
{
	int tid = 0;	//	TODO
	if(tid >= width * height)
	{
		return;
	}

	int i, j;		//	TODO

	float fd = *(depth_buf0_gpu + tid);
	if(fd > 0 && (i % nSampleStep == 0) && (j % nSampleStep == 0))
	{
		getAvgNormal(avg_normals_gpu + 3 * tid, i, j, nAvgNormRad);
	}
}

__global__
void calcAllNormals_gpu(unsigned nSampleStep)
{
	int tid = 0; // TODO
	if(tid >= width * height)
	{
		return;
	}

	int i, j; // TODO
	if( (i % nSampleStep == 0) && (j % nSampleStep == 0))
	{
		*(pixle_normals_gpu + 3 * tid + 0) = *(avg_normals_gpu + 3 * tid + 0);
		*(pixle_normals_gpu + 3 * tid + 1) = *(avg_normals_gpu + 3 * tid + 1);
		*(pixle_normals_gpu + 3 * tid + 2) = *(avg_normals_gpu + 3 * tid + 2);
	}

	float fd = *(depth_buf0_gpu + tid);
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
		currNormals[0] = pixle_normals_gpu + 3 * (nGridX0 + nGridY0 * width);

		if((i + 1) < width && (nGridX0 + nSampleStep) < width ) 
		{
			currNormals[1] = pixle_normals_gpu + 3 * (nGridX0 + nSampleStep + nGridY0 * width);
		}
		else
		{
			currNormals[1] = NullNorm;
		}

		if((j + 1)< height && (nGridY0 + nSampleStep) < height)
		{
			currNormals[2] = pixle_normals_gpu + 3 * (nGridX0 + (nGridY0 + nSampleStep) * width);
		}
		else
		{
			currNormals[2] = NullNorm;
		}

		if(	(i + 1) < width && (j + 1)< height && 
			(nGridY0 + nSampleStep) < height && 
			(nGridX0 + nSampleStep) < width )
		{
			currNormals[3] = pixle_normals_gpu + 3 * (nGridX0 + nSampleStep + (nGridY0 + nSampleStep) * width);
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

		float *pCurrNorm = pixle_normals_gpu + 3 * (tid);
		vecScale(n01, (1 - fDistPercY), tmp);
		vecScale(n23, fDistPercY, tmp1);
		vecAdd(tmp, tmp1, pCurrNorm);
	}
}

//////
__device__
void transform_point(float out[4], const float m[16], const float in[4])   
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

__device__ float modelview[16] = {0};					
__device__ float projection[16] = {0}; 
__device__ int viewport[4] = {0};

__device__
bool myProject(float objx, float objy, float objz, 
			   const float  modelMatrix[16], const float projMatrix[16], const int viewport[4], 
			   float *winx, float *winy, float *winz)   
{   
    // matrice transformation   
    float in[4], out[4];   
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

__global__
void projectPars_gpu(int nCount)
{
	int tid = 0; //	TODO
	if(tid < nCount)
	{
		float vPrjPos[3] = {0};
		Particle_gpu *par = pars_gpu + tid;
		if(!myProject( par->_pos[0], par->_pos[1], par->_pos[2],
						modelview, projection, viewport, 
						vPrjPos + 0, vPrjPos + 1, vPrjPos + 2) )
		{
			return;
		}

		par->x2d = vPrjPos[0];
		par->y2d = vPrjPos[1];
	}
}

#define max(x,y) ( ((x) > (y)) ? (x) : (y))
__device__
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

__global__
void calcAllDensity()
{
	int tid = 0;	//	TODO
	if(tid < width * height)
	{
		int i, j;	//	TODO
		float fDepth = *(depth_buf0_gpu + tid);
		if(fDepth == 0)
		{
			*(thick_buf_gpu + (tid)) = 0;
		}
		else
		{
			float fThick = 0;
			
			for(int n = 0; n < /*pars2d.size()*/10; n ++)	//	TODO
			{
				fThick += thickKern(i, j, (pars_gpu + n)->_pos, fDepth) * 0.04;
			}

			*(thick_buf_gpu + tid) = fThick;
		}
	}
}

#endif