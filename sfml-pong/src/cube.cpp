#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <GL/gl.h>
#include <iostream>
#include <random>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <string>
#include "texture_atlas.h"
#include "world.h"
#include "rendering.h"
#include <fstream>

// Helper to find assets directory
static std::string findAssetsDirectory() {
    std::vector<std::string> candidates = {
        "assets",
        "sfml-pong/assets",
        "../assets",
        "../../assets",
        "sfml-pong/build_fix/assets",
        "build_fix/assets"
    };
    for (const auto& p : candidates) {
        std::ifstream f(p + "/atlas.png");
        if (f.good()) return p;
    }
    return "assets"; // Default fallback
}

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
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u{WINDOW_W, WINDOW_H}), "Cube - Textured (Minecraft-like)");
    window.setFramerateLimit(60);

    // Basic GL setup
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_2D);
    // Sky Blue background
    glClearColor(0.529f, 0.808f, 0.922f, 1.0f);

    // Texture & atlas initialization
    TextureAtlas atlas;
    std::string assetsDir = findAssetsDirectory();
    std::cout << "Found assets directory: " << assetsDir << "\n";
    
    // Load atlas using the found directory
    std::string atlasPath = assetsDir + "/atlas.png";
    if (atlas.loadAtlas(atlasPath)) {
        std::cout << "Loaded atlas: " << atlasPath << "\n";
    }

    // Load fallbacks if needed, using the detected assets directory
    if (!atlas.atlasLoaded) {
        atlas.loadFallbacks(assetsDir);
    }

    // Auto-detect likely tiles for grass-top, grass-side and dirt if atlas loaded
    if (atlas.atlasLoaded){
        try{
            sf::Image atlasImg = atlas.atlasTex.copyToImage();
            struct Scores { float green=0.f; float brown=0.f; float topGreenFrac=0.f; float bottomBrownFrac=0.f; };
            std::vector<Scores> scores(atlas.atlasCols * atlas.atlasRows);
            for(int ty=0; ty<atlas.atlasRows; ++ty){
                for(int tx=0; tx<atlas.atlasCols; ++tx){
                    Scores s;
                    int baseX = tx * atlas.atlasTileSize;
                    int baseY = ty * atlas.atlasTileSize;
                    int total = atlas.atlasTileSize * atlas.atlasTileSize;
                    int topCount=0, topGreen=0;
                    int bottomCount=0, bottomBrown=0;
                    for(int y=0;y<atlas.atlasTileSize;++y){
                        for(int x=0;x<atlas.atlasTileSize;++x){
                            auto c = atlasImg.getPixel(sf::Vector2u{static_cast<unsigned>(baseX + x), static_cast<unsigned>(baseY + y)});
                            float r = static_cast<float>(c.r);
                            float g = static_cast<float>(c.g);
                            float b = static_cast<float>(c.b);
                            s.green += std::max(0.f, g - (r + b) * 0.5f);
                            s.brown += std::max(0.f, r - (g + b) * 0.5f);
                            if (y < atlas.atlasTileSize/4){ ++topCount; if (g > r + 8 && g > b + 8) ++topGreen; }
                            if (y >= atlas.atlasTileSize/2){ ++bottomCount; if (r > g + 6 && r > b) ++bottomBrown; }
                        }
                    }
                    s.topGreenFrac = topCount ? (float)topGreen / (float)topCount : 0.f;
                    s.bottomBrownFrac = bottomCount ? (float)bottomBrown / (float)bottomCount : 0.f;
                    s.green /= static_cast<float>(total);
                    s.brown /= static_cast<float>(total);
                    scores[ty * atlas.atlasCols + tx] = s;
                }
            }

            float bestTopScore = -1.f; sf::Vector2i bestTop{0,0};
            float bestDirtScore = -1.f; sf::Vector2i bestDirt{0,0};
            float bestSideScore = -1.f; sf::Vector2i bestSide{0,0};
            for(int ty=0; ty<atlas.atlasRows; ++ty){
                for(int tx=0; tx<atlas.atlasCols; ++tx){
                    auto &s = scores[ty*atlas.atlasCols + tx];
                    float topScore = s.green * (0.7f + 0.6f * s.topGreenFrac);
                    float dirtScore = s.brown;
                    float sideScore = s.topGreenFrac * (0.5f + s.bottomBrownFrac);
                    if (topScore > bestTopScore){ bestTopScore = topScore; atlas.TOP_TILE = {tx,ty}; }
                    if (dirtScore > bestDirtScore){ bestDirtScore = dirtScore; atlas.DIRT_TILE = {tx,ty}; }
                    if (sideScore > bestSideScore){ bestSideScore = sideScore; atlas.SIDE_TILE = {tx,ty}; }
                }
            }
            std::cout << "Auto-detected tiles: TOP(" << atlas.TOP_TILE.x << "," << atlas.TOP_TILE.y << ") SIDE(" << atlas.SIDE_TILE.x << "," << atlas.SIDE_TILE.y << ") DIRT(" << atlas.DIRT_TILE.x << "," << atlas.DIRT_TILE.y << ")\n";
        } catch(...){ std::cout << "Atlas auto-detection failed, keep defaults.\n"; }
    }

    // ensure procedural fallbacks are generated if needed
    unsigned texSize = 64;
    if (!atlas.atlasLoaded && !atlas.loadFallbacks(assetsDir)){
        std::cout << "Some or all grass textures not found in assets/, generating procedural fallbacks.\n";
        sf::Image img = makeTopImage(texSize); atlas.topTex.loadFromImage(img);
        img = makeSideImage(texSize); atlas.sideTex.loadFromImage(img);
        img = makeDirtImage(texSize); atlas.dirtTex.loadFromImage(img);
    }

    // --- Block definition and terrain ----------------------------------
    std::vector<Block> blocks;
    // user-specified dirt block (all faces same tile)
    blocks.push_back({"Dirt", sf::Vector2i{2,0}, sf::Vector2i{2,0}, sf::Vector2i{2,0}});
    // grass block: top/side/bottom
    if (atlas.atlasLoaded) {
        blocks.push_back({"Grass", sf::Vector2i{11,8}, sf::Vector2i{11,8}, sf::Vector2i{11,8}});
        blocks.push_back({"Stone", sf::Vector2i{1,0}, sf::Vector2i{1,0}, sf::Vector2i{1,0}});
    } else {
        blocks.push_back({"Grass", sf::Vector2i{0,0}, sf::Vector2i{0,0}, sf::Vector2i{2,0}}); // Fallback
        blocks.push_back({"Stone", sf::Vector2i{2,0}, sf::Vector2i{2,0}, sf::Vector2i{2,0}}); // Fallback to Dirt
    }

    int currentBlockIndex = 1; // Grass by default

    // Terrain seed and generation (uses world module)
    int terrainSeed = 123;
    generateTerrain(terrainSeed);





    

    std::cout << "Controls: Arrow keys = rotate camera, W/S = zoom (or fly forward/back when Fly is ON), A/D/Q/E = pan (or strafe when Fly is ON), R = regenerate terrain (seed+1), M = random seed, 1/2 = select blocks, F = toggle Fly, C = toggle FPS, V = invert mouse\n"
              << "T/Y = cycle top tile, G/H = cycle side tile, B/N = cycle dirt tile, +/- or PgUp/PgDn = adjust fly speed. ESC = exit.\n";


    // Camera (orbit) parameters
    float camDistance = 5.0f;
    float camYawDeg = 45.0f;
    float camPitchDeg = -10.0f;
    sf::Vector3f camCenter{0.f, 0.f, 0.f};

    // Mouse control state
    bool rotating = false;
    bool panning = false;
    sf::Vector2i lastMouse{0,0};
    float mouseRotateSpeed = 0.25f; // degrees per pixel
    float mousePanFactor = 0.01f;   // world units per pixel
    float scrollZoomSpeed = 0.8f;   // units per scroll step

    std::cout << "Controls: Arrow keys = rotate camera, W/S = zoom, A/D/Q/E = pan, R = regenerate terrain, Space = random seed,\n"
                 "Mouse: Left-drag = rotate, Right-drag = pan, Scroll = zoom.\n"
                 "T/Y = cycle top tile, G/H = cycle side tile, B/N = cycle dirt tile. ESC = exit.\n";

    float angle = 0.f;
    sf::Clock clock;

    // FPS counter and HUD
    int frameCount = 0;
    float fps = 0.f;
    float fpsAccum = 0.f;
    sf::Font hudFont;
    bool haveFont = false;
    // try project font first (assets), else try system Arial
if (hudFont.openFromFile("assets/arial.ttf") || hudFont.openFromFile("C:/Windows/Fonts/arial.ttf")){
        haveFont = true;
    }
    // Construct Text with font and character size (SFML3 requires arguments)
    sf::Text fpsText(hudFont, "", 16);
    if (haveFont){
        fpsText.setFillColor(sf::Color::White);
        fpsText.setPosition(sf::Vector2f{6.f, 6.f});
    }

    // Built-in 3x5 pixel digit font used when no TTF is available
    static const unsigned char DIGITS[10][5] = {
        {0b111,0b101,0b101,0b101,0b111}, //0
        {0b010,0b110,0b010,0b010,0b111}, //1
        {0b111,0b001,0b111,0b100,0b111}, //2
        {0b111,0b001,0b111,0b001,0b111}, //3
        {0b101,0b101,0b111,0b001,0b001}, //4
        {0b111,0b100,0b111,0b001,0b111}, //5
        {0b111,0b100,0b111,0b101,0b111}, //6
        {0b111,0b001,0b010,0b010,0b010}, //7
        {0b111,0b101,0b111,0b101,0b111}, //8
        {0b111,0b101,0b111,0b001,0b111}  //9
    };
    auto drawBitmapText = [&](const std::string &s, int px, int py, int scale){
        // draw using small rectangles via SFML shapes (push GL states first)
        sf::RectangleShape rect(sf::Vector2f((float)scale, (float)scale));
        int x = px;
        for(char c : s){
            if (c >= '0' && c <= '9'){
                int d = c - '0';
                for(int ry=0; ry<5; ++ry){
                    unsigned row = DIGITS[d][ry];
                    for(int rx=0; rx<3; ++rx){
                        if (row & (1u << (2-rx))){
                            rect.setPosition(sf::Vector2f{(float)(x + rx*scale), (float)(py + ry*scale)});
                            rect.setFillColor(sf::Color::White);
                            window.draw(rect);
                        }
                    }
                }
                x += (3 + 1) * scale;
            } else {
                // simple spacer for other chars
                x += (3 + 1) * scale;
            }
        }
    };

    // Fly / creative mode
    bool flyMode = false;
    float flySpeed = 6.0f; // units/sec
    float flySpeedBoost = 2.5f; // multiplier for LCtrl
    bool showSpeed = true; // show speed on HUD

    // FPS (first-person) mode
    bool fpsMode = false;
    sf::Vector3f playerPos{0.f, 2.f, 0.f};
    float playerVy = 0.f;
    bool canJump = false;
    float gravity = 20.0f;
    const float eyeHeight = 1.62f;
    const float walkSpeed = 4.0f;
    const float sprintMultiplier = 1.9f;
    float mouseLookSpeed = 0.15f; // degrees per pixel-ish
    float jumpSpeed = 6.5f; // upward impulse
    bool invertMouse = false; // invert Y axis for mouse look (toggle V)
    
    // Keep last mouse so we can re-center for FPS look
    sf::Vector2i fpsCenterMouse{0,0};


    while (window.isOpen()) {
        // Events
        while (const auto eventOpt = window.pollEvent()){
            const auto &event = *eventOpt;
            if (event.is<sf::Event::Closed>()) window.close();
            if (event.is<sf::Event::KeyPressed>()){
                auto kp = event.getIf<sf::Event::KeyPressed>();
                if (kp->code == sf::Keyboard::Key::Escape) window.close();
                // atlas tile cycling keys (only meaningful when atlas is loaded)
                if (atlas.atlasLoaded){
                    if (kp->code == sf::Keyboard::Key::T){ atlas.TOP_TILE.x = (atlas.TOP_TILE.x + 1) % std::max(1, atlas.atlasCols); std::cout << "TOP tile = (" << atlas.TOP_TILE.x << "," << atlas.TOP_TILE.y << ")\n"; }
                    if (kp->code == sf::Keyboard::Key::Y){ atlas.TOP_TILE.y = (atlas.TOP_TILE.y + 1) % std::max(1, atlas.atlasRows); std::cout << "TOP tile = (" << atlas.TOP_TILE.x << "," << atlas.TOP_TILE.y << ")\n"; }
                    if (kp->code == sf::Keyboard::Key::G){ atlas.SIDE_TILE.x = (atlas.SIDE_TILE.x + 1) % std::max(1, atlas.atlasCols); std::cout << "SIDE tile = (" << atlas.SIDE_TILE.x << "," << atlas.SIDE_TILE.y << ")\n"; }
                    if (kp->code == sf::Keyboard::Key::H){ atlas.SIDE_TILE.y = (atlas.SIDE_TILE.y + 1) % std::max(1, atlas.atlasRows); std::cout << "SIDE tile = (" << atlas.SIDE_TILE.x << "," << atlas.SIDE_TILE.y << ")\n"; }
                    if (kp->code == sf::Keyboard::Key::B){ atlas.DIRT_TILE.x = (atlas.DIRT_TILE.x + 1) % std::max(1, atlas.atlasCols); std::cout << "DIRT tile = (" << atlas.DIRT_TILE.x << "," << atlas.DIRT_TILE.y << ")\n"; }
                    if (kp->code == sf::Keyboard::Key::N){ atlas.DIRT_TILE.y = (atlas.DIRT_TILE.y + 1) % std::max(1, atlas.atlasRows); std::cout << "DIRT tile = (" << atlas.DIRT_TILE.x << "," << atlas.DIRT_TILE.y << ")\n"; }
                }

                // Terrain and block controls
                if (kp->code == sf::Keyboard::Key::R){ generateTerrain(terrainSeed + 1); }
                if (kp->code == sf::Keyboard::Key::M){ std::random_device rd; generateTerrain(rd()); }
                // Jump when in FPS walking mode; otherwise Space was moved to M earlier
                if (kp->code == sf::Keyboard::Key::Space){ if (fpsMode && !flyMode && canJump){ playerVy = jumpSpeed; canJump = false; } }
                if (kp->code == sf::Keyboard::Key::F){ flyMode = !flyMode; std::cout << "Fly mode " << (flyMode ? "ON" : "OFF") << "\n"; }
                // Mouse sensitivity +/-, jump speed U/J, gravity I/K, reset 0
                if (kp->code == sf::Keyboard::Key::LBracket){ mouseLookSpeed = std::max(0.01f, mouseLookSpeed - 0.01f); std::cout << "Mouse sens = " << mouseLookSpeed << "\n"; }
                if (kp->code == sf::Keyboard::Key::RBracket){ mouseLookSpeed = std::min(5.0f, mouseLookSpeed + 0.01f); std::cout << "Mouse sens = " << mouseLookSpeed << "\n"; }
                if (kp->code == sf::Keyboard::Key::U){ jumpSpeed += 0.5f; std::cout << "Jump = " << jumpSpeed << "\n"; }
                if (kp->code == sf::Keyboard::Key::J){ jumpSpeed = std::max(0.5f, jumpSpeed - 0.5f); std::cout << "Jump = " << jumpSpeed << "\n"; }
                if (kp->code == sf::Keyboard::Key::I){ /* increase gravity */ const_cast<float&>(gravity) += 1.0f; std::cout << "Gravity = " << gravity << "\n"; }
                if (kp->code == sf::Keyboard::Key::K){ const_cast<float&>(gravity) = std::max(0.1f, gravity - 1.0f); std::cout << "Gravity = " << gravity << "\n"; }
                if (kp->code == sf::Keyboard::Key::Num0){ mouseLookSpeed = 0.15f; jumpSpeed = 6.5f; gravity = 20.0f; std::cout << "Tunables reset\n"; }
                if (kp->code == sf::Keyboard::Key::V){ invertMouse = !invertMouse; std::cout << "Invert mouse " << (invertMouse ? "ON" : "OFF") << "\n"; }
                if (kp->code == sf::Keyboard::Key::Equal || kp->code == sf::Keyboard::Key::Add || kp->code == sf::Keyboard::Key::PageUp){ flySpeed *= 1.1f; if (flySpeed > 100.f) flySpeed = 100.f; std::cout << "Fly speed = " << flySpeed << "\n"; }
                if (kp->code == sf::Keyboard::Key::Hyphen || kp->code == sf::Keyboard::Key::Subtract || kp->code == sf::Keyboard::Key::PageDown){ flySpeed /= 1.1f; if (flySpeed < 0.01f) flySpeed = 0.01f; std::cout << "Fly speed = " << flySpeed << "\n"; }
                if (kp->code == sf::Keyboard::Key::C){
                    fpsMode = !fpsMode;
                    auto s = window.getSize();
                    fpsCenterMouse = sf::Vector2i{static_cast<int>(s.x/2), static_cast<int>(s.y/2)};
                    sf::Mouse::setPosition(fpsCenterMouse, window);
                    window.setMouseCursorVisible(!fpsMode);
                    window.setMouseCursorGrabbed(fpsMode);
                    // set initial player position from camera center
                    playerPos.x = camCenter.x;
                    playerPos.z = camCenter.z;
                    // place player above terrain
                    int half = CHUNK/2;
                    int xi = std::clamp(int(round(playerPos.x + half)), 0, CHUNK-1);
                    int zi = std::clamp(int(round(playerPos.z + half)), 0, CHUNK-1);
                    float groundY = static_cast<float>(getHeightAt(xi, zi)) + eyeHeight;
                    // Ensure we spawn slightly above ground to avoid sticking
                    if (playerPos.y < groundY + 0.5f) playerPos.y = groundY + 0.5f;
                    playerVy = 0.f;
                    canJump = true;
                    std::cout << "FPS mode " << (fpsMode ? "ON" : "OFF") << "\n";
                }
                if (kp->code == sf::Keyboard::Key::Num1){ currentBlockIndex = 0; std::cout << "Selected block: " << blocks[currentBlockIndex].name << "\n"; }
                if (kp->code == sf::Keyboard::Key::Num2){ if (blocks.size() > 1) { currentBlockIndex = 1; std::cout << "Selected block: " << blocks[currentBlockIndex].name << "\n"; } }
            } else if (event.is<sf::Event::MouseButtonPressed>()){
                auto mb = event.getIf<sf::Event::MouseButtonPressed>();
                if (mb->button == sf::Mouse::Button::Left){ rotating = true; lastMouse = sf::Mouse::getPosition(window); }
                if (mb->button == sf::Mouse::Button::Right){ panning = true; lastMouse = sf::Mouse::getPosition(window); }
            } else if (event.is<sf::Event::MouseButtonReleased>()){
                auto mb = event.getIf<sf::Event::MouseButtonReleased>();
                if (mb->button == sf::Mouse::Button::Left) rotating = false;
                if (mb->button == sf::Mouse::Button::Right) panning = false;
            } else if (event.is<sf::Event::MouseMoved>()){
                // In FPS mode we don't use event-driven mouse moves; we poll relative to center each frame.
                if (fpsMode) {
                    // ignore
                } else {
                    // Use global mouse position (safer API across SFML versions)
                    sf::Vector2i cur = sf::Mouse::getPosition(window);
                    sf::Vector2i d = cur - lastMouse;
                    lastMouse = cur;
                    if (rotating){
                        float sign = invertMouse ? -1.f : 1.f;
                        camYawDeg += static_cast<float>(d.x) * mouseRotateSpeed * sign;
                        camPitchDeg += static_cast<float>(d.y) * mouseRotateSpeed * sign;
                        if (camPitchDeg > 89.f) camPitchDeg = 89.f;
                        if (camPitchDeg < -89.f) camPitchDeg = -89.f;
                    }
                    if (panning){
                        float panFactor = mousePanFactor * camDistance;
                        camCenter.x -= static_cast<float>(d.x) * panFactor;
                        camCenter.y += static_cast<float>(d.y) * panFactor;
                    }
                }
            } else if (event.is<sf::Event::MouseWheelScrolled>()){
                auto ws = event.getIf<sf::Event::MouseWheelScrolled>();
                camDistance -= ws->delta * scrollZoomSpeed;
                if (camDistance < 1.f) camDistance = 1.f;
            }
        }

        float dt = clock.restart().asSeconds();
        angle += 30.f * dt; // cube self-rotation

        // FPS accounting
        frameCount++;
        fpsAccum += dt;
        if (fpsAccum >= 1.0f){
            fps = static_cast<float>(frameCount) / fpsAccum;
            frameCount = 0;
            fpsAccum = 0.f;
            char buf[256];
            snprintf(buf, sizeof(buf), "Cube - Textured (Minecraft-like) - FPS: %.0f%s", fps, (flyMode ? " - FLY" : ""));
            window.setTitle(buf);
            if (haveFont) fpsText.setString(std::to_string(static_cast<int>(fps)) + " FPS");
        }

        // Handle continuous camera controls (keyboard held)
        const float yawSpeed = 90.f; // deg/sec
        const float pitchSpeed = 80.f; // deg/sec
        const float zoomSpeed = 3.f; // units/sec
        const float panSpeed = 2.f; // units/sec
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) camYawDeg -= yawSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) camYawDeg += yawSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) camPitchDeg += pitchSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) camPitchDeg -= pitchSpeed * dt;
        if (fpsMode){
            // FPS mouse look: read cursor delta relative to center and re-center every frame
            auto sz = window.getSize();
            sf::Vector2i center{static_cast<int>(sz.x/2), static_cast<int>(sz.y/2)};
            sf::Vector2i cur = sf::Mouse::getPosition(window);
            sf::Vector2i d = cur - center;
            // always reset mouse to center so we can get relative movement next frame
            sf::Mouse::setPosition(center, window);
            if (d.x != 0 || d.y != 0){
                float sign = invertMouse ? -1.f : 1.f;
                camYawDeg += static_cast<float>(d.x) * mouseLookSpeed * sign;
                camPitchDeg += static_cast<float>(d.y) * mouseLookSpeed * sign;
                if (camPitchDeg > 89.f) camPitchDeg = 89.f;
                if (camPitchDeg < -89.f) camPitchDeg = -89.f;
            }

            // Movement on XZ plane for walking
            float yawR = camYawDeg * 3.14159265f / 180.f;
            sf::Vector3f forwardXZ{ sinf(yawR), 0.f, cosf(yawR) };
            sf::Vector3f rightXZ{ forwardXZ.z, 0.f, -forwardXZ.x };
            float fwd=0.f, rgt=0.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) fwd += 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) fwd -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) rgt += 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) rgt -= 1.f;
            // invert strafing when invertMouse is set
            rgt *= (invertMouse ? -1.f : 1.f);
            float speed = walkSpeed;
            bool sprinting = false;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) { speed *= sprintMultiplier; sprinting = true; }

            if (flyMode){
                // Fly-style FPS (no gravity), include pitch for forward/back
                float pitchR = camPitchDeg * 3.14159265f / 180.f;
                sf::Vector3f forward{ sinf(yawR) * cosf(pitchR), sinf(pitchR), cosf(yawR) * cosf(pitchR) };
                sf::Vector3f right{ forward.y*0.f - forward.z*0.f, forward.z*1.f - forward.x*0.f, forward.x*0.f - forward.y*1.f };
                float upf = 0.f;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) upf += 1.f;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) upf -= 1.f;
                sf::Vector3f delta = sf::Vector3f{ forward.x * fwd + right.x * rgt, forward.y * fwd + upf, forward.z * fwd + right.z * rgt };
                playerPos.x += delta.x * speed * dt;
                playerPos.y += delta.y * speed * dt;
                playerPos.z += delta.z * speed * dt;
            } else {
                // walking with gravity and simple block collision
                sf::Vector3f delta = sf::Vector3f{ forwardXZ.x * fwd + rightXZ.x * rgt, 0.f, forwardXZ.z * fwd + rightXZ.z * rgt };
                float moveX = delta.x * speed * dt;
                float moveZ = delta.z * speed * dt;
                float oldX = playerPos.x;
                float oldZ = playerPos.z;

                // attempt X movement
                playerPos.x += moveX;
                int half = CHUNK/2;
                int xi = std::clamp(int(round(playerPos.x + half)), 0, CHUNK-1);
                int zi = std::clamp(int(round(playerPos.z + half)), 0, CHUNK-1);
                float footY = playerPos.y - eyeHeight;
                if (getHeightAt(xi, zi) > footY + 0.2f){ // blocked
                    playerPos.x = oldX; // rollback X
                }

                // attempt Z movement
                playerPos.z += moveZ;
                xi = std::clamp(int(round(playerPos.x + half)), 0, CHUNK-1);
                zi = std::clamp(int(round(playerPos.z + half)), 0, CHUNK-1);
                if (getHeightAt(xi, zi) > footY + 0.2f){ // blocked
                    playerPos.z = oldZ; // rollback Z
                }

                // Apply gravity
                playerVy -= gravity * dt;
                playerPos.y += playerVy * dt;

                // ground collision
                xi = std::clamp(int(round(playerPos.x + half)), 0, CHUNK-1);
                zi = std::clamp(int(round(playerPos.z + half)), 0, CHUNK-1);
                float groundY = static_cast<float>(getHeightAt(xi, zi)) + eyeHeight;
                if (playerPos.y <= groundY){
                    playerPos.y = groundY;
                    playerVy = 0.f;
                    canJump = true;
                }
            }
        } else if (!flyMode){
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) camDistance -= zoomSpeed * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) camDistance += zoomSpeed * dt;
            float hSign = invertMouse ? -1.f : 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) camCenter.x -= panSpeed * dt * hSign;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) camCenter.x += panSpeed * dt * hSign;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)) camCenter.y += panSpeed * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) camCenter.y -= panSpeed * dt;
        } else {
            // Fly movement: W/S forward/back, A/D strafe, Space up, LShift down
            float yawR = camYawDeg * 3.14159265f / 180.f;
            float pitchR = camPitchDeg * 3.14159265f / 180.f;
            sf::Vector3f forward{ sinf(yawR) * cosf(pitchR), sinf(pitchR), cosf(yawR) * cosf(pitchR) };
            sf::Vector3f upVec{0.f,1.f,0.f};
            sf::Vector3f right{ forward.y*upVec.z - forward.z*upVec.y, forward.z*upVec.x - forward.x*upVec.z, forward.x*upVec.y - forward.y*upVec.x };
            auto normalize3 = [](sf::Vector3f v)->sf::Vector3f{ float l = sqrtf(v.x*v.x+v.y*v.y+v.z*v.z); if (l==0.f) return v; return sf::Vector3f{v.x/l, v.y/l, v.z/l}; };
            forward = normalize3(forward);
            right = normalize3(right);
            float fwd=0.f, rgt=0.f, upf=0.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) fwd += 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) fwd -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) rgt += 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) rgt -= 1.f;
            // invert strafe when invertMouse is set
            rgt *= (invertMouse ? -1.f : 1.f);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) upf += 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) upf -= 1.f;
            float speed = flySpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) speed *= flySpeedBoost;
            sf::Vector3f delta{ forward.x * fwd + right.x * rgt, forward.y * fwd + right.y * rgt + upf, forward.z * fwd + right.z * rgt };
            camCenter.x += delta.x * speed * dt;
            camCenter.y += delta.y * speed * dt;
            camCenter.z += delta.z * speed * dt;
        }
        // clamp
        if (camDistance < 1.0f) camDistance = 1.0f;
        if (camPitchDeg > 89.f) camPitchDeg = 89.f;
        if (camPitchDeg < -89.f) camPitchDeg = -89.f;

        // Prepare viewport & perspective projection
        auto size = window.getSize();
        int w = static_cast<int>(size.x);
        int h = static_cast<int>(size.y);
        float aspect = static_cast<float>(w) / static_cast<float>(h);

        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        const float fov = 60.f;
        const float znear = 0.1f;
        const float zfar = 100.f;
        float top = znear * tanf(fov * 3.14159265f / 360.f);
        float right = top * aspect;
        glFrustum(-right, right, -top, top, znear, zfar);

        // Camera (view) matrix - compute lookAt and load it into MODELVIEW
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        // compute eye position and center from either orbit or FPS mode
        float yawRad = camYawDeg * 3.14159265f / 180.f;
        float pitchRad = camPitchDeg * 3.14159265f / 180.f;
        sf::Vector3f eye;
        sf::Vector3f center;
        if (!fpsMode){
            float ex = camCenter.x + camDistance * cosf(pitchRad) * sinf(yawRad);
            float ey = camCenter.y + camDistance * sinf(pitchRad);
            float ez = camCenter.z + camDistance * cosf(pitchRad) * cosf(yawRad);
            eye = sf::Vector3f{ex, ey, ez};
            center = sf::Vector3f{camCenter};
        } else {
            // First-person: eye is player position, center is position + forward vector
            eye = playerPos;
            float fx = sinf(yawRad) * cosf(pitchRad);
            float fy = sinf(pitchRad);
            float fz = cosf(yawRad) * cosf(pitchRad);
            center = sf::Vector3f{eye.x + fx, eye.y + fy, eye.z + fz};
        }

        // lookAt matrix
        auto normalize = [](sf::Vector3f v){
            float l = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
            if (l==0) return v;
            return sf::Vector3f{v.x/l, v.y/l, v.z/l};
        };
        sf::Vector3f f = center - eye; f = normalize(f);
        sf::Vector3f up{0.f,1.f,0.f};
        sf::Vector3f s{f.y*up.z - f.z*up.y, f.z*up.x - f.x*up.z, f.x*up.y - f.y*up.x}; // cross(f,up)
        s = normalize(s);
        sf::Vector3f u{ s.y*f.z - s.z*f.y, s.z*f.x - s.x*f.z, s.x*f.y - s.y*f.x };
        float m[16] = {
            s.x, u.x, -f.x, 0.f,
            s.y, u.y, -f.y, 0.f,
            s.z, u.z, -f.z, 0.f,
            - (s.x*eye.x + s.y*eye.y + s.z*eye.z), - (u.x*eye.x + u.y*eye.y + u.z*eye.z), (f.x*eye.x + f.y*eye.y + f.z*eye.z), 1.f
        };
        glLoadMatrixf(m);

        // Clear
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw Sun (Skybox style - rotation only)
        {
            glPushMatrix();
            // Remove translation from view matrix to make sun infinite
            float viewM[16];
            glGetFloatv(GL_MODELVIEW_MATRIX, viewM);
            viewM[12] = 0.f; viewM[13] = 0.f; viewM[14] = 0.f;
            glLoadMatrixf(viewM);

            glDisable(GL_TEXTURE_2D);
            glDisable(GL_DEPTH_TEST);
            
            // Sun direction: Fixed (e.g. Noon or late afternoon)
            sf::Vector3f sunDir = {0.3f, 0.8f, -0.2f};
            float sl = sqrt(sunDir.x*sunDir.x + sunDir.y*sunDir.y + sunDir.z*sunDir.z);
            sunDir = {sunDir.x/sl, sunDir.y/sl, sunDir.z/sl};
            
            float sunDist = 80.0f; // Push it out further
            float sunSize = 12.0f; // Make it bigger
            sf::Vector3f sunPos = {sunDir.x * sunDist, sunDir.y * sunDist, sunDir.z * sunDist};
            
            glTranslatef(sunPos.x, sunPos.y, sunPos.z);
            
            // Billboard rotation: Face the origin
            sf::Vector3f f = {-sunDir.x, -sunDir.y, -sunDir.z}; // forward
            sf::Vector3f up = {0.f, 1.f, 0.f};
            if (std::abs(f.y) > 0.99f) up = {1.f, 0.f, 0.f}; // Gimbal lock fix
            sf::Vector3f r = {up.y*f.z - up.z*f.y, up.z*f.x - up.x*f.z, up.x*f.y - up.y*f.x}; // right
            float rl = sqrt(r.x*r.x + r.y*r.y + r.z*r.z); r = {r.x/rl, r.y/rl, r.z/rl};
            sf::Vector3f u = {f.y*r.z - f.z*r.y, f.z*r.x - f.x*r.z, f.x*r.y - f.y*r.x}; // up
            
            float lookMat[16] = {
                r.x, r.y, r.z, 0.f,
                u.x, u.y, u.z, 0.f,
                f.x, f.y, f.z, 0.f,
                0.f, 0.f, 0.f, 1.f
            };
            glMultMatrixf(lookMat);

            glColor3f(1.0f, 1.0f, 0.0f);
            
            glBegin(GL_QUADS);
                glVertex3f(-sunSize, -sunSize, 0.f);
                glVertex3f( sunSize, -sunSize, 0.f);
                glVertex3f( sunSize,  sunSize, 0.f);
                glVertex3f(-sunSize,  sunSize, 0.f);
            glEnd();
            
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_TEXTURE_2D);
            glPopMatrix();
        }

        // Render terrain grid of blocks
        glColor3f(1,1,1);
        glPushMatrix();
            const int half = CHUNK/2;



            for(int xi=0; xi<CHUNK; ++xi){
                for(int zi=0; zi<CHUNK; ++zi){
                    int h = getHeightAt(xi, zi);
                    for(int yi=0; yi<h; ++yi){
                        // Determine block type based on depth
                        // Top level: Grass (index 1)
                        // Next 3 levels: Dirt (index 0)
                        // Deeper: Stone (index 2) - check if Stone exists
                        const Block* bPtr = &blocks[0];
                        if (yi == h-1) bPtr = &blocks[1];
                        else if (yi >= h-4) bPtr = &blocks[0];
                        else if (blocks.size() > 2) bPtr = &blocks[2];
                        
                        const Block &b = *bPtr;
                        float gx = static_cast<float>(xi - half);
                        float gz = static_cast<float>(zi - half);
                        float gy = static_cast<float>(yi);

                        int mask = 0;
                        if (yi == h-1) mask |= FACE_TOP;
                        if (yi == 0) mask |= FACE_BOTTOM;
                        if (isAirAt(xi, zi+1, yi)) mask |= FACE_FRONT;
                        if (isAirAt(xi, zi-1, yi)) mask |= FACE_BACK;
                        if (isAirAt(xi+1, zi, yi)) mask |= FACE_RIGHT;
                        if (isAirAt(xi-1, zi, yi)) mask |= FACE_LEFT;

                        if (mask == 0) continue; // block fully surrounded
                        drawBlockAt(gx, gy, gz, b, mask, atlas);
                    }
                }
            }
        glPopMatrix();

        // Draw HUD overlay
        window.pushGLStates();
        // Build status strings
        std::string modeStr = "Orbit";
        if (fpsMode) modeStr = flyMode ? "FPS-Fly" : "FPS-Walk";
        else if (flyMode) modeStr = "Creative-Fly";
        std::string sprintStr = (fpsMode && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) ? "SPRINT" : "";
        if (haveFont){
            // show FPS, mode, sprint and optionally speed
            char buf[256];
            if (showSpeed) snprintf(buf, sizeof(buf), "%d FPS  | %s %s  | speed=%d  | sens=%.2f jump=%.1f grav=%.1f  | invert=%s", static_cast<int>(fps), modeStr.c_str(), sprintStr.c_str(), static_cast<int>(roundf(flySpeed)), mouseLookSpeed, jumpSpeed, gravity, (invertMouse ? "ON" : "OFF"));
            else snprintf(buf, sizeof(buf), "%d FPS  | %s %s  | sens=%.2f jump=%.1f grav=%.1f  | invert=%s", static_cast<int>(fps), modeStr.c_str(), sprintStr.c_str(), mouseLookSpeed, jumpSpeed, gravity, (invertMouse ? "ON" : "OFF"));
            fpsText.setString(buf);
            window.draw(fpsText);
        } else {
            // Draw numeric FPS and mode using built-in bitmap font
            std::string fpsstr = std::to_string(static_cast<int>(fps));
            drawBitmapText(fpsstr, 8, 8, 2);
            std::string m2 = modeStr + (sprintStr.empty() ? std::string() : " ") + sprintStr;
            drawBitmapText(m2, 8, 22, 2);
            if (showSpeed){
                std::string s2 = "spd:" + std::to_string(static_cast<int>(roundf(flySpeed)));
                drawBitmapText(s2, 8, 36, 2);
            }
            // invert status
            std::string invs = std::string("invert:") + (invertMouse ? "ON" : "OFF");
            drawBitmapText(invs, 8, 50, 2);
        }
        window.popGLStates();

        window.display();
    }

    return 0;
}
