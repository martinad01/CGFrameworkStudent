// LAB 1
#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "framework.h"
#include "image.h"

class ParticleSystem {
public:
    static const int MAX_PARTICLES = 100;

    struct Particle {
        Vector2 position;
        Vector2 velocity; 
        Color color;
        float acceleration;
        float ttl = 0.0f;
        bool inactive = true;
    };

    Particle particles[MAX_PARTICLES];


    void Init();
    void Render(Image* framebuffer);
    void Update(float dt);

};
