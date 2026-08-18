#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <fstream>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <cmath>

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

const float PIPE_SPEED = 200.0f;
const float PIPE_GAP_SIZE = 180.0f;
const float PIPE_WIDTH = 200.0f;
const float PIPE_HEIGHT = 500.0f;
const float PIPE_SPAWN_TIME = 1.75f;

const float GROUND_Y = 615.0f;

const float PIPE_VISIBLE_LEFT = 70.0f;
const float PIPE_VISIBLE_RIGHT = 130.0f;
const float PIPE_MARGIN_Y = 8.0f;

const float TOM_START_X = 350.0f;
const float TOM_START_Y = 300.0f;
const float TOM_WIDTH = 70.0f;
const float TOM_HEIGHT = 50.0f;
const float TOM_GRAVITY = 900.0f;
const float TOM_LIFT = -400.0f;

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

void loadBestScore(int& bs, std::string& bName) {
    ifstream file("assets/best_score.txt");
    bs = 0;
    bName = "PLAYER";
    if (file.is_open()) {
        file >> bs;
        file >> bName;
        file.close();
    }
}

void saveBestScore(int bs, const std::string& bName) {
    ofstream file("assets/best_score.txt");
    if (file.is_open()) {
        file << bs << "\n" << bName;
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

SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), 0, color);
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
    tom.x = TOM_START_X;
    tom.y = TOM_START_Y;
    tom.velocity = 0.0f;
    tom.currentFrame = 0;
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

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::cout << "Error SDL: " << SDL_GetError() << "\n";
        return 1;
    }

    if (!TTF_Init()) {
        std::cout << "Error TTF\n";
        return 1;
    }

    if (!MIX_Init()) {
        std::cout << "Error Mixer Init: " << SDL_GetError() << "\n";
    }

    MIX_Mixer* gMixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!gMixer) {
        std::cout << "Error Mixer Create: " << SDL_GetError() << "\n";
    }

    MIX_Audio* bgMusic = MIX_LoadAudio(gMixer, "assets/main_theme.mp3", true);
    MIX_Audio* jumpSound = MIX_LoadAudio(gMixer, "assets/jump.wav", true);
    MIX_Audio* scoreSound = MIX_LoadAudio(gMixer, "assets/score.wav", true);
    MIX_Audio* dieSound = MIX_LoadAudio(gMixer, "assets/die.wav", true);

    MIX_Track* musicTrack = nullptr;
    if (bgMusic) {
        musicTrack = MIX_CreateTrack(gMixer);
        MIX_SetTrackAudio(musicTrack, bgMusic);

        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
        MIX_PlayTrack(musicTrack, props);
        SDL_DestroyProperties(props);
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!SDL_CreateWindowAndRenderer("TomBay Game", 1000, 700, 0, &window, &renderer)) {
        std::cout << "Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);

    TTF_Font* font = TTF_OpenFont("assets/font.ttf", 28);

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
    tom.x = TOM_START_X;
    tom.y = TOM_START_Y;
    tom.width = TOM_WIDTH;
    tom.height = TOM_HEIGHT;
    tom.velocity = 0.0f;
    tom.gravity = TOM_GRAVITY;
    tom.lift = TOM_LIFT;

    SDL_Texture* gameOverTexture = loadTexture(renderer, "assets/menu.png");
    SDL_FRect gameOverRect = { -50.0f, 20.0f, 1000.0f, 700.0f };

    SDL_FRect restartBtnRect = { 570.0f, 370.0f, 80.0f, 57.0f };

    SDL_Texture* soundOnTex = loadTexture(renderer, "assets/sound_on.png");
    SDL_Texture* soundOffTex = loadTexture(renderer, "assets/sound_off.png");
    SDL_Texture* topRestartTex = loadTexture(renderer, "assets/restart.png");

    bool isMuted = false;

    SDL_FRect soundBtnRect = { 950.0f, 10.0f, 45.0f, 45.0f };
    SDL_FRect topRestartBtnRect = { 900.0f, 10.0f, 45.0f, 45.0f };

    std::vector<pipe> pipes;
    float pipeTimer = 0.0f;

    SDL_Texture* pipeTex = loadTexture(renderer, "assets/pipe.png");

    Uint64 lastFrameTime = SDL_GetTicks();
    const Uint64 frameDelay = 100;
    Uint64 lastTime = SDL_GetTicks();

    bool isRunning = true;
    bool isGameOver = false;
    bool isGameStarted = false;
    int score = 0;

    int bestScore = 0;
    std::string bestName = "";
    loadBestScore(bestScore, bestName);

    std::string playerName = "";
    bool isEnteringName = false;

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

            if (isEnteringName) {
                if (event.type == SDL_EVENT_TEXT_INPUT) {
                    if (playerName.length() < 13) {
                        playerName += event.text.text;
                    }
                }
                else if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.key == SDLK_BACKSPACE && playerName.length() > 0) {
                        playerName.pop_back();
                    }
                    else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
                        isEnteringName = false;
                        if (playerName.empty()) {
                            playerName = "PLAYER";
                        }
                        bestName = playerName;
                        saveBestScore(bestScore, bestName);
                        SDL_StopTextInput(window);
                    }
                }
            }
            else {
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.key == SDLK_SPACE && !isGameOver) {
                        tom.velocity = tom.lift;
                        isGameStarted = true;
                        if (!isMuted && jumpSound) MIX_PlayAudio(gMixer, jumpSound);
                    }
                }
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        if (isButtonClicked(event.button.x, event.button.y, topRestartBtnRect)) {
                            resetGame(tom, pipes, pipeTimer, isGameOver, isGameStarted, score);
                        }
                        else if (isButtonClicked(event.button.x, event.button.y, soundBtnRect)) {
                            isMuted = !isMuted;
                            if (isMuted) {
                                if (musicTrack) MIX_PauseTrack(musicTrack);
                            }
                            else {
                                if (musicTrack) MIX_ResumeTrack(musicTrack);
                            }
                        }
                        else if (isGameOver) {
                            if (isButtonClicked(event.button.x, event.button.y, restartBtnRect)) {
                                resetGame(tom, pipes, pipeTimer, isGameOver, isGameStarted, score);
                            }
                        }
                        else {
                            tom.velocity = tom.lift;
                            isGameStarted = true;
                            if (!isMuted && jumpSound) MIX_PlayAudio(gMixer, jumpSound);
                        }
                    }
                }
            }
        }

        if (!isGameOver) {
            Uint64 currentTime = SDL_GetTicks();
            if (currentTime - lastFrameTime >= frameDelay) {
                tom.currentFrame = (tom.currentFrame + 1) % birdFrames.size();
                lastFrameTime = currentTime;
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
                if (!it->passed && tom.x > it->x + PIPE_VISIBLE_RIGHT) {
                    score++;
                    it->passed = true;
                    if (!isMuted && scoreSound) MIX_PlayAudio(gMixer, scoreSound);
                }
                if (it->x + PIPE_WIDTH < 0) {
                    it = pipes.erase(it);
                }
                else {
                    ++it;
                }
            }

            if (checkGround(tom, GROUND_Y) || checkPipeCollision(tom, pipes)) {
                if (!isGameOver) {
                    if (!isMuted && dieSound) MIX_PlayAudio(gMixer, dieSound);
                    isGameOver = true;
                    if (score > bestScore) {
                        bestScore = score;
                        isEnteringName = true;
                        playerName = "";
                        SDL_StartTextInput(window);
                    }
                }
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

        for (const auto& p : pipes) {
            float topPipeY = p.gapY - p.gapSize / 2.0f - PIPE_HEIGHT;
            SDL_FRect topRect = { p.x, topPipeY, PIPE_WIDTH, PIPE_HEIGHT };
            SDL_RenderTextureRotated(renderer, pipeTex, nullptr, &topRect, 0.0, nullptr, SDL_FLIP_VERTICAL);

            float bottomPipeY = p.gapY + p.gapSize / 2.0f;
            SDL_FRect bottomRect = { p.x, bottomPipeY, PIPE_WIDTH, PIPE_HEIGHT };
            SDL_RenderTexture(renderer, pipeTex, nullptr, &bottomRect);
        }

        if (datTex) {
            SDL_RenderTexture(renderer, datTex, nullptr, &datRect);
        }

        if (!birdFrames.empty() && birdFrames[tom.currentFrame]) {
            SDL_FRect renderRect = { tom.x, tom.y, tom.width, tom.height };
            double angle = isGameStarted ? (tom.velocity * 0.04) : 0.0;
            if (angle < -20.0) angle = -20.0;
            if (angle > 60.0)  angle = 60.0;
            SDL_RenderTextureRotated(renderer, birdFrames[tom.currentFrame], nullptr, &renderRect, angle, nullptr, SDL_FLIP_NONE);
        }

        if (!isGameOver && isGameStarted) {
            if (font) {
                SDL_Color playingScoreColor = { 255, 255, 255, 255 };
                std::string currentScoreStr = std::to_string(score);
                SDL_Texture* currentScoreTex = renderText(renderer, font, currentScoreStr, playingScoreColor);
                if (currentScoreTex) {
                    float texW = 0.0f, texH = 0.0f;
                    SDL_GetTextureSize(currentScoreTex, &texW, &texH);
                    float renderHeight = 60.0f;
                    float renderWidth = texW * (renderHeight / texH);
                    SDL_FRect sRect = { 500.0f - (renderWidth / 2.0f), 70.0f, renderWidth, renderHeight };
                    SDL_RenderTexture(renderer, currentScoreTex, nullptr, &sRect);
                    SDL_DestroyTexture(currentScoreTex);
                }
            }
        }

        if (isGameOver && gameOverTexture) {
            SDL_RenderTexture(renderer, gameOverTexture, nullptr, &gameOverRect);

            if (font) {
                SDL_Color scoreColor = { 128, 117, 101, 255 };
                std::string scoreStr = "SCORE : " + std::to_string(score);
                SDL_Texture* scoreTex = renderText(renderer, font, scoreStr, scoreColor);
                if (scoreTex) {
                    float texW = 0.0f, texH = 0.0f;
                    SDL_GetTextureSize(scoreTex, &texW, &texH);
                    float renderWidth = texW * (32.0f / texH);
                    SDL_FRect sRect = { 355.0f, 378.0f, renderWidth, 35.0f };
                    SDL_RenderTexture(renderer, scoreTex, nullptr, &sRect);
                    SDL_DestroyTexture(scoreTex);
                }

                SDL_Color bestScoreColor = { 128, 117, 101, 255 };
                std::string bestStr = "BEST SCORE : " + std::to_string(bestScore);
                SDL_Texture* bestTex = renderText(renderer, font, bestStr, bestScoreColor);
                if (bestTex) {
                    float texW = 0.0f, texH = 0.0f;
                    SDL_GetTextureSize(bestTex, &texW, &texH);
                    float renderWidth = texW * (32.0f / texH);
                    SDL_FRect bRect = { 415.0f, 240.0f, renderWidth, 35.0f };
                    SDL_RenderTexture(renderer, bestTex, nullptr, &bRect);
                    SDL_DestroyTexture(bestTex);
                }

                if (isEnteringName) {
                    if ((SDL_GetTicks() / 500) % 2 == 0) {
                        SDL_Color notifyColor = { 255, 0, 0, 255 };
                        std::string notifyStr = "NEW RECORD! ENTER NAME";
                        SDL_Texture* notifyTex = renderText(renderer, font, notifyStr, notifyColor);
                        if (notifyTex) {
                            float texW = 0.0f, texH = 0.0f;
                            SDL_GetTextureSize(notifyTex, &texW, &texH);
                            float rWidth = texW * (18.0f / texH);
                            SDL_FRect notifyRect = { 385.0f, 340.0f, rWidth, 19.0f };
                            SDL_RenderTexture(renderer, notifyTex, nullptr, &notifyRect);
                            SDL_DestroyTexture(notifyTex);
                        }
                    }
                }

                std::string nameToRender = isEnteringName ? (playerName + "_") : bestName;
                if (!nameToRender.empty()) {
                    SDL_Color playerNameColor = { 230, 174, 158, 255 };
                    SDL_Texture* nameTex = renderText(renderer, font, nameToRender, playerNameColor);
                    if (nameTex) {
                        float texW = 0.0f, texH = 0.0f;
                        SDL_GetTextureSize(nameTex, &texW, &texH);
                        float rWidth = texW * (32.0f / texH);
                        float renderX = 520.0f - (rWidth / 2.0f);

                        SDL_FRect nameRect = { renderX, 290.0f, rWidth, 30.0f };
                        SDL_RenderTexture(renderer, nameTex, nullptr, &nameRect);
                        SDL_DestroyTexture(nameTex);
                    }
                }
            }
        }
        if (!isGameStarted && !isGameOver) {
            if (font) {
                SDL_Color startColor = { 255, 255, 255, 255 };
                std::string startStr = "Press SPACE or CLICK to Fly";
                SDL_Texture* startTex = renderText(renderer, font, startStr, startColor);
                if (startTex) {
                    float texW = 0.0f, texH = 0.0f;
                    SDL_GetTextureSize(startTex, &texW, &texH);
                    float renderWidth = texW * (30.0f / texH);

                    float offsetY = sin(SDL_GetTicks() / 200.0f) * 10.0f;
                    SDL_FRect startRect = { 500.0f - (renderWidth / 2.0f), 220.0f + offsetY, renderWidth, 30.0f };

                    SDL_RenderTexture(renderer, startTex, nullptr, &startRect);
                    SDL_DestroyTexture(startTex);
                }
            }
        }
        if (topRestartTex) {
            SDL_RenderTexture(renderer, topRestartTex, nullptr, &topRestartBtnRect);
        }

        SDL_Texture* currentSoundTex = isMuted ? soundOffTex : soundOnTex;
        if (currentSoundTex) {
            SDL_RenderTexture(renderer, currentSoundTex, nullptr, &soundBtnRect);
        }

        SDL_RenderPresent(renderer);
    }

    if (musicTrack) MIX_DestroyTrack(musicTrack);
    if (bgMusic) MIX_DestroyAudio(bgMusic);
    if (jumpSound) MIX_DestroyAudio(jumpSound);
    if (scoreSound) MIX_DestroyAudio(scoreSound);
    if (dieSound) MIX_DestroyAudio(dieSound);
    if (gMixer) MIX_DestroyMixer(gMixer);

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

    if (font) TTF_CloseFont(font);
    TTF_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}