#pragma once

#include <sdl.h>

class sdlHelper
{
public:
	SDL_Rect gRect;

	SDL_Window* gWindow = nullptr;
	SDL_Renderer* gRender = nullptr;
	SDL_Texture* texture = nullptr;

	int initSdl();

	int close();

};

