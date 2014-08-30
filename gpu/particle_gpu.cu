#ifndef PARTICLE_GPU_CU
#define PARTICLE_GPU_CU

struct Particle_gpu
{
	int nId;	

	int x, y, z;

	bool _bCollided[3];

	float	_currRho;

	float	_pos[3];
	float	_vel[3];
	float	_color[3];

	float x2d;
	float y2d;
	
};

#endif