#pragma once

#include <chrono>
#include <iostream>
#include <string>

namespace console
{

// ANSI color codes
inline const char* GREEN = "\033[32m";
inline const char* RED = "\033[31m";
inline const char* YELLOW = "\033[33m";
inline const char* CYAN = "\033[36m";
inline const char* DIM = "\033[2m";
inline const char* BOLD = "\033[1m";
inline const char* RESET = "\033[0m";

// Unicode symbols
inline const char* CHECK = "\xe2\x9c\x93";  // ✓
inline const char* CROSS = "\xe2\x9c\x97";  // ✗
inline const char* ARROW = "\xe2\x86\x92";  // →
inline const char* BULLET = "\xe2\x80\xa2"; // •

inline void header(const std::string& title)
{
    std::cout << "\n  " << BOLD << title << RESET << "\n" << std::endl;
}

inline void success(const std::string& msg)
{
    std::cout << "  " << GREEN << CHECK << RESET << " " << msg << std::endl;
}

inline void error(const std::string& msg)
{
    std::cerr << "  " << RED << CROSS << RESET << " " << msg << std::endl;
}

inline void warning(const std::string& msg)
{
    std::cout << "  " << YELLOW << "!" << RESET << " " << msg << std::endl;
}

inline void step(const std::string& msg)
{
    std::cout << "    " << CYAN << ARROW << RESET << " " << msg << std::endl;
}

inline void detail(const std::string& msg)
{
    std::cout << "      " << DIM << BULLET << " " << msg << RESET << std::endl;
}

inline void blank() { std::cout << std::endl; }

inline void summary(const std::string& msg)
{
    std::cout << "  " << DIM << msg << RESET << std::endl;
}

// Timer
class Timer
{
public:
    Timer() : m_start(std::chrono::steady_clock::now()) {}

    double elapsed() const
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - m_start).count();
    }

    std::string elapsedStr() const
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.2fs", elapsed());
        return buf;
    }

private:
    std::chrono::steady_clock::time_point m_start;
};

// Stats accumulator
struct Stats
{
    int files = 0;
    int artboards = 0;
    int animations = 0;
    int stateMachines = 0;
    int assets = 0;
    int enums = 0;
    int viewModels = 0;
    int errors = 0;
};

inline std::string pluralize(int count, const std::string& singular)
{
    return std::to_string(count) + " " + singular + (count != 1 ? "s" : "");
}

inline std::string assetBreakdown(int images, int fonts, int audio, int unknown)
{
    std::string result;
    auto append = [&](int count, const std::string& label) {
        if (count > 0)
        {
            if (!result.empty())
                result += ", ";
            result += std::to_string(count) + " " + label;
        }
    };
    append(images, "images");
    append(fonts, "fonts");
    append(audio, "audio");
    append(unknown, "unknown");
    return result;
}

} // namespace console
