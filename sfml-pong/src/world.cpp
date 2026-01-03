#include "world.h"
#include <cmath>
#include <algorithm>
#include <iostream>

int heights[CHUNK][CHUNK];

void generateTerrain(unsigned seed){
    for(int x=0;x<CHUNK;++x){
        for(int z=0;z<CHUNK;++z){
            float nx = (x - CHUNK/2) * 0.12f;
            float nz = (z - CHUNK/2) * 0.12f;
            float h = sinf(nx*1.0f + seed*0.1f) + sinf(nz*1.3f + seed*0.07f)*0.6f + sinf((nx+nz)*0.5f)*0.4f;
            int hh = std::max(1, int(3 + h * 3.0f));
            heights[x][z] = hh;
        }
    }
    std::cout << "Terrain generated (seed=" << seed << ")\n";
}

bool isAirAt(int x, int z, int y){
    if (x < 0 || x >= CHUNK || z < 0 || z >= CHUNK) return true;
    return y >= heights[x][z];
}

int getHeightAt(int x, int z){
    if (x < 0 || x >= CHUNK || z < 0 || z >= CHUNK) return 0;
    return heights[x][z];
}

