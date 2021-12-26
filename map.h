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
  build_quadtree(root->ne, depth-1);
  build_quadtree(root->nw, depth-1);
  build_quadtree(root->se, depth-1);
  build_quadtree(root->sw, depth-1);
}


void empty_quadtree(const std::unique_ptr<QuadtreeNode>& root) {
  if (root == nullptr) return;

  root->node_boids.empty();
  empty_quadtree(root->ne);
  empty_quadtree(root->nw);
  empty_quadtree(root->se);
  empty_quadtree(root->sw);
}


void quadtree_insert(const std::unique_ptr<QuadtreeNode>& root, const Boid* b, int width, int height, gmtl::Vec2f offset={0, 0}) {
  if (root->ne == nullptr) {
    root->node_boids.push_back(std::weak_ptr<Boid>(*b));
    return;
  }

  if (b->p[0] < offset[0] + (width << 2)) {
    if (b->p[1] < offset[1] + (height << 2)) {
      quadtree_insert(root->ne, b, width << 2, height << 2, offset);
    } else {
      quadtree_insert(root->ne, b, width << 2, height << 2, offset + gmtl::Vec2f(0, height << 2));
    }
  } else {
    if (b->p[1] < offset[1] + (height << 2)) {
      quadtree_insert(root->ne, b, width << 2, height << 2, offset + gmtl::Vec2f(width << 2, 0));
    } else {
      quadtree_insert(root->ne, b, width << 2, height << 2, offset + gmtl::Vec2f(width << 2, height << 2));
    }
  }
}


class Map {
public:
  Map(float width, float height)
    : width(width)
    , height(height) {
    root = std::make_unique<QuadtreeNode>();
    build_quadtree(root, QUADTREE_LEVELS);
  }


  void regenerate_tree() {
    empty_quadtree(root);
    for (auto &b: boids) {
      quadtree_insert(root, &b, width, height);
    }
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
