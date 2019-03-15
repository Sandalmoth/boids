#ifndef __BOID_H__
#define __BOID_H__


#include <vector>

#include <gmtl/gmtl.h>


class Boid {
public:
  Boid(gmtl::Point2f p0, gmtl::Point2f v0)
    : p(p0)
    , v(v0) {
    v_prev = gmtl::Vec2f(0, 0);
  }

  gmtl::Vec2f get_position() { return p; }
  gmtl::Vec2f get_velocity() { return v_prev; }

  void move() {
    // move the boid
    p += v;
    v_prev = v;
  }

  void update(std::vector<Boid *> nbs) {
    // Move towards center off mass
    gmtl::Point2f center;
    for (auto b: nbs) {
      center += b->get_position();
    }
    center /= nbs.size();
    gmtl::Vec2f v_center(center - p);

    // Avoid any boids that are too close
    gmtl::Vec2f v_near;
    for (auto b: nbs) {
      gmtl::Vec2f v_other(b->get_position() - p);
      if (gmtl::lengthSquared(v_other) < 10) {
        v_near -= v_other;
      }
    }

    // Steer towards direction of group
    gmtl::Vec2f v_steer;
    for (auto b: nbs) {
      v_steer += b->get_velocity();
    }
    v_steer /= nbs.size();

    // Avoid edge of map
    gmtl::Vec2f v_edge;
    if (p[0] < 10) {
      double dx = p[0] - 10;
      v_edge -= gmtl::Vec2f(dx, 0);
    }
    if (p[1] < 10) {
      double dy = p[1] - 10;
      v_edge -= gmtl::Vec2f(0, dy);
    }
    if (p[0] > 1014) {
      double dx = 1014 - p[0];
      v_edge += gmtl::Vec2f(dx, 0);
    }
    if (p[1] > 758) {
      double dy = 758 - p[1];
      v_edge += gmtl::Vec2f(0, dy);
    }



    // update velocity with coefficients
    v += 0.005f*v_center + 0.1f*v_near + 0.05f*v_steer + 1.0f*v_edge;
    if (gmtl::lengthSquared(v) > 4) {
      gmtl::normalize(v);
      v *= 2.0f;
    }
  }

private:
  gmtl::Point2f p; // position
  gmtl::Vec2f v; // velocity
  gmtl::Vec2f v_prev; // needed for averaging headings
};


#endif
