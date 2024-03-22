// dawn/include/dawn/proto/nimble/prph_ots.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <map>
#include <vector>

#include "dawn/porting/config.hxx"
#include "dawn/proto/nimble/iprph.hxx"
#include "host/ble_gatt.h"
#include "host/ble_l2cap.h"

namespace dawn
{
/**
 * @brief Object Transfer Service (OTS) for BLE Peripheral.
 *
 * Implements the Bluetooth SIG Object Transfer Service v1.0 (UUID 0x1825).
 *
 * The service exposes one or more @c CIOCommon objects whose
 * @c isSeekable() returns @c true (e.g. @c CIOFile, @c CIODescriptor,
 * @c CIOCapabilities) as OTS Objects. Bulk data is carried over an
 * L2CAP CoC channel (PSM 0x0025); the GATT side carries only metadata
 * and the OACP/OLCP control points.
 *
 * Supported OACP opcodes: Read, Write, Abort.
 * Supported OLCP opcodes: First, Last, Previous, Next, Go To.
 * Optional characteristics (List Filter, First-Created, Last-Modified,
 * Object Changed) are not implemented in this revision.
 *
 * @note Single-client only. The L2CAP CoC server accepts at most one
 *       active channel; concurrent OTS sessions from a second central are
 *       refused with @c BLE_HS_EBUSY at @c BLE_L2CAP_EVENT_COC_ACCEPT.
 *       The pre-allocated transfer buffer is shared and not safe for
 *       multi-client use.
 */

class CProtoNimblePrphOts : public IProtoNimblePrphService
{
public:
  /** @brief OTS Service UUID (Bluetooth SIG). */

  constexpr static uint16_t UUID16_OTS = 0x1825;

  /** @brief OTS Feature characteristic. */

  constexpr static uint16_t UUID16_FEATURE = 0x2abd;

  /** @brief Object Name characteristic. */

  constexpr static uint16_t UUID16_OBJ_NAME = 0x2abe;

  /** @brief Object Type characteristic. */

  constexpr static uint16_t UUID16_OBJ_TYPE = 0x2abf;

  /** @brief Object Size characteristic (current + allocated, 8 bytes). */

  constexpr static uint16_t UUID16_OBJ_SIZE = 0x2ac0;

  /** @brief Object ID characteristic (48-bit). */

  constexpr static uint16_t UUID16_OBJ_ID = 0x2ac3;

  /** @brief Object Properties characteristic. */

  constexpr static uint16_t UUID16_OBJ_PROPS = 0x2ac4;

  /** @brief Object Action Control Point characteristic. */

  constexpr static uint16_t UUID16_OACP = 0x2ac5;

  /** @brief Object List Control Point characteristic. */

  constexpr static uint16_t UUID16_OLCP = 0x2ac6;

  /** @brief Object Type for an "Unspecified" file (Bluetooth SIG). */

  constexpr static uint16_t OBJ_TYPE_UNSPECIFIED = 0x2aca;

  /**
   * @brief Object Type for a Dawn descriptor blob (Dawn-private UUID16).
   *
   * Allocated in the unassigned 16-bit UUID range. Subject to revision if
   * the Bluetooth SIG ever assigns this value. Clients that don't recognise
   * it should treat the object as opaque bytes.
   */

  constexpr static uint16_t OBJ_TYPE_DESCRIPTOR = 0xffe0;

  /**
   * @brief Object Type for a Dawn capabilities blob (Dawn-private UUID16).
   *
   * Allocated in the unassigned 16-bit UUID range; same caveat as
   * @c OBJ_TYPE_DESCRIPTOR.
   */

  constexpr static uint16_t OBJ_TYPE_CAPS = 0xffe1;

  /** @brief L2CAP PSM for OTS bulk data (Bluetooth SIG-assigned). */

  constexpr static uint16_t L2CAP_PSM_OTS = 0x0025;

  /** @brief L2CAP CoC MTU for OTS data channel. */

  constexpr static uint16_t L2CAP_MTU_OTS = 512;

  /** @brief Object Properties bits. */

  enum
  {
    OBJ_PROP_DELETE = 1u << 0,
    OBJ_PROP_EXECUTE = 1u << 1,
    OBJ_PROP_READ = 1u << 2,
    OBJ_PROP_WRITE = 1u << 3,
    OBJ_PROP_APPEND = 1u << 4,
    OBJ_PROP_TRUNC = 1u << 5,
    OBJ_PROP_PATCH = 1u << 6,
    OBJ_PROP_MARKED = 1u << 7
  };

  /** @brief OACP request opcodes (Bluetooth OTS spec Table 3.9). */

  enum
  {
    OACP_OP_CREATE = 0x01,
    OACP_OP_DELETE = 0x02,
    OACP_OP_READ = 0x04,
    OACP_OP_EXECUTE = 0x05,
    OACP_OP_WRITE = 0x06,
    OACP_OP_ABORT = 0x07,
    OACP_OP_RESPONSE = 0x60
  };

  /** @brief OACP result codes (Bluetooth OTS spec Table 3.10). */

  enum
  {
    OACP_RES_SUCCESS = 0x01,
    OACP_RES_OPCODE_NS = 0x02, ///< Opcode Not Supported.
    OACP_RES_INVALID_PARAM = 0x03,
    OACP_RES_INSUF_RES = 0x04,
    OACP_RES_INVALID_OBJ = 0x05,
    OACP_RES_CHANNEL_UNAVAIL = 0x06,
    OACP_RES_PROC_NOT_PERM = 0x07,
    OACP_RES_OBJ_LOCKED = 0x08,
    OACP_RES_OPER_FAILED = 0x0a
  };

  /** @brief OLCP request opcodes (Bluetooth OTS spec Table 3.16). */

  enum
  {
    OLCP_OP_FIRST = 0x01,
    OLCP_OP_LAST = 0x02,
    OLCP_OP_PREVIOUS = 0x03,
    OLCP_OP_NEXT = 0x04,
    OLCP_OP_GOTO = 0x05,
    OLCP_OP_ORDER = 0x06,
    OLCP_OP_REQ_NUM = 0x07,
    OLCP_OP_CLEAR_MK = 0x08,
    OLCP_OP_RESPONSE = 0x70
  };

  /** @brief OLCP result codes (Bluetooth OTS spec Table 3.17). */

  enum
  {
    OLCP_RES_SUCCESS = 0x01,
    OLCP_RES_OPCODE_NS = 0x02,
    OLCP_RES_INVALID_PARAM = 0x03,
    OLCP_RES_OPER_FAILED = 0x04,
    OLCP_RES_OOR = 0x05, ///< Out Of Bounds.
    OLCP_RES_NO_OBJECT = 0x06,
    OLCP_RES_OBJECT_ID_NF = 0x07
  };

  /** @brief OTS object kinds derived from descriptor cfg. */

  enum
  {
    PRPH_OTS_TYPE_FILE = 0,
    PRPH_OTS_TYPE_DESCRIPTOR = 1,
    PRPH_OTS_TYPE_CAPS = 2
  };

  /** @brief Per-object access mode (descriptor-supplied). */

  enum
  {
    PRPH_OTS_ACCESS_READ = 0,
    PRPH_OTS_ACCESS_WRITE = 1,
    PRPH_OTS_ACCESS_RW = 2
  };

  /** @brief Per-object cfg word layout (lower bits of @c obj.cfg). */

  constexpr static uint8_t CFG_TYPE_SHIFT = 0;
  constexpr static uint8_t CFG_TYPE_MASK = 0xf;
  constexpr static uint8_t CFG_ACCESS_SHIFT = 4;
  constexpr static uint8_t CFG_ACCESS_MASK = 0x3;

  /** @brief OTS object name length (bytes, fixed). */

  constexpr static uint8_t OBJ_NAME_MAX = 16;

  /**
   * @brief Per-object descriptor entry.
   *
   * One of these is emitted by dawnpy for every object listed in the
   * @c services.ots.objects[] YAML.
   */

  struct
  {
    uint32_t cfg;               ///< type | access bits
    char name[OBJ_NAME_MAX];    ///< object name (NUL-padded)
    SObjectId::UObjectId objid; ///< I/O object ID for backing storage
  } typedef SProtoNimblePrphIOBindOtsObjid;

  /**
   * @brief Top-level OTS binding payload.
   *
   * Fields after @c cfg0 are an array of @c SProtoNimblePrphIOBindOtsObjid
   * entries. Length is taken from @c item->cfgid.s.size.
   */

  struct
  {
    uint32_t cfg0;                        ///< Number of objects in @c obj[].
    uint32_t cfg1;                        ///< Reserved.
    uint32_t cfg2;                        ///< Reserved.

    SProtoNimblePrphIOBindOtsObjid obj[]; ///< Object entries.
  } typedef SProtoNimblePrphIOBindOts;

  CProtoNimblePrphOts(const SObjectCfg::SObjectCfgItem *item, IProtoNimblePrphCb *cb_);

  ~CProtoNimblePrphOts() override;

  int init() override;
  int deinit() override;
  int start() override;
  int stop() override;

private:
  /** @brief Per-object metadata maintained by the service. */

  struct SOtsObjectMeta
  {
    char name[OBJ_NAME_MAX]; ///< Object name (NUL-padded).
    uint16_t typeUuid;       ///< Object Type UUID16.
    uint8_t access;          ///< PRPH_OTS_ACCESS_*.
    uint8_t kind;            ///< PRPH_OTS_TYPE_*.
    SObjectId::ObjectId iid; ///< Backing IO object ID.
  };

  /** @brief Per-connection transfer state. */

  struct SOtsConnState
  {
    int16_t cursor;              ///< Selected object index (or -1).
    uint8_t mode;                ///< 0=idle, 1=reading, 2=writing.
    uint32_t offset;             ///< Current byte offset into selected object.
    uint32_t remaining;          ///< Bytes left in active OACP transfer.
    struct ble_l2cap_chan *chan; ///< Active CoC channel.

    SOtsConnState()
      : cursor(-1)
      , mode(0)
      , offset(0)
      , remaining(0)
      , chan(nullptr)
    {
    }
  };

  static constexpr uint8_t MODE_IDLE = 0;
  static constexpr uint8_t MODE_READING = 1;
  static constexpr uint8_t MODE_WRITING = 2;

  int id;                                  ///< Service ID.
  bool created;                            ///< Whether runtime service validation has completed.
  struct ble_gatt_svc_def svc;             ///< GATT service definition.
  struct ble_gatt_chr_def *chrs;           ///< Heap-allocated chr def array.
  uint16_t hOacp;                          ///< Cached OACP value attr handle.
  uint16_t hOlcp;                          ///< Cached OLCP value attr handle.
  io_ddata_t *xferBuf;                     ///< Init-time L2CAP RX/TX buffer.
  std::vector<SOtsObjectMeta> vmeta;       ///< Metadata per object.
  std::map<uint16_t, SOtsConnState> vconn; ///< Per-connection state.

  void configureDesc(const SObjectCfg::SObjectCfgItem *item);
  void allocObject(const SProtoNimblePrphIOBindOtsObjid &entry);
  int allocOTS();
  int createOTS();
  void deleteOTS();
  int setChrDef(size_t idx,
                uint16_t uuid16,
                ble_gatt_access_fn *cb_,
                uint16_t flags,
                uint16_t *valHandle);

  // GATT access callbacks (one per characteristic)

  static int featureCb(uint16_t conn_handle,
                       uint16_t attr_handle,
                       struct ble_gatt_access_ctxt *ctxt,
                       void *arg);

  static int nameCb(uint16_t conn_handle,
                    uint16_t attr_handle,
                    struct ble_gatt_access_ctxt *ctxt,
                    void *arg);

  static int typeCb(uint16_t conn_handle,
                    uint16_t attr_handle,
                    struct ble_gatt_access_ctxt *ctxt,
                    void *arg);

  static int sizeCb(uint16_t conn_handle,
                    uint16_t attr_handle,
                    struct ble_gatt_access_ctxt *ctxt,
                    void *arg);

  static int idCb(uint16_t conn_handle,
                  uint16_t attr_handle,
                  struct ble_gatt_access_ctxt *ctxt,
                  void *arg);

  static int propsCb(uint16_t conn_handle,
                     uint16_t attr_handle,
                     struct ble_gatt_access_ctxt *ctxt,
                     void *arg);

  static int oacpCb(uint16_t conn_handle,
                    uint16_t attr_handle,
                    struct ble_gatt_access_ctxt *ctxt,
                    void *arg);

  static int olcpCb(uint16_t conn_handle,
                    uint16_t attr_handle,
                    struct ble_gatt_access_ctxt *ctxt,
                    void *arg);

  // L2CAP CoC server event handler

  static int l2capEventCb(struct ble_l2cap_event *event, void *arg);
  void onL2capRx(struct ble_l2cap_event *event);

  // OACP / OLCP handlers (per connection state)

  int handleOacpRead(uint16_t conn, uint32_t off, uint32_t len);
  int handleOacpWrite(uint16_t conn, uint32_t off, uint32_t len, uint8_t mode);
  int handleOacpAbort(uint16_t conn);
  uint8_t oacpValidate(uint16_t conn, bool wantWrite, SOtsConnState *&st, CIOCommon *&io);

  int handleOlcpFirst(uint16_t conn);
  int handleOlcpLast(uint16_t conn);
  int handleOlcpPrev(uint16_t conn);
  int handleOlcpNext(uint16_t conn);
  int handleOlcpGoto(uint16_t conn, uint64_t obj_id);
  int olcpSeekAbs(uint16_t conn, uint8_t op, size_t idx);
  int olcpStep(uint16_t conn, uint8_t op, int delta);

  // Helpers

  void sendCpResponse(uint16_t conn,
                      uint16_t handle,
                      uint8_t respOp,
                      uint8_t reqOp,
                      uint8_t result);
  void sendOacpResponse(uint16_t conn, uint8_t req_op, uint8_t result);
  void sendOlcpResponse(uint16_t conn, uint8_t req_op, uint8_t result);

  static int metaPrologue(uint16_t conn,
                          ble_gatt_access_ctxt *ctxt,
                          void *arg,
                          CProtoNimblePrphOts **self,
                          SOtsObjectMeta **meta,
                          int16_t *cursorOut);
  int pumpRead(uint16_t conn);
  bool selected(uint16_t conn, SOtsConnState *&state, SOtsObjectMeta *&meta);

  static uint8_t cfgGetType(uint32_t cfg)
  {
    return (cfg >> CFG_TYPE_SHIFT) & CFG_TYPE_MASK;
  }

  static uint8_t cfgGetAccess(uint32_t cfg)
  {
    return (cfg >> CFG_ACCESS_SHIFT) & CFG_ACCESS_MASK;
  }

  static uint32_t cfgPack(uint8_t type, uint8_t access)
  {
    return ((type & CFG_TYPE_MASK) << CFG_TYPE_SHIFT) |
           ((access & CFG_ACCESS_MASK) << CFG_ACCESS_SHIFT);
  }
};

} // namespace dawn
