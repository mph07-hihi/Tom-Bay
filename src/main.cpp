#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

struct bird {
    int currentFrame;
    float x, y;
	float width, height;
    float velocity, gravity, lift;
};

struct pipe {
    float x;
    float gapY;
    float gapSize;
	float width, height;
};
const float PIPE_SPEED = 200.0f; // tốc độ di chuyển của pipe
const float PIPE_GAP_SIZE = 180.0f; // khoảng cách giữa 2 pipe trên dưới 
const float PIPE_WIDTH = 200.0f; // chiều rộng của pipe
const float PIPE_HEIGHT = 500.0f; // chiều cao của pipe
const float PIPE_SPAWN_TIME = 1.75f; // 1.75s tạo 1 pipe 

void spawnPipe(std::vector<pipe>& pipes) {
    pipe p;
    p.x = 1000.0f;
    p.width = PIPE_WIDTH;
    p.height = PIPE_HEIGHT;
    p.gapSize = PIPE_GAP_SIZE;

    const float SCREEN_HEIGHT = 600.0f;
    float minY = 30.0f + p.gapSize / 2.0f;
    float maxY = SCREEN_HEIGHT - 30.0f - p.gapSize / 2.0f;
    p.gapY = minY + static_cast<float>(rand()) / RAND_MAX * (maxY - minY);

    pipes.push_back(p);
}


SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    return texture;
}
// Kiểm tra Tom chạm đất chưa
bool checkGround(bird& tom, float groundY) {
    float groundLevel = groundY - tom.height; 
    if (tom.y >= groundLevel) {
        tom.y = groundLevel;
        tom.velocity = 0.0f;
        return true;
    }
    return false;
}
// Kiểm tra tọa độ của con trỏ chuột click
bool isButtonClicked(float mouseX, float mouseY, const SDL_FRect& buttonRect) {
    return (mouseX >= buttonRect.x && mouseX <= buttonRect.x + buttonRect.w &&
        mouseY >= buttonRect.y && mouseY <= buttonRect.y + buttonRect.h);
}
// Hàm resetGame
void resetGame(bird& tom, std::vector<pipe>& pipes, float& pipeTimer, bool& isGameOver) {
    tom.y = 150.0f;
    tom.velocity = 0.0f;
    pipes.clear();
    pipeTimer = 0.0f;
    isGameOver = false;
}
//  Hàm kiểm tra 2 hình chữ nhật AABB có đè lên nhau hay không
bool checkAABB(const SDL_FRect& a, const SDL_FRect& b) {
    return (a.x < b.x + b.w &&
        a.x + a.w > b.x &&
        a.y < b.y + b.h &&
        a.y + a.h > b.y);
}

// Hàm kiểm tra va chạm 
bool checkPipeCollision(const bird& tom, const std::vector<pipe>& pipes) {
    SDL_FRect tomHitbox = {
        tom.x + tom.width * 0.15f,
        tom.y + tom.height * 0.15f,
        tom.width * 0.70f,
        tom.height * 0.70f
    };

    for (const auto& p : pipes) {
        float pipeHitWidth = p.width * 0.80f;
        float pipeHitX = p.x + p.width * 0.10f;
        // Khung cột trên
        float topPipeY = p.gapY - p.gapSize / 2.0f - PIPE_HEIGHT;
        SDL_FRect topRect = { pipeHitX, topPipeY, pipeHitWidth, PIPE_HEIGHT };
        // Khung cột dưới
        float bottomPipeY = p.gapY + p.gapSize / 2.0f;
        SDL_FRect bottomRect = { pipeHitX, bottomPipeY, pipeHitWidth, PIPE_HEIGHT };

        if (checkAABB(tomHitbox, topRect) || checkAABB(tomHitbox, bottomRect)) {
            return true;
        }
    }
    return false;
}

int main(int argc, char* argv[]) {
    srand (static_cast<unsigned int>(time(nullptr))); // random
    
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "Error: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!SDL_CreateWindowAndRenderer("TomBay Game", 1000, 700, 0, &window, &renderer)) {
        std::cout << "Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);

    SDL_Texture* bgTexture = loadTexture(renderer, "assets/background.png");
    SDL_FRect bgRect;
    bgRect.x = 0.0f;
    bgRect.y = 0.0f;
    bgRect.w = 1000.0f;
    bgRect.h = 700.0f;

    SDL_Texture* datTex = loadTexture(renderer, "assets/dat.png");
    SDL_FRect datRect;
    datRect.x = 0.0f;
    datRect.y = 0.0f;
    datRect.w = 1000.0f;
    datRect.h = 700.0f;

    std::vector<SDL_Texture*> birdFrames;
    birdFrames.push_back(loadTexture(renderer, "assets/tom1.png"));
    birdFrames.push_back(loadTexture(renderer, "assets/tom2.png"));
    birdFrames.push_back(loadTexture(renderer, "assets/tom3.png"));

    bird tom;
    tom.currentFrame = 0;
    tom.x = 350.0f;
    tom.y = 300.0f;
    tom.width = 70.0f;
    tom.height = 50.0f;

    tom.velocity = 0.0f;
    tom.gravity = 900.0f;
    tom.lift = -400.0f;

    SDL_Texture* gameOverTexture = loadTexture(renderer, "assets/menu.png");
    SDL_FRect gameOverRect;
    gameOverRect.w = 1000.0f;
    gameOverRect.h = 700.0f;
    gameOverRect.x = -50.0f;
    gameOverRect.y = 20.0f;
    // vị trí của nút restart
    SDL_FRect restartBtnRect;
    restartBtnRect.w = 80.0f;
    restartBtnRect.h = 57.0f;
    restartBtnRect.x = 570.0f;
    restartBtnRect.y = 370.0f;
    
    SDL_Texture* soundOnTex = loadTexture(renderer, "assets/sound_on.png");
    SDL_Texture* soundOffTex = loadTexture(renderer, "assets/sound_off.png");
    SDL_Texture* topRestartTex = loadTexture(renderer, "assets/restart.png");

    bool isMuted = false;

    SDL_FRect soundBtnRect;
    soundBtnRect.x = 950.0f;
    soundBtnRect.y = 10.0f;
    soundBtnRect.w = 45.0f;
    soundBtnRect.h = 45.0f;

    SDL_FRect topRestartBtnRect;
    topRestartBtnRect.x = 900.0f;
    topRestartBtnRect.y = 10.0f;
    topRestartBtnRect.w = 45.0f;
    topRestartBtnRect.h = 45.0f;

    SDL_Texture* pipeTex = loadTexture(renderer, "assets/pipe.png");
    if (!pipeTex) {
        std::cout << "Không load được pipe texture!\n";
    }

	std::vector<pipe> pipes;
    
	float pipeTimer = 0.0f;
	
    Uint64 lastFrameTime = SDL_GetTicks();
    const Uint64 frameDelay = 100;

    Uint64 lastTime = SDL_GetTicks();

    bool isRunning = true;
    bool isGameOver = false;
    SDL_Event event;

    while (isRunning) {
        Uint64 now = SDL_GetTicks();
        float deltaTime = (now - lastTime) / 1000.0f;
        lastTime = now;
        // sự kiện phím
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                isRunning = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_SPACE && !isGameOver) {
                    tom.velocity = tom.lift;
                }
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (isButtonClicked(event.button.x, event.button.y, topRestartBtnRect)) {
                        resetGame(tom, pipes, pipeTimer, isGameOver);
                    }
                    else if (isButtonClicked(event.button.x, event.button.y, soundBtnRect)) {
                        isMuted = !isMuted;
                    }
                    else if (isGameOver) {
                        if (isButtonClicked(event.button.x, event.button.y, restartBtnRect)) {
                            resetGame(tom, pipes, pipeTimer, isGameOver);
                        }
                    }
                    else {
                        tom.velocity = tom.lift;
                    }
                }
            }
        }
        if (!isGameOver) {
            tom.velocity += tom.gravity * deltaTime;
            tom.y += tom.velocity * deltaTime;

            if (tom.y < 0.0f) {
                tom.y = 0.0f;
                tom.velocity = 0.0f;
            }
            if (checkGround(tom, 615.0f)) {
                isGameOver = true;
            }

            if (checkPipeCollision(tom, pipes)) {
                isGameOver = true; 
            }

            Uint64 currentTime = SDL_GetTicks();
            if (currentTime - lastFrameTime >= frameDelay) {
                tom.currentFrame = (tom.currentFrame + 1) % birdFrames.size();
                lastFrameTime = currentTime;
            }
            pipeTimer += deltaTime;
            if (pipeTimer >= PIPE_SPAWN_TIME) {
                pipeTimer -= PIPE_SPAWN_TIME;
                spawnPipe(pipes);
            }

            for (auto it = pipes.begin(); it != pipes.end(); ) {
                it->x -= PIPE_SPEED * deltaTime;
                if (it->x + PIPE_WIDTH < 0) {
                    it = pipes.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
        SDL_RenderClear(renderer);

        if (bgTexture) {
            SDL_RenderTexture(renderer, bgTexture, nullptr, &bgRect);
        }
        // vẽ pipe
        for (const auto& p : pipes) {
			float topPipeY = p.gapY - p.gapSize / 2.0f - PIPE_HEIGHT;
            SDL_FRect topRect = { p.x, topPipeY, PIPE_WIDTH, PIPE_HEIGHT };
            SDL_RenderTextureRotated(renderer, pipeTex, nullptr, &topRect, 0.0, nullptr, SDL_FLIP_VERTICAL);
            float bottomPipeY = p.gapY + p.gapSize / 2.0f;
            SDL_FRect bottomRect = { p.x, bottomPipeY, PIPE_WIDTH, PIPE_HEIGHT };
            SDL_RenderTexture(renderer, pipeTex, nullptr, &bottomRect);
        }
        // vẽ đất
        if (datTex) {
            SDL_RenderTexture(renderer, datTex, nullptr, &datRect);
        }
        // vẽ Tom
        if (!birdFrames.empty() && birdFrames[tom.currentFrame]) {
            SDL_FRect renderRect;
            renderRect.x = tom.x;
            renderRect.y = tom.y;
            renderRect.w = tom.width;
            renderRect.h = tom.height; 
            double angle = tom.velocity * 0.04;
            if (angle < -20.0) angle = -20.0;
            if (angle > 60.0)  angle = 60.0;
            SDL_RenderTextureRotated(renderer, birdFrames[tom.currentFrame], nullptr, &renderRect, angle, nullptr, SDL_FLIP_NONE);
        }
        // vẽ menu
        if (isGameOver && gameOverTexture) {
            SDL_RenderTexture(renderer, gameOverTexture, nullptr, &gameOverRect);
        }
        // vẽ nút restart ở góc
        if (topRestartTex) {
            SDL_RenderTexture(renderer, topRestartTex, nullptr, &topRestartBtnRect);
        }
        // nút âm thanh
        SDL_Texture* currentSoundTex = isMuted ? soundOffTex : soundOnTex;
        if (currentSoundTex) {
            SDL_RenderTexture(renderer, currentSoundTex, nullptr, &soundBtnRect);
        }

        SDL_RenderPresent(renderer);
        
    }
   
    if (bgTexture) SDL_DestroyTexture(bgTexture);
    if (datTex) SDL_DestroyTexture(datTex);
    if (soundOnTex) SDL_DestroyTexture(soundOnTex);
    if (soundOffTex) SDL_DestroyTexture(soundOffTex);
    if (topRestartTex) SDL_DestroyTexture(topRestartTex);
    if (gameOverTexture) SDL_DestroyTexture(gameOverTexture);

    for (SDL_Texture* tex : birdFrames) {
        if (tex) SDL_DestroyTexture(tex);
    }
	SDL_DestroyTexture(pipeTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}