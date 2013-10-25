#ifndef LOD_AYUMI_HPP_
#define LOD_AYUMI_HPP_

#if defined(__APPLE__)
#include <OpenGL/glew.h>
#else
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/glew.h>
#endif

#include "oglwrap/oglwrap.hpp"
#include "oglwrap/mesh/animatedMesh.hpp"
#include "oglwrap/utils/camera.hpp"

#include "charmove.hpp"
#include "skybox.hpp"
#include "shadow.hpp"

extern const float GRAVITY;
/* 0 -> max quality
   4 -> max performance */
extern const int PERFORMANCE;

class Ayumi {
  oglwrap::AnimatedMesh mesh_;
  oglwrap::Program prog_, shadow_prog_;

  oglwrap::LazyUniform<glm::mat4> uProjectionMatrix_, uCameraMatrix_, uModelMatrix_, uBones_, uShadowCP_;
  oglwrap::LazyUniform<glm::mat4> shadow_uMCP_, shadow_uBones_;
  oglwrap::LazyUniform<glm::vec4> uSunData_;
  oglwrap::LazyUniform<int> uNumUsedShadowMaps_, uShadowSoftness_;

  Skybox& skybox_;
public:
  Ayumi(Skybox& skybox, const CharacterMovement& charmove);
  oglwrap::AnimatedMesh& getMesh();
  void resize(glm::mat4 projMat);
  void updateStatus(float time, const CharacterMovement& charmove);
  void shadowRender(float time, Shadow& shadow, const CharacterMovement& charmove);
  void render(float time, const oglwrap::Camera& cam,
              const CharacterMovement& charmove, const Shadow& shadow);
};

#endif // LOD_AYUMI_HPP_
