#ifndef __MAP_H__
#define __MAP_H__


#include <memory>
#include <vector>

#include <gmtl/gmtl.h>

#include "boid.h"


const int QUADTREE_LEVELS = 4;


struct QuadtreeNode {
  std::unique_ptr<QuadtreeNode> ne = nullptr;
  std::unique_ptr<QuadtreeNode> nw = nullptr;
  std::unique_ptr<QuadtreeNode> se = nullptr;
  std::unique_ptr<QuadtreeNode> sw = nullptr;
  std::vector<std::weak_ptr<Boid>> node_boids;
};


void build_quadtree(const std::unique_ptr<QuadtreeNode> & root, int depth) {
  if (depth < 1) return;

  root->ne = std::make_unique<QuadtreeNode>();
  root->nw = std::make_unique<QuadtreeNode>();
  root->se = std::make_unique<QuadtreeNode>();
  root->sw = std::make_unique<QuadtreeNode>();
  build_quadtree(root->ne, depth-1)
}


class Map {
public:
  Map(float width, float height)
    : width(width)
    , height(height) {
    root = std::make_unique<QuadtreeNode>();
  }


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
  const int width;
  const int height;

  std::unique_ptr<QuadtreeNode> root;

  std::vector<Boid> boids;
};


#endif
