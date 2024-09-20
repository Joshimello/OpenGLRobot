#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aUV;
layout(location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out vec3 UV;
out vec2 TexCoords;

uniform mat4 camera;
uniform mat4 rotation;
uniform vec4 position;
uniform mat4 scale;

void main() {
  gl_Position = camera * (rotation * (scale * vec4(aPos, 1.0)) + position);
  FragPos = vec3(position * vec4(aPos, 1.0));
  TexCoords = vec2(aUV.x, aUV.y);
  Normal = mat3(transpose(inverse(rotation))) * aNormal;
}