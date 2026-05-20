# Unit tests
add_executable(orderbook_test test/test.cpp)
target_link_libraries(orderbook_test orderbook)
target_include_directories(orderbook_test PRIVATE include)

# Enable testing
enable_testing()
add_test(NAME OrderBookBasic COMMAND orderbook_test)

# For memory checking
add_custom_target(test-valgrind
    COMMAND valgrind --leak-check=full --error-exitcode=1 ./orderbook_test
    DEPENDS orderbook_test
)

# For coverage analysis
if(CMAKE_COMPILER_IS_GNUCXX)
    add_custom_target(test-coverage
        COMMAND gcovr --root . --exclude test --html -o coverage.html
        COMMAND echo "Coverage report: coverage.html"
    )
endif()