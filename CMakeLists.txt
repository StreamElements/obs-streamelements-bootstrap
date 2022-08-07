project(obs-streamelements-bootstrap)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(APPLE)
    add_definitions(-DAPPLE)
    add_definitions(-D__APPLE__)
endif()

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/UI/obs-frontend-api")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/deps")

set(
    obs-streamelements-bootstrap_SOURCES
    obs-streamelements-bootstrap-plugin.cpp
)

set(
    obs-streamelements-bootstrap_HEADERS
    obs-streamelements-bootstrap-config.h
    obs-streamelements-bootstrap-plugin.hpp
    utils.hpp
)

set(
	obs-streamelements-bootstrap_LIBRARIES
	libobs
	obs-frontend-api
)

if(WIN32)
    list(APPEND obs-streamelements-bootstrap_LIBRARIES
        w32-pthreads
    )
endif()

if (APPLE)
    list(APPEND obs-streamelements-bootstrap_LIBRARIES
        "-framework System"
    )
endif(APPLE)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(
	obs-streamelements-bootstrap MODULE
	${obs-streamelements-bootstrap_SOURCES}
	${obs-streamelements-bootstrap_HEADERS}
)

if (APPLE)
    set_target_properties(obs-streamelements-bootstrap PROPERTIES
        COMPILE_FLAGS "-mmacosx-version-min=10.15 -target x86_64-apple-macos10.15")
endif(APPLE)

target_link_libraries(
	obs-streamelements-bootstrap
	${obs-streamelements-bootstrap_LIBRARIES}
)

#install_obs_plugin_with_data(obs-streamelements-bootstrap data)
install_obs_plugin(obs-streamelements-bootstrap data)
