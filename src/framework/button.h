// LAB 1
#pragma once

#include "image.h"
#include "utils.h"

// toobar buttons 
enum ButtonType
{
    BTN_CLEAR,
    BTN_LOAD,
    BTN_SAVE,

    BTN_ERASER,
    BTN_PENCIL,

    BTN_LINE,
    BTN_RECTANGLE,
    BTN_TRIANGLE,

    BTN_BLACK,
    BTN_WHITE,
    BTN_PINK,
    BTN_YELLOW,
    BTN_RED,
    BTN_BLUE,
    BTN_CYAN,
    BTN_GREEN
};

class Button
{
private:
    Image icon;          // button icon
    Vector2 position;   // top-left position in framebuffer
    ButtonType type;    // button function

public:
    Button() = default;

    Button(const char* filename, const Vector2& pos, ButtonType t)
        : position(pos), type(t)
    {
        icon.LoadPNG(filename, true);
    }

    void Render(Image& framebuffer) const
    {
        framebuffer.DrawImage(icon, (int)position.x, (int)position.y);
    }

    bool IsMouseInside(const Vector2& mouse) const
    {
        return mouse.x >= position.x &&
               mouse.x < position.x + icon.width &&
               mouse.y >= position.y &&
               mouse.y < position.y + icon.height;
    }

    ButtonType GetType() const { return type; }
    const Image& GetImage() const { return icon; }
    Vector2 GetPosition() const { return position; }
};
