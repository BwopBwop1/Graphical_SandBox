#version 430 core

layout(binding = 0, row_major) uniform CameraData {
 mat4 ViewTransform;
};

layout(location=0) in vec3 vertexPos;


void main() {
  gl_Position = ViewTransform * vec4(vertexPos, 1.0f);
}
