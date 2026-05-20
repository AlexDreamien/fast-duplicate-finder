add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/doctest)

add_executable(dupfinder_tests
    test_main.cpp
    test_hashing.cpp
    test_scan.cpp
    test_finders.cpp
)
target_link_libraries(dupfinder_tests PRIVATE dupfinder_core doctest)

add_test(NAME dupfinder_tests COMMAND dupfinder_tests)
