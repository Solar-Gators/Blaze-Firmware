cmake_minimum_required(VERSION 3.20)

function(add_solargators_drivers firmware_target)
    set(DRV_ROOT
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/SolarGators_drivers"
    )

    file(GLOB_RECURSE DRIVER_SOURCES CONFIGURE_DEPENDS
            "${DRV_ROOT}/Src/*.c"
            "${DRV_ROOT}/Src/*.cpp"
    )

    if(NOT DRIVER_SOURCES)
        message(FATAL_ERROR "No driver sources found in ${DRV_ROOT}/Src")
    endif()

    set(driver_target "${firmware_target}_drivers")

    add_library(${driver_target} STATIC ${DRIVER_SOURCES})

    # Compile this library with this board's settings only.
    foreach(property IN ITEMS
            INCLUDE_DIRECTORIES
            COMPILE_DEFINITIONS
            COMPILE_OPTIONS
    )
        get_target_property(board_value ${firmware_target} ${property})

        if(board_value)
            set_property(TARGET ${driver_target}
                    APPEND PROPERTY ${property} "${board_value}"
            )
        endif()
    endforeach()

    # Make shared headers available to the library and its board.
    target_include_directories(${driver_target} PUBLIC
            "${DRV_ROOT}/Inc"
    )

    target_link_libraries(${firmware_target} PRIVATE ${driver_target})
endfunction()