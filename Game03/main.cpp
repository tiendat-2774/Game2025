#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <bits/stdc++.h>
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

enum { MENU_CLASSIC = 1, MENU_TWOLAYER, MENU_QUIT, MENU_RESUME };

struct Point {
    int x, y;
    Point(int x_val, int y_val) : x(x_val), y(y_val) {}  // Constructor
    Point() : x(0), y(0) {} // Default constructor (optional but useful)
    bool operator==(const Point& o) const { return x==o.x && y==o.y; }
};

// Global textures
SDL_Texture* gHeadTexture       = nullptr;
SDL_Texture* gBodyTexture       = nullptr;
SDL_Texture* gFoodTexture       = nullptr;
SDL_Texture* gFakeTexture       = nullptr;
SDL_Texture* gBackgroundTexture = nullptr;

// Game State Variables (Global for now - consider a GameState struct later)
vector<Point> savedSnake;
Point savedDir, savedNextDir, savedFood, savedFake;
bool savedTwoLayer = false;
bool savedFakeConsumed = false;
int savedScore = 0;
Uint32 savedLast = 0;
Uint32 savedInterval = 150;

// Music and Sound Effects
Mix_Music* gMusic = nullptr;
Mix_Chunk* gEatSound = nullptr;
Mix_Chunk* gLoseSound = nullptr;

// Prototypes
void QuitSDL(SDL_Window* w, SDL_Renderer* r);
bool InitSDL(SDL_Window*& w, SDL_Renderer*& r);
int ShowMenu(SDL_Renderer* ren, TTF_Font* font, bool canResume = false);
void CoreGame(SDL_Renderer* ren, SDL_Window* win, TTF_Font* font, int mode, bool resuming = false);
bool LoadMedia();
void FreeMedia();

int main(int argc, char* argv[]) {
    srand((unsigned)time(nullptr));
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!InitSDL(window, renderer)) return 1;

    if (!LoadMedia()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed to load media!", window);
        QuitSDL(window, renderer);
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("timesbd.ttf", 24);
    if (!font) {
        cerr << "TTF_OpenFont Error: " << TTF_GetError() << endl;
        QuitSDL(window, renderer);
        return 1;
    }

    gBackgroundTexture = loadTexture("background.jpg", renderer);
    if (!gBackgroundTexture) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed to load background.jpg", window);
        QuitSDL(window, renderer);
        return 1;
    }

    // Start the music
    if (Mix_PlayingMusic() == 0) {
        Mix_PlayMusic(gMusic, -1); // -1 means loop indefinitely
    }

    int mode;
    bool canResume = false;
    while ((mode = ShowMenu(renderer, font, canResume)) != MENU_QUIT) {
        if (mode == MENU_RESUME) {
            CoreGame(renderer, window, font, savedTwoLayer ? MENU_TWOLAYER : MENU_CLASSIC, true);
        } else {
            CoreGame(renderer, window, font, mode);
        }
        canResume = !savedSnake.empty(); // Can resume if a game was played
    }

    FreeMedia();
    QuitSDL(window, renderer);
    return 0;
}

bool InitSDL(SDL_Window*& w, SDL_Renderer*& r) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        cerr << "SDL_Init Error: " << SDL_GetError() << endl;
        return false;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        cerr << "IMG_Init Error: " << IMG_GetError() << endl;
        SDL_Quit();
        return false;
    }
    if (Mix_Init(MIX_INIT_OGG | MIX_INIT_MP3) != (MIX_INIT_OGG | MIX_INIT_MP3)) {  // Initialize both OGG and MP3
        cerr << "Mix_Init Error: " << Mix_GetError() << endl;
        IMG_Quit(); SDL_Quit();
        return false;
    }

    // Open the audio device.  Important to call this *after* Mix_Init
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        cerr << "Mix_OpenAudio Error: " << Mix_GetError() << endl;
        Mix_Quit(); IMG_Quit(); SDL_Quit();
        return false;
    }

    if (TTF_Init() != 0) {
        cerr << "TTF_Init Error: " << TTF_GetError() << endl;
        Mix_Quit(); IMG_Quit(); SDL_Quit();
        return false;
    }

    w = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    r = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!w || !r) {
        cerr << "SDL Window/Renderer Error: " << SDL_GetError() << endl;
        if (r) SDL_DestroyRenderer(r);
        if (w) SDL_DestroyWindow(w);
        TTF_Quit(); IMG_Quit(); Mix_Quit(); SDL_Quit();
        return false;
    }
    return true;
}

void QuitSDL(SDL_Window* w, SDL_Renderer* r) {
    FreeMedia(); // Free music and sound effects
    if (gHeadTexture) SDL_DestroyTexture(gHeadTexture);
    if (gBodyTexture) SDL_DestroyTexture(gBodyTexture);
    if (gFoodTexture) SDL_DestroyTexture(gFoodTexture);
    if (gFakeTexture) SDL_DestroyTexture(gFakeTexture);
    if (gBackgroundTexture) SDL_DestroyTexture(gBackgroundTexture);
    if (r) SDL_DestroyRenderer(r);
    if (w) SDL_DestroyWindow(w);
    TTF_Quit(); IMG_Quit(); Mix_Quit(); SDL_Quit();
}

int ShowMenu(SDL_Renderer* ren, TTF_Font* font, bool canResume) {
    vector<string> opts = {"Classic Mode","Two-Layer Mode"};
    if (canResume) {
        opts.push_back("Resume Game");
    }
    opts.push_back("Quit");

    int sel = 0;
    SDL_Event e;
    vector<SDL_Texture*> tex(opts.size());
    vector<SDL_Rect> dst(opts.size());

    for (size_t i=0;i<opts.size();++i) {
        tex[i]=renderText(opts[i].c_str(), font, {255,255,255}, ren);
        SDL_QueryTexture(tex[i], nullptr,nullptr, &dst[i].w,&dst[i].h);
        dst[i].x=(SCREEN_WIDTH-dst[i].w)/2; dst[i].y=300+i*60;
    }

    while (true) {
        while (SDL_PollEvent(&e)) {
            if (e.type==SDL_QUIT) {
                for (auto t:tex) SDL_DestroyTexture(t);
                return MENU_QUIT;
            }
            if (e.type==SDL_KEYDOWN) {
                if (e.key.keysym.sym==SDLK_UP||e.key.keysym.sym==SDLK_w) sel=(sel-1+opts.size())%opts.size();
                if (e.key.keysym.sym==SDLK_DOWN||e.key.keysym.sym==SDLK_s) sel=(sel+1)%opts.size();
                if (e.key.keysym.sym==SDLK_RETURN||e.key.keysym.sym==SDLK_KP_ENTER) {
                    for (auto t:tex) SDL_DestroyTexture(t);
                    if (canResume && sel == opts.size() - 2) { // "Resume Game" selected
                        return MENU_RESUME;
                    } else {
                        return sel + 1; // Classic, Two-Layer, or Quit
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(ren,0,0,0,255); SDL_RenderClear(ren);
        for (size_t i=0;i<opts.size();++i) {
            SDL_Color c=(i==sel?SDL_Color{255,0,0}:SDL_Color{255,255,255});
            SDL_DestroyTexture(tex[i]);  // Destroy the old texture before creating a new one
            tex[i]=renderText(opts[i].c_str(), font, c, ren);
            SDL_QueryTexture(tex[i], nullptr,nullptr, &dst[i].w,&dst[i].h);
            dst[i].x=(SCREEN_WIDTH-dst[i].w)/2;
            SDL_RenderCopy(ren, tex[i], nullptr, &dst[i]);
        }
        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

}

void CoreGame(SDL_Renderer* ren, SDL_Window* win, TTF_Font* font, int mode, bool resuming) {
    bool twoLayer = (mode==MENU_TWOLAYER);
    savedTwoLayer = twoLayer; // Save the game mode

    // Load textures
    gHeadTexture=loadTexture("head.png",ren);
    gBodyTexture=loadTexture("body.png",ren);
    gFoodTexture=loadTexture("food.png",ren);
    if (twoLayer) gFakeTexture=loadTexture("fake.png",ren);
    if (!gHeadTexture||!gBodyTexture||!gFoodTexture||(twoLayer&&!gFakeTexture)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Error","Missing textures",win); return;
    }

    vector<Point> &snake = resuming ? savedSnake : savedSnake = {}; // Use/reset saved snake
    Point &dir = resuming ? savedDir : savedDir = {RECT_SIZE, 0};
    Point &nextDir = resuming ? savedNextDir : savedNextDir = {dir.x, dir.y};
    Point &food = resuming ? savedFood : savedFood;
    Point &fake = resuming ? savedFake : savedFake;
    bool &fakeConsumed = resuming ? savedFakeConsumed : savedFakeConsumed = false;
    int &score = resuming ? savedScore : savedScore = 0;
    Uint32 &last = resuming ? savedLast : savedLast = SDL_GetTicks();
    Uint32 &interval = resuming ? savedInterval : savedInterval = 150;

    // Added variable to track if the fake apple has been passed through
    bool fakePassed = false;

    // Flag to indicate if the fake apple has been converted to food
    bool fakeIsFood = false;

    if (!resuming) {
        snake.emplace_back(SCREEN_WIDTH/2/RECT_SIZE*RECT_SIZE, SCREEN_HEIGHT/2/RECT_SIZE*RECT_SIZE);
    }


    auto place=[&](Point &p){ do{ p.x=(rand()%(SCREEN_WIDTH/RECT_SIZE))*RECT_SIZE;
                                   p.y=(rand()%(SCREEN_HEIGHT/RECT_SIZE))*RECT_SIZE;
                              }while(find(snake.begin(),snake.end(),p)!=snake.end() || (twoLayer && p == fake)); };

    auto placeFake = [&](Point &p){ do{ p.x=(rand()%(SCREEN_WIDTH/RECT_SIZE))*RECT_SIZE;
                                   p.y=(rand()%(SCREEN_HEIGHT/RECT_SIZE))*RECT_SIZE;
                              }while(find(snake.begin(),snake.end(),p)!=snake.end() || p == food); };

    if (!resuming) {
        place(food);
        if(twoLayer) placeFake(fake);
    }

    bool running=true;
    SDL_Event e;

    // Score Variables
    SDL_Color textColor = {255, 255, 255}; // White color for the text
    SDL_Texture* scoreTexture = nullptr;
    SDL_Rect scoreRect;

    // Function to update the score texture
    auto updateScore = [&](int newScore) {
        score = newScore;
        string scoreText = "Score: " + to_string(score);
        SDL_DestroyTexture(scoreTexture); // Destroy the old texture
        scoreTexture = renderText(scoreText.c_str(), font, textColor, ren);
        SDL_QueryTexture(scoreTexture, nullptr, nullptr, &scoreRect.w, &scoreRect.h);
        scoreRect.x = 10; // Position of the score text
        scoreRect.y = 10;
    };

    updateScore(score); // Initialize/Update the score

    while (running) {
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT){running=false;break;}
            if(e.type==SDL_KEYDOWN){
                if(e.key.keysym.sym==SDLK_ESCAPE){
                    // Save Game State
                    savedSnake = snake;
                    savedDir = dir;
                    savedNextDir = nextDir;
                    savedFood = food;
                    savedFake = fake;
                    savedFakeConsumed = fakeConsumed;
                    savedScore = score;
                    savedLast = last;
                    savedInterval = interval;
                    running=false;
                    break;
                }
                if((e.key.keysym.sym==SDLK_UP||e.key.keysym.sym==SDLK_w)&&dir.y==0) nextDir = Point(0,-RECT_SIZE);
                if((e.key.keysym.sym==SDLK_DOWN||e.key.keysym.sym==SDLK_s)&&dir.y==0) nextDir = Point(0,RECT_SIZE);
                if((e.key.keysym.sym==SDLK_LEFT||e.key.keysym.sym==SDLK_a)&&dir.x==0) nextDir = Point(-RECT_SIZE,0);
                if((e.key.keysym.sym==SDLK_RIGHT||e.key.keysym.sym==SDLK_d)&&dir.x==0) nextDir = Point(RECT_SIZE,0);
            }
        }
        Uint32 now=SDL_GetTicks();
        if(now-last>=interval){
            last=now;
            dir = Point(nextDir.x, nextDir.y); // Update dir

            Point head(snake.front().x + dir.x, snake.front().y + dir.y);  // Use constructor
            // boundary check
            if(head.x<0||head.x>=SCREEN_WIDTH||head.y<0||head.y>=SCREEN_HEIGHT)
            {
                Mix_PlayChannel(-1, gLoseSound, 0); // Play lose sound
                running=false;
                break;
            }
            if(find(snake.begin(),snake.end(),head)!=snake.end())
            {
                Mix_PlayChannel(-1, gLoseSound, 0); // Play lose sound
                running=false;
                break;
            }

            snake.insert(snake.begin(),head);
            if(head==food){
                place(food);
                updateScore(score + 10); // Increase score when eating food
                Mix_PlayChannel(-1, gEatSound, 0); // Play eat sound
            }
            else if(twoLayer){
                if(head==fake && !fakePassed){
                   // First pass through the fake apple, convert it to food
                    fakePassed = true;
                    fakeIsFood = true;  // The fake is now a food
                }
                else if(head == fake && fakePassed){
                    // Second pass through the (now real) apple
                    placeFake(fake); // Move the fake apple after eating it
                    fakePassed = false; // Reset fakePassed for the next cycle.
                    fakeIsFood = false; //reset flag
                    updateScore(score + 20);
                    Mix_PlayChannel(-1, gEatSound, 0);
                    placeFake(fake);

                }
                else {
                    snake.pop_back();
                }

            }
            else { snake.pop_back(); }
        }
        SDL_SetRenderDrawColor(ren,0,0,0,255);SDL_RenderClear(ren);
        SDL_RenderCopy(ren,gBackgroundTexture,nullptr,nullptr);
        SDL_Rect rf{food.x,food.y,RECT_SIZE,RECT_SIZE}; SDL_RenderCopy(ren,gFoodTexture,nullptr,&rf);
        if(twoLayer){
            SDL_Rect rfa{fake.x,fake.y,RECT_SIZE,RECT_SIZE};
            SDL_RenderCopy(ren,(fakeIsFood ? gFoodTexture : gFakeTexture),nullptr,&rfa);
        }
        for(size_t i=0;i<snake.size();++i){ SDL_Rect rs{snake[i].x,snake[i].y,RECT_SIZE,RECT_SIZE}; SDL_RenderCopy(ren,i==0?gHeadTexture:gBodyTexture,nullptr,&rs);}

        // Render the score
        SDL_RenderCopy(ren, scoreTexture, nullptr, &scoreRect);

        SDL_RenderPresent(ren); SDL_Delay(16);
    }
    // Clean up the savedSnake if the game ended (not paused).
    if (running == false)
        savedSnake.clear();

    // Clean up score texture
    SDL_DestroyTexture(scoreTexture);

    SDL_DestroyTexture(gHeadTexture); SDL_DestroyTexture(gBodyTexture);
    SDL_DestroyTexture(gFoodTexture);
    if(twoLayer) SDL_DestroyTexture(gFakeTexture);
}

bool LoadMedia() {
    bool success = true;

    // Load music
    gMusic = loadMusic("assets/RunningAway.mp3");
    if (gMusic == nullptr) {
        cerr << "Failed to load music! SDL_mixer Error: " << Mix_GetError() << endl;
        success = false;
    }

    // Load eat sound effect
    gEatSound = loadSound("assets/eating.wav");
    if (gEatSound == nullptr) {
        cerr << "Failed to load eat sound effect! SDL_mixer Error: " << Mix_GetError() << endl;
        success = false;
    }

    // Load lose sound effect
    gLoseSound = loadSound("assets/lose.wav");
    if (gLoseSound == nullptr) {
        cerr << "Failed to load lose sound effect! SDL_mixer Error: " << Mix_GetError() << endl;
        success = false;
    }

    return success;
}

void FreeMedia() {
    // Free the music
    if (gMusic != nullptr) {
        Mix_FreeMusic(gMusic);
        gMusic = nullptr;
    }

    // Free the sound effects
    if (gEatSound != nullptr) {
        Mix_FreeChunk(gEatSound);
        gEatSound = nullptr;
    }
    if (gLoseSound != nullptr) {
        Mix_FreeChunk(gLoseSound);
        gLoseSound = nullptr;
    }
     Mix_Quit();
}
