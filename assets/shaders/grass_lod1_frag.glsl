#version 430

out vec4 finalColor;

in vec4 fragColor;

void main() {
  finalColor = fragColor;
}
