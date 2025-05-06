#include <iostream>
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <string>
TTF_Font* loadFont(const char* path, int size);
SDL_Texture* renderText(const char* text, TTF_Font* font, SDL_Color textColor,SDL_Renderer* renderer);

