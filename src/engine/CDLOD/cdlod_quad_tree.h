// Copyright (c) 2014, Tamas Csala

#ifndef ENGINE_CDLOD_QUAD_TREE_H_
#define ENGINE_CDLOD_QUAD_TREE_H_

#include <memory>
#include <iostream>
#include "grid_mesh_renderer.h"
#include "../collision/bounding_box.h"

namespace engine {

class CDLODQuadTree : public engine::GameObject {
  GridMeshRenderer renderer_;

  struct Node {
    GLshort x, z;
    GLshort min_y, max_y;
    GLushort level;
    GLushort size;
    std::unique_ptr<Node> tl, tr, bl, br;

    Node(GLshort x, GLshort z, GLushort level)
        : x(x)
        , z(z)
        , min_y(0)
        , max_y(0)
        , level(level)
        , size(GridMeshRenderer::max_dim * (1 << level))
        , tl(nullptr)
        , tr(nullptr)
        , bl(nullptr)
        , br(nullptr) {

      if (level > 0) {
        tl = std::unique_ptr<Node>(new Node(x-size/4, z+size/4, level-1));
        tr = std::unique_ptr<Node>(new Node(x+size/4, z+size/4, level-1));
        bl = std::unique_ptr<Node>(new Node(x-size/4, z-size/4, level-1));
        br = std::unique_ptr<Node>(new Node(x+size/4, z-size/4, level-1));
      }
    }

    BoundingBox boundingBox() const {
      return BoundingBox(glm::vec3(x-size/2, min_y, z-size/2),
                         glm::vec3(x+size/2, max_y, z+size/2));
    }

    bool collidesWithSphere(const glm::vec3& center, float radius) {
      return boundingBox().collidesWithSphere(center, radius);
    }


    void render(const engine::Camera& cam, const Frustum& frustum,
                GridMeshRenderer& renderer) {
      float scale = float(size) / GridMeshRenderer::max_dim;
      BoundingBox bb = boundingBox();

      if(!bb.collidesWithFrustum(frustum)) {
        return;
      }

      // if we can cover the whole area or if we are a leaf
      if (!bb.collidesWithSphere(cam.pos(), size) || level == 0) {
        renderer.render(cam, glm::vec2(x, z), scale, level);
      } else {
        bool btl = tl->collidesWithSphere(cam.pos(), size);
        bool btr = tr->collidesWithSphere(cam.pos(), size);
        bool bbl = bl->collidesWithSphere(cam.pos(), size);
        bool bbr = br->collidesWithSphere(cam.pos(), size);

        // Ask childs to render what we can't
        if (btl) {
          tl->render(cam, frustum, renderer);
        }
        if (btr) {
          tr->render(cam, frustum, renderer);
        }
        if (bbl) {
          bl->render(cam, frustum, renderer);
        }
        if (bbr) {
          br->render(cam, frustum, renderer);
        }

        // Render, what the childs didn't do
        renderer.render(cam, glm::vec2(x, z), scale, level, !btl, !btr, !bbl, !bbr);
      }
    }
  } root_;

 public:
  CDLODQuadTree(/*size_t terrain_size, GLubyte node_resolution*/)
      : renderer_(32)
      , root_(0, 0, 5) { }

  virtual void render(float time, const engine::Camera& cam) override {
    root_.render(cam, cam.frustum(), renderer_);
  }

};

} // namespace engine

#endif
