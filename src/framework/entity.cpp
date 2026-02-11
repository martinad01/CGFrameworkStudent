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


void Entity::RenderWireframe(Image* framebuffer, Camera* camera)
{
    if (!mesh || !framebuffer || !camera)
        return;

    // 1) vertices (ya vienen triangulados: cada 3 = 1 triángulo)
    const std::vector<Vector3>& vertices = mesh->GetVertices();

    // 2) por cada triángulo
    for (size_t i = 0; i + 2 < vertices.size(); i += 3)
    {
        Vector3 va = vertices[i];
        Vector3 vb = vertices[i + 1];
        Vector3 vc = vertices[i + 2];

        // 3) local -> world
        // OJO: aquí puede fallar según cómo se multiplica Matrix44 en tu framework.
        Vector3 wa = model * va;
        Vector3 wb = model * vb;
        Vector3 wc = model * vc;

        // 4) world -> NDC (tu camera versión 2 devuelve ya NDC dividiendo por w)
        Vector3 na = camera->ProjectVector(wa);
        Vector3 nb = camera->ProjectVector(wb);
        Vector3 nc = camera->ProjectVector(wc);

        // c triangles completely outside the clip cube [-1,1]^3
        if (!InsideClipCube(na) || !InsideClipCube(nb) || !InsideClipCube(nc))
            continue;

        // 5) NDC -> Screen
        float x0 = (na.x * 0.5f + 0.5f) * framebuffer->width;
        float y0 = (1.0f - (na.y * 0.5f + 0.5f)) * framebuffer->height;

        float x1 = (nb.x * 0.5f + 0.5f) * framebuffer->width;
        float y1 = (1.0f - (nb.y * 0.5f + 0.5f)) * framebuffer->height;

        float x2 = (nc.x * 0.5f + 0.5f) * framebuffer->width;
        float y2 = (1.0f - (nc.y * 0.5f + 0.5f)) * framebuffer->height;

        // 6) dibujar aristas
        framebuffer->DrawLineDDA(x0, y0, x1, y1, Color::WHITE);
        framebuffer->DrawLineDDA(x1, y1, x2, y2, Color::WHITE);
        framebuffer->DrawLineDDA(x2, y2, x0, y0, Color::WHITE);
    }
}

