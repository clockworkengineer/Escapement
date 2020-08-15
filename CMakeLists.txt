cmake_minimum_required(VERSION 3.10.2)

project("Escapement" VERSION 0.1.0 DESCRIPTION "FTP Server File Synchronisation Program")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra")

# Build Antik library

add_subdirectory(antik)

# Escapement sources and includes

set (ESCAPEMENT_SOURCES
    Escapement.cpp
    Escapement_CommandLine.cpp
    Escapement_FileCache.cpp
    Escapement_Files.cpp
)

set (ESCAPEMENT_INCLUDES
    Escapement.hpp
    Escapement_CommandLine.hpp
    Escapement_FileCache.hpp
    Escapement_Files.hpp
)

# Escapement target

add_executable(${PROJECT_NAME} ${ESCAPEMENT_SOURCES} )
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} antik)

# Install Escapement

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
