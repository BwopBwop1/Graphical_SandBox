#version 430

layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding=0) buffer ssbo0 {
  vec4 positions[];
};

layout(location=0) uniform float dt;
layout(location=1) uniform int grid_size;
layout(location=2) uniform int chunk_size;
layout(location=3) uniform int chunk_side_length;
layout(location=4) uniform vec2 chunk_position;

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

void main() {

  uint index = gl_GlobalInvocationID.x;
  vec3 pos = positions[index].xyz;

  initRand(index);

  float ratio_x = (float(index % grid_size) / float(grid_size));
  float ratio_y = (float(index / grid_size) / float(grid_size));

  float chunk_s = (float(chunk_size) * (float(chunk_side_length) / 2.0));
  float cell_size = float(chunk_size) / float(grid_size);

  pos.x = (ratio_x * chunk_size) + (cell_size * 0.5) + ((chunk_size * chunk_position.x) - chunk_s);
  pos.z = (ratio_y * chunk_size) + (cell_size * 0.5) + ((chunk_size * chunk_position.y) - chunk_s);

  pos.x += (randValue() * 2.0 - 1.0) * (cell_size * 0.4);
  pos.z += (randValue() * 2.0 - 1.0) * (cell_size * 0.4);

  positions[index].xyz = pos;
}
