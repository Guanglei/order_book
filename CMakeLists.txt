cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

project(OrderBook)

find_library(BOOST_LIBRARY boost_system HINTS /usr/local/lib)

add_executable(FeedHandler feed_handler.cpp)
target_include_directories(FeedHandler PUBLIC /usr/local/include)
target_link_libraries(FeedHandler ${BOOST_LIBRARY})

find_library(BENCHMARK_LIBRARY benchmark HINTS /usr/local/lib)

add_executable(OrderBookBenchmark order_book_benchmark.cpp)
target_include_directories(OrderBookBenchmark PUBLIC /usr/local/include)
target_link_libraries(OrderBookBenchmark ${BOOST_LIBRARY} ${BENCHMARK_LIBRARY})
