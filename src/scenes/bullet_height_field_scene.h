// Copyright (c) 2014, Tamas Csala

#ifndef LOD_SCENES_BULLET_HEIGHT_FIELD_SCENE_H_
#define LOD_SCENES_BULLET_HEIGHT_FIELD_SCENE_H_

#include <vector>
#include <algorithm>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

// fck windows.h
#undef min
#undef max

#include "../engine/misc.h"
#include "../engine/scene.h"
#include "../engine/camera.h"
#include "../engine/behaviour.h"
#include "../engine/debug/debug_shape.h"
#include "../engine/gui/label.h"

#include "../terrain.h"
#include "../after_effects.h"
#include "../fps_display.h"
#include "../loading_screen.h"
#include "./main_scene.h"

class BulletRigidBody : public engine::Behaviour, public btMotionState {
 public:
  BulletRigidBody(GameObject* parent, float mass,
                  btCollisionShape* shape, bool ignore_rotation = false)
      : Behaviour(parent), ignore_rotation_(ignore_rotation) {
    init(mass, shape);
  }

  BulletRigidBody(GameObject* parent, float mass, btCollisionShape* shape,
                  const glm::vec3& pos, bool ignore_rotation = false)
      : Behaviour(parent), ignore_rotation_(ignore_rotation) {
    transform()->set_pos(pos);
    init(mass, shape);
  }

  BulletRigidBody(GameObject* parent, float mass, btCollisionShape* shape,
                  const glm::vec3& pos, const glm::fquat& rot)
      : Behaviour(parent), ignore_rotation_(false) {
    transform()->set_pos(pos);
    transform()->set_rot(rot);
    init(mass, shape);
  }

  virtual ~BulletRigidBody() {
    scene_->world()->removeCollisionObject(bt_rigid_body_.get());
  }

  btRigidBody* bt_rigid_body() { return bt_rigid_body_.get(); }
  const btRigidBody* bt_rigid_body() const { return bt_rigid_body_.get(); }

 private:
  std::unique_ptr<btCollisionShape> shape_;
  std::unique_ptr<btRigidBody> bt_rigid_body_;
  bool ignore_rotation_;

  void init(float mass, btCollisionShape* shape) {
    shape_ = std::unique_ptr<btCollisionShape>(shape);
    btVector3 inertia(0, 0, 0);
    shape_->calculateLocalInertia(mass, inertia);
    btRigidBody::btRigidBodyConstructionInfo
        info{mass, this, shape_.get(), inertia};
    bt_rigid_body_ = engine::make_unique<btRigidBody>(info);
    bt_rigid_body_->setUserPointer(parent_);
    if (mass == 0.0f) { bt_rigid_body_->setRestitution(1.0f); }
    scene_->world()->addRigidBody(bt_rigid_body_.get());
  }

  virtual void getWorldTransform(btTransform &t) const override {
    const glm::vec3& pos = transform()->pos();
    t.setOrigin(btVector3{pos.x, pos.y, pos.z});
    if (!ignore_rotation_) {
      const glm::fquat& rot = transform()->rot();
      t.setRotation(btQuaternion{rot.x, rot.y, rot.z, rot.w});
    }
  }

  virtual void setWorldTransform(const btTransform &t) override {
    const btVector3& o = t.getOrigin();
    parent_->transform()->set_pos(glm::vec3(o.x(), o.y(), o.z()));
    if (!ignore_rotation_) {
      const btQuaternion& r = t.getRotation();
      parent_->transform()->set_rot(glm::quat(r.getW(), r.getX(),
                                              r.getY(), r.getZ()));
    }
  }
};

class HeightField : public engine::GameObject {
 public:
  explicit HeightField(GameObject* parent) : GameObject(parent) {
    Terrain* terrain = addComponent<Terrain>();
    const auto& height_map = terrain->height_map();
    int w = height_map.w(), h = height_map.h();
    GLubyte *data = new GLubyte[w*h];
    for (int x = 0; x < w; ++x) {
      for (int y = 0; y < h; ++y) {
        data[y*w + x] = height_map.heightAt(x, y);
      }
    }

    btCollisionShape* shape = new btHeightfieldTerrainShape{
        height_map.w(), height_map.h(), data,
        1, 0, 256, 1, PHY_UCHAR, true};

    glm::vec3 pos{height_map.w()/2.0f, 128, height_map.h()/2.0f};
    addComponent<BulletRigidBody>(0.0f, shape, pos);
  }
};

class BulletCube : public engine::Behaviour {
 public:
  explicit BulletCube(GameObject* parent, const glm::vec3& pos,
                   const glm::vec3& v, const glm::quat& rot = glm::quat{})
      : Behaviour(parent) {
    transform()->set_pos(pos);
    transform()->set_rot(rot);
    btVector3 half_extents(0.5f, 0.5f, 0.5f);
    btCollisionShape* shape = new btBoxShape(half_extents);
    auto rbody = addComponent<BulletRigidBody>(1.0f, shape);
    auto bt_rigid_body = rbody->bt_rigid_body();
    bt_rigid_body->setLinearVelocity(btVector3(v.x, v.y, v.z));
    bt_rigid_body->setRestitution(0.3f);
    // Continous Collision Detection (CCD) is needed, when the cubes move more
    // than half their extents (0.5f) in a frame, or otherwise, they would
    // fall through other objects
    bt_rigid_body->setCcdMotionThreshold(0.5f);
    bt_rigid_body->setCcdSweptSphereRadius(0.25f);
    mesh_ = addComponent<engine::debug::Cube>(glm::vec3(0.5, 0.0, 0.0));
  }

  virtual void collision(const GameObject* other) override {
    addColor(glm::vec3{0.0f, 0.02f, 0.0f});
  }

 private:
  engine::debug::Cube* mesh_;

  virtual void update() override {
    glm::vec3 color = mesh_->color();
    color = glm::vec3(color.r, std::min(0.98f*color.g, 0.9f),
                               std::min(0.98f*color.b, 0.9f));
    mesh_->set_color(color);
  }

  void addColor(const glm::vec3& color) {
    mesh_->set_color(glm::clamp(mesh_->color() + color,
                                glm::vec3{}, glm::vec3{1}));
  }
};

class BulletSphere : public engine::Behaviour {
 public:
  explicit BulletSphere(GameObject* parent, const glm::vec3& pos,
                        const glm::vec3& v)
      : Behaviour(parent) {
    transform()->set_pos(pos);
    btCollisionShape* shape = new btSphereShape(0.5f);
    auto rbody = addComponent<BulletRigidBody>(1.0f, shape);
    auto bt_rigid_body = rbody->bt_rigid_body();
    bt_rigid_body->setLinearVelocity(btVector3(v.x, v.y, v.z));
    bt_rigid_body->setRestitution(0.5f);
    bt_rigid_body->setCcdMotionThreshold(0.5f);
    bt_rigid_body->setCcdSweptSphereRadius(0.25f);
    mesh_ = addComponent<engine::debug::Sphere>(glm::vec3(0.5, 0.0, 0.0));
  }

  virtual void collision(const GameObject* other) override {
    addColor(glm::vec3{0.0f, 0.02f, 0.0f});
  }

 private:
  engine::debug::Sphere* mesh_;

  virtual void update() override {
    glm::vec3 color = mesh_->color();
    color = glm::vec3(color.r, std::min(0.98f*color.g, 0.9f),
                               std::min(0.98f*color.b, 0.9f));
    mesh_->set_color(color);
  }

  void addColor(const glm::vec3& color) {
    mesh_->set_color(glm::clamp(mesh_->color() + color,
                                glm::vec3{}, glm::vec3{1}));
  }
};

class BulletFreeFlyCamera : public engine::FreeFlyCamera {
 public:
  BulletFreeFlyCamera(GameObject* parent, float fov, float z_near,
                      float z_far, const glm::vec3& pos,
                      const glm::vec3& target = glm::vec3(),
                      float speed_per_sec = 5.0f,
                      float mouse_sensitivity = 1.0f)
      : FreeFlyCamera(parent, fov, z_near, z_far, pos, target,
                      speed_per_sec, mouse_sensitivity) {
    btCollisionShape* shape = new btSphereShape(2.0f);
    auto rbody = addComponent<BulletRigidBody>(0.001f, shape, true);
    bt_rigid_body_ = rbody->bt_rigid_body();
    bt_rigid_body_->setGravity(btVector3{0, 0, 0});

    bt_rigid_body_->setCcdMotionThreshold(2.0f);
    bt_rigid_body_->setCcdSweptSphereRadius(0.5f);
  }

 private:
  btRigidBody* bt_rigid_body_;

  virtual void update() override {
    glm::dvec2 cursor_pos;
    GLFWwindow* window = scene_->window();
    glfwGetCursorPos(window, &cursor_pos.x, &cursor_pos.y);
    static glm::dvec2 prev_cursor_pos;
    glm::dvec2 diff = cursor_pos - prev_cursor_pos;
    prev_cursor_pos = cursor_pos;

    // We get invalid diff values at the startup
    if (first_call_) {
      diff = glm::dvec2(0, 0);
      first_call_ = false;
    }

    const float dt = scene_->camera_time().dt;

    // Mouse movement - update the coordinate system
    if (diff.x || diff.y) {
      float dx(diff.x * mouse_sensitivity_ * dt / 16);
      float dy(-diff.y * mouse_sensitivity_ * dt / 16);

      // If we are looking up / down, we don't want to be able
      // to rotate to the other side
      float dot_up_fwd = glm::dot(transform()->up(), transform()->forward());
      if (dot_up_fwd > cos_max_pitch_angle_ && dy > 0) {
        dy = 0;
      }
      if (dot_up_fwd < -cos_max_pitch_angle_ && dy < 0) {
        dy = 0;
      }

      transform()->set_forward(transform()->forward() +
                               transform()->right()*dx +
                               transform()->up()*dy);
    }

    // Calculate the offset
    float ds = dt * speed_per_sec_;
    glm::vec3 offset;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
      offset = transform()->forward() * ds;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      offset = -transform()->forward() * ds;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
      offset = transform()->right() * ds;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
      offset = -transform()->right() * ds;
    }
    if (scene_->game_time().dt) {
      offset /= float(scene_->game_time().dt);
    }

    // Update the "position"
    bt_rigid_body_->setLinearVelocity(btVector3{offset.x, offset.y, offset.z});
  }
};

class BulletHeightFieldScene : public engine::Scene {
  void shootCube(float speed = 20.0f) {
    auto cam = camera();
    glm::vec3 pos = cam->transform()->pos() + 3.0f*cam->transform()->forward();
    addComponent<BulletSphere>(pos, speed*cam->transform()->forward());
  }

  void dropCubes() {
    auto cam = camera();
    glm::vec3 base_pos = cam->transform()->pos() - 2.0f*cam->transform()->up();
    for (int x = -2; x <= 2; ++x) {
      for (int y = -2; y <= 2; ++y) {
        addComponent<BulletCube>(base_pos + 1.5f*glm::vec3(x, 0, y), glm::vec3());
      }
    }
  }

 public:
  BulletHeightFieldScene() {
    glfwSetInputMode(window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    LoadingScreen().render();
    glfwSwapBuffers(window());

    collision_config_ = engine::make_unique<btDefaultCollisionConfiguration>();
    dispatcher_ =
        engine::make_unique<btCollisionDispatcher>(collision_config_.get());
    // Dynamic bounding volume tree broadphase
    // Alternatively I could use btAxisSweep3 for finite bound worlds.
    broadphase_ = engine::make_unique<btDbvtBroadphase>();
    solver_ = engine::make_unique<btSequentialImpulseConstraintSolver>();
    world_ = engine::make_unique<btDiscreteDynamicsWorld>(
        dispatcher_.get(), broadphase_.get(),
        solver_.get(), collision_config_.get());
    world_->setGravity(btVector3(0, -9.81, 0));

    world_->getSolverInfo().m_numIterations /= 2; // ++performance

    auto skybox = addComponent<Skybox>();
    addComponent<HeightField>();
    auto after_effects = addComponent<AfterEffects>(skybox);
    after_effects->set_group(1);

    auto cam = addComponent<BulletFreeFlyCamera>(M_PI/3, 1, 3000,
        glm::vec3(2050, 200, 2050), glm::vec3(2048, 200, 2048), 20, 5);
    set_camera(cam);

    auto label = addComponent<engine::gui::Label>(
        L"Press left click to shoot a slow, or right click to shoot a fast sphere.", glm::vec2(0, -0.85));
    label->set_vertical_alignment(engine::gui::Font::VerticalAlignment::kCenter);
    label->set_font_size(16);
    label->set_group(2);

    auto label2 = addComponent<engine::gui::Label>(
        L"Press space to drop a bunch of cubes.", glm::vec2(0, -0.9));
    label2->set_vertical_alignment(engine::gui::Font::VerticalAlignment::kCenter);
    label2->set_font_size(20);
    label2->set_group(2);


    auto label3 = addComponent<engine::gui::Label>(L"You can load the main "
      L"scene by pressing the home button.", glm::vec2(0, -0.95));
    label3->set_vertical_alignment(engine::gui::Font::VerticalAlignment::kCenter);
    label3->set_font_size(14);
    label3->set_group(2);

    auto fps = addComponent<FpsDisplay>();
    fps->set_group(2);
  }

  void findCollisions() {
    int num_manifolds = world_->getDispatcher()->getNumManifolds();
    for (int i = 0; i < num_manifolds; ++i) {
      btPersistentManifold* contact_manifold =
         world_->getDispatcher()->getManifoldByIndexInternal(i);
      const btCollisionObject* obA = contact_manifold->getBody0();
      const btCollisionObject* obB = contact_manifold->getBody1();

      int num_contacts = contact_manifold->getNumContacts();
      for (int j = 0; j < num_contacts; ++j) {
        btManifoldPoint& pt = contact_manifold->getContactPoint(j);
        if (pt.getDistance() < 1e-3f) {
          // const btVector3& ptA = pt.getPositionWorldOnA();
          // const btVector3& ptB = pt.getPositionWorldOnB();
          // const btVector3& normalOnB = pt.m_normalWorldOnB;
          auto go1 = (engine::GameObject*)obA->getUserPointer();
          auto go2 = (engine::GameObject*)obB->getUserPointer();
          if (go1) { go1->collisionAll(go2); }
          if (go2) { go2->collisionAll(go1); }
        }
      }
    }
  }

  virtual void update() override {
    // With CCD, there's no need to run step simulation more than one times.
    world_->stepSimulation(game_time().dt, 5);
    findCollisions();
    Scene::update();
  }

  virtual void keyAction(int key, int scancode, int action, int mods) override {
    if (action == GLFW_PRESS) {
      if (key == GLFW_KEY_SPACE) {
        dropCubes();
      } else if (key == GLFW_KEY_HOME) {
        engine::GameEngine::LoadScene<MainScene>();
      }
    }
  }

  virtual void mouseButtonPressed(int button, int action, int mods) override {
    if (action == GLFW_PRESS) {
      if (button == GLFW_MOUSE_BUTTON_LEFT) {
        shootCube();
      } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        shootCube(200.0f);
      }
    }
  }
};

#endif