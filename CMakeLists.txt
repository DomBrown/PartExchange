cmake_minimum_required(VERSION 3.10)

project(PartExchange)

# Locate custom Find*.cmake files
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)


# Convenience macros for setting directories, finding/checking packages
macro(require_pkg_directory pkg_name pkg_user_name)
  #message(STATUS "require_directory: name=${pkg_name}")
  option(${pkg_name}_DIR "Root folder for ${pkg_user_name} installation" OFF)
  if (NOT ${pkg_name}_DIR)
    message(
      FATAL_ERROR
      "Must specify valid ${pkg_user_name} installation root "
      "with -D${pkg_name}_DIR="
    )
  endif()
endmacro(require_pkg_directory)

macro(find_package_local pkg_name pkg_directory)
  message(
    STATUS "find_package_local: "
    "pkg name=\"${pkg_name}\", directory=\"${pkg_directory}\""
  )
  # search locally only for package
  find_package(
    ${pkg_name}
    PATHS ${pkg_directory}
    REQUIRED
    NO_CMAKE_PACKAGE_REGISTRY
    NO_CMAKE_BUILDS_PATH
    NO_CMAKE_SYSTEM_PATH
    NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
  )
endmacro(find_package_local)

macro(check_package_found pkg_name)
  #message(STATUS "check_package_found: name=\"${pkg_name}\"")
  if (NOT ${pkg_name}_FOUND)
    message(
      FATAL_ERROR
      "DARMA backend requires VT to include \"${pkg_name}\" package - "
      "package not found!"
    )
  else()
    message(STATUS "DARMA backend found required VT package: \"${pkg_name}\"")
  endif()
endmacro(check_package_found)


# require directories for these packages
require_pkg_directory(vt "VT")

# find these required packages
find_package_local(vt "${vt_DIR}/cmake")

# Find yaml
find_package(YamlCpp REQUIRED)

set(SOURCE_FILES
  src/PartExchange.cpp
  src/Particle.cpp
  src/ParticleMover.cpp
  src/ParticleContainer.cpp
  src/OutputWriter.cpp
  src/InputDeck.cpp
  src/GraphGenerator.cpp
)
set(HEADER_FILES
  src/Particle.hpp
  src/ParticleMover.hpp
  src/ParticleContainer.hpp
  src/OutputWriter.hpp
  src/CustomReducer.hpp
  src/InputDeck.hpp
  src/GraphGenerator.hpp
)

add_executable(PartExchange ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(PartExchange PUBLIC ${YamlCpp_INCLUDES})
target_link_libraries(PartExchange PUBLIC ${YamlCpp_LIBRARIES})
#message(STATUS "LIBS!!!!!! ${YamlCpp_LIBRARIES}")

#Uncomment the below if we fail to link ldl
target_link_libraries(PartExchange PUBLIC vt::runtime::vt -ldl)
#target_link_libraries(PartExchange PUBLIC ${YamlCpp_LIBRARIES})

