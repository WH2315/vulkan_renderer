#include "core/log.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace wen {

Log* g_log = nullptr;

Log::Log(const std::string& name) {
    logger_ = spdlog::stdout_color_mt(name);
    logger_->set_level(spdlog::level::trace);
}

Log::~Log() {
    spdlog::drop(logger_->name());
    logger_.reset();
}

} // namespace wen