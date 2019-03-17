#ifndef __MAP_H__
#define __MAP_H__


#include <array>
#include <iostream>
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
  std::vector<Boid*> node_boids;
};


void build_quadtree(const std::unique_ptr<QuadtreeNode> &root, int depth) {
  if (depth < 1) return;

  root->ne = std::make_unique<QuadtreeNode>();
  root->nw = std::make_unique<QuadtreeNode>();
  root->se = std::make_unique<QuadtreeNode>();
  root->sw = std::make_unique<QuadtreeNode>();
  build_quadtree(root->ne, depth - 1);
  build_quadtree(root->nw, depth - 1);
  build_quadtree(root->se, depth - 1);
  build_quadtree(root->sw, depth - 1);
}


void empty_leaves(const std::unique_ptr<QuadtreeNode> &root, int depth) {
  if (depth < 1) {
    root->node_boids.clear();
    return;
  }

  empty_leaves(root->ne, depth -1);
  empty_leaves(root->nw, depth -1);
  empty_leaves(root->se, depth -1);
  empty_leaves(root->sw, depth -1);
}


const std::unique_ptr<QuadtreeNode> &locate(const std::unique_ptr<QuadtreeNode> &root,
                                            int depth, float width, float height,
                                            gmtl::Point2f p) {
  return root;
}


class Map {
public:
  Map(float width, float height)
    : width(width)
    , height(height) {
    root = std::make_unique<QuadtreeNode>();
    build_quadtree(root, QUADTREE_LEVELS);
    // create quick access to leaves such that we can place boids in leaves with just 2 divisions
    int n_leaves = (2 << QUADTREE_LEVELS);
    float x_grid = width / (2 << QUADTREE_LEVELS);
    float y_grid = height / (2 << QUADTREE_LEVELS);
    for (int x = 0; x < n_leaves; ++x) {
      for (int y = 0; y < n_leaves; ++y) {
        leaves[x*n_leaves + y] = &(locate(root, QUADTREE_LEVELS, width, height,
                                          gmtl::Point2f(x_grid*x + 1, y_grid*y + 1))
                                   ->node_boids);
      }
    }
  }


  void insert(Boid && b) {
    boids.emplace_back(b);
  }


  void rebuild_tree() {
    empty_leaves(root, QUADTREE_LEVELS);
    for (auto &b: boids) {
      auto p = b.get_position();
      float x_grid = width / (2 << QUADTREE_LEVELS);
      float y_grid = height / (2 << QUADTREE_LEVELS);
      leaves[static_cast<int>(p[0]/x_grid*(2 >> QUADTREE_LEVELS)) +
             static_cast<int>(p[1]/y_grid)]->push_back(&b);
    }
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
  const float width;
  const float height;

  std::unique_ptr<QuadtreeNode> root;
  // fast access to leaves for faster rebuilding
  std::array<std::vector<Boid *> *,
             (2 << QUADTREE_LEVELS) * (2 << QUADTREE_LEVELS)> leaves;

  std::vector<Boid> boids;
};


#endif
