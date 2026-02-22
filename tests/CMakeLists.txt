# Tests CMakeLists.txt

# Find Google Test (or use Qt Test)
find_package(GTest QUIET)

if(GTest_FOUND)
    enable_testing()
    include(GoogleTest)
    
    # Scanner tests
    add_executable(test_scanner
        test_scanner.cpp
        ../src/core/Scanner.cpp
    )
    target_include_directories(test_scanner PRIVATE ../src)
    target_link_libraries(test_scanner GTest::gtest_main)
    gtest_discover_tests(test_scanner)
    
    # Clustering tests
    add_executable(test_clustering
        test_clustering.cpp
        ../src/core/Clusterer.cpp
        ../src/services/Database.cpp
        ../src/services/FaceService.cpp
    )
    target_include_directories(test_clustering PRIVATE ../src)
    target_link_libraries(test_clustering 
        GTest::gtest_main
        SQLite::SQLite3
    )
    gtest_discover_tests(test_clustering)
    
else()
    message(STATUS "Google Test not found, tests will not be built")
    message(STATUS "Install with: brew install googletest")
endif()
