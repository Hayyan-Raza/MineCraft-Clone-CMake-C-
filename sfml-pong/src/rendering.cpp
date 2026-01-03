#include "rendering.h"
#include <GL/gl.h>

const int FACE_TOP = 1;
const int FACE_BOTTOM = 2;
const int FACE_FRONT = 4;
const int FACE_BACK = 8;
const int FACE_LEFT = 16;
const int FACE_RIGHT = 32;

static void drawFaceWithVerts(const std::array<float,4> &uv, std::array<sf::Vector3f,4> verts){
    glTexCoord2f(uv[0], uv[1]); glVertex3f(verts[0].x, verts[0].y, verts[0].z);
    glTexCoord2f(uv[2], uv[1]); glVertex3f(verts[1].x, verts[1].y, verts[1].z);
    glTexCoord2f(uv[2], uv[3]); glVertex3f(verts[2].x, verts[2].y, verts[2].z);
    glTexCoord2f(uv[0], uv[3]); glVertex3f(verts[3].x, verts[3].y, verts[3].z);
}

void drawBlockAt(float gx, float gy, float gz, const Block &b, int faceMask, const TextureAtlas &atlas){
    if (faceMask == 0) return;
    const float s = 0.5f;
    auto sideUV = atlas.getUV_fromAtlasTile(b.side);
    auto topUV = atlas.getUV_fromAtlasTile(b.top);
    auto botUV = atlas.getUV_fromAtlasTile(b.bottom);

    auto drawFace = [&](const std::array<float,4> &uv, std::array<sf::Vector3f,4> verts){
        drawFaceWithVerts(uv, verts);
    };

    glPushMatrix();
    glTranslatef(gx, gy + s, gz);

    if (faceMask & FACE_FRONT){ atlas.bindSide(); glBegin(GL_QUADS); drawFace(sideUV, { sf::Vector3f{-s,-s, s}, sf::Vector3f{ s,-s, s}, sf::Vector3f{ s, s, s}, sf::Vector3f{-s, s, s} }); glEnd(); }
    if (faceMask & FACE_BACK){ atlas.bindSide(); auto backUV = sideUV; std::swap(backUV[0], backUV[2]); glBegin(GL_QUADS); drawFace(backUV, { sf::Vector3f{-s,-s,-s}, sf::Vector3f{-s, s,-s}, sf::Vector3f{ s, s,-s}, sf::Vector3f{ s,-s,-s} }); glEnd(); }
    if (faceMask & FACE_LEFT){ atlas.bindSide(); auto leftUV = sideUV; std::swap(leftUV[0], leftUV[2]); glBegin(GL_QUADS); drawFace(leftUV, { sf::Vector3f{-s,-s,-s}, sf::Vector3f{-s,-s, s}, sf::Vector3f{-s, s, s}, sf::Vector3f{-s, s,-s} }); glEnd(); }
    if (faceMask & FACE_RIGHT){ atlas.bindSide(); glBegin(GL_QUADS); drawFace(sideUV, { sf::Vector3f{ s,-s,-s}, sf::Vector3f{ s, s,-s}, sf::Vector3f{ s, s, s}, sf::Vector3f{ s,-s, s} }); glEnd(); }

    if (faceMask & FACE_TOP){ atlas.bindTop(); glBegin(GL_QUADS); drawFace(topUV, { sf::Vector3f{-s, s,-s}, sf::Vector3f{-s, s, s}, sf::Vector3f{ s, s, s}, sf::Vector3f{ s, s,-s} }); glEnd(); }
    if (faceMask & FACE_BOTTOM){ atlas.bindDirt(); glBegin(GL_QUADS); drawFace(botUV, { sf::Vector3f{-s,-s,-s}, sf::Vector3f{ s,-s,-s}, sf::Vector3f{ s,-s, s}, sf::Vector3f{-s,-s, s} }); glEnd(); }

    sf::Texture::bind(nullptr);
    glPopMatrix();
}
