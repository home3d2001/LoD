// Copyright (c) 2014, Tamas Csala

#version 120

#include "sky.frag"

#export vec3 ApplyFog(vec3 color, vec3 c_pos);

const float kFogMin = 128.0;
const float kFogMax = 2048.0;

vec3 ApplyFog(vec3 color, vec3 c_pos) {
  vec3 fog_color = AmbientColor() / 3;
  float length_from_camera = length(c_pos);
  float alpha = clamp((length_from_camera - kFogMin) /
                      (kFogMax - kFogMin), 0, 1);

  return mix(color, fog_color, alpha);
}
