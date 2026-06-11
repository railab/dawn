// dawn/src/system/gnss.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/system/gnss.hxx"

#include <cerrno>
#include <cstdio>
#include <unistd.h>

#include <nuttx/uorb.h>

#include "dawn/debug.hxx"
#include "dawn/porting/lte.hxx"
#include "dawn/porting/sensors.hxx"

using namespace dawn;

// Fix-monitor poll period. The GNSS device is non-blocking, so the loop polls
// for a fix and stays responsive to stop requests.

#define GNSS_MONITOR_POLL_US (1000000)

// Coexistence timing (seconds). LTE is the priority: the manager only ever
// borrows the radio for a bounded window and always hands it back, so the
// device stays usable on LTE even when GNSS never fixes (e.g. indoors).

#define GNSS_SETTLE_S    CONFIG_DAWN_SYSTEM_GNSS_SETTLE
#define GNSS_ACQUIRE_S   CONFIG_DAWN_SYSTEM_GNSS_ACQUIRE
#define GNSS_RETRY_S     CONFIG_DAWN_SYSTEM_GNSS_RETRY
#define GNSS_MAXATTEMPTS CONFIG_DAWN_SYSTEM_GNSS_MAXATTEMPTS
#define GNSS_REARM_S     CONFIG_DAWN_SYSTEM_GNSS_REARM

// Default runtime on/off state. GNSS starts disabled (LTE only) unless either
// Kconfig or the descriptor turns it on; a server can flip it at runtime over
// LwM2M (IPSO Actuation 3306/5850 -> the 'enabled' config item).

#ifdef CONFIG_DAWN_SYSTEM_GNSS_ENABLED
#  define GNSS_ENABLED_DEFAULT (true)
#else
#  define GNSS_ENABLED_DEFAULT (false)
#endif

CSystemGnss::CSystemGnss(CDescObject &desc)
  : CSystemCommon(desc)
  , mode(0)
  , enabled(false)
  , settle(GNSS_SETTLE_S)
  , acquire(GNSS_ACQUIRE_S)
  , retry(GNSS_RETRY_S)
  , maxAttempts(GNSS_MAXATTEMPTS)
  , rearm(GNSS_REARM_S)
  , fd(-1)
{
}

void CSystemGnss::loadParams()
{
  CDescObject &desc = getDesc();
  size_t count = desc.getSize();
  size_t offset = 0;
  size_t i;
  SObjectCfg::SObjectCfgItem *item;

  // Kconfig defaults

  mode = CONFIG_DAWN_SYSTEM_GNSS_MODE;
  enabled = GNSS_ENABLED_DEFAULT;
  settle = GNSS_SETTLE_S;
  acquire = GNSS_ACQUIRE_S;
  retry = GNSS_RETRY_S;
  maxAttempts = GNSS_MAXATTEMPTS;
  rearm = GNSS_REARM_S;

  // Override from descriptor config items

  for (i = 0; i < count; i++)
    {
      item = desc.objectCfgItemNext(offset);
      if (item == nullptr)
        {
          break;
        }

      if (item->cfgid.s.cls != SYSTEM_CLASS_GNSS)
        {
          continue;
        }

      switch (item->cfgid.s.id)
        {
          case GNSS_CFG_MODE:
            mode = static_cast<uint8_t>(item->data[0] & 0xff);
            break;

          case GNSS_CFG_SETTLE:
            settle = item->data[0];
            break;

          case GNSS_CFG_ACQUIRE:
            acquire = item->data[0];
            break;

          case GNSS_CFG_RETRY:
            retry = item->data[0];
            break;

          case GNSS_CFG_MAXATTEMPTS:
            maxAttempts = item->data[0];
            break;

          case GNSS_CFG_REARM:
            rearm = item->data[0];
            break;

          case GNSS_CFG_ENABLED:
            enabled = (item->data[0] != 0);
            break;

          default:
            break;
        }
    }
}

int CSystemGnss::configure()
{
  loadParams();
  return OK;
}

void CSystemGnss::monitor()
{
  struct sensor_gnss gnss;
  int n;

  // Activate this subscription so the GNSS fix events are delivered to our
  // read() (the device only pushes a sample on a valid fix).

  sensor_set_interval(fd, GNSS_MONITOR_POLL_US);

  // Wait up to 'timeout' seconds for LTE to be CONNECTED (true) or give up
  // (false). Used both for the initial LTE-first deferral and to let LTE
  // recover after each GNSS window before counting any cooldown.

  auto waitLte = [this](uint32_t timeout)
    {
      uint32_t status;
      for (uint32_t t = 0; t < timeout && !shouldQuit(); t++)
        {
          if (lte_port_status(&status) == OK && status == DAWN_LTE_STATUS_CONNECTED)
            {
              return true;
            }

          sleep(1);
        }

      return false;
    };

  // Interruptible LTE-reachable wait (no PSM, no GNSS priority).

  auto idle = [this](uint32_t secs)
    {
      for (uint32_t t = 0; t < secs && !shouldQuit(); t++)
        {
          sleep(1);
        }
    };

  // LTE first. Do not disturb the radio until LTE is connected and reachable;
  // GNSS is deferred behind it. Indoors this is the whole point - the device
  // stays usable on LTE even if a fix never comes.

  waitLte(settle);

  // Arming loop. Each arming makes up to 'maxAttempts' bounded acquisition
  // windows (maxAttempts == 0 => unlimited, i.e. retry forever - the
  // registration-safe default, which relies on the LwM2M lifetime exceeding
  // acquire+retry). A cold fix needs the radio (LTE asleep in PSM + GNSS
  // priority), so each window is time-boxed and ALWAYS followed by handing
  // the radio back to LTE and waiting for it to recover - LTE availability is
  // never sacrificed to GNSS. If a round gives up without a fix, the manager
  // parks fully on LTE for 'rearm' seconds and then arms again. A fix ends the
  // policy entirely (the modem keeps tracking warm).

  bool fixed = false;

  while (!shouldQuit() && !fixed)
    {
      uint32_t attempt = 0;

      while (!shouldQuit() && (maxAttempts == 0 || attempt < maxAttempts))
        {
          attempt++;

          // Bounded acquisition window: LTE sleeps, GNSS owns the radio.

          lte_port_set_psave(DAWN_LTE_PSAVE_PSM);
          sensor_gnss_set_priority(fd, true);
          DAWNINFO("GNSS manager: acquiring (bounded %lus, attempt %lu)\n",
                   (unsigned long)acquire,
                   (unsigned long)attempt);

          for (uint32_t t = 0; t < acquire && !shouldQuit(); t++)
            {
              n = sensor_read(fd, &gnss, sizeof(gnss));
              if (n > 0)
                {
                  fixed = true;
                  break;
                }

              sleep(1);
            }

          // Always yield the radio back to LTE, then wait for LTE to recover
          // before counting any cooldown - this keeps the registration alive.

          sensor_gnss_set_priority(fd, false);
          lte_port_set_psave(DAWN_LTE_PSAVE_NONE);
          waitLte(settle);

          if (fixed)
            {
              DAWNINFO("GNSS manager: fix acquired (lat=%f); LTE restored\n",
                       static_cast<double>(gnss.latitude));
              break;
            }

          // Cool down (LTE reachable) before the next window, unless this was
          // the last allowed attempt (then 'rearm' below takes over).

          if (maxAttempts == 0 || attempt < maxAttempts)
            {
              DAWNINFO("GNSS manager: no fix in %lus; LTE reachable, "
                       "retry in %lus\n",
                       (unsigned long)acquire,
                       (unsigned long)retry);
              idle(retry);
            }
        }

      if (fixed || shouldQuit())
        {
          break;
        }

      // Gave up after maxAttempts windows: park on LTE and re-arm later.

      DAWNINFO("GNSS manager: gave up after %lu attempts; LTE only, "
               "re-arm in %lus\n",
               (unsigned long)attempt,
               (unsigned long)rearm);
      idle(rearm);
    }

  // Leave the radio reachable on stop.

  sensor_gnss_set_priority(fd, false);
  lte_port_set_psave(DAWN_LTE_PSAVE_NONE);
}

int CSystemGnss::startMonitor()
{
  char path[32];

  if (isRunning())
    {
      return OK;
    }

  std::snprintf(path, sizeof(path), "/dev/uorb/sensor_gnss0");

  fd = sensor_open(path);
  if (fd < 0)
    {
      DAWNERR("GNSS manager: open %s failed %d\n", path, fd);
      return fd;
    }

  DAWNINFO("GNSS manager: enabled, starting acquisition\n");
  return startWorkerThread([this]() { monitor(); });
}

void CSystemGnss::stopMonitor()
{
  if (isRunning())
    {
      stopWorkerThread();
    }

  if (fd >= 0)
    {
      sensor_close(fd);
      fd = -1;
    }
}

int CSystemGnss::doStart()
{
  loadParams();

  // mode OFF disables the policy entirely (the manager is inert and cannot be
  // turned on at runtime). Otherwise honour the runtime on/off switch: GNSS is
  // deferred until 'enabled' (default off => LTE only), which a server can flip
  // live over LwM2M.

  if (mode == DAWN_GNSS_MODE_OFF)
    {
      DAWNINFO("GNSS manager: off (mode)\n");
      return OK;
    }

  if (!enabled)
    {
      DAWNINFO("GNSS manager: idle (disabled); enable over LwM2M to start\n");
      return OK;
    }

  return startMonitor();
}

int CSystemGnss::doStop()
{
  stopMonitor();
  return OK;
}

int CSystemGnss::onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len)
{
  // Runtime control hook: invoked on every config write before the value is
  // stored. The 'enabled' switch (IPSO Actuation 3306/5850 over LwM2M) starts
  // or stops the monitor live; mode OFF still wins. Other params take effect on
  // the next start.

  if (SObjectCfg::objectCfgGetCls(objcfg) != SYSTEM_CLASS_GNSS ||
      SObjectCfg::objectCfgGetId(objcfg) != GNSS_CFG_ENABLED || len < 1)
    {
      return OK;
    }

  enabled = (data[0] != 0);

  if (enabled && mode != DAWN_GNSS_MODE_OFF)
    {
      return startMonitor();
    }

  DAWNINFO("GNSS manager: disabled over LwM2M; LTE only\n");
  stopMonitor();
  return OK;
}
