#include "renderer.hpp"
#include "resources/material.hpp"
#include <cmath>

static float linear_to_gamma(float linear_component) {
    if (linear_component > 0) {
        return std::sqrt(linear_component);
    }
    return 0;
}

static uint32_t convert(const glm::vec4& color) {
    auto r = color.r;
    auto g = color.g;
    auto b = color.b;
    auto a = color.a;

    if (!std::isfinite(r)) r = 0.0f;
    if (!std::isfinite(g)) g = 0.0f;
    if (!std::isfinite(b)) b = 0.0f;
    if (!std::isfinite(a)) a = 1.0f;

    r = linear_to_gamma(r);
    g = linear_to_gamma(g);
    b = linear_to_gamma(b);

    static const Interval intensity(0.000f, 0.999f);
    const uint8_t rbyte = static_cast<uint8_t>(intensity.clamp(r) * 255.0f);
    const uint8_t gbyte = static_cast<uint8_t>(intensity.clamp(g) * 255.0f);
    const uint8_t bbyte = static_cast<uint8_t>(intensity.clamp(b) * 255.0f);
    const uint8_t abyte = static_cast<uint8_t>(glm::clamp(a, 0.0f, 1.0f) * 255.0f);

    return (abyte << 24) | (bbyte << 16) | (gbyte << 8) | rbyte;
}

void Renderer::resize(uint32_t width, uint32_t height) {
    if (image_ && image_->width() == width && image_->height() == height) {
        return;
    }

    if (image_) {
        image_->resize(width, height);
    } else {
        image_ = std::make_shared<Image>(width, height, ImageFormat::RGBA);
    }

    delete[] data_;
    data_ = new uint32_t[width * height];

    delete[] accumulation_;
    accumulation_ = new glm::vec4[width * height];
}

void Renderer::render(const Camera& camera, const Scene& scene) {
    scene_ = &scene;

    auto w = image_->width(), h = image_->height();

    if (index_ == 1) {
        memset(accumulation_, 0, w * h * sizeof(glm::vec4));
    }

    thread_pool_.parallelFor(w, h, [&](size_t x, size_t y) {
        const uint32_t idx = static_cast<uint32_t>(y * w + x);
        for (uint32_t i = 0; i < samples_per_pixel_; i++) {
            // (-0.5, 0.5)
            glm::vec2 offset = {Random::Float() - 0.5f, Random::Float() - 0.5f};
            Ray ray = camera.generateRay({x, y}, offset);
            glm::vec4 color = glm::vec4(traceRay(ray, 50), 1.0f);
            accumulation_[idx] += color;
        }

        // Average across all accumulated samples for this pixel.
        const float sample_count = static_cast<float>(index_ * samples_per_pixel_);
        glm::vec4 color = accumulation_[idx] / sample_count;
        data_[idx] = convert(color);
    });
    thread_pool_.wait();

    image_->set(data_);

    if (accumulated_) {
        index_++;
    } else {
        index_ = 1;
    }
}

glm::vec3 Renderer::traceRay(const Ray& ray, int depth) {
    if (depth <= 0) {
        return glm::vec3(0.0f);
    }

    auto& world = scene_->world;
    HitRecord hit_record;
    if (!world->hit(ray, Interval(0.001f, infinity), hit_record)) {
        return background;
    }

    glm::vec3 emitted = hit_record.material->emitted(hit_record);

    ScatterRecord scatter_record;
    if (!hit_record.material->scatter(ray, hit_record, scatter_record)) {
        return emitted;
    }

    glm::vec3 scattered;

    if (!scene_->lights) {
        scattered =
            scatter_record.attenuation * traceRay(scatter_record.ray_out, depth - 1);
        return emitted + scattered;
    }

    if (!scatter_record.pdf) {
        scattered =
            scatter_record.attenuation * traceRay(scatter_record.ray_out, depth - 1);
        return scattered;
    }

    auto light = std::make_shared<HittablePDF>(scene_->lights, hit_record.point);
    MixturePDF mixture(light, scatter_record.pdf);
    Ray ray_out(hit_record.point, glm::normalize(mixture.generate()), ray.time);
    float pdf_value = mixture.value(ray_out.direction);
    float pdf = hit_record.material->scatteringPDF(hit_record, ray_out);
    if (abs(pdf_value) < glm::epsilon<float>()) {
        return emitted;
    }
    scattered =
        (scatter_record.attenuation * pdf * traceRay(ray_out, depth - 1)) / pdf_value;

    return emitted + scattered;
}