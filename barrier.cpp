#include <assert.h>
#include <math.h>

#include "barrier.h"
#include "math_util.h"
#include "vector.h"
#include "consts.h"
#include "particle.h"

#include "GL/glut.h"

std::vector<Barrier*> barVec;

///
///		Ball
///

Ball::Ball(float fRad, float *pCtr)
	: _fRad(fRad)
{
	_ctr[0] = pCtr[0];
	_ctr[1] = pCtr[1];
	_ctr[2] = pCtr[2];
}

bool Ball::isHit(Particle *p, float *pHitPos, float *pHitNorm, float *pDeltaLen)
{
	assert(p && pHitPos && pHitNorm);

	float vNextPos[3] = {0};
	
	vNextPos[0] = p->_pos[0] + p->_vel[0] * fVelFactor;
	vNextPos[1] = p->_pos[1] + p->_vel[1] * fVelFactor;
	vNextPos[2] = p->_pos[2] + p->_vel[2] * fVelFactor;

	float tmp[3] = {0};
	vecSub(_ctr, p->_pos, tmp);
	float r0 = vecLen(tmp);
	vecSub(_ctr, vNextPos, tmp);
	float r1 = vecLen(tmp);

	//printf("%.2f -> %.2f\n", r0, r1);

	if( r0 > _fRad && r1 <= _fRad) //	let's assume the ball is large enough
	{
		//	get hit point
		if(r1 == _fRad)
		{
			*pDeltaLen  = 0;

			pHitNorm[0] = vNextPos[0] - _ctr[0];
			pHitNorm[1] = vNextPos[1] - _ctr[1];
			pHitNorm[2] = vNextPos[2] - _ctr[2];
			normalize(pHitNorm);

			pHitPos[0] = vNextPos[0];
			pHitPos[1] = vNextPos[1];
			pHitPos[2] = vNextPos[2];

			return true;
		}

		//
		float k0 = p->_pos[0] - _ctr[0];
		float k1 = p->_pos[1] - _ctr[1];
		float k2 = p->_pos[2] - _ctr[2];
		float a = pow(vecLen(p->_vel), 2);	assert(a != 0);
		float b = 2 *(k0 * p->_vel[0] + k1 * p->_vel[1]  + k2 * p->_vel[2] );
		float c = k0 * k0 + k1 * k1 + k2 * k2 - _fRad * _fRad;
		float t = (-b-sqrt(b*b - 4 * a *c))/ (2*a);
		
		pHitPos[0] = p->_pos[0] + t * p->_vel[0];
		pHitPos[1] = p->_pos[1] + t * p->_vel[1];
		pHitPos[2] = p->_pos[2] + t * p->_vel[2];

		pHitNorm[0] = pHitPos[0] - _ctr[0];
		pHitNorm[1] = pHitPos[1] - _ctr[1];
		pHitNorm[2] = pHitPos[2] - _ctr[2];
		normalize(pHitNorm);

		//
		float vDeltaLen[3] = {0};
		vecSub(pHitPos, vNextPos, vDeltaLen);
		*pDeltaLen  = vecLen(vDeltaLen);

		return true;
	}

	return false;
}

void Ball::render()
{
	glPushMatrix();

		glColor3f(0, 0, 0.5);
		glTranslatef(_ctr[0], _ctr[1], _ctr[2]);
		glutSolidSphere(_fRad, 20, 20);

	glPopMatrix();
}
