#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include "SDL_utils.h"
#include "SDL-Mix.h"
#include "SDL_text.h"

using namespace std;

const int SCREEN_WIDTH  = 600;
const int SCREEN_HEIGHT = 800;
const int RECT_SIZE     = 20;
const char* WINDOW_TITLE = "Snake Game";

SDL_Texture* gHeadTexture       = nullptr;
SDL_Texture* gBodyTexture       = nullptr;
SDL_Texture* gFoodTexture       = nullptr;
SDL_Texture* gFakeTexture       = nullptr;
SDL_Texture* gBackgroundTexture = nullptr;

struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
};

// Menu return values
enum { MENU_CLASSIC = 1, MENU_TWOLAYER, MENU_QUIT };

void quitSDL(SDL_Window* window, SDL_Renderer* renderer) {
    if (gHeadTexture)       SDL_DestroyTexture(gHeadTexture);
    if (gBodyTexture)       SDL_DestroyTexture(gBodyTexture);
    if (gFoodTexture)       SDL_DestroyTexture(gFoodTexture);
    if (gFakeTexture)       SDL_DestroyTexture(gFakeTexture);
    if (gBackgroundTexture) SDL_DestroyTexture(gBackgroundTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit(); IMG_Quit(); TTF_Quit(); SDL_Quit();
}

int ShowMenu(SDL_Renderer* ren, TTF_Font* font) {
    vector<string> options = { "Classic Mode", "Two-Layer Mode", "Quit" };
    int selected = 0;
    SDL_Event e;
    while (true) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        if (gBackgroundTexture) SDL_RenderCopy(ren, gBackgroundTexture, nullptr, nullptr);
        for (int i = 0; i < (int)options.size(); ++i) {
            SDL_Color color = (i == selected) ? SDL_Color{255, 0, 0} : SDL_Color{255, 255, 255};
            SDL_Texture* txt = renderText(options[i].c_str(), font, color, ren);
            int w, h;
            SDL_QueryTexture(txt, nullptr, nullptr, &w, &h);
            renderTexture(txt, (SCREEN_WIDTH - w) / 2, 300 + i * 60, ren);
            SDL_DestroyTexture(txt);
        }
        SDL_RenderPresent(ren);
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return MENU_QUIT;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_UP) selected = (selected - 1 + options.size()) % options.size();
                else if (e.key.keysym.sym == SDLK_DOWN) selected = (selected + 1) % options.size();
                else if (e.key.keysym.sym == SDLK_RETURN) return selected + 1;
            }
        }
        SDL_Delay(100);
    }
}

int ShowPauseMenu(SDL_Renderer* ren, TTF_Font* font) {
    vector<string> options = { "Resume", "Main Menu" };
    int selected = 0;
    SDL_Event e;
    while (true) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 200);
        SDL_RenderClear(ren);
        if (gBackgroundTexture) SDL_RenderCopy(ren, gBackgroundTexture, nullptr, nullptr);
        for (int i = 0; i < (int)options.size(); ++i) {
            SDL_Color color = (i == selected) ? SDL_Color{255, 0, 0} : SDL_Color{255, 255, 255};
            SDL_Texture* txt = renderText(options[i].c_str(), font, color, ren);
            int w, h;
            SDL_QueryTexture(txt, nullptr, nullptr, &w, &h);
            renderTexture(txt, (SCREEN_WIDTH - w) / 2, 300 + i * 60, ren);
            SDL_DestroyTexture(txt);
        }
        SDL_RenderPresent(ren);
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return 1;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_UP) selected = (selected - 1 + options.size()) % options.size();
                else if (e.key.keysym.sym == SDLK_DOWN) selected = (selected + 1) % options.size();
                else if (e.key.keysym.sym == SDLK_RETURN) return selected;
            }
        }
        SDL_Delay(100);
    }
}

void CoreGame(SDL_Renderer* ren, SDL_Window* win, TTF_Font* font, int mode) {
    bool isClassic = (mode == MENU_CLASSIC);
    gHeadTexture       = loadTexture("head.png", ren);
    gBodyTexture       = loadTexture("body.png", ren);
    gFoodTexture       = loadTexture("food.png", ren);
    gFakeTexture       = loadTexture("fake.png", ren);
    gBackgroundTexture = loadTexture("background.jpg", ren);
    if (!gHeadTexture || !gBodyTexture || !gFoodTexture || !gFakeTexture) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Missing textures.", win);
        return;
    }

    srand((unsigned)time(nullptr));
    vector<Point> body;
    Point dir = {RECT_SIZE, 0}, nextDir = dir;
    Uint32 lastMove = SDL_GetTicks();
    const Uint32 moveInterval = 150;
    const Uint32 frameDelay   = 1000 / 60;
    bool quit = false, gameOver = false, paused = false;
    SDL_Event e;

    Point realFood, fakeFood;
    bool fakeConverted = false;

    auto genReal = [&]() {
        do { realFood.x = (rand() % (SCREEN_WIDTH / RECT_SIZE)) * RECT_SIZE;
             realFood.y = (rand() % (SCREEN_HEIGHT / RECT_SIZE)) * RECT_SIZE;
        } while (find(body.begin(), body.end(), realFood) != body.end());
    };
    auto genFake = [&]() {
        do { fakeFood.x = (rand() % (SCREEN_WIDTH / RECT_SIZE)) * RECT_SIZE;
             fakeFood.y = (rand() % (SCREEN_HEIGHT / RECT_SIZE)) * RECT_SIZE;
        } while (isClassic || find(body.begin(), body.end(), fakeFood) != body.end() || fakeFood == realFood);
    };

    // Initialize
    body.push_back({SCREEN_WIDTH/2/RECT_SIZE*RECT_SIZE, SCREEN_HEIGHT/2/RECT_SIZE*RECT_SIZE});
    genReal(); genFake();

    while (!quit && !gameOver) {
        Uint32 frameStart = SDL_GetTicks();
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;
            else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) paused = true;
                if (!paused) {
                    if ((e.key.keysym.sym == SDLK_w || e.key.keysym.sym == SDLK_UP) && dir.y == 0) nextDir = {0,-RECT_SIZE};
                    if ((e.key.keysym.sym == SDLK_s || e.key.keysym.sym == SDLK_DOWN) && dir.y == 0) nextDir = {0,RECT_SIZE};
                    if ((e.key.keysym.sym == SDLK_a || e.key.keysym.sym == SDLK_LEFT) && dir.x == 0) nextDir = {-RECT_SIZE,0};
                    if ((e.key.keysym.sym == SDLK_d || e.key.keysym.sym == SDLK_RIGHT) && dir.x == 0) nextDir = {RECT_SIZE,0};
                }
            }
        }
        if (paused) {
            int choice = ShowPauseMenu(ren, font);
            if (choice == 0) paused = false;
            else return;
            continue;
        }
        Uint32 now = SDL_GetTicks();
        if (now - lastMove >= moveInterval) {
            lastMove = now;
            dir = nextDir;
            Point head = body.front();
            Point nh = {head.x + dir.x, head.y + dir.y};
            if (nh.x < 0 || nh.x >= SCREEN_WIDTH || nh.y < 0 || nh.y >= SCREEN_HEIGHT) gameOver = true;
            else if (find(body.begin()+1, body.end(), nh) != body.end()) gameOver = true;
            if (!gameOver) {
                body.insert(body.begin(), nh);
                if (isClassic) {
                    if (nh == realFood) genReal();
                    else body.pop_back();
                } else {
                    if (!fakeConverted && nh == fakeFood) {
                        // first fake eat: convert
                        realFood = fakeFood;
                        fakeConverted = true;
                        body.pop_back();
                        continue;
                    } else if (fakeConverted && nh == fakeFood) {
                        // second fake eat: treat as real
                        genReal(); genFake(); fakeConverted = false;
                        continue;
                    } else if (nh == realFood) {
                        genReal(); genFake(); fakeConverted = false;
                        continue;
                    } else {
                        body.pop_back();
                    }
                }
            }
        }
        // Render
        SDL_SetRenderDrawColor(ren,0,0,0,255); SDL_RenderClear(ren);
        if (gBackgroundTexture) SDL_RenderCopy(ren,gBackgroundTexture,nullptr,nullptr);
        if (!isClassic) {
            SDL_Rect rf = {fakeFood.x,fakeFood.y,RECT_SIZE,RECT_SIZE};
            SDL_RenderCopy(ren, fakeConverted? gFoodTexture : gFakeTexture, nullptr, &rf);
        }
        SDL_Rect rr = {realFood.x,realFood.y,RECT_SIZE,RECT_SIZE}; SDL_RenderCopy(ren,gFoodTexture,nullptr,&rr);
        SDL_Rect rs = {0,0,RECT_SIZE,RECT_SIZE}; rs.x=body[0].x; rs.y=body[0].y;
        double ang=0; if (dir.x>0) ang=90; else if (dir.x<0) ang=270; else if (dir.y>0) ang=180;
        SDL_RenderCopyEx(ren,gHeadTexture,nullptr,&rs,ang,nullptr,SDL_FLIP_NONE);
        for (size_t i=1;i<body.size();++i) { rs.x=body[i].x; rs.y=body[i].y; SDL_RenderCopy(ren,gBodyTexture,nullptr,&rs); }
        int score = (int)body.size() - 1;
        SDL_Texture* st = renderText((string("Score: ")+to_string(score)).c_str(), font, SDL_Color{255,0,0}, ren);
        renderTexture(st,10,10,ren); SDL_DestroyTexture(st);
        SDL_RenderPresent(ren);
        Uint32 frameTime = SDL_GetTicks() - frameStart; if(frameDelay > frameTime) SDL_Delay(frameDelay - frameTime);
    }
    if (gameOver) {
        SDL_SetRenderDrawColor(ren,0,0,0,255); SDL_RenderClear(ren);
        if (gBackgroundTexture) SDL_RenderCopy(ren,gBackgroundTexture,nullptr,nullptr);
        SDL_Texture* go = renderText("Game Over",font,SDL_Color{255,0,0},ren);
        renderTexture(go,180,375,ren); SDL_DestroyTexture(go);
        SDL_RenderPresent(ren); SDL_Delay(1000);
    }
}

int main(int argc,char*argv[]) {
    SDL_Window* win = initSDL(SCREEN_WIDTH,SCREEN_HEIGHT,WINDOW_TITLE);
    SDL_Renderer* ren = createRenderer(win,SCREEN_WIDTH,SCREEN_HEIGHT);
    TTF_Font* font = loadFont("timesbd.ttf",50);
    Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,2048);
    Mix_Music* mus = Mix_LoadMUS("assets/RunningAway.mp3"); if(mus) Mix_PlayMusic(mus,-1);
    while (true) {
        int choice = ShowMenu(ren,font);
        if (choice == MENU_QUIT) break;
        CoreGame(ren,win,font,choice);
    }
    if (mus) Mix_FreeMusic(mus);
    Mix_CloseAudio();
    quitSDL(win,ren);
    return 0;
}


