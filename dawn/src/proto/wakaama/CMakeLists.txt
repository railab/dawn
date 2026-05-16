# dawn/src/proto/wakaama/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_DAWN_PROTO_WAKAAMA)
  set(SRCS client.cxx object.cxx objects.cxx platform.cxx transport.cxx wakaama.cxx)

  target_link_libraries(dawn PUBLIC wakaama)
  if(CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK)
    target_link_libraries(dawn PUBLIC tinydtls)
  endif()

  target_sources(dawn PRIVATE ${SRCS})
endif()
