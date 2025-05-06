#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
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
SDL_Texture* gBackgroundTexture = nullptr;

struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
};

// Forward declarations
int ShowMenu(SDL_Renderer* renderer, TTF_Font* font);
void CoreGame(SDL_Renderer* renderer, SDL_Window* window, TTF_Font* font, int mode);

void quitSDL(SDL_Window* window, SDL_Renderer* renderer) {
    if (gHeadTexture)       SDL_DestroyTexture(gHeadTexture);
    if (gBodyTexture)       SDL_DestroyTexture(gBodyTexture);
    if (gBackgroundTexture) SDL_DestroyTexture(gBackgroundTexture);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    Mix_Quit();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

int ShowMenu(SDL_Renderer* renderer, TTF_Font* font) {
    vector<string> options = { "Classic Mode", "Two-Layer Food Mode" };
    int selected = 0;
    SDL_Event e;
    while (true) {
        // Render menu background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (gBackgroundTexture) SDL_RenderCopy(renderer, gBackgroundTexture, nullptr, nullptr);

        // Render options
        for (int i = 0; i < options.size(); i++) {
            SDL_Color color = (i == selected) ? SDL_Color{255, 0, 0} : SDL_Color{255, 255, 255};
            SDL_Texture* txt = renderText(options[i].c_str(), font, color, renderer);
            int w, h;
            SDL_QueryTexture(txt, nullptr, nullptr, &w, &h);
            renderTexture(txt, (SCREEN_WIDTH - w) / 2, 250 + i * 60, renderer);
            SDL_DestroyTexture(txt);
        }
        SDL_RenderPresent(renderer);

        // Event handling
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                return 0;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_UP) {
                    selected = (selected - 1 + options.size()) % options.size();
                } else if (e.key.keysym.sym == SDLK_DOWN) {
                    selected = (selected + 1) % options.size();
                } else if (e.key.keysym.sym == SDLK_RETURN) {
                    return selected + 1;
                }
            }
        }
        SDL_Delay(100);
    }
}

void CoreGame(SDL_Renderer* renderer, SDL_Window* window, TTF_Font* font, int mode) {
    // Load textures once
    gHeadTexture       = loadTexture("head.png", renderer);
    gBodyTexture       = loadTexture("body.png", renderer);
    gBackgroundTexture = loadTexture("SnakeGameBackGround.jpg", renderer);

    if (!gHeadTexture || !gBodyTexture) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
            "Cannot load snake head/body textures.", window);
        return;
    }

    srand(static_cast<unsigned int>(time(nullptr)));
    vector<Point> body;
    Point direction = {RECT_SIZE, 0}, next_direction = direction;
    Uint32 lastMoveTime = SDL_GetTicks();
    const Uint32 moveInterval = 150;
    const Uint32 frameDelay   = 1000 / 60;
    bool quit = false, gameOver = false, paused = false;
    SDL_Event e;

    // Food variables
    Point foodClassic;
    Point outerFood, innerFood;
    bool hasInner = false;

    auto generateClassic = [&]() {
        bool onBody;
        do {
            onBody = false;
            int fx = rand() % (SCREEN_WIDTH / RECT_SIZE);
            int fy = rand() % (SCREEN_HEIGHT / RECT_SIZE);
            foodClassic = { fx * RECT_SIZE, fy * RECT_SIZE };
            for (auto& p : body) if (foodClassic == p) { onBody = true; break; }
        } while (onBody);
    };
    auto generateOuter = [&]() {
        bool onBody;
        do {
            onBody = false;
            int fx = rand() % (SCREEN_WIDTH / RECT_SIZE);
            int fy = rand() % (SCREEN_HEIGHT / RECT_SIZE);
            outerFood = { fx * RECT_SIZE, fy * RECT_SIZE };
            for (auto& p : body) if (outerFood == p) { onBody = true; break; }
        } while (onBody);
        hasInner = false;
    };

    // Initialize snake & food
    body.push_back({ (SCREEN_WIDTH/RECT_SIZE/2)*RECT_SIZE, (SCREEN_HEIGHT/RECT_SIZE/2)*RECT_SIZE });
    if (mode == 1) generateClassic(); else generateOuter();

    while (!quit && !gameOver) {
        Uint32 frameStart = SDL_GetTicks();
        // Event
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: paused = !paused; break;
                    case SDLK_w: case SDLK_UP:
                        if (!paused && direction.y == 0) next_direction = {0, -RECT_SIZE}; break;
                    case SDLK_s: case SDLK_DOWN:
                        if (!paused && direction.y == 0) next_direction = {0, RECT_SIZE}; break;
                    case SDLK_a: case SDLK_LEFT:
                        if (!paused && direction.x == 0) next_direction = {-RECT_SIZE, 0}; break;
                    case SDLK_d: case SDLK_RIGHT:
                        if (!paused && direction.x == 0) next_direction = {RECT_SIZE, 0}; break;
                }
            }
        }

        if (paused) {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            SDL_RenderClear(renderer);
            if (gBackgroundTexture) SDL_RenderCopy(renderer, gBackgroundTexture, nullptr, nullptr);
            SDL_Color c = {255, 0, 0};
            SDL_Texture* t = renderText("Press ESC to Resume", font, c, renderer);
            renderTexture(t, 140, 350, renderer);
            SDL_DestroyTexture(t);
            SDL_RenderPresent(renderer);
            SDL_Delay(100);
            continue;
        }

        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastMoveTime >= moveInterval) {
            lastMoveTime = currentTime;
            direction = next_direction;
            Point head = body[0];
            Point nextHead = { head.x + direction.x, head.y + direction.y };
            // Wall collision
            if (nextHead.x < 0 || nextHead.x >= SCREEN_WIDTH || nextHead.y < 0 || nextHead.y >= SCREEN_HEIGHT) {
                gameOver = true;
            } else {
                // Self collision
                for (size_t i = 1; i < body.size(); ++i) if (nextHead == body[i]) { gameOver = true; break; }
            }
            if (!gameOver) {
                body.insert(body.begin(), nextHead);
                // Food logic
                if (mode == 1) {
                    if (nextHead == foodClassic) generateClassic();
                    else body.pop_back();
                } else {
                    // Two-layer food
                    if (!hasInner) {
                        if (nextHead == outerFood) {
                            hasInner = true;
                            innerFood = outerFood;
                        }
                        body.pop_back();
                    } else {
                        if (nextHead == innerFood) {
                            generateOuter();
                        } else {
                            body.pop_back();
                        }
                    }
                }
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (gBackgroundTexture) SDL_RenderCopy(renderer, gBackgroundTexture, nullptr, nullptr);
        // Draw food
        if (mode == 1) {
            SDL_Rect r{ foodClassic.x, foodClassic.y, RECT_SIZE, RECT_SIZE };
            SDL_RenderCopy(renderer, nullptr, nullptr, nullptr);
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &r);
        } else {
            if (!hasInner) {
                SDL_Rect r{ outerFood.x, outerFood.y, RECT_SIZE, RECT_SIZE };
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                SDL_RenderFillRect(renderer, &r);
            } else {
                SDL_Rect r{ innerFood.x, innerFood.y, RECT_SIZE, RECT_SIZE };
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &r);
            }
        }
        // Draw snake
        SDL_Rect rect{0,0, RECT_SIZE, RECT_SIZE};
        if (!body.empty()) {
            rect.x = body[0].x; rect.y = body[0].y;
            double angle = 0;
            if      (direction.x > 0) angle = 90;
            else if (direction.x < 0) angle = 270;
            else if (direction.y > 0) angle = 180;
            SDL_RenderCopyEx(renderer, gHeadTexture, nullptr, &rect, angle, nullptr, SDL_FLIP_NONE);
        }
        for (size_t i = 1; i < body.size(); ++i) {
            rect.x = body[i].x; rect.y = body[i].y;
            SDL_RenderCopy(renderer, gBodyTexture, nullptr, &rect);
        }
        // Score
        int score = static_cast<int>(body.size()) - 1;
        string s = "Score: " + to_string(score);
        SDL_Color sc = {255,0,0};
        SDL_Texture* st = renderText(s.c_str(), font, sc, renderer);
        renderTexture(st, 10, 10, renderer);
        SDL_DestroyTexture(st);
        SDL_RenderPresent(renderer);

        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) SDL_Delay(frameDelay - frameTime);
    }

    // Game Over
    if (gameOver) {
        SDL_SetRenderDrawColor(renderer, 0,0,0,255);
        SDL_RenderClear(renderer);
        if (gBackgroundTexture) SDL_RenderCopy(renderer, gBackgroundTexture, nullptr, nullptr);
        SDL_Color co = {255,0,0};
        SDL_Texture* go = renderText("Game Over", font, co, renderer);
        renderTexture(go, 180, 375, renderer);
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(go);
        SDL_Delay(1000);
    }
}

int main(int argc, char* argv[]) {
    SDL_Window*   window   = initSDL(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    SDL_Renderer* renderer = createRenderer(window, SCREEN_WIDTH, SCREEN_HEIGHT);
    TTF_Font*     font     = loadFont("timesbd.ttf", 50);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    Mix_Music* gMusic = Mix_LoadMUS("assets/RunningAway.mp3");
    if (gMusic) Mix_PlayMusic(gMusic, -1);

    int mode = ShowMenu(renderer, font);
    if (mode == 0) {
        quitSDL(window, renderer);
        return 0;
    }
    CoreGame(renderer, window, font, mode);

    if (gMusic) Mix_FreeMusic(gMusic);
    Mix_CloseAudio();
    quitSDL(window, renderer);
    return 0;
}
