
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24") 
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Disable installing GTEST")
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

include_directories(${CMAKE_SOURCE_DIR}/include)
  
function (add_unit_test UNIT_TEST)
    set(UNIT_TESTS ${UNIT_TESTS} ${UNIT_TEST} PARENT_SCOPE)
    add_executable(${UNIT_TEST} ${UNIT_TEST}.cpp)
    target_compile_options(${UNIT_TEST} PRIVATE ${PROJECT_COMPILE_OPTIONS})
    target_compile_features(${UNIT_TEST} PRIVATE cxx_std_20)
    target_link_libraries(${UNIT_TEST} PRIVATE gtest_main ${ARGN})
    gtest_discover_tests(${UNIT_TEST} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

# arrcmp can't work on non intelesque architectures
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR
    ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64" OR
    ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i386" OR
    ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i686")
add_unit_test(test_arrcmp)
endif()

add_unit_test(test_search hibp flat_file toc)
add_unit_test(test_diffutils hibp flat_file diffutils)

add_custom_target(all_tests ALL DEPENDS ${all_targets} ${UNIT_TESTS})

add_custom_command(
  TARGET all_tests
  POST_BUILD 
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> --output-on-failure # --verbose
  USES_TERMINAL       
)

add_custom_command(
  TARGET all_tests
  POST_BUILD 
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND bash ./system_tests.sh ${CMAKE_CURRENT_BINARY_DIR}/..
  USES_TERMINAL       
)
