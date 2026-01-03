#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include "texture_atlas.h"

struct Block { std::string name; sf::Vector2i top, side, bottom; };

extern const int FACE_TOP;
extern const int FACE_BOTTOM;
extern const int FACE_FRONT;
extern const int FACE_BACK;
extern const int FACE_LEFT;
extern const int FACE_RIGHT;

void drawBlockAt(float gx, float gy, float gz, const Block &b, int faceMask, const TextureAtlas &atlas);
