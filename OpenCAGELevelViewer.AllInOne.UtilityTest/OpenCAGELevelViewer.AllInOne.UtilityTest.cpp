#include <SDL3/SDL.h>

#include <iostream>

std::string glVersion = "";

int main()
{
    {
        SDL_InitSubSystem(SDL_INIT_VIDEO);

        SDL_Window *sdlWindow = SDL_CreateWindow("OpenCAGELevelViewer Utility Window", 1, 1, SDL_WINDOW_OPENGL | SDL_WINDOW_NOT_FOCUSABLE | SDL_WINDOW_UTILITY | SDL_WINDOW_OCCLUDED);

        SDL_GLContext glContext = SDL_GL_CreateContext(sdlWindow);


        const unsigned char *(*glGetString)(int name) = reinterpret_cast<const unsigned char *(*)(int name)>(SDL_GL_GetProcAddress("glGetString"));

        // 7938 is GL_VERSION
        glVersion = std::string(reinterpret_cast < const char * >(glGetString(7938)));


        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(sdlWindow);

        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    std::cout << glVersion << std::endl;

    std::cout << "Press any key to continue..." << std::endl;
    getchar();
}