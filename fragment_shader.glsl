#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform vec3 diffuseColor;
uniform sampler2D textureSampler;

void main() {
  vec4 color = texture(textureSampler, TexCoords);
  // FragColor = color * vec4(diffuseColor, 1.0);
  FragColor = color;
}