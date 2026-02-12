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


void Entity::Render(Image* framebuffer, Camera* camera, FloatImage* zBuffer)
{
    if (!mesh || !framebuffer || !camera)
        return;

    if (mode == eRenderMode::TRIANGLES)
    {
        RenderTriangles(framebuffer, camera, zBuffer);
        return;
    }

    // Wireframe: usamos un color fijo por ahora (Lab 2 toggle W lo manejará igual)
    const Color c = Color::WHITE;

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

        framebuffer->DrawLineDDA((int)x0, (int)y0, (int)x1, (int)y1, c);
        framebuffer->DrawLineDDA((int)x1, (int)y1, (int)x2, (int)y2, c);
        framebuffer->DrawLineDDA((int)x2, (int)y2, (int)x0, (int)y0, c);
    }
}





static inline Vector3 NDCToScreen(const Vector3& ndc, int w, int h)
{
    float x = (ndc.x * 0.5f + 0.5f) * (float)w;
    float y = (1.0f - (ndc.y * 0.5f + 0.5f)) * (float)h;
    return Vector3(x, y, ndc.z);
}

void Entity::RenderTriangles(Image* framebuffer, Camera* camera, FloatImage* zBuffer)
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

        // 3.2: colores por vértice + barycentric interpolation
        // (por ahora generamos 3 colores distintos; luego en 3.4 usaremos UV/texture)
        const std::vector<Vector2>& uvs = mesh->GetUVs();

        Color ca = Color::RED;
        Color cb = Color::GREEN;
        Color cc = Color::BLUE;

        // UVs por vértice (si el modelo no tiene UVs, ponemos 0 para evitar crash)
        Vector2 uv0(0.0f, 0.0f);
        Vector2 uv1(0.0f, 0.0f);
        Vector2 uv2(0.0f, 0.0f);

        if (uvs.size() == vertices.size())
        {
            uv0 = uvs[i];
            uv1 = uvs[i + 1];
            uv2 = uvs[i + 2];
        }
        if (!interpolateUV)
        {
            // modo incorrecto: usar solo UV del primer vértice
            uv1 = uv0;
            uv2 = uv0;
        }

        if (useTexture)
        {
            framebuffer->DrawTriangleInterpolated(a, b, c2, ca, cb, cc, zBuffer, texture, uv0, uv1, uv2);
        }
        else
        {
            framebuffer->DrawTriangleInterpolated(a, b, c2, ca, cb, cc, zBuffer);
        }


        
    }
}

