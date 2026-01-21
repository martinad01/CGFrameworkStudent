#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <climits>

#include "GL/glew.h"
#include "../extra/picopng.h"
#include "image.h"
#include "utils.h"
#include "camera.h"
#include "mesh.h"

struct Cell
{
    int minX;
    int maxX;

    Cell()
    {
        minX = INT_MAX;
        maxX = INT_MIN;
    }
};

Image::Image()
{
    width = 0; height = 0;
    pixels = NULL;
}

Image::Image(unsigned int width, unsigned int height)
{
    this->width = width;
    this->height = height;
    pixels = new Color[width * height];
    memset(pixels, 0, width * height * sizeof(Color));
}

Image::Image(const Image& c)
{
    pixels = NULL;
    width = c.width;
    height = c.height;
    bytes_per_pixel = c.bytes_per_pixel;
    if (c.pixels)
    {
        pixels = new Color[width * height];
        memcpy(pixels, c.pixels, width * height * bytes_per_pixel);
    }
}

Image& Image::operator=(const Image& c)
{
    if (pixels)
        delete[] pixels;

    pixels = NULL;
    width = c.width;
    height = c.height;
    bytes_per_pixel = c.bytes_per_pixel;

    if (c.pixels)
    {
        pixels = new Color[width * height];
        memcpy(pixels, c.pixels, width * height * bytes_per_pixel);
    }
    return *this;
}

Image::~Image()
{
    if (pixels)
        delete[] pixels;
}

void Image::Render()
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glDrawPixels(width, height,
                 bytes_per_pixel == 3 ? GL_RGB : GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);
}

void Image::Resize(unsigned int width, unsigned int height)
{
    Color* new_pixels = new Color[width * height];
    unsigned int min_width = this->width > width ? width : this->width;
    unsigned int min_height = this->height > height ? height : this->height;

    for (unsigned int x = 0; x < min_width; ++x)
        for (unsigned int y = 0; y < min_height; ++y)
            new_pixels[y * width + x] = GetPixel(x, y);

    delete[] pixels;
    this->width = width;
    this->height = height;
    pixels = new_pixels;
}

void Image::Scale(unsigned int width, unsigned int height)
{
    Color* new_pixels = new Color[width * height];

    for (unsigned int x = 0; x < width; ++x)
        for (unsigned int y = 0; y < height; ++y)
            new_pixels[y * width + x] =
                GetPixel((unsigned int)(this->width * (x / (float)width)),
                         (unsigned int)(this->height * (y / (float)height)));

    delete[] pixels;
    this->width = width;
    this->height = height;
    pixels = new_pixels;
}

// (MARTINA) 2.1.1: drawing lines
void Image::DrawLineDDA(int x0, int y0, int x1, int y1, const Color& c)
{
    //given two pts, we define a tangent vector between them using line equation derivation.
    int dx = x1 - x0;
    int dy = y1 - y0;

    int d = std::max(std::abs(dx), std::abs(dy));
    
    float x = (float)x0;
    float y = (float)y0;

    float sx = dx / (float)d;
    float sy = dy / (float)d;

    //d  steps, painting in each one
    for (int i = 0; i <= d; i++)
    {
        SetPixel((int)x, (int)y, c);
        x += sx;
        y += sy;
    }
}

// (MARTINA) 2.1.2: drawing rectangle
void Image::DrawRect(int x, int y, int w, int h, const Color& borderColor,
int borderWidth, bool isFilled, const Color& fillColor)
{
    //if its a filled rectangle we paint each (x_j,y_i) point from (x,y) to (x+w,y+h)
    if (isFilled)
    {
        for (int i = y ; i <= h; i++)
            for (int j = x; x <= w; x++)
                SetPixel(x, y, fillColor);
    }

    //drawing the borders
    DrawLineDDA(x, y, x + w, y, borderColor);
    DrawLineDDA(x, y, x, y + h, borderColor);
    DrawLineDDA(x + w, y, x + w, y + h, borderColor);
    DrawLineDDA(x, y + h, x + w, y + h, borderColor);
}


//updated DDA algorithm using yable as reference
static void ScanLineDDA(const Vector2& a,
                        const Vector2& b,
                        std::vector<Cell>& table)
{
    float x0 = a.x;
    float y0 = a.y;
    float x1 = b.x;
    float y1 = b.y;

    float dx = x1 - x0;
    float dy = y1 - y0;

    int d = (int)std::ceil(std::max(std::abs(dx), std::abs(dy)));
    if (d == 0) return;

    float sx = dx / (float)d;
    float sy = dy / (float)d;

    float x = x0;
    float y = y0;

    for (int i = 0; i <= d; i++)
    {
        int yi = (int)std::floor(y);
        if (yi >= 0 && yi < (int)table.size())
        {
            int xi = (int)std::floor(x);
            if (xi < table[yi].minX) table[yi].minX = xi;
            if (xi > table[yi].maxX) table[yi].maxX = xi;
        }
        x += sx;
        y += sy;
    }
}



void Image::DrawTriangle(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Color& borderColor, bool isFilled, const Color& fillColor)
{
    DrawLineDDA((int)p0.x, (int)p0.y, (int)p1.x, (int)p1.y, borderColor);
    DrawLineDDA((int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, borderColor);
    DrawLineDDA((int)p2.x, (int)p2.y, (int)p0.x, (int)p0.y, borderColor);


    if (!isFilled)
        return;

    std::vector<Cell> table(height);

    ScanLineDDA(p0, p1, table);
    ScanLineDDA(p1, p2, table);
    ScanLineDDA(p2, p0, table);

    for (int y = 0; y < height; y++)
        for (int x = table[y].minX; x <= table[y].maxX; x++)
            SetPixel(x, y, fillColor);
}
