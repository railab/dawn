// dawn/src/proto/factory.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/factory.hxx"

#ifdef CONFIG_DAWN_PROTO_DUMMY
#  include "dawn/proto/dummy.hxx"
#endif

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_DUMMY
#  include "dawn/proto/nxscope/dummy.hxx"
#endif
#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SERIAL
#  include "dawn/proto/nxscope/serial.hxx"
#endif
#ifdef CONFIG_DAWN_PROTO_NXSCOPE_UDP
#  include "dawn/proto/nxscope/udp.hxx"
#endif
#ifdef CONFIG_DAWN_PROTO_SHELL_PRETTY
#  include "dawn/proto/shell/pretty.hxx"
#endif
#ifdef CONFIG_DAWN_PROTO_SERIAL
#  include "dawn/proto/serial/simple.hxx"
#endif
#ifdef CONFIG_DAWN_PROTO_UDP
#  include "dawn/proto/udp/simple.hxx"
#endif
#ifdef CONFIG_DAWN_PROTO_IPC
#  include "dawn/proto/ipc/simple.hxx"
#endif
#ifdef CONFIG_DAWN_PROTO_NIMBLE_PERIPHERAL
#  include "dawn/proto/nimble/prph.hxx"
#endif
#ifdef CONFIG_DAWN_PROTO_CAN
#  include "dawn/proto/can/can.hxx"
#endif
#ifdef CONFIG_DAWN_PROTO_MODBUS_RTU
#  include "dawn/proto/modbus/rtu.hxx"
#endif
#ifdef CONFIG_DAWN_PROTO_MODBUS_TCP
#  include "dawn/proto/modbus/tcp.hxx"
#endif
#ifdef CONFIG_DAWN_PROTO_WAKAAMA
#  include "dawn/proto/wakaama/wakaama.hxx"
#endif

using namespace dawn;

CProtoCommon *CProtoFactory::create(CDescObject &desc)
{
  DEBUGASSERT(desc.getObjectType() == SObjectId::OBJTYPE_PROTO);

  switch (desc.getObjectCls())
    {
#ifdef CONFIG_DAWN_PROTO_DUMMY
      case CProtoCommon::PROTO_CLASS_DUMMY:
        return new CProtoDummy(desc);
#endif

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_DUMMY
      case CProtoCommon::PROTO_CLASS_NXSCOPE_DUMMY:
        return new CProtoNxscopeDummy(desc);
#endif

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SERIAL
      case CProtoCommon::PROTO_CLASS_NXSCOPE_SERIAL:
        return new CProtoNxscopeSerial(desc);
#endif

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_UDP
      case CProtoCommon::PROTO_CLASS_NXSCOPE_UDP:
        return new CProtoNxscopeUdp(desc);
#endif

#ifdef CONFIG_DAWN_PROTO_SHELL_PRETTY
      case CProtoCommon::PROTO_CLASS_SHELL_STD:
        return new CProtoShellPretty(desc);
#endif

#ifdef CONFIG_DAWN_PROTO_SERIAL
      case CProtoCommon::PROTO_CLASS_SERIAL:
        return new CProtoSerial(desc);
#endif

#ifdef CONFIG_DAWN_PROTO_UDP
      case CProtoCommon::PROTO_CLASS_UDP:
        return new CProtoUdp(desc);
#endif

#ifdef CONFIG_DAWN_PROTO_IPC
      case CProtoCommon::PROTO_CLASS_IPC:
        return new CProtoIpc(desc);
#endif

#ifdef CONFIG_DAWN_PROTO_NIMBLE_PERIPHERAL
      case CProtoCommon::PROTO_CLASS_NIMBLE_PRPH:
        return new CProtoNimblePrph(desc);
#endif

#ifdef CONFIG_DAWN_PROTO_CAN
      case CProtoCommon::PROTO_CLASS_CAN:
        return new CProtoCan(desc);
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_RTU
      case CProtoCommon::PROTO_CLASS_MODBUS_RTU:
        return new CProtoModbusRtu(desc);
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_TCP
      case CProtoCommon::PROTO_CLASS_MODBUS_TCP:
        return new CProtoModbusTcp(desc);
#endif

#ifdef CONFIG_DAWN_PROTO_WAKAAMA
      case CProtoCommon::PROTO_CLASS_WAKAAMA:
        return new CProtoWakaama(desc);
#endif

      default:
        {
          DAWNERR("Unknown PROTO class %d\n", desc.getObjectCls());
          return nullptr;
        }
    }
}
