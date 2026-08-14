#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <fstream>

using namespace std;

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
    bool passed;
};

const float PIPE_SPEED = 200.0f;     // Tốc độ di chuyển của cột
const float PIPE_GAP_SIZE = 180.0f;  // Khoảng cách giữa cột trên và dưới
const float PIPE_WIDTH = 200.0f;     // Bề rộng file ảnh cột
const float PIPE_HEIGHT = 500.0f;    // Chiều cao file ảnh cột
const float PIPE_SPAWN_TIME = 1.75f; // Thời gian sinh cột mới (giây)

const float GROUND_Y = 615.0f;       // Tọa độ Y của mặt đất

// Lề Hitbox cột (Trừ khoảng trống thừa ở 2 bên và miệng cột)
const float PIPE_VISIBLE_LEFT = 70.0f;
const float PIPE_VISIBLE_RIGHT = 130.0f;
const float PIPE_MARGIN_Y = 8.0f;   // Trừ lề không khí ở miệng cột trên/dưới

// Lề Hitbox của Tom (Mở rộng hợp lý & tránh quẹt góc khi nghiêng)
const float TOM_HIT_X = 0.20f;
const float TOM_HIT_Y = 0.20f;
const float TOM_HIT_W = 0.63f;
const float TOM_HIT_H = 0.63f;

SDL_FRect getTomHitbox(const bird& tom) {
    return {
        tom.x + tom.width * TOM_HIT_X,
        tom.y + tom.height * TOM_HIT_Y,
        tom.width * TOM_HIT_W,
        tom.height * TOM_HIT_H
    };
}

SDL_FRect getTopPipeHitbox(const pipe& p) {
    float topPipeY = p.gapY - p.gapSize / 2.0f - p.height;
    return {
        p.x + PIPE_VISIBLE_LEFT,
        topPipeY,
        PIPE_VISIBLE_RIGHT - PIPE_VISIBLE_LEFT,
        p.height - PIPE_MARGIN_Y
    };
}

SDL_FRect getBottomPipeHitbox(const pipe& p) {
    float bottomPipeY = p.gapY + p.gapSize / 2.0f;
    return {
        p.x + PIPE_VISIBLE_LEFT,
        bottomPipeY + PIPE_MARGIN_Y,
        PIPE_VISIBLE_RIGHT - PIPE_VISIBLE_LEFT,
        p.height - PIPE_MARGIN_Y
    };
}

int loadBestScore() {
    ifstream file("assets/best_score.txt");
    int bs = 0;
    if (file.is_open()) {
        file >> bs;
        file.close();
    }
    return bs;
}

void saveBestScore(int bs) {
    ofstream file("assets/best_score.txt");
    if (file.is_open()) {
        file << bs;
        file.close();
    }
}

void spawnPipe(std::vector<pipe>& pipes) {
    pipe p;
    p.x = 1000.0f;
    p.width = PIPE_WIDTH;
    p.height = PIPE_HEIGHT;
    p.gapSize = PIPE_GAP_SIZE;
	p.passed = false;

    const float MAX_PLAY_HEIGHT = GROUND_Y;
    float minY = 50.0f + p.gapSize / 2.0f;
    float maxY = MAX_PLAY_HEIGHT - 50.0f - p.gapSize / 2.0f;
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

bool checkGround(bird& tom, float groundY) {
    float groundLevel = groundY - tom.height;
    if (tom.y >= groundLevel) {
        tom.y = groundLevel;
        tom.velocity = 0.0f;
        return true;
    }
    return false;
}

bool isButtonClicked(float mouseX, float mouseY, const SDL_FRect& buttonRect) {
    return (mouseX >= buttonRect.x && mouseX <= buttonRect.x + buttonRect.w &&
        mouseY >= buttonRect.y && mouseY <= buttonRect.y + buttonRect.h);
}

void resetGame(bird& tom, std::vector<pipe>& pipes, float& pipeTimer, bool& isGameOver, bool& isGameStarted, int& score) {
    tom.y = 300.0f;
    tom.velocity = 0.0f;
    pipes.clear();
    pipeTimer = 0.0f;
    isGameOver = false;
    isGameStarted = false;
    score = 0;
}

bool checkAABB(const SDL_FRect& a, const SDL_FRect& b) {
    return (a.x < b.x + b.w &&
        a.x + a.w > b.x &&
        a.y < b.y + b.h &&
        a.y + a.h > b.y);
}

bool checkPipeCollision(const bird& tom, const std::vector<pipe>& pipes) {
    SDL_FRect tomHitbox = getTomHitbox(tom);

    for (const auto& p : pipes) {
        SDL_FRect topPipeHitbox = getTopPipeHitbox(p);
        SDL_FRect bottomPipeHitbox = getBottomPipeHitbox(p);

        if (checkAABB(tomHitbox, topPipeHitbox) || checkAABB(tomHitbox, bottomPipeHitbox)) {
            return true;
        }
    }
    return false;
}

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned int>(time(nullptr)));

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
    SDL_FRect bgRect = { 0.0f, 0.0f, 1000.0f, 700.0f };

    SDL_Texture* datTex = loadTexture(renderer, "assets/dat.png");
    SDL_FRect datRect = { 0.0f, 0.0f, 1000.0f, 700.0f };

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
    SDL_FRect gameOverRect = { -50.0f, 20.0f, 1000.0f, 700.0f };

    SDL_FRect restartBtnRect = { 570.0f, 370.0f, 80.0f, 57.0f };

    SDL_Texture* soundOnTex = loadTexture(renderer, "assets/sound_on.png");
    SDL_Texture* soundOffTex = loadTexture(renderer, "assets/sound_off.png");
    SDL_Texture* topRestartTex = loadTexture(renderer, "assets/restart.png");

    bool isMuted = false;

    SDL_FRect soundBtnRect = { 950.0f, 10.0f, 45.0f, 45.0f };
    SDL_FRect topRestartBtnRect = { 900.0f, 10.0f, 45.0f, 45.0f };

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
    bool isGameStarted = false;
    int score = 0;
    int bestScore = loadBestScore();
    SDL_Event event;

    while (isRunning) {
        Uint64 now = SDL_GetTicks();
        float deltaTime = (now - lastTime) / 1000.0f;
        if (deltaTime > 0.05f) {
            deltaTime = 0.05f;
        }
        lastTime = now;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                isRunning = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_SPACE && !isGameOver) {
                    tom.velocity = tom.lift;
					isGameStarted = true;
                }
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (isButtonClicked(event.button.x, event.button.y, topRestartBtnRect)) {
                        resetGame(tom, pipes, pipeTimer, isGameOver, isGameStarted, score);
                    }
                    else if (isButtonClicked(event.button.x, event.button.y, soundBtnRect)) {
                        isMuted = !isMuted;
                    }
                    else if (isGameOver) {
                        if (isButtonClicked(event.button.x, event.button.y, restartBtnRect)) {
                            resetGame(tom, pipes, pipeTimer, isGameOver, isGameStarted, score);
                        }
                    }
                    else {
                        tom.velocity = tom.lift;
						isGameStarted = true;
                    }
                }
            }
        }

        if (!isGameOver && isGameStarted) {
            tom.velocity += tom.gravity * deltaTime;
            tom.y += tom.velocity * deltaTime;

            if (tom.y < 0.0f) {
                tom.y = 0.0f;
                tom.velocity = 0.0f;
            }

            for (auto it = pipes.begin(); it != pipes.end(); ) {
                it->x -= PIPE_SPEED * deltaTime;
                if (!it->passed && tom.x > it->x + PIPE_VISIBLE_RIGHT ) {
                    score++;
                    it->passed = true;
                    std::cout << "Score: " << score << "\n";
                }
                if (it->x + PIPE_WIDTH < 0) {
                    it = pipes.erase(it);
                }
                else {
                    ++it;
                }
            }

            if (checkGround(tom, GROUND_Y)) {
                cout << "DEAD: GROUND\n";
                isGameOver = true;
                if (score > bestScore) {
                    bestScore = score;
                    saveBestScore(bestScore);
                }
                cout << "Best Score: " << bestScore << "\n";
            }
            else if (checkPipeCollision(tom, pipes)) {
                cout << "DEAD: PIPE\n";
                isGameOver = true;
                if (score > bestScore) {
                    bestScore = score;
                    saveBestScore(bestScore);
                }
                cout << "Best Score: " << bestScore << "\n";
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
        }

        SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
        SDL_RenderClear(renderer);

        if (bgTexture) {
            SDL_RenderTexture(renderer, bgTexture, nullptr, &bgRect);
        }

        // Vẽ Cột
        for (const auto& p : pipes) {
            float topPipeY = p.gapY - p.gapSize / 2.0f - PIPE_HEIGHT;
            SDL_FRect topRect = { p.x, topPipeY, PIPE_WIDTH, PIPE_HEIGHT };
            SDL_RenderTextureRotated(renderer, pipeTex, nullptr, &topRect, 0.0, nullptr, SDL_FLIP_VERTICAL);

            float bottomPipeY = p.gapY + p.gapSize / 2.0f;
            SDL_FRect bottomRect = { p.x, bottomPipeY, PIPE_WIDTH, PIPE_HEIGHT };
            SDL_RenderTexture(renderer, pipeTex, nullptr, &bottomRect);
        }

        // Vẽ Đất
        if (datTex) {
            SDL_RenderTexture(renderer, datTex, nullptr, &datRect);
        }

        // Vẽ Tom
        if (!birdFrames.empty() && birdFrames[tom.currentFrame]) {
            SDL_FRect renderRect = { tom.x, tom.y, tom.width, tom.height };
            double angle = tom.velocity * 0.04;
            if (angle < -20.0) angle = -20.0;
            if (angle > 60.0)  angle = 60.0;
            SDL_RenderTextureRotated(renderer, birdFrames[tom.currentFrame], nullptr, &renderRect, angle, nullptr, SDL_FLIP_NONE);
        }

        // Vẽ Menu Game Over
        if (isGameOver && gameOverTexture) {
            SDL_RenderTexture(renderer, gameOverTexture, nullptr, &gameOverRect);
        }

        // Vẽ nút Restart góc trên
        if (topRestartTex) {
            SDL_RenderTexture(renderer, topRestartTex, nullptr, &topRestartBtnRect);
        }

        // Vẽ nút Âm thanh
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
    if (pipeTex) SDL_DestroyTexture(pipeTex);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}