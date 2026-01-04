#pragma once

#include "resources/ray.hpp"
#include <glm/glm.hpp>

enum class Direction { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

class Camera {
public:
    Camera(float fov, float near, float far);

    void setup(const glm::vec3& position, const glm::vec3& direction);
    bool update(float ts);
    void resize(uint32_t width, uint32_t height);

    Ray generateRay(const glm::ivec2& coord, const glm::vec2& offset) const;

    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};

    glm::vec3 position;
    glm::vec3 direction;

private:
    uint32_t width_ = 0, height_ = 0;

    float fov_ = 60.0f;
    float near_ = 0.1f;
    float far_ = 100.0f;

    float theta_, phi_;

    bool is_cursor_locked_ = false;

    void move(float dt, Direction dir);
    void turn(const glm::vec2& delta);
    void zoom(float delta);
    void update();
};