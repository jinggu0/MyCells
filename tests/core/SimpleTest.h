#pragma once

#include <algorithm>
#include <exception>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace acell
{
namespace test
{

using TestFunction = void (*)();

struct TestCase
{
    const char* name = "";
    const char* file = "";
    int line = 0;
    TestFunction function = nullptr;
};

inline std::vector<TestCase>& registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

class Registrar
{
public:
    Registrar(const char* name, const char* file, int line, TestFunction function)
    {
        registry().push_back({name, file, line, function});
    }
};

class AssertionFailure : public std::runtime_error
{
public:
    AssertionFailure(const char* expression, const char* file, int line)
        : std::runtime_error(std::string(file) + ":" + std::to_string(line) +
                             ": requirement failed: " + expression)
    {
    }
};

inline void require(bool condition, const char* expression, const char* file, int line)
{
    if(!condition)
    {
        throw AssertionFailure(expression, file, line);
    }
}

inline int runAllTests()
{
    int failed = 0;

    for(const TestCase& test : registry())
    {
        try
        {
            test.function();
            std::cout << "[pass] " << test.name << '\n';
        }
        catch(const std::exception& ex)
        {
            failed++;
            std::cerr << "[fail] " << test.name << '\n'
                      << "       " << ex.what() << '\n';
        }
        catch(...)
        {
            failed++;
            std::cerr << "[fail] " << test.name << '\n'
                      << "       unknown exception\n";
        }
    }

    if(failed == 0)
    {
        std::cout << registry().size() << " test(s) passed\n";
        return 0;
    }

    std::cerr << failed << " test(s) failed out of " << registry().size() << '\n';
    return 1;
}

} // namespace test
} // namespace acell

class Approx
{
public:
    explicit Approx(double value)
        : value_(value)
    {
    }

    friend bool operator==(double lhs, const Approx& rhs)
    {
        return rhs.compare(lhs);
    }

    friend bool operator==(const Approx& lhs, double rhs)
    {
        return lhs.compare(rhs);
    }

    friend bool operator!=(double lhs, const Approx& rhs)
    {
        return !(lhs == rhs);
    }

    friend bool operator!=(const Approx& lhs, double rhs)
    {
        return !(lhs == rhs);
    }

private:
    bool compare(double other) const
    {
        const double scale = std::max(1.0, std::max(std::abs(value_), std::abs(other)));
        return std::abs(other - value_) <= epsilon_ * scale;
    }

    double value_;
    double epsilon_ = 1e-12;
};

#define ACELL_TEST_CONCAT_INNER(a, b) a##b
#define ACELL_TEST_CONCAT(a, b) ACELL_TEST_CONCAT_INNER(a, b)

#define TEST_CASE(name) \
    static void ACELL_TEST_CONCAT(acell_test_, __LINE__)(); \
    namespace \
    { \
    const ::acell::test::Registrar ACELL_TEST_CONCAT(acell_test_registrar_, __LINE__)( \
        name, __FILE__, __LINE__, &ACELL_TEST_CONCAT(acell_test_, __LINE__)); \
    } \
    static void ACELL_TEST_CONCAT(acell_test_, __LINE__)()

#define REQUIRE(expression) \
    do \
    { \
        ::acell::test::require(static_cast<bool>(expression), #expression, __FILE__, __LINE__); \
    } while(false)

#define REQUIRE_FALSE(expression) \
    do \
    { \
        ::acell::test::require(!static_cast<bool>(expression), "!(" #expression ")", __FILE__, __LINE__); \
    } while(false)
