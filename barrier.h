#ifndef BARRIER_H
#define BARRIER_H

#include <vector>

class Particle;

class Barrier
{
public:

	//	param: p -> input: particle to judge
	//	param: pHitPos -> output: the hit point
	//	param: pHitNorm -> output: the hit normal
	//	param: pDeltaLen : the rebounded length, only for performance consideration
	//
	virtual bool isHit(Particle *p, float *pHitPos, float *pHitNorm, float *pDeltaLen) = 0;

	virtual void render() = 0;
};

//	
class Ball : public Barrier
{
public:
	Ball(float fRad, float *pCtr);
	
	bool isHit(Particle *p, float *pHitPos, float *pHitNorm, float *pDeltaLen);

	void render();

private:
	float _fRad;
	float _ctr[3];
};

///	
///		TODO
///
//
//class Square : public Barrier
//{
//public:
//	Square(float *pCtr, float *pNorm, float *pVec);
//
//	bool isHit(Particle *p, float *pHitPos, float *pHitNorm);
//
//	void render();
//
//public:
//	float _ctr[3];
//	float _norm[3];
//	float _vec1[3];	// pointing out from ctr to one side
//	float _vec2[3];
//
//};

extern std::vector<Barrier*> barVec;

#endif