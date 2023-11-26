add_library(RetGui
    src/retgui/retgui.cpp
)
target_include_directories(RetGui
    PRIVATE
    src
    PUBLIC
    include
)
set_target_properties(RetGui PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)