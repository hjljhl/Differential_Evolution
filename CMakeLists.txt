project(de)
cmake_minimum_required(VERSION 3.2.1)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE debug)
endif()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# All targets
set(DE_SHARED de)         # de shared library
set(DE_STATIC de-static)  # de static library

# source list
set(DE_INC
    inc/global.h
    inc/DE/SaDE.h
    inc/DE/DERandomF.h
    inc/DE/DEOrigin.h
    inc/DE/strategy/DEInterface.h
    inc/DE/strategy/DEBuiltInStrategy.h)
set(DE_SRC 
    src/global.cpp
    src/DE/SaDE.cpp
    src/DE/DERandomF.cpp
    src/DE/DEOrigin.cpp
    src/DE/strategy/DEInterface.cpp
    src/DE/strategy/DEBuiltInStrategy.cpp)
if(WIN32) # for visual studio
    set(DE_SRC     ${DE_SRC}     ${DE_INC})
endif(WIN32)

# build targets
include_directories(inc)
add_library(${DE_SHARED} SHARED ${DE_SRC})
add_library(${DE_STATIC} STATIC ${DE_SRC})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${DE_SHARED} ${DE_STATIC} PROPERTY CXX_STANDARD 11)

# find dependencies
# use can use -DBOOST_ROOT or -DBOOST_INCLUDEDIR and -DBOOST_LIBRARYDIR as hint
find_package(Boost 1.57 REQUIRED)
if(Boost_FOUND)
    message(STATUS "boost inc dir: ${Boost_INCLUDE_DIR}")
    message(STATUS "boost lib dir: ${Boost_LIB_DIR}")
    include_directories(${Boost_INCLUDE_DIR})
else(Boost_FOUND)
    message(FATAL_ERROR "Boost not found")
endif(Boost_FOUND)
find_package(OpenMP)
if(OPENMP_FOUND)
    message(STATUS "Support OpenMP: ${OpenMP_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
else(OPENMP_FOUND)
	message(FATAL_ERROR "OpenMP not found")
endif(OPENMP_FOUND)

# install program, libs, headers and docs
if(CMAKE_INSTALL_PREFIX)
    message(STATUS "Cmake install prefix: ${CMAKE_INSTALL_PREFIX}")
    install(TARGETS ${DE_SHARED} ${DE_STATIC}
            RUNTIME DESTINATION bin
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib)
    install(FILES inc/DifferentialEvolution.h
            DESTINATION inc
            PERMISSIONS OWNER_READ GROUP_READ)
    install(DIRECTORY inc/DE 
            DESTINATION inc)
    install(FILES README.md
            DESTINATION doc
            PERMISSIONS OWNER_READ GROUP_READ)
endif()
