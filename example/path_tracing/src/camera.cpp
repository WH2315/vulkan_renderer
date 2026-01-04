#include "camera.hpp"
#include "application.hpp"
#include "tools/random.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

Camera::Camera(float fov, float near, float far) : fov_(fov), near_(near), far_(far) {}

void Camera::setup(const glm::vec3& position, const glm::vec3& direction) {
    this->position = position;
    this->direction = direction;
    theta_ = glm::degrees(glm::acos(direction.y));
    if (glm::abs(direction.y) == 1.0f) {
        phi_ = 0.0f;
    } else {
        phi_ = glm::degrees(glm::atan(direction.z, direction.x));
        if (phi_ < 0.0f) phi_ += 360.0f;
    }
}

Ray Camera::generateRay(const glm::ivec2& coord, const glm::vec2& offset) const {
    glm::vec2 ndc = {
        (static_cast<float>(coord.x) + offset.x) / static_cast<float>(width_),
        (static_cast<float>(coord.y) + offset.y) / static_cast<float>(height_)};
    // [0, 1] -> [-1, 1]
    ndc = ndc * 2.0f - 1.0f;

    glm::vec4 target = glm::inverse(projection) * glm::vec4(ndc.x, ndc.y, 1.0f, 1.0f);
    glm::vec3 direction =
        glm::vec3(glm::inverse(view) *
                  glm::vec4(glm::normalize(glm::vec3(target) / target.w), 0));

    return Ray{position, glm::normalize(direction), Random::Float()};
}

bool Camera::update(float ts) {
    auto window = Application::get().getWindow();

    static glm::vec2 last = {0.0f, 0.0f};
    static bool first = true;
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    glm::vec2 now = {x, y};
    if (first) {
        first = false;
        last = now;
        return false;
    }

    static bool space_down = false;
    int space_state = glfwGetKey(window, GLFW_KEY_SPACE);
    if (!space_down && space_state == GLFW_PRESS) {
        space_down = true;
    } else if (space_down && space_state == GLFW_RELEASE) {
        space_down = false;
        is_cursor_locked_ = !is_cursor_locked_;
        glfwSetInputMode(window, GLFW_CURSOR,
                         is_cursor_locked_ ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    if (!is_cursor_locked_) {
        last = now;
        return false;
    }

    bool moved = false;

    if (glfwGetKey(window, GLFW_KEY_W)) {
        move(ts, Direction::FORWARD);
        moved = true;
    } else if (glfwGetKey(window, GLFW_KEY_S)) {
        move(ts, Direction::BACKWARD);
        moved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A)) {
        move(ts, Direction::LEFT);
        moved = true;
    } else if (glfwGetKey(window, GLFW_KEY_D)) {
        move(ts, Direction::RIGHT);
        moved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_Q)) {
        move(ts, Direction::UP);
        moved = true;
    } else if (glfwGetKey(window, GLFW_KEY_E)) {
        move(ts, Direction::DOWN);
        moved = true;
    }

    glm::vec2 cursor_delta = now - last;
    last = now;
    if (cursor_delta.x != 0.0f || cursor_delta.y != 0.0f) {
        turn(cursor_delta);
        moved = true;
    }

    static float scroll_offset = 0.0f;
    glfwSetScrollCallback(window,
                          [](GLFWwindow* window, double xoffset, double yoffset) {
                              scroll_offset += static_cast<float>(yoffset);
                          });
    if (scroll_offset != 0.0f) {
        zoom(scroll_offset * 2.0f);
        scroll_offset = 0.0f;
        moved = true;
    }

    return moved;
}

void Camera::resize(uint32_t width, uint32_t height) {
    if (width_ == width && height_ == height) {
        return;
    }

    width_ = width;
    height_ = height;

    update();
}

void Camera::move(float dt, Direction dir) {
    glm::vec3 forward = direction;
    forward.y = 0;
    forward = glm::normalize(forward);
    glm::vec3 move_dir{};

    switch (dir) {
        case Direction::FORWARD:
            move_dir = forward;
            break;
        case Direction::BACKWARD:
            move_dir = -forward;
            break;
        case Direction::LEFT:
            move_dir = -glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        case Direction::RIGHT:
            move_dir = glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        case Direction::UP:
            move_dir = glm::vec3(0.0f, 1.0f, 0.0f);
            break;
        case Direction::DOWN:
            move_dir = glm::vec3(0.0f, -1.0f, 0.0f);
            break;
    }

    float move_speed = 3.0f;
    position += move_dir * move_speed * dt;

    update();
}

void Camera::turn(const glm::vec2& delta) {
    glm::vec2 turn_speed = {0.07f, 0.07f};
    phi_ += delta.x * turn_speed.x;
    if (phi_ > 360) phi_ -= 360;
    if (phi_ < 0) phi_ += 360;
    theta_ += delta.y * turn_speed.y;
    theta_ = glm::clamp(theta_, 1.0f, 179.0f);

    float sin_theta = glm::sin(glm::radians(theta_));
    float cos_theta = glm::cos(glm::radians(theta_));
    float sin_phi = glm::sin(glm::radians(phi_));
    float cos_phi = glm::cos(glm::radians(phi_));
    direction = {sin_theta * cos_phi, cos_theta, sin_theta * sin_phi};

    update();
}

void Camera::zoom(float delta) {
    fov_ = glm::clamp(fov_ - delta, 1.0f, 179.0f);

    update();
}

void Camera::update() {
    view = glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
    projection = glm::perspective(
        glm::radians(fov_), static_cast<float>(width_) / static_cast<float>(height_),
        near_, far_);
}