#include <iostream>
#include <SDL.h>
#include <SDL_mixer.h>
#include <vector>
#include <string>
Mix_Music *loadMusic(const char* path);
void play(Mix_Music *gMusic);
