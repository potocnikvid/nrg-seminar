#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

struct Mesh {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    int    indexCount;
};

Mesh createSphere(int xSegments = 64, int ySegments = 64);
Mesh createCube();
Mesh createTorus(float outerRadius = 1.0f, float innerRadius = 0.4f,
                 int rings = 48, int sides = 24);

struct Camera {
    float yaw      = 0.0f;
    float pitch     = 0.0f;
    float distance  = 8.0f;
    glm::vec3 target = glm::vec3(0.0f);

    glm::vec3 getPosition() const;
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect) const;
};
