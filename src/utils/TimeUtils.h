#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace acell
{
namespace utils
{

inline std::tm safeGmTime(std::time_t t)
{
    std::tm tmv{};
#if defined(_WIN32)
    gmtime_s(&tmv, &t);
#else
    gmtime_r(&t, &tmv);
#endif
    return tmv;
}

inline std::tm safeLocalTime(std::time_t t)
{
    std::tm tmv{};
#if defined(_WIN32)
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    return tmv;
}

inline std::string nowIsoUtc()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);
    const std::tm tmv = safeGmTime(tt);

    std::ostringstream oss;
    oss << std::put_time(&tmv, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

inline std::time_t timegm_portable(std::tm* tmv)
{
#if defined(_WIN32)
    return _mkgmtime(tmv);
#else
    return timegm(tmv);
#endif
}

inline std::chrono::system_clock::time_point parseIsoUtc(const std::string& s)
{
    std::tm tmv{};
    std::istringstream iss(s);

    if(s.size() >= 20 && s.back() == 'Z')
    {
        iss >> std::get_time(&tmv, "%Y-%m-%dT%H:%M:%SZ");
        if(iss.fail())
        {
            throw std::runtime_error("Failed to parse UTC time: " + s);
        }
        const std::time_t tt = timegm_portable(&tmv);
        return std::chrono::system_clock::from_time_t(tt);
    }

    if(s.size() >= 19)
    {
        iss.clear();
        iss.str(s);
        iss >> std::get_time(&tmv, "%Y-%m-%d %H:%M:%S");
        if(!iss.fail())
        {
            const std::time_t tt = timegm_portable(&tmv);
            return std::chrono::system_clock::from_time_t(tt);
        }
    }

    throw std::runtime_error("Unsupported time format: " + s);
}

inline double elapsedSecondsUtc(const std::string& from_iso_utc,
                                const std::string& to_iso_utc)
{
    const auto a = parseIsoUtc(from_iso_utc);
    const auto b = parseIsoUtc(to_iso_utc);
    return std::chrono::duration<double>(b - a).count();
}

} // namespace utils
} // namespace acell
