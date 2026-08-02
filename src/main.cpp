#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <vector>

struct bird {
    int currentFrame;
    float x, y;
	float width, height;
    float velocity, gravity, lift;
};

SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    return texture;
}

int main(int argc, char* argv[]) {
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
    SDL_Texture* bgTexture = loadTexture(renderer, "assets/background.png");
    SDL_FRect bgRect;
    bgRect.x = 0.0f;
    bgRect.y = 0.0f;
    bgRect.w = 1000.0f;
    bgRect.h = 700.0f;

    std::vector<SDL_Texture*> birdFrames;
    birdFrames.push_back(loadTexture(renderer, "assets/tom1.png"));
    birdFrames.push_back(loadTexture(renderer, "assets/tom2.png"));
    birdFrames.push_back(loadTexture(renderer, "assets/tom3.png"));

    bird tom;
    tom.currentFrame = 0;
    tom.x = 200.0f;
    tom.y = 150.0f;
    tom.width = 220.0f;
    tom.height = 210.0f;
    tom.velocity = 0.0f;
    tom.gravity = 900.0f;
    tom.lift = -400.0f;

	int currentFrame = 0;
    Uint64 lastFrameTime = SDL_GetTicks();
    const Uint64 frameDelay = 100;
   
    Uint64 lastTime = SDL_GetTicks();

    bool isRunning = true;
    SDL_Event event;

    while (isRunning) {
        Uint64 currentTimePhysics = SDL_GetTicks();
        float deltaTime = (currentTimePhysics - lastTime) / 1000.0f;
        lastTime = currentTimePhysics;

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

        if (tom.y < 0.0f) {
            tom.y = 0.0f;
            tom.velocity = 0.0f;
        }
        if (tom.y > 700.0f - tom.height) {
            tom.y = 700.0f - tom.height;
            tom.velocity = 0.0f;
        }

        Uint64 currentTime = SDL_GetTicks();
        if (currentTime - lastFrameTime >= frameDelay) {
            tom.currentFrame = (tom.currentFrame + 1) % birdFrames.size();
            lastFrameTime = currentTime;
        }

        SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
        SDL_RenderClear(renderer);

        if (bgTexture) {
            SDL_RenderTexture(renderer, bgTexture, nullptr, &bgRect);
        }

        if (!birdFrames.empty() && birdFrames[tom.currentFrame]) {
            SDL_FRect renderRect;
            renderRect.x = tom.x;
            renderRect.y = tom.y;
            renderRect.w = tom.width;
            renderRect.h = tom.height; 
            SDL_RenderTexture(renderer, birdFrames[tom.currentFrame], nullptr, &renderRect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
   
    if (bgTexture) SDL_DestroyTexture(bgTexture);

    for (SDL_Texture* tex : birdFrames) {
        if (tex) SDL_DestroyTexture(tex);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}