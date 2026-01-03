#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <GL/gl.h>
#include <iostream>
#include <random>

static sf::Image makeTopImage(unsigned size){
    sf::Image img(sf::Vector2u{size, size}, sf::Color::Transparent);
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> d(0,255);
    // base green
    sf::Color grass(106,190,48);
    sf::Color dark(80,140,34);
    for(unsigned y=0;y<size;++y){
        for(unsigned x=0;x<size;++x){
            // sprinkle darker pixels
            if (d(rng) < 30) img.setPixel(sf::Vector2u{x,y}, dark);
            else img.setPixel(sf::Vector2u{x,y}, grass);
        }
    }
    return img;
}

static sf::Image makeSideImage(unsigned size){
    sf::Image img(sf::Vector2u{size,size}, sf::Color::Transparent);
    std::mt19937 rng(67890);
    std::uniform_int_distribution<int> d(0,255);
    sf::Color topGreen(106,190,48);
    sf::Color dirt(120,72,24);
    sf::Color darkDirt(90,60,20);
    // top strip of grass
    unsigned topH = size/4;
    for(unsigned y=0;y<size;++y){
        for(unsigned x=0;x<size;++x){
            if (y < topH){
                if (d(rng) < 20) img.setPixel(sf::Vector2u{x,y}, sf::Color(80,140,34));
                else img.setPixel(sf::Vector2u{x,y}, topGreen);
            } else {
                // dirt with noise
                if (d(rng) < 25) img.setPixel(sf::Vector2u{x,y}, darkDirt);
                else img.setPixel(sf::Vector2u{x,y}, dirt);
            }
        }
    }
    return img;
}

static sf::Image makeDirtImage(unsigned size){
    sf::Image img(sf::Vector2u{size,size}, sf::Color::Transparent);
    std::mt19937 rng(999);
    std::uniform_int_distribution<int> d(0,255);
    sf::Color dirt(120,72,24);
    sf::Color darkDirt(90,60,20);
    for(unsigned y=0;y<size;++y){
        for(unsigned x=0;x<size;++x){
            if (d(rng) < 30) img.setPixel(sf::Vector2u{x,y}, darkDirt);
            else img.setPixel(sf::Vector2u{x,y}, dirt);
        }
    }
    return img;
}

int main() {
    const unsigned WINDOW_W = 800, WINDOW_H = 600;
    sf::Window window(sf::VideoMode(sf::Vector2u{WINDOW_W, WINDOW_H}), "Cube - Textured (Minecraft-like)");
    window.setFramerateLimit(60);

    // Basic GL setup
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);

    // Create textures (try loading specific textures from assets/ first, else generate)
    const unsigned texSize = 64;
    sf::Texture topTex, sideTex, dirtTex;
    bool loadedTop = topTex.loadFromFile("assets/grass_top.png");
    bool loadedSide = sideTex.loadFromFile("assets/grass_side.png");
    bool loadedDirt = dirtTex.loadFromFile("assets/dirt.png");

    // Atlas support: if an atlas is present, use it instead. Configure tile size and
    // tile coordinates (col,row) for top/side/dirt below.
    sf::Texture atlasTex;
    const unsigned ATLAS_TILE_SIZE = 16; // default tile pixel size in the atlas
    // Default tile coords (col, row). Adjust these if your atlas arranges tiles differently.
    const sf::Vector2i TOP_TILE{0, 0};   // (col,row) for grass top tile in atlas
    const sf::Vector2i SIDE_TILE{1, 0};  // for grass side tile
    const sf::Vector2i DIRT_TILE{2, 0};  // for dirt tile
    bool atlasLoaded = atlasTex.loadFromFile("assets/atlas.png");
    if (atlasLoaded) {
        std::cout << "Loaded atlas: assets/atlas.png\n";
        atlasTex.setSmooth(false);
    }

    if (!atlasLoaded && (!loadedTop || !loadedSide || !loadedDirt)) {
        std::cout << "Some or all grass textures not found in assets/, generating procedural fallbacks.\n";
        if (!loadedTop){
            sf::Image img = makeTopImage(texSize);
            if (!topTex.loadFromImage(img)) std::cerr << "Failed to load generated top texture\n";
            else loadedTop = true;
        }
        if (!loadedSide){
            sf::Image img = makeSideImage(texSize);
            if (!sideTex.loadFromImage(img)) std::cerr << "Failed to load generated side texture\n";
            else loadedSide = true;
        }
        if (!loadedDirt){
            sf::Image img = makeDirtImage(texSize);
            if (!dirtTex.loadFromImage(img)) std::cerr << "Failed to load generated dirt texture\n";
            else loadedDirt = true;
        }
    }

    topTex.setSmooth(false);
    sideTex.setSmooth(false);
    dirtTex.setSmooth(false);

    float angle = 0.f;
    sf::Clock clock;

    while (window.isOpen()) {
        // Events
        while (const auto eventOpt = window.pollEvent()){
            const auto &event = *eventOpt;
            if (event.is<sf::Event::Closed>()) window.close();
            if (event.is<sf::Event::KeyPressed>()){
                auto kp = event.getIf<sf::Event::KeyPressed>();
                if (kp->code == sf::Keyboard::Key::Escape) window.close();
            }
        }

        float dt = clock.restart().asSeconds();
        angle += 60.f * dt; // degrees per second

        // Prepare viewport & projection
        auto size = window.getSize();
        int w = static_cast<int>(size.x);
        int h = static_cast<int>(size.y);
        float aspect = static_cast<float>(w) / static_cast<float>(h);

        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float viewSize = 1.5f;
        glOrtho(-aspect * viewSize, aspect * viewSize, -viewSize, viewSize, -10.f, 10.f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0.f, 0.f, -5.f);
        glRotatef(angle, 1.f, 1.f, 0.f);

        // Clear
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw a textured cube
        glColor3f(1,1,1);
        if (atlasLoaded) {
            // compute normalized coords for tiles in the atlas
            auto getUV = [&](const sf::Vector2i &tile){
                float aw = static_cast<float>(atlasTex.getSize().x);
                float ah = static_cast<float>(atlasTex.getSize().y);
                float x0 = (tile.x * ATLAS_TILE_SIZE) / aw;
                float x1 = (tile.x * ATLAS_TILE_SIZE + ATLAS_TILE_SIZE) / aw;
                // flip Y for OpenGL coordinates (SFML images origin is top-left)
                float y0 = 1.0f - (tile.y * ATLAS_TILE_SIZE + ATLAS_TILE_SIZE) / ah; // bottom
                float y1 = 1.0f - (tile.y * ATLAS_TILE_SIZE) / ah; // top
                return std::array<float,4>{x0,y0,x1,y1}; // u0,v0,u1,v1
            };

            auto sideUV = getUV(SIDE_TILE);
            auto topUV = getUV(TOP_TILE);
            auto dirtUV = getUV(DIRT_TILE);

            // Front (z+)
            sf::Texture::bind(&atlasTex);
            glBegin(GL_QUADS);
                glTexCoord2f(sideUV[0], sideUV[1]); glVertex3f(-1,-1, 1);
                glTexCoord2f(sideUV[2], sideUV[1]); glVertex3f( 1,-1, 1);
                glTexCoord2f(sideUV[2], sideUV[3]); glVertex3f( 1, 1, 1);
                glTexCoord2f(sideUV[0], sideUV[3]); glVertex3f(-1, 1, 1);
            glEnd();

            // Back (z-)
            sf::Texture::bind(&atlasTex);
            glBegin(GL_QUADS);
                glTexCoord2f(sideUV[2], sideUV[1]); glVertex3f(-1,-1,-1);
                glTexCoord2f(sideUV[2], sideUV[3]); glVertex3f(-1, 1,-1);
                glTexCoord2f(sideUV[0], sideUV[3]); glVertex3f( 1, 1,-1);
                glTexCoord2f(sideUV[0], sideUV[1]); glVertex3f( 1,-1,-1);
            glEnd();

            // Left (x-)
            sf::Texture::bind(&atlasTex);
            glBegin(GL_QUADS);
                glTexCoord2f(sideUV[0], sideUV[1]); glVertex3f(-1,-1,-1);
                glTexCoord2f(sideUV[2], sideUV[1]); glVertex3f(-1,-1, 1);
                glTexCoord2f(sideUV[2], sideUV[3]); glVertex3f(-1, 1, 1);
                glTexCoord2f(sideUV[0], sideUV[3]); glVertex3f(-1, 1,-1);
            glEnd();

            // Right (x+)
            sf::Texture::bind(&atlasTex);
            glBegin(GL_QUADS);
                glTexCoord2f(sideUV[2], sideUV[1]); glVertex3f(1,-1,-1);
                glTexCoord2f(sideUV[2], sideUV[3]); glVertex3f(1, 1,-1);
                glTexCoord2f(sideUV[0], sideUV[3]); glVertex3f(1, 1, 1);
                glTexCoord2f(sideUV[0], sideUV[1]); glVertex3f(1,-1, 1);
            glEnd();

            // Top (y+)
            sf::Texture::bind(&atlasTex);
            glBegin(GL_QUADS);
                glTexCoord2f(topUV[0], topUV[1]); glVertex3f(-1,1,-1);
                glTexCoord2f(topUV[0], topUV[3]); glVertex3f(-1,1, 1);
                glTexCoord2f(topUV[2], topUV[3]); glVertex3f( 1,1, 1);
                glTexCoord2f(topUV[2], topUV[1]); glVertex3f( 1,1,-1);
            glEnd();

            // Bottom (y-)
            sf::Texture::bind(&atlasTex);
            glBegin(GL_QUADS);
                glTexCoord2f(dirtUV[0], dirtUV[1]); glVertex3f(-1,-1,-1);
                glTexCoord2f(dirtUV[2], dirtUV[1]); glVertex3f( 1,-1,-1);
                glTexCoord2f(dirtUV[2], dirtUV[3]); glVertex3f( 1,-1, 1);
                glTexCoord2f(dirtUV[0], dirtUV[3]); glVertex3f(-1,-1, 1);
            glEnd();

            sf::Texture::bind(nullptr);
        } else {
            // fallback to individual textures (existing behavior)
            // Front (z+): side texture
            sf::Texture::bind(&sideTex);
            glBegin(GL_QUADS);
                glTexCoord2f(0,0); glVertex3f(-1,-1, 1);
                glTexCoord2f(1,0); glVertex3f( 1,-1, 1);
                glTexCoord2f(1,1); glVertex3f( 1, 1, 1);
                glTexCoord2f(0,1); glVertex3f(-1, 1, 1);
            glEnd();

            // Back (z-): side texture (mirrored horizontally)
            sf::Texture::bind(&sideTex);
            glBegin(GL_QUADS);
                glTexCoord2f(1,0); glVertex3f(-1,-1,-1);
                glTexCoord2f(1,1); glVertex3f(-1, 1,-1);
                glTexCoord2f(0,1); glVertex3f( 1, 1,-1);
                glTexCoord2f(0,0); glVertex3f( 1,-1,-1);
            glEnd();

            // Left (x-): side texture
            sf::Texture::bind(&sideTex);
            glBegin(GL_QUADS);
                glTexCoord2f(0,0); glVertex3f(-1,-1,-1);
                glTexCoord2f(1,0); glVertex3f(-1,-1, 1);
                glTexCoord2f(1,1); glVertex3f(-1, 1, 1);
                glTexCoord2f(0,1); glVertex3f(-1, 1,-1);
            glEnd();

            // Right (x+): side texture (mirrored)
            sf::Texture::bind(&sideTex);
            glBegin(GL_QUADS);
                glTexCoord2f(1,0); glVertex3f(1,-1,-1);
                glTexCoord2f(1,1); glVertex3f(1, 1,-1);
                glTexCoord2f(0,1); glVertex3f(1, 1, 1);
                glTexCoord2f(0,0); glVertex3f(1,-1, 1);
            glEnd();

            // Top (y+): top texture
            sf::Texture::bind(&topTex);
            glBegin(GL_QUADS);
                glTexCoord2f(0,1); glVertex3f(-1,1,-1);
                glTexCoord2f(0,0); glVertex3f(-1,1, 1);
                glTexCoord2f(1,0); glVertex3f( 1,1, 1);
                glTexCoord2f(1,1); glVertex3f( 1,1,-1);
            glEnd();

            // Bottom (y-): dirt texture
            sf::Texture::bind(&dirtTex);
            glBegin(GL_QUADS);
                glTexCoord2f(0,1); glVertex3f(-1,-1,-1);
                glTexCoord2f(1,1); glVertex3f( 1,-1,-1);
                glTexCoord2f(1,0); glVertex3f( 1,-1, 1);
                glTexCoord2f(0,0); glVertex3f(-1,-1, 1);
            glEnd();

            // Unbind texture
            sf::Texture::bind(nullptr);
        }

        window.display();
    }

    return 0;
}
