#if defined(__has_include)
#    if __has_include(<catch2/catch_session.hpp>)
#        include <catch2/catch_session.hpp>

int main(int argc, char* argv[])
{
    return Catch::Session().run(argc, argv);
}

#    elif __has_include(<catch2/catch.hpp>)
#        define CATCH_CONFIG_MAIN
#        include <catch2/catch.hpp>
#    else
#        include "SimpleTest.h"

int main()
{
    return ::acell::test::runAllTests();
}

#    endif
#else
#    include "SimpleTest.h"

int main()
{
    return ::acell::test::runAllTests();
}

#endif
