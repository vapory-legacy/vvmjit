# Compile tests for headers:
add_library(test-vvm-header-cpp STATIC test-vvm.cpp)
target_include_directories(test-vvm-header-cpp PRIVATE ../include)


# Library tests:
add_executable(test-vvmjit-standalone test-vvmjit-standalone.c)
target_include_directories(test-vvmjit-standalone PRIVATE ../include)
if (MSVC)
    set(SYSTEM_LIBS "")
elseif (APPLE)
    set(SYSTEM_LIBS stdc++ curses z pthread m)
else()
    set(SYSTEM_LIBS stdc++ ncurses z pthread dl m)
endif()
target_link_libraries(test-vvmjit-standalone PRIVATE ${VVMJIT_STANDALONE_LIB} ${SYSTEM_LIBS})
add_dependencies(test-vvmjit-standalone vvmjit-standalone)


add_test(vvmjit-standalone test-vvmjit-standalone)
