#pragma once
#include <iostream>
#include <sstream>

enum BOX_LOG_LEVEL {
    BOX_LOG_LEVEL_INIT = 0,
    BOX_LOG_LEVEL_ERROR = 1,
    BOX_LOG_LEVEL_WARNING = 2,
    BOX_LOG_LEVEL_INFO = 3,
    BOX_LOG_LEVEL_TRACE = 4,
    BOX_LOG_LEVEL_DEBUG = 5
};

// ANSI color codes
#define BOX_COLOR_RESET   "\x1b[0m"
#define BOX_COLOR_RED     "\x1b[31m"
#define BOX_COLOR_YELLOW  "\x1b[33m"
#define BOX_COLOR_GREEN   "\x1b[32m"
#define BOX_COLOR_CYAN    "\x1b[36m"
#define BOX_COLOR_BLUE   "\x1b[34m"

#ifdef _DEBUG
#ifndef BOX_LOG_LEVEL_ACTIVE
#define BOX_LOG_LEVEL_ACTIVE BOX_LOG_LEVEL_DEBUG
#endif

//Select color based on level
#define BOX_LOG_COLOR(level) \
        ((level) == BOX_LOG_LEVEL_INIT ?    BOX_COLOR_GREEN : \
         (level) == BOX_LOG_LEVEL_ERROR ?   BOX_COLOR_RED : \
         (level) == BOX_LOG_LEVEL_WARNING ? BOX_COLOR_YELLOW : \
         (level) == BOX_LOG_LEVEL_INFO ?    BOX_COLOR_GREEN : \
         (level) == BOX_LOG_LEVEL_TRACE ?   BOX_COLOR_CYAN : \
         BOX_COLOR_BLUE)

// Internal macro for logging with level and color
#define BOX_LOG(level, level_name, ...) do { \
        if ((level) <= BOX_LOG_LEVEL_ACTIVE) { \
            std::ostringstream box_log_stream; \
            box_log_stream << __VA_ARGS__; \
            std::cout << BOX_LOG_COLOR(level) \
                      << "[3d BOX_LOG][" << level_name << "] " << box_log_stream.str() \
                      << BOX_COLOR_RESET << std::endl; \
        } \
    } while (0)


//#define GLWIN_LOG_INIT(x) std::cout << x << std::endl
#define BOX_LOG_INIT(...)    BOX_LOG(BOX_LOG_LEVEL_INIT,      "INIT", __VA_ARGS__)
#define BOX_LOG_ERROR(...)   BOX_LOG(BOX_LOG_LEVEL_ERROR,     "ERROR", __VA_ARGS__)
#define BOX_LOG_WARNING(...) BOX_LOG(BOX_LOG_LEVEL_WARNING,   "WARN", __VA_ARGS__)
#define BOX_LOG_INFO(...)    BOX_LOG(BOX_LOG_LEVEL_INFO,      "INFO", __VA_ARGS__)
#define BOX_LOG_TRACE(...)   BOX_LOG(BOX_LOG_LEVEL_TRACE,     "TRACE", __VA_ARGS__)
#define BOX_LOG_DEBUG(...)   BOX_LOG(BOX_LOG_LEVEL_DEBUG,     "DEBUG", __VA_ARGS__)

#else
// In release: macros do nothing
#define BOX_LOG_INIT(x)
#define BOX_LOG_ERROR(...)
#define BOX_LOG_WARNING(...)
#define BOX_LOG_INFO(...)
#define BOX_LOG_TRACE(...)
#define BOX_LOG_DEBUG(...)
#endif

