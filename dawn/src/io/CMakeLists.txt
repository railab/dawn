# dawn/src/io/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

nuttx_add_subdirectory()

set(SRCS common.cxx handler.cxx factory.cxx)

if(CONFIG_DAWN_IO_LIMITS)
  list(APPEND SRCS limits.cxx)
endif()

if(CONFIG_DAWN_IO_TIMERFD)
  list(APPEND SRCS timerfd.cxx)
endif()

if(CONFIG_DAWN_IO_NOTIFY)
  list(APPEND SRCS notifier.cxx notify_manager.cxx)
endif()

if(CONFIG_DAWN_IO_NOTIFY_STREAM)
  list(APPEND SRCS stream_notifier.cxx)
endif()

if(CONFIG_DAWN_IO_CONFIG)
  list(APPEND SRCS config.cxx)
endif()

if(CONFIG_DAWN_IO_CONTROL)
  list(APPEND SRCS control.cxx)
endif()

if(CONFIG_DAWN_IO_TRIGGER)
  list(APPEND SRCS trigger.cxx)
endif()

if(CONFIG_DAWN_IO_DUMMY)
  list(APPEND SRCS dummy.cxx)
endif()

if(CONFIG_DAWN_IO_DUMMY_NOTIFY)
  list(APPEND SRCS dummy_notify.cxx)
endif()

if(CONFIG_DAWN_IO_VIRT)
  list(APPEND SRCS virt.cxx)
endif()

if(CONFIG_DAWN_IO_SENSOR)
  list(APPEND SRCS sensor.cxx)
endif()

if(CONFIG_DAWN_IO_SENSOR_PRODUCER)
  list(APPEND SRCS sensor_producer.cxx)
endif()

if(CONFIG_DAWN_IO_SYSINFO)
  list(APPEND SRCS sysinfo.cxx)
endif()

if(CONFIG_DAWN_IO_BOARDCTL)
  list(APPEND SRCS boardctl.cxx)
endif()

if(CONFIG_DAWN_IO_UNAME)
  list(APPEND SRCS uname.cxx)
endif()

if(CONFIG_DAWN_IO_UUID)
  list(APPEND SRCS uuid.cxx)
endif()

if(CONFIG_DAWN_IO_SYSTIME)
  list(APPEND SRCS systime.cxx)
endif()

if(CONFIG_DAWN_IO_ADC)
  list(APPEND SRCS adc_base.cxx)
endif()

if(CONFIG_DAWN_IO_ADC_FETCH)
  list(APPEND SRCS adc_fetch.cxx)
endif()

if(CONFIG_DAWN_IO_ADC_SYNC)
  list(APPEND SRCS adc_sync.cxx)
endif()

if(CONFIG_DAWN_IO_ADC_STREAM)
  list(APPEND SRCS adc_stream.cxx)
endif()

if(CONFIG_DAWN_IO_DAC)
  list(APPEND SRCS dac.cxx)
endif()

if(CONFIG_DAWN_IO_ENCODER)
  list(APPEND SRCS encoder.cxx)
endif()

if(CONFIG_DAWN_IO_ENCODER_INDEX)
  list(APPEND SRCS encoder_index.cxx)
endif()

if(CONFIG_DAWN_IO_GPI)
  list(APPEND SRCS gpi.cxx)
endif()

if(CONFIG_DAWN_IO_GPO)
  list(APPEND SRCS gpo.cxx)
endif()

if(CONFIG_DAWN_IO_PWM)
  list(APPEND SRCS pwm.cxx)
endif()

if(CONFIG_DAWN_IO_PULSECOUNT)
  list(APPEND SRCS pulsecount.cxx)
endif()

if(CONFIG_DAWN_IO_CONFIG)
  list(APPEND SRCS config.cxx)
endif()

if(CONFIG_DAWN_IO_BUTTONS)
  list(APPEND SRCS buttons.cxx)
endif()

if(CONFIG_DAWN_IO_LEDS)
  list(APPEND SRCS leds.cxx)
endif()

if(CONFIG_DAWN_IO_RGB_LED)
  list(APPEND SRCS rgbled.cxx)
endif()

if(CONFIG_DAWN_IO_TIMESTAMPIO)
  list(APPEND SRCS timestamp.cxx)
endif()

if(CONFIG_DAWN_IO_RANDIO)
  list(APPEND SRCS rand.cxx)
endif()

if(CONFIG_DAWN_IO_DESCRIPTOR)
  list(APPEND SRCS descriptor.cxx)
endif()

if(CONFIG_DAWN_IO_CAPABILITIES)
  list(APPEND SRCS capabilities.cxx)
endif()

if(CONFIG_DAWN_IO_DESC_SELECTOR)
  list(APPEND SRCS descselector.cxx)
endif()

if(CONFIG_DAWN_IO_FILE)
  list(APPEND SRCS fileio.cxx)
endif()

target_sources(dawn PRIVATE ${SRCS})
