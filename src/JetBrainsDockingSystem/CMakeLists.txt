set(CMAKE_AUTOMOC on)
set(CMAKE_AUTORCC on)
set(CMAKE_AUTOUIC on)

if(JBDS_BUILD_STATIC)
    add_library(${PROJECT_NAME} STATIC)
    target_compile_definitions(${PROJECT_NAME} PUBLIC JBDS_STATIC)
else()
    add_library(${PROJECT_NAME} SHARED)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE JBDS_LIBRARY)

# add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
file(GLOB_RECURSE _src *.h *.cpp)
target_sources(${PROJECT_NAME} PRIVATE ${_src})

target_include_directories(${PROJECT_NAME} PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
)

target_include_directories(${PROJECT_NAME} PRIVATE . private)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${Qt${QT_VERSION_MAJOR}Core_PRIVATE_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Widgets_PRIVATE_INCLUDE_DIRS}
)

# Add platform specific
if(WIN32)
    set(RC_DESCRIPTION "JetBrains Docking System Library")
    set(RC_COPYRIGHT "Copyright (C) 2020-2024 Stdware Collections")
    include("../../cmake/winrc.cmake")
endif()

if(JBDS_INSTALL)
    target_include_directories(${PROJECT_NAME} PUBLIC
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )

    install(TARGETS ${PROJECT_NAME}
        EXPORT ${JBDS_INSTALL_NAME}Targets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" OPTIONAL
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" OPTIONAL
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" OPTIONAL
    )

    # Install headers
    install(DIRECTORY ./
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${JBDS_INSTALL_NAME}
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*_p.h" EXCLUDE
    )
endif()

if(JBDS_INSTALL)
    # Add install target
    set(_install_dir ${CMAKE_INSTALL_LIBDIR}/cmake/${JBDS_INSTALL_NAME})

    # Add version file
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${JBDS_INSTALL_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    # Add configuration file
    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/${JBDS_INSTALL_NAME}Config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/${JBDS_INSTALL_NAME}Config.cmake"
        INSTALL_DESTINATION ${_install_dir}
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

    # Install cmake files
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${JBDS_INSTALL_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${JBDS_INSTALL_NAME}ConfigVersion.cmake"
        DESTINATION ${_install_dir}
    )

    # Install cmake targets files
    install(EXPORT ${JBDS_INSTALL_NAME}Targets
        FILE "${JBDS_INSTALL_NAME}Targets.cmake"

        # NAMESPACE ${JBDS_INSTALL_NAME}::
        DESTINATION ${_install_dir}
    )
endif()