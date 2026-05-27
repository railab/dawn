set(SRCS)

if(CONFIG_DAWN_FAKE_ADC)
  list(APPEND SRCS fake_adc.c)
endif()

if(CONFIG_DAWN_FAKE_DAC)
  list(APPEND SRCS fake_dac.c)
endif()

if(CONFIG_DAWN_FAKE_PWM)
  list(APPEND SRCS fake_pwm.c)
endif()

if(CONFIG_DAWN_FAKE_PULSECOUNT)
  list(APPEND SRCS fake_pulsecount.c)
endif()

if(CONFIG_DAWN_FAKE_ENCODER)
  list(APPEND SRCS fake_encoder.c)
endif()

if(CONFIG_DAWN_FAKE_IOEXPANDER)
  list(APPEND SRCS fake_ioexpander.c)
endif()

if(CONFIG_DAWN_FAKE_SENSORS)
  list(APPEND SRCS fake_sensors.c fakesensor2_uorb.c)
endif()

if(CONFIG_DAWN_FAKE_USERLEDS)
  list(APPEND SRCS fake_userleds.c)
endif()

if(CONFIG_DAWN_FAKE_RGBLED)
  list(APPEND SRCS fake_rgbled.c)
endif()

if(CONFIG_DAWN_FAKE_BUTTONS)
  list(APPEND SRCS fake_buttons.c)
endif()

if(CONFIG_DAWN_FAKE_UID)
  list(APPEND SRCS fake_uid.c)
endif()

if(CONFIG_DAWN_FAKE_RESET)
  list(APPEND SRCS fake_reset.c)
endif()

if(CONFIG_DAWN_FAKE_POWEROFF)
  list(APPEND SRCS fake_poweroff.c)
endif()

if(CONFIG_DAWN_FAKE_FILES)
  list(APPEND SRCS fake_files.c)
endif()

if(SRCS)
  target_sources(board_common PRIVATE ${SRCS})
endif()
