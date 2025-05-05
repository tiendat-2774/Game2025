#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include "SDL_utils.h"
#include <vector>
#include <string>
#include <SDL_ttf.h>

void logErrorAndExit(const char* msg, const char* error)
{
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "%s: %s", msg, error);
    IMG_Quit(); // <<< THÊM VÀO >>> Đảm bảo IMG_Quit được gọi khi có lỗi sau khi IMG_Init
    SDL_Quit();
    exit(1); // <<< THÊM VÀO >>> Thoát chương trình khi có lỗi nghiêm trọng
}
SDL_Renderer* createRenderer(SDL_Window* window,int SCREEN_WIDTH,int  SCREEN_HEIGHT)
{
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED |
                                              SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
         SDL_DestroyWindow(window);
         logErrorAndExit("CreateRenderer", SDL_GetError());
    }


    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    return renderer;
}
SDL_Window* initSDL(int SCREEN_WIDTH, int SCREEN_HEIGHT, const char* WINDOW_TITLE)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        logErrorAndExit("SDL_Init", SDL_GetError());

    SDL_Window* window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    //full screen
    //window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (window == nullptr) logErrorAndExit("CreateWindow", SDL_GetError());

    // <<< THÊM VÀO >>> Khởi tạo SDL_image sớm hơn để logErrorAndExit có thể gọi IMG_Quit
    if (!IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG)) {
        // Log lỗi nhưng không gọi logErrorAndExit vì SDL_Quit chưa nên gọi ngay
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "IMG_Init Error: %s", IMG_GetError());
        SDL_DestroyWindow(window); // Dọn dẹp window đã tạo
        SDL_Quit();                // Thoát SDL chính
        exit(1);                   // Thoát chương trình
    }
     if (TTF_Init() == -1) {
            logErrorAndExit("SDL_ttf could not initialize! SDL_ttf Error: ",
                             TTF_GetError());
        }


    return window;
}
SDL_Texture *loadTexture(const char *filename, SDL_Renderer* renderer)
{
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "Loading %s", filename);

	SDL_Texture *texture = IMG_LoadTexture(renderer, filename);
	if (texture == NULL)
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "Load texture %s failed: %s", filename, IMG_GetError()); // <<< SỬA LỖI >>> Thêm filename vào log lỗi

	return texture;
}
void renderTexture(SDL_Texture *texture, int x, int y,
                   SDL_Renderer* renderer)
{
	SDL_Rect dest;
	dest.x = x;
	dest.y = y;
	SDL_QueryTexture(texture, NULL, NULL, &dest.w, &dest.h);

	SDL_RenderCopy(renderer, texture, NULL, &dest);
}
