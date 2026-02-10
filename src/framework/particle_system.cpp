#include "particle_system.h"
#include <cstdlib> // rand

// Simple constants for the lab
static const int W = 1280;
static const int H = 720;
static const int TOOLBAR = 50;

// Random int in [a, b]
static int RandInt(int a, int b)
{
    return a + (rand() % (b - a + 1));
}

// Create / reset one particle
static void ResetParticle(ParticleSystem::Particle& p)
{
    // Start somewhere on screen (not in toolbar)
    p.position.x = (float)RandInt(0, W - 1);
    p.position.y = (float)RandInt(TOOLBAR + 1, H - 1);

    // Falling speed (snow-like), with small horizontal drift
    float vx = (float)RandInt(-15, 15);
    float vy = (float)RandInt(60, 160);

    p.velocity.x = vx;
    p.velocity.y = -vy;          // goes up (like your v2)
    p.acceleration = 1.0f;       // keep it simple

    p.color = Color::WHITE;

    // Lifetime just to occasionally refresh particles
    p.ttl = (float)RandInt(20, 60) / 10.0f; // 2.0 .. 6.0 seconds
    p.inactive = false;
}

void ParticleSystem::Init()
{
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        ResetParticle(particles[i]);

        // Spread particles so it looks filled from the start
        particles[i].position.y = (float)RandInt(TOOLBAR + 1, H - 1);
    }
}

void ParticleSystem::Update(float dt)
{
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        Particle& p = particles[i];
        if (p.inactive)
            continue;

        // lifetime
        p.ttl -= dt;
        if (p.ttl <= 0.0f)
        {
            ResetParticle(p);
            continue;
        }

        // Move (simple Euler)
        p.position.x += p.velocity.x * dt * p.acceleration;
        p.position.y += p.velocity.y * dt * p.acceleration;

        // Wrap X
        if (p.position.x < 0.0f)      p.position.x = (float)(W - 1);
        else if (p.position.x >= W)   p.position.x = 0.0f;

        // If it reaches the toolbar area, recycle it to the bottom
        if (p.position.y <= (float)TOOLBAR)
        {
            p.position.y = (float)(H - 1);
            p.position.x = (float)RandInt(0, W - 1);
        }
    }
}

void ParticleSystem::Render(Image* framebuffer)
{
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        const Particle& p = particles[i];
        if (p.inactive)
            continue;

        int x = (int)p.position.x;
        int y = (int)p.position.y;

        // do not draw in toolbar
        if (y <= TOOLBAR)
            continue;

        // Use the simplest pixel write your framework supports:
        // If your Image has SetPixelSafeInt, use that. Otherwise use SetPixel.
        framebuffer->SetPixel(x, y, p.color);
    }
}
