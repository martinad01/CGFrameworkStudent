//
//  entity.cpp
//  ComputerGraphics
//
//  Created by Martina Dulanto Ostornol on 29/1/26.
//


#include "entity.h"
#include "image.h"
#include "camera.h"
#include "mesh.h"

Entity::Entity()
{
    mesh = nullptr;
    model.SetIdentity();
}

void Entity::Update(float seconds_elapsed)
{
    // modelo = T * R * S (para column-vectors M*p: el orden de multiplicación importa)
    Matrix44 T, R, S;

    // animación simple
    float t = seconds_elapsed + animPhase;

    T.MakeTranslationMatrix(basePos.x, basePos.y, basePos.z);
    R.MakeRotationMatrix(t * 0.7f, Vector3(0,1,0));
    S.MakeScaleMatrix(baseScale.x, baseScale.y, baseScale.z);

    // composición: primero escala, luego rota, luego traslada
    model = T * R * S;
}


static bool InsideClipCube(const Vector3& p)
{
    return (p.x >= -1.0f && p.x <= 1.0f &&
            p.y >= -1.0f && p.y <= 1.0f &&
            p.z >= -1.0f && p.z <= 1.0f);
}


void Entity::Render(Image* framebuffer, Camera* camera, const Color& c)
{
    if (!mesh || !framebuffer || !camera)
        return;

    const std::vector<Vector3>& vertices = mesh->GetVertices();

    for (size_t i = 0; i + 2 < vertices.size(); i += 3)
    {
        Vector3 wa = model * vertices[i];
        Vector3 wb = model * vertices[i + 1];
        Vector3 wc = model * vertices[i + 2];

        Vector3 na = camera->ProjectVector(wa);
        Vector3 nb = camera->ProjectVector(wb);
        Vector3 nc = camera->ProjectVector(wc);

        if (!InsideClipCube(na) || !InsideClipCube(nb) || !InsideClipCube(nc))
            continue;

        float x0 = (na.x * 0.5f + 0.5f) * framebuffer->width;
        float y0 = (1.0f - (na.y * 0.5f + 0.5f)) * framebuffer->height;

        float x1 = (nb.x * 0.5f + 0.5f) * framebuffer->width;
        float y1 = (1.0f - (nb.y * 0.5f + 0.5f)) * framebuffer->height;

        float x2 = (nc.x * 0.5f + 0.5f) * framebuffer->width;
        float y2 = (1.0f - (nc.y * 0.5f + 0.5f)) * framebuffer->height;

        framebuffer->DrawLineDDA(x0, y0, x1, y1, c);
        framebuffer->DrawLineDDA(x1, y1, x2, y2, c);
        framebuffer->DrawLineDDA(x2, y2, x0, y0, c);
    }
}



static inline Vector3 NDCToScreen(const Vector3& ndc, int w, int h)
{
    float x = (ndc.x * 0.5f + 0.5f) * (float)w;
    float y = (1.0f - (ndc.y * 0.5f + 0.5f)) * (float)h;
    return Vector3(x, y, ndc.z);
}

void Entity::RenderTriangles(Image* framebuffer, Camera* camera, const Color& c)
{
    if (!mesh || !framebuffer || !camera)
        return;

    const std::vector<Vector3>& vertices = mesh->GetVertices();

    for (size_t i = 0; i + 2 < vertices.size(); i += 3)
    {
        // Local -> World
        Vector3 wa = model * vertices[i];
        Vector3 wb = model * vertices[i + 1];
        Vector3 wc = model * vertices[i + 2];

        // World -> NDC
        Vector3 na = camera->ProjectVector(wa);
        Vector3 nb = camera->ProjectVector(wb);
        Vector3 nc = camera->ProjectVector(wc);

        // Clip cube culling (simple)
        if (!InsideClipCube(na) || !InsideClipCube(nb) || !InsideClipCube(nc))
            continue;

        // NDC -> Screen (keep z in .z)
        Vector3 a = NDCToScreen(na, framebuffer->width, framebuffer->height);
        Vector3 b = NDCToScreen(nb, framebuffer->width, framebuffer->height);
        Vector3 c2 = NDCToScreen(nc, framebuffer->width, framebuffer->height);

        // Filled triangle (NECESITAMOS esta función en Image)
        Vector2 p0(a.x, a.y);
        Vector2 p1(b.x, b.y);
        Vector2 p2(c2.x, c2.y);

        framebuffer->DrawTriangle(
            p0,
            p1,
            p2,
            c,        // border color
            true,     // filled
            c         // fill color
        );
    }
}

