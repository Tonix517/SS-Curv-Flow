#ifndef PARTICLE_H
#define PARTICLE_H

#include <vector>
#include <map>
#include <time.h>

class Particle
{
	friend class Ball;

public:

	Particle(int id, float *vPos, float *vVel, float *vColor);

	void render();
	void update();

	//	STATIC
	static void genParticlesByRandom(unsigned, unsigned long nCurrFrame);	// called once globally	
	static unsigned clearParticles();	// called once per frame

private:
		
	void updateVelByScene();
	void updateVelByBarriers();

	void getRelativeParticles(std::vector<int> &);

	void updateVelByPressueAndViscosity(std::vector<int> &rel);
	void updateVelByRigid(std::vector<int> &rel);
	void updateDensity(std::vector<int> &rel);

public:
	
	int nId;	

	int x, y, z;

//private:

	bool _bCollided[3];

	float	_currRho;

	float	_pos[3];
	float	_vel[3];
	float	_color[3];

	// for alignment with GPU
	float x2d;
	float y2d;

};
#endif