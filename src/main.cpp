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
const float PIPE_WIDTH = 700.0f; // chiều rộng của pipe
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
    tom.x = 400.0f;
    tom.y = 150.0f;
    tom.width = 220.0f;
    tom.height = 210.0f;

    tom.velocity = 0.0f;
    tom.gravity = 900.0f;
    tom.lift = -400.0f;

    SDL_Texture* pipeTex = loadTexture(renderer, "assets/pipe.png");
    if (!pipeTex) {
        std::cout << "Không load được pipe texture!\n";
    }

	std::vector<pipe> pipes;
    
	float pipeTimer = 0.0f;
	
    Uint64 lastFrameTime = SDL_GetTicks();
    Uint64 lastTime = SDL_GetTicks();

    bool isRunning = true;
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
                if (event.key.key == SDLK_SPACE) {
                    tom.velocity = tom.lift;
                }
            }
        }

        tom.velocity += tom.gravity * deltaTime;
        tom.y += tom.velocity * deltaTime;
        // giới hạn biên
        if (tom.y < -80.0f) {
            tom.y = -80.0f;
            tom.velocity = 0.0f;
        }
        if (tom.y > 700.0f - tom.height) {
            tom.y = 700.0f - tom.height;
            tom.velocity = 0.0f;
        }

        Uint64 currentTime = SDL_GetTicks();
        if (currentTime - lastFrameTime >= 100) {
            tom.currentFrame = (tom.currentFrame + 1) % birdFrames.size();
            lastFrameTime = currentTime;
        }

        pipeTimer += deltaTime;
        if (pipeTimer >= PIPE_SPAWN_TIME) {
            pipeTimer -= PIPE_SPAWN_TIME;
            spawnPipe(pipes);
        }
		for (auto it = pipes.begin(); it != pipes.end(); ){
			it->x -= PIPE_SPEED * deltaTime;
			if (it->x + PIPE_WIDTH < 0) {
				it = pipes.erase(it);
			}
			else {
				++it;
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
            SDL_RenderTexture(renderer, birdFrames[tom.currentFrame], nullptr, &renderRect);
        }

        SDL_RenderPresent(renderer);
        
    }
   
    if (bgTexture) SDL_DestroyTexture(bgTexture);

    for (SDL_Texture* tex : birdFrames) {
        if (tex) SDL_DestroyTexture(tex);
    }
	SDL_DestroyTexture(pipeTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}