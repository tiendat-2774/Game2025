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

enum { MENU_CLASSIC = 1, MENU_TWOLAYER, MENU_QUIT };

struct Point { int x, y; bool operator==(const Point& o) const { return x == o.x && y == o.y; } };

// Global textures
SDL_Texture* gHeadTexture       = nullptr;
SDL_Texture* gBodyTexture       = nullptr;
SDL_Texture* gFoodTexture       = nullptr;
SDL_Texture* gFakeTexture       = nullptr;
SDL_Texture* gBackgroundTexture = nullptr;

// Function prototypes
void QuitSDL(SDL_Window* window, SDL_Renderer* renderer);
bool CheckInit(SDL_Window* w, SDL_Renderer* r);
int ShowMenu(SDL_Renderer* ren, TTF_Font* font);
void CoreGame(SDL_Renderer* ren, SDL_Window* win, TTF_Font* font, int mode);

int main(int argc, char* argv[]) {
    srand((unsigned)time(nullptr));
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        cerr << "SDL_Init Error: " << SDL_GetError() << endl;
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        cerr << "IMG_Init Error: " << IMG_GetError() << endl;
        SDL_Quit();
        return 1;
    }
    if (Mix_Init(MIX_INIT_OGG) != MIX_INIT_OGG) {
        cerr << "Mix_Init Error: " << Mix_GetError() << endl;
        IMG_Quit(); SDL_Quit();
        return 1;
    }
    if (TTF_Init() != 0) {
        cerr << "TTF_Init Error: " << TTF_GetError() << endl;
        Mix_Quit(); IMG_Quit(); SDL_Quit();
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow(WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!CheckInit(win, ren)) return 1;

    TTF_Font* font = TTF_OpenFont("times.ttf", 24);
    if (!font) {
        cerr << "TTF_OpenFont Error: " << TTF_GetError() << endl;
        QuitSDL(win, ren);
        return 1;
    }

    gBackgroundTexture = loadTexture("background.jpg", ren);
    if (!gBackgroundTexture) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed loading background.jpg", win);
        QuitSDL(win, ren);
        return 1;
    }

    int mode;
    while ((mode = ShowMenu(ren, font)) != MENU_QUIT) {
        CoreGame(ren, win, font, mode);
    }

    QuitSDL(win, ren);
    return 0;
}

void QuitSDL(SDL_Window* window, SDL_Renderer* renderer) {
    if (gHeadTexture)       SDL_DestroyTexture(gHeadTexture);
    if (gBodyTexture)       SDL_DestroyTexture(gBodyTexture);
    if (gFoodTexture)       SDL_DestroyTexture(gFoodTexture);
    if (gFakeTexture)       SDL_DestroyTexture(gFakeTexture);
    if (gBackgroundTexture) SDL_DestroyTexture(gBackgroundTexture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window)   SDL_DestroyWindow(window);
    Mix_Quit(); IMG_Quit(); TTF_Quit(); SDL_Quit();
}

bool CheckInit(SDL_Window* w, SDL_Renderer* r) {
    if (!w) {
        cerr << "SDL_CreateWindow Error: " << SDL_GetError() << endl;
        return false;
    }
    if (!r) {
        cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << endl;
        SDL_DestroyWindow(w);
        return false;
    }
    return true;
}

int ShowMenu(SDL_Renderer* ren, TTF_Font* font) {
    vector<string> options = { "Classic Mode", "Two-Layer Mode", "Quit" };
    int selected = 0;
    SDL_Event e;
    // Pre-render texts
    vector<SDL_Texture*> texts(options.size());
    vector<SDL_Rect> rects(options.size());
    while (SDL_PollEvent(&e)); // clear input
    for (size_t i = 0; i < options.size(); ++i) {
        texts[i] = renderText(options[i].c_str(), font, {255,255,255}, ren);
        SDL_QueryTexture(texts[i], nullptr, nullptr, &rects[i].w, &rects[i].h);
        rects[i].x = (SCREEN_WIDTH - rects[i].w) / 2;
        rects[i].y = 300 + int(i) * 60;
    }
    while (true) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                return MENU_QUIT;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_UP || e.key.keysym.sym == SDLK_w)
                    selected = (selected - 1 + options.size()) % options.size();
                else if (e.key.keysym.sym == SDLK_DOWN || e.key.keysym.sym == SDLK_s)
                    selected = (selected + 1) % options.size();
                else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)
                    return selected + 1;
            }
        }
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        if (gBackgroundTexture) SDL_RenderCopy(ren, gBackgroundTexture, nullptr, nullptr);
        for (size_t i = 0; i < options.size(); ++i) {
            SDL_DestroyTexture(texts[i]);
            SDL_Color col = (i == selected) ? SDL_Color{255,0,0} : SDL_Color{255,255,255};
            texts[i] = renderText(options[i].c_str(), font, col, ren);
            SDL_QueryTexture(texts[i], nullptr, nullptr, &rects[i].w, &rects[i].h);
            rects[i].x = (SCREEN_WIDTH - rects[i].w) / 2;
            SDL_RenderCopy(ren, texts[i], nullptr, &rects[i]);
        }
        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }
}

void CoreGame(SDL_Renderer* ren, SDL_Window* win, TTF_Font* font, int mode) {
    bool isClassic = (mode == MENU_CLASSIC);
    // Load textures
    gHeadTexture = loadTexture("head.png", ren);
    gBodyTexture = loadTexture("body.png", ren);
    gFoodTexture = loadTexture("food.png", ren);
    gFakeTexture = loadTexture("fake.png", ren);
    if (!gHeadTexture || !gBodyTexture || !gFoodTexture || (!isClassic && !gFakeTexture)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Missing textures.", win);
        return;
    }
    vector<Point> snake;
    snake.push_back({SCREEN_WIDTH/2/RECT_SIZE*RECT_SIZE, SCREEN_HEIGHT/2/RECT_SIZE*RECT_SIZE});
    Point dir = {RECT_SIZE,0}, nextDir = dir;
    Point food, fake;
    bool fakeActive = !isClassic, fakeEaten = false;
    auto placeFood = [&]() {
        do { food.x = (rand()%(SCREEN_WIDTH/RECT_SIZE))*RECT_SIZE;
             food.y = (rand()%(SCREEN_HEIGHT/RECT_SIZE))*RECT_SIZE;
        } while (find(snake.begin(), snake.end(), food) != snake.end());
    };
    auto placeFake = [&]() {
        do { fake.x = (rand()%(SCREEN_WIDTH/RECT_SIZE))*RECT_SIZE;
             fake.y = (rand()%(SCREEN_HEIGHT/RECT_SIZE))*RECT_SIZE;
        } while (find(snake.begin(), snake.end(), fake) != snake.end() || fake == food);
    };
    placeFood(); if (fakeActive) placeFake();
    Uint32 lastMove = SDL_GetTicks(), interval = 150;
    bool running = true, paused = false;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { running = false; break; }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) paused = true;
                if (!paused) {
                    if ((e.key.keysym.sym == SDLK_UP || e.key.keysym.sym == SDLK_w) && dir.y==0) nextDir={0,-RECT_SIZE};
                    else if ((e.key.keysym.sym == SDLK_DOWN || e.key.keysym.sym == SDLK_s) && dir.y==0) nextDir={0,RECT_SIZE};
                    else if ((e.key.keysym.sym == SDLK_LEFT || e.key.keysym.sym == SDLK_a) && dir.x==0) nextDir={-RECT_SIZE,0};
                    else if ((e.key.keysym.sym == SDLK_RIGHT || e.key.keysym.sym == SDLK_d) && dir.x==0) nextDir={RECT_SIZE,0};
                }
            }
        }
        if (paused) {
            int sel = ShowMenu(ren, font);
            if (sel == MENU_CLASSIC) paused=false;
            else break;
        }
        Uint32 now = SDL_GetTicks();
        if (now - lastMove >= interval) {
            lastMove = now; dir = nextDir;
            Point head = { (snake.front().x + dir.x + SCREEN_WIDTH)%SCREEN_WIDTH,
                           (snake.front().y + dir.y + SCREEN_HEIGHT)%SCREEN_HEIGHT };
            if (find(snake.begin(), snake.end(), head) != snake.end()) break;
            snake.insert(snake.begin(), head);
            if (head == food) { placeFood(); if (fakeActive && fakeEaten) placeFake(); }
            else if (fakeActive && head == fake) { fakeEaten = true; }
            else snake.pop_back();
        }
        SDL_SetRenderDrawColor(ren,0,0,0,255);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren,gBackgroundTexture,nullptr,nullptr);
        SDL_Rect rf={food.x,food.y,RECT_SIZE,RECT_SIZE}; SDL_RenderCopy(ren,gFoodTexture,nullptr,&rf);
        if(fakeActive && !fakeEaten){ SDL_Rect rfa={fake.x,fake.y,RECT_SIZE,RECT_SIZE}; SDL_RenderCopy(ren,gFakeTexture,nullptr,&rfa);}
        for(size_t i=0;i<snake.size();++i){ SDL_Rect rs={snake[i].x,snake[i].y,RECT_SIZE,RECT_SIZE}; SDL_RenderCopy(ren, i==0?gHeadTexture:gBodyTexture,nullptr,&rs);}
        SDL_RenderPresent(ren); SDL_Delay(16);
    }
    SDL_DestroyTexture(gHeadTexture);
    SDL_DestroyTexture(gBodyTexture);
    SDL_DestroyTexture(gFoodTexture);
    if(fakeActive) SDL_DestroyTexture(gFakeTexture);
}
