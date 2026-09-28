# Install script for directory: /work/hitr-psp/libs

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local/pspdev/psp")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/local/pspdev/bin/psp-objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/work/hitr-psp/build/libs/choreo/cmake_install.cmake")
  include("/work/hitr-psp/build/libs/poser/cmake_install.cmake")
  include("/work/hitr-psp/build/libs/libpng/cmake_install.cmake")
  include("/work/hitr-psp/build/libs/pure3d/cmake_install.cmake")
  include("/work/hitr-psp/build/libs/radcontent/cmake_install.cmake")
  include("/work/hitr-psp/build/libs/radcore/cmake_install.cmake")
  include("/work/hitr-psp/build/libs/radmath/cmake_install.cmake")
  include("/work/hitr-psp/build/libs/radmovie/cmake_install.cmake")
  include("/work/hitr-psp/build/libs/radmusic/cmake_install.cmake")
  include("/work/hitr-psp/build/libs/radscript/cmake_install.cmake")
  include("/work/hitr-psp/build/libs/radsound/cmake_install.cmake")
  include("/work/hitr-psp/build/libs/scrooby/cmake_install.cmake")
  include("/work/hitr-psp/build/libs/sim/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/work/hitr-psp/build/libs/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
