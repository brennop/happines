#include "frontend.h"

#include <SDL2/SDL.h>

void frontend_draw_tiles(Frontend *frontend, uint8_t *mem);
void frontend_draw_sprites(Frontend *frontend, uint8_t *mem);

#define SCALE 4
#define WIDTH 256
#define HEIGHT 240

void frontend_init(Frontend *frontend) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
    SDL_Quit();
  }

  SDL_Window *window =
      SDL_CreateWindow("leekboy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * SCALE,
                       144 * SCALE, SDL_WINDOW_SHOWN);

  if (window == NULL) {
    printf("Window creation failed: %s\n", SDL_GetError());
    SDL_Quit();
  }

  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                           SDL_TEXTUREACCESS_STREAMING, 160, 144);

  frontend->renderer = renderer;
  frontend->texture = texture;
}

void frontend_update(Frontend *frontend, Emulator *emulator) {
  // handle quit
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      SDL_Quit();
      exit(0);
    } else if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        SDL_Quit();
        exit(0);
      }
    }
  }

  // draw
  SDL_SetRenderDrawColor(frontend->renderer, 0, 0, 0, 255);
  SDL_RenderClear(frontend->renderer);

  // Rect
  SDL_Rect rect;
  rect.x = 0;
  rect.y = 0;
  rect.w = WIDTH * SCALE;
  rect.h = HEIGHT * SCALE;

  SDL_UpdateTexture(frontend->texture, NULL, emulator->ppu.framebuffer,
                    WIDTH * sizeof(uint32_t));
  SDL_RenderCopy(frontend->renderer, frontend->texture, NULL, &rect);

  SDL_RenderPresent(frontend->renderer);
}

void frontend_run(Frontend *frontend, Emulator *emulator) {
  while (1) {
    frontend_update(frontend, emulator);
    emulator_step(emulator);
  }
}
