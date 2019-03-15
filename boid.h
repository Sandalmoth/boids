#ifndef __BOID_H__
#define __BOID_H__


#include <gmtl/gmtl.h>


class Boid {
public:
  Boid(gmtl::Point2f p0, gmtl::Point2f v0)
    : p(p0)
    , v(v0) {
    v_prev = gmtl::Vec2f(0, 0);
  }

  gmtl::Vec2f get_position() { return p; }

private:
  gmtl::Point2f p; // position
  gmtl::Vec2f v; // velocity
  gmtl::Vec2f v_prev; // needed for averaging headings
};


#endif
