//
//  entity.h
//  ComputerGraphics
//
//  Created by Martina Dulanto Ostornol on 29/1/26.
//

#ifndef entity_h
#define entity_h
#pragma once
#include "mesh.h"

#include "image.h"
#include "camera.h"

class Entity {
public:
    Mesh* mesh;
    Matrix44 model;
    Image* texture = nullptr;


    Entity();
    void Update(float seconds_elapsed);
    
    void Render(Image* framebuffer, Camera* camera, FloatImage* zBuffer);
    float animPhase = 0.0f;
    Vector3 basePos = Vector3(0,0,0);
    Vector3 baseScale = Vector3(1,1,1);
    enum class eRenderMode { WIREFRAME, TRIANGLES };
    eRenderMode mode = eRenderMode::WIREFRAME;
    void RenderTriangles(Image* framebuffer, Camera* camera, FloatImage* zBuffer);
    bool useTexture = true;
    bool interpolateUV = true;




};

#endif /* entity_h */
