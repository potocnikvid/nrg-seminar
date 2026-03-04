#include "pbr_renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>

static const float PI = 3.14159265359f;

static Mesh uploadMesh(const std::vector<float>& data,
                       const std::vector<unsigned int>& indices) {
    Mesh m{};
    m.indexCount = static_cast<int>(indices.size());

    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);

    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    int stride = 8 * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
    return m;
}

Mesh createSphere(int xSegments, int ySegments) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uv;
    std::vector<unsigned int> indices;

    for (int y = 0; y <= ySegments; ++y) {
        for (int x = 0; x <= xSegments; ++x) {
            float xSeg = static_cast<float>(x) / xSegments;
            float ySeg = static_cast<float>(y) / ySegments;
            float xPos = std::cos(xSeg * 2.0f * PI) * std::sin(ySeg * PI);
            float yPos = std::cos(ySeg * PI);
            float zPos = std::sin(xSeg * 2.0f * PI) * std::sin(ySeg * PI);
            positions.emplace_back(xPos, yPos, zPos);
            normals.emplace_back(xPos, yPos, zPos);
            uv.emplace_back(xSeg, ySeg);
        }
    }
    for (int y = 0; y < ySegments; ++y) {
        for (int x = 0; x < xSegments; ++x) {
            unsigned int i0 = y * (xSegments + 1) + x;
            unsigned int i1 = (y + 1) * (xSegments + 1) + x;
            indices.push_back(i1);
            indices.push_back(i0);
            indices.push_back(i0 + 1);
            indices.push_back(i1);
            indices.push_back(i0 + 1);
            indices.push_back(i1 + 1);
        }
    }

    std::vector<float> data;
    for (size_t i = 0; i < positions.size(); ++i) {
        data.push_back(positions[i].x); data.push_back(positions[i].y); data.push_back(positions[i].z);
        data.push_back(normals[i].x);   data.push_back(normals[i].y);   data.push_back(normals[i].z);
        data.push_back(uv[i].x);        data.push_back(uv[i].y);
    }
    return uploadMesh(data, indices);
}

Mesh createCube() {
    struct Vert { glm::vec3 p, n; glm::vec2 uv; };
    auto face = [](glm::vec3 n, glm::vec3 right, glm::vec3 up) {
        std::vector<Vert> verts;
        glm::vec3 corners[4] = {
            n - right - up, n + right - up,
            n + right + up, n - right + up
        };
        glm::vec2 uvs[4] = {{0,0},{1,0},{1,1},{0,1}};
        for (int i = 0; i < 4; ++i) verts.push_back({corners[i], n, uvs[i]});
        return verts;
    };

    std::vector<float> data;
    std::vector<unsigned int> indices;
    unsigned int base = 0;

    auto addFace = [&](glm::vec3 n, glm::vec3 r, glm::vec3 u) {
        auto verts = face(n, r, u);
        for (auto& v : verts) {
            data.push_back(v.p.x); data.push_back(v.p.y); data.push_back(v.p.z);
            data.push_back(v.n.x); data.push_back(v.n.y); data.push_back(v.n.z);
            data.push_back(v.uv.x); data.push_back(v.uv.y);
        }
        indices.push_back(base); indices.push_back(base+1); indices.push_back(base+2);
        indices.push_back(base); indices.push_back(base+2); indices.push_back(base+3);
        base += 4;
    };

    addFace({ 0, 0, 1}, { 1, 0, 0}, {0, 1, 0});
    addFace({ 0, 0,-1}, {-1, 0, 0}, {0, 1, 0});
    addFace({ 1, 0, 0}, { 0, 0,-1}, {0, 1, 0});
    addFace({-1, 0, 0}, { 0, 0, 1}, {0, 1, 0});
    addFace({ 0, 1, 0}, { 1, 0, 0}, {0, 0,-1});
    addFace({ 0,-1, 0}, { 1, 0, 0}, {0, 0, 1});

    return uploadMesh(data, indices);
}

Mesh createTorus(float R, float r, int rings, int sides) {
    std::vector<float> data;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= rings; ++i) {
        float u = static_cast<float>(i) / rings;
        float theta = u * 2.0f * PI;
        float ct = std::cos(theta), st = std::sin(theta);

        for (int j = 0; j <= sides; ++j) {
            float v = static_cast<float>(j) / sides;
            float phi = v * 2.0f * PI;
            float cp = std::cos(phi), sp = std::sin(phi);

            float px = (R + r * cp) * ct;
            float py = r * sp;
            float pz = (R + r * cp) * st;

            float nx = cp * ct;
            float ny = sp;
            float nz = cp * st;

            data.push_back(px); data.push_back(py); data.push_back(pz);
            data.push_back(nx); data.push_back(ny); data.push_back(nz);
            data.push_back(u);  data.push_back(v);
        }
    }

    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < sides; ++j) {
            unsigned int a = i * (sides + 1) + j;
            unsigned int b = (i + 1) * (sides + 1) + j;
            indices.push_back(a); indices.push_back(b);     indices.push_back(b + 1);
            indices.push_back(a); indices.push_back(b + 1); indices.push_back(a + 1);
        }
    }

    return uploadMesh(data, indices);
}

glm::vec3 Camera::getPosition() const {
    float x = distance * std::cos(glm::radians(pitch)) * std::sin(glm::radians(yaw));
    float y = distance * std::sin(glm::radians(pitch));
    float z = distance * std::cos(glm::radians(pitch)) * std::cos(glm::radians(yaw));
    return target + glm::vec3(x, y, z);
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(getPosition(), target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
}
