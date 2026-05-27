// dawn/src/io/factory.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/factory.hxx"
#include "dawn/io/common.hxx"

#ifdef CONFIG_DAWN_IO_ADC_FETCH
#  include "dawn/io/adc_fetch.hxx"
#endif
#ifdef CONFIG_DAWN_IO_ADC_SYNC
#  include "dawn/io/adc_sync.hxx"
#endif
#ifdef CONFIG_DAWN_IO_ADC_STREAM
#  include "dawn/io/adc_stream.hxx"
#endif
#ifdef CONFIG_DAWN_IO_CONFIG
#  include "dawn/io/config.hxx"
#endif
#ifdef CONFIG_DAWN_IO_CONTROL
#  include "dawn/io/control.hxx"
#endif
#ifdef CONFIG_DAWN_IO_TRIGGER
#  include "dawn/io/trigger.hxx"
#endif
#ifdef CONFIG_DAWN_IO_DUMMY
#  include "dawn/io/dummy.hxx"
#endif
#ifdef CONFIG_DAWN_IO_DUMMY_NOTIFY
#  include "dawn/io/dummy_notify.hxx"
#endif
#ifdef CONFIG_DAWN_IO_DAC
#  include "dawn/io/dac.hxx"
#endif
#ifdef CONFIG_DAWN_IO_ENCODER
#  include "dawn/io/encoder.hxx"
#endif
#ifdef CONFIG_DAWN_IO_ENCODER_INDEX
#  include "dawn/io/encoder_index.hxx"
#endif
#ifdef CONFIG_DAWN_IO_PWM
#  include "dawn/io/pwm.hxx"
#endif
#ifdef CONFIG_DAWN_IO_PULSECOUNT
#  include "dawn/io/pulsecount.hxx"
#endif
#ifdef CONFIG_DAWN_IO_DESCRIPTOR
#  include "dawn/io/descriptor.hxx"
#endif
#ifdef CONFIG_DAWN_IO_CAPABILITIES
#  include "dawn/io/capabilities.hxx"
#endif
#ifdef CONFIG_DAWN_IO_DESC_SELECTOR
#  include "dawn/io/descselector.hxx"
#endif
#ifdef CONFIG_DAWN_IO_FILE
#  include "dawn/io/fileio.hxx"
#endif
#ifdef CONFIG_DAWN_IO_GPO
#  include "dawn/io/gpo.hxx"
#endif
#ifdef CONFIG_DAWN_IO_GPI
#  include "dawn/io/gpi.hxx"
#endif
#ifdef CONFIG_DAWN_IO_BUTTONS
#  include "dawn/io/buttons.hxx"
#endif
#ifdef CONFIG_DAWN_IO_LEDS
#  include "dawn/io/leds.hxx"
#endif
#ifdef CONFIG_DAWN_IO_RGB_LED
#  include "dawn/io/rgbled.hxx"
#endif
#ifdef CONFIG_DAWN_IO_VIRT
#  include "dawn/io/virt.hxx"
#endif
#ifdef CONFIG_DAWN_IO_SENSOR
#  include "dawn/io/sensor.hxx"
#endif
#ifdef CONFIG_DAWN_IO_SENSOR_PRODUCER
#  include "dawn/io/sensor_producer.hxx"
#endif
#ifdef CONFIG_DAWN_IO_BOARDCTL
#  include "dawn/io/boardctl.hxx"
#endif
#ifdef CONFIG_DAWN_IO_SYSINFO
#  include "dawn/io/sysinfo.hxx"
#endif
#ifdef CONFIG_DAWN_IO_UNAME
#  include "dawn/io/uname.hxx"
#endif
#ifdef CONFIG_DAWN_IO_UUID
#  include "dawn/io/uuid.hxx"
#endif
#ifdef CONFIG_DAWN_IO_SYSTIME
#  include "dawn/io/systime.hxx"
#endif
#ifdef CONFIG_DAWN_IO_TIMESTAMPIO
#  include "dawn/io/timestamp.hxx"
#endif
#ifdef CONFIG_DAWN_IO_RANDIO
#  include "dawn/io/rand.hxx"
#endif

using namespace dawn;

CIOCommon *CIOFactory::create(CDescObject &desc)
{
  DEBUGASSERT(desc.getObjectType() == SObjectId::OBJTYPE_IO);

  // Create io object

  switch (desc.getObjectCls())
    {
#ifdef CONFIG_DAWN_IO_CONFIG
      case CIOCommon::IO_CLASS_CONFIG:
        return new CIOConfig(desc);
#endif

#ifdef CONFIG_DAWN_IO_CONTROL
      case CIOCommon::IO_CLASS_CONTROL:
        return new CIOControl(desc);
#endif

#ifdef CONFIG_DAWN_IO_TRIGGER
      case CIOCommon::IO_CLASS_TRIGGER:
        return new CIOTrigger(desc);
#endif

#ifdef CONFIG_DAWN_IO_DUMMY
      case CIOCommon::IO_CLASS_DUMMY:
        return new CIODummy(desc);
#endif

#ifdef CONFIG_DAWN_IO_DUMMY_NOTIFY
      case CIOCommon::IO_CLASS_DUMMY_NOTIFY:
        return new CIODummyNotify(desc);
#endif

#ifdef CONFIG_DAWN_IO_TIMESTAMPIO
      case CIOCommon::IO_CLASS_TIMESTAMP:
        return new CIOTimestamp(desc);
#endif

#ifdef CONFIG_DAWN_IO_RANDIO
      case CIOCommon::IO_CLASS_RAND:
        return new CIORand(desc);
#endif

#ifdef CONFIG_DAWN_IO_SYSINFO
      case CIOCommon::IO_CLASS_SYSTEM_UPTIME:
      case CIOCommon::IO_CLASS_SYSTEM_CPULOAD:
        return new CIOSysinfo(desc);
#endif

#ifdef CONFIG_DAWN_IO_BOARDCTL
      case CIOCommon::IO_CLASS_SYSTEM_RESET:
      case CIOCommon::IO_CLASS_SYSTEM_RESETCAUSE:
      case CIOCommon::IO_CLASS_SYSTEM_POWEROFF:
        return new CIOBoardctl(desc);
#endif

#ifdef CONFIG_DAWN_IO_UNAME
      case CIOCommon::IO_CLASS_SYSTEM_HOSTNAME:
        return new CIOUname(desc);
#endif

#ifdef CONFIG_DAWN_IO_UUID
      case CIOCommon::IO_CLASS_SYSTEM_UUID:
        return new CIOUuid(desc);
#endif

#ifdef CONFIG_DAWN_IO_SYSTIME
      case CIOCommon::IO_CLASS_SYSTEM_SYSTEMTIME:
        return new CIOSystime(desc);
#endif

#ifdef CONFIG_DAWN_IO_SENSOR
      case CIOCommon::IO_CLASS_SENSOR_ACCELEROMETER:
      case CIOCommon::IO_CLASS_SENSOR_MAGNETICFIELD:
      case CIOCommon::IO_CLASS_SENSOR_GYROSCOPE:
      case CIOCommon::IO_CLASS_SENSOR_LIGHT:
      case CIOCommon::IO_CLASS_SENSOR_BAROMETER:
      case CIOCommon::IO_CLASS_SENSOR_PROXIMITY:
      case CIOCommon::IO_CLASS_SENSOR_HUMIDITY:
      case CIOCommon::IO_CLASS_SENSOR_TEMPERATURE:
      case CIOCommon::IO_CLASS_SENSOR_ATEMPERATURE:
      case CIOCommon::IO_CLASS_SENSOR_RGB:
      case CIOCommon::IO_CLASS_SENSOR_IR:
      case CIOCommon::IO_CLASS_SENSOR_UV:
      case CIOCommon::IO_CLASS_SENSOR_GAS:
        return new CIOSensor(desc);
#endif

#ifdef CONFIG_DAWN_IO_SENSOR_PRODUCER
      case CIOCommon::IO_CLASS_SENSOR_PRODUCER_ACCELEROMETER:
      case CIOCommon::IO_CLASS_SENSOR_PRODUCER_MAGNETICFIELD:
      case CIOCommon::IO_CLASS_SENSOR_PRODUCER_GYROSCOPE:
      case CIOCommon::IO_CLASS_SENSOR_PRODUCER_LIGHT:
      case CIOCommon::IO_CLASS_SENSOR_PRODUCER_BAROMETER:
      case CIOCommon::IO_CLASS_SENSOR_PRODUCER_PROXIMITY:
      case CIOCommon::IO_CLASS_SENSOR_PRODUCER_HUMIDITY:
      case CIOCommon::IO_CLASS_SENSOR_PRODUCER_TEMPERATURE:
      case CIOCommon::IO_CLASS_SENSOR_PRODUCER_ATEMPERATURE:
      case CIOCommon::IO_CLASS_SENSOR_PRODUCER_RGB:
      case CIOCommon::IO_CLASS_SENSOR_PRODUCER_IR:
      case CIOCommon::IO_CLASS_SENSOR_PRODUCER_UV:
      case CIOCommon::IO_CLASS_SENSOR_PRODUCER_GAS:
        return new CIOSensorProducer(desc);
#endif

#ifdef CONFIG_DAWN_IO_ADC_FETCH
      case CIOCommon::IO_CLASS_ADC_FETCH:
        return new CIOAdcFetch(desc);
#endif

#ifdef CONFIG_DAWN_IO_ADC_SYNC
      case CIOCommon::IO_CLASS_ADC_SYNC:
        return new CIOAdcSync(desc);
#endif

#ifdef CONFIG_DAWN_IO_ADC_STREAM
      case CIOCommon::IO_CLASS_ADC_STREAM:
        return new CIOAdcStream(desc);
#endif

#ifdef CONFIG_DAWN_IO_DAC
      case CIOCommon::IO_CLASS_DAC:
        return new CIODac(desc);
#endif

#ifdef CONFIG_DAWN_IO_ENCODER
      case CIOCommon::IO_CLASS_ENCODER:
        return new CIOEncoder(desc);
#endif

#ifdef CONFIG_DAWN_IO_ENCODER_INDEX
      case CIOCommon::IO_CLASS_ENCODER_INDEX:
        return new CIOEncoderIndex(desc);
#endif

#ifdef CONFIG_DAWN_IO_PWM
      case CIOCommon::IO_CLASS_PWM:
        return new CIOPwm(desc);
#endif

#ifdef CONFIG_DAWN_IO_PULSECOUNT
      case CIOCommon::IO_CLASS_PULSECOUNT:
        return new CIOPulseCount(desc);
#endif

#ifdef CONFIG_DAWN_IO_DESCRIPTOR
      case CIOCommon::IO_CLASS_DESCRIPTOR:
        return new CIODescriptor(desc);
#endif

#ifdef CONFIG_DAWN_IO_CAPABILITIES
      case CIOCommon::IO_CLASS_CAPABILITIES:
        return new CIOCapabilities(desc);
#endif

#ifdef CONFIG_DAWN_IO_DESC_SELECTOR
      case CIOCommon::IO_CLASS_DESC_SELECTOR:
        return new CIODescSelector(desc);
#endif

#ifdef CONFIG_DAWN_IO_FILE
      case CIOCommon::IO_CLASS_FILE:
        return new CIOFile(desc);
#endif

#ifdef CONFIG_DAWN_IO_GPO
      case CIOCommon::IO_CLASS_GPO_SINGLE:
        return new CIOGpo(desc);
#endif

#ifdef CONFIG_DAWN_IO_GPI
      case CIOCommon::IO_CLASS_GPI_SINGLE:
        return new CIOGpi(desc);
#endif

#ifdef CONFIG_DAWN_IO_BUTTONS
      case CIOCommon::IO_CLASS_BUTTONS:
        return new CIOButtons(desc);
#endif

#ifdef CONFIG_DAWN_IO_LEDS
      case CIOCommon::IO_CLASS_LEDS:
        return new CIOLeds(desc);
#endif

#ifdef CONFIG_DAWN_IO_RGB_LED
      case CIOCommon::IO_CLASS_RGBLED:
        return new CIORgbLed(desc);
#endif

#ifdef CONFIG_DAWN_IO_VIRT
      case CIOCommon::IO_CLASS_VIRT:
        return new CIOVirt(desc);
#endif

      default:
        {
          DAWNERR("Unknown IO class %d\n", desc.getObjectCls());
          return nullptr;
        }
    }
}
