// dawn/src/system/lte.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/system/lte.hxx"

#include <cerrno>
#include <cstring>

#include "dawn/debug.hxx"

using namespace dawn;

// Copy a DTYPE_CHAR config item payload into a null-terminated buffer.

static void cfgCopyStr(char *dst, size_t dstsz, const SObjectCfg::SObjectCfgItem *item)
{
  size_t bytes = static_cast<size_t>(item->cfgid.s.size) * sizeof(uint32_t);
  size_t n = (bytes < dstsz - 1) ? bytes : dstsz - 1;

  std::memcpy(dst, reinterpret_cast<const char *>(&item->data), n);
  dst[n] = '\0';
}

CSystemLte::CSystemLte(CDescObject &desc)
  : CSystemCommon(desc)
  , auth_type(0)
  , ip_type(0)
  , psave_mode(0)
  , reg_timeout(0)
{
  apn[0] = '\0';
  username[0] = '\0';
  password[0] = '\0';
}

void CSystemLte::loadParams()
{
  CDescObject &desc = getDesc();
  size_t count = desc.getSize();
  size_t offset = 0;
  size_t i;
  SObjectCfg::SObjectCfgItem *item;

  // Kconfig defaults

  std::strncpy(apn, CONFIG_DAWN_SYSTEM_LTE_APN, sizeof(apn) - 1);
  apn[sizeof(apn) - 1] = '\0';
  std::strncpy(username, CONFIG_DAWN_SYSTEM_LTE_USERNAME, sizeof(username) - 1);
  username[sizeof(username) - 1] = '\0';
  std::strncpy(password, CONFIG_DAWN_SYSTEM_LTE_PASSWORD, sizeof(password) - 1);
  password[sizeof(password) - 1] = '\0';
  auth_type = CONFIG_DAWN_SYSTEM_LTE_AUTHTYPE;
  ip_type = CONFIG_DAWN_SYSTEM_LTE_IPTYPE;
  psave_mode = CONFIG_DAWN_SYSTEM_LTE_PSAVE;
  reg_timeout = CONFIG_DAWN_SYSTEM_LTE_REG_TIMEOUT;

  // Override from descriptor config items

  for (i = 0; i < count; i++)
    {
      item = desc.objectCfgItemNext(offset);
      if (item == nullptr)
        {
          break;
        }

      if (item->cfgid.s.cls != SYSTEM_CLASS_LTE)
        {
          continue;
        }

      switch (item->cfgid.s.id)
        {
          case LTE_CFG_APN:
            cfgCopyStr(apn, sizeof(apn), item);
            break;

          case LTE_CFG_USERNAME:
            cfgCopyStr(username, sizeof(username), item);
            break;

          case LTE_CFG_PASSWORD:
            cfgCopyStr(password, sizeof(password), item);
            break;

          case LTE_CFG_AUTHTYPE:
            auth_type = static_cast<uint8_t>(item->data[0] & 0xff);
            break;

          case LTE_CFG_IPTYPE:
            ip_type = static_cast<uint8_t>(item->data[0] & 0xff);
            break;

          case LTE_CFG_REG_TIMEOUT:
            reg_timeout = item->data[0];
            break;

          default:
            break;
        }
    }
}

int CSystemLte::configure()
{
  loadParams();

  if (auth_type > DAWN_LTE_AUTH_CHAP || ip_type > DAWN_LTE_IPTYPE_V4V6)
    {
      DAWNERR("LTE invalid auth/ip config\n");
      return -EINVAL;
    }

  return OK;
}

int CSystemLte::doStart()
{
  struct SLteParams params;
  int ret;

  loadParams();

  params.apn = apn[0] ? apn : nullptr;
  params.username = username[0] ? username : nullptr;
  params.password = password[0] ? password : nullptr;
  params.auth_type = auth_type;
  params.ip_type = ip_type;
  params.psave_mode = psave_mode;
  params.reg_timeout = reg_timeout;

  ret = lte_port_connect(&params);
  if (ret < 0)
    {
      DAWNERR("LTE connect failed: %d\n", ret);
      return ret;
    }

  DAWNINFO("LTE connected (apn=%s)\n", params.apn ? params.apn : "default");
  return OK;
}

int CSystemLte::doStop()
{
  return lte_port_disconnect();
}
