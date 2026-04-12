function(sc_add_test test_name test_source link_lib)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE
        ${link_lib}
        GTest::gtest_main
    )
    gtest_discover_tests(${test_name})
endfunction()

add_subdirectory(core)
add_subdirectory(calc)
add_subdirectory(controller)
