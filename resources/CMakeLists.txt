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

    set(driver_target "${firmware_target}_drivers_interface")

    if(NOT TARGET ${driver_target})
        add_library(${driver_target} INTERFACE)

        target_sources(${driver_target} INTERFACE
                ${DRIVER_SOURCES}
        )

        target_include_directories(${driver_target} INTERFACE
                "${DRV_ROOT}/Inc"
        )
    endif()

    # Driver sources compile inside the board executable using its settings.
    target_link_libraries(${firmware_target} PRIVATE ${driver_target})
endfunction()