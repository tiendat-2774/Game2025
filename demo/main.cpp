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
SDL_Texture* gFoodTexture       = nullptr;
SDL_Texture* gBackgroundTexture = nullptr;

struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
};

vector<Point> body;
Point food;

void quitSDL(SDL_Window* window, SDL_Renderer* renderer) {
    if (gHeadTexture)       SDL_DestroyTexture(gHeadTexture);
    if (gBodyTexture)       SDL_DestroyTexture(gBodyTexture);
    if (gFoodTexture)       SDL_DestroyTexture(gFoodTexture);
    if (gBackgroundTexture) SDL_DestroyTexture(gBackgroundTexture);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    Mix_Quit();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

void drawSnake(SDL_Renderer* renderer, const Point& direction) {
    SDL_Rect rect {0,0, RECT_SIZE, RECT_SIZE};
    // Head
    if (!body.empty()) {
        rect.x = body[0].x;
        rect.y = body[0].y;
        double angle = 0.0;
        if      (direction.x > 0) angle = 90;
        else if (direction.x < 0) angle = 270;
        else if (direction.y > 0) angle = 180;
        SDL_RenderCopyEx(renderer, gHeadTexture, nullptr, &rect, angle, nullptr, SDL_FLIP_NONE);
    }
    // Body
    for (size_t i = 1; i < body.size(); ++i) {
        rect.x = body[i].x;
        rect.y = body[i].y;
        SDL_RenderCopy(renderer, gBodyTexture, nullptr, &rect);
    }
}

void drawFood(SDL_Renderer* renderer) {
    if (!gFoodTexture) {
        SDL_Rect rect { food.x, food.y, RECT_SIZE, RECT_SIZE };
        SDL_SetRenderDrawColor(renderer, 255,0,0,255);
        SDL_RenderFillRect(renderer, &rect);
        return;
    }
    SDL_Rect rect { food.x, food.y, RECT_SIZE, RECT_SIZE };
    SDL_RenderCopy(renderer, gFoodTexture, nullptr, &rect);
}

void generateFood() {
    bool onBody;
    do {
        onBody = false;
        int fx = rand() % (SCREEN_WIDTH / RECT_SIZE);
        int fy = rand() % (SCREEN_HEIGHT / RECT_SIZE);
        food = { fx * RECT_SIZE, fy * RECT_SIZE };
        for (const auto& p : body) {
            if (food == p) { onBody = true; break; }
        }
    } while (onBody);
}

void CoreGame(SDL_Renderer* renderer, SDL_Window* window, TTF_Font* font) {
    // Load textures once
    gHeadTexture       = loadTexture("head.png", renderer);
    gBodyTexture       = loadTexture("body.png", renderer);
    gFoodTexture       = loadTexture("food.png", renderer);
    gBackgroundTexture = loadTexture("SnakeGameBackGround.jpg", renderer);

    // Check required textures
    if (!gHeadTexture || !gBodyTexture) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
            "Cannot load snake head/body textures.", window);
        return;
    }

    bool quit = false;
    bool gameOver = false;
    bool paused = false;

    // Initial snake and food
    body.clear();
    int startX = (SCREEN_WIDTH / RECT_SIZE / 2) * RECT_SIZE;
    int startY = (SCREEN_HEIGHT / RECT_SIZE / 2) * RECT_SIZE;
    body.push_back({startX, startY});
    generateFood();

    Point direction      = { RECT_SIZE, 0 };
    Point next_direction = direction;

    Uint32 lastMoveTime = SDL_GetTicks();
    const Uint32 moveInterval = 150;
    const Uint32 frameDelay   = 1000 / 60;

    SDL_Event e;

    while (!quit && !gameOver) {
        Uint32 frameStart = SDL_GetTicks();

        // 1. Handle events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        paused = !paused;
                        break;
                    case SDLK_w: case SDLK_UP:
                        if (!paused && direction.y == 0) next_direction = {0, -RECT_SIZE};
                        break;
                    case SDLK_s: case SDLK_DOWN:
                        if (!paused && direction.y == 0) next_direction = {0, RECT_SIZE};
                        break;
                    case SDLK_a: case SDLK_LEFT:
                        if (!paused && direction.x == 0) next_direction = {-RECT_SIZE, 0};
                        break;
                    case SDLK_d: case SDLK_RIGHT:
                        if (!paused && direction.x == 0) next_direction = {RECT_SIZE, 0};
                        break;
                    default:
                        break;
                }
            }
        }

        if (paused) {
            // Render pause screen
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            SDL_RenderClear(renderer);
            if (gBackgroundTexture)
                SDL_RenderCopy(renderer, gBackgroundTexture, nullptr, nullptr);

            SDL_Color c = {255, 0, 0};
            SDL_Texture* pauseText = renderText("Press ESC to Resume", font, c, renderer);
            renderTexture(pauseText, 140, 350, renderer);
            SDL_RenderPresent(renderer);
            SDL_DestroyTexture(pauseText);

            SDL_Delay(100);
        } else {
            // 2. Update game logic
            Uint32 currentTime = SDL_GetTicks();
            if (currentTime - lastMoveTime >= moveInterval) {
                lastMoveTime = currentTime;
                direction = next_direction;
                Point head = body[0];
                Point next_head = { head.x + direction.x, head.y + direction.y };

                // Check collision with walls
                if (next_head.x < 0 || next_head.x >= SCREEN_WIDTH ||
                    next_head.y < 0 || next_head.y >= SCREEN_HEIGHT) {
                    gameOver = true;
                } else {
                    // Check self-collision
                    for (size_t i = 1; i < body.size(); ++i) {
                        if (next_head == body[i]) { gameOver = true; break; }
                    }
                }
                if (!gameOver) {
                    body.insert(body.begin(), next_head);
                    if (next_head == food) {
                        generateFood();
                    } else {
                        body.pop_back();
                    }
                }
            }

            // 3. Render
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            if (gBackgroundTexture)
                SDL_RenderCopy(renderer, gBackgroundTexture, nullptr, nullptr);

            // Draw food, snake, score
            drawFood(renderer);
            drawSnake(renderer, direction);

            int score = static_cast<int>(body.size()) - 1;
            string scoreText = "Score: " + to_string(score);
            SDL_Color sc = {255,0,0};
            SDL_Texture* scoreTexture = renderText(scoreText.c_str(), font, sc, renderer);
            renderTexture(scoreTexture, 10, 10, renderer);
            SDL_DestroyTexture(scoreTexture);

            SDL_RenderPresent(renderer);
        }

        // Frame rate cap
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) SDL_Delay(frameDelay - frameTime);
    }

    // Game Over screen
    if (gameOver) {
        SDL_SetRenderDrawColor(renderer, 0,0,0,255);
        SDL_RenderClear(renderer);
        if (gBackgroundTexture)
            SDL_RenderCopy(renderer, gBackgroundTexture, nullptr, nullptr);
        SDL_Color co = {255,0,0};
        SDL_Texture* goText = renderText("Game Over", font, co, renderer);
        renderTexture(goText, 180, 375, renderer);
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(goText);
        SDL_Delay(1000);
    }
}

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned int>(time(nullptr)));

    SDL_Window*   window   = initSDL(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    SDL_Renderer* renderer = createRenderer(window, SCREEN_WIDTH, SCREEN_HEIGHT);
    TTF_Font*     font     = loadFont("timesbd.ttf", 50);

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        SDL_Log("SDL_mixer could not initialize: %s", Mix_GetError());
    }
    Mix_Music* gMusic = Mix_LoadMUS("assets/RunningAway.mp3");
    if (!gMusic) SDL_Log("Failed to load music: %s", Mix_GetError());
    Mix_PlayMusic(gMusic, -1); // loop indefinitely

    CoreGame(renderer, window, font);

    // Cleanup audio
    if (gMusic) Mix_FreeMusic(gMusic);
    Mix_CloseAudio();

    quitSDL(window, renderer);
    return 0;
}


