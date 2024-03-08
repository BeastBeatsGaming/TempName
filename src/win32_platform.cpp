#include "platform.h"
#include "logger.h"

#include "input.h"
#include <glcorearb.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "wglext.h"

//Windows Globals
static HWND window;
static HDC dc;

//Platform Implementations
LRESULT CALLBACK windows_window_callback(HWND window, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (uMsg)
    {
        case WM_CLOSE:
        {
            running = false;
        } break;

        case WM_SIZE:
        {
            RECT rect = {};
            GetClientRect(window, &rect);
            input.screenSizeX = rect.right - rect.left;
            input.screenSizeY = rect.bottom - rect.top;
        } break;

        default:
        {
            result = DefWindowProcA(window, uMsg, wParam, lParam);
        } break;
    }

    return result;
}

bool platform_create_window(int width, int height, char* title)
{
    HINSTANCE instance = GetModuleHandleA(0);

    WNDCLASSA wc = {};
    wc.hInstance = instance;
    wc.hIcon = LoadIcon(instance, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = title;
    wc.lpfnWndProc = windows_window_callback; //Callback for input into the window

    if (!RegisterClassA(&wc))
    {
        return false;
    }

    //WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
    int dwStyle = WS_OVERLAPPEDWINDOW;

    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;

    //Fake Window inititalizating OpenGL
    {
        window = CreateWindowExA(
            0,title,                        //This references lpzClassName
            title,                          //This references the actual title
            dwStyle,
            100,
            100,
            width,
            height,
            NULL,                           //Parent
            NULL,                           //Menu
            instance,
            NULL                           //lpPar
        );

        if (window == false)
        {
            SM_ASSERT("Window", "Failed to create window");
            return false;
        }

        HDC fakeDC = GetDC(window);
        if (!fakeDC)
        {
            SM_ASSERT("Window", "Failed to create fake DC");
            return false;
        }

        PIXELFORMATDESCRIPTOR pfd = {0};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cAlphaBits = 8;
        pfd.cDepthBits = 24;

        int pixelFormat = ChoosePixelFormat(fakeDC, &pfd);
        if (!pixelFormat)
        {
            SM_ASSERT("Window", "Failed to choose pixel format");
            return false;
        }

        if (!SetPixelFormat(fakeDC, pixelFormat, &pfd))
        {
            SM_ASSERT("Window", "Failed to set pixel format");
            return false;
        }

        //Create a handle to a fake OpenGL Rendering context
        HGLRC fakeRC = wglCreateContext(fakeDC);
        if (!fakeRC)
        {
            SM_ASSERT("Window", "Failed to create fake rendering context");
            return false;
        }

        if (!wglMakeCurrent(fakeDC, fakeRC))
        {
            SM_ASSERT("Window", "Failed to make fake rendering context current");
            return false;
        }

        wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)platform_load_gl_function("wglChoosePixelFormatARB");
        wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)platform_load_gl_function("wglCreateContextAttribsARB");

        if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB)
        {
            SM_ASSERT("OpenGL", "Failed to load WGL extensions");
            return false;
        }

        //Clean up the fake stuff
        wglMakeCurrent(fakeDC, 0);
        wglDeleteContext(fakeRC);
        ReleaseDC(window, fakeDC);
        DestroyWindow(window);
    }

    //Actual OpenGL initialization
    {
        //Add the border size of the window
        {
            RECT borderRect = {};
            AdjustWindowRectEx(&borderRect, dwStyle, false, 0);

            width += borderRect.right - borderRect.left;
            height += borderRect.bottom - borderRect.top;
        }

        window = CreateWindowExA(
            0,title,                        //This references lpzClassName
            title,                          //This references the actual title
            dwStyle,
            100,
            100,
            width,
            height,
            NULL,                           //Parent
            NULL,                           //Menu
            instance,
            NULL                           //lpPar
        );

        if (window == false)
        {
            SM_ASSERT("Window", "Failed to create window");
            return false;
        }

        dc = GetDC(window);
        if (!dc)
        {
            SM_ASSERT("Window", "Failed to getDevice Context");
            return false;
        }

        const int pixelAttribs[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_SWAP_METHOD_ARB, WGL_SWAP_COPY_ARB,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_COLOR_BITS_ARB, 32,
            WGL_ALPHA_BITS_ARB, 8,
            WGL_DEPTH_BITS_ARB, 24,
            0
        };

        UINT numPixelFormats;
        int pixelFormat;
        if (!wglChoosePixelFormatARB(dc, pixelAttribs, 0, 1, &pixelFormat, &numPixelFormats))
        {
            SM_ASSERT("Window", "Failed to choose pixel format");
            return false;
        }

        PIXELFORMATDESCRIPTOR pfd = {0};
        DescribePixelFormat(dc, pixelFormat, sizeof(pfd), &pfd);
        if (!SetPixelFormat(dc, pixelFormat, &pfd))
        {
            SM_ASSERT("Window", "Failed to set pixel format");
            return false;
        }

        const int contextAttribs[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
            0
        };

        HGLRC rc = wglCreateContextAttribsARB(dc, 0, contextAttribs);
        if (!rc)
        {
            SM_ASSERT("Window", "Failed to create rendering context for OpenGL");
            return false;
        }

        if (!wglMakeCurrent(dc, rc))
        {
            SM_ASSERT("Window", "Failed to make rendering context current");
            return false;
        }
    }

    ShowWindow(window, SW_SHOW);

    return true;
}

void platform_update_window()
{
    MSG msg;

    while (PeekMessageA(&msg,window,0,0,PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);                      //Calls the callback specificed when creating the window
    }
}

void* platform_load_gl_function(char* funName)
{
    PROC proc = wglGetProcAddress(funName);
    if (!proc)
    {
        static HMODULE openglDLL = LoadLibraryA("opengl32.dll");
        proc = GetProcAddress(openglDLL, funName);
        if (!proc)
        {
            SM_ASSERT("OpenGL", "Failed to load function: %s", funName);
            return nullptr;
        }
    }

    return (void*)proc;
}

void platform_swap_buffers()
{
    SwapBuffers(dc);
}