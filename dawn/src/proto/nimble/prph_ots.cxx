// dawn/src/proto/nimble/prph_ots.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nimble/prph_ots.hxx"

#include <cstdlib>
#include <cstring>
#include <new>

#include "dawn/io/common.hxx"
#include "host/ble_hs.h"
#include "host/ble_hs_mbuf.h"
#include "os/os_mbuf.h"

using namespace dawn;

// OTS Feature characteristic (BT OTS spec Table 3.7).
// OACP Features: Read=bit4, Write=bit5, Truncation=bit7, Abort=bit9.
// Truncation is advertised because our seekable backing IOs (CIOFile et
// al.) inherently truncate the object on offset==0 writes, which matches
// the spec's "truncate" mode (mode bit 1 / 0x02 in OACP Write per BT OTS
// spec Table 3.11). Non-truncating writes (mode == 0) are also accepted
// for clients that don't request truncation.

static constexpr uint32_t OACP_FEATURE_BITS = (1u << 4) | (1u << 5) | (1u << 7) | (1u << 9);

// OACP Write mode byte: only bits 0 (reserved=0) and 1 (Truncate=0x02)
// are defined by the spec. We accept both 0 and 0x02; any other value is
// either reserved or unsupported.

static constexpr uint8_t OACP_WRITE_MODE_TRUNCATE = 0x02;

// OLCP Features: Go To=bit0. First/Last/Prev/Next are mandatory and not
// advertised in the bitmap.

static constexpr uint32_t OLCP_FEATURE_BITS = (1u << 0);

// Pack a little-endian uint32 value into a 4-byte buffer.

static inline void packU32LE(uint8_t *out, uint32_t v)
{
  out[0] = static_cast<uint8_t>(v & 0xff);
  out[1] = static_cast<uint8_t>((v >> 8) & 0xff);
  out[2] = static_cast<uint8_t>((v >> 16) & 0xff);
  out[3] = static_cast<uint8_t>((v >> 24) & 0xff);
}

static inline uint32_t unpackU32LE(const uint8_t *in)
{
  return (static_cast<uint32_t>(in[0])) | (static_cast<uint32_t>(in[1]) << 8) |
         (static_cast<uint32_t>(in[2]) << 16) | (static_cast<uint32_t>(in[3]) << 24);
}

static inline void packU48LE(uint8_t *out, uint64_t v)
{
  int i;

  for (i = 0; i < 6; i++)
    {
      out[i] = static_cast<uint8_t>((v >> (8 * i)) & 0xff);
    }
}

static inline uint64_t unpackU48LE(const uint8_t *in)
{
  uint64_t v = 0;
  int i;

  for (i = 0; i < 6; i++)
    {
      v |= static_cast<uint64_t>(in[i]) << (8 * i);
    }

  return v;
}

// We allocate the 16-bit UUIDs on the heap so the chr def can hold a stable
// pointer. NimBLE's ble_uuid_any_t is the canonical "any uuid" structure.

static ble_uuid_any_t *makeUuid16(uint16_t value)
{
  ble_uuid_any_t *u = new (std::nothrow) ble_uuid_any_t{};
  if (u == nullptr)
    {
      return nullptr;
    }

  u->u.type = BLE_UUID_TYPE_16;
  u->u16.value = value;
  return u;
}

static void freeUuid(const ble_uuid_t *u)
{
  if (u != nullptr)
    {
      delete reinterpret_cast<const ble_uuid_any_t *>(u);
    }
}

CProtoNimblePrphOts::CProtoNimblePrphOts(const SObjectCfg::SObjectCfgItem *desc_,
                                         IProtoNimblePrphCb *cb_)
  : IProtoNimblePrphService(desc_, cb_)
  , created(false)
{
  id = -1;
  chrs = nullptr;
  hOacp = 0;
  hOlcp = 0;
  xferBuf = nullptr;

  std::memset(&svc, 0, sizeof(svc));
}

CProtoNimblePrphOts::~CProtoNimblePrphOts()
{
  deinit();
}

void CProtoNimblePrphOts::allocObject(const SProtoNimblePrphIOBindOtsObjid &entry)
{
  SOtsObjectMeta meta;
  uint8_t kind;

  kind = cfgGetType(entry.cfg);
  if (kind != PRPH_OTS_TYPE_FILE && kind != PRPH_OTS_TYPE_DESCRIPTOR && kind != PRPH_OTS_TYPE_CAPS)
    {
      DAWNERR("OTS: invalid object kind %u\n", kind);
      return;
    }

  std::memcpy(meta.name, entry.name, OBJ_NAME_MAX);
  switch (kind)
    {
      case PRPH_OTS_TYPE_DESCRIPTOR:
        meta.typeUuid = OBJ_TYPE_DESCRIPTOR;
        break;
      case PRPH_OTS_TYPE_CAPS:
        meta.typeUuid = OBJ_TYPE_CAPS;
        break;
      case PRPH_OTS_TYPE_FILE:
      default:
        meta.typeUuid = OBJ_TYPE_UNSPECIFIED;
        break;
    }

  meta.access = cfgGetAccess(entry.cfg);
  meta.kind = kind;
  meta.iid = entry.objid.v;

  vmeta.push_back(meta);

  cb->regObject(entry.objid.v);
  vio.push_back(entry.objid.v);

  DAWNINFO("OTS: register object '%.*s' kind=%u access=%u id=0x%" PRIx32 "\n",
           OBJ_NAME_MAX,
           entry.name,
           kind,
           meta.access,
           entry.objid.v);
}

void CProtoNimblePrphOts::configureDesc(const SObjectCfg::SObjectCfgItem *item)
{
  size_t k;

  for (k = 0; k < item->cfgid.s.size;)
    {
      const SProtoNimblePrphIOBindOts *blob;
      uint32_t count;
      uint32_t i;

      blob = reinterpret_cast<const SProtoNimblePrphIOBindOts *>(&item->data[k]);
      count = blob->cfg0;

      for (i = 0; i < count; i++)
        {
          allocObject(blob->obj[i]);
        }

      k += (sizeof(SProtoNimblePrphIOBindOts) + count * sizeof(SProtoNimblePrphIOBindOtsObjid)) / 4;
    }
}

int CProtoNimblePrphOts::setChrDef(size_t idx,
                                   uint16_t uuid16,
                                   ble_gatt_access_fn *cb_,
                                   uint16_t flags,
                                   uint16_t *valHandle)
{
  ble_uuid_any_t *u;

  u = makeUuid16(uuid16);
  if (u == nullptr)
    {
      return -ENOMEM;
    }

  chrs[idx].uuid = &u->u;
  chrs[idx].access_cb = cb_;
  chrs[idx].arg = this;
  chrs[idx].flags = flags;
  chrs[idx].val_handle = valHandle;

  return OK;
}

// Number of mandatory OTS characteristics (Feature, Name, Type, Size, ID,
// Props, OACP, OLCP). Shared between allocOTS() and deleteOTS().

static constexpr size_t kNumChrs = 8;

int CProtoNimblePrphOts::allocOTS()
{
  // OACP and OLCP need val_handle pointers we can use later for
  // indications; the metadata characteristics are read-only and don't.

  constexpr uint16_t kRd = BLE_GATT_CHR_F_READ;
  constexpr uint16_t kInd = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_INDICATE;

  const struct
  {
    uint16_t uuid;
    ble_gatt_access_fn *cb;
    uint16_t flags;
    uint16_t *handle;
  } table[kNumChrs] = {
    {UUID16_FEATURE, featureCb, kRd, nullptr},
    {UUID16_OBJ_NAME, nameCb, kRd, nullptr},
    {UUID16_OBJ_TYPE, typeCb, kRd, nullptr},
    {UUID16_OBJ_SIZE, sizeCb, kRd, nullptr},
    {UUID16_OBJ_ID, idCb, kRd, nullptr},
    {UUID16_OBJ_PROPS, propsCb, kRd, nullptr},
    {UUID16_OACP, oacpCb, kInd, &hOacp},
    {UUID16_OLCP, olcpCb, kInd, &hOlcp},
  };

  ble_uuid_any_t *svcUuid;

  chrs = new (std::nothrow) ble_gatt_chr_def[kNumChrs + 1]();
  if (chrs == nullptr)
    {
      DAWNERR("OTS: chr alloc failed\n");
      return -ENOMEM;
    }

  for (size_t i = 0; i < kNumChrs; i++)
    {
      if (setChrDef(i, table[i].uuid, table[i].cb, table[i].flags, table[i].handle) < 0)
        {
          return -ENOMEM;
        }
    }

  // Service definition

  svcUuid = makeUuid16(UUID16_OTS);
  if (svcUuid == nullptr)
    {
      return -ENOMEM;
    }

  svc.type = BLE_GATT_SVC_TYPE_PRIMARY;
  svc.uuid = &svcUuid->u;
  svc.includes = nullptr;
  svc.characteristics = chrs;

  // Pre-allocate the L2CAP TX/RX scratch buffer at init time. The L2CAP
  // RX/TX paths must NOT allocate per-packet (no runtime allocation
  // outside init).

  xferBuf = new (std::nothrow) io_ddata_t(1, L2CAP_MTU_OTS, 1, SObjectId::DTYPE_UINT8);
  if (xferBuf == nullptr || !xferBuf->isAllocated())
    {
      DAWNERR("OTS: xfer buffer alloc failed\n");
      delete xferBuf;
      xferBuf = nullptr;
      return -ENOMEM;
    }

  return OK;
}

void CProtoNimblePrphOts::deleteOTS()
{
  if (chrs != nullptr)
    {
      for (size_t i = 0; i < kNumChrs; i++)
        {
          freeUuid(chrs[i].uuid);
        }

      delete[] chrs;
      chrs = nullptr;
    }

  if (svc.uuid != nullptr)
    {
      freeUuid(svc.uuid);
      svc.uuid = nullptr;
    }

  if (xferBuf != nullptr)
    {
      delete xferBuf;
      xferBuf = nullptr;
    }
}

int CProtoNimblePrphOts::createOTS()
{
  // After NimBLE has finished registering the service, hOacp and hOlcp hold
  // the assigned attr handles (NimBLE wrote into the val_handle pointers we
  // gave it). Validate that every bound IO is seekable -- non-seekable IOs
  // cannot back an OTS object (inverse of the AIOS rule).

  for (SObjectId::ObjectId objid : vio)
    {
      CIOCommon *io;

      io = cb->getObject(objid);
      if (io == nullptr)
        {
          DAWNERR("OTS: IO 0x%" PRIx32 " not bound\n", objid);
          return -EIO;
        }

      if (!io->isSeekable())
        {
          DAWNERR("OTS: IO 0x%" PRIx32 " is not seekable\n", objid);
          return -ENOTSUP;
        }
    }

  return OK;
}

int CProtoNimblePrphOts::init()
{
  int ret;

  configureDesc(desc);

  ret = allocOTS();
  if (ret != OK)
    {
      return ret;
    }

  id = cb->serviceRegister(&svc);
  if (id < 0)
    {
      DAWNERR("OTS service registration failed\n");
      return -EIO;
    }

  return OK;
}

int CProtoNimblePrphOts::deinit()
{
  deleteOTS();
  vmeta.clear();
  vconn.clear();
  created = false;
  return OK;
}

int CProtoNimblePrphOts::start()
{
  int ret;

  if (!created)
    {
      ret = createOTS();
      if (ret != OK)
        {
          return ret;
        }

      created = true;
    }

    // Open L2CAP CoC server. The dummy backend (used in tests) ignores this.

#ifndef CONFIG_DAWN_PROTO_NIMBLE_DUMMY
  ret =
    ble_l2cap_create_server(L2CAP_PSM_OTS, L2CAP_MTU_OTS, CProtoNimblePrphOts::l2capEventCb, this);
  if (ret != 0)
    {
      DAWNERR("OTS: ble_l2cap_create_server failed: %d\n", ret);
      return -EIO;
    }
#endif

  return cb->startService(id);
}

int CProtoNimblePrphOts::stop()
{
  // Drain per-connection state; channel disconnects are pushed by NimBLE
  // through l2capEventCb, but we clear here to be safe.

  vconn.clear();

  return cb->stopService(id);
}

bool CProtoNimblePrphOts::selected(uint16_t conn, SOtsConnState *&state, SOtsObjectMeta *&meta)
{
  std::map<uint16_t, SOtsConnState>::iterator it;

  it = vconn.find(conn);
  if (it == vconn.end())
    {
      return false;
    }

  state = &it->second;
  if (state->cursor < 0 || static_cast<size_t>(state->cursor) >= vmeta.size())
    {
      return false;
    }

  meta = &vmeta[state->cursor];
  return true;
}

// Append a flat byte buffer to a GATT read response mbuf. Returns 0 on
// success or BLE_ATT_ERR_INSUFFICIENT_RES so the caller can `return` directly.

static inline int gattAppend(struct os_mbuf *om, const void *buf, size_t len)
{
  return os_mbuf_append(om, buf, len) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

// Common prologue for every metadata-read callback: enforce READ op, cast
// arg to ``self``, and resolve the currently-selected object for ``conn``.
// On success returns 0 with @c self / @c meta filled; on failure returns the
// BLE ATT error code the caller should propagate.

int CProtoNimblePrphOts::metaPrologue(uint16_t conn,
                                      ble_gatt_access_ctxt *ctxt,
                                      void *arg,
                                      CProtoNimblePrphOts **self,
                                      SOtsObjectMeta **meta,
                                      int16_t *cursorOut)
{
  SOtsConnState *st;

  if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
    {
      return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
    }

  *self = static_cast<CProtoNimblePrphOts *>(arg);
  if (!(*self)->selected(conn, st, *meta))
    {
      return BLE_ATT_ERR_UNLIKELY;
    }

  if (cursorOut != nullptr)
    {
      *cursorOut = st->cursor;
    }

  return 0;
}

int CProtoNimblePrphOts::featureCb(uint16_t /*conn*/,
                                   uint16_t /*attr*/,
                                   struct ble_gatt_access_ctxt *ctxt,
                                   void * /*arg*/)
{
  uint8_t buf[8];

  if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
    {
      return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
    }

  packU32LE(&buf[0], OACP_FEATURE_BITS);
  packU32LE(&buf[4], OLCP_FEATURE_BITS);

  return gattAppend(ctxt->om, buf, sizeof(buf));
}

int CProtoNimblePrphOts::nameCb(uint16_t conn,
                                uint16_t /*attr*/,
                                struct ble_gatt_access_ctxt *ctxt,
                                void *arg)
{
  CProtoNimblePrphOts *self;
  SOtsConnState *st;
  SOtsObjectMeta *meta;

  // Empty Name when nothing is selected yet -- nameCb is the only metadata
  // characteristic that returns success in that state (helps naive clients
  // that don't drive OLCP first).

  if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
    {
      return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
    }

  self = static_cast<CProtoNimblePrphOts *>(arg);
  if (!self->selected(conn, st, meta))
    {
      return 0;
    }

  return gattAppend(ctxt->om, meta->name, strnlen(meta->name, OBJ_NAME_MAX));
}

int CProtoNimblePrphOts::typeCb(uint16_t conn,
                                uint16_t /*attr*/,
                                struct ble_gatt_access_ctxt *ctxt,
                                void *arg)
{
  CProtoNimblePrphOts *self;
  SOtsObjectMeta *meta;
  uint8_t buf[2];
  int err;

  err = metaPrologue(conn, ctxt, arg, &self, &meta, nullptr);
  if (err != 0)
    {
      return err;
    }

  buf[0] = static_cast<uint8_t>(meta->typeUuid & 0xff);
  buf[1] = static_cast<uint8_t>((meta->typeUuid >> 8) & 0xff);
  return gattAppend(ctxt->om, buf, sizeof(buf));
}

int CProtoNimblePrphOts::sizeCb(uint16_t conn,
                                uint16_t /*attr*/,
                                struct ble_gatt_access_ctxt *ctxt,
                                void *arg)
{
  CProtoNimblePrphOts *self;
  SOtsObjectMeta *meta;
  CIOCommon *io;
  uint32_t cur;
  uint8_t buf[8];
  int err;

  err = metaPrologue(conn, ctxt, arg, &self, &meta, nullptr);
  if (err != 0)
    {
      return err;
    }

  io = self->cb->getObject(meta->iid);
  cur = (io != nullptr) ? static_cast<uint32_t>(io->getDataSize()) : 0;

  // The Object Size characteristic carries (current, allocated). The
  // backing IOs (CIOFile, CIODescriptor, CIOCapabilities) do not expose a
  // separate "allocated" capacity distinct from the current data size, so
  // both fields are reported as `cur`. Clients should treat current ==
  // max writable size and not attempt to grow the object beyond it.

  packU32LE(&buf[0], cur);
  packU32LE(&buf[4], cur);
  return gattAppend(ctxt->om, buf, sizeof(buf));
}

int CProtoNimblePrphOts::idCb(uint16_t conn,
                              uint16_t /*attr*/,
                              struct ble_gatt_access_ctxt *ctxt,
                              void *arg)
{
  CProtoNimblePrphOts *self;
  SOtsObjectMeta *meta;
  uint8_t buf[6];
  int16_t cursor = 0;
  int err;

  err = metaPrologue(conn, ctxt, arg, &self, &meta, &cursor);
  if (err != 0)
    {
      return err;
    }

  // Map array index 0..N-1 to object IDs 256..256+N-1 (spec reserves
  // 0..255 for "Directory Listing" object plus future use).

  packU48LE(buf, 256ull + static_cast<uint64_t>(cursor));
  return gattAppend(ctxt->om, buf, sizeof(buf));
}

int CProtoNimblePrphOts::propsCb(uint16_t conn,
                                 uint16_t /*attr*/,
                                 struct ble_gatt_access_ctxt *ctxt,
                                 void *arg)
{
  CProtoNimblePrphOts *self;
  SOtsObjectMeta *meta;
  uint8_t buf[4];
  uint32_t props = 0;
  int err;

  err = metaPrologue(conn, ctxt, arg, &self, &meta, nullptr);
  if (err != 0)
    {
      return err;
    }

  if (meta->access == PRPH_OTS_ACCESS_READ || meta->access == PRPH_OTS_ACCESS_RW)
    {
      props |= OBJ_PROP_READ;
    }

  if (meta->access == PRPH_OTS_ACCESS_WRITE || meta->access == PRPH_OTS_ACCESS_RW)
    {
      props |= OBJ_PROP_WRITE | OBJ_PROP_TRUNC;
    }

  packU32LE(buf, props);
  return gattAppend(ctxt->om, buf, sizeof(buf));
}

void CProtoNimblePrphOts::sendCpResponse(uint16_t conn,
                                         uint16_t handle,
                                         uint8_t respOp,
                                         uint8_t reqOp,
                                         uint8_t result)
{
#ifndef CONFIG_DAWN_PROTO_NIMBLE_DUMMY
  uint8_t buf[3] = {respOp, reqOp, result};
  struct os_mbuf *om;
  int rc;

  om = ble_hs_mbuf_from_flat(buf, sizeof(buf));
  if (om == nullptr)
    {
      DAWNERR("OTS: response mbuf alloc failed\n");
      return;
    }

  rc = ble_gatts_indicate_custom(conn, handle, om);
  if (rc != 0)
    {
      DAWNERR("OTS: indicate failed: %d\n", rc);
    }
#else
  (void)conn;
  (void)handle;
  (void)respOp;
  (void)reqOp;
  (void)result;
#endif
}

inline void CProtoNimblePrphOts::sendOacpResponse(uint16_t conn, uint8_t reqOp, uint8_t result)
{
  sendCpResponse(conn, hOacp, OACP_OP_RESPONSE, reqOp, result);
}

inline void CProtoNimblePrphOts::sendOlcpResponse(uint16_t conn, uint8_t reqOp, uint8_t result)
{
  sendCpResponse(conn, hOlcp, OLCP_OP_RESPONSE, reqOp, result);
}

int CProtoNimblePrphOts::oacpCb(uint16_t conn,
                                uint16_t /*attr*/,
                                struct ble_gatt_access_ctxt *ctxt,
                                void *arg)
{
  CProtoNimblePrphOts *self;
  uint8_t op;
  uint8_t pkt[16];
  uint16_t copied = 0;
  int rc;

  if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
    {
      return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
    }

  rc = ble_hs_mbuf_to_flat(ctxt->om, pkt, sizeof(pkt), &copied);
  if (rc != 0 || copied < 1)
    {
      return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

  self = static_cast<CProtoNimblePrphOts *>(arg);
  op = pkt[0];

  // Make sure connection has state.

  self->vconn.try_emplace(conn);

  switch (op)
    {
      case OACP_OP_READ:
        {
          uint32_t off;
          uint32_t len;

          if (copied < 9)
            {
              self->sendOacpResponse(conn, op, OACP_RES_INVALID_PARAM);
              return 0;
            }

          off = unpackU32LE(&pkt[1]);
          len = unpackU32LE(&pkt[5]);
          self->handleOacpRead(conn, off, len);
          return 0;
        }

      case OACP_OP_WRITE:
        {
          uint32_t off;
          uint32_t len;
          uint8_t mode;

          if (copied < 10)
            {
              self->sendOacpResponse(conn, op, OACP_RES_INVALID_PARAM);
              return 0;
            }

          off = unpackU32LE(&pkt[1]);
          len = unpackU32LE(&pkt[5]);
          mode = pkt[9];
          self->handleOacpWrite(conn, off, len, mode);
          return 0;
        }

      case OACP_OP_ABORT:
        self->handleOacpAbort(conn);
        return 0;

      default:
        self->sendOacpResponse(conn, op, OACP_RES_OPCODE_NS);
        return 0;
    }
}

int CProtoNimblePrphOts::olcpCb(uint16_t conn,
                                uint16_t /*attr*/,
                                struct ble_gatt_access_ctxt *ctxt,
                                void *arg)
{
  CProtoNimblePrphOts *self;
  uint8_t op;
  uint8_t pkt[16];
  uint16_t copied = 0;
  int rc;

  if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
    {
      return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
    }

  rc = ble_hs_mbuf_to_flat(ctxt->om, pkt, sizeof(pkt), &copied);
  if (rc != 0 || copied < 1)
    {
      return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

  self = static_cast<CProtoNimblePrphOts *>(arg);
  op = pkt[0];

  self->vconn.try_emplace(conn);

  switch (op)
    {
      case OLCP_OP_FIRST:
        return self->handleOlcpFirst(conn);
      case OLCP_OP_LAST:
        return self->handleOlcpLast(conn);
      case OLCP_OP_PREVIOUS:
        return self->handleOlcpPrev(conn);
      case OLCP_OP_NEXT:
        return self->handleOlcpNext(conn);
      case OLCP_OP_GOTO:
        {
          uint64_t oid;

          if (copied < 7)
            {
              self->sendOlcpResponse(conn, op, OLCP_RES_INVALID_PARAM);
              return 0;
            }

          oid = unpackU48LE(&pkt[1]);
          return self->handleOlcpGoto(conn, oid);
        }

      default:
        self->sendOlcpResponse(conn, op, OLCP_RES_OPCODE_NS);
        return 0;
    }
}

// Validate the OACP read/write preconditions: an object is selected, the
// requested access matches the object's properties, and the backing IO is
// resolvable and seekable. On success returns OACP_RES_SUCCESS with @c st
// and @c io filled; on failure returns the OACP result code the caller
// should reply with.

uint8_t CProtoNimblePrphOts::oacpValidate(uint16_t conn,
                                          bool wantWrite,
                                          SOtsConnState *&st,
                                          CIOCommon *&io)
{
  SOtsObjectMeta *meta;

  if (!selected(conn, st, meta))
    {
      return OACP_RES_INVALID_OBJ;
    }

  if (wantWrite)
    {
      if (meta->access != PRPH_OTS_ACCESS_WRITE && meta->access != PRPH_OTS_ACCESS_RW)
        {
          return OACP_RES_PROC_NOT_PERM;
        }
    }
  else
    {
      if (meta->access != PRPH_OTS_ACCESS_READ && meta->access != PRPH_OTS_ACCESS_RW)
        {
          return OACP_RES_PROC_NOT_PERM;
        }
    }

  io = cb->getObject(meta->iid);
  if (io == nullptr || !io->isSeekable())
    {
      return OACP_RES_OPER_FAILED;
    }

  return OACP_RES_SUCCESS;
}

int CProtoNimblePrphOts::handleOacpRead(uint16_t conn, uint32_t off, uint32_t len)
{
  SOtsConnState *st;
  CIOCommon *io;
  uint8_t res;

  res = oacpValidate(conn, false, st, io);
  if (res == OACP_RES_SUCCESS && off + len > io->getDataSize())
    {
      res = OACP_RES_INVALID_PARAM;
    }

  if (res != OACP_RES_SUCCESS)
    {
      sendOacpResponse(conn, OACP_OP_READ, res);
      return 0;
    }

  st->mode = MODE_READING;
  st->offset = off;
  st->remaining = len;

  sendOacpResponse(conn, OACP_OP_READ, OACP_RES_SUCCESS);
  pumpRead(conn);
  return 0;
}

int CProtoNimblePrphOts::handleOacpWrite(uint16_t conn, uint32_t off, uint32_t len, uint8_t mode)
{
  SOtsConnState *st;
  CIOCommon *io;
  uint8_t res;

  // OACP Write mode parameter validation. We support mode == 0 (plain
  // write) and mode == 0x02 (truncate). The truncate flag matches our
  // backing IOs' natural behavior on offset-0 writes, so both modes
  // produce the same on-disk result; rejecting other values keeps the
  // contract honest and surfaces typos / future spec extensions.

  if (mode != 0 && mode != OACP_WRITE_MODE_TRUNCATE)
    {
      sendOacpResponse(conn, OACP_OP_WRITE, OACP_RES_INVALID_PARAM);
      return 0;
    }

  res = oacpValidate(conn, true, st, io);
  if (res != OACP_RES_SUCCESS)
    {
      sendOacpResponse(conn, OACP_OP_WRITE, res);
      return 0;
    }

  st->mode = MODE_WRITING;
  st->offset = off;
  st->remaining = len;

  sendOacpResponse(conn, OACP_OP_WRITE, OACP_RES_SUCCESS);
  return 0;
}

int CProtoNimblePrphOts::handleOacpAbort(uint16_t conn)
{
  std::map<uint16_t, SOtsConnState>::iterator it;

  it = vconn.find(conn);
  if (it != vconn.end())
    {
      it->second.mode = MODE_IDLE;
      it->second.remaining = 0;
    }

  sendOacpResponse(conn, OACP_OP_ABORT, OACP_RES_SUCCESS);
  return 0;
}

// Set the per-connection cursor to an absolute index, replying with
// OLCP_RES_NO_OBJECT when the object list is empty. Used by First/Last.

int CProtoNimblePrphOts::olcpSeekAbs(uint16_t conn, uint8_t op, size_t idx)
{
  if (vmeta.empty())
    {
      sendOlcpResponse(conn, op, OLCP_RES_NO_OBJECT);
      return 0;
    }

  vconn[conn].cursor = static_cast<int16_t>(idx);
  sendOlcpResponse(conn, op, OLCP_RES_SUCCESS);
  return 0;
}

// Step the per-connection cursor by ``delta`` (+/-1), replying with
// OLCP_RES_OOR when the result would fall outside [0, vmeta.size()).
// Used by Prev/Next.

int CProtoNimblePrphOts::olcpStep(uint16_t conn, uint8_t op, int delta)
{
  int16_t &c = vconn[conn].cursor;
  int next = static_cast<int>(c) + delta;

  if (next < 0 || static_cast<size_t>(next) >= vmeta.size())
    {
      sendOlcpResponse(conn, op, OLCP_RES_OOR);
      return 0;
    }

  c = static_cast<int16_t>(next);
  sendOlcpResponse(conn, op, OLCP_RES_SUCCESS);
  return 0;
}

int CProtoNimblePrphOts::handleOlcpFirst(uint16_t conn)
{
  return olcpSeekAbs(conn, OLCP_OP_FIRST, 0);
}

int CProtoNimblePrphOts::handleOlcpLast(uint16_t conn)
{
  return olcpSeekAbs(conn, OLCP_OP_LAST, vmeta.empty() ? 0 : vmeta.size() - 1);
}

int CProtoNimblePrphOts::handleOlcpPrev(uint16_t conn)
{
  return olcpStep(conn, OLCP_OP_PREVIOUS, -1);
}

int CProtoNimblePrphOts::handleOlcpNext(uint16_t conn)
{
  return olcpStep(conn, OLCP_OP_NEXT, +1);
}

int CProtoNimblePrphOts::handleOlcpGoto(uint16_t conn, uint64_t obj_id)
{
  if (obj_id < 256)
    {
      sendOlcpResponse(conn, OLCP_OP_GOTO, OLCP_RES_OBJECT_ID_NF);
      return 0;
    }

  uint64_t idx = obj_id - 256;
  if (idx >= vmeta.size())
    {
      sendOlcpResponse(conn, OLCP_OP_GOTO, OLCP_RES_OBJECT_ID_NF);
      return 0;
    }

  vconn[conn].cursor = static_cast<int16_t>(idx);
  sendOlcpResponse(conn, OLCP_OP_GOTO, OLCP_RES_SUCCESS);
  return 0;
}

int CProtoNimblePrphOts::pumpRead(uint16_t conn)
{
#ifdef CONFIG_DAWN_PROTO_NIMBLE_DUMMY
  (void)conn;
  return 0;
#else
  SOtsConnState *st;
  SOtsObjectMeta *meta;
  CIOCommon *io;

  if (!selected(conn, st, meta))
    {
      return -EINVAL;
    }

  if (st->mode != MODE_READING || st->chan == nullptr)
    {
      return 0;
    }

  io = cb->getObject(meta->iid);
  if (io == nullptr)
    {
      return -EINVAL;
    }

  if (xferBuf == nullptr)
    {
      DAWNERR("OTS: xferBuf not initialised\n");
      st->mode = MODE_IDLE;
      return -EINVAL;
    }

  while (st->remaining > 0)
    {
      struct os_mbuf *sdu;
      size_t chunk;
      int rc;

      chunk = std::min<size_t>(st->remaining, L2CAP_MTU_OTS);

      // The pre-allocated xferBuf is fixed at L2CAP_MTU_OTS bytes; trim
      // its logical size (T*N) to the per-chunk amount so the seekable
      // IO read fills exactly `chunk` bytes.

      xferBuf->N = chunk;

      rc = io->getData(*xferBuf, 1, st->offset);
      if (rc < 0)
        {
          st->mode = MODE_IDLE;
          return rc;
        }

      sdu = ble_hs_mbuf_from_flat(xferBuf->getDataPtr(), chunk);
      if (sdu == nullptr)
        {
          return -ENOMEM;
        }

      rc = ble_l2cap_send(st->chan, sdu);
      if (rc == BLE_HS_ESTALLED)
        {
          // Wait for TX_UNSTALLED -- flow control will resume the pump.
          // Account this chunk as already in flight (NimBLE owns the mbuf).

          st->offset += chunk;
          st->remaining -= chunk;
          return 0;
        }

      if (rc != 0)
        {
          DAWNERR("OTS: l2cap_send failed: %d\n", rc);
          st->mode = MODE_IDLE;
          return -EIO;
        }

      st->offset += chunk;
      st->remaining -= chunk;
    }

  st->mode = MODE_IDLE;
  return 0;
#endif
}

#ifndef CONFIG_DAWN_PROTO_NIMBLE_DUMMY
void CProtoNimblePrphOts::onL2capRx(struct ble_l2cap_event *event)
{
  std::map<uint16_t, SOtsConnState>::iterator it;
  struct os_mbuf *sdu;
  struct os_mbuf *next;
  SOtsObjectMeta *meta;
  CIOCommon *io;
  uint16_t conn;
  uint16_t pktLen;
  uint16_t copied;
  bool drop;
  int rc;

  conn = event->receive.conn_handle;
  sdu = event->receive.sdu_rx;
  pktLen = OS_MBUF_PKTLEN(sdu);
  it = vconn.find(conn);
  drop = false;

  // Validate prerequisites: connection known, in WRITING mode, packet
  // size sane, scratch buffer available, cursor valid. Anything else is
  // a protocol violation -- log and tear down the channel rather than
  // silently dropping bytes.

  if (it == vconn.end() || it->second.mode != MODE_WRITING || pktLen == 0 ||
      pktLen > L2CAP_MTU_OTS || xferBuf == nullptr || it->second.cursor < 0 ||
      static_cast<size_t>(it->second.cursor) >= vmeta.size())
    {
      DAWNERR("OTS: unexpected L2CAP RX (conn=%u len=%u mode=%u)\n",
              conn,
              pktLen,
              (it != vconn.end()) ? it->second.mode : 0xff);
      drop = true;
    }
  else if (pktLen > it->second.remaining)
    {
      // Client sent more data than declared in the OACP Write request.

      DAWNERR("OTS: write overflow (conn=%u declared=%" PRIu32 " got=%u)\n",
              conn,
              it->second.remaining,
              pktLen);
      it->second.mode = MODE_IDLE;
      drop = true;
    }
  else
    {
      meta = &vmeta[it->second.cursor];
      io = cb->getObject(meta->iid);
      if (io == nullptr)
        {
          DAWNERR("OTS: backing IO 0x%" PRIx32 " gone mid-write\n", meta->iid);
          it->second.mode = MODE_IDLE;
          drop = true;
        }
      else
        {
          copied = 0;
          ble_hs_mbuf_to_flat(sdu, xferBuf->getDataPtr(), pktLen, &copied);

          // The pre-allocated xferBuf is fixed at L2CAP_MTU_OTS bytes;
          // trim its logical size so setDataAtImpl writes exactly the
          // bytes received in this packet.

          xferBuf->N = copied;
          rc = io->setData(*xferBuf, it->second.offset);
          if (rc < 0)
            {
              DAWNERR("OTS: setData failed off=%" PRIu32 " rc=%d\n", it->second.offset, rc);
              it->second.mode = MODE_IDLE;
              drop = true;
            }
          else
            {
              it->second.offset += copied;
              it->second.remaining =
                (it->second.remaining > copied) ? it->second.remaining - copied : 0;
              if (it->second.remaining == 0)
                {
                  it->second.mode = MODE_IDLE;
                }
            }
        }
    }

  os_mbuf_free_chain(sdu);

  if (drop)
    {
      // Tear down the L2CAP channel so the client sees the failure.
      // NimBLE will deliver COC_DISCONNECTED, which clears chan in vconn.

      if (event->receive.chan != nullptr)
        {
          ble_l2cap_disconnect(event->receive.chan);
        }

      return;
    }

  // Re-arm the L2CAP receive buffer for the next inbound SDU.

  next = ble_hs_mbuf_att_pkt();
  if (next != nullptr)
    {
      ble_l2cap_recv_ready(event->receive.chan, next);
    }
}
#endif

int CProtoNimblePrphOts::l2capEventCb(struct ble_l2cap_event *event, void *arg)
{
#ifdef CONFIG_DAWN_PROTO_NIMBLE_DUMMY
  (void)event;
  (void)arg;
  return 0;
#else
  CProtoNimblePrphOts *self;
  uint16_t conn;

  self = static_cast<CProtoNimblePrphOts *>(arg);

  switch (event->type)
    {
      case BLE_L2CAP_EVENT_COC_ACCEPT:
        {
          // Accept the channel: NimBLE expects us to provide an SDU receive
          // buffer. We stash an empty mbuf for now.
          //
          // Single-client policy: the per-instance scratch buffer xferBuf
          // is shared across the service, so concurrent OTS sessions are
          // unsafe. If any vconn entry already has an active CoC channel,
          // refuse this accept with BLE_HS_EBUSY -- the central will see
          // the L2CAP connect fail and can retry once the first client
          // disconnects.

          struct os_mbuf *sdu;

          for (auto &kv : self->vconn)
            {
              if (kv.second.chan != nullptr)
                {
                  DAWNINFO("OTS: refusing second L2CAP CoC (busy)\n");
                  return BLE_HS_EBUSY;
                }
            }

          sdu = ble_hs_mbuf_att_pkt();
          if (sdu == nullptr)
            {
              return BLE_HS_ENOMEM;
            }

          ble_l2cap_recv_ready(event->accept.chan, sdu);
          return 0;
        }

      case BLE_L2CAP_EVENT_COC_CONNECTED:
        {
          conn = event->connect.conn_handle;
          self->vconn.try_emplace(conn);
          self->vconn[conn].chan = event->connect.chan;

          // If an OACP Read is already pending (the client may issue it
          // before opening the channel, but our server can also accept
          // the spec-recommended order channel-first then OACP), kick
          // off the TX pump now that we have a channel.

          if (self->vconn[conn].mode == MODE_READING)
            {
              self->pumpRead(conn);
            }

          return 0;
        }

      case BLE_L2CAP_EVENT_COC_DISCONNECTED:
        {
          std::map<uint16_t, SOtsConnState>::iterator it;

          conn = event->disconnect.conn_handle;
          it = self->vconn.find(conn);
          if (it != self->vconn.end())
            {
              it->second.chan = nullptr;
              it->second.mode = MODE_IDLE;
            }
          return 0;
        }

      case BLE_L2CAP_EVENT_COC_TX_UNSTALLED:
        self->pumpRead(event->tx_unstalled.conn_handle);
        return 0;

      case BLE_L2CAP_EVENT_COC_DATA_RECEIVED:
        self->onL2capRx(event);
        return 0;

      default:
        return 0;
    }
#endif
}
