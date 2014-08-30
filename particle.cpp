#include "particle.h"

#include <cmath>
#include <cassert>
#include <memory>
#include <cstdlib>
using namespace std;

#include "consts.h"
#include "GL/glut.h"

#include "global.h"
#include "barrier.h"
#include "vector.h"
#include "math_util.h"


Particle::Particle(int id, float *vPos, float *vVel, float *vColor)
	:nId(id)
{
	assert(vPos && vColor);

	memcpy(_pos, vPos, sizeof(float) * 3);
	memcpy(_color, vColor, sizeof(float) * 3);
	memcpy(_vel, vVel, sizeof(float) * 3);

}

void Particle::render()
{
	glPushMatrix();

#if 0
		float vCurrColor[3] = {_currRho / 10.f, 0, 0};
#else
		float *vCurrColor = _color;
#endif
		glColor3fv(vCurrColor);
		
		float fShininess = 100;
		glMaterialfv(GL_FRONT, GL_AMBIENT, vCurrColor);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, vCurrColor);
		glMaterialfv(GL_FRONT, GL_SPECULAR, vCurrColor);
		glMaterialfv(GL_FRONT, GL_SHININESS, &fShininess);

		glTranslatef(_pos[0], _pos[1], _pos[2]);
		glutSolidSphere(ParRad, 12, 12);

	glPopMatrix();
}

void Particle::genParticlesByRandom(unsigned nCount, unsigned long nCurrFrame)
{
	particles.clear();

//	0
	//float color[3] = {1, 1, 1};
	//float veloc[3] = {0, 0, 0};
	//float posit[3] = {0, SceneHeight, 0};
	//particles.push_back(Particle(0, posit, veloc, color));
	//posit[0] += 2 * ParRad;
	//particles.push_back(Particle(1, posit, veloc, color));
	//posit[2] += 2 * ParRad;
	//particles.push_back(Particle(2, posit, veloc, color));
	//posit[0] -= 2 * ParRad;
	//particles.push_back(Particle(3, posit, veloc, color));
	//posit[2] -= 2 * ParRad;
	//posit[0] += ParRad; posit[2] += ParRad + 0.2;
	//posit[1] += 5;
	//particles.push_back(Particle(4, posit, veloc, color));
//	1
	//float color[3] = {1, 1, 1};
	//float veloc[3] = {0, 0, 0};
	//float posit[3] = {0, SceneHeight, 0};
	//particles.push_back(Particle(0, posit, veloc, color));
	//posit[1] = 5;
	//particles.push_back(Particle(1, posit, veloc, color));

//	2
	//nCount = SceneDim / (2 * KernelRad);
	//
	//for(int i = 0; i < nCount / 2; i ++)
	//for(int j = 0; j < nCount; j ++)
	//{
	//	float color[3] = {1, 1, 1};
	//	float pos[3] = { -(SceneDim/2) + i * 2 * KernelRad, SceneHeight, -(SceneDim/2) + j * 2 * KernelRad };
	//	particles.push_back(Particle(i * nCount /2 + j, pos, InitVel, color));
	//}

	//for(int i = 0; i < 5; i ++)
	//for(int j = 0; j < 5; j ++)
	//{
	//	float color[3] = {1, 1, 1};
	//	float pos[3] = { -5 + i *  2 * KernelRad + KernelRad, SceneHeight + 20, -10 + j * 2 * KernelRad + KernelRad};
	//	particles.push_back(Particle(nCount * nCount / 2 + i * 5 + j, pos, InitVel, color));
	//}

//	3
	for(unsigned i = 0; i < nCount; i++)
	{
		//	randomize color
		float color[3] = {1, 1, 1};
		color[0] = rand() % 255 / 255.f;
		color[1] = rand() % 255 / 255.f;
		color[2] = rand() % 255 / 255.f;

		//	randomize pos
		int dim = 10;
		int xi = i % dim;
		int zi = i / (dim * dim);
		int yi = (i - zi * (dim * dim) - xi) / dim;

		float fRnd = 0.03;
		float pos[3] = { InitPos[0] + xi * ParRad * 1.4 + rand() % 10 * fRnd ,
						 InitPos[1] + yi * ParRad * 1.4 + rand() % 10 * fRnd,
						 InitPos[2] + zi * ParRad * 1.4 + rand() % 10 * fRnd};		

		//float f = 0.3;
		//InitPos[0] += (rand() % 10 - 4) * f;
		//InitPos[1] += (rand() % 10 - 4) * f;
		//InitPos[2] += (rand() % 10 - 4) * f;

		//	randomize velocity
		//InitVel[0] += (rand() % 10 - 4) * 0.1 ;
		//InitVel[1] += (rand() % 10 - 4) * 0.02 ;
		//InitVel[2] += (rand() % 10 - 4) * 0.1 ;
		//float ctr[3] = {0, 10, 0};
		//float vec[3]; vecSub(ctr, pos, vec);

		//if(vecLen(vec) < 6)
		//{
			particles.push_back(Particle(i, pos, InitVel, color));
		//}
	}
}


///
///		TODO: this method could be CUDA-lized
///
void Particle::update()
{

	//	1. Collision Handling
	//
	updateVelByScene();	
	updateVelByBarriers();

	//	2. Update
	//	

	//	1) gravity
	float old = _vel[1];
	_vel[1] += g * fGravFactor;

	//	2) N-S & Rigid
	std::vector<int> vRel;
	getRelativeParticles(vRel);

	updateVelByRigid(vRel);

	updateDensity(vRel);	
	updateVelByPressueAndViscosity(vRel);

	//	3) Position Update
	_pos[0] += _vel[0] * fVelFactor; 
	_pos[1] += _vel[1] * fVelFactor;
	_pos[2] += _vel[2] * fVelFactor; 

	//	3. Update to new Grid Id by current Pos
	//
	x = (_pos[0] + SceneDim/2) / GridDim;
	y = (_pos[1] - SceneHeight) / GridDim;
	z = (_pos[2] + SceneDim/2) / GridDim;

	////	color..
	//if(x >=0 && y >= 0 && z >= 0 &&
	//   x < GridCountPerDim && y < GridCountPerDim && z < GridCountPerDim)
	//{
	//	_color[0] = x * 1.f/GridCountPerDim;
	//	_color[1] = y * 1.f/GridCountPerDim;
	//	_color[2] = z * 1.f/GridCountPerDim;
	//}
}

unsigned Particle::clearParticles()
{
	unsigned nCount = 0;

	vector<Particle>::iterator iter = particles.begin();
	for(; iter != particles.end(); iter ++)
	{
		if(iter->_pos[1] < -5)
		{
			particles.erase(iter--);
			nCount ++;
		}
	}

	return nCount;
}

void Particle::updateVelByScene()
{
	float fHalfDim = SceneDim / 2;

	for(int i = 0; i < 3; i ++)
	{
		if(_vel[i] != 0 )
		{
			float fNextPos = _pos[i] + _vel[i] * fVelFactor;

			switch(i)
			{
			case 0:	//	x					
			case 2: //	z
				if( fNextPos < -fHalfDim && _vel[i] < 0)
				{
					float fDelta = -fHalfDim - fNextPos;
					_pos[i] = -fHalfDim + fDelta;
					_vel[i] = -_vel[i] * fVelDissByWall;
				}
				else if( fNextPos > fHalfDim && _vel[i] > 0)
				{
					float fDelta = fNextPos - fHalfDim;
					_pos[i] = fHalfDim - fDelta;
					_vel[i] = -_vel[i] * fVelDissByWall;
				}
				break;

			case 1:
				//printf("[%.3f, %.3f]\n", fNextPos, SceneHeight);
				if( fNextPos < SceneHeight && _vel[1] < 0)
				{
					float fDelta = SceneHeight - fNextPos;
					_pos[1] = SceneHeight + fDelta;
					_vel[1] = -_vel[1] * fVelDissByWall;
				}
				break;
			}//	switch
		}//	if
	}//	for
}

//	CUDA-lizable
//
void Particle::updateDensity(std::vector<int> &rel)
{
	_currRho = 0;
	
	for(int i = 0; i < rel.size(); i ++)
	{
		Particle *currPar = &particles[rel[i]];

		float r = dist3d(currPar->_pos, _pos);
		if(r <= KernelRad)
		{
			float c =  KernelRad * KernelRad - r * r;
			_currRho += fMassPerPar * mPoly6Kern * pow(c, 3);				
		}
	}		
	
}

void Particle::updateVelByPressueAndViscosity(std::vector<int> &tmpSet)
{
	float vPress[3] = {0};

	for(int i = 0; i < tmpSet.size(); i ++)
	{
		if(i != nId)
		{
			float vCurrPress[3] = {0};
			float vCurrVisco[3] = {0};

			float r = dist3d(particles[i]._pos, particles[nId]._pos);

			if(r <= KernelRad && r > 0)
			{
				//	avoid over repulsion force
				const float threshold = ParRad * 1.5;
				r = r < threshold ? threshold : r;

				float pi = (particles[i]._currRho - fStdFluidDensity)* fFluidDensityFactor;
				float pj = (particles[nId]._currRho - fStdFluidDensity)* fFluidDensityFactor;
				float c = (KernelRad - r);
				
				float dterm = (c * particles[i]._currRho * particles[nId]._currRho);
				float pterm = -0.5f * c * mSpikyKern * ( pi + pj) / r;

				//	Pressue
				if(kPress > 0)
				{
					float vCurrPress[3] = {0};
					vecSub(particles[nId]._pos, particles[i]._pos, vCurrPress);
					vecScale(vCurrPress, pterm * dterm * kPress, vCurrPress);
					vecAdd(vPress, vCurrPress, vPress);
				}

				//	Viscosity
				if(kVisco > 0)
				{
					float vCurrVisco[3] = {0};
					vecSub(particles[i]._vel, particles[nId]._vel, vCurrVisco);
					vecScale(vCurrVisco, mLapKern * 0.2 * dterm * kVisco, vCurrVisco);
					vecAdd(vPress, vCurrVisco, vPress);
				}
			}
		}
	}// for

	for(int i = 0; i< 3; i++)
	{
		_vel[i] += vPress[i] / fMassPerPar;
	}
}

void Particle::updateVelByRigid(vector<int> &tmpSet)
{

	float vRigidF[3] = {0};
	
	for(int i = 0; i < tmpSet.size(); i ++)
	{
		if(tmpSet[i] != nId)
		{
			float r = dist3d(particles[nId]._pos, particles[tmpSet[i]]._pos);
			if(r < 2 * ParRad && r > 0)
			{
				//	avoid over repulsion force
				const float threshold = ParRad;
				r = r < threshold ? threshold : r;

				//	Distance Vector
				float dVec[3] = {0};
				vecSub(particles[nId]._pos, particles[tmpSet[i]]._pos, dVec);
				float r = vecLen(dVec);

				//	relative vel
				float dVel[3] = {0};
				vecSub(particles[nId]._vel, particles[tmpSet[i]]._vel, dVel);
				
				if(fSpringCoe > 0)
				{
					float dSpringF[3] = {0};
					vecScale(dVec,  -fSpringCoe * (ParRad * 2 - r) / r, dSpringF);
					vecAdd(vRigidF, dSpringF, vRigidF);
				}

				if(fDampingCoe > 0)
				{
					float dDampingF[3] = {0};
					vecScale(dVel, fDampingCoe, dDampingF);
					vecAdd(vRigidF, dDampingF, vRigidF);
				}
			}
		}
	}//	for

	float tmp[3] = {_vel[0], _vel[1], _vel[2]};
	for(int i = 0; i< 3; i++)
	{
		_vel[i] += vRigidF[i]/fMassPerPar;
	}

}

void Particle::updateVelByBarriers()
{
	//	check Barrier
	for(int i = 0; i < barVec.size(); i ++)
	{
		float vHitPoint[3] = {0};
		float vHitNormal[3] = {0};
		float fDeltaLen = 0;

		if(barVec[i]->isHit(this, vHitPoint, vHitNormal, &fDeltaLen))
		{
			float vNewVec[3] = {0};

			float vOrigVec[3] = {0};
			vecSub(vHitPoint, _pos, vOrigVec);
			reflectVec(vOrigVec, vHitNormal, vNewVec);
			normalize(vNewVec);

			//	new velocity
			float vNewVel[3] = {vNewVec[0], vNewVec[1], vNewVec[2]};
			vecScale(vNewVel, -vecLen(_vel) * fVelDissByWall, _vel);

			//	new position
			if(fDeltaLen > 0)
			{
				_pos[0] = vHitPoint[0] + _vel[0] * fVelFactor;
				_pos[1] = vHitPoint[1] + _vel[1] * fVelFactor;
				_pos[2] = vHitPoint[2] + _vel[2] * fVelFactor;
			}

			break;	// one barrier per check
		}
	}
}

void Particle::getRelativeParticles(std::vector<int> &vec)
{
#define GRID_OFFSET 0

	if( x >= 0 && x < GridCountPerDim && 
	    y >= 0 && y < GridCountPerDim && 
	    z >= 0 && z < GridCountPerDim )
	{
#if GRID_OFFSET
		float currGridCtrX = -SceneDim / 2.f + x * GridDim;
		float currGridCtrZ = -SceneDim / 2.f + z * GridDim;
		float currGridCtrY =  SceneHeight + y * GridDim;
#endif
		for(int i = -1; i <= 1; i ++)
		{
#if GRID_OFFSET
			if( (_pos[0] < currGridCtrX && i == 1) ||
				(_pos[0] > currGridCtrX && i == -1) )
			{
				continue;
			}
#endif
			for(int j = -1; j <= 1; j ++)
			{
#if GRID_OFFSET
				if( (_pos[1] < currGridCtrY && j == 1) ||
					(_pos[1] > currGridCtrY && j == -1) )
				{
					continue;
				}
#endif
				for(int k = -1; k <= 1; k ++)
				{
#if GRID_OFFSET
					if( (_pos[2] < currGridCtrZ && k == 1) ||
						(_pos[2] > currGridCtrZ && k == -1) )
					{
						continue;
					}
#endif
					int currX = x + i;
					int currY = y + j;
					int currZ = z + k;

					if( currX >= 0 && currX < GridCountPerDim && 
						currY >= 0 && currY < GridCountPerDim && 
						currZ >= 0 && currZ < GridCountPerDim )
					{
						Cell *currCell = (_grid + currX + currY * GridCountPerDim + currZ * GridCountPerDim * GridCountPerDim);
						for(int n = 0; n < currCell->_parIds.size(); n ++)
						{
							vec.push_back(currCell->_parIds[n]);
						}
					}
				}//	k
			}//	j
		}//	i
	}
}