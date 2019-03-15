#ifndef __MAP_H__
#define __MAP_H__


#include <vector>

#include <gmtl/gmtl.h>

#include "boid.h"


class Map {
public:
  void insert(Boid && b) {
    boids.emplace_back(b);
  }

  auto begin() { return boids.begin(); }
  auto end() { return boids.end(); }

  std::vector<Boid *> within_distance(gmtl::Point2f p, float d) {
    // Return vector with pointers to all
    // boids within distance d from point p
    std::vector<Boid *> inside;
    float d2 = d*d;
    for (auto &b: boids) {
      if (gmtl::lengthSquared(gmtl::Vec2f(b.get_position() - p)) <= d2) {
        inside.push_back(&b);
      }
    }
    return inside;
  }

private:
  std::vector<Boid> boids;
};


#endif
