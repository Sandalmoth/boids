#include <iostream>
#include <random>
#include <vector>

#include <SDL2/SDL.h>
#include <gmtl/gmtl.h>

#include "boid.h"
#include "map.h"


const int WIDTH = 1024;
const int HEIGHT = 1024;


int main() {
  // ### Init SDL ### //
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window = SDL_CreateWindow("boids"
                                        , SDL_WINDOWPOS_UNDEFINED
                                        , SDL_WINDOWPOS_UNDEFINED
                                        , WIDTH
                                        , HEIGHT
                                        , SDL_WINDOW_SHOWN
                                        );
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);


  // ### System setup ### //
  Map boids(WIDTH, HEIGHT);
  // spawn some boids randomly
  std::random_device rd;
  std::mt19937 rng;
  rng.seed(rd());
  std::uniform_real_distribution<float> x_distribution(0, WIDTH);
  std::uniform_real_distribution<float> y_distribution(0, HEIGHT);
  // std::normal_distribution<float> x_distribution(WIDTH/2, WIDTH/2);
  // std::normal_distribution<float> y_distribution(HEIGHT/2, HEIGHT/2);
  std::uniform_real_distribution<float> v_distribution(-1, 1);
  for (int i = 0; i < 10000; ++i) {
    gmtl::Vec2f p0 = {x_distribution(rng), y_distribution(rng)};
    gmtl::Vec2f v0 = {v_distribution(rng), v_distribution(rng)};
    boids.insert(Boid(p0, v0));
  }

  // ### Simulation Loop ### //
  SDL_Event event;
  bool run = true;
  int mouse_x = WIDTH/2;
  int mouse_y = HEIGHT/2;
  std::cout << mouse_x << mouse_y << std::endl; // dummy
  while (run) {

    SDL_Delay(16);

    boids.regenerate_tree();

    // ### Engine step ### //
    // update boid velocity
    for (auto &b: boids) {
      b.update(boids.within_distance(b.get_position(), 50));
    }

    // move boids
    for (auto &b: boids) {
      b.move();
    }

    // ### Drawing and events ### //
    SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0xFF);
    SDL_RenderClear(renderer);

    for (auto &b: boids) {
      SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
      // gmtl::Point2f pos((b.get_position() - gmtl::Point2f(WIDTH, HEIGHT)*0.5f)*0.3f
      //                   + gmtl::Point2f(WIDTH, HEIGHT)*0.5f);
      gmtl::Point2f pos(b.get_position());
      SDL_Rect c = {static_cast<int>(pos[0] - 0.5),
                    static_cast<int>(pos[1] - 0.5), 1, 1};
      SDL_RenderFillRect(renderer, &c);
    }

    SDL_RenderPresent(renderer);

    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        run = false;
      } else if (event.type == SDL_MOUSEMOTION) {
        mouse_x = event.motion.x;
        mouse_y = event.motion.y;
      } else if (event.type == SDL_MOUSEBUTTONDOWN) {
      }
    }
  }


  // ### Cleanup SDL ### //
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
