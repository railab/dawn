// dawn/src/dawn.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/dawn.hxx"
#include "dawn/oot.hxx"
#ifdef CONFIG_DAWN_DESC_SWITCH
#  include "dawn/dev/descriptor.hxx"
#  include "dawn/dev/descswitch.hxx"
#endif
#ifdef CONFIG_DAWN_LIFECYCLE_TEARDOWN
#  include "dawn/dev/shutdown.hxx"
#endif

using namespace dawn;

CDawn::CDawn(IIOFactory *iofactory, IProgFactory *progfactory, IProtoFactory *protofactory)
  : userIOFactory(iofactory)
  , userProgFactory(progfactory)
  , userProtoFactory(protofactory)
{
}

int CDawn::load_descriptor(uint32_t *bin, size_t len)
{
  int ret = OK;

  DAWNASSERT(bin != nullptr && len > 0, "invalid input");

  // Load binary data

  ret = desc.loadBin(bin, len, false);
  if (ret < 0)
    {
      DAWNERR("ERROR: _desc.loadBin failed %d\n", ret);
      return ret;
    }

  // Initialize IO handler

  ret = io.init(desc, userIOFactory);
  if (ret < 0)
    {
      DAWNERR("failed to initialize IO handler\n");
      return ret;
    }

  // Initialize IOs

  ret = io.initAll();
  if (ret < 0)
    {
      DAWNERR("failed to initialize IOs\n");
      return ret;
    }

  // Initialize PROG handler

  ret = prog.init(desc, &io, userProgFactory);
  if (ret < 0)
    {
      DAWNERR("failed to initialize PROG handler\n");
      return ret;
    }

  // Initialize PROGs

  ret = prog.initAll();
  if (ret < 0)
    {
      DAWNERR("failed to initialize PROGs\n");
      return ret;
    }

  // Initialize PROTO handler

  ret = proto.init(desc, &io, userProtoFactory);
  if (ret < 0)
    {
      DAWNERR("failed to initialize PROTO handler\n");
      return ret;
    }

  // Initialize PROTOs

  ret = proto.initAll();
  if (ret < 0)
    {
      DAWNERR("failed to initialize PROTOs\n");
      return ret;
    }

  // Initialize SYSTEM handler

  ret = system.init(desc, nullptr);
  if (ret < 0)
    {
      DAWNERR("failed to initialize SYSTEM handler\n");
      return ret;
    }

  // Initialize SYSTEM objects

  ret = system.initAll();
  if (ret < 0)
    {
      DAWNERR("failed to initialize SYSTEM objects\n");
      return ret;
    }

  // Bind special IO objects (Config, Control, Trigger) to their targets

  ret = io.bindObjects(io, prog, proto, system);
  if (ret < 0)
    {
      DAWNERR("failed to bind special IO objects\n");
      return ret;
    }

  return OK;
}

#ifdef CONFIG_DAWN_DESC_SWITCH
int CDawn::load_descriptor_from_slot(uint8_t slot)
{
  CDevDescriptor::SDescriptorReg reg;
  int ret;

  ret = CDevDescriptor::getInst()->getDescriptor(slot, reg);
  if (ret < 0)
    {
      return ret;
    }

  if (reg.ptr == nullptr || reg.len == 0)
    {
      return -ENODATA;
    }

  return load_descriptor(static_cast<uint32_t *>(reg.ptr), reg.len);
}
#endif

int CDawn::start(bool no_loop)
{
  int ret = OK;
#ifdef CONFIG_DAWN_LIFECYCLE_TEARDOWN
  int tmp;
#endif
#ifdef CONFIG_DAWN_DESC_SWITCH
  uint8_t newSlot;

  CDescSwitch::clear();
  CDescSwitch::setActiveSlot(0);
#endif

  for (;;)
    {
#ifdef CONFIG_DAWN_LIFECYCLE_TEARDOWN
      CShutdown::clear();
#endif
      // Start IO

      ret = io.startAll();
      if (ret < 0)
        {
#ifdef CONFIG_DAWN_LIFECYCLE_TEARDOWN
          goto deinit_all;
#else
          return ret;
#endif
        }

      // Start SYSTEM objects (e.g. bring up LTE before protocols need the network)

      ret = system.startAll();
      if (ret < 0)
        {
#ifdef CONFIG_DAWN_LIFECYCLE_TEARDOWN
          goto stop_io;
#else
          return ret;
#endif
        }

      // Start programs

      ret = prog.startAll();
      if (ret < 0)
        {
#ifdef CONFIG_DAWN_LIFECYCLE_TEARDOWN
          goto stop_sys;
#else
          return ret;
#endif
        }

      // Start protocols

      ret = proto.startAll();
      if (ret < 0)
        {
#ifdef CONFIG_DAWN_LIFECYCLE_TEARDOWN
          goto stop_prog;
#else
          return ret;
#endif
        }

      DAWNINFO("DAWN: START SUCCESS!\n");

      while (!no_loop)
        {
#ifdef CONFIG_DAWN_LIFECYCLE_TEARDOWN
          if (CShutdown::isRequested())
            {
              DAWNINFO("shutdown requested - quit\n");
              break;
            }
#endif

#ifdef CONFIG_DAWN_DESC_SWITCH
          if (CDescSwitch::isSwitchRequested())
            {
              DAWNINFO("descriptor switch requested\n");
              break;
            }
#endif

          if (!desc.getNoIdleQuit() && !(proto.hasThread() || prog.hasThread()))
            {
              DAWNINFO("no active protocols - quit\n");
              break;
            }

          // Idle: protocols and progs run their own threads; the main loop
          // hands the wait to the user idle hook (default is sleep(1)).
          // The hook owns cadence and must return promptly so the
          // shutdown / descriptor-switch / no-thread checks above run
          // each iteration.

          dawn::oot::user_on_idle(this);
        }

#ifdef CONFIG_DAWN_LIFECYCLE_TEARDOWN
      // User-supplied pre-shutdown hook (weak default is a no-op).
      // A negative return is logged but does not abort teardown.

      tmp = dawn::oot::user_pre_shutdown(this);
      if (tmp < 0)
        {
          DAWNWARN("user_pre_shutdown returned %d\n", tmp);
        }

      tmp = proto.stopAll();
      if (tmp < 0)
        {
          DAWNERR("failed to stop protos %d\n", tmp);
          if (ret == OK)
            {
              ret = tmp;
            }
        }

    stop_prog:
      tmp = prog.stopAll();
      if (tmp < 0)
        {
          DAWNERR("failed to stop programs %d\n", tmp);
          if (ret == OK)
            {
              ret = tmp;
            }
        }

    stop_sys:
      tmp = system.stopAll();
      if (tmp < 0)
        {
          DAWNERR("failed to stop SYSTEM objects %d\n", tmp);
          if (ret == OK)
            {
              ret = tmp;
            }
        }

    stop_io:
      tmp = io.stopAll();
      if (tmp < 0)
        {
          DAWNERR("failed to stop IO %d\n", tmp);
          if (ret == OK)
            {
              ret = tmp;
            }
        }

    deinit_all:
      tmp = proto.deinitAll();
      if (tmp < 0)
        {
          DAWNERR("failed to deinit protos %d\n", tmp);
          if (ret == OK)
            {
              ret = tmp;
            }
        }

      tmp = prog.deinitAll();
      if (tmp < 0)
        {
          DAWNERR("failed to deinit programs %d\n", tmp);
          if (ret == OK)
            {
              ret = tmp;
            }
        }

      tmp = system.deinitAll();
      if (tmp < 0)
        {
          DAWNERR("failed to deinit SYSTEM objects %d\n", tmp);
          if (ret == OK)
            {
              ret = tmp;
            }
        }

      tmp = io.deinitAll();
      if (tmp < 0)
        {
          DAWNERR("failed to deinit IO %d\n", tmp);
          if (ret == OK)
            {
              ret = tmp;
            }
        }
#else
      return ret;
#endif

#ifdef CONFIG_DAWN_DESC_SWITCH
      if (CDescSwitch::isSwitchRequested())
        {
          newSlot = CDescSwitch::getSwitchSlot();
          CDescSwitch::setActiveSlot(newSlot);
          CDescSwitch::clear();

          ret = load_descriptor_from_slot(newSlot);
          if (ret < 0)
            {
              DAWNERR("descriptor switch failed for slot %d\n", newSlot);
              return ret;
            }

          continue;
        }
#endif

      return ret;
    }
}
