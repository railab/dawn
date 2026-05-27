# dawn/tests/io/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

set(SRCS test_data.cxx test_common.cxx test_handler.cxx test_run.cxx
         test_factory.cxx)

if(CONFIG_DAWN_IO_LIMITS)
  list(APPEND SRCS test_limits.cxx)
endif()

if(CONFIG_DAWN_IO_NOTIFY)
  list(APPEND SRCS test_notifier.cxx)
endif()

if(CONFIG_DAWN_IO_CONFIG)
  list(APPEND SRCS test_config.cxx)
endif()

if(CONFIG_DAWN_IO_CONTROL)
  list(APPEND SRCS test_control.cxx)
endif()

if(CONFIG_DAWN_IO_TRIGGER)
  list(APPEND SRCS test_trigger.cxx)
endif()

if(CONFIG_DAWN_IO_DUMMY)
  list(APPEND SRCS test_dummy.cxx)
endif()

if(CONFIG_DAWN_IO_DUMMY_NOTIFY)
  list(APPEND SRCS test_dummy_notify.cxx)
endif()

if(CONFIG_DAWN_IO_VIRT)
  list(APPEND SRCS test_virt.cxx)
endif()

if(CONFIG_DAWN_IO_SYSINFO)
  list(APPEND SRCS test_sysinfo.cxx)
endif()

if(CONFIG_DAWN_IO_BOARDCTL)
  list(APPEND SRCS test_boardctl.cxx)
endif()

if(CONFIG_DAWN_IO_UNAME)
  list(APPEND SRCS test_uname.cxx)
endif()

if(CONFIG_DAWN_IO_UUID)
  list(APPEND SRCS test_uuid.cxx)
endif()

if(CONFIG_DAWN_IO_SYSTIME)
  list(APPEND SRCS test_systime.cxx)
endif()

if(CONFIG_DAWN_IO_SENSOR)
  list(APPEND SRCS test_sensor.cxx)
endif()

if(CONFIG_DAWN_IO_SENSOR_PRODUCER)
  list(APPEND SRCS test_sensor_producer.cxx)
endif()

if(CONFIG_DAWN_IO_ADC)
  list(APPEND SRCS test_adc.cxx)
endif()

if(CONFIG_DAWN_IO_DAC)
  list(APPEND SRCS test_dac.cxx)
endif()

if(CONFIG_DAWN_IO_ENCODER)
  list(APPEND SRCS test_encoder.cxx)
endif()

if(CONFIG_DAWN_IO_ENCODER_INDEX)
  list(APPEND SRCS test_encoder_index.cxx)
endif()

if(CONFIG_DAWN_IO_GPI)
  list(APPEND SRCS test_gpi.cxx)
endif()

if(CONFIG_DAWN_IO_GPO)
  list(APPEND SRCS test_gpo.cxx)
endif()

if(CONFIG_DAWN_IO_BUTTONS)
  list(APPEND SRCS test_buttons.cxx)
endif()

if(CONFIG_DAWN_IO_LEDS)
  list(APPEND SRCS test_leds.cxx)
endif()

if(CONFIG_DAWN_IO_RGB_LED)
  list(APPEND SRCS test_rgbled.cxx)
endif()

if(CONFIG_DAWN_IO_PWM)
  list(APPEND SRCS test_pwm.cxx)
endif()

if(CONFIG_DAWN_IO_PULSECOUNT)
  list(APPEND SRCS test_pulsecount.cxx)
endif()

if(CONFIG_DAWN_IO_TIMERFD)
  list(APPEND SRCS test_timerfd.cxx)
endif()

if(CONFIG_DAWN_IO_TIMESTAMP)
  list(APPEND SRCS test_timestamp.cxx)
endif()

if(CONFIG_DAWN_IO_RANDIO)
  list(APPEND SRCS test_rand.cxx)
endif()

if(CONFIG_DAWN_IO_DESCRIPTOR)
  list(APPEND SRCS test_descriptor.cxx)
endif()

if(CONFIG_DAWN_IO_CAPABILITIES)
  list(APPEND SRCS test_capabilities.cxx)
endif()

if(CONFIG_DAWN_IO_DESC_SELECTOR)
  list(APPEND SRCS test_descselector.cxx)
endif()

if(CONFIG_DAWN_IO_FILE)
  list(APPEND SRCS test_fileio.cxx)
endif()

target_sources(dawn_tests PRIVATE ${SRCS})
