#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <string>

struct TextureAtlas {
    bool atlasLoaded = false;
    sf::Texture atlasTex;
    sf::Texture topTex, sideTex, dirtTex;
    int atlasTileSize = 0;
    int atlasCols = 0, atlasRows = 0;
    sf::Vector2i TOP_TILE{0,0};
    sf::Vector2i SIDE_TILE{1,0};
    sf::Vector2i DIRT_TILE{2,0};

    bool loadAtlas(const std::string &path);
    bool loadFallbacks();
    std::array<float,4> getUV_fromAtlasTile(const sf::Vector2i &tile) const;
    void bindTop() const;
    void bindSide() const;
    void bindDirt() const;
};
