#include "logger.h"

#include "input.h"

#include "game.cpp"

#include "platform.h"

#define APIENTRY
#define GL_GLEXT_PROTOTYPES
#include "glcorearb.h"

//Windows Platform
#ifdef _WIN32
#include "win32_platform.cpp"
#endif

#include "gl_renderer.cpp"

int main()
{
    BumpAllocator transientStorage = make_bump_allocator(MB(50));

    platform_create_window(1200,720,"Temp");
    input.screenSizeX = 1200;
    input.screenSizeY = 720;

    gl_init(&transientStorage);

    while (running)
    {
        //Update
        platform_update_window();
        update_game();
        gl_render();

        platform_swap_buffers();
    }
    
    return 0;
}