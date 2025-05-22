find_package(Catch2 CONFIG REQUIRED)

add_executable(orderbook_tests test_orderbook.cpp)

# Link against Catch2 (which brings in its include dirs) and your library
target_link_libraries(orderbook_tests
  PRIVATE
    Catch2::Catch2WithMain
    simulator_lib        # your OrderBook.cpp compiled as a library
)

include(CTest)
include(Catch)
catch_discover_tests(orderbook_tests)
