#include "texture_atlas.h"
#include <iostream>

bool TextureAtlas::loadAtlas(const std::string &path){
    atlasLoaded = false;
    if (atlasTex.loadFromFile(path)){
        atlasLoaded = true;
        atlasTex.setSmooth(false);
        atlasTex.setRepeated(false);
        // detect tile size
        std::vector<int> candidates = {16,32,64,8};
        for (int c : candidates){
            if (atlasTex.getSize().x % c == 0 && atlasTex.getSize().y % c == 0){
                atlasTileSize = c;
                atlasCols = atlasTex.getSize().x / atlasTileSize;
                atlasRows = atlasTex.getSize().y / atlasTileSize;
                break;
            }
        }
        if (atlasTileSize == 0){ atlasCols = 1; atlasRows = 6; atlasTileSize = atlasTex.getSize().x / atlasCols; }
        return true;
    }
    return false;
}

bool TextureAtlas::loadFallbacks(){
    bool loadedTop = topTex.loadFromFile("assets/grass_top.png");
    bool loadedSide = sideTex.loadFromFile("assets/grass_side.png");
    bool loadedDirt = dirtTex.loadFromFile("assets/dirt.png");
    topTex.setSmooth(false); topTex.setRepeated(false);
    sideTex.setSmooth(false); sideTex.setRepeated(false);
    dirtTex.setSmooth(false); dirtTex.setRepeated(false);
    return loadedTop && loadedSide && loadedDirt;
}

std::array<float,4> TextureAtlas::getUV_fromAtlasTile(const sf::Vector2i &tile) const{
    if (!atlasLoaded) return {0.f,0.f,1.f,1.f};
    float aw = static_cast<float>(atlasTex.getSize().x);
    float ah = static_cast<float>(atlasTex.getSize().y);
    float x0 = (tile.x * atlasTileSize) / aw;
    float x1 = (tile.x * atlasTileSize + atlasTileSize) / aw;
    float y0 = 1.0f - (tile.y * atlasTileSize + atlasTileSize) / ah; // bottom
    float y1 = 1.0f - (tile.y * atlasTileSize) / ah; // top
    return {x0,y0,x1,y1};
}

void TextureAtlas::bindTop() const{ if (atlasLoaded) sf::Texture::bind(&atlasTex); else sf::Texture::bind(&topTex); }
void TextureAtlas::bindSide() const{ if (atlasLoaded) sf::Texture::bind(&atlasTex); else sf::Texture::bind(&sideTex); }
void TextureAtlas::bindDirt() const{ if (atlasLoaded) sf::Texture::bind(&atlasTex); else sf::Texture::bind(&dirtTex); }
