// dawn/tests/proto/test_shell.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <cstring>
#include <fcntl.h>
#include <pty.h>
#include <termios.h>
#include <unistd.h>

#include "dawn/io/dummy.hxx"
#include "dawn/proto/shell/pretty.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto SHELL_PIPE_PATH = "/dev/pipe0";

static constexpr auto SHELL_DUMMYIO1 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto SHELL_DUMMYIO2 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 2);
static constexpr auto SHELL_DUMMYIO3 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 3);
static constexpr auto SHELL_DUMMYIO_UNBOUND = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 1);

static int g_pty_fd;

static uint32_t g_cfg_dummy1[] = {
  SHELL_DUMMYIO1,
  2,
  CIODummy::cfgIdDim(),
  10,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_INT32, true, 10),
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
};

static uint32_t g_cfg_dummy2[] = {
  SHELL_DUMMYIO2,
  2,
  CIODummy::cfgIdDim(),
  10,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_INT32, true, 10),
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
};

static uint32_t g_cfg_dummy3[] = {
  SHELL_DUMMYIO3,
  2,
  CIODummy::cfgIdDim(),
  10,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_INT32, true, 10),
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
};

//***************************************************************************
// Description: shell descriptor binding three IOs plus one unbound id.
//***************************************************************************

static uint32_t g_bin_shell_pretty[] = {
  CProtoShellPretty::objectId(0),
  3,

  CProtoShellPretty::cfgIdIOBind(4),
  SHELL_DUMMYIO1,
  SHELL_DUMMYIO_UNBOUND,
  SHELL_DUMMYIO2,
  SHELL_DUMMYIO3,

  // Path: /dev/ttyp0

  CProtoShellPretty::cfgIdPath(3),
  0x7665642f,
  0x7974742f,
  0x00003070,

  // Prompt: abcde>

  CProtoShellPretty::cfgIdPrompt(2),
  0x64636261,
  0x00203e65,
};

//***************************************************************************
// PTY helpers — each test reopens /dev/pty0 to get a fresh PTY pair.
//***************************************************************************

static void open_test_pty()
{
  int ret;

  g_pty_fd = open("/dev/pty0", O_RDWR);
  TEST_ASSERT(g_pty_fd > 0);

  ret = unlockpt(g_pty_fd);
  TEST_ASSERT_EQUAL(0, ret);

  dawn_test_drain_pty_master(g_pty_fd);
}

static void close_test_pty()
{
  if (g_pty_fd >= 0)
    {
      dawn_test_drain_pty_master(g_pty_fd);
      close(g_pty_fd);
      g_pty_fd = -1;
    }
}

// Configure + init the three dummy IOs, configure the shell proto, bind the
// IOs, init + start.

static void shell_setup(CIODummy &d1, CIODummy &d2, CIODummy &d3, CProtoShellPretty &shell)
{
  TEST_ASSERT_EQUAL(OK, shell.configure());
  TEST_ASSERT_EQUAL(OK, d1.configure());
  TEST_ASSERT_EQUAL(OK, d1.init());
  TEST_ASSERT_EQUAL(OK, d2.configure());
  TEST_ASSERT_EQUAL(OK, d2.init());
  TEST_ASSERT_EQUAL(OK, d3.configure());
  TEST_ASSERT_EQUAL(OK, d3.init());
  shell.setObjectMapItem(SHELL_DUMMYIO1, &d1);
  shell.setObjectMapItem(SHELL_DUMMYIO2, &d2);
  shell.setObjectMapItem(SHELL_DUMMYIO3, &d3);
  TEST_ASSERT_EQUAL(OK, shell.init());
  TEST_ASSERT_EQUAL(OK, shell.start());
}

// Send a shell command terminated with "\n\r", wait, then read the
// response.  buffer is zeroed before read.

static void shell_send_recv(const char *cmd, char *buffer, size_t bufsize)
{
  ssize_t ret;

  usleep(100);
  ret = write(g_pty_fd, cmd, std::strlen(cmd));
  TEST_ASSERT(ret > 0);

  usleep(100);
  std::memset(buffer, 0, bufsize);
  ret = read(g_pty_fd, buffer, bufsize);
  TEST_ASSERT(ret > 0);
}

// Send "exit\n\r" so the shell read-loop returns from its blocking read on
// /dev/ttyp0, then call shell.stop() (which joins the thread).  Without
// the exit, stop() deadlocks.

static void shell_exit_and_stop(CProtoShellPretty &shell)
{
  ssize_t ret;

  usleep(100);
  ret = write(g_pty_fd, "exit\n\r", 6);
  TEST_ASSERT(ret > 0);

  usleep(1000);
  TEST_ASSERT_EQUAL(OK, shell.stop());
}

//***************************************************************************
// Description: shell proto runs through start -> hasThread -> stop.
//***************************************************************************

static void test_proto_shell_pretty_lifecycle()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_shell_pretty);
  CProtoShellPretty shell(desc);

  TEST_ASSERT_EQUAL(false, shell.hasThread());
  shell_setup(dummy1, dummy2, dummy3, shell);
  TEST_ASSERT_EQUAL(true, shell.hasThread());

  shell_exit_and_stop(shell);
  TEST_ASSERT_EQUAL(false, shell.hasThread());
}

//***************************************************************************
// Description: the help command produces output.
//***************************************************************************

static void test_proto_shell_pretty_help()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_shell_pretty);
  CProtoShellPretty shell(desc);
  char buffer[1024];

  shell_setup(dummy1, dummy2, dummy3, shell);

  shell_send_recv("\n\r", buffer, sizeof(buffer));
  shell_send_recv("help\n\r", buffer, sizeof(buffer));

  shell_exit_and_stop(shell);
}

//***************************************************************************
// Description: the info command prints a compact shell binding inventory and
// marks unbound ids as "unbound".
//***************************************************************************

static void test_proto_shell_pretty_info_lists_unbound()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_shell_pretty);
  CProtoShellPretty shell(desc);
  char buffer[1024];

  shell_setup(dummy1, dummy2, dummy3, shell);

  shell_send_recv("\n\r", buffer, sizeof(buffer));
  shell_send_recv("info\n\r", buffer, sizeof(buffer));

  TEST_ASSERT(std::strstr(buffer, "INFO: shell IO bindings") != nullptr);
  TEST_ASSERT(std::strstr(buffer, "0x40a60001") != nullptr);
  TEST_ASSERT(std::strstr(buffer, "unbound") != nullptr);

  shell_exit_and_stop(shell);
}

//***************************************************************************
// Description: getio dumps the contents of the requested IO.
//***************************************************************************

static void test_proto_shell_pretty_getio()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_shell_pretty);
  CProtoShellPretty shell(desc);
  char buffer[1024];

  shell_setup(dummy1, dummy2, dummy3, shell);

  shell_send_recv("\n\r", buffer, sizeof(buffer));

  shell_send_recv("getio 0x40a60000\n\r", buffer, sizeof(buffer));
  TEST_ASSERT(std::strstr(buffer, "0 1 2 3 4 5 6 7 8 9 ") != nullptr);

  shell_send_recv("getio 0x40a60002\n\r", buffer, sizeof(buffer));
  TEST_ASSERT(std::strstr(buffer, "0 1 2 3 4 5 6 7 8 9 ") != nullptr);

  shell_send_recv("getio 0x40a60003\n\r", buffer, sizeof(buffer));
  TEST_ASSERT(std::strstr(buffer, "0 1 2 3 4 5 6 7 8 9 ") != nullptr);

  shell_exit_and_stop(shell);
}

// Reopen the master PTY around every test so each one starts with a clean
// /dev/pty0 ↔ /dev/ttyp0 pair.  Using a single long-lived PTY across tests
// causes NuttX to accumulate state between shell instances.

#define SHELL_RUN_TEST(fn) \
  do                       \
    {                      \
      open_test_pty();     \
      DAWN_RUN_TEST(fn);   \
      close_test_pty();    \
    }                      \
  while (0)

extern "C"
{
  int test_proto_shell()
  {
    UNITY_BEGIN();

    SHELL_RUN_TEST(test_proto_shell_pretty_lifecycle);
    SHELL_RUN_TEST(test_proto_shell_pretty_help);
    SHELL_RUN_TEST(test_proto_shell_pretty_info_lists_unbound);
    SHELL_RUN_TEST(test_proto_shell_pretty_getio);

    return UNITY_END();
  }
}
