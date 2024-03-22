# dawn/src/proto/nimble/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_DAWN_PROTO_NIMBLE)
  set(SRCS adv.cxx hci.cxx host.cxx)

  if(CONFIG_DAWN_PROTO_NIMBLE_PERIPHERAL)
    list(APPEND SRCS gatt_runtime.cxx prph.cxx prph_custom.cxx)
  endif()

  if(CONFIG_DAWN_PROTO_NIMBLE_DIS)
    list(APPEND SRCS prph_dis.cxx)
  endif()

  if(CONFIG_DAWN_PROTO_NIMBLE_BAS)
    list(APPEND SRCS prph_bas.cxx)
  endif()

  if(CONFIG_DAWN_PROTO_NIMBLE_AIOS)
    list(APPEND SRCS prph_aios.cxx)
  endif()

  if(CONFIG_DAWN_PROTO_NIMBLE_ESS)
    list(APPEND SRCS prph_ess.cxx)
  endif()

  if(CONFIG_DAWN_PROTO_NIMBLE_IMDS)
    list(APPEND SRCS prph_imds.cxx)
  endif()

  if(CONFIG_DAWN_PROTO_NIMBLE_OTS)
    list(APPEND SRCS prph_ots.cxx)
  endif()

  target_link_libraries(dawn PUBLIC nimble)
  target_sources(dawn PRIVATE ${SRCS})
endif()
