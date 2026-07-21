#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <vector>

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

    if (!SDL_CreateWindowAndRenderer("TomBay Game", 800, 600, 0, &window, &renderer)) {
        std::cout << "Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    std::vector<SDL_Texture*> birdFrames;
    birdFrames.push_back(loadTexture(renderer, "assets/tom1.png"));
    birdFrames.push_back(loadTexture(renderer, "assets/tom2.png"));
    birdFrames.push_back(loadTexture(renderer, "assets/tom3.png"));

    SDL_FRect birdRect;
    birdRect.x = 150.0f;
    birdRect.y = 100.0f;
    birdRect.w = 300.0f;
    birdRect.h = 210.0f;

    int currentFrame = 0;
    Uint64 lastFrameTime = SDL_GetTicks();
    const Uint64 frameDelay = 100;

    bool isRunning = true;
    SDL_Event event;

    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                isRunning = false;
            }
        }

        Uint64 currentTime = SDL_GetTicks();
        if (currentTime - lastFrameTime >= frameDelay) {
            currentFrame = (currentFrame + 1) % birdFrames.size();
            lastFrameTime = currentTime;
        }

        SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
        SDL_RenderClear(renderer);

        if (!birdFrames.empty() && birdFrames[currentFrame]) {
            SDL_RenderTexture(renderer, birdFrames[currentFrame], nullptr, &birdRect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    for (SDL_Texture* tex : birdFrames) {
        if (tex) SDL_DestroyTexture(tex);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}