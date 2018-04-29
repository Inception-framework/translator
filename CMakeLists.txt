#===------------------------------------------------------------------------===#
#
#                     The Inception Translator & Merger
#
# This file is derived from the INCEPTION toolchain
#
# Modified by : Corteggiani Nassim
#
#===------------------------------------------------------------------------===#

###############################################################################
# Minimum CMake version and policies
###############################################################################
cmake_minimum_required(VERSION 2.8.12)
if (POLICY CMP0054)
  # FIXME: This is horrible. With the old behaviour,
  # quoted strings like "MSVC" in if() conditionals
  # get implicitly dereferenced. The NEW behaviour
  # doesn't do this but CMP0054 was only introduced
  # in CMake 3.1 and we support lower versions as the
  # minimum. We could set NEW here but it would be very
  # confusing to use NEW for some builds and OLD for others
  # which could lead to some subtle bugs. Instead when the
  # minimum version is 3.1 change this policy to NEW and remove
  # the hacks in place to work around it.
  cmake_policy(SET CMP0054 OLD)
endif()

if (POLICY CMP0042)
  # Enable `MACOSX_RPATH` by default.
  cmake_policy(SET CMP0042 NEW)
endif()

if (POLICY CMP0037)
  # Disallow reserved target names
  cmake_policy(SET CMP0037 NEW)
endif()

# This overrides the default flags for the different CMAKE_BUILD_TYPEs
set(CMAKE_USER_MAKE_RULES_OVERRIDE_C
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flags_override.cmake")
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flags_override.cmake")
project(INCEPTION CXX C)

###############################################################################
# Project version
###############################################################################
set(PACKAGE_URL "\"https://gitlab.eurecom.fr/nasm/Inception-compiler.git\"")

################################################################################
# Set various useful variables depending on CMake version
################################################################################
if (("${CMAKE_VERSION}" VERSION_EQUAL "3.2") OR ("${CMAKE_VERSION}" VERSION_GREATER "3.2"))
  # In CMake >= 3.2 add_custom_command() supports a ``USES_TERMINAL`` argument
  set(ADD_CUSTOM_COMMAND_USES_TERMINAL_ARG "USES_TERMINAL")
else()
  set(ADD_CUSTOM_COMMAND_USES_TERMINAL_ARG "")
endif()

if (("${CMAKE_VERSION}" VERSION_EQUAL "3.4") OR ("${CMAKE_VERSION}" VERSION_GREATER "3.4"))
  # In CMake >= 3.4 ExternalProject_Add_Step() supports a `USES_TERMINAL` argument
  set(EXTERNAL_PROJECT_ADD_STEP_USES_TERMINAL_ARG "USES_TERMINAL" "1")
else()
  set(EXTERNAL_PROJECT_ADD_STEP_USES_TERMINAL_ARG "")
endif()

################################################################################
# Sanity check - Disallow building in source.
# Otherwise we would overwrite the Makefiles of the old build system.
################################################################################
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In source builds are not allowed. You should invoke "
          "CMake from a different directory.")
endif()

################################################################################
# Build type
################################################################################
message(STATUS "CMake generator: ${CMAKE_GENERATOR}")
if (DEFINED CMAKE_CONFIGURATION_TYPES)
  # Multi-configuration build (e.g. Xcode). Here
  # CMAKE_BUILD_TYPE doesn't matter
  message(STATUS "Available configurations: ${CMAKE_CONFIGURATION_TYPES}")
else()
  # Single configuration generator (e.g. Unix Makefiles, Ninja)
  set(available_build_types Debug Release RelWithDebInfo MinSizeRel)
  if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE is not set. Setting default")
    message(STATUS "The available build types are: ${available_build_types}")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE String
        "Options are ${available_build_types}"
        FORCE)
    # Provide drop down menu options in cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${available_build_types})
  endif()
  message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

  # Check the selected build type is valid
  list(FIND available_build_types "${CMAKE_BUILD_TYPE}" _build_type_index)
  if ("${_build_type_index}" EQUAL "-1")
    message(FATAL_ERROR "\"${CMAKE_BUILD_TYPE}\" is an invalid build type.\n"
      "Use one of the following build types ${available_build_types}")
  endif()
endif()

################################################################################
# Compiler flags for INCEPTION components
# Subsequent commands will append to these. These are used instead of
# directly modifying CMAKE_CXX_FLAGS so that other code can be easily built with
# different flags.
################################################################################
set(INCEPTION_COMPONENT_EXTRA_INCLUDE_DIRS "")
set(INCEPTION_COMPONENT_CXX_DEFINES "")
set(INCEPTION_COMPONENT_CXX_FLAGS "")
set(INCEPTION_COMPONENT_EXTRA_LIBRARIES "")

################################################################################
# Assertions
################################################################################
option(ENABLE_INCEPTION_ASSERTS "Enable INCEPTION assertions" ON)
if (ENABLE_INCEPTION_ASSERTS)
  message(STATUS "INCEPTION assertions enabled")
  # Assume that -DNDEBUG isn't set.
else()
  message(STATUS "INCEPTION assertions disabled")
  list(APPEND INCEPTION_COMPONENT_CXX_DEFINES "-DNDEBUG")
endif()

################################################################################
# INCEPTION timestamps
################################################################################
option(INCEPTION_ENABLE_TIMESTAMP "Add timestamps to INCEPTION sources" OFF)

################################################################################
# Include useful CMake functions
################################################################################
include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckPrototypeDefinition)
include("${CMAKE_SOURCE_DIR}/cmake/string_to_list.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/inception_component_add_cxx_flag.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/add_global_flag.cmake")


################################################################################
# Find LLVM
################################################################################
include(${CMAKE_SOURCE_DIR}/cmake/find_llvm.cmake)
set(NEEDED_LLVM_VARS
  LLVM_PACKAGE_VERSION
  LLVM_VERSION_MAJOR
  LLVM_VERSION_MINOR
  LLVM_VERSION_PATCH
  LLVM_DEFINITIONS
  LLVM_ENABLE_ASSERTIONS
  LLVM_ENABLE_EH
  LLVM_ENABLE_RTTI
  LLVM_INCLUDE_DIRS
  LLVM_LIBRARY_DIRS
  LLVM_TOOLS_BINARY_DIR
  LLVM_ENABLE_VISIBILITY_INLINES_HIDDEN
  TARGET_TRIPLE
)

foreach (vname ${NEEDED_LLVM_VARS})
  message(STATUS "${vname}: \"${${vname}}\"")
  if (NOT (DEFINED "${vname}"))
    message(FATAL_ERROR "${vname} was not defined")
  endif()
endforeach()

if (LLVM_ENABLE_ASSERTIONS)
  # Certain LLVM debugging macros only work when LLVM was built with asserts
  set(ENABLE_INCEPTION_DEBUG 1) # for config.h
else()
  unset(ENABLE_INCEPTION_DEBUG) # for config.h
endif()

# Warn about mixing build types.
# This is likely a bad idea because some of LLVM's header files use the NDEBUG
# macro which can change things like data layout.
if (LLVM_ENABLE_ASSERTIONS AND (NOT ENABLE_INCEPTION_ASSERTS))
  message(WARNING
    "LLVM was built with assertions but INCEPTION will be built without them.\n"
    "This might lead to unexpected behaviour."
  )
elseif ((NOT LLVM_ENABLE_ASSERTIONS) AND ENABLE_INCEPTION_ASSERTS)
  message(WARNING
    "LLVM was built without assertions but INCEPTION will be built with them.\n"
    "This might lead to unexpected behaviour."
  )
endif()

if (LLVM_ENABLE_VISIBILITY_INLINES_HIDDEN)
  list(APPEND INCEPTION_COMPONENT_CXX_FLAGS "-fvisibility-inlines-hidden")
endif()


list(APPEND INCEPTION_COMPONENT_CXX_DEFINES ${LLVM_DEFINITIONS})
list(APPEND INCEPTION_COMPONENT_EXTRA_INCLUDE_DIRS ${LLVM_INCLUDE_DIRS})

# Find llvm-link
set(LLVM_LINK "${LLVM_TOOLS_BINARY_DIR}/llvm-link")
if (NOT EXISTS "${LLVM_LINK}")
  message(FATAL_ERROR "Failed to find llvm-link at \"${LLVM_LINK}\"")
endif()

# Find llvm-ar
set(LLVM_AR "${LLVM_TOOLS_BINARY_DIR}/llvm-ar")
if (NOT EXISTS "${LLVM_AR}")
  message(FATAL_ERROR "Failed to find llvm-ar at \"${LLVM_AR}\"")
endif()

# Find llvm-as
set(LLVM_AS "${LLVM_TOOLS_BINARY_DIR}/llvm-as")
if (NOT EXISTS "${LLVM_AS}")
  message(FATAL_ERROR "Failed to find llvm-as at \"${LLVM_AS}\"")
endif()

################################################################################
# C++ version
################################################################################
if ("${CMAKE_VERSION}" VERSION_LESS "3.1")
  add_global_cxx_flag("-std=c++11" REQUIRED)
else ()
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif ()

################################################################################
# Warnings
################################################################################
include(${CMAKE_SOURCE_DIR}/cmake/compiler_warnings.cmake)

###############################################################################
# Exception handling
###############################################################################
if (NOT LLVM_ENABLE_EH)
    inception_component_add_cxx_flag("-fno-exceptions" REQUIRED)
endif()

###############################################################################
# RTTI
###############################################################################
if (NOT LLVM_ENABLE_RTTI)
    inception_component_add_cxx_flag("-fno-rtti" REQUIRED)
endif()

################################################################################
# Miscellaneous header file detection
################################################################################
check_function_exists(mallinfo HAVE_MALLINFO) # FIXME: should test CXX compiler not C
check_function_exists(__ctype_b_loc HAVE_CTYPE_EXTERNALS) # FIXME: should test CXX compiler not C

check_include_file_cxx(malloc/malloc.h HAVE_MALLOC_MALLOC_H)
check_function_exists(malloc_zone_statistics HAVE_MALLOC_ZONE_STATISTICS)

# FIXME: This is needed by the runtime not INCEPTION itself so we are testing the wrong
# compiler.
check_include_file("selinux/selinux.h" HAVE_SELINUX_SELINUX_H)
check_include_file("sys/acl.h" HAVE_SYS_ACL_H)
if (HAVE_SELINUX_SELINUX_H)
  message(STATUS "SELinux support enabled")
  set(HAVE_SELINUX 1)
  # Test what function signature we need to use for SELinux. The signatures
  # have changed between 2.2 and 2.3. In particular, the type of the "security
  # context" parameter was changed from char * to const char *, with this
  # patch: [PATCH] Get rid of security_context_t and fix const declarations.
  # [http://www.spinics.net/lists/selinux/msg14827.html]
  check_prototype_definition(setcon
    "int setcon(char* context)"
    "0"
    "selinux/selinux.h"
    SELINUX_SECURITY_CTX_NO_CONST
  )
  if (SELINUX_SECURITY_CTX_NO_CONST)
    message(STATUS "security_context_t is char*")
    set(INCEPTION_SELINUX_CTX_CONST " ")
  else()
    check_prototype_definition(setcon
      "int setcon(const char* context)"
      "0"
      "selinux/selinux.h"
      SELINUX_SECURITY_CTX_WITH_CONST
    )
    if (SELINUX_SECURITY_CTX_WITH_CONST)
      message(STATUS "security_context_t is const char*")
      set(INCEPTION_SELINUX_CTX_CONST "const")
    else()
      message(FATAL_ERROR "Failed to determine function signature for \"setcon\"")
    endif()
  endif()
else()
  message(STATUS "SELinux support disabled")
  set(HAVE_SELINUX 0)
endif()

################################################################################
# Global clean target
################################################################################
# CMake already uses the "clean" target name but it doesn't clean everything
# unfortunately. We can't modify the target so we provide our own "clean_all"
# target that runs clean. Other rules for performing clean up should declare
# that "clean_all" depends on those rules.
add_custom_target(clean_all
  # Invoke CMake's own clean target
  COMMAND
    "${CMAKE_COMMAND}"
    "--build"
    "${CMAKE_BINARY_DIR}"
    "--target"
    "clean"
)

################################################################################
# Generate `config.h`
################################################################################
configure_file(${CMAKE_SOURCE_DIR}/include/Config/config.h.cmin
  ${CMAKE_BINARY_DIR}/include/Config/config.h)

################################################################################
# Generate `CompileTimeInfo.h`
################################################################################
# if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
#   # Get information from git. We use third-party code to do this. The nice
#   # thing about this code is it will trigger a re-configure if the HEAD changes
#   # which means when we build INCEPTION, it should always have the correct git
#   # information.
#   include(${CMAKE_SOURCE_DIR}/cmake/GetGitRevisionDescription.cmake)
#   get_git_head_revision(_NOT_USED_INCEPTION_GIT_REFSPEC INCEPTION_GIT_SHA1HASH)
#   message(STATUS "INCEPTION_GIT_SHA1HASH: ${INCEPTION_GIT_SHA1HASH}")
# else()
#   set(INCEPTION_GIT_SHA1HASH "unknown")
# endif()
# set(AUTO_GEN_MSG "AUTOMATICALLY GENERATED. DO NOT EDIT!")
# configure_file(${CMAKE_SOURCE_DIR}/include/Config/CompileTimeInfo.h.cmin
#   ${CMAKE_BINARY_DIR}/include/Config/CompileTimeInfo.h
# )

################################################################################
# Global include directories
################################################################################
include_directories("${CMAKE_BINARY_DIR}/include")
include_directories("${CMAKE_SOURCE_DIR}/include")

################################################################################
# Set default location for targets in the build directory
################################################################################
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


################################################################################
# Report the value of various variables to aid debugging
################################################################################
message(STATUS "INCEPTION_COMPONENT_EXTRA_INCLUDE_DIRS: '${INCEPTION_COMPONENT_EXTRA_INCLUDE_DIRS}'")
message(STATUS "INCEPTION_COMPONENT_CXX_DEFINES: '${INCEPTION_COMPONENT_CXX_DEFINES}'")
message(STATUS "INCEPTION_COMPONENT_CXX_FLAGS: '${INCEPTION_COMPONENT_CXX_FLAGS}'")
message(STATUS "INCEPTION_COMPONENT_EXTRA_LIBRARIES: '${INCEPTION_COMPONENT_EXTRA_LIBRARIES}'")

################################################################################
# INCEPTION components
################################################################################
include("${CMAKE_SOURCE_DIR}/cmake/inception_add_component.cmake")
add_subdirectory(lib)

################################################################################
# INCEPTION tools
################################################################################
add_subdirectory(tools)

################################################################################
# Testing
################################################################################
# option(ENABLE_UNIT_TESTS "Enable unit tests" ON)
# option(ENABLE_SYSTEM_TESTS "Enable system tests" ON)
#
# # This provides a migration path for older build directories that have this
# # variable set in their cache. Leaving it behind could lead to confusion so
# # removing it is probably a good idea.
# # TODO: Remove this eventually (probably next release or something).
# if (DEFINED ENABLE_TESTS)
#   message(WARNING "You have the \"ENABLE_TESTS\" variable is your CMake cache."
#     "This variable no longer has any meaning so removing it from your cache")
#   unset(ENABLE_TESTS CACHE)
# endif()
#
# if (ENABLE_UNIT_TESTS OR ENABLE_SYSTEM_TESTS)
#   message(STATUS "Testing is enabled")
#
#   # Find lit
#   set(LIT_TOOL_NAMES "llvm-lit" "lit")
#   find_program(
#     LIT_TOOL
#     NAMES ${LIT_TOOL_NAMES}
#     HINTS "${LLVM_TOOLS_BINARY_DIR}"
#     DOC "Path to lit tool"
#   )
#
#   set(LIT_ARGS
#     "-v;-s"
#     CACHE
#     STRING
#     "Lit arguments"
#   )
#
#   if ((NOT LIT_TOOL) OR (NOT EXISTS "${LIT_TOOL}"))
#     message(FATAL_ERROR "The lit tool is required for testing."
#       " CMake tried to find lit with the following names \"${LIT_TOOL_NAMES}\""
#       " but it could not be found.\n"
#       "You should either disable testing by passing "
#       "\"-DENABLE_UNIT_TESTS=OFF -DENABLE_SYSTEM_TESTS=OFF\" to cmake"
#       " or you should install the lit tool from the Python Package Index by running"
#       " \"pip install lit\". Note \"pip\" requires root privileges to run. If you"
#       " don't have root privileges you can create a virtual python environment using"
#       " the \"virtualenv\" tool and run \"pip\" from there.")
#   else()
#     message(STATUS "Using lit: ${LIT_TOOL}")
#   endif()
#
#   # Add global test target
#   add_custom_target(check
#     COMMENT "Running tests"
#   )
# else()
#   message(STATUS "Testing is disabled")
# endif()
#
#
# if (ENABLE_UNIT_TESTS)
#   message(STATUS "Unit tests enabled")
#   add_subdirectory(unittests)
#   add_dependencies(check unittests)
# else()
#   message(STATUS "Unit tests disabled")
# endif()
# if (ENABLE_SYSTEM_TESTS)
#   message(STATUS "System tests enabled")
#   add_subdirectory(test)
#   add_dependencies(check systemtests)
# else()
#   message(STATUS "System tests disabled")
# endif()

################################################################################
# Documentation
################################################################################
# option(ENABLE_DOCS "Enable building documentation" ON)
# if (ENABLE_DOCS)
#   add_subdirectory(docs)
# endif()
