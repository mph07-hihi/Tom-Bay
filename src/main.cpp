#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>

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

    SDL_Texture* tom1 = loadTexture(renderer, "assets/tom1.png");

    SDL_FRect birdRect;
    birdRect.x = 150.0f;
    birdRect.y = 100.0f;
    birdRect.w = 300.0f;
    birdRect.h = 210.0f;

    bool isRunning = true;
    SDL_Event event;

    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                isRunning = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
        SDL_RenderClear(renderer);

        if (tom1) {
            SDL_RenderTexture(renderer, tom1, nullptr, &birdRect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(tom1);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}