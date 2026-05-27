// dawn/tests/proto/test_modbus_rtu.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <fcntl.h>
#include <unistd.h>

#include "dawn/io/dummy.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/proto/modbus/rtu.hxx"
#include "mocks/fake_ioseekmock.hxx"
#include "mocks/fake_iowriteonlymock.hxx"
#include "test_common.hxx"

extern "C"
{
  uint16_t nxmb_crc16(const uint8_t *buf, uint16_t len);
}

using namespace dawn;

// Modbus functions

static constexpr auto MB_COIL_GET = 0x1;
static constexpr auto MB_DISCRETE_GET = 0x2;
static constexpr auto MB_HOLDING_GET = 0x3;
static constexpr auto MB_INPUT_GET = 0x4;
static constexpr auto MB_COIL_SET = 0x5;
static constexpr auto MB_HOLDING_SET = 0x6;
static constexpr auto MB_HOLDING_MUL_SET = 0x10;
static constexpr auto MODBUS_SEEKABLE_WINDOW_REGS = 8;

// IOs

static constexpr auto MODBUS_DUMMYIO1 = CIODummy::objectId(SObjectId::DTYPE_BOOL, false, 0);
static constexpr auto MODBUS_DUMMYIO2 = CIODummy::objectId(SObjectId::DTYPE_BOOL, false, 2);
static constexpr auto MODBUS_DUMMYIO3 = CIODummy::objectId(SObjectId::DTYPE_BOOL, false, 3);

static constexpr auto MODBUS_DUMMYIO4 = CIODummy::objectId(SObjectId::DTYPE_UINT16, false, 4);
static constexpr auto MODBUS_DUMMYIO5 = CIODummy::objectId(SObjectId::DTYPE_UINT16, false, 5);
static constexpr auto MODBUS_DUMMYIO6 = CIODummy::objectId(SObjectId::DTYPE_UINT16, false, 6);

static constexpr auto MODBUS_DUMMYIO7 = CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 7);
static constexpr auto MODBUS_DUMMYIO8 = CIODummy::objectId(SObjectId::DTYPE_UINT64, false, 8);
static constexpr auto MODBUS_WRONLYIO1 = CIOWriteOnlyScalarMock::objectId(SObjectId::DTYPE_BOOL, 9);
static constexpr auto MODBUS_WRONLYIO4 =
  CIOWriteOnlyScalarMock::objectId(SObjectId::DTYPE_UINT16, 10);

// Regs definitions

static constexpr auto REGS_COILS1 = 0x01;
static constexpr auto REGS_COILS2 = 0x100;
static constexpr auto REGS_INPUTS1 = 0x1000;
static constexpr auto REGS_INPUTS2 = 0x2000;
static constexpr auto REGS_DISCRETE1 = 0x500;
static constexpr auto REGS_DISCRETE2 = 0x600;
static constexpr auto REGS_HOLDING1 = 0x3000;
static constexpr auto REGS_HOLDING2 = 0x4000;

// Pty0 used to communicate with programs

static int g_pty_fd;

static void open_test_pty()
{
  int ret;

  g_pty_fd = open("/dev/pty0", O_RDWR);
  TEST_ASSERT(g_pty_fd > 0);

  ret = unlockpt(g_pty_fd);
  TEST_ASSERT(ret == 0);
}

static void close_test_pty()
{
  if (g_pty_fd >= 0)
    {
      close(g_pty_fd);
      g_pty_fd = -1;
    }
}

//***************************************************************************
// Description: dummy IOs
//***************************************************************************

static uint32_t g_cfg_dummy1[] = {
  MODBUS_DUMMYIO1,
  0,
};

static uint32_t g_cfg_dummy2[] = {
  MODBUS_DUMMYIO2,
  0,
};

static uint32_t g_cfg_dummy3[] = {
  MODBUS_DUMMYIO3,
  0,
};

static uint32_t g_cfg_dummy4[] = {
  MODBUS_DUMMYIO4,
  0,
};

static uint32_t g_cfg_dummy5[] = {
  MODBUS_DUMMYIO5,
  0,
};

static uint32_t g_cfg_dummy6[] = {
  MODBUS_DUMMYIO6,
  0,
};

static uint32_t g_cfg_dummy7[] = {
  MODBUS_DUMMYIO7,
  0,
};

static uint32_t g_cfg_wronly1[] = {
  MODBUS_WRONLYIO1,
  0,
};

static uint32_t g_cfg_wronly4[] = {
  MODBUS_WRONLYIO4,
  0,
};

// Static uint32_t g_cfg_dummy8[] =
// {
//   MODBUS_DUMMYIO8, 0,
// };

//***************************************************************************
// Description: single coil register with blocking IO
//***************************************************************************

static uint32_t g_bin_modbus_coil_blocking[] = {
  // Object ID

  CProtoModbusRtu::objectId(0),
  2,

  // Path

  CProtoModbusRtu::cfgIdPath(3),
  0x7665642f, // Path: /dev/ttyp0
  0x7974742f, //
  0x00003070, //

  // Allocated objects

  CProtoModbusRtu::cfgIdIOBind(5),
  CProtoModbusRegs::MODBUS_TYPE_COIL,
  0,
  REGS_COILS1,
  1,
  MODBUS_DUMMYIO1,
};

static uint32_t g_bin_modbus_coil_writeonly[] = {
  CProtoModbusRtu::objectId(0),
  2,

  CProtoModbusRtu::cfgIdPath(3),
  0x7665642f,
  0x7974742f,
  0x00003070,

  CProtoModbusRtu::cfgIdIOBind(5),
  CProtoModbusRegs::MODBUS_TYPE_COIL,
  0,
  REGS_COILS1,
  1,
  MODBUS_WRONLYIO1,
};

//***************************************************************************
// Description: many packed coil register with blocking IOs
//***************************************************************************

static uint32_t g_bin_modbus_coil_packed_blocking[] = {
  // Object ID

  CProtoModbusRtu::objectId(0),
  2,

  // Path

  CProtoModbusRtu::cfgIdPath(3),
  0x7665642f, // Path: /dev/ttyp0
  0x7974742f, //
  0x00003070, //

  // Allocated objects

  CProtoModbusRtu::cfgIdIOBind(26),
  CProtoModbusRegs::MODBUS_TYPE_COIL_PACKED,
  0,
  REGS_COILS1,
  9,
  MODBUS_DUMMYIO1,
  MODBUS_DUMMYIO2,
  MODBUS_DUMMYIO3,
  MODBUS_DUMMYIO1,
  MODBUS_DUMMYIO2,
  MODBUS_DUMMYIO3,
  MODBUS_DUMMYIO1,
  MODBUS_DUMMYIO2,
  MODBUS_DUMMYIO3,

  CProtoModbusRegs::MODBUS_TYPE_COIL_PACKED,
  0,
  REGS_COILS2,
  9,
  MODBUS_DUMMYIO3, // Swaped order
  MODBUS_DUMMYIO2,
  MODBUS_DUMMYIO1,
  MODBUS_DUMMYIO3,
  MODBUS_DUMMYIO2,
  MODBUS_DUMMYIO1,
  MODBUS_DUMMYIO3,
  MODBUS_DUMMYIO2,
  MODBUS_DUMMYIO1,
};

//***************************************************************************
// Description: single discrete register with blocking IO
//***************************************************************************

static uint32_t g_bin_modbus_discrete_blocking[] = {
  // Object ID

  CProtoModbusRtu::objectId(0),
  2,

  // Path

  CProtoModbusRtu::cfgIdPath(3),
  0x7665642f, // Path: /dev/ttyp0
  0x7974742f, //
  0x00003070, //

  // Allocated objects

  CProtoModbusRtu::cfgIdIOBind(5),
  CProtoModbusRegs::MODBUS_TYPE_DISCRETE,
  0,
  REGS_DISCRETE1,
  1,
  MODBUS_DUMMYIO1,
};

//***************************************************************************
// Description: many packed discrete register with blocking IOs
//***************************************************************************

static uint32_t g_bin_modbus_discrete_packed_blocking[] = {
  // Object ID

  CProtoModbusRtu::objectId(0),
  2,

  // Path

  CProtoModbusRtu::cfgIdPath(3),
  0x7665642f, // Path: /dev/ttyp0
  0x7974742f, //
  0x00003070, //

  // Allocated objects

  CProtoModbusRtu::cfgIdIOBind(26),
  CProtoModbusRegs::MODBUS_TYPE_DISCRETE_PACKED,
  0,
  REGS_DISCRETE1,
  9,
  MODBUS_DUMMYIO1,
  MODBUS_DUMMYIO2,
  MODBUS_DUMMYIO3,
  MODBUS_DUMMYIO1,
  MODBUS_DUMMYIO2,
  MODBUS_DUMMYIO3,
  MODBUS_DUMMYIO1,
  MODBUS_DUMMYIO2,
  MODBUS_DUMMYIO3,

  CProtoModbusRegs::MODBUS_TYPE_DISCRETE_PACKED,
  0,
  REGS_DISCRETE2,
  9,
  MODBUS_DUMMYIO3, // Swaped order
  MODBUS_DUMMYIO2,
  MODBUS_DUMMYIO1,
  MODBUS_DUMMYIO3,
  MODBUS_DUMMYIO2,
  MODBUS_DUMMYIO1,
  MODBUS_DUMMYIO3,
  MODBUS_DUMMYIO2,
  MODBUS_DUMMYIO1,
};

//***************************************************************************
// Description: single input register with blocking IO, data is 2B long
//***************************************************************************

static uint32_t g_bin_modbus_input_blocking[] = {
  // Object ID

  CProtoModbusRtu::objectId(0),
  2,

  // Path

  CProtoModbusRtu::cfgIdPath(3),
  0x7665642f, // Path: /dev/ttyp0
  0x7974742f, //
  0x00003070, //

  // Allocated objects

  CProtoModbusRtu::cfgIdIOBind(5),
  CProtoModbusRegs::MODBUS_TYPE_INPUT,
  0,
  REGS_INPUTS1,
  1,
  MODBUS_DUMMYIO4,
};

//***************************************************************************
// Description: many input register with blocking IOs, data is 2B
//***************************************************************************

uint32_t g_bin_modbus_input_many_blocking[] = {
  // Object ID

  CProtoModbusRtu::objectId(0),
  2,

  // Path

  CProtoModbusRtu::cfgIdPath(3),
  0x7665642f, // Path: /dev/ttyp0
  0x7974742f, //
  0x00003070, //

  // Allocated objects

  CProtoModbusRtu::cfgIdIOBind(14),
  CProtoModbusRegs::MODBUS_TYPE_INPUT,
  0,
  REGS_INPUTS1,
  3,
  MODBUS_DUMMYIO4,
  MODBUS_DUMMYIO5,
  MODBUS_DUMMYIO6,

  CProtoModbusRegs::MODBUS_TYPE_INPUT,
  0,
  REGS_INPUTS2,
  3,
  MODBUS_DUMMYIO6, // Swaped order
  MODBUS_DUMMYIO5,
  MODBUS_DUMMYIO4,
};

//***************************************************************************
// Description: single holding register with blocking IO, data is 2B long
//***************************************************************************

uint32_t g_bin_modbus_holding_blocking[] = {
  // Object ID

  CProtoModbusRtu::objectId(0),
  2,

  // Path

  CProtoModbusRtu::cfgIdPath(3),
  0x7665642f, // Path: /dev/ttyp0
  0x7974742f, //
  0x00003070, //

  // Allocated objects

  CProtoModbusRtu::cfgIdIOBind(5),
  CProtoModbusRegs::MODBUS_TYPE_HOLDING,
  0,
  REGS_HOLDING1,
  1,
  MODBUS_DUMMYIO4,
};

uint32_t g_bin_modbus_holding_writeonly[] = {
  CProtoModbusRtu::objectId(0),
  2,

  CProtoModbusRtu::cfgIdPath(3),
  0x7665642f,
  0x7974742f,
  0x00003070,

  CProtoModbusRtu::cfgIdIOBind(5),
  CProtoModbusRegs::MODBUS_TYPE_HOLDING,
  0,
  REGS_HOLDING1,
  1,
  MODBUS_WRONLYIO4,
};

//***************************************************************************
// Description: single holding register with seekable IO
//***************************************************************************

uint32_t g_bin_modbus_holding_seekable[] = {
  // Object ID

  CProtoModbusRtu::objectId(0),
  2,

  // Path

  CProtoModbusRtu::cfgIdPath(3),
  0x7665642f, // Path: /dev/ttyp0
  0x7974742f, //
  0x00003070, //

  // Allocated objects

  CProtoModbusRtu::cfgIdIOBind(5),
  CProtoModbusRegs::MODBUS_TYPE_SEEKABLE,
  MODBUS_SEEKABLE_WINDOW_REGS,
  REGS_HOLDING1,
  1,
  MODBUS_DUMMYIO4,
};

//***************************************************************************
// Description: many holding register with blocking IOs, data is 2B long
//***************************************************************************

uint32_t g_bin_modbus_holding_many_blocking[] = {
  // Object ID

  CProtoModbusRtu::objectId(0),
  2,

  // Path

  CProtoModbusRtu::cfgIdPath(3),
  0x7665642f, // Path: /dev/ttyp0
  0x7974742f, //
  0x00003070, //

  // Allocated objects

  CProtoModbusRtu::cfgIdIOBind(14),
  CProtoModbusRegs::MODBUS_TYPE_HOLDING,
  0,
  REGS_HOLDING1,
  3,
  MODBUS_DUMMYIO4,
  MODBUS_DUMMYIO5,
  MODBUS_DUMMYIO6,

  CProtoModbusRegs::MODBUS_TYPE_HOLDING,
  0,
  REGS_HOLDING2,
  3,
  MODBUS_DUMMYIO6, // Swaped order
  MODBUS_DUMMYIO5,
  MODBUS_DUMMYIO4,
};

//***************************************************************************
// Description: single input register with blocking IO, IO data is longer than
// 2B
//***************************************************************************

uint32_t g_bin_modbus_input_longdata_blocking[] = {
  // Object ID

  CProtoModbusRtu::objectId(0),
  2,

  // Path

  CProtoModbusRtu::cfgIdPath(3),
  0x7665642f, // Path: /dev/ttyp0
  0x7974742f, //
  0x00003070, //

  // Allocated objects

  CProtoModbusRtu::cfgIdIOBind(5),
  CProtoModbusRegs::MODBUS_TYPE_INPUT,
  0,
  REGS_INPUTS1,
  1,
  MODBUS_DUMMYIO7,
};

//***************************************************************************
// Description: single 2B holding register with blocking IO, IO data is longer
// than 2B
//***************************************************************************

uint32_t g_bin_modbus_holding_longdata_blocking[] = {
  // Object ID

  CProtoModbusRtu::objectId(0),
  2,

  // Path

  CProtoModbusRtu::cfgIdPath(3),
  0x7665642f, // Path: /dev/ttyp0
  0x7974742f, //
  0x00003070, //

  // Allocated objects

  CProtoModbusRtu::cfgIdIOBind(5),
  CProtoModbusRegs::MODBUS_TYPE_HOLDING,
  0,
  REGS_HOLDING1,
  1,
  MODBUS_DUMMYIO7,
};

//***************************************************************************
// Description: modbus rtu with many blocking IO
//***************************************************************************

uint32_t g_bin_modbus_many_blocking[] = {
  // Object ID

  CProtoModbusRtu::objectId(0),
  2,

  // Path

  CProtoModbusRtu::cfgIdPath(3),
  0x7665642f, // Path: /dev/ttyp0
  0x7974742f, //
  0x00003070, //

  // Allocated objects

  CProtoModbusRtu::cfgIdIOBind(18),
  CProtoModbusRegs::MODBUS_TYPE_DISCRETE,
  0,
  0x01,
  3,
  MODBUS_DUMMYIO1,
  MODBUS_DUMMYIO2,
  MODBUS_DUMMYIO3,

  CProtoModbusRegs::MODBUS_TYPE_INPUT,
  0,
  0x4000,
  2,
  MODBUS_DUMMYIO7,
  MODBUS_DUMMYIO8,

  CProtoModbusRegs::MODBUS_TYPE_HOLDING,
  0,
  0x5000,
  1,
  MODBUS_DUMMYIO6,
};

static int modbus_frame_send(uint8_t func, uint16_t addr, uint16_t n)
{
  uint16_t crc;
  uint8_t buffer[255];

  buffer[0] = CONFIG_DAWN_PROTO_MODBUS_RTU_ADDR;
  buffer[1] = func;
  buffer[2] = addr >> 8;
  buffer[3] = addr & 0xff;
  buffer[4] = n >> 8;
  buffer[5] = n & 0xff;

  crc = nxmb_crc16(buffer, 6);

  buffer[6] = crc & 0xff;
  buffer[7] = crc >> 8;

  return write(g_pty_fd, buffer, 8);
}

static int modbus_frame_send_many(uint8_t func,
                                  uint16_t addr,
                                  uint16_t n,
                                  const uint8_t *data,
                                  uint8_t len)
{
  uint16_t crc;
  uint8_t buffer[255];

  buffer[0] = CONFIG_DAWN_PROTO_MODBUS_RTU_ADDR;
  buffer[1] = func;
  buffer[2] = addr >> 8;
  buffer[3] = addr & 0xff;
  buffer[4] = n >> 8;
  buffer[5] = n & 0xff;
  buffer[6] = len;

  for (int i = 0; i < len; i++)
    {
      buffer[7 + i] = data[i];
    }

  crc = nxmb_crc16(buffer, 7 + len);

  buffer[7 + len] = crc & 0xff;
  buffer[7 + len + 1] = crc >> 8;

  return write(g_pty_fd, buffer, 7 + len + 2);
}

// Configure + init the dummy IO, configure + bind + init + start the modbus
// proto.  An initial sleep gives the slave thread time to enter its receive
// loop before the test sends a frame.  Caller stops the modbus.

static void modbus_setup_single_io(CIOCommon &io, uint32_t io_id, CProtoModbusRtu &modbus)
{
  TEST_ASSERT_EQUAL(OK, modbus.configure());
  TEST_ASSERT_EQUAL(OK, io.configure());
  TEST_ASSERT_EQUAL(OK, io.init());
  modbus.setObjectMapItem(io_id, &io);
  TEST_ASSERT_EQUAL(OK, modbus.init());
  TEST_ASSERT_EQUAL(OK, modbus.start());
  usleep(100000);
}

static void modbus_setup_single(CIODummy &dummy, uint32_t io_id, CProtoModbusRtu &modbus)
{
  modbus_setup_single_io(dummy, io_id, modbus);
}

// Configure + init three CIODummy IOs, bind them to the proto by id, then
// init + start the proto.  Caller stops the modbus.

static void modbus_setup_three(CIODummy &d1,
                               uint32_t id1,
                               CIODummy &d2,
                               uint32_t id2,
                               CIODummy &d3,
                               uint32_t id3,
                               CProtoModbusRtu &modbus)
{
  TEST_ASSERT_EQUAL(OK, modbus.configure());
  TEST_ASSERT_EQUAL(OK, d1.configure());
  TEST_ASSERT_EQUAL(OK, d1.init());
  TEST_ASSERT_EQUAL(OK, d2.configure());
  TEST_ASSERT_EQUAL(OK, d2.init());
  TEST_ASSERT_EQUAL(OK, d3.configure());
  TEST_ASSERT_EQUAL(OK, d3.init());
  modbus.setObjectMapItem(id1, &d1);
  modbus.setObjectMapItem(id2, &d2);
  modbus.setObjectMapItem(id3, &d3);
  TEST_ASSERT_EQUAL(OK, modbus.init());
  TEST_ASSERT_EQUAL(OK, modbus.start());
  usleep(100000);
}

// Send a single-register modbus frame and read the slave's reply, asserting
// the reply length is exactly expected_len bytes.

static void modbus_send_and_read(uint8_t func,
                                 uint16_t addr,
                                 uint16_t count,
                                 uint8_t *buffer,
                                 size_t bufsize,
                                 int expected_len)
{
  int ret;

  usleep(1000);
  ret = modbus_frame_send(func, addr, count);
  TEST_ASSERT_EQUAL(8, ret);

  usleep(1000);
  ret = (int)read(g_pty_fd, buffer, bufsize);
  TEST_ASSERT_EQUAL(expected_len, ret);
}

// Assert a Modbus exception reply: 5 bytes total, header [SLAVE][FN|0x80]
// [EXCEPTION].

static void modbus_assert_exception(const uint8_t *buffer, uint8_t func, uint8_t exception)
{
  TEST_ASSERT_EQUAL(CONFIG_DAWN_PROTO_MODBUS_RTU_ADDR, buffer[0]);
  TEST_ASSERT_EQUAL(0x80 + func, buffer[1]);
  TEST_ASSERT_EQUAL(exception, buffer[2]);
}

//***************************************************************************
// Description: coil-only proto rejects a discrete-register read with
// exception 0x02 (illegal data address).
//***************************************************************************

static void test_proto_modbus_coil_read_wrong_function()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_modbus_coil_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single(dummy1, MODBUS_DUMMYIO1, modbus);

  modbus_send_and_read(MB_DISCRETE_GET, 0, 1, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_DISCRETE_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading a coil count past the bound range returns
// exception 0x02.
//***************************************************************************

static void test_proto_modbus_coil_read_out_of_range()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_modbus_coil_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single(dummy1, MODBUS_DUMMYIO1, modbus);

  modbus_send_and_read(MB_COIL_GET, REGS_COILS1, 2, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_COIL_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading a single coil returns the IO's initial value (0) and
// the value seen on the bus matches the dummy's data.
//***************************************************************************

static void test_proto_modbus_coil_read_initial()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_modbus_coil_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<bool, 1> data;

  modbus_setup_single(dummy1, MODBUS_DUMMYIO1, modbus);

  modbus_send_and_read(MB_COIL_GET, REGS_COILS1, 1, buffer, sizeof(buffer), 6);
  TEST_ASSERT_EQUAL(CONFIG_DAWN_PROTO_MODBUS_RTU_ADDR, buffer[0]);
  TEST_ASSERT_EQUAL(MB_COIL_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x01, buffer[2]); // Byte count
  TEST_ASSERT_EQUAL(0x00, buffer[3]); // Coil status

  TEST_ASSERT_EQUAL(OK, dummy1.getData(data, 1));
  TEST_ASSERT_EQUAL(data(0), buffer[3]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: writing a coil with 0xff00 echoes the request and a
// subsequent read sees the new value (1).
//***************************************************************************

static void test_proto_modbus_coil_write_then_read()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_modbus_coil_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<bool, 1> data;

  modbus_setup_single(dummy1, MODBUS_DUMMYIO1, modbus);

  // Write coil ON.

  modbus_send_and_read(MB_COIL_SET, REGS_COILS1, 0xff00, buffer, sizeof(buffer), 8);
  TEST_ASSERT_EQUAL(CONFIG_DAWN_PROTO_MODBUS_RTU_ADDR, buffer[0]);
  TEST_ASSERT_EQUAL(MB_COIL_SET, buffer[1]);
  TEST_ASSERT_EQUAL((REGS_COILS1) >> 8, buffer[2]);
  TEST_ASSERT_EQUAL((REGS_COILS1) & 0xff, buffer[3]);
  TEST_ASSERT_EQUAL(0xff, buffer[4]);
  TEST_ASSERT_EQUAL(0x00, buffer[5]);

  // Read it back; should now be 1.

  modbus_send_and_read(MB_COIL_GET, REGS_COILS1, 1, buffer, sizeof(buffer), 6);
  TEST_ASSERT_EQUAL(MB_COIL_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x01, buffer[2]); // Byte count
  TEST_ASSERT_EQUAL(0x01, buffer[3]); // Coil status

  TEST_ASSERT_EQUAL(OK, dummy1.getData(data, 1));
  TEST_ASSERT_EQUAL(data(0), buffer[3]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: write-only coil IO initializes, accepts writes, and reads
// back the cached last written bit value.
//***************************************************************************

static void test_proto_modbus_coil_writeonly_shadow_readback()
{
  CDescObject descv1(g_cfg_wronly1);
  CIOWriteOnlyScalarMock wronly1(descv1);
  CDescObject desc(g_bin_modbus_coil_writeonly);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single_io(wronly1, MODBUS_WRONLYIO1, modbus);

  modbus_send_and_read(MB_COIL_GET, REGS_COILS1, 1, buffer, sizeof(buffer), 6);
  TEST_ASSERT_EQUAL(MB_COIL_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x00, buffer[3]);

  modbus_send_and_read(MB_COIL_SET, REGS_COILS1, 0xff00, buffer, sizeof(buffer), 8);
  TEST_ASSERT_EQUAL(MB_COIL_SET, buffer[1]);
  TEST_ASSERT_EQUAL(0xff, buffer[4]);
  TEST_ASSERT_EQUAL(0x00, buffer[5]);
  TEST_ASSERT_EQUAL(1u, wronly1.getLastValue());

  modbus_send_and_read(MB_COIL_GET, REGS_COILS1, 1, buffer, sizeof(buffer), 6);
  TEST_ASSERT_EQUAL(MB_COIL_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x01, buffer[3]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: modbus rtu proto runs through start -> hasThread -> stop and
// reports the right hasThread() state on each side.
//***************************************************************************

static void test_proto_modbus_coil_lifecycle()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_modbus_coil_blocking);
  CProtoModbusRtu modbus(desc);

  TEST_ASSERT_EQUAL(OK, modbus.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());
  modbus.setObjectMapItem(MODBUS_DUMMYIO1, &dummy1);

  TEST_ASSERT_EQUAL(false, modbus.hasThread());
  TEST_ASSERT_EQUAL(OK, modbus.init());
  TEST_ASSERT_EQUAL(OK, modbus.start());
  TEST_ASSERT_EQUAL(true, modbus.hasThread());
  TEST_ASSERT_EQUAL(OK, modbus.stop());
  TEST_ASSERT_EQUAL(false, modbus.hasThread());
}

//***************************************************************************
// Description: discrete-only proto rejects a coil-register read with
// exception 0x02.
//***************************************************************************

static void test_proto_modbus_discrete_read_wrong_function()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_modbus_discrete_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single(dummy1, MODBUS_DUMMYIO1, modbus);

  modbus_send_and_read(MB_COIL_GET, 0, 1, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_COIL_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading a discrete count past the bound range returns
// exception 0x02.
//***************************************************************************

static void test_proto_modbus_discrete_read_out_of_range()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_modbus_discrete_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single(dummy1, MODBUS_DUMMYIO1, modbus);

  modbus_send_and_read(MB_DISCRETE_GET, REGS_DISCRETE1, 2, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_DISCRETE_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading a single discrete returns the IO's initial value (0).
//***************************************************************************

static void test_proto_modbus_discrete_read_initial()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_modbus_discrete_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<bool, 1> data;

  modbus_setup_single(dummy1, MODBUS_DUMMYIO1, modbus);

  modbus_send_and_read(MB_DISCRETE_GET, REGS_DISCRETE1, 1, buffer, sizeof(buffer), 6);
  TEST_ASSERT_EQUAL(CONFIG_DAWN_PROTO_MODBUS_RTU_ADDR, buffer[0]);
  TEST_ASSERT_EQUAL(MB_DISCRETE_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x01, buffer[2]); // Byte count
  TEST_ASSERT_EQUAL(0x00, buffer[3]); // Discrete status

  TEST_ASSERT_EQUAL(OK, dummy1.getData(data, 1));
  TEST_ASSERT_EQUAL(data(0), buffer[3]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: when the bound IO is updated, the next discrete read
// reports the new value.
//***************************************************************************

static void test_proto_modbus_discrete_read_after_update()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_modbus_discrete_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<bool, 1> data;

  modbus_setup_single(dummy1, MODBUS_DUMMYIO1, modbus);

  data(0) = 1;
  TEST_ASSERT_EQUAL(OK, dummy1.setData(data));

  modbus_send_and_read(MB_DISCRETE_GET, REGS_DISCRETE1, 1, buffer, sizeof(buffer), 6);
  TEST_ASSERT_EQUAL(0x02, buffer[1]); // Read discrete
  TEST_ASSERT_EQUAL(0x01, buffer[2]); // Byte count
  TEST_ASSERT_EQUAL(data(0), buffer[3]);

  TEST_ASSERT_EQUAL(OK, dummy1.getData(data, 1));
  TEST_ASSERT_EQUAL(data(0), buffer[3]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: input-only proto rejects a coil-register read with
// exception 0x02.
//***************************************************************************

static void test_proto_modbus_input_read_wrong_function()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject desc(g_bin_modbus_input_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single(dummy4, MODBUS_DUMMYIO4, modbus);

  modbus_send_and_read(MB_COIL_GET, 0, 1, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_COIL_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading an input count past the bound range returns
// exception 0x02.
//***************************************************************************

static void test_proto_modbus_input_read_out_of_range()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject desc(g_bin_modbus_input_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single(dummy4, MODBUS_DUMMYIO4, modbus);

  modbus_send_and_read(MB_INPUT_GET, REGS_INPUTS1, 2, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_INPUT_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading a single input register returns the IO's initial
// value (0) as a 2-byte big-endian payload.
//***************************************************************************

static void test_proto_modbus_input_read_initial()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject desc(g_bin_modbus_input_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<uint16_t, 1> data;

  modbus_setup_single(dummy4, MODBUS_DUMMYIO4, modbus);

  modbus_send_and_read(MB_INPUT_GET, REGS_INPUTS1, 1, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(CONFIG_DAWN_PROTO_MODBUS_RTU_ADDR, buffer[0]);
  TEST_ASSERT_EQUAL(MB_INPUT_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x02, buffer[2]); // Byte count
  TEST_ASSERT_EQUAL(0x00, buffer[3]);
  TEST_ASSERT_EQUAL(0x00, buffer[4]);

  TEST_ASSERT_EQUAL(OK, dummy4.getData(data, 1));
  TEST_ASSERT_EQUAL(data(0), (buffer[3] << 8) | buffer[4]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: when the bound IO is updated, the next input read reports
// the new big-endian value.
//***************************************************************************

static void test_proto_modbus_input_read_after_update()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject desc(g_bin_modbus_input_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<uint16_t, 1> data;

  modbus_setup_single(dummy4, MODBUS_DUMMYIO4, modbus);

  data(0) = 0xdead;
  TEST_ASSERT_EQUAL(OK, dummy4.setData(data));

  modbus_send_and_read(MB_INPUT_GET, REGS_INPUTS1, 1, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(MB_INPUT_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x02, buffer[2]); // Byte count
  TEST_ASSERT_EQUAL(data(0) >> 8, buffer[3]);
  TEST_ASSERT_EQUAL(data(0) & 0xff, buffer[4]);

  TEST_ASSERT_EQUAL(OK, dummy4.getData(data, 1));
  TEST_ASSERT_EQUAL(data(0), (buffer[3] << 8) | buffer[4]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: holding-only proto rejects a coil-register read with
// exception 0x02.
//***************************************************************************

static void test_proto_modbus_holding_read_wrong_function()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject desc(g_bin_modbus_holding_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single(dummy4, MODBUS_DUMMYIO4, modbus);

  modbus_send_and_read(MB_COIL_GET, 0, 1, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_COIL_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading a holding count past the bound range returns
// exception 0x02.
//***************************************************************************

static void test_proto_modbus_holding_read_out_of_range()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject desc(g_bin_modbus_holding_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single(dummy4, MODBUS_DUMMYIO4, modbus);

  modbus_send_and_read(MB_HOLDING_GET, REGS_HOLDING1, 2, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_HOLDING_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading a single holding register returns the IO's initial
// value (0).
//***************************************************************************

static void test_proto_modbus_holding_read_initial()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject desc(g_bin_modbus_holding_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<uint16_t, 1> data;

  modbus_setup_single(dummy4, MODBUS_DUMMYIO4, modbus);

  modbus_send_and_read(MB_HOLDING_GET, REGS_HOLDING1, 1, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(CONFIG_DAWN_PROTO_MODBUS_RTU_ADDR, buffer[0]);
  TEST_ASSERT_EQUAL(MB_HOLDING_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x02, buffer[2]); // Byte count
  TEST_ASSERT_EQUAL(0x00, buffer[3]);
  TEST_ASSERT_EQUAL(0x00, buffer[4]);

  TEST_ASSERT_EQUAL(OK, dummy4.getData(data, 1));
  TEST_ASSERT_EQUAL(data(0), (buffer[3] << 8) | buffer[4]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: writing a holding register echoes the request and a
// subsequent read sees the new value.
//***************************************************************************

static void test_proto_modbus_holding_write_then_read()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject desc(g_bin_modbus_holding_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<uint16_t, 1> data;

  modbus_setup_single(dummy4, MODBUS_DUMMYIO4, modbus);

  // Write 0xdead.

  modbus_send_and_read(MB_HOLDING_SET, REGS_HOLDING1, 0xdead, buffer, sizeof(buffer), 8);
  TEST_ASSERT_EQUAL(CONFIG_DAWN_PROTO_MODBUS_RTU_ADDR, buffer[0]);
  TEST_ASSERT_EQUAL(MB_HOLDING_SET, buffer[1]);
  TEST_ASSERT_EQUAL((REGS_HOLDING1) >> 8, buffer[2]);
  TEST_ASSERT_EQUAL((REGS_HOLDING1) & 0xff, buffer[3]);
  TEST_ASSERT_EQUAL(0xde, buffer[4]);
  TEST_ASSERT_EQUAL(0xad, buffer[5]);

  // Read it back.

  modbus_send_and_read(MB_HOLDING_GET, REGS_HOLDING1, 1, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(MB_HOLDING_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x02, buffer[2]); // Byte count
  TEST_ASSERT_EQUAL(0xde, buffer[3]);
  TEST_ASSERT_EQUAL(0xad, buffer[4]);

  TEST_ASSERT_EQUAL(OK, dummy4.getData(data, 1));
  TEST_ASSERT_EQUAL(data(0), (buffer[3] << 8) | buffer[4]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: write-only holding IO initializes, accepts writes, and reads
// back the cached last written value.
//***************************************************************************

static void test_proto_modbus_holding_writeonly_shadow_readback()
{
  CDescObject descv4(g_cfg_wronly4);
  CIOWriteOnlyScalarMock wronly4(descv4);
  CDescObject desc(g_bin_modbus_holding_writeonly);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single_io(wronly4, MODBUS_WRONLYIO4, modbus);

  modbus_send_and_read(MB_HOLDING_GET, REGS_HOLDING1, 1, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(MB_HOLDING_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x00, buffer[3]);
  TEST_ASSERT_EQUAL(0x00, buffer[4]);

  modbus_send_and_read(MB_HOLDING_SET, REGS_HOLDING1, 0xbeef, buffer, sizeof(buffer), 8);
  TEST_ASSERT_EQUAL(MB_HOLDING_SET, buffer[1]);
  TEST_ASSERT_EQUAL(0xbe, buffer[4]);
  TEST_ASSERT_EQUAL(0xef, buffer[5]);
  TEST_ASSERT_EQUAL(0xbeefu, wronly4.getLastValue());

  modbus_send_and_read(MB_HOLDING_GET, REGS_HOLDING1, 1, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(MB_HOLDING_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0xbe, buffer[3]);
  TEST_ASSERT_EQUAL(0xef, buffer[4]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: seekable IO over holding register window
//***************************************************************************

// Configure + init the seekable mock with a 0..31 byte seed pattern,
// configure + bind + init + start the modbus proto.  Caller stops the
// modbus.

static void modbus_setup_seekable(CIOSeekableMock &seek, CProtoModbusRtu &modbus)
{
  io_sdata_t<uint8_t, 32> seed;
  size_t i;

  TEST_ASSERT_EQUAL(OK, modbus.configure());
  TEST_ASSERT_EQUAL(OK, seek.configure());
  TEST_ASSERT_EQUAL(OK, seek.init());

  for (i = 0; i < 32; i++)
    {
      seed(i) = static_cast<uint8_t>(i);
    }
  TEST_ASSERT_EQUAL(OK, seek.setData(seed, 0));

  modbus.setObjectMapItem(MODBUS_DUMMYIO4, &seek);
  TEST_ASSERT_EQUAL(OK, modbus.init());
  TEST_ASSERT_EQUAL(OK, modbus.start());
  usleep(100000);
}

//***************************************************************************
// Description: reading a full window from offset 0 returns the seed bytes
// and the modbus reply matches the configured window size.
//***************************************************************************

static void test_proto_modbus_holding_seekable_read_full_window()
{
  CDescObject descv4(g_cfg_dummy4);
  CIOSeekableMock seek(descv4);
  CDescObject desc(g_bin_modbus_holding_seekable);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_seekable(seek, modbus);

  modbus_send_and_read(MB_HOLDING_GET,
                       REGS_HOLDING1,
                       MODBUS_SEEKABLE_WINDOW_REGS,
                       buffer,
                       sizeof(buffer),
                       5 + (MODBUS_SEEKABLE_WINDOW_REGS * 2));
  TEST_ASSERT_EQUAL(MB_HOLDING_GET, buffer[1]);
  TEST_ASSERT_EQUAL(MODBUS_SEEKABLE_WINDOW_REGS * 2, buffer[2]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: after a complete window read the offset register reflects
// the byte count consumed by that read.
//***************************************************************************

static void test_proto_modbus_holding_seekable_offset_auto_increments()
{
  CDescObject descv4(g_cfg_dummy4);
  CIOSeekableMock seek(descv4);
  CDescObject desc(g_bin_modbus_holding_seekable);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  uint16_t ctrlRegAddr = (uint16_t)((REGS_HOLDING1) + MODBUS_SEEKABLE_WINDOW_REGS);

  modbus_setup_seekable(seek, modbus);

  modbus_send_and_read(MB_HOLDING_GET,
                       REGS_HOLDING1,
                       MODBUS_SEEKABLE_WINDOW_REGS,
                       buffer,
                       sizeof(buffer),
                       5 + (MODBUS_SEEKABLE_WINDOW_REGS * 2));

  modbus_send_and_read(MB_HOLDING_GET, ctrlRegAddr, 1, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(MODBUS_SEEKABLE_WINDOW_REGS * 2, (buffer[3] << 8) + buffer[4]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: writing the offset register sets the read window position;
// the bus echoes the request and the offset is reflected on next read.
//***************************************************************************

static void test_proto_modbus_holding_seekable_set_offset()
{
  CDescObject descv4(g_cfg_dummy4);
  CIOSeekableMock seek(descv4);
  CDescObject desc(g_bin_modbus_holding_seekable);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  uint16_t ctrlRegAddr = (uint16_t)((REGS_HOLDING1) + MODBUS_SEEKABLE_WINDOW_REGS);

  modbus_setup_seekable(seek, modbus);

  modbus_send_and_read(MB_HOLDING_SET, ctrlRegAddr, 4, buffer, sizeof(buffer), 8);
  TEST_ASSERT_EQUAL(MB_HOLDING_SET, buffer[1]);
  TEST_ASSERT_EQUAL(0x00, buffer[4]);
  TEST_ASSERT_EQUAL(0x04, buffer[5]);

  modbus_send_and_read(MB_HOLDING_GET, ctrlRegAddr, 1, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(4, (buffer[3] << 8) + buffer[4]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: a partial read (smaller than the full window) does not
// auto-increment the offset register.
//***************************************************************************

static void test_proto_modbus_holding_seekable_partial_read_no_increment()
{
  CDescObject descv4(g_cfg_dummy4);
  CIOSeekableMock seek(descv4);
  CDescObject desc(g_bin_modbus_holding_seekable);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  uint16_t ctrlRegAddr = (uint16_t)((REGS_HOLDING1) + MODBUS_SEEKABLE_WINDOW_REGS);
  int ret;

  modbus_setup_seekable(seek, modbus);

  // Position the offset.

  modbus_send_and_read(MB_HOLDING_SET, ctrlRegAddr, 4, buffer, sizeof(buffer), 8);

  // Partial read (1 register only).

  usleep(1000);
  ret = modbus_frame_send(MB_HOLDING_GET, REGS_HOLDING1, 1);
  TEST_ASSERT_EQUAL(8, ret);
  usleep(1000);
  ret = (int)read(g_pty_fd, buffer, sizeof(buffer));
  TEST_ASSERT(ret == 7 || ret == 5);
  if (ret == 7)
    {
      TEST_ASSERT_EQUAL(MB_HOLDING_GET, buffer[1]);
      TEST_ASSERT_EQUAL(0x02, buffer[2]);
    }

  // Offset must still be 4.

  modbus_send_and_read(MB_HOLDING_GET, ctrlRegAddr, 1, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(4, (buffer[3] << 8) + buffer[4]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: many coils blocking IO
//***************************************************************************

static void test_proto_modbus_coil_many_blocking()
{
  TEST_IGNORE_MESSAGE("not implemented");
}

//***************************************************************************
// Description: requesting more packed coils than the descriptor binds
// returns exception 0x02.
//***************************************************************************

static void test_proto_modbus_coil_packed_read_out_of_range()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_modbus_coil_packed_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_three(
    dummy1, MODBUS_DUMMYIO1, dummy2, MODBUS_DUMMYIO2, dummy3, MODBUS_DUMMYIO3, modbus);

  modbus_send_and_read(MB_COIL_GET, REGS_COILS1, 10, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_COIL_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading the full 9-coil packed group returns two zero status
// bytes when all dummies are at their initial value.
//***************************************************************************

static void test_proto_modbus_coil_packed_read_initial()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_modbus_coil_packed_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_three(
    dummy1, MODBUS_DUMMYIO1, dummy2, MODBUS_DUMMYIO2, dummy3, MODBUS_DUMMYIO3, modbus);

  modbus_send_and_read(MB_COIL_GET, REGS_COILS1, 9, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(MB_COIL_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x02, buffer[2]); // byte count
  TEST_ASSERT_EQUAL(0x00, buffer[3]); // coils 0..7
  TEST_ASSERT_EQUAL(0x00, buffer[4]); // coil  8

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: writing coil #1 in the packed group lands on dummy2; the
// next read shows dummy2 reflected at bit positions 1, 4, 7.
//***************************************************************************

static void test_proto_modbus_coil_packed_write_first_byte()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_modbus_coil_packed_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<bool, 1> data;

  modbus_setup_three(
    dummy1, MODBUS_DUMMYIO1, dummy2, MODBUS_DUMMYIO2, dummy3, MODBUS_DUMMYIO3, modbus);

  modbus_send_and_read(MB_COIL_SET, REGS_COILS1 + 1, 0xff00, buffer, sizeof(buffer), 8);
  TEST_ASSERT_EQUAL(MB_COIL_SET, buffer[1]);

  modbus_send_and_read(MB_COIL_GET, REGS_COILS1, 9, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(0x02, buffer[2]);
  TEST_ASSERT_EQUAL(0x92, buffer[3]); // bits 1, 4, 7
  TEST_ASSERT_EQUAL(0x00, buffer[4]); // coil 8 clear

  TEST_ASSERT_EQUAL(OK, dummy1.getData(data, 1));
  TEST_ASSERT_EQUAL(false, data(0));
  TEST_ASSERT_EQUAL(OK, dummy2.getData(data, 1));
  TEST_ASSERT_EQUAL(true, data(0));
  TEST_ASSERT_EQUAL(OK, dummy3.getData(data, 1));
  TEST_ASSERT_EQUAL(false, data(0));

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: writing coil #8 (in the second status byte) lands on dummy3;
// after also setting coil #1, the read shows dummy2 + dummy3 bits.
//***************************************************************************

static void test_proto_modbus_coil_packed_write_second_byte()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_modbus_coil_packed_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<bool, 1> data;

  modbus_setup_three(
    dummy1, MODBUS_DUMMYIO1, dummy2, MODBUS_DUMMYIO2, dummy3, MODBUS_DUMMYIO3, modbus);

  modbus_send_and_read(MB_COIL_SET, REGS_COILS1 + 1, 0xff00, buffer, sizeof(buffer), 8);
  modbus_send_and_read(MB_COIL_SET, REGS_COILS1 + 8, 0xff00, buffer, sizeof(buffer), 8);
  TEST_ASSERT_EQUAL(MB_COIL_SET, buffer[1]);

  modbus_send_and_read(MB_COIL_GET, REGS_COILS1, 9, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(0x02, buffer[2]);
  TEST_ASSERT_EQUAL(0xb6, buffer[3]); // bits 1, 2, 4, 5, 7
  TEST_ASSERT_EQUAL(0x01, buffer[4]); // coil 8 set

  TEST_ASSERT_EQUAL(OK, dummy3.getData(data, 1));
  TEST_ASSERT_EQUAL(true, data(0));

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: many discrete blocking IO
//***************************************************************************

static void test_proto_modbus_discrete_many_blocking()
{
  TEST_IGNORE_MESSAGE("not implemented");
}

//***************************************************************************
// Description: requesting more packed discretes than the descriptor binds
// returns exception 0x02.
//***************************************************************************

static void test_proto_modbus_discrete_packed_read_out_of_range()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_modbus_discrete_packed_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_three(
    dummy1, MODBUS_DUMMYIO1, dummy2, MODBUS_DUMMYIO2, dummy3, MODBUS_DUMMYIO3, modbus);

  modbus_send_and_read(MB_DISCRETE_GET, REGS_DISCRETE1, 12, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_DISCRETE_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading the full 9-discrete packed group returns two zero
// status bytes when all dummies are at their initial value.
//***************************************************************************

static void test_proto_modbus_discrete_packed_read_initial()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_modbus_discrete_packed_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_three(
    dummy1, MODBUS_DUMMYIO1, dummy2, MODBUS_DUMMYIO2, dummy3, MODBUS_DUMMYIO3, modbus);

  modbus_send_and_read(MB_DISCRETE_GET, REGS_DISCRETE1, 9, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(MB_DISCRETE_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x02, buffer[2]); // byte count
  TEST_ASSERT_EQUAL(0x00, buffer[3]);
  TEST_ASSERT_EQUAL(0x00, buffer[4]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: setting dummy1 = 1 reflects across all bit positions where
// dummy1 is bound in group1 (0b01001001).
//***************************************************************************

static void test_proto_modbus_discrete_packed_read_group1_after_update()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_modbus_discrete_packed_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<bool, 1> data;

  modbus_setup_three(
    dummy1, MODBUS_DUMMYIO1, dummy2, MODBUS_DUMMYIO2, dummy3, MODBUS_DUMMYIO3, modbus);

  data(0) = 1;
  TEST_ASSERT_EQUAL(OK, dummy1.setData(data));

  modbus_send_and_read(MB_DISCRETE_GET, REGS_DISCRETE1, 9, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(0x02, buffer[1]); // Read discrete
  TEST_ASSERT_EQUAL(0x02, buffer[2]); // byte count
  TEST_ASSERT_EQUAL(0b01001001, buffer[3]);
  TEST_ASSERT_EQUAL(0b00000000, buffer[4]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: group2 packs the same dummies in a different bit pattern;
// reading after dummy1 = 1 returns 0b00100100 / 0b00000001.
//***************************************************************************

static void test_proto_modbus_discrete_packed_read_group2_pattern()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_modbus_discrete_packed_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<bool, 1> data;

  modbus_setup_three(
    dummy1, MODBUS_DUMMYIO1, dummy2, MODBUS_DUMMYIO2, dummy3, MODBUS_DUMMYIO3, modbus);

  data(0) = 1;
  TEST_ASSERT_EQUAL(OK, dummy1.setData(data));

  modbus_send_and_read(MB_DISCRETE_GET, REGS_DISCRETE2, 9, buffer, sizeof(buffer), 7);
  TEST_ASSERT_EQUAL(0x02, buffer[1]);
  TEST_ASSERT_EQUAL(0x02, buffer[2]);
  TEST_ASSERT_EQUAL(0b00100100, buffer[3]);
  TEST_ASSERT_EQUAL(0b00000001, buffer[4]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: input-many proto rejects a coil read with exception 0x02.
//***************************************************************************

static void test_proto_modbus_input_many_read_wrong_function()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject descv5(g_cfg_dummy5);
  CIODummy dummy5(descv5);
  CDescObject descv6(g_cfg_dummy6);
  CIODummy dummy6(descv6);
  CDescObject desc(g_bin_modbus_input_many_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_three(
    dummy4, MODBUS_DUMMYIO4, dummy5, MODBUS_DUMMYIO5, dummy6, MODBUS_DUMMYIO6, modbus);

  modbus_send_and_read(MB_COIL_GET, 0, 1, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_COIL_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: requesting more input registers than the descriptor binds
// returns exception 0x02.
//***************************************************************************

static void test_proto_modbus_input_many_read_out_of_range()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject descv5(g_cfg_dummy5);
  CIODummy dummy5(descv5);
  CDescObject descv6(g_cfg_dummy6);
  CIODummy dummy6(descv6);
  CDescObject desc(g_bin_modbus_input_many_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_three(
    dummy4, MODBUS_DUMMYIO4, dummy5, MODBUS_DUMMYIO5, dummy6, MODBUS_DUMMYIO6, modbus);

  modbus_send_and_read(MB_INPUT_GET, REGS_INPUTS1, 20, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_INPUT_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading the first group of 3 input registers returns the
// dummies' initial values (all zero).
//***************************************************************************

static void test_proto_modbus_input_many_read_group1_initial()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject descv5(g_cfg_dummy5);
  CIODummy dummy5(descv5);
  CDescObject descv6(g_cfg_dummy6);
  CIODummy dummy6(descv6);
  CDescObject desc(g_bin_modbus_input_many_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_three(
    dummy4, MODBUS_DUMMYIO4, dummy5, MODBUS_DUMMYIO5, dummy6, MODBUS_DUMMYIO6, modbus);

  modbus_send_and_read(MB_INPUT_GET, REGS_INPUTS1, 3, buffer, sizeof(buffer), 11);
  TEST_ASSERT_EQUAL(MB_INPUT_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x06, buffer[2]); // byte count
  TEST_ASSERT_EQUAL(0x00, buffer[3]);
  TEST_ASSERT_EQUAL(0x00, buffer[4]);
  TEST_ASSERT_EQUAL(0x00, buffer[5]);
  TEST_ASSERT_EQUAL(0x00, buffer[6]);
  TEST_ASSERT_EQUAL(0x00, buffer[7]);
  TEST_ASSERT_EQUAL(0x00, buffer[8]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: after updating the bound dummies, reading group1 returns the
// new values in declared order (dummy4, dummy5, dummy6).
//***************************************************************************

static void test_proto_modbus_input_many_read_group1_after_update()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject descv5(g_cfg_dummy5);
  CIODummy dummy5(descv5);
  CDescObject descv6(g_cfg_dummy6);
  CIODummy dummy6(descv6);
  CDescObject desc(g_bin_modbus_input_many_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<uint16_t, 1> data;

  modbus_setup_three(
    dummy4, MODBUS_DUMMYIO4, dummy5, MODBUS_DUMMYIO5, dummy6, MODBUS_DUMMYIO6, modbus);

  data(0) = 0x1234;
  TEST_ASSERT_EQUAL(OK, dummy4.setData(data));
  data(0) = 0x2345;
  TEST_ASSERT_EQUAL(OK, dummy5.setData(data));
  data(0) = 0x3456;
  TEST_ASSERT_EQUAL(OK, dummy6.setData(data));

  modbus_send_and_read(MB_INPUT_GET, REGS_INPUTS1, 3, buffer, sizeof(buffer), 11);
  TEST_ASSERT_EQUAL(MB_INPUT_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x06, buffer[2]);
  TEST_ASSERT_EQUAL(0x12, buffer[3]); // dummy4
  TEST_ASSERT_EQUAL(0x34, buffer[4]);
  TEST_ASSERT_EQUAL(0x23, buffer[5]); // dummy5
  TEST_ASSERT_EQUAL(0x45, buffer[6]);
  TEST_ASSERT_EQUAL(0x34, buffer[7]); // dummy6
  TEST_ASSERT_EQUAL(0x56, buffer[8]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: group2 maps the same dummies in swapped order; after
// updating the dummies, reading group2 returns dummy6, dummy5, dummy4.
//***************************************************************************

static void test_proto_modbus_input_many_read_group2_swapped()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject descv5(g_cfg_dummy5);
  CIODummy dummy5(descv5);
  CDescObject descv6(g_cfg_dummy6);
  CIODummy dummy6(descv6);
  CDescObject desc(g_bin_modbus_input_many_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<uint16_t, 1> data;

  modbus_setup_three(
    dummy4, MODBUS_DUMMYIO4, dummy5, MODBUS_DUMMYIO5, dummy6, MODBUS_DUMMYIO6, modbus);

  data(0) = 0x1234;
  TEST_ASSERT_EQUAL(OK, dummy4.setData(data));
  data(0) = 0x2345;
  TEST_ASSERT_EQUAL(OK, dummy5.setData(data));
  data(0) = 0x3456;
  TEST_ASSERT_EQUAL(OK, dummy6.setData(data));

  modbus_send_and_read(MB_INPUT_GET, REGS_INPUTS2, 3, buffer, sizeof(buffer), 11);
  TEST_ASSERT_EQUAL(MB_INPUT_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x06, buffer[2]);
  TEST_ASSERT_EQUAL(0x34, buffer[3]); // dummy6 first
  TEST_ASSERT_EQUAL(0x56, buffer[4]);
  TEST_ASSERT_EQUAL(0x23, buffer[5]); // dummy5
  TEST_ASSERT_EQUAL(0x45, buffer[6]);
  TEST_ASSERT_EQUAL(0x12, buffer[7]); // dummy4 last
  TEST_ASSERT_EQUAL(0x34, buffer[8]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: holding-many proto rejects a coil read with exception 0x02.
//***************************************************************************

static void test_proto_modbus_holding_many_read_wrong_function()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject descv5(g_cfg_dummy5);
  CIODummy dummy5(descv5);
  CDescObject descv6(g_cfg_dummy6);
  CIODummy dummy6(descv6);
  CDescObject desc(g_bin_modbus_holding_many_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_three(
    dummy4, MODBUS_DUMMYIO4, dummy5, MODBUS_DUMMYIO5, dummy6, MODBUS_DUMMYIO6, modbus);

  modbus_send_and_read(MB_COIL_GET, 0, 1, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_COIL_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: requesting more holding registers than the descriptor binds
// returns exception 0x02.
//***************************************************************************

static void test_proto_modbus_holding_many_read_out_of_range()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject descv5(g_cfg_dummy5);
  CIODummy dummy5(descv5);
  CDescObject descv6(g_cfg_dummy6);
  CIODummy dummy6(descv6);
  CDescObject desc(g_bin_modbus_holding_many_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_three(
    dummy4, MODBUS_DUMMYIO4, dummy5, MODBUS_DUMMYIO5, dummy6, MODBUS_DUMMYIO6, modbus);

  modbus_send_and_read(MB_HOLDING_GET, REGS_HOLDING1, 20, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_HOLDING_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading the first group of 3 holding registers returns the
// dummies' initial values (all zero) in declared order.
//***************************************************************************

static void test_proto_modbus_holding_many_read_group1_initial()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject descv5(g_cfg_dummy5);
  CIODummy dummy5(descv5);
  CDescObject descv6(g_cfg_dummy6);
  CIODummy dummy6(descv6);
  CDescObject desc(g_bin_modbus_holding_many_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_three(
    dummy4, MODBUS_DUMMYIO4, dummy5, MODBUS_DUMMYIO5, dummy6, MODBUS_DUMMYIO6, modbus);

  modbus_send_and_read(MB_HOLDING_GET, REGS_HOLDING1, 3, buffer, sizeof(buffer), 11);
  TEST_ASSERT_EQUAL(MB_HOLDING_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x06, buffer[2]); // byte count
  TEST_ASSERT_EQUAL(0x00, buffer[3]);
  TEST_ASSERT_EQUAL(0x00, buffer[4]);
  TEST_ASSERT_EQUAL(0x00, buffer[5]);
  TEST_ASSERT_EQUAL(0x00, buffer[6]);
  TEST_ASSERT_EQUAL(0x00, buffer[7]);
  TEST_ASSERT_EQUAL(0x00, buffer[8]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: writing 3 holding registers to group1 lands on the bound
// dummies in declared order (dummy4, dummy5, dummy6).
//***************************************************************************

static void test_proto_modbus_holding_many_write_group1_in_order()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject descv5(g_cfg_dummy5);
  CIODummy dummy5(descv5);
  CDescObject descv6(g_cfg_dummy6);
  CIODummy dummy6(descv6);
  CDescObject desc(g_bin_modbus_holding_many_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<uint16_t, 1> data;
  int ret;

  modbus_setup_three(
    dummy4, MODBUS_DUMMYIO4, dummy5, MODBUS_DUMMYIO5, dummy6, MODBUS_DUMMYIO6, modbus);

  buffer[0] = 0x11;
  buffer[1] = 0x22;
  buffer[2] = 0x33;
  buffer[3] = 0x44;
  buffer[4] = 0x55;
  buffer[5] = 0x66;
  usleep(1000);
  ret = modbus_frame_send_many(MB_HOLDING_MUL_SET, REGS_HOLDING1, 3, buffer, 6);
  TEST_ASSERT_EQUAL(15, ret);

  usleep(1000);
  ret = (int)read(g_pty_fd, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL(8, ret);
  TEST_ASSERT_EQUAL(MB_HOLDING_MUL_SET, buffer[1]);

  // dummy4 receives reg 0 (0x1122), dummy5 reg 1 (0x3344), dummy6 reg 2.

  TEST_ASSERT_EQUAL(OK, dummy4.getData(data, 1));
  TEST_ASSERT_EQUAL(0x1122, data(0));
  TEST_ASSERT_EQUAL(OK, dummy5.getData(data, 1));
  TEST_ASSERT_EQUAL(0x3344, data(0));
  TEST_ASSERT_EQUAL(OK, dummy6.getData(data, 1));
  TEST_ASSERT_EQUAL(0x5566, data(0));

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: writing 3 holding registers to group2 lands on the bound
// dummies in swapped order (dummy6, dummy5, dummy4).
//***************************************************************************

static void test_proto_modbus_holding_many_write_group2_swapped()
{
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject descv5(g_cfg_dummy5);
  CIODummy dummy5(descv5);
  CDescObject descv6(g_cfg_dummy6);
  CIODummy dummy6(descv6);
  CDescObject desc(g_bin_modbus_holding_many_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<uint16_t, 1> data;
  int ret;

  modbus_setup_three(
    dummy4, MODBUS_DUMMYIO4, dummy5, MODBUS_DUMMYIO5, dummy6, MODBUS_DUMMYIO6, modbus);

  buffer[0] = 0x11;
  buffer[1] = 0x22;
  buffer[2] = 0x33;
  buffer[3] = 0x44;
  buffer[4] = 0x55;
  buffer[5] = 0x66;
  usleep(1000);
  ret = modbus_frame_send_many(MB_HOLDING_MUL_SET, REGS_HOLDING2, 3, buffer, 6);
  TEST_ASSERT_EQUAL(15, ret);

  usleep(1000);
  ret = (int)read(g_pty_fd, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL(8, ret);
  TEST_ASSERT_EQUAL(MB_HOLDING_MUL_SET, buffer[1]);

  // Group2 maps in reversed order: dummy6 reg 0, dummy5 reg 1, dummy4 reg 2.

  TEST_ASSERT_EQUAL(OK, dummy6.getData(data, 1));
  TEST_ASSERT_EQUAL(0x1122, data(0));
  TEST_ASSERT_EQUAL(OK, dummy5.getData(data, 1));
  TEST_ASSERT_EQUAL(0x3344, data(0));
  TEST_ASSERT_EQUAL(OK, dummy4.getData(data, 1));
  TEST_ASSERT_EQUAL(0x5566, data(0));

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: input-longdata proto rejects a coil read with exception
// 0x02.
//***************************************************************************

static void test_proto_modbus_input_longdata_read_wrong_function()
{
  CDescObject descv7(g_cfg_dummy7);
  CIODummy dummy7(descv7);
  CDescObject desc(g_bin_modbus_input_longdata_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single(dummy7, MODBUS_DUMMYIO7, modbus);

  modbus_send_and_read(MB_COIL_GET, 0, 1, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_COIL_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: requesting more registers than the 4-byte IO occupies
// returns exception 0x02.
//***************************************************************************

static void test_proto_modbus_input_longdata_read_out_of_range()
{
  CDescObject descv7(g_cfg_dummy7);
  CIODummy dummy7(descv7);
  CDescObject desc(g_bin_modbus_input_longdata_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single(dummy7, MODBUS_DUMMYIO7, modbus);

  modbus_send_and_read(MB_INPUT_GET, REGS_INPUTS1, 4, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_INPUT_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading both registers of the 4-byte IO at its initial
// value returns four zero bytes.
//***************************************************************************

static void test_proto_modbus_input_longdata_read_initial()
{
  CDescObject descv7(g_cfg_dummy7);
  CIODummy dummy7(descv7);
  CDescObject desc(g_bin_modbus_input_longdata_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single(dummy7, MODBUS_DUMMYIO7, modbus);

  modbus_send_and_read(MB_INPUT_GET, REGS_INPUTS1, 2, buffer, sizeof(buffer), 9);
  TEST_ASSERT_EQUAL(MB_INPUT_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x04, buffer[2]); // byte count
  TEST_ASSERT_EQUAL(0x00, buffer[3]);
  TEST_ASSERT_EQUAL(0x00, buffer[4]);
  TEST_ASSERT_EQUAL(0x00, buffer[5]);
  TEST_ASSERT_EQUAL(0x00, buffer[6]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: after writing 0xdeadbeef into the 4-byte IO, reading two
// registers returns the four payload bytes in modbus word-swap order.
//***************************************************************************

static void test_proto_modbus_input_longdata_read_after_update()
{
  CDescObject descv7(g_cfg_dummy7);
  CIODummy dummy7(descv7);
  CDescObject desc(g_bin_modbus_input_longdata_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<uint32_t, 1> data;

  modbus_setup_single(dummy7, MODBUS_DUMMYIO7, modbus);

  data(0) = 0xdeadbeef;
  TEST_ASSERT_EQUAL(OK, dummy7.setData(data));

  modbus_send_and_read(MB_INPUT_GET, REGS_INPUTS1, 2, buffer, sizeof(buffer), 9);
  TEST_ASSERT_EQUAL(MB_INPUT_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x04, buffer[2]);
  TEST_ASSERT_EQUAL((data(0) >> 8) & 0xff, buffer[3]);
  TEST_ASSERT_EQUAL(data(0) & 0xff, buffer[4]);
  TEST_ASSERT_EQUAL((data(0) >> 24) & 0xff, buffer[5]);
  TEST_ASSERT_EQUAL((data(0) >> 16) & 0xff, buffer[6]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: holding-longdata proto rejects a coil read with exception
// 0x02.
//***************************************************************************

static void test_proto_modbus_holding_longdata_read_wrong_function()
{
  CDescObject descv7(g_cfg_dummy7);
  CIODummy dummy7(descv7);
  CDescObject desc(g_bin_modbus_holding_longdata_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single(dummy7, MODBUS_DUMMYIO7, modbus);

  modbus_send_and_read(MB_COIL_GET, 0, 1, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_COIL_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: requesting more registers than the 4-byte IO occupies
// returns exception 0x02.
//***************************************************************************

static void test_proto_modbus_holding_longdata_read_out_of_range()
{
  CDescObject descv7(g_cfg_dummy7);
  CIODummy dummy7(descv7);
  CDescObject desc(g_bin_modbus_holding_longdata_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single(dummy7, MODBUS_DUMMYIO7, modbus);

  modbus_send_and_read(MB_HOLDING_GET, REGS_HOLDING1, 4, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_HOLDING_GET, 0x02);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: reading both registers of the 4-byte holding IO at its
// initial value returns four zero bytes.
//***************************************************************************

static void test_proto_modbus_holding_longdata_read_initial()
{
  CDescObject descv7(g_cfg_dummy7);
  CIODummy dummy7(descv7);
  CDescObject desc(g_bin_modbus_holding_longdata_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];

  modbus_setup_single(dummy7, MODBUS_DUMMYIO7, modbus);

  modbus_send_and_read(MB_HOLDING_GET, REGS_HOLDING1, 2, buffer, sizeof(buffer), 9);
  TEST_ASSERT_EQUAL(MB_HOLDING_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x04, buffer[2]); // byte count
  TEST_ASSERT_EQUAL(0x00, buffer[3]);
  TEST_ASSERT_EQUAL(0x00, buffer[4]);
  TEST_ASSERT_EQUAL(0x00, buffer[5]);
  TEST_ASSERT_EQUAL(0x00, buffer[6]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: a single-register write to a 4-byte holding IO is rejected
// with exception 0x04 (illegal data value), and the IO state is preserved.
//***************************************************************************

static void test_proto_modbus_holding_longdata_single_write_rejected()
{
  CDescObject descv7(g_cfg_dummy7);
  CIODummy dummy7(descv7);
  CDescObject desc(g_bin_modbus_holding_longdata_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  io_sdata_t<uint32_t, 1> data;

  modbus_setup_single(dummy7, MODBUS_DUMMYIO7, modbus);

  modbus_send_and_read(MB_HOLDING_SET, REGS_HOLDING1, 0xdead, buffer, sizeof(buffer), 5);
  modbus_assert_exception(buffer, MB_HOLDING_SET, 0x04);

  TEST_ASSERT_EQUAL(OK, dummy7.getData(data, 1));
  TEST_ASSERT_EQUAL(0u, data(0));

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: a 4-byte multi-register write succeeds, the bus echoes the
// request, and the next read returns the new payload bytes.
//***************************************************************************

static void test_proto_modbus_holding_longdata_multi_write_then_read()
{
  CDescObject descv7(g_cfg_dummy7);
  CIODummy dummy7(descv7);
  CDescObject desc(g_bin_modbus_holding_longdata_blocking);
  CProtoModbusRtu modbus(desc);
  uint8_t buffer[1024];
  int ret;

  modbus_setup_single(dummy7, MODBUS_DUMMYIO7, modbus);

  buffer[0] = 0xef;
  buffer[1] = 0xbe;
  buffer[2] = 0xad;
  buffer[3] = 0xde;
  usleep(1000);
  ret = modbus_frame_send_many(MB_HOLDING_MUL_SET, REGS_HOLDING1, 2, buffer, 4);
  TEST_ASSERT_EQUAL(13, ret);
  usleep(1000);
  ret = (int)read(g_pty_fd, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL(8, ret);
  TEST_ASSERT_EQUAL(MB_HOLDING_MUL_SET, buffer[1]);

  modbus_send_and_read(MB_HOLDING_GET, REGS_HOLDING1, 2, buffer, sizeof(buffer), 9);
  TEST_ASSERT_EQUAL(MB_HOLDING_GET, buffer[1]);
  TEST_ASSERT_EQUAL(0x04, buffer[2]);
  TEST_ASSERT_EQUAL(0xef, buffer[3]);
  TEST_ASSERT_EQUAL(0xbe, buffer[4]);
  TEST_ASSERT_EQUAL(0xad, buffer[5]);
  TEST_ASSERT_EQUAL(0xde, buffer[6]);

  TEST_ASSERT_EQUAL(OK, modbus.stop());
}

//***************************************************************************
// Description: placeholder for many blocking Modbus RTU bindings.
//***************************************************************************

static void test_proto_modbus_many_blocking()
{
  TEST_IGNORE_MESSAGE("not implemented");
}

//***************************************************************************
// Description: placeholder for single notify Modbus RTU binding.
//***************************************************************************

static void test_proto_modbus_simple_notify()
{
  TEST_IGNORE_MESSAGE("not implemented");
}

//***************************************************************************
// Description: placeholder for many notify Modbus RTU bindings.
//***************************************************************************

static void test_proto_modbus_many_notify()
{
  TEST_IGNORE_MESSAGE("not implemented");
}

extern "C"
{
  int test_proto_modbus_rtu()
  {
    UNITY_BEGIN();

    // Each subtest reopens /dev/pty0 so that the NuttX pty driver tears
    // down and rebuilds the internal pipe endpoints; otherwise reopening
    // only the slave leaves the shared pipe structures closed and causes
    // EBADF on read in subsequent tests.

#define RUN_WITH_PTY(fn) \
  do                     \
    {                    \
      open_test_pty();   \
      DAWN_RUN_TEST(fn); \
      close_test_pty();  \
    }                    \
  while (0)

    // Single register access

    RUN_WITH_PTY(test_proto_modbus_coil_lifecycle);
    RUN_WITH_PTY(test_proto_modbus_coil_read_wrong_function);
    RUN_WITH_PTY(test_proto_modbus_coil_read_out_of_range);
    RUN_WITH_PTY(test_proto_modbus_coil_read_initial);
    RUN_WITH_PTY(test_proto_modbus_coil_write_then_read);
    RUN_WITH_PTY(test_proto_modbus_coil_writeonly_shadow_readback);

    RUN_WITH_PTY(test_proto_modbus_discrete_read_wrong_function);
    RUN_WITH_PTY(test_proto_modbus_discrete_read_out_of_range);
    RUN_WITH_PTY(test_proto_modbus_discrete_read_initial);
    RUN_WITH_PTY(test_proto_modbus_discrete_read_after_update);

    RUN_WITH_PTY(test_proto_modbus_input_read_wrong_function);
    RUN_WITH_PTY(test_proto_modbus_input_read_out_of_range);
    RUN_WITH_PTY(test_proto_modbus_input_read_initial);
    RUN_WITH_PTY(test_proto_modbus_input_read_after_update);

    RUN_WITH_PTY(test_proto_modbus_holding_read_wrong_function);
    RUN_WITH_PTY(test_proto_modbus_holding_read_out_of_range);
    RUN_WITH_PTY(test_proto_modbus_holding_read_initial);
    RUN_WITH_PTY(test_proto_modbus_holding_write_then_read);
    RUN_WITH_PTY(test_proto_modbus_holding_writeonly_shadow_readback);

    // Many registers access

    RUN_WITH_PTY(test_proto_modbus_coil_many_blocking);
    RUN_WITH_PTY(test_proto_modbus_discrete_many_blocking);
    RUN_WITH_PTY(test_proto_modbus_coil_packed_read_out_of_range);
    RUN_WITH_PTY(test_proto_modbus_coil_packed_read_initial);
    RUN_WITH_PTY(test_proto_modbus_coil_packed_write_first_byte);
    RUN_WITH_PTY(test_proto_modbus_coil_packed_write_second_byte);
    RUN_WITH_PTY(test_proto_modbus_discrete_packed_read_out_of_range);
    RUN_WITH_PTY(test_proto_modbus_discrete_packed_read_initial);
    RUN_WITH_PTY(test_proto_modbus_discrete_packed_read_group1_after_update);
    RUN_WITH_PTY(test_proto_modbus_discrete_packed_read_group2_pattern);
    RUN_WITH_PTY(test_proto_modbus_input_many_read_wrong_function);
    RUN_WITH_PTY(test_proto_modbus_input_many_read_out_of_range);
    RUN_WITH_PTY(test_proto_modbus_input_many_read_group1_initial);
    RUN_WITH_PTY(test_proto_modbus_input_many_read_group1_after_update);
    RUN_WITH_PTY(test_proto_modbus_input_many_read_group2_swapped);
    RUN_WITH_PTY(test_proto_modbus_holding_many_read_wrong_function);
    RUN_WITH_PTY(test_proto_modbus_holding_many_read_out_of_range);
    RUN_WITH_PTY(test_proto_modbus_holding_many_read_group1_initial);
    RUN_WITH_PTY(test_proto_modbus_holding_many_write_group1_in_order);
    RUN_WITH_PTY(test_proto_modbus_holding_many_write_group2_swapped);

    // Complex data struct for input and holding

    RUN_WITH_PTY(test_proto_modbus_input_longdata_read_wrong_function);
    RUN_WITH_PTY(test_proto_modbus_input_longdata_read_out_of_range);
    RUN_WITH_PTY(test_proto_modbus_input_longdata_read_initial);
    RUN_WITH_PTY(test_proto_modbus_input_longdata_read_after_update);
    RUN_WITH_PTY(test_proto_modbus_holding_longdata_read_wrong_function);
    RUN_WITH_PTY(test_proto_modbus_holding_longdata_read_out_of_range);
    RUN_WITH_PTY(test_proto_modbus_holding_longdata_read_initial);
    RUN_WITH_PTY(test_proto_modbus_holding_longdata_single_write_rejected);
    RUN_WITH_PTY(test_proto_modbus_holding_longdata_multi_write_then_read);

    // Complex register combinations

    RUN_WITH_PTY(test_proto_modbus_many_blocking);

    // IOs with notify

    RUN_WITH_PTY(test_proto_modbus_simple_notify);
    RUN_WITH_PTY(test_proto_modbus_many_notify);

    // Keep seekable test last. If it fails, it must not block legacy cases
    // due to single-instance Modbus RTU limitation.
    RUN_WITH_PTY(test_proto_modbus_holding_seekable_read_full_window);
    RUN_WITH_PTY(test_proto_modbus_holding_seekable_offset_auto_increments);
    RUN_WITH_PTY(test_proto_modbus_holding_seekable_set_offset);
    RUN_WITH_PTY(test_proto_modbus_holding_seekable_partial_read_no_increment);

#undef RUN_WITH_PTY

    return UNITY_END();
  }
}
