#include "sdlHelper.h"
#include "iostream"


//sdl初始化
int sdlHelper::initSdl()
{
	bool success = true;

	if (SDL_Init(SDL_INIT_VIDEO))
	{
		printf("init sdl error:%s\n", SDL_GetError());
		success = false;
	}

	//创建window
	gWindow = SDL_CreateWindow("decode video", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN);
	if (gWindow == nullptr)
	{
		printf("create window error: %s\n", SDL_GetError());
		success = false;
	}

	//创建渲染器
	gRender = SDL_CreateRenderer(gWindow, -1, 0);
	if (gRender == nullptr)
	{
		printf("init window Render error: %s\n", SDL_GetError());
		success = false;
	}

	return success;
}

int sdlHelper::close()
{
	bool success = true;

	SDL_DestroyTexture(texture);
	texture = nullptr;
	SDL_DestroyRenderer(gRender);
	gRender = nullptr;
	SDL_DestroyWindow(gWindow);
	gWindow = nullptr;

	SDL_Quit();

	return success;
}