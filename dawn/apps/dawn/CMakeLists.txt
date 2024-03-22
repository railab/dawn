# dawn/apps/dawn/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_DAWN_APPS_DAWN)

  include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/yaml_generator.cmake)

  list(APPEND CSRCS dawn_parseargs.cxx dawn_descriptor.cxx)

  nuttx_add_application(
    NAME
    dawn
    SRCS
    dawn_main.cxx
    ${CSRCS}
    STACKSIZE
    ${CONFIG_DAWN_APPS_DAWN_STACKSIZE})

  if(CONFIG_DAWN_APPS_EXAMPLE_DESC_FORMAT_YAML)
    dawn_enable_yaml_descriptor_generation(
      TARGET apps_dawn
      YAML_PATH ${CONFIG_DAWN_APPS_EXAMPLE_DESC_YAML_PATH}
    )
  endif()

  # Out-of-tree wiring (descriptor include path + OOT-owned target setup).
  include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/dawn_oot.cmake")
  dawn_oot_app_setup(apps_dawn)

  target_link_libraries(apps_dawn PUBLIC dawn)

endif()
