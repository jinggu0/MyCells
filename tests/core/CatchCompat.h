#pragma once

#if defined(__has_include)
#    if __has_include(<catch2/catch_test_macros.hpp>)
#        include <catch2/catch_test_macros.hpp>
#        if __has_include(<catch2/catch_approx.hpp>)
#            include <catch2/catch_approx.hpp>
using Catch::Approx;
#        endif
#    elif __has_include(<catch2/catch.hpp>)
#        include <catch2/catch.hpp>
#    else
#        include "SimpleTest.h"
#    endif
#else
#    include "SimpleTest.h"
#endif
