#pragma once
#include <array>

const int CHUNK = 32;
extern int heights[CHUNK][CHUNK];
void generateTerrain(unsigned seed);
bool isAirAt(int x, int z, int y);
int getHeightAt(int x, int z);
