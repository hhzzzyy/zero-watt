#
# SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
#
# SPDX-License-Identifier: MIT
# Packaging and install layout for CardputerZero Debian builds.

include(GNUInstallDirs)

set(APP_DISPLAY_NAME "Zero Watt" CACHE STRING "Human-readable application name used by launchers")
set(APP_PACKAGE_BASENAME "ZeroWatt" CACHE STRING "Filename stem used for the Debian artifact")
set(APP_DEBIAN_REVISION "rev1" CACHE STRING "Debian package revision/vendor suffix")
set(APP_DEBIAN_ARCHITECTURE "arm64" CACHE STRING "Debian package architecture")
set(APP_MAINTAINER "hzy <573493999@qq.com>" CACHE STRING "Debian package maintainer")
set(APP_PACKAGE_DESCRIPTION "Battery power monitor for Cardputer Zero" CACHE STRING "Debian package summary")
set(APP_SOURCE_REPO "https://github.com/hhzzzyy/zero-watt" CACHE STRING "Public source repository")
set(APP_INSTALL_COLLECTOR_SERVICE ON CACHE BOOL "Install the user-level collector service")

set(APP_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/package")
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/templates/app.desktop.in"
    "${APP_GENERATED_DIR}/${PROJECT_NAME}.desktop"
    @ONLY
)

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/templates/postinst"
    "${APP_GENERATED_DIR}/postinst"
    COPYONLY
)
file(CHMOD "${APP_GENERATED_DIR}/postinst"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES "${APP_GENERATED_DIR}/${PROJECT_NAME}.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/APPLaunch/applications"
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/applaunch/share/images/zero-watt.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/APPLaunch/share/images"
)

if(APP_INSTALL_COLLECTOR_SERVICE)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/applaunch/zero-watt-collector.service"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/systemd/user"
    )
endif()

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/dist")
set(CPACK_PACKAGE_NAME "zero-watt")
set(CPACK_PACKAGE_VENDOR "${APP_MAINTAINER}")
set(CPACK_PACKAGE_CONTACT "${APP_MAINTAINER}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${APP_PACKAGE_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${APP_PACKAGE_BASENAME}_${PROJECT_VERSION}_${APP_DEBIAN_REVISION}_${APP_DEBIAN_ARCHITECTURE}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(CPACK_DEBIAN_PACKAGE_NAME "zero-watt")
set(CPACK_DEBIAN_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_DEBIAN_PACKAGE_RELEASE "${APP_DEBIAN_REVISION}")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${APP_DEBIAN_ARCHITECTURE}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${APP_MAINTAINER}")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${APP_SOURCE_REPO}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6, libgcc-s1")
# The project was renamed before its first store release. Let dpkg replace an
# existing development install instead of leaving both application entries.
set(CPACK_DEBIAN_PACKAGE_REPLACES "power-monitor")
set(CPACK_DEBIAN_PACKAGE_CONFLICTS "power-monitor")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${APP_GENERATED_DIR}/postinst")

include(CPack)
