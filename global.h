#ifndef GLOBAL_H
#define GLOBAL_H

#include <vector>
#include "particle.h"

//
//	Grid Acceleration
//
struct Cell
{
	std::vector<int> _parIds;
};

extern struct Cell * _grid;
void resetGrid();

extern std::vector<Particle> particles;
extern std::vector<Particle> particles2d;

void global_init();
void global_destroy();

#endif