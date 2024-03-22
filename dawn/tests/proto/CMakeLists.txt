# dawn/tests/proto/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

set(SRCS test_run.cxx test_handler.cxx test_common.cxx test_factory.cxx)

if(CONFIG_DAWN_PROTO_SHELL)
  list(APPEND SRCS test_shell.cxx)
endif()

if(CONFIG_DAWN_PROTO_SERIAL)
  list(APPEND SRCS test_serial.cxx)
endif()

if(CONFIG_DAWN_PROTO_UDP)
  list(APPEND SRCS test_udp.cxx)
endif()

if(CONFIG_DAWN_PROTO_IPC)
  list(APPEND SRCS test_ipc.cxx)
endif()

if(CONFIG_DAWN_PROTO_NXSCOPE_DUMMY)
  list(APPEND SRCS test_nxscope_dummy.cxx)
endif()

if(CONFIG_DAWN_PROTO_NXSCOPE_SERIAL)
  list(APPEND SRCS test_nxscope_serial.cxx)
endif()

if(CONFIG_DAWN_PROTO_NXSCOPE_UDP)
  list(APPEND SRCS test_nxscope_udp.cxx)
endif()

if(CONFIG_DAWN_PROTO_NIMBLE)
  list(APPEND SRCS test_nimble.cxx)
endif()

if(CONFIG_DAWN_PROTO_CAN)
  list(APPEND SRCS test_can.cxx)
endif()

if(CONFIG_DAWN_PROTO_MODBUS_RTU)
  list(APPEND SRCS test_modbus_rtu.cxx)
endif()

if(CONFIG_DAWN_PROTO_MODBUS_TCP)
  list(APPEND SRCS test_modbus_tcp.cxx)
endif()

target_sources(dawn_tests PRIVATE ${SRCS})
