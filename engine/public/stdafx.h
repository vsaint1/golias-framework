#pragma once

#include <array>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#define GLM_FORCE_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <json/json.hpp>


#ifdef _WIN32

    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #include <windows.h>

#endif


#ifdef NDEBUG

    #define GOLIAS_ASSERT(x) ((void) 0)

#else

    #define GOLIAS_ASSERT(x)                           \
        do {                                           \
            if (!(x)) {                                \
                LOG_ERROR("Assertion failed: {}", #x); \
                assert(x);                             \
            }                                          \
        } while (0)


    #define GOLIAS_ASSERT_MSG(x, msg)                   \
        do {                                            \
            if (!(x)) {                                 \
                LOG_ERROR("Assertion failed: {}", msg); \
                assert(x);                              \
            }                                           \
        } while (0)


#endif


#define LOG_TRACE(...) ::spdlog::trace("{} - {}", __FUNCTION__, fmt::format(__VA_ARGS__))


#define LOG_INFO(...) ::spdlog::info("{} - {}", __FUNCTION__, fmt::format(__VA_ARGS__))


#define LOG_WARN(...) ::spdlog::warn("{} - {}", __FUNCTION__, fmt::format(__VA_ARGS__))


#define LOG_ERROR(...) ::spdlog::error("{} - {}", __FUNCTION__, fmt::format(__VA_ARGS__))


#define LOG_CRITICAL(...) ::spdlog::critical("{} - {}", __FUNCTION__, fmt::format(__VA_ARGS__))

#define LOG_FATAL(...)                                                     \
    ::spdlog::critical("{} - {}", __FUNCTION__, fmt::format(__VA_ARGS__)); \
    std::abort()


namespace golias {


    template <class T>
    using Ref = std::shared_ptr<T>;

    template <class T>
    using Scope = std::unique_ptr<T>;

    template <class T>
    using WeakRef = std::weak_ptr<T>;

    using String = std::string;

    using Json = nlohmann::json;

} // namespace golias
