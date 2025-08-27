cmake_minimum_required(VERSION 3.22)

project(OrionSort VERSION 0.5.0)

# Project-wide settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/dist")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -Wall -Wextra -Wno-deprecated-literal-operator -Wno-unused-parameter -Wno-sign-compare")
set(REPO "https://www.github.com/TheUltimateOrion/Sorting")

option(BUILD_TESTING "Build Test Executable?" OFF)

# if(BUILD_TESTING)
#     include(CTest)
#     enable_testing()
#     add_subdirectory(test)
# endif(BUILD_TESTING)

# External dependencies
add_subdirectory(external)

# Collect source files
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cpp)

# Define executable
if(ANDROID)
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
else() 
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCE_FILES})
endif()

target_compile_definitions(
    ${PROJECT_NAME} PRIVATE 
    APP_VERSION="${PROJECT_VERSION}"
    GITHUB_REPO="${REPO}"
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    includes/
)

if (ANDROID)
    target_link_libraries(${PROJECT_NAME} PRIVATE ImGui SDL3::SDL3 SDL3_ttf::SDL3_ttf OpenAL::OpenAL ImGui::ImGui android log)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE ImGui SDL3::SDL3 SDL3_ttf::SDL3_ttf OpenAL::OpenAL ImGui::ImGui)
endif()
# Install rules per platform
if(WIN32)
    install(TARGETS 
                ${PROJECT_NAME} 
                SDL3-shared 
                SDL3_ttf-shared
                OpenAL
            RUNTIME_DEPENDENCY_SET 
                runtime_set
            RUNTIME 
                DESTINATION .
            )

    message(STATUS "MSYSTEM PREFIX: $ENV{MSYSTEM_PREFIX}")
    install(RUNTIME_DEPENDENCY_SET 
                runtime_set 
            DIRECTORIES
                    "C:/msys64/ucrt64/bin"  # Adjust path as needed
                    "C:/msys64/mingw64/bin"  # Adjust path as needed
                    "$ENV{MSYSTEM_PREFIX}/bin"  # Adjust path as needed
            PRE_INCLUDE_REGEXES "lib.*\\.dll"
            PRE_EXCLUDE_REGEXES
                ".*" 
            DESTINATION .
            )

    install(DIRECTORY 
                res/ 
            DESTINATION 
                res
            )   
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "app"
        MACOSX_BUNDLE_GUI_IDENTIFIER com.orion.${PROJECT_NAME}
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/macos/Info.plist
        INSTALL_RPATH "@executable_path/../Frameworks"
    )

    install(TARGETS 
                ${PROJECT_NAME} 
                SDL3-shared 
                SDL3_ttf-shared
                OpenAL
            BUNDLE 
                DESTINATION .
            LIBRARY 
                DESTINATION "${PROJECT_NAME}.app/Contents/Frameworks"
            RUNTIME 
                DESTINATION "${PROJECT_NAME}.app/Contents/MacOS"
            )

    install(DIRECTORY 
                res/ 
            DESTINATION 
                "${PROJECT_NAME}.app/Contents/Resources/res"
            )
elseif(LINUX)
    install(TARGETS 
                ${PROJECT_NAME} 
                SDL3-shared 
                SDL3_ttf-shared
                OpenAL
            LIBRARY 
                DESTINATION lib/
            RUNTIME 
                DESTINATION .
            )
            
    install(DIRECTORY 
                res/ 
            DESTINATION 
                res
            )

    # Set proper RPATH
    set(RPATH "$ORIGIN/lib")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUILD_RPATH "${RPATH}"
        INSTALL_RPATH "${RPATH}"
    )    
endif()

# Packaging metadata
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "orion")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sorting visualizer application written in C++ using SDL3, OpenAL-Soft, and Dear ImGui")
set(CPACK_PACKAGE_HOMEPAGE_URL ${REPO})
set(CPACK_PACKAGE_CONTACT "TheUltimateOrion")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
# Optional if present
if(EXISTS "${CMAKE_SOURCE_DIR}/README.md")
  set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
endif()

# Versioning
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

# Common defaults
set(CPACK_STRIP_FILES ON)                         # smaller packages
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)         # install to root of archive/DMG

set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_SOURCE_DIR}/dist")

# Platform‑specific generators and polish
if(WIN32)
  # Generators
  set(CPACK_GENERATOR "NSIS;ZIP")

  # Install root (displayed in NSIS)
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")

  set(CPACK_NSIS_MENU_LINKS
    ${REPO} "Project Homepage"
  )

  set(CPACK_NSIS_EXECUTABLES_DIRECTORY .)

  # NSIS tweaks
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
#   set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/windows/icon.ico")          # optional
#   set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/windows/icon.ico")       # optional
  set(CPACK_NSIS_INSTALLED_ICON_NAME "${PROJECT_NAME}.exe")
  set(CPACK_NSIS_URL_INFO_ABOUT ${REPO})

  # File name (architecture suffix helps when distributing)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_ARCH_SUFFIX x64)
  else()
    set(_ARCH_SUFFIX x86)
  endif()
  set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-win-${_ARCH_SUFFIX}")

elseif(APPLE)
  # macOS: create a DMG that contains the .app bundle produced by install()
  set(CPACK_GENERATOR "DragNDrop")
  set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME} ${PROJECT_VERSION}")
  set(CPACK_DMG_FORMAT "UDZO")  
  set(CPACK_DMG_DISABLE_APPLICATIONS_SYMLINK OFF)
  # Optional background if you add an image file
  if(EXISTS "${CMAKE_SOURCE_DIR}/macos/dmg_background.png")
    set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/macos/dmg_background.png")
  endif()
  set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")

elseif(UNIX)
  # Linux: always produce a tarball; add DEB/RPM if tooling is available
  set(CPACK_GENERATOR "TGZ")
  set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-linux")

  # DEB
  if(EXISTS "/usr/bin/dpkg-deb")
    list(APPEND CPACK_GENERATOR DEB)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "TheUltimateOrion")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    # Runtime deps are conservative; adjust to match your SDL/OpenAL packaging
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6, libgcc-s1, libasound2, libx11-6, libx11-xcb1")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
  endif()

  # RPM
  if(EXISTS "/usr/bin/rpmbuild")
    list(APPEND CPACK_GENERATOR RPM)
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
  endif()
endif()

# Component setup (simple single component by default)
set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE ON)

# Finalize
include(CPack)
