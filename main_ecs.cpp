#include <atomic>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>

#include "glad/glad.h"

#include "GLFW/glfw3.h"

#include "glm/glm.hpp"

#define ECSOPLATM_IMPLEMENTATION
#include "ecsoplatm.h"


#include "shader.cpp"


constexpr double LOGIC_DT = 0.1;
constexpr int NUM_BOIDS = 4096;

constexpr float BOID_VEL = 0.05;
constexpr float SENSE_RAD = 0.1;
constexpr float BOID_CENTER = 0.002;
constexpr float BOID_NEAR = 0.02;
constexpr float BOID_STEER = 0.03;


std::mutex triple_buffer_mutex;

double last_tick_time {0.0}; // should be manupulated under triple_buffer_mutex
double next_tick_time {0.0}; // should be maipulated under triple_buffer_mutex

std::atomic<bool> running {true};


struct Posbuf {
  glm::vec2 prev;
  glm::vec2 next;
};


int hashable(glm::vec2 v) {
  // convert for use in spatial hash
  int x = v.x/SENSE_RAD;
  int y = v.y/SENSE_RAD;
  return x ^ y;
}


std::vector<uint32_t> neighbours(glm::vec2 v,
                                 std::unordered_multimap<int, uint32_t> *spatial_hash,
                                 ecs::Component<glm::vec2> *c_pos) {
  // returns the neighbours of a point
  std::vector<uint32_t> result;
  glm::vec2 dx(SENSE_RAD, 0.0);
  glm::vec2 dy(0.0, SENSE_RAD);

  std::vector<int> hashes;
  hashes.reserve(9);
  hashes.push_back(hashable(v));
  hashes.push_back(hashable(v + dx));
  hashes.push_back(hashable(v - dx));
  hashes.push_back(hashable(v + dy));
  hashes.push_back(hashable(v - dy));
  hashes.push_back(hashable(v + dx + dy));
  hashes.push_back(hashable(v + dx - dy));
  hashes.push_back(hashable(v - dx + dy));
  hashes.push_back(hashable(v - dx - dy));

  for (auto hash: hashes) {
    auto range = spatial_hash->equal_range(hash);
    for (auto it = range.first; it != range.second; ++it) {
      glm::vec2 other = *(*c_pos)[it->second];
      if (glm::dot(other - v, other - v) < SENSE_RAD*SENSE_RAD) {
        result.push_back(it->second);
      }
    }
  }

  return result;
}


void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glfwMakeContextCurrent(window); // unsure about this...
  glViewport(0, 0, width, height);
  // alternative implementation, update some atomic width height
  // and then update viewport in draw thread
}

struct update_vel_payload {
  ecs::Component<glm::vec2> *c_pos;
  ecs::Component<glm::vec2> *c_vel;
  std::unordered_multimap<int, uint32_t> *spatial_hash;
};

void update_vel(glm::vec2 &pos, glm::vec2 &vel, void *payload) {
  // NOTE having a boid struct with pos and vel would be more elegant
  auto pl = static_cast<update_vel_payload *>(payload);
  auto nbs = neighbours(pos, pl->spatial_hash, pl->c_pos);

  glm::vec2 center(0.0f);
  glm::vec2 near(0.0f);
  for (auto nb: nbs) {
    glm::vec2 *p = (*(pl->c_pos))[nb]; // confusinsg operator precedence
    center += *p;
    near -= (*p) - pos;
  }
  center /= static_cast<float>(pl->c_pos->data.size());
  center = center - pos;

  glm::vec2 steer(0.0f);
  for (auto nb: nbs) {
    glm::vec2 *v = (*(pl->c_vel))[nb];
    steer += *v;
  }
  steer /= static_cast<float>(pl->c_vel->data.size());

  if (glm::length(center) > 0.0f)
    center = glm::normalize(center);
  if (glm::length(near) > 0.0f)
    near = glm::normalize(near);
  if (glm::length(steer) > 0.0f)
    steer = glm::normalize(steer);

  vel = BOID_VEL*glm::normalize(vel +
                                BOID_CENTER*center +
                                BOID_NEAR*near +
                                BOID_STEER*steer);
}

void move(glm::vec2 &pos, glm::vec2 &vel) {
  pos += vel;
  if (pos.x < -1.0f) {
    pos.x = -2.0f - pos.x;
    vel.x = -vel.x;
  }
  if (pos.y < -1.0f) {
    pos.y = -2.0f - pos.y;
    vel.y = -vel.y;
  }
  if (pos.x > 1.0f) {
    pos.x = 2.0f - pos.x;
    vel.x = -vel.x;
  }
  if (pos.y > 1.0f) {
    pos.y = 2.0f - pos.y;
    vel.y = -vel.y;
  }
}

void update_posbuf(glm::vec2 &pos, Posbuf &posbuf) {
  posbuf.prev = posbuf.next;
  posbuf.next = pos;
}


void draw(GLFWwindow *window, ecs::Component<Posbuf> &c_posbuf) {

  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  GLuint shader = load_shaders();
  glUseProgram(shader);
  glDisable(GL_DEPTH_TEST);

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
    glUseProgram(shader);
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

  glViewport(0, 0, 400, 300);
  // glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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

  // for trackin the time taken
  // so we can optimize
  double logic_time = 0.0;
  double worst_logic_time = 0.0;
  int logic_ticks = 0;

  std::unordered_multimap<int, uint32_t> spatial_hash;

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

      auto logic_timer = [start = glfwGetTime()]{ return glfwGetTime() - start; };

      // logic here
      // first build our spatial hash
      spatial_hash.clear();
      for (auto [id, pos]: c_pos.data) {
        spatial_hash.insert(std::make_pair(hashable(pos), id));
      }

      // then update all the boids
      update_vel_payload uv_payload(&c_pos, &c_vel, &spatial_hash);
      ecs.apply(&update_vel, c_pos, c_vel, static_cast<void *>(&uv_payload));
      ecs.apply(&move, c_pos, c_vel);
      ecs.wait();

      {
        std::scoped_lock lock(triple_buffer_mutex);
        last_tick_time = next_tick_time;
        next_tick_time = glfwGetTime();

        ecs.apply(&update_posbuf, c_pos, c_posbuf);
        ecs.wait();
      }

      accumulator -= LOGIC_DT;

      logic_time += logic_timer();
      worst_logic_time = std::max(logic_timer(), worst_logic_time);
      ++logic_ticks;

      if (logic_ticks == 9) {
        std::cout << "Averageg logic step: " << logic_time/10.0;
        std::cout << "\tWorst: " << worst_logic_time << std::endl;
        logic_ticks = 0;
        logic_time = 0.0;
        worst_logic_time = 0.0;
      }

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
