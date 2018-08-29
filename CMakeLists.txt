cmake_minimum_required(VERSION 3.10.2)

project("escapement" VERSION 0.1.0 DESCRIPTION "FTP Server File Synchronisation Program")

# Build Antik library

add_subdirectory(../Antik antik)

# Escapement sources and includes

file(GLOB PROGRAM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB PROGRAM_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" )

# Escapement target

add_executable(${PROJECT_NAME} ${PROGRAM_SOURCES} )
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} antik)

# Install Escapement

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
