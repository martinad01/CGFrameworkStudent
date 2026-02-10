#include "application.h"
#include "mesh.h"
#include "shader.h"
#include "utils.h"

Application::Application(const char* caption, int width, int height)
{
    this->window = createWindow(caption, width, height);

    int w,h;
    SDL_GetWindowSize(window,&w,&h);

    this->mouse_state = 0;
    this->time = 0.f;
    this->window_width = w;
    this->window_height = h;
    this->keystate = SDL_GetKeyboardState(nullptr);

    this->framebuffer.Resize(w, h);
}

Application::~Application()
{
}
//----LAB1-----------
void Application::Init(void)
{
    std::cout << "Initiating app..." << std::endl;

    //(MATEO): init background
    framebuffer.Fill(Color::BLACK);
    tempbuffer = framebuffer;

    //(MATEO): laoding toolbar icons
    std::vector<const char*> imagePaths = {
        "images/clear.png",
        "images/load.png",
        "images/save.png",
        "images/eraser.png",
        "images/pencil.png",
        "images/line.png",
        "images/rectangle.png",
        "images/triangle.png",
        "images/black.png",
        "images/white.png",
        "images/pink.png",
        "images/yellow.png",
        "images/red.png",
        "images/blue.png",
        "images/cyan.png"
    };

    std::vector<ButtonType> buttonTypes = {
        BTN_CLEAR, BTN_LOAD, BTN_SAVE,
        BTN_ERASER, BTN_PENCIL,
        BTN_LINE, BTN_RECTANGLE, BTN_TRIANGLE,
        BTN_BLACK, BTN_WHITE, BTN_PINK, BTN_YELLOW,
        BTN_RED, BTN_BLUE, BTN_CYAN
    };

    //(MATEO): toolbar coordinates
    int x = 15;
    int y = 10;
    int d = 40;

    //(MATEO): drawing toolbar
    for (size_t i = 0; i < imagePaths.size(); ++i)
    {
        toolbarButtons.emplace_back(
            imagePaths[i],
            Vector2((float)x, (float)y),
            buttonTypes[i]
        );
        x += d;
    }

    particleSystem.Init();
}


// Render one frame
void Application::Render(void)
{
    // toolbar background
    framebuffer.DrawRect(0, 0, window_width, 50, Color::GRAY, 0, true, Color::GRAY);

    // buttons
    for (size_t i = 0; i < toolbarButtons.size(); ++i)
        toolbarButtons[i].Render(framebuffer);

    // animation mode
    if (mode == MODE_ANIMATION)
        particleSystem.Render(&framebuffer);

    framebuffer.Render();
}


// Called after render
void Application::Update(float dt)
{
    if (mode == MODE_ANIMATION)
        particleSystem.Update(dt);
}


//(MATEO): key pressing modes
void Application::OnKeyPressed(SDL_KeyboardEvent event)
{
    // Debug: show pressed key
    std::cout << "Key pressed: " << event.keysym.sym << std::endl;

    switch (event.keysym.sym) {
        case SDLK_ESCAPE: exit(0); break; // ESC key, kill the app
            
        // border width +
        case SDLK_PLUS:
        case SDLK_KP_PLUS:
            if (borderWidth < MAX_BORDER_WIDTH)
                borderWidth++;
            break;
            
        // border width -
        case SDLK_MINUS:
        case SDLK_KP_MINUS:
            if (borderWidth > MIN_BORDER_WIDTH)
                borderWidth--;
            break;

        // change drawing tools
        case SDLK_1:
            mode = MODE_PAINT;
            tool = TOOL_LINE;
            break;

        case SDLK_2:
            mode = MODE_PAINT;
            tool = TOOL_RECT;
            break;

        case SDLK_3:
            mode = MODE_PAINT;
            tool = TOOL_TRIANGLE;
            triPoints.clear(); //
            break;
        //paint mode
        case SDLK_4:
            mode = MODE_PAINT;
            tool = TOOL_PENCIL;
            break;

        // animation mode
        case SDLK_5:
            mode = MODE_ANIMATION;
            particleSystem.Init();
            break;

        // fill figures
        case SDLK_f:
            isFilled = !isFilled;
            break;
    }
}

void Application::OnMouseButtonDown(SDL_MouseButtonEvent event)
{
    if (event.button == SDL_BUTTON_LEFT)
    {
        for (size_t i = 0; i < toolbarButtons.size(); ++i)
        {
            if (toolbarButtons[i].IsMouseInside(mouse_position))
            {
                HandleButton(toolbarButtons[i].GetType());
                return; // does not start drawing if a button was clicked
            }
        }

        // draws on the canvas
        mouseDown = true;
        startPos = mouse_position;

        tempbuffer = framebuffer;
    }
}


//(MARTINA)
void Application::OnMouseButtonUp(SDL_MouseButtonEvent event)
{
    if (event.button == SDL_BUTTON_LEFT)
    {
        // finish mouse action
        mouseDown = false;
        lastMousePosition = Vector2(-1, -1);

        // commit
        if (tool == TOOL_LINE)
        {
            framebuffer = tempbuffer;
            framebuffer.DrawLineDDA(
                (int)startPos.x, (int)startPos.y,
                (int)mouse_position.x, (int)mouse_position.y,
                drawingColor
            );
            tempbuffer = framebuffer;
        }
        else if (tool == TOOL_RECT)
        {
            framebuffer = tempbuffer;

            int x0 = (int)startPos.x;
            int y0 = (int)startPos.y;
            int x1 = (int)mouse_position.x;
            int y1 = (int)mouse_position.y;

            //make rect valid regardless of direction
            int rx = std::min(x0, x1);
            int ry = std::min(y0, y1);
            int rw = std::abs(x1 - x0);
            int rh = std::abs(y1 - y0);

            framebuffer.DrawRect(rx, ry, rw, rh, drawingColor, borderWidth, isFilled, drawingColor);
            tempbuffer = framebuffer;
        }
        else if (tool == TOOL_TRIANGLE)
        {
            // triangle w 3 clicks
            triPoints.push_back(mouse_position);

            if (triPoints.size() == 3)
            {
                framebuffer.DrawTriangle(
                    triPoints[0], triPoints[1], triPoints[2],
                    drawingColor, isFilled, drawingColor
                );

                triPoints.clear();
                tempbuffer = framebuffer;
            }
        }
        else if (tool == TOOL_PENCIL || tool == TOOL_ERASER)
        {

            tempbuffer = framebuffer;
        }
    }
}

//(MARTINA): toolbar button functions
void Application::HandleButton(ButtonType type)
{
    switch (type)
    {
    case BTN_CLEAR:
       mode = MODE_PAINT;
       framebuffer.Fill(Color::BLACK);
       tempbuffer = framebuffer;
       break;
            
   case BTN_SAVE:
       framebuffer.SaveTGA("saved.tga");
       break;

   case BTN_LOAD:
       framebuffer.LoadTGA("saved.tga");
       tempbuffer = framebuffer;
       break;

    case BTN_ERASER:
        tool = TOOL_ERASER;
        mode = MODE_PAINT;
        break;
    case BTN_PENCIL:
        tool = TOOL_PENCIL;
        mode = MODE_PAINT;
        break;
    case BTN_LINE:
        tool = TOOL_LINE;
        mode = MODE_PAINT;
        break;
    case BTN_RECTANGLE:
        tool = TOOL_RECT;
        mode = MODE_PAINT;
        break;
    case BTN_TRIANGLE:
        tool = TOOL_TRIANGLE;
        mode = MODE_PAINT;
        triPoints.clear();
        break;
        
    
    case BTN_BLACK: drawingColor = Color::BLACK; break;
    case BTN_WHITE: drawingColor = Color::WHITE; break;
    case BTN_PINK:  drawingColor = Color(255, 0, 255); break;
    case BTN_YELLOW: drawingColor = Color::YELLOW; break;
    case BTN_RED:   drawingColor = Color::RED; break;
    case BTN_BLUE:  drawingColor = Color::BLUE; break;
    case BTN_CYAN:  drawingColor = Color::CYAN; break;
    default:
        break;
    }
}

//(MARTINA)
void Application::OnMouseMove(SDL_MouseButtonEvent event)
{
    if (!mouseDown)
        return;

    int mx = (int)mouse_position.x;
    int my = (int)mouse_position.y;

    // pencil/eraser
    if (tool == TOOL_PENCIL || tool == TOOL_ERASER)
    {
        Color c = drawingColor;
        if (tool == TOOL_ERASER)
            c = Color::BLACK;

        if (lastMousePosition.x >= 0 && lastMousePosition.y >= 0)
        {
            framebuffer.DrawLineDDA(
                (int)lastMousePosition.x, (int)lastMousePosition.y,
                mx, my,
                c
            );
        }

        lastMousePosition = mouse_position;
        tempbuffer = framebuffer;
    }
    // line/rectangle
    {
        framebuffer = tempbuffer;

        int x0 = (int)startPos.x;
        int y0 = (int)startPos.y;

        if (tool == TOOL_LINE)
        {
            framebuffer.DrawLineDDA(x0, y0, mx, my, drawingColor);
        }
        else if (tool == TOOL_RECT)
        {
            int rx = std::min(x0, mx);
            int ry = std::min(y0, my);
            int rw = std::abs(mx - x0);
            int rh = std::abs(my - y0);

            framebuffer.DrawRect(rx, ry, rw, rh, drawingColor, borderWidth, isFilled, drawingColor);
        }
    }
}


//-------------------


void Application::OnWheel(SDL_MouseWheelEvent event)
{
    float dy = event.preciseY;

    // ...
}

void Application::OnFileChanged(const char* filename)
{
    Shader::ReloadSingleShader(filename);
}
