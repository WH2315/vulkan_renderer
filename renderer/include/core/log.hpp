#pragma once

#include <spdlog/spdlog.h>

namespace wen {

class Log final {
public:
    Log(const std::string& name);
    ~Log();

    template <typename... Args>
    void trace(fmt::format_string<Args...> format, Args&&... args) {
        logger_->trace(fmt::format(format, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void debug(fmt::format_string<Args...> format, Args&&... args) {
        logger_->debug(fmt::format(format, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void info(fmt::format_string<Args...> format, Args&&... args) {
        logger_->info(fmt::format(format, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void warn(fmt::format_string<Args...> format, Args&&... args) {
        logger_->warn(fmt::format(format, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void error(fmt::format_string<Args...> format, Args&&... args) {
        logger_->error(fmt::format(format, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void critical(fmt::format_string<Args...> format, Args&&... args) {
        logger_->critical(fmt::format(format, std::forward<Args>(args)...));
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
};

extern Log* g_log;

} // namespace wen

#define WEN_TRACE(...) ::wen::g_log->trace(__VA_ARGS__);
#define WEN_DEBUG(...) ::wen::g_log->debug(__VA_ARGS__);
#define WEN_INFO(...) ::wen::g_log->info(__VA_ARGS__);
#define WEN_WARN(...) ::wen::g_log->warn(__VA_ARGS__);
#define WEN_ERROR(...) ::wen::g_log->error(__VA_ARGS__);
#define WEN_CRITICAL(...) ::wen::g_log->critical(__VA_ARGS__);