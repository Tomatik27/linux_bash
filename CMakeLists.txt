cmake_minimum_required(VERSION 3.10) 
project(kubsh VERSION 1.0) 
add_executable(kubsh main.cpp) # 
Replace main.cpp with your source files

install(TARGETS kubsh DESTINATION bin)

include(CPack) 
set(CPACK_GENERATOR "DEB") 
set(CPACK_PACKAGE_NAME "mycppapp") 
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}") 
set(CPACK_PACKAGE_VENDOR 
"Your Company Name") 
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simple C++ application") 
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name <your.email@example.com>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6")
