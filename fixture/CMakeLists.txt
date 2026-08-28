cmake_minimum_required(VERSION 3.20)
project(diff2test_fixture LANGUAGES CXX)

enable_testing()

add_library(core STATIC src/core.cpp)
target_include_directories(core PUBLIC include)
target_compile_features(core PUBLIC cxx_std_20)

add_library(alpha STATIC src/alpha.cpp)
target_include_directories(alpha PUBLIC include)
target_link_libraries(alpha PUBLIC core)

add_library(beta STATIC src/beta.cpp)
target_include_directories(beta PUBLIC include)
target_link_libraries(beta PUBLIC core)

add_executable(alpha_test tests/alpha_test.cpp)
target_link_libraries(alpha_test PRIVATE alpha)
add_test(NAME AlphaTest COMMAND alpha_test)

add_executable(beta_test tests/beta_test.cpp)
target_link_libraries(beta_test PRIVATE beta)
add_test(NAME BetaTest COMMAND beta_test)

add_executable(core_test tests/core_test.cpp)
target_link_libraries(core_test PRIVATE core)
add_test(NAME CoreTest COMMAND core_test)
