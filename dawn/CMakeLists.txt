# dawn/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

# dawnpy exports DAWN_BOARDS_COMMON, DAWN_EXTENSION_APPS_KCONFIG (and
# DAWN_OOT_ROOT for OOT builds) before invoking cmake. Fall back to
# in-tree defaults when they are not populated (e.g. cmake invoked
# directly by NTFC).
if(NOT DEFINED ENV{DAWN_BOARDS_COMMON})
  set(ENV{DAWN_BOARDS_COMMON} "${CMAKE_CURRENT_SOURCE_DIR}/../boards/common")
endif()
if(NOT DEFINED ENV{DAWN_EXTENSION_APPS_KCONFIG})
  # Sentinel: intentionally non-existent file so orsource silently skips;
  # same path dawnpy uses for in-tree builds (tools/dawnpy/src/dawnpy/dawn/project.py).
  set(ENV{DAWN_EXTENSION_APPS_KCONFIG}
      "${CMAKE_CURRENT_SOURCE_DIR}/../.dawn-no-extension-apps.Kconfig")
endif()

function(_dawn_try_bootstrap_oot_from_board_config)
  if(DEFINED ENV{DAWN_OOT_ROOT} AND DEFINED ENV{DAWN_EXTENSION_APPS_KCONFIG})
    return()
  endif()

  if(NOT DEFINED BOARD_CONFIG OR NOT IS_ABSOLUTE "${BOARD_CONFIG}")
    return()
  endif()

  get_filename_component(_board_config_real "${BOARD_CONFIG}" REALPATH)
  if(NOT IS_DIRECTORY "${_board_config_real}")
    return()
  endif()

  get_filename_component(_dawn_root "${CMAKE_CURRENT_SOURCE_DIR}/.." REALPATH)
  string(FIND "${_board_config_real}" "${_dawn_root}/" _in_tree_idx)
  if(NOT _in_tree_idx EQUAL -1)
    return()
  endif()

  set(_probe "${_board_config_real}")
  set(_oot_root "")
  while(TRUE)
    if(EXISTS "${_probe}/boards")
      set(_oot_root "${_probe}")
      break()
    endif()

    get_filename_component(_parent "${_probe}" DIRECTORY)
    if(_parent STREQUAL _probe)
      break()
    endif()
    set(_probe "${_parent}")
  endwhile()

  if(NOT _oot_root)
    return()
  endif()

  if(NOT DEFINED ENV{DAWN_OOT_ROOT})
    set(ENV{DAWN_OOT_ROOT} "${_oot_root}")
  endif()

  if((NOT DEFINED DAWN_OOT_CMAKE_FILE OR NOT DAWN_OOT_CMAKE_FILE)
     AND EXISTS "${_oot_root}/external/dawn_oot.cmake")
    set(
      DAWN_OOT_CMAKE_FILE "${_oot_root}/external/dawn_oot.cmake"
      CACHE STRING "OOT Dawn CMake extension file"
    )
  endif()

  if(NOT DEFINED ENV{DAWN_EXTENSION_APPS_KCONFIG}
     AND EXISTS "${_oot_root}/external/apps/Kconfig")
    set(
      ENV{DAWN_EXTENSION_APPS_KCONFIG}
      "${_oot_root}/external/apps/Kconfig"
    )
  endif()
endfunction()

_dawn_try_bootstrap_oot_from_board_config()

if(CONFIG_DAWN)
  nuttx_add_library(dawn)

  nuttx_add_subdirectory()

  target_link_libraries(dawn PUBLIC dawn_porting)
  target_include_directories(dawn_porting PUBLIC include)
  target_include_directories(dawn PUBLIC include)

endif()

nuttx_generate_kconfig(MENUDESC "Dawn")
