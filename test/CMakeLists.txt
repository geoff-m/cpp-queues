add_executable(test-queues
        src/TestMultipleProducerMultipleConsumerBlockingQueue.cpp
        include/Stopwatch.h
        src/Stopwatch.cpp
        include/TestUtilities.h
)

target_include_directories(test-queues
        PRIVATE
        include
        ../lib/include)
target_link_libraries(test-queues
        queues
        GTest::gtest_main)
target_compile_options(test-queues PRIVATE $<$<CONFIG:Debug>:-g -O3>)

include(GoogleTest)
gtest_discover_tests(test-queues DISCOVERY_MODE PRE_TEST)