#define GLM_ENABLE_EXPERIMENTAL
#define TINYOBJLOADER_IMPLEMENTATION
#define M_PI 3.1415926535897932384626433832795
#include "glad.h" 
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <numbers> 
#include <vector>
#include <chrono>
#include <thread>
#include <stb/stb_image.h>
#include "tiny_obj_loader.h"

using namespace std;
using namespace std::chrono;
using namespace std::this_thread;
using namespace glm;

class Shape {
  vector<GLfloat> vertices;
  vector<GLuint> indices;
  vector<tinyobj::material_t> materials;
  GLuint vao, vbo, ebo;
  public:
  Shape(string path) {
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "./texture";
    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path, reader_config)) {
      if (!reader.Error().empty()) {
        cerr << "TinyObjReader: " << reader.Error();
      }
      exit(1);
    }
    if (!reader.Warning().empty()) {
      cout << "TinyObjReader: " << reader.Warning();
    }
    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    this->materials  = reader.GetMaterials();
    for (size_t s = 0; s < shapes.size(); s++) {
      size_t index_offset = 0;
      for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
        size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
        for (size_t v = 0; v < fv; v++) {
          tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
          tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
          tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
          tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];
          this->vertices.push_back(vx);
          this->vertices.push_back(vy);
          this->vertices.push_back(vz);
          if (idx.normal_index >= 0) {
            tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
            tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
            tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];
            this->vertices.push_back(nx);
            this->vertices.push_back(ny);
            this->vertices.push_back(nz);
          }
          if (idx.texcoord_index >= 0) {
            tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
            tinyobj::real_t ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];
            this->vertices.push_back(tx);
            this->vertices.push_back(ty);
          }
          this->indices.push_back(index_offset + v);
        }
        index_offset += fv;
      }
    }
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }
  void render(GLint diffuseLoc) {
    for (const auto& mat : this->materials) {
      glUniform3fv(diffuseLoc, 1, mat.diffuse);
      glBindVertexArray(vao);
      glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);      
    }
  }
  void remove() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
  }
};

class Texture {
  GLuint texture;
  public:
  Texture(const char *path) {
    int w, h, c; // width, height, channels
    unsigned char *byets = stbi_load(path, &w, &h, &c, 0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    GLenum formats[] = {GL_RED, 0, GL_RGB, GL_RGBA};
    GLenum format = formats[c - 1];
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, format, GL_UNSIGNED_BYTE, byets);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(byets);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  void render() {
    glBindTexture(GL_TEXTURE_2D, texture);
  }
};

class Camera {
  vec3 target = vec3(0.0f, 3.0f, 0.0f);
  vec3 up = vec3(0.0f, 10.0f, 0.0f);
  float hAngle = M_PI + 0.5f;
  float vAngle = 0.2f;
  float r = 20.0f;
  int w, h;
  float sens = 0.01f;
  public:
  Camera (int w, int h) {
    this->w = w;
    this->h = h;
  }
  void update (GLint viewLoc) {
    vec3 pos;
    pos.x = target.x + r * cos(vAngle) * sin(hAngle);
    pos.y = target.y + r * sin(vAngle);
    pos.z = target.z + r * cos(vAngle) * cos(hAngle);
    vec3 dir = normalize(pos - target);
    vec3 right = normalize(cross(up, dir));
    vec3 up = cross(dir, right);
    mat4 view = lookAt(pos, target, up);
    mat4 projection = perspective(radians(45.0f), (float)w / (float)h, 0.1f, 100.0f);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(projection * view));
  }
  void rotate (float hAngle, float vAngle) {
    this->hAngle += hAngle;
    this->vAngle += vAngle;
    this->vAngle = clamp(this->vAngle, -pi<float>() / 2 + 0.1f, pi<float>() / 2 - 0.1f);
  }
  void input(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) r -= 0.1;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) r += 0.1;
    static bool left_click = true;
    static double init_x, init_y;
    double curr_x, curr_y;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      if (!left_click) {
        glfwGetCursorPos(window, &init_x, &init_y);
        left_click = true;
      } else {
        glfwGetCursorPos(window, &curr_x, &curr_y);
        double dx = curr_x - init_x;
        double dy = curr_y - init_y;
        rotate(- dx * sens, dy * sens);
        init_x = curr_x;
        init_y = curr_y;
      }
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
      left_click = false;
    }
  }
};

const int TARGET_FPS = 120;
const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

int main() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow *window = glfwCreateWindow(800, 800, "uwu", NULL, NULL);
  glfwMakeContextCurrent(window);
  gladLoadGL();
  glViewport(0, 0, 800, 800);
  glEnable(GL_DEPTH_TEST);

  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  ifstream vertexShaderFile("vertex_shader.glsl");
  stringstream vertexShaderStream;
  vertexShaderStream << vertexShaderFile.rdbuf();
  string vertexShaderSource = vertexShaderStream.str();
  const char *vertexShaderSourceC = vertexShaderSource.c_str();
  glShaderSource(vertexShader, 1, &vertexShaderSourceC, NULL);
  glCompileShader(vertexShader);

  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
  }

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  ifstream fragmentShaderFile("fragment_shader.glsl");
  stringstream fragmentShaderStream;
  fragmentShaderStream << fragmentShaderFile.rdbuf();
  string fragmentShaderSource = fragmentShaderStream.str();
  const char *fragmentShaderSourceC = fragmentShaderSource.c_str();
  glShaderSource(fragmentShader, 1, &fragmentShaderSourceC, NULL);
  glCompileShader(fragmentShader);

  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success){
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
  }

  GLint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
      glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
  }

  glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

  GLint cameraLoc = glGetUniformLocation(shaderProgram, "camera");
  GLint rotationLoc = glGetUniformLocation(shaderProgram, "rotation");
  GLint positionLoc = glGetUniformLocation(shaderProgram, "position");
  GLint scaleLoc = glGetUniformLocation(shaderProgram, "scale");
  GLint textureLoc = glGetUniformLocation(shaderProgram, "texture");
  GLint diffuseLoc = glGetUniformLocation(shaderProgram, "diffuseColor");
  glUniform1i(textureLoc, 0);
  
  Shape headShape("obj/head.obj");
  Shape torsoShape("obj/torso.obj");
  Shape armLeftShape("obj/arm_left.obj");
  Shape armRightShape("obj/arm_right.obj");
  Shape legLeftShape("obj/leg_left.obj");
  Shape legRightShape("obj/leg_right.obj");
  Texture headTex("texture/head.png");
  Texture torsoTex("texture/torso.png");
  Camera camera(800, 800);

  while (!glfwWindowShouldClose(window)) {
    auto startTime = high_resolution_clock::now();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    camera.input(window);
    camera.update(cameraLoc);

    mat4 rotationMat, scaleMat;

    bool isAnimate = true;

    if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_SPACE)) {
      isAnimate = !isAnimate;
    }

    float headRotation = 0.0f;
    float legLeftZ = 0.0f;
    float legLeftY = 0.0f;
    float legRightZ = 0.0f;
    float legRightY = 0.0f;
    float armLeftZ = 0.0f;
    float armLeftY = 0.0f;
    float armRightZ = 0.0f;
    float armRightY = 0.0f;
    if (isAnimate) {
      headRotation = sin(glfwGetTime() * 2.0f) * 0.8f;
      legLeftZ = sin(glfwGetTime() * 2.0f) * 0.5f;
      legLeftY = std::max(-cos(glfwGetTime() * 2.0f) * 0.5, 0.0);
      legRightZ = -sin(glfwGetTime() * 2.0f) * 0.5f;
      legRightY = std::max(cos(glfwGetTime() * 2.0f) * 0.5, 0.0);
      armLeftZ = -sin(glfwGetTime());
      armLeftY = std::max(cos(glfwGetTime()), 0.0);
      armRightZ = sin(glfwGetTime());
      armRightY = std::max(-cos(glfwGetTime()), 0.0);
    }

    // head
    headTex.render();
    rotationMat = rotate(mat4(1.0f), radians(headRotation), vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, value_ptr(rotationMat));
    glUniform4f(positionLoc, 0.0f, -1.7f, 0.4f, 0.0f);
    scaleMat = scale(mat4(1.0f), vec3(1.0f, 1.0f, 1.0f));
    glUniformMatrix4fv(scaleLoc, 1, GL_FALSE, value_ptr(scaleMat));
    headShape.render(diffuseLoc);

    // torso
    torsoTex.render();
    rotationMat = rotate(mat4(1.0f), radians(0.0f), vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, value_ptr(rotationMat));
    glUniform4f(positionLoc, 0.0f, 0.0f, 0.0f, 0.0f);
    scaleMat = scale(mat4(1.0f), vec3(1.0f, 1.0f, 1.0f));
    glUniformMatrix4fv(scaleLoc, 1, GL_FALSE, value_ptr(scaleMat));
    torsoShape.render(diffuseLoc);

    // leg left
    torsoTex.render();
    rotationMat = rotate(mat4(1.0f), radians(0.0f), vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, value_ptr(rotationMat));
    glUniform4f(positionLoc, -1.5f, legLeftY, legLeftZ, 0.0f);
    scaleMat = scale(mat4(1.0f), vec3(1.0f, 1.0f, 1.0f));
    glUniformMatrix4fv(scaleLoc, 1, GL_FALSE, value_ptr(scaleMat));
    legLeftShape.render(diffuseLoc);

    // leg right
    torsoTex.render();
    rotationMat = rotate(mat4(1.0f), radians(0.0f), vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, value_ptr(rotationMat));
    glUniform4f(positionLoc, 1.9f, legRightY, legRightZ, 0.0f);
    scaleMat = scale(mat4(1.0f), vec3(1.0f, 1.0f, 1.0f));
    glUniformMatrix4fv(scaleLoc, 1, GL_FALSE, value_ptr(scaleMat));
    legRightShape.render(diffuseLoc);

    // arm left
    torsoTex.render();
    rotationMat = rotate(mat4(1.0f), radians(0.0f), vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, value_ptr(rotationMat));
    glUniform4f(positionLoc, -3.2f, armLeftY, armLeftZ, 0.0f);
    scaleMat = scale(mat4(1.0f), vec3(1.0f, 1.0f, 1.0f));
    glUniformMatrix4fv(scaleLoc, 1, GL_FALSE, value_ptr(scaleMat));
    armLeftShape.render(diffuseLoc);

    // arm right
    torsoTex.render();
    rotationMat = rotate(mat4(1.0f), radians(0.0f), vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, value_ptr(rotationMat));
    glUniform4f(positionLoc, 4.0f, armRightY, armRightZ, 0.0f);
    scaleMat = scale(mat4(1.0f), vec3(1.0f, 1.0f, 1.0f));
    glUniformMatrix4fv(scaleLoc, 1, GL_FALSE, value_ptr(scaleMat));
    armRightShape.render(diffuseLoc);

    glfwSwapBuffers(window);
    glfwPollEvents();

    auto endTime = high_resolution_clock::now();
    duration<double> elapsedTime = endTime - startTime;
    double sleepTime = TARGET_FRAME_TIME - elapsedTime.count();
    if (sleepTime > 0) {
      sleep_for(duration<double>(sleepTime));
    }
  }

  armLeftShape.remove();
  armRightShape.remove();
  headShape.remove();
  legLeftShape.remove();
  legRightShape.remove();
  torsoShape.remove();
  glDeleteProgram(shaderProgram);
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;

}