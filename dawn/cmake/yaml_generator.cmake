# dawn/cmake/yaml_generator.cmake
#
# SPDX-License-Identifier: Apache-2.0
#

include_guard(GLOBAL)

get_filename_component(DAWN_YAML_GENERATOR_DIR "${CMAKE_CURRENT_LIST_DIR}" REALPATH)

function(dawn_enable_yaml_descriptor_generation)
  cmake_parse_arguments(
    DAWN_DESC
    ""
    "TARGET;YAML_PATH;OUTPUT_DEFINE"
    ""
    ${ARGN}
  )

  if(NOT DAWN_DESC_TARGET)
    message(FATAL_ERROR "dawn_enable_yaml_descriptor_generation: TARGET is required")
  endif()

  if(NOT DAWN_DESC_YAML_PATH)
    message(FATAL_ERROR "dawn_enable_yaml_descriptor_generation: YAML_PATH is empty")
  endif()

  if(NOT DAWN_DESC_OUTPUT_DEFINE)
    set(DAWN_DESC_OUTPUT_DEFINE DAWN_APPS_EXAMPLE_DESC_GENERATED_PATH)
  endif()

  if(NOT Python3_EXECUTABLE)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
  endif()

  if(NOT DEFINED DAWN_DAWNPY_COMMAND)
    set(
      DAWN_DAWNPY_COMMAND
      "${Python3_EXECUTABLE};-m;dawnpy"
      CACHE STRING
      "Command used to invoke the dawnpy CLI"
    )
  endif()

  if(NOT DEFINED DAWN_DAWNPY_PYTHONPATH)
    set(DAWN_DAWNPY_PYTHONPATH "")
  endif()

  get_filename_component(
    DAWN_ROOT_DIR "${DAWN_YAML_GENERATOR_DIR}/../.." REALPATH
  )

  if(
    NOT DAWN_DAWNPY_PYTHONPATH
    AND EXISTS "${DAWN_ROOT_DIR}/tools/dawnpy/src/dawnpy/__init__.py"
  )
    set(DAWN_DAWNPY_PYTHONPATH "${DAWN_ROOT_DIR}/tools/dawnpy/src")
  endif()

  set(DAWN_DESC_YAML_PATH_RAW ${DAWN_DESC_YAML_PATH})
  string(REPLACE "\"" "" DAWN_DESC_YAML_PATH_CFG
    "${DAWN_DESC_YAML_PATH_RAW}"
  )
  string(REGEX REPLACE "^(\\.\\./)+" ""
    DAWN_DESC_YAML_PATH_REL "${DAWN_DESC_YAML_PATH_CFG}")
  # For OOT projects, try DAWN_OOT_ROOT first; fall back to DAWN_ROOT_DIR
  # for in-tree builds.
  set(DAWN_DESC_YAML_PATH_ABS "")
  if(DEFINED ENV{DAWN_OOT_ROOT})
    set(_oot_candidate "$ENV{DAWN_OOT_ROOT}/${DAWN_DESC_YAML_PATH_REL}")
    if(EXISTS "${_oot_candidate}")
      set(DAWN_DESC_YAML_PATH_ABS "${_oot_candidate}")
    endif()
  endif()
  if(NOT DAWN_DESC_YAML_PATH_ABS)
    set(DAWN_DESC_YAML_PATH_ABS "${DAWN_ROOT_DIR}/${DAWN_DESC_YAML_PATH_REL}")
  endif()
  set(DAWN_DAWNPY_COMMAND_LIST ${DAWN_DAWNPY_COMMAND})
  if(DAWN_DAWNPY_PYTHONPATH)
    file(
      GLOB_RECURSE
      DAWN_DAWNPY_MODULES
      "${DAWN_DAWNPY_PYTHONPATH}/dawnpy/*.py"
    )
    set(
      DAWN_DAWNPY_RUNNER
      ${CMAKE_COMMAND} -E env PYTHONPATH=${DAWN_DAWNPY_PYTHONPATH}
      ${DAWN_DAWNPY_COMMAND_LIST}
    )
  else()
    set(DAWN_DAWNPY_MODULES "")
    set(DAWN_DAWNPY_RUNNER ${DAWN_DAWNPY_COMMAND_LIST})
  endif()
  set(DAWN_DESC_GEN_OUT
      "${CMAKE_CURRENT_BINARY_DIR}/generated_descriptor.cxx")

  # Out-of-tree custom-type registry: when an OOT project ships a
  # `dawnpy_types.py` at its root, pass it to dawnpy via the global
  # `--types-from` flag so the generated descriptor can reference custom
  # IO/PROG/PROTO types defined by the project.

  set(DAWN_DAWNPY_GLOBAL_FLAGS "")
  set(DAWN_DAWNPY_EXTRA_DEPENDS "")
  if(DEFINED ENV{DAWN_OOT_ROOT})
    set(_oot_types "$ENV{DAWN_OOT_ROOT}/dawnpy_types.py")
    if(EXISTS "${_oot_types}")
      list(APPEND DAWN_DAWNPY_GLOBAL_FLAGS --types-from "${_oot_types}")
      list(APPEND DAWN_DAWNPY_EXTRA_DEPENDS "${_oot_types}")
    endif()
  endif()

  add_custom_command(
    OUTPUT ${DAWN_DESC_GEN_OUT}
    COMMAND ${DAWN_DAWNPY_RUNNER}
      ${DAWN_DAWNPY_GLOBAL_FLAGS}
      desc-gen
      ${DAWN_DESC_YAML_PATH_ABS}
      -o ${DAWN_DESC_GEN_OUT}
    DEPENDS
      ${DAWN_DESC_YAML_PATH_ABS}
      ${DAWN_DAWNPY_MODULES}
      ${DAWN_DAWNPY_EXTRA_DEPENDS}
    COMMENT "Generating Dawn descriptor from YAML"
    VERBATIM
  )

  set(DAWN_DESC_GEN_TARGET "${DAWN_DESC_TARGET}_descriptor_gen")
  add_custom_target(${DAWN_DESC_GEN_TARGET} DEPENDS ${DAWN_DESC_GEN_OUT})
  add_dependencies(${DAWN_DESC_TARGET} ${DAWN_DESC_GEN_TARGET})

  target_compile_definitions(
    ${DAWN_DESC_TARGET} PRIVATE
    ${DAWN_DESC_OUTPUT_DEFINE}="${DAWN_DESC_GEN_OUT}"
  )
endfunction()
