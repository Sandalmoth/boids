#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>

#include "glad/glad.h"

#include "GLFW/glfw3.h"

#include "glm/glm.hpp"


constexpr double LOGIC_DT = 0.1;
std::mutex triple_buffer_mutex;
std::atomic<bool> running {true};


void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}


void draw(GLFWwindow *window) {

  glfwMakeContextCurrent(window);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

  while (running.load()) {
    glClear(GL_COLOR_BUFFER_BIT);

    // draw here

    glfwSwapBuffers(window);
  }

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

  glfwMakeContextCurrent(nullptr);
  std::thread draw_thread(&draw, window);

  float alpha;

  // for maintaining gameloop timestep
  double start_time = glfwGetTime();
  double accumulator = 0.0;
  double current_time;

  // for tracking graphics fps
  int frames = 0;
  double frame_start = glfwGetTime();
  double frame_time;

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

      accumulator -= LOGIC_DT;

    }

    alpha = accumulator / LOGIC_DT;

    // draw here ?

    frame_time = glfwGetTime();
    if (frame_time - frame_start > 1.0 || frames == 0) {
      double fps = static_cast<double>(frames) / (frame_time - frame_start);
      double frm_time = (frame_time - frame_start) / static_cast<double>(frames);
      frame_start = frame_time;
      frames = 0;
      std::cout << "fps\t" << fps << "\tframe_time\t" << frm_time << std::endl;
    }
    ++frames;

  }

  running.store(false);
  draw_thread.join();

  // end program section

  glfwTerminate();
  return 0;
}
