//Path: 01_Normals\Source Files\main.cpp

#include <GL/glew.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include <iostream>
#include <sstream>

#include "GLDebugMessageCallback.h"

#include "Raytrace.h"


int main(int argc, char* args[])
{
	// Init SDL
	if (SDL_Init(SDL_INIT_VIDEO) == -1)
	{
		std::cout << "[SDL initialization] Error during the SDL initialization: " << SDL_GetError() << std::endl;
		return 1;
	}

	// Init OpenGL
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef _DEBUG
	// If the compilation is in debug mode, the OpenGL context should also be in debug mode for the debug callback to work
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	// Set rgb, opacity size (bits)
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	// Double buffer
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	// Depth buffer
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Antialiasing
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); // MSAA with 4 samples
	glEnable(GL_MULTISAMPLE);

	// Create window
	SDL_Window* win = 0;
	win = SDL_CreateWindow("Raytrace", 50, 50, 1200, 650,
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	//win = SDL_CreateWindow("Raytrace", 0, 0, 0, 0,
	//	SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);


	// Error handling
	if (win == 0)
	{
		std::cout << "[Window creation] SDL initalization error: " << SDL_GetError() << std::endl;
		return 1;
	}


	// Create openGL context (through this will we draw)
	SDL_GLContext	context = SDL_GL_CreateContext(win);
	if (context == 0)
	{
		std::cout << "[Window creation] Error during the creation of an SDL window: " << SDL_GetError() << std::endl;
		return 1;
	}

	// Display: waiting for vsync
	SDL_GL_SetSwapInterval(1);

	// Init GLEW
	GLenum error = glewInit();
	if (error != GLEW_OK)
	{
		std::cout << "[GLEW] Error during the initialization of glew." << std::endl;
		return 1;
	}

	// Asking for openGL version
	int glVersion[2] = { -1, -1 };
	glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
	glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);
	std::cout << "Running OpenGL " << glVersion[0] << "." << glVersion[1] << std::endl;

	if (glVersion[0] == -1 && glVersion[1] == -1)
	{
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(win);

		std::cout << "[OGL context creation] Error during the creation of the OGL context: " << SDL_GetError() << std::endl;

		return 1;
	}

	std::stringstream window_title;
	window_title << "OpenGL " << glVersion[0] << "." << glVersion[1];
	SDL_SetWindowTitle(win, window_title.str().c_str());

	// Enable and set the debug callback function if we are in a debug context
	GLint context_flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
	if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		glDebugMessageCallback(GLDebugMessageCallback, nullptr);
	}

	// Start main message processing loop
	bool quit = false;
	SDL_Event ev;

	Raytrace app;
	if (!app.Init())
	{
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(win);
		std::cout << "[app.Init] Error during the initialization of the application!" << std::endl;
		return 1;
	}

	// Pass the width and height of the window to Init() in order to set the camera's aspect ratio with perspective()
	int w, h;
	SDL_GetWindowSize(win, &w, &h);
	app.Resize(w, h);

	while (!quit)
	{
		while (SDL_PollEvent(&ev))
		{
			switch (ev.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				if (ev.key.keysym.sym == SDLK_ESCAPE)
					quit = true;
				else if (ev.key.keysym.sym == SDLK_RIGHT)
				{
					app.current_scene = std::min(2, app.current_scene + 1);
					std::cout << "Current scene: " << app.current_scene << std::endl;
				}
				else if (ev.key.keysym.sym == SDLK_LEFT)
				{
					app.current_scene = std::max(0, app.current_scene - 1);
					std::cout << "Current scene: " << app.current_scene << std::endl;
				}
				else if (ev.key.keysym.sym == SDLK_r) {
					// Generate a new random position for the fourth sphere
					app.spheres[app.spheres.size() - 1] = app.getRandomPosition();
					app.meteorVelocity = { 0.0f,0.0f,0.0f };
					app.r_pressed = true;
				}
				else if (ev.key.keysym.sym == SDLK_t)
				{
					app.spheres[app.spheres.size() - 1] = { 0.0f,0.0f,0.0f,10.0f };
				}
				else if (ev.key.keysym.sym == SDLK_PLUS || ev.key.keysym.sym == SDLK_KP_PLUS) {
					if (app.depth + 1 <= app.maxDepth)
						app.depth++;
				}
				else if (ev.key.keysym.sym == SDLK_MINUS || ev.key.keysym.sym == SDLK_KP_MINUS) {
					if (app.depth >= 1)
						app.depth--;
				}
				else if (ev.key.keysym.sym == SDLK_SPACE) {
					app.stop_animation = !app.stop_animation;
				} else if (ev.key.keysym.sym == SDLK_TAB) {
					app.slow_down_animation = !app.slow_down_animation;
				}
				app.KeyboardDown(ev.key);
				break;
			case SDL_KEYUP:
				app.KeyboardUp(ev.key);
				break;
			case SDL_MOUSEBUTTONDOWN:
				app.MouseDown(ev.button);
				break;
			case SDL_MOUSEBUTTONUP:
				app.MouseUp(ev.button);
				break;
			case SDL_MOUSEWHEEL:
				app.MouseWheel(ev.wheel);
				break;
			case SDL_MOUSEMOTION:
				app.MouseMove(ev.motion);
				break;
			case SDL_WINDOWEVENT:
				if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				{
					app.Resize(ev.window.data1, ev.window.data2);
				}
				break;
			}
		}

		app.Update();
		app.Render();

		SDL_GL_SwapWindow(win);
	}

	// Exit
	app.Clean();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);

	return 0;
}