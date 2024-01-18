#include "frontend.h"
#include "ppu.h"

#include <SDL2/SDL.h>

void frontend_draw_tiles(Frontend *frontend, uint8_t *mem);
void frontend_draw_sprites(Frontend *frontend, uint8_t *mem);

#define SCALE 2
#define WIDTH 256
#define HEIGHT 240

void frontend_init(Frontend *frontend) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
    SDL_Quit();
  }

  SDL_Window *window = SDL_CreateWindow(
      "leekboy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      256 * SCALE + 128 * SCALE, (128 + 128) * SCALE, SDL_WINDOW_SHOWN);

  if (window == NULL) {
    printf("Window creation failed: %s\n", SDL_GetError());
    SDL_Quit();
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED );

  SDL_Texture *texture = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 240);

  SDL_Texture *pattern_table_1 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                                   SDL_TEXTUREACCESS_STREAMING, 128, 128);
  SDL_Texture *pattern_table_2 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                                   SDL_TEXTUREACCESS_STREAMING, 128, 128);

  frontend->renderer = renderer;
  frontend->texture = texture;

  frontend->pattern_table_textures[0] = pattern_table_1;
  frontend->pattern_table_textures[1] = pattern_table_2;
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

  // handle input
  const uint8_t *keyboard_state = SDL_GetKeyboardState(NULL);
  emulator->controller[0] = 0x00;
  emulator->controller[0] |= keyboard_state[SDL_SCANCODE_X] << 7;
  emulator->controller[0] |= keyboard_state[SDL_SCANCODE_Z] << 6;
  emulator->controller[0] |= keyboard_state[SDL_SCANCODE_A] << 5;
  emulator->controller[0] |= keyboard_state[SDL_SCANCODE_S] << 4;
  emulator->controller[0] |= keyboard_state[SDL_SCANCODE_UP] << 3;
  emulator->controller[0] |= keyboard_state[SDL_SCANCODE_DOWN] << 2;
  emulator->controller[0] |= keyboard_state[SDL_SCANCODE_LEFT] << 1;
  emulator->controller[0] |= keyboard_state[SDL_SCANCODE_RIGHT] << 0;

  // draw
  SDL_SetRenderDrawColor(frontend->renderer, 0, 0, 0, 255);
  SDL_RenderClear(frontend->renderer);

  // Rect
  SDL_Rect rect;
  rect.x = 0;
  rect.y = 0;
  rect.w = WIDTH * 2;
  rect.h = HEIGHT * 2;

  SDL_UpdateTexture(frontend->texture, NULL, emulator->ppu.framebuffer,
                    WIDTH * sizeof(uint32_t));
  SDL_RenderCopy(frontend->renderer, frontend->texture, NULL, &rect);

  // Debug
  SDL_Rect pattern_table_1;
  pattern_table_1.x = rect.w;
  pattern_table_1.y = 0;
  pattern_table_1.w = 128 * 2;
  pattern_table_1.h = 128 * 2;

  SDL_Rect pattern_table_2;
  pattern_table_2.x = rect.w;
  pattern_table_2.y = pattern_table_1.h;
  pattern_table_2.w = 128 * 2;
  pattern_table_2.h = 128 * 2;

  SDL_UpdateTexture(frontend->pattern_table_textures[0], NULL,
                    ppu_get_pattern_table(&emulator->ppu, 0, 0),
                    128 * sizeof(uint32_t));
  SDL_RenderCopy(frontend->renderer, frontend->pattern_table_textures[0], NULL,
                 &pattern_table_1);

  SDL_UpdateTexture(frontend->pattern_table_textures[1], NULL,
                    ppu_get_pattern_table(&emulator->ppu, 1, 0),
                    128 * sizeof(uint32_t));
  SDL_RenderCopy(frontend->renderer, frontend->pattern_table_textures[1], NULL,
                 &pattern_table_2);

  SDL_RenderPresent(frontend->renderer);
}

void frontend_run(Frontend *frontend, Emulator *emulator) {
  while (1) {
    frontend_update(frontend, emulator);
    emulator_step(emulator);
  }
}
