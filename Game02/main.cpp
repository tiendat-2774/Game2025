#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include "SDL_utils.h"
#include <SDL_mixer.h>
#include "SDL-Mix.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <SDL_ttf.h>
#include "SDL_text.h"
using namespace std;
const int SCREEN_HEIGHT = 800;
const int SCREEN_WIDTH = 600;
const char* WINDOW_TITLE = "SDL Snake Game";
const int RECT_SIZE = 20;
SDL_Texture* gHeadTexture = nullptr;
SDL_Texture* gFoodTexture = nullptr;
SDL_Texture* gBodyTexture = nullptr;
SDL_Texture* gBackgroundTexture = nullptr;
void quitSDL(SDL_Window* window, SDL_Renderer* renderer) {
    if (gHeadTexture != nullptr) {
        SDL_DestroyTexture(gHeadTexture);
        gHeadTexture = nullptr;
    }
    if (gFoodTexture != nullptr) {
        SDL_DestroyTexture(gFoodTexture);
        gFoodTexture = nullptr;
    }
    if (gBodyTexture != nullptr) {
        SDL_DestroyTexture(gBodyTexture);
        gBodyTexture = nullptr;
    }
    if (gBackgroundTexture != nullptr) {
        SDL_DestroyTexture(gBackgroundTexture);
        gBackgroundTexture = nullptr;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    TTF_Quit();
    Mix_Quit();


}
struct Point {
    int x, y;
    bool operator==(const Point other) const {
        return x == other.x && y == other.y;
    }
};
vector<Point> body;
Point food;
void drawSnake(SDL_Renderer* renderer, const Point& currentDirection) {
    SDL_Rect destRect;
    destRect.w = RECT_SIZE;
    destRect.h = RECT_SIZE;
    if (!body.empty()) {
        destRect.x = body[0].x;
        destRect.y = body[0].y;

        // <<< THÊM VÀO: Tính toán góc xoay >>>
        double angle = 0.0; // Mặc định là 0 độ (hướng lên)
        if (currentDirection.x > 0) {        // Đi sang phải (RECT_SIZE, 0)
            angle = 90.0;
        } else if (currentDirection.x < 0) { // Đi sang trái (-RECT_SIZE, 0)
            angle = 270.0; // Hoặc -90.0
        } else if (currentDirection.y > 0) { // Đi xuống (0, RECT_SIZE)
            angle = 180.0;
        }
        SDL_RenderCopyEx(renderer, gHeadTexture, NULL, &destRect, angle, NULL, SDL_FLIP_NONE);
    }

    for (int i = 1; i < body.size(); ++i) {
        destRect.x = body[i].x;
        destRect.y = body[i].y;
        SDL_RenderCopy(renderer, gBodyTexture, NULL, &destRect); // Thân không cần xoay (trừ khi bạn muốn hiệu ứng phức tạp hơn)
    }
}

// Vẽ thức ăn (Giữ nguyên - vẫn vẽ hình chữ nhật đỏ)
// Vẽ thức ăn bằng ảnh quả táo
void drawFood(SDL_Renderer* renderer) {
    // Kiểm tra xem texture thức ăn đã được tải chưa
    if (gFoodTexture == nullptr) {
        // Nếu chưa tải được, vẽ tạm hình chữ nhật đỏ để biết vị trí
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, "Food texture not loaded, drawing red rectangle instead.");
        SDL_Rect rect;
        rect.x = food.x;
        rect.y = food.y;
        rect.w = RECT_SIZE;
        rect.h = RECT_SIZE;
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Màu đỏ
        SDL_RenderFillRect(renderer, &rect);
        return; // Thoát khỏi hàm sau khi vẽ hình chữ nhật
    }

    // Nếu texture đã tải, tiến hành vẽ ảnh
    SDL_Rect destRect; // Đổi tên biến để tránh nhầm lẫn nếu cần
    destRect.x = food.x;
    destRect.y = food.y;
    destRect.w = RECT_SIZE; // Kích thước vẽ mong muốn
    destRect.h = RECT_SIZE; // Kích thước vẽ mong muốn

    // Vẽ texture thức ăn vào vị trí và kích thước đã xác định
    SDL_RenderCopy(renderer, gFoodTexture, NULL, &destRect);
}

// --- Logic Game ---
// Tạo thức ăn ở vị trí ngẫu nhiên, không trùng với thân rắn (Giữ nguyên)
void generateFood() {
    bool onBody; // <<< SỬA LỖI >>> Đổi tên biến để rõ nghĩa hơn
    do {
        onBody = false;
        // Tạo tọa độ trên lưới rồi nhân với kích thước ô
        int food_X = rand() % (SCREEN_WIDTH / RECT_SIZE);
        int food_Y = rand() % (SCREEN_HEIGHT / RECT_SIZE);
        food = {food_X * RECT_SIZE, food_Y * RECT_SIZE};

        // Kiểm tra xem thức ăn có nằm trên thân rắn không
        for (const auto& p : body) {
            if (food == p) { // Sử dụng toán tử == đã định nghĩa
                onBody = true;
                break;
            }
        }
    } while (onBody);
}

void CoreGame(SDL_Renderer* renderer, SDL_Window* window,TTF_Font* font,Mix_Music* gMusic) {
// <<< THÊM VÀO >>> Tải texture cho rắn và nền
gHeadTexture = loadTexture("head.png", renderer);
gBodyTexture = loadTexture("body.png", renderer);
gFoodTexture = loadTexture("food.png", renderer);
gBackgroundTexture = loadTexture("SnakeGameBackGround.jpg", renderer); // Tải 1 lần
// <<< THÊM VÀO >>> Kiểm tra xem texture có tải thành công không
if (gHeadTexture == nullptr || gBodyTexture == nullptr) {
     SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                             "Texture Load Error",
                             "Could not load snake textures (head.jpg or body.jpg). Check file paths.",
                             window);
    // Không thoát ngay, game có thể vẫn chạy nhưng không thấy rắn
    // Hoặc có thể chọn thoát ở đây:
    // return;
}
 if (gBackgroundTexture == nullptr) {
     SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING,
                             "Texture Load Warning",
                             "Could not load background texture (SnakeGameBackGround.jpg).",
                             window);
    // Vẫn tiếp tục chạy với nền đen
}
 /*play(gMusic);
 if (gMusic != nullptr) Mix_FreeMusic( gMusic );*/


// Khởi tạo trạng thái ban đầu
body.clear();
int startX = (SCREEN_WIDTH / RECT_SIZE / 2) * RECT_SIZE; // Giữa màn hình (theo lưới)
int startY = (SCREEN_HEIGHT / RECT_SIZE / 2) * RECT_SIZE;
body.push_back({startX, startY}); // Đầu rắn
generateFood();                    // Tạo thức ăn đầu tiên

// Hướng di chuyển hiện tại (dx, dy)
Point direction = {RECT_SIZE, 0}; // Bắt đầu đi sang phải
// Hướng sẽ áp dụng ở lần di chuyển tới (để xử lý input không bị xung đột)
Point next_direction = direction;

bool quit = false;
bool gameOver = false;

Uint32 lastMoveTime = SDL_GetTicks(); // Thời điểm lần di chuyển cuối
const Uint32 moveInterval = 150;     // Tốc độ rắn (ms giữa các lần di chuyển)

SDL_Event e;

// Vòng lặp game chính
while (!quit && !gameOver) {
 /* <<< XÓA HOẶC GHI CHÚ >>> Không cần tải background trong vòng lặp nữa
 SDL_Texture* background = loadTexture("bikiniBottom.jpg", renderer);
 SDL_RenderCopy( renderer, background, NULL, NULL);
 SDL_RenderPresent(renderer);
 */
  play(gMusic);
 if (gMusic != nullptr) Mix_FreeMusic( gMusic );
    int currentScore = body.size() - 1;
    string scoreText = "Score: " + to_string(currentScore);
    // 1. Xử lý Input (Non-blocking)
    while (SDL_PollEvent(&e)) {

            // Lấy tất cả sự kiện đang chờ
        if (e.type == SDL_QUIT) {
            quit = true;
        } else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                // Chỉ cập nhật next_direction, và kiểm tra không cho quay đầu 180 độ
                case SDLK_w: case SDLK_UP:
                    if (direction.y == 0) { // Chỉ cho rẽ lên/xuống nếu đang không đi dọc
                       next_direction = {0, -RECT_SIZE};
                    }
                    break;
                case SDLK_s: case SDLK_DOWN:
                    if (direction.y == 0) {
                       next_direction = {0, RECT_SIZE};
                    }
                    break;
                case SDLK_a: case SDLK_LEFT:
                     if (direction.x == 0) { // Chỉ cho rẽ trái/phải nếu đang không đi ngang
                       next_direction = {-RECT_SIZE, 0};
                     }
                    break;
                case SDLK_d: case SDLK_RIGHT:
                     if (direction.x == 0) {
                        next_direction = {RECT_SIZE, 0};
                     }
                    break;
                case SDLK_ESCAPE: // Thêm nút Esc để thoát nhanh
                    SDL_Color Pause_Color= {255,0,0};
                SDL_Texture* MenuText = renderText("SPACE to Resume", font, Pause_Color,renderer);
              renderTexture(MenuText, 140,350,renderer);
              SDL_RenderPresent(renderer);
              bool _Space = false;
                while(! _Space)
                {
                while (SDL_PollEvent(&e))
                {

                    if(e.key.keysym.sym == SDLK_SPACE)
                    {
                        _Space=true;
                        break;
                    }
                }
            }
        }
        }
    }
    // Thoát vòng lặp chính nếu người dùng yêu cầu quit
    if (quit) break;

    // 2. Cập nhật trạng thái game (theo thời gian)
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastMoveTime >= moveInterval) {
        lastMoveTime = currentTime; // Đặt lại thời gian cho lần di chuyển tiếp theo
        // Áp dụng hướng mới từ input (nếu có)
        direction = next_direction;
        // Tính toán vị trí đầu mới
        Point head = body[0];
        Point next_head = {head.x + direction.x, head.y + direction.y};

        // ---- Kiểm tra va chạm ----
        // a) Va chạm biên
        if (next_head.x < 0 || next_head.x >= SCREEN_WIDTH ||
            next_head.y < 0 || next_head.y >= SCREEN_HEIGHT) {
            gameOver = true;
            continue; // Bỏ qua phần còn lại của bước cập nhật này
        }

        // b) Va chạm thân (kiểm tra từ phần tử thứ 1 trở đi)
        for (int i = 1; i < body.size(); ++i) {
            if (next_head == body[i]) {
                gameOver = true;
                break; // Thoát vòng lặp kiểm tra thân
            }
        }
         if (gameOver) continue; // Bỏ qua phần còn lại nếu đã game over

        // --- Di chuyển và xử lý ăn mồi ---
        body.insert(body.begin(), next_head); // Thêm đầu mới

        // Kiểm tra có ăn mồi không
        if (next_head == food) {
            // Ăn mồi: Không xóa đuôi (rắn dài ra), tạo mồi mới
            generateFood();
        } else {
            // Không ăn mồi: Xóa phần đuôi
            body.pop_back();
        }
    } // Kết thúc khối cập nhật theo thời gian

    // 3. Vẽ màn hình (thực hiện mỗi frame, độc lập với tốc độ di chuyển)
    // Xóa nền thành màu đen (nếu không có background) hoặc vẽ background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer); // Xóa màn hình trước

    // <<< THÊM VÀO >>> Vẽ background nếu tải thành công
    if (gBackgroundTexture != nullptr) {
        SDL_RenderCopy(renderer, gBackgroundTexture, NULL, NULL); // Vẽ toàn màn hình
    }
     /* <<< XÓA HOẶC GHI CHÚ >>> Đoạn code tải và vẽ background cũ trong vòng lặp
      SDL_Texture* background = loadTexture("SnakeGameBackGround.jpg", renderer);
      SDL_RenderCopy( renderer, background, NULL, NULL);
     */
    SDL_Color scoreColor= {255,0,0};
   SDL_Texture* scoreTexture = renderText(scoreText.c_str(), font, scoreColor, renderer);
   renderTexture(scoreTexture, 10, 10, renderer);
    // Vẽ thức ăn
    drawFood(renderer);

    // Vẽ rắn (bây giờ sẽ vẽ ảnh)
    drawSnake(renderer, direction);

    // Hiển thị những gì đã vẽ lên màn hình
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(scoreTexture);

} // Kết thúc vòng lặp game chính

// Xử lý sau khi game kết thúc (ví dụ: hiển thị thông báo)
if (gameOver) {
    // Có thể thêm màn hình "Game Over" ở đây
    SDL_Color color = {255, 0, 0, 0};
     SDL_Texture* _Text = renderText("Game Over", font, color,renderer);
     renderTexture(_Text, 180,375,renderer);
     SDL_RenderPresent(renderer);
    SDL_Delay(1000); // Chờ 1 giây trước khi đóng (giảm từ 2s)
}
}
int main (int argc, char* argv[]) {
    srand(time(0)); // Khởi tạo bộ sinh số ngẫu nhiên MỘT LẦN DUY NHẤT
    SDL_Window* window = initSDL(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    SDL_Renderer* renderer = createRenderer(window,SCREEN_WIDTH, SCREEN_HEIGHT);
    TTF_Font* font = loadFont("timesbd.ttf", 50);

    Mix_Music *gMusic = loadMusic("assets\\RunningAway.mp3");
    play(gMusic);

    CoreGame(renderer, window,font,gMusic); // Chạy vòng lặp game
    quitSDL(window, renderer); // Dọn dẹp tài nguyên SDL
}
