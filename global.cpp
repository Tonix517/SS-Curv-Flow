#include "global.h"

#include "consts.h"
#include "particle.h"

struct Cell * _grid;
std::vector<Particle> particles;
std::vector<Particle> particles2d;

void global_init()
{
	_grid = new Cell[GridCountPerDim * GridCountPerDim * GridCountPerDim];
}

void global_destroy()
{
	 delete [] _grid;
}

///
///		Grid Acceleration 
///
void resetGrid()
{
	//	Clear first
	for(int x = 0; x < GridCountPerDim; x ++)
	for(int y = 0; y < GridCountPerDim; y ++)
	for(int z = 0; z < GridCountPerDim; z ++)
	{
		(_grid + x + y * GridCountPerDim + z * GridCountPerDim * GridCountPerDim)->_parIds.clear();
	}

	//	Fill
	for(int i = 0; i < particles.size(); i ++)
	{	
		int x = particles[i].x;
		int y = particles[i].y;
		int z = particles[i].z;

		if( x >= 0 && x < GridCountPerDim &&
			y >= 0 && y < GridCountPerDim &&
			z >= 0 && z < GridCountPerDim )
		{
			(_grid + x + y * GridCountPerDim + z * GridCountPerDim * GridCountPerDim)->_parIds.push_back(particles[i].nId);
		}
	}
}