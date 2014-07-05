// Copyright (c) 2014, Tamas Csala

#ifndef ENGINE_GAME_OBJECT_H_
#define ENGINE_GAME_OBJECT_H_

#include <set>
#include <memory>
#include <vector>
#include <algorithm>

#include "./camera.h"
#include "./rigid_body.h"

namespace engine {

class Scene;
class Behaviour;

class GameObject {
 public:
  Transform transform;
  std::unique_ptr<RigidBody> rigid_body;

  explicit GameObject(Scene* scene)
      : scene_(scene), layer_(0), group_(0), enabled_(true) {
    sorted_components_.insert(this);
  }
  virtual ~GameObject() {}

  void addRigidBody(const HeightMapInterface& height_map,
                    double starting_height = NAN) {
    rigid_body = std::unique_ptr<RigidBody>(
        new RigidBody(transform, height_map, starting_height));
  }

  template<typename T, typename... Args>
  T* addComponent(Args&&... args) {
    static_assert(std::is_base_of<GameObject, T>::value, "Unknown type");

    try {
      T *obj = new T(scene_, std::forward<Args>(args)...);
      obj->transform.set_parent(&transform);
      obj->parent_ = this;
      components_.push_back(std::unique_ptr<GameObject>(obj));
      components_just_enabled_.push_back(obj);

      return obj;
    } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      return nullptr;
    }
  }

  GameObject* parent() { return parent_; }
  const GameObject* parent() const { return parent_; }

  Scene* scene() { return scene_; }
  const Scene* scene() const { return scene_; }
  void set_scene(Scene* scene) { scene_ = scene; }

  bool enabled() const { return enabled_; }
  void set_enabled(bool value) {
    enabled_ = value;
    if (value) {
      components_just_enabled_.push_back(this);
      if (parent_) {
        parent_->components_just_enabled_.push_back(this);
      }
    } else {
      components_just_disabled_.push_back(this);
      if (parent_) {
        parent_->components_just_disabled_.push_back(this);
      }
    }
  }

  int layer() const { return layer_; }
  void set_layer(int value) { layer_ = value; }

  int group() const { return group_; }
  void set_group(int value) {
    group_ = value;
    components_just_enabled_.push_back(this);
    components_just_disabled_.push_back(this);
    if (parent_) {
      parent_->components_just_enabled_.push_back(this);
      parent_->components_just_disabled_.push_back(this);
    }
  }

  virtual void shadowRender() {}
  virtual void render() {}
  virtual void render2D() {}
  virtual void screenResized(size_t width, size_t height) {}

  virtual void shadowRenderAll();
  virtual void renderAll();
  virtual void render2DAll();
  virtual void screenResizedAll(size_t width, size_t height);

  virtual void updateAll();
  virtual void keyActionAll(int key, int scancode, int action, int mods);
  virtual void charTypedAll(unsigned codepoint);
  virtual void mouseScrolledAll(double xoffset, double yoffset);
  virtual void mouseButtonPressedAll(int button, int action, int mods);
  virtual void mouseMovedAll(double xpos, double ypos);

 protected:
  Scene* scene_;
  GameObject* parent_;
  std::vector<std::unique_ptr<GameObject>> components_;
  std::vector<GameObject*> components_just_enabled_, components_just_disabled_;

  struct CompareGameObjects {
    bool operator() (GameObject* x, GameObject* y) const {
      if (x->group() == y->group()) {
        if (y->parent() == x) {
          return true;  // we should render the parent before the children
        } else {
          return false;
        }
      } else {
        return x->group() < y->group();
      }
    }
  };

  std::multiset<GameObject*, CompareGameObjects> sorted_components_;
  int layer_, group_;
  bool enabled_;

  void updateSortedComponents();
};

}  // namespace engine

#endif
