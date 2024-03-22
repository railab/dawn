// dawn/tests/proto/test_run.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <debug.h>

#include <cstdio>

#include "test_common.hxx"

extern "C"
{
  // Common

  int test_proto_common();
  int test_proto_handler();
  int test_proto_factory();

  // Protocol classes

  int test_proto_nxscope_dummy();
  int test_proto_nxscope_serial();
  int test_proto_nxscope_udp();
  int test_proto_shell();
  int test_proto_serial();
  int test_proto_udp();
  int test_proto_ipc();
  int test_proto_nimbleprph();
  int test_proto_can();
  int test_proto_modbus_rtu();
  int test_proto_modbus_tcp();
}

static int (*test_array[])(void) = {
  // Common

  test_proto_common,
  test_proto_handler,
  test_proto_factory,

// Protocol classes

#ifdef CONFIG_DAWN_PROTO_SHELL
  test_proto_shell,
#endif

#ifdef CONFIG_DAWN_PROTO_SERIAL
  test_proto_serial,
#endif

#ifdef CONFIG_DAWN_PROTO_UDP
  test_proto_udp,
#endif

#ifdef CONFIG_DAWN_PROTO_IPC
  test_proto_ipc,
#endif

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_DUMMY
  test_proto_nxscope_dummy,
#endif

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SERIAL
  test_proto_nxscope_serial,
#endif

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_UDP
  test_proto_nxscope_udp,
#endif

#ifdef CONFIG_DAWN_PROTO_NIMBLE
  test_proto_nimbleprph,
#endif

#ifdef CONFIG_DAWN_PROTO_CAN
  test_proto_can,
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_RTU
  test_proto_modbus_rtu,
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_TCP
  test_proto_modbus_tcp,
#endif

  nullptr,
};

extern "C"
{
  int test_run_proto()
  {
    int ret = 0;
    int i = 0;
    bool fail = false;

    DAWN_TEST_SEPARATOR();

    for (i = 0; test_array[i] != nullptr; i += 1)
      {
        ret = test_array[i]();
        if (ret != 0)
          {
            fail = true;
          }
#ifdef CONFIG_DAWN_TEST_EXIT_ON_FAIL
        if (ret != 0)
          {
            printf("Force exit on the first fail!\n");
            goto errout;
          }
#endif
      }

#ifdef CONFIG_DAWN_TEST_EXIT_ON_FAIL
  errout:
#endif
    return fail ? -1 : 0;
  }
}
