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

    # Use a unique INTERFACE target name per board so it doesn't collide globally
    set(driver_target "${firmware_target}_drivers_interface")

    if(NOT TARGET ${driver_target})
        add_library(${driver_target} INTERFACE)

        # Attach sources and public include directories to the interface
        target_sources(${driver_target} INTERFACE ${DRIVER_SOURCES})
        target_include_directories(${driver_target} INTERFACE
                "${DRV_ROOT}/Inc"
        )
    endif()

    # Link the interface to the board target.
    # Because it's an INTERFACE library, its sources compile straight into the board executable!
    target_link_libraries(${firmware_target} PRIVATE ${driver_target})

    # Inherit the board's include directories, compile definitions, and options
    foreach(property IN ITEMS
            INCLUDE_DIRECTORIES
            COMPILE_DEFINITIONS
            COMPILE_OPTIONS
    )
        get_target_property(board_value ${firmware_target} ${property})

        if(board_value)
            set_property(TARGET ${driver_target}
                    APPEND PROPERTY INTERFACE_${property} "${board_value}"
            )
        endif()
    endforeach()
endfunction()