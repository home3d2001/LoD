#include "terrain.hpp"

using namespace oglwrap;

/* 0 -> max quality
   2 -> max performance */
extern const int PERFORMANCE;
const float xz_scale = (1 << PERFORMANCE)/2.0f;
static const glm::vec3 scale_vector = glm::vec3(xz_scale, 1.0f, xz_scale);

Terrain::Terrain(Skybox& skybox, Shadow& shadow)
  : vs_("terrain.vert")
  , fs_("terrain.frag")
  , uProjectionMatrix_(prog_, "uProjectionMatrix")
  , uCameraMatrix_(prog_, "uCameraMatrix")
  , uShadowCP_(prog_, "uShadowCP")
  , uSunData_(prog_, "uSunData")
  , uScales_(prog_, "uScales")
  , uOffset_(prog_, "uOffset")
  , uTexSize_(prog_, "uTexSize")
  , uMipmapLevel_(prog_, "uMipmapLevel")
  , uNumUsedShadowMaps_(prog_, "uNumUsedShadowMaps")
  , mesh_(std::string("terrain/mideu.rtd") + char(PERFORMANCE + '0'))
  , skybox_(skybox)
  , shadow_(shadow)
  , w_(mesh_.w)
  , h_(mesh_.h)
  , w(w_)
  , h(h_) {

  prog_ << vs_ << fs_ << skybox_.sky_fs;
  prog_.link().use();

  UniformSampler(prog_, "uEnvMap").set(0);
  UniformSampler(prog_, "uHeightMap").set(1);
  UniformSampler(prog_, "uGrassMap0").set(2);
  UniformSampler(prog_, "uGrassMap1").set(3);
  UniformSampler(prog_, "uGrassNormalMap").set(4);
  UniformSampler(prog_, "uShadowMap").set(5);
  Uniform<glm::ivec2>(prog_, "uShadowAtlasSize").set(shadow.getAtlasDimensions());

  prog_.validate();

  uScales_ = scale_vector;
  uTexSize_ = glm::ivec2(mesh_.w, mesh_.h);
}

glm::vec3 Terrain::getScales() const {
  return scale_vector;
}

unsigned char Terrain::fetchHeight(glm::ivec2 v) const {
  return mesh_.fetchHeight(v);
}

double Terrain::getHeight(double x, double y) const {
  return scale_vector.y * mesh_.getHeight(x / scale_vector.x, y / scale_vector.z);
}

void Terrain::screenResized(const glm::mat4& projMat, GLuint, GLuint) {
  prog_.use();
  uProjectionMatrix_ = projMat;
}

void Terrain::render(float time, const engine::Camera& cam) {
  prog_.use();
  uCameraMatrix_.set(cam);
  uSunData_.set(skybox_.getSunData());
  for(size_t i = 0; i < shadow_.getDepth(); ++i) {
    uShadowCP_[i] = shadow_.shadowCPs()[i];
  }
  uNumUsedShadowMaps_ = shadow_.getDepth();
  skybox_.env_map.active(0);
  skybox_.env_map.bind();
  shadow_.shadowTex().active(5);
  shadow_.shadowTex().bind();

  mesh_.render(cam.pos(), cam.forward(), uOffset_, uMipmapLevel_);

  shadow_.shadowTex().active(5);
  shadow_.shadowTex().unbind();
  skybox_.env_map.active(0);
  skybox_.env_map.unbind();
}


