#version 430

// NOTE: This should be the exact same shader that is ran by raylib
// As the default renderer.

layout(location=0) in vec3 vertexPosition;
layout(location=1) in vec2 LocalUV;

layout(location=0) uniform mat4 projectionMatrix;
layout(location=1) uniform mat4 viewMatrix;
layout(location=2) uniform vec2 chunk_pos;
layout(location=3) uniform float time;
layout(location=4) uniform float d_scale;

layout(std430, binding=0) buffer ssbo0 { vec4 positions[]; };

out vec4 fragColor;

const float TAU = 6.283185;

uint rng_state;

void wang_hash(uint seed) {
    rng_state = (seed ^ 61) ^ (seed >> 16);
    rng_state *= 9;
    rng_state = rng_state ^ (rng_state >> 4);
    rng_state *= 0x27d4eb2d;
    rng_state = rng_state ^ (rng_state >> 15);
}

uint rand_xorshift() {
    rng_state ^= (rng_state << 13);
    rng_state ^= (rng_state >> 17);
    rng_state ^= (rng_state << 5);

    return rng_state;
}

float randValue() {
    return rand_xorshift() * (1.0 / 4294967296.0);
}
void initRand(uint seed) {
    wang_hash(seed);
}

float randValue(uint seed) {
    initRand(seed);
    return randValue();
}

vec4 RotateAroundYInDegrees(vec4 vertex, float degrees) {
  float alpha = degrees * (TAU / 360.0);

  float sina = sin(alpha); 
  float cosa = cos(alpha);

  mat2 m = mat2(cosa, -sina, sina, cosa);

  return vec4(m * vertex.xz, vertex.yw).xzyw;
}

vec4 RotateAroundXInDegrees (vec4 vertex, float degrees) {
  float alpha = degrees * (TAU / 360.0);

  float sina = sin(alpha); 
  float cosa = cos(alpha);

  mat2 m = mat2(cosa, -sina, sina, cosa);

  return vec4(m * vertex.yz, vertex.xw).zxyw;
}

void main() {

  initRand(uint(gl_InstanceID * TAU));

  vec4 worldPosition = vec4(vertexPosition, 1.0);
  worldPosition.xyz *= 0.5;

  float movement = LocalUV.y * LocalUV.y * mix(0.8, 1.0, randValue()) * (sin(time) * 0.1);
  worldPosition.xz += movement;

  worldPosition = RotateAroundYInDegrees(worldPosition, (45.0 * (randValue() - 0.5)));


  worldPosition.xyz += positions[gl_InstanceID].xyz;

  float r = randValue(uint(chunk_pos.x + chunk_pos.y)) * LocalUV.y;
  float g = randValue() * LocalUV.y;
  float b = randValue() * LocalUV.y;

  
  fragColor = vec4(r, g, b, 1.0);
  // I have no clue why this works
  //gl_Position = worldPosition * projectionMatrix * viewMatrix; 
  // God am I so stupid. Forever Documenting my stupidity. - Bwop
  gl_Position = projectionMatrix * viewMatrix * worldPosition; 
}
