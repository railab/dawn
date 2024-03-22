# dawn/cmake/dawn_oot.cmake
#
# SPDX-License-Identifier: Apache-2.0
#

# Wire out-of-tree user extensions into the dawn build.
#
# An out-of-tree (OOT) project is a directory containing `boards/` that
# lives outside the upstream Dawn repo. dawnpy detects such projects from
# the requested defconfig path, exports DAWN_OOT_ROOT into the cmake/Kconfig
# environment, and passes the OOT cmake extension entry point explicitly as
# -DDAWN_OOT_CMAKE_FILE=<abs path> when the project provides one.
#
# Functions exposed by this module:
#
#   dawn_oot_link()
#       Called from dawn/src/CMakeLists.txt. Pulls user IO/PROG/PROTO
#       sources into the dawn library by including
#       ${DAWN_OOT_CMAKE_FILE} and calling the user hook
#       ``dawn_oot_user_link_dawn(dawn)``. Also adds
#       $DAWN_OOT_ROOT/external/include to dawn's PUBLIC include path.
#
#   dawn_oot_link_apps()
#       Called from dawn/apps/CMakeLists.txt. Pulls user-defined NuttX
#       applications by including ${DAWN_OOT_CMAKE_FILE}
#       and calling the user hook ``dawn_oot_user_link_apps()``. The
#       matching Kconfig hook is in dawn/apps/Kconfig (orsource of
#       $(DAWN_OOT_ROOT)/external/apps/Kconfig).
#
#   dawn_oot_app_setup(<target>)
#       Called from dawn/apps/dawn/CMakeLists.txt with apps_dawn as the
#       target. Adds DAWN_OOT_ROOT to the target include path (so user
#       descriptor paths resolve relative to the OOT project), then calls
#       the user hook ``dawn_oot_user_setup_app(<target>)`` from
#       ${DAWN_OOT_CMAKE_FILE} so the OOT project can attach any required
#       C++ files itself.
#
# All three functions are no-ops when DAWN_OOT_ROOT is unset.
#

set(
  DAWN_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}"
  CACHE INTERNAL "Dawn common CMake module directory"
)

function(_dawn_oot_get_root out_var)
  if(NOT DEFINED ENV{DAWN_OOT_ROOT})
    set(${out_var} "" PARENT_SCOPE)
    return()
  endif()

  set(_oot_root "$ENV{DAWN_OOT_ROOT}")

  if(NOT IS_DIRECTORY "${_oot_root}")
    message(WARNING
      "DAWN_OOT_ROOT='${_oot_root}' is not a directory; ignoring.")
    set(${out_var} "" PARENT_SCOPE)
    return()
  endif()

  set(${out_var} "${_oot_root}" PARENT_SCOPE)
endfunction()


function(_dawn_oot_include_user_hooks out_var)
  _dawn_oot_get_root(_oot_root)
  if(NOT _oot_root)
    set(${out_var} "" PARENT_SCOPE)
    return()
  endif()

  if(NOT DEFINED DAWN_OOT_CMAKE_FILE OR NOT DAWN_OOT_CMAKE_FILE)
    set(${out_var} "" PARENT_SCOPE)
    return()
  endif()

  if(IS_ABSOLUTE "${DAWN_OOT_CMAKE_FILE}")
    set(_user_hooks "${DAWN_OOT_CMAKE_FILE}")
  else()
    get_filename_component(
      _user_hooks "${DAWN_OOT_CMAKE_FILE}" REALPATH
      BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    )
  endif()

  if(EXISTS "${_user_hooks}")
    include("${_user_hooks}")
    set(${out_var} "${_user_hooks}" PARENT_SCOPE)
    return()
  endif()

  message(WARNING
    "DAWN_OOT_CMAKE_FILE='${DAWN_OOT_CMAKE_FILE}' does not exist; "
    "ignoring OOT CMake extensions.")
  set(${out_var} "" PARENT_SCOPE)
endfunction()

function(dawn_oot_link)
  _dawn_oot_get_root(_oot_root)
  if(NOT _oot_root)
    return()
  endif()

  _dawn_oot_include_user_hooks(_user_hooks)

  message(STATUS "Dawn out-of-tree root: ${_oot_root}")

  if(_user_hooks AND COMMAND dawn_oot_user_link_dawn)
    dawn_oot_user_link_dawn(dawn)
  endif()

  if(IS_DIRECTORY "${_oot_root}/external/include")
    target_include_directories(dawn PUBLIC "${_oot_root}/external/include")
  endif()
endfunction()


function(dawn_oot_link_apps)
  _dawn_oot_get_root(_oot_root)
  if(NOT _oot_root)
    return()
  endif()

  _dawn_oot_include_user_hooks(_user_hooks)

  if(_user_hooks AND COMMAND dawn_oot_user_link_apps)
    dawn_oot_user_link_apps()
  endif()
endfunction()


function(dawn_oot_app_setup target)
  _dawn_oot_get_root(_oot_root)
  if(NOT _oot_root)
    return()
  endif()

  _dawn_oot_include_user_hooks(_user_hooks)

  target_include_directories(${target} PRIVATE "${_oot_root}")

  if(_user_hooks AND COMMAND dawn_oot_user_setup_app)
    dawn_oot_user_setup_app(${target})
  endif()
endfunction()
