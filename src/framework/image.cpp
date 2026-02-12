#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
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

Image::Image() {
    width = 0; height = 0;
    pixels = NULL;
}

Image::Image(unsigned int width, unsigned int height)
{
    this->width = width;
    this->height = height;
    pixels = new Color[width*height];
    memset(pixels, 0, width * height * sizeof(Color));
}

// Copy constructor
Image::Image(const Image& c)
{
    pixels = NULL;
    width = c.width;
    height = c.height;
    bytes_per_pixel = c.bytes_per_pixel;
    if(c.pixels)
    {
        pixels = new Color[width*height];
        memcpy(pixels, c.pixels, width*height*bytes_per_pixel);
    }
}

// Assign operator
Image& Image::operator = (const Image& c)
{
    if(pixels) delete[] pixels;
    pixels = NULL;

    width = c.width;
    height = c.height;
    bytes_per_pixel = c.bytes_per_pixel;

    if(c.pixels)
    {
        pixels = new Color[width*height*bytes_per_pixel];
        memcpy(pixels, c.pixels, width*height*bytes_per_pixel);
    }
    return *this;
}

Image::~Image()
{
    if(pixels)
        delete[] pixels;
}

void Image::Render()
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glDrawPixels(width, height, bytes_per_pixel == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

// Change image size (the old one will remain in the top-left corner)
void Image::Resize(unsigned int width, unsigned int height)
{
    Color* new_pixels = new Color[width*height];
    unsigned int min_width = this->width > width ? width : this->width;
    unsigned int min_height = this->height > height ? height : this->height;

    for(unsigned int x = 0; x < min_width; ++x)
        for(unsigned int y = 0; y < min_height; ++y)
            new_pixels[ y * width + x ] = GetPixel(x,y);

    delete[] pixels;
    this->width = width;
    this->height = height;
    pixels = new_pixels;
}

// Change image size and scale the content
void Image::Scale(unsigned int width, unsigned int height)
{
    Color* new_pixels = new Color[width*height];

    for(unsigned int x = 0; x < width; ++x)
        for(unsigned int y = 0; y < height; ++y)
            new_pixels[ y * width + x ] = GetPixel((unsigned int)(this->width * (x / (float)width)), (unsigned int)(this->height * (y / (float)height)) );

    delete[] pixels;
    this->width = width;
    this->height = height;
    pixels = new_pixels;
}

Image Image::GetArea(unsigned int start_x, unsigned int start_y, unsigned int width, unsigned int height)
{
    Image result(width, height);
    for(unsigned int x = 0; x < width; ++x)
        for(unsigned int y = 0; y < height; ++y)
        {
            if( (x + start_x) < this->width && (y + start_y) < this->height)
                result.SetPixelUnsafe( x, y, GetPixel(x + start_x,y + start_y) );
        }
    return result;
}

void Image::FlipY()
{
    int row_size = bytes_per_pixel * width;
    Uint8* temp_row = new Uint8[row_size];
#pragma omp simd
    for (int y = 0; y < height * 0.5; y += 1)
    {
        Uint8* pos = (Uint8*)pixels + y * row_size;
        memcpy(temp_row, pos, row_size);
        Uint8* pos2 = (Uint8*)pixels + (height - y - 1) * row_size;
        memcpy(pos, pos2, row_size);
        memcpy(pos2, temp_row, row_size);
    }
    delete[] temp_row;
}

bool Image::LoadPNG(const char* filename, bool flip_y)
{
    std::string sfullPath = absResPath(filename);
    std::ifstream file(sfullPath, std::ios::in | std::ios::binary | std::ios::ate);

    // Get filesize
    std::streamsize size = 0;
    if (file.seekg(0, std::ios::end).good()) size = file.tellg();
    if (file.seekg(0, std::ios::beg).good()) size -= file.tellg();

    if (!size){
        std::cerr << "--- Failed to load file: " << sfullPath.c_str() << std::endl;
        return false;
    }

    std::vector<unsigned char> buffer;

    // Read contents of the file into the vector
    if (size > 0)
    {
        buffer.resize((size_t)size);
        file.read((char*)(&buffer[0]), size);
    }
    else
        buffer.clear();

    std::vector<unsigned char> out_image;

    if (decodePNG(out_image, width, height, buffer.empty() ? 0 : &buffer[0], (unsigned long)buffer.size(), true) != 0){
        std::cerr << "--- Failed to load file: " << sfullPath.c_str() << std::endl;
        return false;
    }

    size_t bufferSize = out_image.size();
    unsigned int originalBytesPerPixel = (unsigned int)bufferSize / (width * height);
    
    // Force 3 channels
    bytes_per_pixel = 3;

    if (originalBytesPerPixel == 3) {
        if (pixels) delete[] pixels;
        pixels = new Color[bufferSize];
        memcpy(pixels, &out_image[0], bufferSize);
    }
    else if (originalBytesPerPixel == 4) {
        if (pixels) delete[] pixels;

        unsigned int newBufferSize = width * height * bytes_per_pixel;
        pixels = new Color[newBufferSize];

        unsigned int k = 0;
        for (unsigned int i = 0; i < bufferSize; i += originalBytesPerPixel) {
            pixels[k] = Color(out_image[i], out_image[i + 1], out_image[i + 2]);
            k++;
        }
    }

    // Flip pixels in Y
    if (flip_y)
        FlipY();

    std::cout << "+++ File loaded: " << sfullPath.c_str() << std::endl;

    return true;
}

// Loads an image from a TGA file
bool Image::LoadTGA(const char* filename, bool flip_y)
{
    unsigned char TGAheader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned char TGAcompare[12];
    unsigned char header[6];
    unsigned int imageSize;
    unsigned int bytesPerPixel;

    std::string sfullPath = absResPath( filename );

    FILE * file = fopen( sfullPath.c_str(), "rb");
       if ( file == NULL || fread(TGAcompare, 1, sizeof(TGAcompare), file) != sizeof(TGAcompare) ||
        memcmp(TGAheader, TGAcompare, sizeof(TGAheader)) != 0 ||
        fread(header, 1, sizeof(header), file) != sizeof(header))
    {
        std::cerr << "--- File not found: " << sfullPath.c_str() << std::endl;
        if (file == NULL)
            return NULL;
        else
        {
            fclose(file);
            return NULL;
        }
    }

    TGAInfo* tgainfo = new TGAInfo;
    
    tgainfo->width = header[1] * 256 + header[0];
    tgainfo->height = header[3] * 256 + header[2];
    
    if (tgainfo->width <= 0 || tgainfo->height <= 0 || (header[4] != 24 && header[4] != 32))
    {
        std::cerr << "--- Failed to load file: " << sfullPath.c_str() << std::endl;
        fclose(file);
        delete tgainfo;
        return NULL;
    }
    
    tgainfo->bpp = header[4];
    bytesPerPixel = tgainfo->bpp / 8;
    imageSize = tgainfo->width * tgainfo->height * bytesPerPixel;
    
    tgainfo->data = new unsigned char[imageSize];
    
    if (tgainfo->data == NULL || fread(tgainfo->data, 1, imageSize, file) != imageSize)
    {
        std::cerr << "--- Failed to load file: " << sfullPath.c_str() << std::endl;

        if (tgainfo->data != NULL)
            delete[] tgainfo->data;
            
        fclose(file);
        delete tgainfo;
        return false;
    }

    fclose(file);

    // Save info in image
    if(pixels)
        delete[] pixels;

    width = tgainfo->width;
    height = tgainfo->height;
    pixels = new Color[width*height];

    const char imageDescriptor = header[5];
    bool tgaFlipY = (imageDescriptor & 0x20) > 0; // bit 5 (0-7) -> true == origin on top
    bool tgaFlipX = (imageDescriptor & 0x10) > 0; // bit 4 (0-7) -> true == origin on right

    if (flip_y) {
        tgaFlipY = !tgaFlipY;
    }

    // Convert to float all pixels
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            unsigned int offsetY = (tgaFlipY ? (height - 1 - y) : y) * width * bytesPerPixel;
            unsigned int offsetX = (tgaFlipX ? (width - 1 - x) : x) * bytesPerPixel;
            unsigned int pos = offsetY + offsetX;
            // Make sure we don't access out of memory
            if( pos + 2 < imageSize ) // assuming 1 bytes per channel
                SetPixelUnsafe(x, y, Color(tgainfo->data[pos + 2], tgainfo->data[pos + 1], tgainfo->data[pos]));
        }
    }

    delete[] tgainfo->data;
    delete tgainfo;

    std::cout << "+++ File loaded: " << sfullPath.c_str() << std::endl;

    return true;
}

// Saves the image to a TGA file
bool Image::SaveTGA(const char* filename)
{
    unsigned char TGAheader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    std::string fullPath = absResPath(filename);
    FILE *file = fopen(fullPath.c_str(), "wb");
    if ( file == NULL )
    {
        std::cerr << "--- Failed to save file: " << fullPath.c_str() << std::endl;
        return false;
    }

    unsigned short header_short[3];
    header_short[0] = width;
    header_short[1] = height;
    unsigned char* header = (unsigned char*)header_short;
    header[4] = 24;
    header[5] = 0; // image descriptor: origin in bottom-left

    fwrite(TGAheader, 1, sizeof(TGAheader), file);
    fwrite(header, 1, 6, file);

    // Convert pixels to unsigned char
    unsigned char* bytes = new unsigned char[width*height*3];
    for(unsigned int y = 0; y < height; ++y)
        for(unsigned int x = 0; x < width; ++x)
        {
            Color c = pixels[y*width+x];
            unsigned int pos = (y*width+x)*3;
            bytes[pos+2] = c.r;
            bytes[pos+1] = c.g;
            bytes[pos] = c.b;
        }

    fwrite(bytes, 1, width*height*3, file);
    fclose(file);

    delete[] bytes;

    std::cout << "+++ File saved: " << fullPath.c_str() << std::endl;

    return true;
}

//-----LAB 1-----
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
void Image::DrawRect(int x, int y, int w, int h, const Color& borderColor,int borderWidth,bool isFilled, const Color& fillColor)
{
    //if its a filled rectangle we paint each (x_j,y_i) point from (x,y) to (x+w,y+h)
    if (isFilled)
    {
        for (int i = y; i <= y + h; i++)
            for (int j = x; j <= x + w; j++)
                SetPixel(j, i, fillColor);
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


//(MARTINA): drawing triangle using updated DDA
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

// i was running with a LOT of compilation errors in my buttons.h file so i decided to create this function to make it a bt more clean!!
void Image::DrawImage(const Image& image, int x, int y) {
    for (int i = 0; i < image.width; ++i) {
        for (int j = 0; j < image.height; ++j) {
            int px = x + i;
            int py = y + j;
            if (px >= 0 && px < width && py >= 0 && py < height) {
                Color c = image.GetPixel(i, j);
                SetPixel(px, py, c);
            }
        }
    }
}
//------------------
#ifndef IGNORE_LAMBDAS

// You can apply and algorithm for two images and store the result in the first one
// ForEachPixel( img, img2, [](Color a, Color b) { return a + b; } );
template <typename F>
void ForEachPixel(Image& img, const Image& img2, F f) {
    for(unsigned int pos = 0; pos < img.width * img.height; ++pos)
        img.pixels[pos] = f( img.pixels[pos], img2.pixels[pos] );
}

#endif

FloatImage::FloatImage(unsigned int width, unsigned int height)
{
    this->width = width;
    this->height = height;
    pixels = new float[width * height];
    memset(pixels, 0, width * height * sizeof(float));
}

// Copy constructor
FloatImage::FloatImage(const FloatImage& c) {
    pixels = NULL;

    width = c.width;
    height = c.height;
    if (c.pixels)
    {
        pixels = new float[width * height];
        memcpy(pixels, c.pixels, width * height * sizeof(float));
    }
}

// Assign operator
FloatImage& FloatImage::operator = (const FloatImage& c)
{
    if (pixels) delete[] pixels;
    pixels = NULL;

    width = c.width;
    height = c.height;
    if (c.pixels)
    {
        pixels = new float[width * height * sizeof(float)];
        memcpy(pixels, c.pixels, width * height * sizeof(float));
    }
    return *this;
}

FloatImage::~FloatImage()
{
    if (pixels)
        delete[] pixels;
}

// Change image size (the old one will remain in the top-left corner)
void FloatImage::Resize(unsigned int width, unsigned int height)
{
    float* new_pixels = new float[width * height];
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


void Image::DrawTriangleInterpolated(const Vector3& p0, const Vector3& p1, const Vector3& p2,
                                     const Color& c0, const Color& c1, const Color& c2)
{
    float minXf = std::min(p0.x, std::min(p1.x, p2.x));
    float maxXf = std::max(p0.x, std::max(p1.x, p2.x));
    float minYf = std::min(p0.y, std::min(p1.y, p2.y));
    float maxYf = std::max(p0.y, std::max(p1.y, p2.y));

    int minX = (int)std::floor(minXf);
    int maxX = (int)std::ceil(maxXf);
    int minY = (int)std::floor(minYf);
    int maxY = (int)std::ceil(maxYf);

    if (minX < 0) minX = 0;
    if (minY < 0) minY = 0;
    if (maxX >= (int)width)  maxX = width - 1;
    if (maxY >= (int)height) maxY = height - 1;

    float denom = ((p1.y - p2.y) * (p0.x - p2.x) +
                   (p2.x - p1.x) * (p0.y - p2.y));

    if (fabs(denom) < 1e-8f)
        return;

    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            float px = x + 0.5f;
            float py = y + 0.5f;

            float w0 = ((p1.y - p2.y) * (px - p2.x) +
                        (p2.x - p1.x) * (py - p2.y)) / denom;

            float w1 = ((p2.y - p0.y) * (px - p2.x) +
                        (p0.x - p2.x) * (py - p2.y)) / denom;

            float w2 = 1.0f - w0 - w1;

            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
                continue;

            Color col = c0 * w0 + c1 * w1 + c2 * w2;
            SetPixel(x, y, col);
        }
    }
}



void Image::DrawTriangleInterpolated(const Vector3& p0, const Vector3& p1, const Vector3& p2,
                                     const Color& c0, const Color& c1, const Color& c2,
                                     FloatImage* zbuffer)
{
    if (!zbuffer)
    {
        DrawTriangleInterpolated(p0, p1, p2, c0, c1, c2);
        return;
    }

    float minXf = std::min(p0.x, std::min(p1.x, p2.x));
    float maxXf = std::max(p0.x, std::max(p1.x, p2.x));
    float minYf = std::min(p0.y, std::min(p1.y, p2.y));
    float maxYf = std::max(p0.y, std::max(p1.y, p2.y));

    int minX = (int)std::floor(minXf);
    int maxX = (int)std::ceil(maxXf);
    int minY = (int)std::floor(minYf);
    int maxY = (int)std::ceil(maxYf);

    if (minX < 0) minX = 0;
    if (minY < 0) minY = 0;
    if (maxX >= (int)width)  maxX = width - 1;
    if (maxY >= (int)height) maxY = height - 1;

    float denom = ((p1.y - p2.y) * (p0.x - p2.x) +
                   (p2.x - p1.x) * (p0.y - p2.y));

    if (fabs(denom) < 1e-8f)
        return;

    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            float px = x + 0.5f;
            float py = y + 0.5f;

            float w0 = ((p1.y - p2.y) * (px - p2.x) +
                        (p2.x - p1.x) * (py - p2.y)) / denom;

            float w1 = ((p2.y - p0.y) * (px - p2.x) +
                        (p0.x - p2.x) * (py - p2.y)) / denom;

            float w2 = 1.0f - w0 - w1;

            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
                continue;

            float z = p0.z * w0 + p1.z * w1 + p2.z * w2;
            float& zprev = zbuffer->GetPixelRef(x, y);

            if (z >= zprev)
                continue;

            zprev = z;

            Color col = c0 * w0 + c1 * w1 + c2 * w2;
            SetPixel(x, y, col);
        }
    }
}

void Image::DrawTriangleInterpolated(const Vector3& p0, const Vector3& p1, const Vector3& p2,
                                     const Color& c0, const Color& c1, const Color& c2,
                                     FloatImage* zbuffer,
                                     Image* texture,
                                     const Vector2& uv0, const Vector2& uv1, const Vector2& uv2)
{
    // Seguridad mínima
    if (!zbuffer)
    {
        DrawTriangleInterpolated(p0, p1, p2, c0, c1, c2);
        return;
    }

    if (!texture || texture->width == 0 || texture->height == 0)
    {
        // DEBUG: si esto aparece, NO se está usando textura
        DrawTriangleInterpolated(p0, p1, p2, Color::BLUE, Color::BLUE, Color::BLUE, zbuffer);
        return;
    }


    // Bounding box
    float minXf = std::min(p0.x, std::min(p1.x, p2.x));
    float maxXf = std::max(p0.x, std::max(p1.x, p2.x));
    float minYf = std::min(p0.y, std::min(p1.y, p2.y));
    float maxYf = std::max(p0.y, std::max(p1.y, p2.y));

    int minX = (int)std::floor(minXf);
    int maxX = (int)std::ceil(maxXf);
    int minY = (int)std::floor(minYf);
    int maxY = (int)std::ceil(maxYf);

    if (minX < 0) minX = 0;
    if (minY < 0) minY = 0;
    if (maxX >= (int)width)  maxX = width - 1;
    if (maxY >= (int)height) maxY = height - 1;

    // Barycentric denominator
    float denom = ((p1.y - p2.y) * (p0.x - p2.x) +
                   (p2.x - p1.x) * (p0.y - p2.y));

    if (fabs(denom) < 1e-8f)
        return;

    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            float px = x + 0.5f;
            float py = y + 0.5f;

            float w0 = ((p1.y - p2.y) * (px - p2.x) +
                        (p2.x - p1.x) * (py - p2.y)) / denom;

            float w1 = ((p2.y - p0.y) * (px - p2.x) +
                        (p0.x - p2.x) * (py - p2.y)) / denom;

            float w2 = 1.0f - w0 - w1;

            // Fuera del triángulo
            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
                continue;

            // Z interpolation
            float z = p0.z * w0 + p1.z * w1 + p2.z * w2;
            float& zprev = zbuffer->GetPixelRef(x, y);

            if (z >= zprev)
                continue;

            // ✅ Interpolar UV (NO colores)
            Vector2 uv = uv0 * w0 + uv1 * w1 + uv2 * w2;

            float u = uv.x;
            float v = uv.y;
            v = 1.0f - v;


            // Clamp UV
            if (u < 0.0f) u = 0.0f;
            if (u > 1.0f) u = 1.0f;
            if (v < 0.0f) v = 0.0f;
            if (v > 1.0f) v = 1.0f;

            // Convertir a espacio textura
            int tx = (int)(u * (texture->width  - 1));
            int ty = (int)(v * (texture->height - 1));

            // Sample
            Color texColor = texture->GetPixelSafe(tx, ty);

            // Commit pixel
            zprev = z;
            SetPixel(x, y, texColor);
        }
    }
}

