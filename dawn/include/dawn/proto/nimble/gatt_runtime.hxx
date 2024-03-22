// dawn/include/dawn/proto/nimble/gatt_runtime.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>

#include "dawn/proto/nimble/iprph.hxx"
#include "host/ble_gatt.h"

namespace dawn
{
void nimbleGattChrInit(struct ble_gatt_chr_def &chr, void *arg);
int nimbleGattChrUuid128Set(struct ble_gatt_chr_def &chr, const uint32_t *uuid);
void nimbleGattChrUuidFree(struct ble_gatt_chr_def &chr);
void nimbleGattChrAccessSet(struct ble_gatt_chr_def &chr, CIOCommon &io);

#ifdef CONFIG_DAWN_IO_NOTIFY
int nimbleGattChrNotifySet(struct ble_gatt_chr_def &chr,
                           CIOCommon &io,
                           IProtoNimblePrphService::SPrphNotiferCb &ncb,
                           int (*notifier)(void *priv, io_ddata_t *data),
                           SObjectId::ObjectId objid);
#endif

IProtoNimblePrphService::SPrphNotiferCb *nimbleGattNotifierCreate(CIOCommon *io);
void nimbleGattNotifierFree(IProtoNimblePrphService::SPrphNotiferCb *ncb);
void nimbleGattChrNotifierFree(struct ble_gatt_chr_def &chr);
} // namespace dawn
