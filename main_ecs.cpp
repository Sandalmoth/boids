#include <atomic>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

#include "glad/glad.h"

#include "GLFW/glfw3.h"

#include "glm/glm.hpp"

#define ECSOPLATM_IMPLEMENTATION
#include "ecsoplatm.h"


constexpr double LOGIC_DT = 0.1;
constexpr int NUM_BOIDS = 1024;
constexpr float BOID_VEL = 0.05;


std::mutex triple_buffer_mutex;

double last_tick_time {0.0}; // should be manupulated under triple_buffer_mutex
double next_tick_time {0.0}; // should be maipulated under triple_buffer_mutex

std::atomic<bool> running {true};


struct Posbuf {
  glm::vec2 prev;
  glm::vec2 next;
};


void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}


void draw(GLFWwindow *window, ecs::Component<Posbuf> &c_posbuf) {

  glfwMakeContextCurrent(window);

  std::vector<glm::vec2> boid_buffer(NUM_BOIDS);

  GLuint vao, vbo;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * NUM_BOIDS,
               nullptr, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2),
                        (void *)0);

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

  // for tracking graphics fps
  int frames = 0;
  double frame_start = glfwGetTime();
  double frame_time;

  double alpha {0.0};

  while (running.load()) {

    // harvest the interpolated positions
    {
      std::scoped_lock lock(triple_buffer_mutex);
      alpha = (glfwGetTime() - next_tick_time) / LOGIC_DT; // FIXME may have glitches

      for (int i = 0; i < NUM_BOIDS; ++i) {
        boid_buffer[i] = glm::mix(c_posbuf.data[i].second.prev,
                                  c_posbuf.data[i].second.next, alpha);
      }
    }

    // then actually draw
    glClear(GL_COLOR_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2) * NUM_BOIDS,
                    boid_buffer.data());

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, NUM_BOIDS);

    glfwSwapBuffers(window);

    frame_time = glfwGetTime();
    if (frame_time - frame_start > 1.0 || frames == 0) {
      double fps = static_cast<double>(frames) / (frame_time - frame_start);
      double frm_time =
          (frame_time - frame_start) / static_cast<double>(frames);
      frame_start = frame_time;
      frames = 0;
      std::cout << "fps\t" << fps << "\tframe_time\t" << frm_time << std::endl;
    }
    ++frames;
  }

  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}


int main() {

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(1280, 720, "Boids using ecs", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glViewport(0, 0, 800, 600);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // program here

  ecs::Manager ecs;

  ecs::Component<Posbuf> c_posbuf; // locked by triple_buffer_mutex
  ecs::Component<glm::vec2> c_pos;
  ecs::Component<glm::vec2> c_vel;

  ecs.enlist(&c_posbuf);
  ecs.enlist(&c_pos);
  ecs.enlist(&c_vel);

  // we haven't spawned the graphics thread yet
  // so we don't need any synchronization
  last_tick_time = glfwGetTime();
  next_tick_time = glfwGetTime();

  std::mt19937 rng;
  rng.seed(2701);
  std::uniform_real_distribution<float> dist(-1.0, 1.0);

  for (int i = 0; i < NUM_BOIDS; ++i) {
    auto id = ecs.get_id();
    glm::vec2 pos(dist(rng), dist(rng));
    glm::vec2 vel(dist(rng), dist(rng));
    vel = glm::normalize(vel)*BOID_VEL;
    c_pos.create(id, pos);
    c_posbuf.create(id, Posbuf{pos - vel, pos});
    c_vel.create(id, vel);
  }
  ecs.update();

  // transfer graphics to separate thread
  glfwMakeContextCurrent(nullptr);
  std::thread draw_thread(&draw, window, std::ref(c_posbuf));

  double alpha;

  // for maintaining gameloop timestep
  double start_time = glfwGetTime();
  double accumulator = 0.0;
  double current_time;

  while (!glfwWindowShouldClose(window)) {

    current_time = glfwGetTime();
    double time_diff = current_time - start_time;
    accumulator += time_diff;
    if (accumulator > 0.5) {
      accumulator = 0.5;
    }

    start_time = glfwGetTime();

    glfwPollEvents();

    while (accumulator > LOGIC_DT) {

      // logic here

      {
        std::scoped_lock lock(triple_buffer_mutex);
        last_tick_time = next_tick_time;
        next_tick_time = glfwGetTime();
      }

      accumulator -= LOGIC_DT;

    }

    alpha = accumulator / LOGIC_DT;

    // what goes here? input?

  }

  running.store(false);
  draw_thread.join();

  // end program section

  glfwTerminate();
  return 0;
}
