#pragma once

#if defined(__has_include)
#    if __has_include(<optional>)
#        include <optional>
#    elif __has_include(<experimental/optional>)
#        include <experimental/optional>

namespace std
{
using experimental::nullopt;
using experimental::nullopt_t;
using experimental::optional;
}

#    else
#        error "This project requires std::optional or std::experimental::optional support."
#    endif
#else
#    include <optional>
#endif
