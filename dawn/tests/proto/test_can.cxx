// dawn/tests/proto/test_can.cxx
//
// SPDX-License-Ide
//

#include <fcntl.h>
#include <unistd.h>

#include <vector>

#include "dawn/dev/descriptor.hxx"
#include "dawn/io/descriptor.hxx"
#include "dawn/io/dummy.hxx"
#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/porting/can.hxx"
#include "dawn/proto/can/can.hxx"
#include "test_common.hxx"

using namespace dawn;

#if !defined(CONFIG_SIM_CANDEV_CHAR) || !defined(CONFIG_CAN_LOOPBACK)
#  error CAN loopback support must be enabled
#endif

#if defined(CONFIG_DAWN_PROTO_CAN_EXTID)
static constexpr auto CAN_EXTID_FLAG = 1;
#else
static constexpr auto CAN_EXTID_FLAG = 0;
#endif

// Path to CAN dev

static constexpr auto CAN_DEVPATH = "/dev/can0";

// IOs read only

static constexpr auto CAN_DUMMYIO1 = CIODummy::objectId(SObjectId::DTYPE_BOOL, false, 0);
static constexpr auto CAN_DUMMYIO2 = CIODummy::objectId(SObjectId::DTYPE_BOOL, false, 2);
static constexpr auto CAN_DUMMYIO3 = CIODummy::objectId(SObjectId::DTYPE_BOOL, false, 3);

static constexpr auto CAN_DUMMYIO4 = CIODummy::objectId(SObjectId::DTYPE_UINT8, false, 4);
static constexpr auto CAN_DUMMYIO5 = CIODummy::objectId(SObjectId::DTYPE_UINT16, false, 5);
static constexpr auto CAN_DUMMYIO6 = CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 6);
static constexpr auto CAN_DUMMYIO7 = CIODummy::objectId(SObjectId::DTYPE_UINT64, false, 7);

// IOs read only with notify

static constexpr auto CAN_NOTIFYIO1 = CIODummyNotify::objectId(SObjectId::DTYPE_BOOL, false, 0);
static constexpr auto CAN_NOTIFYIO2 = CIODummyNotify::objectId(SObjectId::DTYPE_BOOL, false, 2);
static constexpr auto CAN_NOTIFYIO3 = CIODummyNotify::objectId(SObjectId::DTYPE_BOOL, false, 3);

static constexpr auto CAN_NOTIFYIO4 = CIODummyNotify::objectId(SObjectId::DTYPE_UINT8, false, 4);
static constexpr auto CAN_NOTIFYIO5 = CIODummyNotify::objectId(SObjectId::DTYPE_UINT16, false, 5);
static constexpr auto CAN_NOTIFYIO6 = CIODummyNotify::objectId(SObjectId::DTYPE_UINT32, false, 6);
static constexpr auto CAN_NOTIFYIO7 = CIODummyNotify::objectId(SObjectId::DTYPE_UINT64, false, 7);

// IOs read-write

static constexpr auto CAN_DUMMYIO8 = CIODummy::objectId(SObjectId::DTYPE_BOOL, false, 0);
static constexpr auto CAN_DUMMYIO9 = CIODummy::objectId(SObjectId::DTYPE_BOOL, false, 2);
static constexpr auto CAN_DUMMYIO10 = CIODummy::objectId(SObjectId::DTYPE_BOOL, false, 3);

static constexpr auto CAN_DUMMYIO11 = CIODummy::objectId(SObjectId::DTYPE_UINT8, false, 4);
static constexpr auto CAN_DUMMYIO12 = CIODummy::objectId(SObjectId::DTYPE_UINT16, false, 5);
static constexpr auto CAN_DUMMYIO13 = CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 6);
static constexpr auto CAN_DUMMYIO14 = CIODummy::objectId(SObjectId::DTYPE_UINT64, false, 7);
static constexpr auto CAN_DUMMYIO15 = CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 8);
static constexpr auto CAN_DESCIO1 = CIODescriptor::objectId(0);

// Test configurations

static constexpr auto CAN_NODE_ID1 = (2);
static constexpr auto CAN_NODE_ID2 = (3);

static constexpr auto CAN_NODE_IO1_START = (0x000);
static constexpr auto CAN_NODE_IO2_START = (0x100);
static constexpr auto CAN_NODE_IO3_START = (0x500);

static int g_can_fd;
static constexpr useconds_t CAN_NOTIFY_WAIT_US = 60000;

//***************************************************************************
// Description: manual notifier forwards dummy notifications to CAN tests.
//***************************************************************************

class CManualNotifier : public IIONotifier
{
public:
  ~CManualNotifier()
  {
    for (auto &entry : entries)
      {
        delete entry.data;
      }
  }

  int regNotifier(SIONotifier n) override
  {
    SEntry entry;

    if (n.io == nullptr || n.cb == nullptr)
      {
        return -EINVAL;
      }

    if (n.io->isNotify() == false)
      {
        return -EINVAL;
      }

    entry.user = n;
    entry.data = n.io->ddata_alloc(1);
    if (entry.data == nullptr)
      {
        return -ENOMEM;
      }

    entries.push_back(entry);
    return OK;
  }

  int notify(CIODummyNotify &io)
  {
    int ret;

    ret = io.notify();
    if (ret < 0)
      {
        return ret;
      }

    usleep(1);

    for (auto &entry : entries)
      {
        if (entry.user.io != &io)
          {
            continue;
          }

        ret = io.getData(*entry.data, 1);
        if (ret < 0)
          {
            return ret;
          }

        entry.user.cb(entry.user.priv, entry.data);
      }

    return OK;
  }

private:
  struct SEntry
  {
    SIONotifier user;
    io_ddata_t *data;
  };

  std::vector<SEntry> entries;
};

static void can_trigger_bool(CIODummyNotify &io, CManualNotifier &notifier, bool value)
{
  io_sdata_t<bool, 1, 1> data;

  data(0, 0) = value;
  TEST_ASSERT_EQUAL(OK, io.setData(data));
  TEST_ASSERT_EQUAL(OK, notifier.notify(io));
  usleep(CAN_NOTIFY_WAIT_US);
}

static void can_set_bool(CIOCommon &io, bool value)
{
  io_sdata_t<bool, 1, 1> data;

  data(0, 0) = value;
  TEST_ASSERT_EQUAL(OK, io.setData(data));
}

static void can_trigger_u8(CIODummyNotify &io, CManualNotifier &notifier, uint8_t value)
{
  io_sdata_t<uint8_t, 1, 1> data;

  data(0, 0) = value;
  TEST_ASSERT_EQUAL(OK, io.setData(data));
  TEST_ASSERT_EQUAL(OK, notifier.notify(io));
  usleep(CAN_NOTIFY_WAIT_US);
}

static void can_set_u8(CIOCommon &io, uint8_t value)
{
  io_sdata_t<uint8_t, 1, 1> data;

  data(0, 0) = value;
  TEST_ASSERT_EQUAL(OK, io.setData(data));
}

static void can_set_u16(CIOCommon &io, uint16_t value)
{
  io_sdata_t<uint16_t, 1, 1> data;

  data(0, 0) = value;
  TEST_ASSERT_EQUAL(OK, io.setData(data));
}

static void can_trigger_u16(CIODummyNotify &io, CManualNotifier &notifier, uint16_t value)
{
  io_sdata_t<uint16_t, 1, 1> data;

  data(0, 0) = value;
  TEST_ASSERT_EQUAL(OK, io.setData(data));
  TEST_ASSERT_EQUAL(OK, notifier.notify(io));
  usleep(CAN_NOTIFY_WAIT_US);
}

static void can_trigger_u32(CIODummyNotify &io, CManualNotifier &notifier, uint32_t value)
{
  io_sdata_t<uint32_t, 1, 1> data;

  data(0, 0) = value;
  TEST_ASSERT_EQUAL(OK, io.setData(data));
  TEST_ASSERT_EQUAL(OK, notifier.notify(io));
  usleep(CAN_NOTIFY_WAIT_US);
}

static void can_set_u32(CIOCommon &io, uint32_t value)
{
  io_sdata_t<uint32_t, 1, 1> data;

  data(0, 0) = value;
  TEST_ASSERT_EQUAL(OK, io.setData(data));
}

static void can_trigger_u64(CIODummyNotify &io, CManualNotifier &notifier, uint64_t value)
{
  io_sdata_t<uint64_t, 1, 1> data;

  data(0, 0) = value;
  TEST_ASSERT_EQUAL(OK, io.setData(data));
  TEST_ASSERT_EQUAL(OK, notifier.notify(io));
  usleep(CAN_NOTIFY_WAIT_US);
}

static void can_set_u64(CIOCommon &io, uint64_t value)
{
  io_sdata_t<uint64_t, 1, 1> data;

  data(0, 0) = value;
  TEST_ASSERT_EQUAL(OK, io.setData(data));
}

static void can_drain_queue()
{
  dawn::porting::canmsg_s msg = {0};
  int ret;
  int i;

  for (i = 0; i < 100; i++)
    {
      ret = can_read(g_can_fd, &msg);
      if (ret < 0)
        {
          TEST_ASSERT_EQUAL(-EAGAIN, ret);
          break;
        }
    }
}

static void can_drain_fd(int fd)
{
  dawn::porting::canmsg_s msg = {0};
  int ret;
  int i;

  for (i = 0; i < 100; i++)
    {
      ret = can_read(fd, &msg);
      if (ret < 0)
        {
          TEST_ASSERT_EQUAL(-EAGAIN, ret);
          break;
        }
    }
}

static int can_read_wait_id(int fd, dawn::porting::canmsg_s *msg, uint32_t id)
{
  int ret;
  int i;

  for (i = 0; i < 200; i++)
    {
      ret = can_read(fd, msg);
      if (ret > 0)
        {
          if (msg->id == id)
            {
              return ret;
            }

          continue;
        }

      if (ret == -EAGAIN)
        {
          usleep(100);
          continue;
        }

      return ret;
    }

  return -EAGAIN;
}

// Wait for a CAN frame matching expected_id and assert id/len/first-byte.

static void can_assert_frame(uint32_t expected_id, uint8_t expected_len, uint8_t expected_byte0)
{
  dawn::porting::canmsg_s msg = {0};
  int ret;

  ret = can_read_wait_id(g_can_fd, &msg, expected_id);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(expected_id, msg.id);
  TEST_ASSERT_EQUAL(expected_len, msg.len);
  TEST_ASSERT_EQUAL(expected_byte0, msg.data[0]);
}

// Drain up to `max` frames from the CAN bus, stopping early on EAGAIN.
// Used to flush startup chatter at the beginning of read/write tests.

static void can_drain_n(int max)
{
  dawn::porting::canmsg_s msg = {0};
  int ret;
  int i;

  for (i = 0; i < max; i++)
    {
      ret = can_read(g_can_fd, &msg);
      if (ret < 0)
        {
          break;
        }
    }
}

//***************************************************************************
// Description: dummy IOs
//***************************************************************************

static uint32_t g_cfg_dummy1[] = {
  CAN_DUMMYIO1,
  0,
};

static uint32_t g_cfg_dummy2[] = {
  CAN_DUMMYIO2,
  0,
};

static uint32_t g_cfg_dummy3[] = {
  CAN_DUMMYIO3,
  0,
};

static uint32_t g_cfg_dummy4[] = {
  CAN_DUMMYIO4,
  0,
};

static uint32_t g_cfg_dummy5[] = {
  CAN_DUMMYIO5,
  0,
};

static uint32_t g_cfg_dummy6[] = {
  CAN_DUMMYIO6,
  0,
};

static uint32_t g_cfg_dummy7[] = {
  CAN_DUMMYIO7,
  0,
};

static uint32_t g_cfg_notify1[] = {
  CAN_NOTIFYIO1,
  1,
  CIODummyNotify::cfgInterval(false),
  50000,
};

static uint32_t g_cfg_notify2[] = {
  CAN_NOTIFYIO2,
  1,
  CIODummyNotify::cfgInterval(false),
  50000,
};

static uint32_t g_cfg_notify3[] = {
  CAN_NOTIFYIO3,
  1,
  CIODummyNotify::cfgInterval(false),
  50000,
};

static uint32_t g_cfg_notify4[] = {
  CAN_NOTIFYIO4,
  1,
  CIODummyNotify::cfgInterval(false),
  50000,
};

static uint32_t g_cfg_notify5[] = {
  CAN_NOTIFYIO5,
  1,
  CIODummyNotify::cfgInterval(false),
  50000,
};

static uint32_t g_cfg_notify6[] = {
  CAN_NOTIFYIO6,
  1,
  CIODummyNotify::cfgInterval(false),
  50000,
};

static uint32_t g_cfg_notify7[] = {
  CAN_NOTIFYIO7,
  1,
  CIODummyNotify::cfgInterval(false),
  50000,
};

static uint32_t g_cfg_dummy8[] = {
  CAN_DUMMYIO8,
  0,
};

static uint32_t g_cfg_dummy9[] = {
  CAN_DUMMYIO9,
  0,
};

static uint32_t g_cfg_dummy10[] = {
  CAN_DUMMYIO10,
  0,
};

static uint32_t g_cfg_dummy11[] = {
  CAN_DUMMYIO11,
  0,
};

static uint32_t g_cfg_dummy12[] = {
  CAN_DUMMYIO12,
  0,
};

static uint32_t g_cfg_dummy13[] = {
  CAN_DUMMYIO13,
  0,
};

static uint32_t g_cfg_dummy14[] = {
  CAN_DUMMYIO14,
  0,
};

static uint32_t g_cfg_dummy15[] = {
  CAN_DUMMYIO15,
  1,
  CIODummy::cfgIdDim(),
  2,
};

#ifdef CONFIG_DAWN_PROTO_CAN_SEG
static uint32_t g_cfg_descio1[] = {
  CAN_DESCIO1,
  0,
};

static uint32_t g_dawn_desc_seek1[] = {
  1,
  2,
  3,
  4,
  5,
};
#endif

//***************************************************************************
// Minimal push descriptors: each binds exactly the IO(s) the matching test
// case exercises.  Keeps the per-test boilerplate to "one dummy, one
// descriptor, one proto".
//***************************************************************************

static uint32_t g_bin_can_push_one_bool[] = {
  CProtoCan::objectId(0),
  2,                        //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1,             //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_PUSH, //
  CAN_NODE_IO1_START,       //
  1,
  CAN_NOTIFYIO1,
};

static uint32_t g_bin_can_push_one_u8[] = {
  CProtoCan::objectId(0),
  2,                        //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1,             //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_PUSH, //
  CAN_NODE_IO1_START,       //
  1,
  CAN_NOTIFYIO4,
};

static uint32_t g_bin_can_push_one_u16[] = {
  CProtoCan::objectId(0),
  2,                        //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1,             //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_PUSH, //
  CAN_NODE_IO1_START,       //
  1,
  CAN_NOTIFYIO5,
};

static uint32_t g_bin_can_push_one_u32[] = {
  CProtoCan::objectId(0),
  2,                        //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1,             //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_PUSH, //
  CAN_NODE_IO1_START,       //
  1,
  CAN_NOTIFYIO6,
};

static uint32_t g_bin_can_push_one_u64[] = {
  CProtoCan::objectId(0),
  2,                        //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1,             //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_PUSH, //
  CAN_NODE_IO1_START,       //
  1,
  CAN_NOTIFYIO7,
};

static uint32_t g_bin_can_push_three_bools[] = {
  CProtoCan::objectId(0),
  2,                        //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1,             //
  CProtoCan::cfgIdIOBind(6),
  CProtoCan::CAN_TYPE_PUSH, //
  CAN_NODE_IO1_START,       //
  3,
  CAN_NOTIFYIO1,
  CAN_NOTIFYIO2,
  CAN_NOTIFYIO3,
};

//***************************************************************************
// Minimal read descriptors (CIODummy, RTR-driven request/response).
//***************************************************************************

static uint32_t g_bin_can_read_one_bool[] = {
  CProtoCan::objectId(0),
  2,                        //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1,             //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_READ, //
  CAN_NODE_IO1_START,       //
  1,
  CAN_DUMMYIO1,
};

static uint32_t g_bin_can_read_one_u8[] = {
  CProtoCan::objectId(0),
  2,                        //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1,             //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_READ, //
  CAN_NODE_IO1_START,       //
  1,
  CAN_DUMMYIO4,
};

static uint32_t g_bin_can_read_one_u16[] = {
  CProtoCan::objectId(0),
  2,                        //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1,             //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_READ, //
  CAN_NODE_IO1_START,       //
  1,
  CAN_DUMMYIO5,
};

static uint32_t g_bin_can_read_one_u32[] = {
  CProtoCan::objectId(0),
  2,                        //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1,             //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_READ, //
  CAN_NODE_IO1_START,       //
  1,
  CAN_DUMMYIO6,
};

static uint32_t g_bin_can_read_one_u64[] = {
  CProtoCan::objectId(0),
  2,                        //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1,             //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_READ, //
  CAN_NODE_IO1_START,       //
  1,
  CAN_DUMMYIO7,
};

//***************************************************************************
// Minimal write descriptors (CIODummy, frame-driven write).
//***************************************************************************

static uint32_t g_bin_can_write_three_bools[] = {
  CProtoCan::objectId(0),
  2,                         //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID2,              //
  CProtoCan::cfgIdIOBind(6),
  CProtoCan::CAN_TYPE_WRITE, //
  CAN_NODE_IO1_START,        //
  3,
  CAN_DUMMYIO8,
  CAN_DUMMYIO9,
  CAN_DUMMYIO10,
};

static uint32_t g_bin_can_write_one_u8[] = {
  CProtoCan::objectId(0),
  2,                         //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID2,              //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_WRITE, //
  CAN_NODE_IO1_START + 3,    //
  1,
  CAN_DUMMYIO11,
};

static uint32_t g_bin_can_write_one_u16[] = {
  CProtoCan::objectId(0),
  2,                         //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID2,              //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_WRITE, //
  CAN_NODE_IO1_START + 4,    //
  1,
  CAN_DUMMYIO12,
};

static uint32_t g_bin_can_write_one_u32[] = {
  CProtoCan::objectId(0),
  2,                         //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID2,              //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_WRITE, //
  CAN_NODE_IO1_START + 5,    //
  1,
  CAN_DUMMYIO13,
};

static uint32_t g_bin_can_write_one_u64[] = {
  CProtoCan::objectId(0),
  2,                         //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID2,              //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_WRITE, //
  CAN_NODE_IO1_START + 6,    //
  1,
  CAN_DUMMYIO14,
};

static uint32_t g_bin_can_write_one_u32_vec2[] = {
  CProtoCan::objectId(0),
  2,                         //
  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID2,              //
  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_WRITE, //
  CAN_NODE_IO1_START + 7,    //
  1,
  CAN_DUMMYIO15,
};

//***************************************************************************
// Description: push data, many groups
//***************************************************************************

static uint32_t g_bin_can_push_many[] = {
  // Object ID

  CProtoCan::objectId(0),
  4,

  // Node ID

  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1, // Node id

  // Allocated objects

  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_PUSH, //
  CAN_NODE_IO1_START,       //
  1,                        //
  CAN_NOTIFYIO1,

  CProtoCan::cfgIdIOBind(6),
  CProtoCan::CAN_TYPE_PUSH, //
  CAN_NODE_IO2_START,       //
  3,                        //
  CAN_NOTIFYIO2,
  CAN_NOTIFYIO3,
  CAN_NOTIFYIO4,

  CProtoCan::cfgIdIOBind(6),
  CProtoCan::CAN_TYPE_PUSH, //
  CAN_NODE_IO3_START,       //
  3,                        //
  CAN_NOTIFYIO5,
  CAN_NOTIFYIO6,
  CAN_NOTIFYIO7,
};

//***************************************************************************
// Description: read data, many group
//***************************************************************************

static uint32_t g_bin_can_read_many[] = {
  // Object ID

  CProtoCan::objectId(0),
  4,

  // Node ID

  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1, // Node id

  // Allocated objects

  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_READ, //
  CAN_NODE_IO1_START,       //
  1,                        //
  CAN_DUMMYIO1,

  CProtoCan::cfgIdIOBind(6),
  CProtoCan::CAN_TYPE_READ, //
  CAN_NODE_IO2_START,       //
  3,                        //
  CAN_DUMMYIO2,
  CAN_DUMMYIO3,
  CAN_DUMMYIO4,

  CProtoCan::cfgIdIOBind(6),
  CProtoCan::CAN_TYPE_READ, //
  CAN_NODE_IO3_START,       //
  3,                        //
  CAN_DUMMYIO5,
  CAN_DUMMYIO6,
  CAN_DUMMYIO7,
};

#ifdef CONFIG_DAWN_PROTO_CAN_SINGLE_ID
//***************************************************************************
// Description: read data, single ID indexed
//***************************************************************************

static uint32_t g_bin_can_read_single_id[] = {
  // Object ID

  CProtoCan::objectId(0),
  2,

  // Node ID

  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1, // Node id

  // Allocated objects

  CProtoCan::cfgIdIOBind(5),
  CProtoCan::CAN_TYPE_INDEXED_READ, //
  CAN_NODE_IO1_START,               //
  2,                                //
  CAN_DUMMYIO11,
  CAN_DUMMYIO14,
};

//***************************************************************************
// Description: write data, single ID indexed
//***************************************************************************

static uint32_t g_bin_can_write_single_id[] = {
  // Object ID

  CProtoCan::objectId(0),
  2,

  // Node ID

  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID2, // Node id

  // Allocated objects

  CProtoCan::cfgIdIOBind(5),
  CProtoCan::CAN_TYPE_INDEXED_WRITE, //
  CAN_NODE_IO1_START,                //
  2,                                 //
  CAN_DUMMYIO11,
  CAN_DUMMYIO14,
};

#endif

#ifdef CONFIG_DAWN_PROTO_CAN_SEG
//***************************************************************************
// Description: read data, segmented
//***************************************************************************

static uint32_t g_bin_can_read_seg[] = {
  // Object ID

  CProtoCan::objectId(0),
  2,

  // Node ID

  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1, // Node id

  // Allocated objects

  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_READ_SEG, //
  CAN_NODE_IO2_START,           //
  1,                            //
  CAN_DUMMYIO14,
};

//***************************************************************************
// Description: read seekable data, segmented
//***************************************************************************

static uint32_t g_bin_can_read_seg_seek[] = {
  // Object ID

  CProtoCan::objectId(0),
  2,

  // Node ID

  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID1, // Node id

  // Allocated objects

  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_READ_SEG, //
  CAN_NODE_IO3_START,           //
  1,                            //
  CAN_DESCIO1,
};

//***************************************************************************
// Description: write data, segmented
//***************************************************************************

static uint32_t g_bin_can_write_seg[] = {
  // Object ID

  CProtoCan::objectId(0),
  2,

  // Node ID

  CProtoCan::cfgIdNodeid(),
  CAN_NODE_ID2, // Node id

  // Allocated objects

  CProtoCan::cfgIdIOBind(4),
  CProtoCan::CAN_TYPE_WRITE_SEG, //
  CAN_NODE_IO2_START,            //
  1,                             //
  CAN_DUMMYIO14,
};
#endif

//***************************************************************************
// Description: verify if HOST CAN network is correctly configured:
//
// Make sure that can0 us available on host:
//
//     ip link add dev can0 type vcan
//     ip link set can0 up
//
//***************************************************************************

static void test_proto_can_host_env()
{
  dawn::porting::canmsg_s msg;
  int ret;
  int fd;

  // Open CAN device in non-blocking mode

  fd = open(CAN_DEVPATH, O_RDWR | O_NONBLOCK);
  TEST_ASSERT(fd > 0);

  can_drain_fd(g_can_fd);
  can_drain_fd(fd);

  // Network must be in loopback mode

  msg.len = 1;
  msg.id = 1;
  msg.data[0] = 0xff;

  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);

  usleep(100);

  ret = can_read_wait_id(fd, &msg, 1);
  TEST_ASSERT(ret > 0);

  TEST_ASSERT(msg.len == 1);
  TEST_ASSERT(msg.id == 1);
  TEST_ASSERT(msg.data[0] == 0xff);

  // Check other direction

  msg.len = 1;
  msg.id = 1;
  msg.data[0] = 0xff;

  ret = can_send(fd, &msg);
  TEST_ASSERT(ret > 0);

  usleep(100);

  ret = can_read_wait_id(g_can_fd, &msg, 1);
  TEST_ASSERT(ret > 0);

  TEST_ASSERT(msg.len == 1);
  TEST_ASSERT(msg.id == 1);
  TEST_ASSERT(msg.data[0] == 0xff);

  // Close temp file

  close(fd);
}

//***************************************************************************
// Description: push proto runs through start -> hasThread -> stop.
//***************************************************************************

static void test_proto_can_push_lifecycle()
{
  CDescObject descv(g_cfg_notify1);
  CIODummyNotify dummy(descv);
  CDescObject desc(g_bin_can_push_one_bool);
  CProtoCan can(desc);
  CManualNotifier notifier;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  dummy.bindNotifier(&notifier);
  can.setObjectMapItem(CAN_NOTIFYIO1, &dummy);

  TEST_ASSERT_EQUAL(false, can.hasThread());
  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  TEST_ASSERT_EQUAL(true, can.hasThread());
  TEST_ASSERT_EQUAL(OK, can.stop());
  TEST_ASSERT_EQUAL(false, can.hasThread());
}

//***************************************************************************
// Description: pushing a bool triggers a 1-byte CAN frame at the IO's id.
//***************************************************************************

static void test_proto_can_push_bool()
{
  CDescObject descv(g_cfg_notify1);
  CIODummyNotify dummy(descv);
  CDescObject desc(g_bin_can_push_one_bool);
  CProtoCan can(desc);
  CManualNotifier notifier;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  dummy.bindNotifier(&notifier);
  can.setObjectMapItem(CAN_NOTIFYIO1, &dummy);

  // Seed the dummy with the opposite value so trigger-with-false carries
  // observable information.

  can_set_bool(dummy, true);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_queue();

  can_trigger_bool(dummy, notifier, false);
  can_assert_frame(CAN_NODE_ID1 + CAN_NODE_IO1_START + 0, 1, 0);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: pushing a uint8 triggers a 1-byte CAN frame; consecutive
// triggers carry the new value each time.
//***************************************************************************

static void test_proto_can_push_uint8()
{
  CDescObject descv(g_cfg_notify4);
  CIODummyNotify dummy(descv);
  CDescObject desc(g_bin_can_push_one_u8);
  CProtoCan can(desc);
  CManualNotifier notifier;
  uint32_t expected_id = CAN_NODE_ID1 + CAN_NODE_IO1_START;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  dummy.bindNotifier(&notifier);
  can.setObjectMapItem(CAN_NOTIFYIO4, &dummy);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_queue();

  can_trigger_u8(dummy, notifier, 1);
  can_assert_frame(expected_id, 1, 1);

  can_trigger_u8(dummy, notifier, 2);
  can_assert_frame(expected_id, 1, 2);

  can_trigger_u8(dummy, notifier, 3);
  can_assert_frame(expected_id, 1, 3);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: pushing a uint16 triggers a 2-byte CAN frame at the IO's id.
//***************************************************************************

static void test_proto_can_push_uint16()
{
  CDescObject descv(g_cfg_notify5);
  CIODummyNotify dummy(descv);
  CDescObject desc(g_bin_can_push_one_u16);
  CProtoCan can(desc);
  CManualNotifier notifier;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  dummy.bindNotifier(&notifier);
  can.setObjectMapItem(CAN_NOTIFYIO5, &dummy);

  can_set_u16(dummy, 0xffff);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_queue();

  can_trigger_u16(dummy, notifier, 0);
  can_assert_frame(CAN_NODE_ID1 + CAN_NODE_IO1_START, 2, 0);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: pushing a uint32 triggers a 4-byte CAN frame at the IO's id.
//***************************************************************************

static void test_proto_can_push_uint32()
{
  CDescObject descv(g_cfg_notify6);
  CIODummyNotify dummy(descv);
  CDescObject desc(g_bin_can_push_one_u32);
  CProtoCan can(desc);
  CManualNotifier notifier;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  dummy.bindNotifier(&notifier);
  can.setObjectMapItem(CAN_NOTIFYIO6, &dummy);

  can_set_u32(dummy, 0xffffffffu);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_queue();

  can_trigger_u32(dummy, notifier, 0);
  can_assert_frame(CAN_NODE_ID1 + CAN_NODE_IO1_START, 4, 0);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: pushing a uint64 triggers an 8-byte CAN frame at the IO's id.
//***************************************************************************

static void test_proto_can_push_uint64()
{
  CDescObject descv(g_cfg_notify7);
  CIODummyNotify dummy(descv);
  CDescObject desc(g_bin_can_push_one_u64);
  CProtoCan can(desc);
  CManualNotifier notifier;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  dummy.bindNotifier(&notifier);
  can.setObjectMapItem(CAN_NOTIFYIO7, &dummy);

  can_set_u64(dummy, 0xffffffffffffffffULL);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_queue();

  can_trigger_u64(dummy, notifier, 0);
  can_assert_frame(CAN_NODE_ID1 + CAN_NODE_IO1_START, 8, 0);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: pushing three bool IOs each delivers a frame at the per-IO
// offset (verifies in-group routing).
//***************************************************************************

static void test_proto_can_push_three_bools()
{
  CDescObject descv1(g_cfg_notify1);
  CIODummyNotify dummy1(descv1);
  CDescObject descv2(g_cfg_notify2);
  CIODummyNotify dummy2(descv2);
  CDescObject descv3(g_cfg_notify3);
  CIODummyNotify dummy3(descv3);
  CDescObject desc(g_bin_can_push_three_bools);
  CProtoCan can(desc);
  CManualNotifier notifier;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());
  TEST_ASSERT_EQUAL(OK, dummy2.configure());
  TEST_ASSERT_EQUAL(OK, dummy2.init());
  TEST_ASSERT_EQUAL(OK, dummy3.configure());
  TEST_ASSERT_EQUAL(OK, dummy3.init());
  dummy1.bindNotifier(&notifier);
  dummy2.bindNotifier(&notifier);
  dummy3.bindNotifier(&notifier);
  can.setObjectMapItem(CAN_NOTIFYIO1, &dummy1);
  can.setObjectMapItem(CAN_NOTIFYIO2, &dummy2);
  can.setObjectMapItem(CAN_NOTIFYIO3, &dummy3);

  can_set_bool(dummy1, true);
  can_set_bool(dummy2, true);
  can_set_bool(dummy3, true);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_queue();

  can_trigger_bool(dummy1, notifier, false);
  can_assert_frame(CAN_NODE_ID1 + CAN_NODE_IO1_START + 0, 1, 0);

  can_trigger_bool(dummy2, notifier, false);
  can_assert_frame(CAN_NODE_ID1 + CAN_NODE_IO1_START + 1, 1, 0);

  can_trigger_bool(dummy3, notifier, false);
  can_assert_frame(CAN_NODE_ID1 + CAN_NODE_IO1_START + 2, 1, 0);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: push notification, many groups
//***************************************************************************

//***************************************************************************
// Description: with a multi-group push descriptor, IOs in different groups
// land on the right CAN id offsets.  Verifies multi-group routing only;
// per-dtype payload framing is covered by the single-group push tests.
//***************************************************************************

static void test_proto_can_push_multigroup_routing()
{
  CDescObject descv1(g_cfg_notify1);
  CIODummyNotify dummy1(descv1);
  CDescObject descv2(g_cfg_notify2);
  CIODummyNotify dummy2(descv2);
  CDescObject descv3(g_cfg_notify3);
  CIODummyNotify dummy3(descv3);
  CDescObject descv4(g_cfg_notify4);
  CIODummyNotify dummy4(descv4);
  CDescObject descv5(g_cfg_notify5);
  CIODummyNotify dummy5(descv5);
  CDescObject descv6(g_cfg_notify6);
  CIODummyNotify dummy6(descv6);
  CDescObject descv7(g_cfg_notify7);
  CIODummyNotify dummy7(descv7);
  CDescObject desc(g_bin_can_push_many);
  CProtoCan can(desc);
  CManualNotifier notifier;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());
  TEST_ASSERT_EQUAL(OK, dummy2.configure());
  TEST_ASSERT_EQUAL(OK, dummy2.init());
  TEST_ASSERT_EQUAL(OK, dummy3.configure());
  TEST_ASSERT_EQUAL(OK, dummy3.init());
  TEST_ASSERT_EQUAL(OK, dummy4.configure());
  TEST_ASSERT_EQUAL(OK, dummy4.init());
  TEST_ASSERT_EQUAL(OK, dummy5.configure());
  TEST_ASSERT_EQUAL(OK, dummy5.init());
  TEST_ASSERT_EQUAL(OK, dummy6.configure());
  TEST_ASSERT_EQUAL(OK, dummy6.init());
  TEST_ASSERT_EQUAL(OK, dummy7.configure());
  TEST_ASSERT_EQUAL(OK, dummy7.init());
  dummy1.bindNotifier(&notifier);
  dummy2.bindNotifier(&notifier);
  dummy3.bindNotifier(&notifier);
  dummy4.bindNotifier(&notifier);
  dummy5.bindNotifier(&notifier);
  dummy6.bindNotifier(&notifier);
  dummy7.bindNotifier(&notifier);
  can.setObjectMapItem(CAN_NOTIFYIO1, &dummy1);
  can.setObjectMapItem(CAN_NOTIFYIO2, &dummy2);
  can.setObjectMapItem(CAN_NOTIFYIO3, &dummy3);
  can.setObjectMapItem(CAN_NOTIFYIO4, &dummy4);
  can.setObjectMapItem(CAN_NOTIFYIO5, &dummy5);
  can.setObjectMapItem(CAN_NOTIFYIO6, &dummy6);
  can.setObjectMapItem(CAN_NOTIFYIO7, &dummy7);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_queue();

  // Trigger one IO in each group.

  can_trigger_bool(dummy1, notifier, false);
  can_assert_frame(CAN_NODE_ID1 + CAN_NODE_IO1_START + 0, 1, 0);

  can_trigger_u8(dummy4, notifier, 1);
  can_assert_frame(CAN_NODE_ID1 + CAN_NODE_IO2_START + 2, 1, 1);

  can_trigger_u64(dummy7, notifier, 0);
  can_assert_frame(CAN_NODE_ID1 + CAN_NODE_IO3_START + 2, 8, 0);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: read data, many group
//***************************************************************************

//***************************************************************************
// Helper: send an RTR request for the given CAN id, drain the loopback
// echo, and read the response frame.  Asserts the response matches
// expected_id, expected_len, and expected_byte0.
//***************************************************************************

static void can_request_and_assert(uint32_t expected_id,
                                   uint8_t expected_len,
                                   uint8_t expected_byte0)
{
  dawn::porting::canmsg_s msg = {0};
  int ret;

  msg.id = expected_id;
  msg.rtr = 1;
  msg.len = 0;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);

  // Drain RTR echo.

  usleep(100);
  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(0, msg.len);
  TEST_ASSERT_EQUAL(1, msg.rtr);

  // Now the response.

  usleep(100);
  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(expected_id, msg.id);
  TEST_ASSERT_EQUAL(expected_len, msg.len);
  TEST_ASSERT_EQUAL(expected_byte0, msg.data[0]);

  usleep(100);
  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT_EQUAL(-EAGAIN, ret);
}

//***************************************************************************
// Helper: send an RTR for an unbound id and assert only the echo arrives
// (no response frame).
//***************************************************************************

static void can_request_unbound(uint32_t id)
{
  dawn::porting::canmsg_s msg = {0};
  int ret;

  msg.id = id;
  msg.rtr = 1;
  msg.len = 0;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);

  usleep(100);
  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(0, msg.len);
  TEST_ASSERT_EQUAL(1, msg.rtr);

  // No response should follow.

  usleep(100);
  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret < 0);
}

//***************************************************************************
// Description: read_many proto runs through start -> hasThread -> stop and
// returns no spurious frames before the first request.
//***************************************************************************

static void test_proto_can_read_lifecycle()
{
  CDescObject descv(g_cfg_dummy1);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_can_read_one_bool);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  int ret;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  can.setObjectMapItem(CAN_DUMMYIO1, &dummy);

  TEST_ASSERT_EQUAL(false, can.hasThread());
  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  TEST_ASSERT_EQUAL(true, can.hasThread());
  usleep(100000);

  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT_EQUAL(-EAGAIN, ret);

  TEST_ASSERT_EQUAL(OK, can.stop());
  TEST_ASSERT_EQUAL(false, can.hasThread());
}

//***************************************************************************
// Description: an RTR for an id not bound in the descriptor produces only
// the loopback echo, no response frame.
//***************************************************************************

static void test_proto_can_read_unbound_id()
{
  CDescObject descv(g_cfg_dummy1);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_can_read_one_bool);
  CProtoCan can(desc);

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  can.setObjectMapItem(CAN_DUMMYIO1, &dummy);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100000);

  can_request_unbound(CAN_NODE_ID1 + CAN_NODE_IO1_START + 5);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: an RTR for a bool IO returns a 1-byte frame with the IO's
// initial value.
//***************************************************************************

static void test_proto_can_read_bool()
{
  CDescObject descv(g_cfg_dummy1);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_can_read_one_bool);
  CProtoCan can(desc);

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  can.setObjectMapItem(CAN_DUMMYIO1, &dummy);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100000);

  can_request_and_assert(CAN_NODE_ID1 + CAN_NODE_IO1_START, 1, 0);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: an RTR for a uint8 IO returns the current value; updating
// the IO and re-requesting reflects the new value.
//***************************************************************************

static void test_proto_can_read_uint8()
{
  CDescObject descv(g_cfg_dummy4);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_can_read_one_u8);
  CProtoCan can(desc);
  uint32_t expected_id = CAN_NODE_ID1 + CAN_NODE_IO1_START;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  can.setObjectMapItem(CAN_DUMMYIO4, &dummy);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100000);

  can_request_and_assert(expected_id, 1, 0);

  can_set_u8(dummy, 1);
  can_request_and_assert(expected_id, 1, 1);

  can_set_u8(dummy, 2);
  can_request_and_assert(expected_id, 1, 2);

  can_set_u8(dummy, 3);
  can_request_and_assert(expected_id, 1, 3);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: an RTR for a uint16 IO returns a 2-byte frame.
//***************************************************************************

static void test_proto_can_read_uint16()
{
  CDescObject descv(g_cfg_dummy5);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_can_read_one_u16);
  CProtoCan can(desc);

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  can.setObjectMapItem(CAN_DUMMYIO5, &dummy);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100000);

  can_request_and_assert(CAN_NODE_ID1 + CAN_NODE_IO1_START, 2, 0);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: an RTR for a uint32 IO returns a 4-byte frame.
//***************************************************************************

static void test_proto_can_read_uint32()
{
  CDescObject descv(g_cfg_dummy6);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_can_read_one_u32);
  CProtoCan can(desc);

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  can.setObjectMapItem(CAN_DUMMYIO6, &dummy);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100000);

  can_request_and_assert(CAN_NODE_ID1 + CAN_NODE_IO1_START, 4, 0);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: an RTR for a uint64 IO returns an 8-byte frame.
//***************************************************************************

static void test_proto_can_read_uint64()
{
  CDescObject descv(g_cfg_dummy7);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_can_read_one_u64);
  CProtoCan can(desc);

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  can.setObjectMapItem(CAN_DUMMYIO7, &dummy);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100000);

  can_request_and_assert(CAN_NODE_ID1 + CAN_NODE_IO1_START, 8, 0);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: with a multi-group read descriptor, RTRs for IOs in
// different groups produce frames at the right CAN id offsets.  Verifies
// multi-group routing only; per-dtype response framing is covered by the
// single-group read tests.
//***************************************************************************

static void test_proto_can_read_multigroup_routing()
{
  CDescObject descv1(g_cfg_dummy1);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy2);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy3);
  CIODummy dummy3(descv3);
  CDescObject descv4(g_cfg_dummy4);
  CIODummy dummy4(descv4);
  CDescObject descv5(g_cfg_dummy5);
  CIODummy dummy5(descv5);
  CDescObject descv6(g_cfg_dummy6);
  CIODummy dummy6(descv6);
  CDescObject descv7(g_cfg_dummy7);
  CIODummy dummy7(descv7);
  CDescObject desc(g_bin_can_read_many);
  CProtoCan can(desc);

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());
  TEST_ASSERT_EQUAL(OK, dummy2.configure());
  TEST_ASSERT_EQUAL(OK, dummy2.init());
  TEST_ASSERT_EQUAL(OK, dummy3.configure());
  TEST_ASSERT_EQUAL(OK, dummy3.init());
  TEST_ASSERT_EQUAL(OK, dummy4.configure());
  TEST_ASSERT_EQUAL(OK, dummy4.init());
  TEST_ASSERT_EQUAL(OK, dummy5.configure());
  TEST_ASSERT_EQUAL(OK, dummy5.init());
  TEST_ASSERT_EQUAL(OK, dummy6.configure());
  TEST_ASSERT_EQUAL(OK, dummy6.init());
  TEST_ASSERT_EQUAL(OK, dummy7.configure());
  TEST_ASSERT_EQUAL(OK, dummy7.init());
  can.setObjectMapItem(CAN_DUMMYIO1, &dummy1);
  can.setObjectMapItem(CAN_DUMMYIO2, &dummy2);
  can.setObjectMapItem(CAN_DUMMYIO3, &dummy3);
  can.setObjectMapItem(CAN_DUMMYIO4, &dummy4);
  can.setObjectMapItem(CAN_DUMMYIO5, &dummy5);
  can.setObjectMapItem(CAN_DUMMYIO6, &dummy6);
  can.setObjectMapItem(CAN_DUMMYIO7, &dummy7);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100000);

  // Hit one IO in each group.

  can_request_and_assert(CAN_NODE_ID1 + CAN_NODE_IO1_START + 0, 1, 0);
  can_request_and_assert(CAN_NODE_ID1 + CAN_NODE_IO2_START + 0, 1, 0);
  can_request_and_assert(CAN_NODE_ID1 + CAN_NODE_IO3_START + 2, 8, 0);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

#ifdef CONFIG_DAWN_PROTO_CAN_SINGLE_ID
//***************************************************************************
// Helper: configure + init the two IOs used by the single-id descriptors
// (uint32 + uint64) and bind them.  Caller is responsible for can.init()/
// start()/stop().
//***************************************************************************

static void can_setup_single_id(CIODummy &d_u32, CIODummy &d_u64, CProtoCan &can)
{
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, d_u32.configure());
  TEST_ASSERT_EQUAL(OK, d_u32.init());
  TEST_ASSERT_EQUAL(OK, d_u64.configure());
  TEST_ASSERT_EQUAL(OK, d_u64.init());
  can.setObjectMapItem(CAN_DUMMYIO11, &d_u32);
  can.setObjectMapItem(CAN_DUMMYIO14, &d_u64);
}

//***************************************************************************
// Description: indexed read for IO1 (uint32) returns a 2-byte echo header
// followed by a 3-byte data frame.
//***************************************************************************

static void test_proto_can_single_id_read_io1()
{
  CDescObject descv1(g_cfg_dummy11);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy14);
  CIODummy dummy2(descv2);
  CDescObject desc(g_bin_can_read_single_id);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  int ret;

  can_drain_queue();
  can_setup_single_id(dummy1, dummy2, can);
  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_n(100);

  // Request IO1 by index 1.

  msg.id = CAN_NODE_ID1 + CAN_NODE_IO1_START;
  msg.len = 2;
  msg.data[0] = 0x80;
  msg.data[1] = 1;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);

  usleep(100);
  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(CAN_NODE_ID1 + CAN_NODE_IO1_START, msg.id);
  TEST_ASSERT_EQUAL(2, msg.len);
  TEST_ASSERT_EQUAL(0x80, msg.data[0]);
  TEST_ASSERT_EQUAL(1, msg.data[1]);

  usleep(100);
  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(CAN_NODE_ID1 + CAN_NODE_IO1_START, msg.id);
  TEST_ASSERT_EQUAL(3, msg.len);
  TEST_ASSERT_EQUAL(0x80, msg.data[0]);
  TEST_ASSERT_EQUAL(1, msg.data[1]);
  TEST_ASSERT_EQUAL(0, msg.data[2]);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: indexed read for IO2 (uint64) returns a 2-byte echo header
// then a segmented 8 + 4 byte payload.
//***************************************************************************

static void test_proto_can_single_id_read_io2_segmented()
{
  CDescObject descv1(g_cfg_dummy11);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy14);
  CIODummy dummy2(descv2);
  CDescObject desc(g_bin_can_read_single_id);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  int ret;

  can_drain_queue();
  can_setup_single_id(dummy1, dummy2, can);
  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_n(100);

  msg.id = CAN_NODE_ID1 + CAN_NODE_IO1_START;
  msg.len = 2;
  msg.data[0] = 0x80;
  msg.data[1] = 2;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);

  usleep(100);
  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(CAN_NODE_ID1 + CAN_NODE_IO1_START, msg.id);
  TEST_ASSERT_EQUAL(2, msg.len);
  TEST_ASSERT_EQUAL(0x80, msg.data[0]);
  TEST_ASSERT_EQUAL(2, msg.data[1]);

  usleep(100);
  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(CAN_NODE_ID1 + CAN_NODE_IO1_START, msg.id);
  TEST_ASSERT_EQUAL(8, msg.len);
  TEST_ASSERT_EQUAL(0x00, msg.data[0]);
  TEST_ASSERT_EQUAL(2, msg.data[1]);

  usleep(100);
  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(CAN_NODE_ID1 + CAN_NODE_IO1_START, msg.id);
  TEST_ASSERT_EQUAL(4, msg.len);
  TEST_ASSERT_EQUAL(0x81, msg.data[0]);
  TEST_ASSERT_EQUAL(2, msg.data[1]);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: indexed read with index 0 returns frames for both bound IOs
// in sequence.
//***************************************************************************

static void test_proto_can_single_id_read_all()
{
  CDescObject descv1(g_cfg_dummy11);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy14);
  CIODummy dummy2(descv2);
  CDescObject desc(g_bin_can_read_single_id);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  int ret;

  can_drain_queue();
  can_setup_single_id(dummy1, dummy2, can);
  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_n(100);

  msg.id = CAN_NODE_ID1 + CAN_NODE_IO1_START;
  msg.len = 2;
  msg.data[0] = 0x80;
  msg.data[1] = 0;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);

  // Echo header.

  usleep(100);
  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(2, msg.len);
  TEST_ASSERT_EQUAL(0x80, msg.data[0]);
  TEST_ASSERT_EQUAL(0, msg.data[1]);

  // IO1 payload.

  usleep(100);
  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(3, msg.len);
  TEST_ASSERT_EQUAL(0x80, msg.data[0]);
  TEST_ASSERT_EQUAL(1, msg.data[1]);

  // IO2 segmented payload.

  usleep(100);
  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(8, msg.len);
  TEST_ASSERT_EQUAL(0x00, msg.data[0]);
  TEST_ASSERT_EQUAL(2, msg.data[1]);

  usleep(100);
  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(4, msg.len);
  TEST_ASSERT_EQUAL(0x81, msg.data[0]);
  TEST_ASSERT_EQUAL(2, msg.data[1]);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: indexed segmented write to IO2 reassembles the 8-byte
// payload from two CAN frames into the underlying uint64 dummy.
//***************************************************************************

static void test_proto_can_single_id_write_io2_segmented()
{
  CDescObject descv1(g_cfg_dummy11);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy14);
  CIODummy dummy2(descv2);
  CDescObject desc(g_bin_can_write_single_id);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  io_sdata_t<uint64_t, 1> data;
  int ret;

  can_drain_queue();
  can_setup_single_id(dummy1, dummy2, can);
  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_n(100);

  // First segment.

  msg.id = CAN_NODE_ID2 + CAN_NODE_IO1_START;
  msg.len = 8;
  msg.data[0] = 0x00;
  msg.data[1] = 2;
  msg.data[2] = 0x01;
  msg.data[3] = 0x02;
  msg.data[4] = 0x03;
  msg.data[5] = 0x04;
  msg.data[6] = 0x05;
  msg.data[7] = 0x06;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  usleep(10000);

  // Final segment.

  msg.len = 4;
  msg.data[0] = 0x81;
  msg.data[1] = 2;
  msg.data[2] = 0x07;
  msg.data[3] = 0x08;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  usleep(1000000);

  can_drain_n(200);
  usleep(10000);

  TEST_ASSERT_EQUAL(OK, dummy2.getData(data, 1));
  TEST_ASSERT_EQUAL(0x0807060504030201ULL, data(0));

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: indexed write with index 0 reassembles frames for both
// bound IOs (uint32 + uint64).
//***************************************************************************

static void test_proto_can_single_id_write_all()
{
  CDescObject descv1(g_cfg_dummy11);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy14);
  CIODummy dummy2(descv2);
  CDescObject desc(g_bin_can_write_single_id);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  io_sdata_t<uint32_t, 1> data_u32;
  io_sdata_t<uint64_t, 1> data_u64;
  int ret;

  can_drain_queue();
  can_setup_single_id(dummy1, dummy2, can);
  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_n(100);

  msg.id = CAN_NODE_ID2 + CAN_NODE_IO1_START;
  msg.len = 8;
  msg.data[0] = 0x00;
  msg.data[1] = 0x00;
  msg.data[2] = 0x5a;
  msg.data[3] = 0x10;
  msg.data[4] = 0x11;
  msg.data[5] = 0x12;
  msg.data[6] = 0x13;
  msg.data[7] = 0x14;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  usleep(10000);

  msg.len = 5;
  msg.data[0] = 0x81;
  msg.data[1] = 0x00;
  msg.data[2] = 0x15;
  msg.data[3] = 0x16;
  msg.data[4] = 0x17;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  usleep(1000000);

  can_drain_n(200);

  TEST_ASSERT_EQUAL(OK, dummy1.getData(data_u32, 1));
  TEST_ASSERT_EQUAL(0x5a, data_u32(0));

  TEST_ASSERT_EQUAL(OK, dummy2.getData(data_u64, 1));
  TEST_ASSERT_EQUAL(0x1716151413121110ULL, data_u64(0));

  TEST_ASSERT_EQUAL(OK, can.stop());
}

#endif

#ifdef CONFIG_DAWN_PROTO_CAN_SEG
//***************************************************************************
// Description: read data, segmented
//***************************************************************************

static void test_proto_can_read_seg()
{
  CDescObject descv1(g_cfg_dummy14);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_can_read_seg);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  int ret;

  can_drain_queue();

  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());
  can.setObjectMapItem(CAN_DUMMYIO14, &dummy1);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_n(100);

  // data request

  msg.id = CAN_NODE_ID1 + CAN_NODE_IO2_START;
  msg.len = 0;
  msg.rtr = 0;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);

  usleep(500000);

  // Read request echo

  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(CAN_NODE_ID1 + CAN_NODE_IO2_START, msg.id);
  TEST_ASSERT_EQUAL(0, msg.len);
  TEST_ASSERT_EQUAL(0, msg.rtr);

  // Read response segment 0

  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(CAN_NODE_ID1 + CAN_NODE_IO2_START, msg.id);
  TEST_ASSERT_EQUAL(8, msg.len);
  TEST_ASSERT_EQUAL(0x00, msg.data[0]);

  // Read response segment 1 (last)

  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(CAN_NODE_ID1 + CAN_NODE_IO2_START, msg.id);
  TEST_ASSERT_EQUAL(2, msg.len);
  TEST_ASSERT_EQUAL(0x81, msg.data[0]);

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: read seekable data, segmented
//***************************************************************************

static void test_proto_can_read_seg_seek()
{
  CDevDescriptor *inst = CDevDescriptor::getInst();
  CDevDescriptor::SDescriptorReg reg;
  CDescObject descio(g_cfg_descio1);
  CIODescriptor desc_io(descio);
  CDescObject desc(g_bin_can_read_seg_seek);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  int ret;

  can_drain_queue();

  reg.ptr = (void *)g_dawn_desc_seek1;
  reg.len = sizeof(g_dawn_desc_seek1);
  ret = inst->regDescriptor(0, reg);
  TEST_ASSERT_EQUAL(OK, ret);

  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, desc_io.configure());
  TEST_ASSERT_EQUAL(OK, desc_io.init());

  can.setObjectMapItem(CAN_DESCIO1, &desc_io);
  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_n(100);

  // Read request

  msg.id = CAN_NODE_ID1 + CAN_NODE_IO3_START;
  msg.rtr = 0;
  msg.len = 0;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);

  usleep(100000);

  // Read request echo

  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(CAN_NODE_ID1 + CAN_NODE_IO3_START, msg.id);
  TEST_ASSERT_EQUAL(0, msg.len);
  TEST_ASSERT_EQUAL(0, msg.rtr);

  // Segment 0

  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(8, msg.len);
  TEST_ASSERT_EQUAL(0x00, msg.data[0]);
  TEST_ASSERT_EQUAL(0x01, msg.data[1]);
  TEST_ASSERT_EQUAL(0x00, msg.data[2]);
  TEST_ASSERT_EQUAL(0x00, msg.data[3]);
  TEST_ASSERT_EQUAL(0x00, msg.data[4]);
  TEST_ASSERT_EQUAL(0x02, msg.data[5]);
  TEST_ASSERT_EQUAL(0x00, msg.data[6]);
  TEST_ASSERT_EQUAL(0x00, msg.data[7]);

  // Segment 1

  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(8, msg.len);
  TEST_ASSERT_EQUAL(0x01, msg.data[0]);
  TEST_ASSERT_EQUAL(0x00, msg.data[1]);
  TEST_ASSERT_EQUAL(0x03, msg.data[2]);
  TEST_ASSERT_EQUAL(0x00, msg.data[3]);
  TEST_ASSERT_EQUAL(0x00, msg.data[4]);
  TEST_ASSERT_EQUAL(0x00, msg.data[5]);
  TEST_ASSERT_EQUAL(0x04, msg.data[6]);
  TEST_ASSERT_EQUAL(0x00, msg.data[7]);

  // Segment 2 (last)

  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(7, msg.len);
  TEST_ASSERT_EQUAL(0x82, msg.data[0]);
  TEST_ASSERT_EQUAL(0x00, msg.data[1]);
  TEST_ASSERT_EQUAL(0x00, msg.data[2]);
  TEST_ASSERT_EQUAL(0x05, msg.data[3]);
  TEST_ASSERT_EQUAL(0x00, msg.data[4]);
  TEST_ASSERT_EQUAL(0x00, msg.data[5]);
  TEST_ASSERT_EQUAL(0x00, msg.data[6]);

  ret = can.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(false, can.hasThread());

  CDevDescriptor::destroy();
}

//***************************************************************************
// Description: write data, segmented
//***************************************************************************

static void test_proto_can_write_seg()
{
  CDescObject descv1(g_cfg_dummy14);
  CIODummy dummy1(descv1);
  CDescObject desc(g_bin_can_write_seg);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  io_sdata_t<uint64_t, 1> datau64;
  int ret;

  can_drain_queue();

  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());
  can.setObjectMapItem(CAN_DUMMYIO14, &dummy1);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_n(100);

  // Segmented write (ISO-TP format)
  // Total: 8 bytes (0x0102030405060708)

  // First Frame: 0x10 (FF) + 0x08 (length) + 6 bytes data

  msg.id = CAN_NODE_ID2 + CAN_NODE_IO2_START;
  msg.len = 8;
  msg.data[0] = 0x10; // ISO-TP First Frame
  msg.data[1] = 0x08; // Total length = 8 bytes
  msg.data[2] = 0x01;
  msg.data[3] = 0x02;
  msg.data[4] = 0x03;
  msg.data[5] = 0x04;
  msg.data[6] = 0x05;
  msg.data[7] = 0x06;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);

  usleep(10000);

  // Consecutive Frame: 0x20 (CF, seq=0) + 2 bytes data

  msg.len = 3;
  msg.data[0] = 0x20; // ISO-TP Consecutive Frame, seq=0
  msg.data[1] = 0x07;
  msg.data[2] = 0x08;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);

  usleep(1000000);

  can_drain_n(200);

  TEST_ASSERT_EQUAL(OK, dummy1.getData(datau64, 1));
  TEST_ASSERT_EQUAL(0x0807060504030201ULL, datau64(0));

  TEST_ASSERT_EQUAL(OK, can.stop());
}
#endif

//***************************************************************************
// Description: write proto runs through start -> hasThread -> stop.
//***************************************************************************

static void test_proto_can_write_lifecycle()
{
  CDescObject descv(g_cfg_dummy11);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_can_write_one_u8);
  CProtoCan can(desc);

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  can.setObjectMapItem(CAN_DUMMYIO11, &dummy);

  TEST_ASSERT_EQUAL(false, can.hasThread());
  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  TEST_ASSERT_EQUAL(true, can.hasThread());
  TEST_ASSERT_EQUAL(OK, can.stop());
  TEST_ASSERT_EQUAL(false, can.hasThread());
}

//***************************************************************************
// Description: writing CAN frames to three consecutive bool IOs delivers
// the supplied bit value to each underlying dummy.
//***************************************************************************

static void test_proto_can_write_bools()
{
  CDescObject descv1(g_cfg_dummy8);
  CIODummy dummy1(descv1);
  CDescObject descv2(g_cfg_dummy9);
  CIODummy dummy2(descv2);
  CDescObject descv3(g_cfg_dummy10);
  CIODummy dummy3(descv3);
  CDescObject desc(g_bin_can_write_three_bools);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  io_sdata_t<bool, 1> data;
  int ret;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.configure());
  TEST_ASSERT_EQUAL(OK, dummy1.init());
  TEST_ASSERT_EQUAL(OK, dummy2.configure());
  TEST_ASSERT_EQUAL(OK, dummy2.init());
  TEST_ASSERT_EQUAL(OK, dummy3.configure());
  TEST_ASSERT_EQUAL(OK, dummy3.init());
  can.setObjectMapItem(CAN_DUMMYIO8, &dummy1);
  can.setObjectMapItem(CAN_DUMMYIO9, &dummy2);
  can.setObjectMapItem(CAN_DUMMYIO10, &dummy3);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_n(100);

  msg.id = CAN_NODE_ID2 + CAN_NODE_IO1_START + 0;
  msg.len = 1;
  msg.data[0] = 0;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  usleep(100);

  msg.id = CAN_NODE_ID2 + CAN_NODE_IO1_START + 1;
  msg.len = 1;
  msg.data[0] = 0x1;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  usleep(100);

  msg.id = CAN_NODE_ID2 + CAN_NODE_IO1_START + 2;
  msg.len = 1;
  msg.data[0] = 0x1;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  usleep(500000);

  can_drain_n(200);

  TEST_ASSERT_EQUAL(OK, dummy1.getData(data, 1));
  TEST_ASSERT_EQUAL(false, data(0));
  TEST_ASSERT_EQUAL(OK, dummy2.getData(data, 1));
  TEST_ASSERT_EQUAL(true, data(0));
  TEST_ASSERT_EQUAL(OK, dummy3.getData(data, 1));
  TEST_ASSERT_EQUAL(true, data(0));

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: writing a 1-byte CAN frame to the uint8 IO updates the
// dummy and the bus echoes the frame.
//***************************************************************************

static void test_proto_can_write_uint8()
{
  CDescObject descv(g_cfg_dummy11);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_can_write_one_u8);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  io_sdata_t<uint8_t, 1> data;
  int ret;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  can.setObjectMapItem(CAN_DUMMYIO11, &dummy);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_n(100);

  msg.id = CAN_NODE_ID2 + CAN_NODE_IO1_START + 3;
  msg.len = 1;
  msg.data[0] = 0xff;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  usleep(100);

  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(CAN_NODE_ID2 + CAN_NODE_IO1_START + 3, msg.id);
  TEST_ASSERT_EQUAL(1, msg.len);
  TEST_ASSERT_EQUAL(0xff, msg.data[0]);

  usleep(10000);

  TEST_ASSERT_EQUAL(OK, dummy.getData(data, 1));
  TEST_ASSERT_EQUAL(0xff, data(0));

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: writing a 2-byte CAN frame to the uint16 IO updates the
// dummy with the little-endian decoded value.
//***************************************************************************

static void test_proto_can_write_uint16()
{
  CDescObject descv(g_cfg_dummy12);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_can_write_one_u16);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  io_sdata_t<uint16_t, 1> data;
  int ret;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  can.setObjectMapItem(CAN_DUMMYIO12, &dummy);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_n(100);

  msg.id = CAN_NODE_ID2 + CAN_NODE_IO1_START + 4;
  msg.len = 2;
  msg.data[0] = 0xad;
  msg.data[1] = 0xde;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  usleep(100);

  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(CAN_NODE_ID2 + CAN_NODE_IO1_START + 4, msg.id);
  TEST_ASSERT_EQUAL(2, msg.len);
  TEST_ASSERT_EQUAL(0xad, msg.data[0]);
  TEST_ASSERT_EQUAL(0xde, msg.data[1]);

  usleep(10000);

  TEST_ASSERT_EQUAL(OK, dummy.getData(data, 1));
  TEST_ASSERT_EQUAL(0xdead, data(0));

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: writing a 4-byte CAN frame to the uint32 IO updates the
// dummy with the little-endian decoded value.
//***************************************************************************

static void test_proto_can_write_uint32()
{
  CDescObject descv(g_cfg_dummy13);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_can_write_one_u32);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  io_sdata_t<uint32_t, 1> data;
  int ret;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  can.setObjectMapItem(CAN_DUMMYIO13, &dummy);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_n(100);

  msg.id = CAN_NODE_ID2 + CAN_NODE_IO1_START + 5;
  msg.len = 4;
  msg.data[0] = 0xde;
  msg.data[1] = 0xad;
  msg.data[2] = 0xbe;
  msg.data[3] = 0xef;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  usleep(100);

  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(CAN_NODE_ID2 + CAN_NODE_IO1_START + 5, msg.id);
  TEST_ASSERT_EQUAL(4, msg.len);

  usleep(10000);

  TEST_ASSERT_EQUAL(OK, dummy.getData(data, 1));
  TEST_ASSERT_EQUAL(0xefbeadde, data(0));

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: writing an 8-byte CAN frame to the uint64 IO updates the
// dummy with the full 64-bit value.
//***************************************************************************

static void test_proto_can_write_uint64()
{
  CDescObject descv(g_cfg_dummy14);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_can_write_one_u64);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  io_sdata_t<uint64_t, 1> data;
  int ret;
  int i;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  can.setObjectMapItem(CAN_DUMMYIO14, &dummy);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_n(100);

  msg.id = CAN_NODE_ID2 + CAN_NODE_IO1_START + 6;
  msg.len = 8;
  for (i = 0; i < 8; i++)
    {
      msg.data[i] = 0xff;
    }
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  usleep(100);

  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(CAN_NODE_ID2 + CAN_NODE_IO1_START + 6, msg.id);
  TEST_ASSERT_EQUAL(8, msg.len);

  usleep(10000);

  TEST_ASSERT_EQUAL(OK, dummy.getData(data, 1));
  TEST_ASSERT_EQUAL(0xffffffffffffffffULL, data(0));

  TEST_ASSERT_EQUAL(OK, can.stop());
}

//***************************************************************************
// Description: writing an 8-byte CAN frame to a two-element uint32 IO uses
// the IO total byte size, not byte size multiplied by dimension.
//***************************************************************************

static void test_proto_can_write_u32_vector_uses_total_size()
{
  CDescObject descv(g_cfg_dummy15);
  CIODummy dummy(descv);
  CDescObject desc(g_bin_can_write_one_u32_vec2);
  CProtoCan can(desc);
  dawn::porting::canmsg_s msg = {0};
  io_sdata_t<uint32_t, 2> data;
  int ret;

  can_drain_queue();
  TEST_ASSERT_EQUAL(OK, can.configure());
  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  TEST_ASSERT_EQUAL(8, dummy.getDataSize());
  TEST_ASSERT_EQUAL(2, dummy.getDataDim());
  can.setObjectMapItem(CAN_DUMMYIO15, &dummy);

  TEST_ASSERT_EQUAL(OK, can.init());
  TEST_ASSERT_EQUAL(OK, can.start());
  usleep(100);
  can_drain_n(100);

  msg.id = CAN_NODE_ID2 + CAN_NODE_IO1_START + 7;
  msg.len = 8;
  msg.data[0] = 0x44;
  msg.data[1] = 0x33;
  msg.data[2] = 0x22;
  msg.data[3] = 0x11;
  msg.data[4] = 0xdd;
  msg.data[5] = 0xcc;
  msg.data[6] = 0xbb;
  msg.data[7] = 0xaa;
  ret = can_send(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  usleep(100);

  ret = can_read(g_can_fd, &msg);
  TEST_ASSERT(ret > 0);
  TEST_ASSERT_EQUAL(CAN_NODE_ID2 + CAN_NODE_IO1_START + 7, msg.id);
  TEST_ASSERT_EQUAL(8, msg.len);

  usleep(10000);

  TEST_ASSERT_EQUAL(OK, dummy.getData(data, 1));
  TEST_ASSERT_EQUAL(0x11223344, data(0));
  TEST_ASSERT_EQUAL(0xaabbccdd, data(1));

  TEST_ASSERT_EQUAL(OK, can.stop());
}

extern "C"
{
  int test_proto_can()
  {
    UNITY_BEGIN();

    // Open CAN device in non-blocking mode

    g_can_fd = open(CAN_DEVPATH, O_RDWR | O_NONBLOCK);
    TEST_ASSERT(g_can_fd > 0);

    // Run tests

    DAWN_RUN_TEST(test_proto_can_host_env);

    DAWN_RUN_TEST(test_proto_can_push_lifecycle);
    DAWN_RUN_TEST(test_proto_can_push_bool);
    DAWN_RUN_TEST(test_proto_can_push_uint8);
    DAWN_RUN_TEST(test_proto_can_push_uint16);
    DAWN_RUN_TEST(test_proto_can_push_uint32);
    DAWN_RUN_TEST(test_proto_can_push_uint64);
    DAWN_RUN_TEST(test_proto_can_push_three_bools);

    DAWN_RUN_TEST(test_proto_can_push_multigroup_routing);

    DAWN_RUN_TEST(test_proto_can_read_lifecycle);
    DAWN_RUN_TEST(test_proto_can_read_unbound_id);
    DAWN_RUN_TEST(test_proto_can_read_bool);
    DAWN_RUN_TEST(test_proto_can_read_uint8);
    DAWN_RUN_TEST(test_proto_can_read_uint16);
    DAWN_RUN_TEST(test_proto_can_read_uint32);
    DAWN_RUN_TEST(test_proto_can_read_uint64);
    DAWN_RUN_TEST(test_proto_can_read_multigroup_routing);
#ifdef CONFIG_DAWN_PROTO_CAN_SINGLE_ID
    DAWN_RUN_TEST(test_proto_can_single_id_read_io1);
    DAWN_RUN_TEST(test_proto_can_single_id_read_io2_segmented);
    DAWN_RUN_TEST(test_proto_can_single_id_read_all);
    DAWN_RUN_TEST(test_proto_can_single_id_write_io2_segmented);
    DAWN_RUN_TEST(test_proto_can_single_id_write_all);
#endif
#ifdef CONFIG_DAWN_PROTO_CAN_SEG
    DAWN_RUN_TEST(test_proto_can_read_seg);
    DAWN_RUN_TEST(test_proto_can_read_seg_seek);
    DAWN_RUN_TEST(test_proto_can_write_seg);
#endif
    DAWN_RUN_TEST(test_proto_can_write_lifecycle);
    DAWN_RUN_TEST(test_proto_can_write_bools);
    DAWN_RUN_TEST(test_proto_can_write_uint8);
    DAWN_RUN_TEST(test_proto_can_write_uint16);
    DAWN_RUN_TEST(test_proto_can_write_uint32);
    DAWN_RUN_TEST(test_proto_can_write_uint64);
    DAWN_RUN_TEST(test_proto_can_write_u32_vector_uses_total_size);

    // Close CAN device

    close(g_can_fd);

    return UNITY_END();
  }
}
