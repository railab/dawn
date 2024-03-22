// dawn/src/proto/nimble/prph_dis.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nimble/prph_dis.hxx"

using namespace dawn;

extern "C"
{
  // Device Information Service Implementation

  void ble_svc_dis_init();
};

// Assumption: Configuration item passed by caller is a valid type supported by
// This class.
void CProtoNimblePrphDis::configureDesc(const SObjectCfg::SObjectCfgItem *item)
{
  // Nothing here
}

CProtoNimblePrphDis::CProtoNimblePrphDis(const SObjectCfg::SObjectCfgItem *desc_,
                                         IProtoNimblePrphCb *cb_)
  : IProtoNimblePrphService(desc_, cb_)
{
  // Service ID not assigned yet

  id = -1;
}

CProtoNimblePrphDis::~CProtoNimblePrphDis()
{
  deinit();
}

int CProtoNimblePrphDis::init()
{
  // ble_svc_dis_init() touches NimBLE singleton state and must run at most
  // once per stack lifetime, regardless of how many DIS objects exist.

  static bool s_dis_initialized = false;

  // Configure object

  configureDesc(desc);

  if (!s_dis_initialized)
    {
      ble_svc_dis_init();
      s_dis_initialized = true;
    }

  // Register servie in peripheral handler

  id = cb->serviceRegister(nullptr);
  if (id < 0)
    {
      DAWNERR("DIS service registration failed\n");
      return -EIO;
    }

  return OK;
}

int CProtoNimblePrphDis::deinit()
{
  return OK;
}

int CProtoNimblePrphDis::start()
{
  // Signal start to peripheral handler

  return cb->startService(id);
}

int CProtoNimblePrphDis::stop()
{
  // Signal stop to peripheral handler

  return cb->stopService(id);
}
