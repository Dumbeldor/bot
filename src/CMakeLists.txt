include_directories(
        /usr/include/libircclient
        /usr/local/include/libircclient

        # for cmake_config.h
        ${PROJECT_BINARY_DIR}/include)

set(UNITTESTS 0)
option(ENABLE_UNITTESTS "Enable unit tests compilation" FALSE)

set(SOURCE_FILES
        main.cpp)
set(PROJECT_LIBS
        ircclient
        )

if (ENABLE_UNITTESTS)
    message("-- Unittests enabled")
    set(UNITTESTS 1)
    set(PROJECT_LIBS ${PROJECT_LIBS} cppunit)
    set(SOURCE_FILES ${SOURCE_FILES} unittests/tests.h)
else()
    message("-- Unittests disabled")
endif()

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBS})

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${BINDIR}
        BUNDLE DESTINATION .
        )