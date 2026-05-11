// dawn/src/porting/nuttx/config.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/config.hxx"

//***************************************************************************
// Pre-processor Definitions
//***************************************************************************

// System checks ************************************************************

#ifndef CONFIG_LIBCXX
#  error CONFIG_LIBCXX must be enabled
#endif

#ifndef CONFIG_BOARDCTL
#  error CONFIG_BOARDCTL must be enabled
#endif

#ifndef CONFIG_LIBCXXABI
#  error CONFIG_LIBCXXABI must be enabled
#endif

#ifdef CONFIG_ENDIAN_BIG
#  error BIG ENDIAN not supported
#endif

// Tests checks *************************************************************

#ifdef CONFIG_DAWN_TESTS
#  ifndef CONFIG_TESTING_UNITY
#    error CONFIG_TESTING_UNITY is required
#  endif
#endif

// Net checks ***************************************************************

#if defined(CONFIG_NET) && !defined(CONFIG_NETUTILS_NETINIT)
#  error CONFIG_NETUTILS_NETINIT is required
#endif

// IO checks ****************************************************************

#ifdef CONFIG_DAWN_IO_ADC
#  ifndef CONFIG_ADC
#    error ADC driver is required
#  endif
#endif

#if defined(CONFIG_DAWN_IO_GPI) || defined(CONFIG_DAWN_IO_GPO)
#  ifndef CONFIG_DEV_GPIO
#    error GPIO driver is required
#  endif
#endif

#ifdef CONFIG_DAWN_IO_PWM
#  ifndef CONFIG_PWM
#    error CONFIG_PWM is required
#  endif
#  ifndef CONFIG_PWM_MULTICHAN
#    error CONFIG_PWM_MULTICHAN is required
#  endif
#endif

#ifdef CONFIG_DAWN_IO_SENSOR
#  ifndef CONFIG_SENSORS
#    error sensor driver is required
#  endif
#endif

#ifdef CONFIG_DAWN_IO_SENSOR_PRODUCER
#  ifndef CONFIG_USENSOR
#    error usensor driver is required
#  endif
#endif

#if defined(CONFIG_DAWN_IO_ENCODER) || defined(CONFIG_DAWN_IO_ENCODER_INDEX)
#  ifndef CONFIG_SENSORS_QENCODER
#    error CONFIG_SENSORS_QENCODER is required for encoder io
#  endif
#endif

#ifdef CONFIG_DAWN_IO_LEDS
#  ifndef CONFIG_USERLED
#    error userled driver is required
#  endif
#endif

#ifdef CONFIG_DAWN_IO_BUTTONS
#  ifndef CONFIG_INPUT_BUTTONS
#    error buttons driver is required
#  endif
#endif

#ifdef CONFIG_DAWN_IO_TIMERFD
#  ifdef CONFIG_DAWN_IO_NOTIFY
#    if !defined(CONFIG_TIMER_FD) || !defined(CONFIG_TIMER_FD_POLL)
#      error CONFIG_TIMER_FD is required for timerfd notifications
#    endif
#  endif
#endif

#ifdef CONFIG_DAWN_IO_RANDIO
#  ifndef CONFIG_DEV_URANDOM
#    error CONFIG_DEV_URANDOM is required for randio
#  endif
#endif

#ifdef CONFIG_DAWN_IO_UUID
#  ifndef CONFIG_BOARDCTL_UNIQUEID
#    error CONFIG_BOARDCTL_UNIQUEID is required for uuidio
#  endif
#endif

// Proto checks *************************************************************

#ifdef CONFIG_DAWN_PROTO_SHELL
#  ifndef CONFIG_SYSTEM_READLINE
#    error PROTO_SHELL requires CONFIG_SYSTEM_READLINE
#  endif
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS
#  ifndef CONFIG_INDUSTRY_NXMODBUS
#    error PROTO_MODBUS requires CONFIG_INDUSTRY_NXMODBUS
#  endif
#endif

#ifdef CONFIG_DAWN_PROTO_NIMBLE
#  ifdef CONFIG_WIRELESS_BLUETOOTH_HOST
#    error PROTO_NIMBLE doesnt work with CONFIG_WIRELESS_BLUETOOTH_HOST
#  endif
#  ifndef CONFIG_NIMBLE
#    error PROTO_NIMBLE requires CONFIG_NIMBLE
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_PERIPHERAL
#    ifndef CONFIG_NIMBLE_ROLE_PERIPHERAL
#      error CONFIG_NIMBLE_ROLE_PERIPHERAL must be selected
#    endif
#  endif

#endif

#ifdef CONFIG_DAWN_PROTO_NXSCOPE
#  ifndef CONFIG_LOGGING_NXSCOPE
#    error PROTO_NXSCOPE requires CONFIG_LOGGING_NXSCOPE
#  endif

#  ifdef CONFIG_DAWN_PROTO_NXSCOPE_DUMMY
#    ifndef CONFIG_LOGGING_NXSCOPE_INTF_DUMMY
#      error CONFIG_LOGGING_NXSCOPE_INTF_DUMMY must be set
#    endif
#    ifndef CONFIG_LOGGING_NXSCOPE_PROTO_SER
#      error CONFIG_LOGGING_NXSCOPE_PROTO_SER must be set
#    endif
#  endif

#  ifdef CONFIG_DAWN_PROTO_NXSCOPE_SERIAL
#    ifndef CONFIG_LOGGING_NXSCOPE_INTF_SERIAL
#      error CONFIG_LOGGING_NXSCOPE_INTF_SERIAL must be selected
#    endif
#    ifndef CONFIG_LOGGING_NXSCOPE_PROTO_SER
#      error CONFIG_LOGGING_NXSCOPE_PROTO_SER must be selected
#    endif
#  endif

#  ifdef CONFIG_DAWN_PROTO_CAN
#    ifndef CONFIG_CAN
#      error CONFIG_CAN must be selected
#    endif
#    ifdef CONFIG_DAWN_PROTO_CAN_EXTID
#      ifndef CONFIG_CAN_EXTID
#        error CONFIG_CAN_EXTID must be selected
#      endif
#    endif
#    ifdef CONFIG_DAWN_PROTO_CAN_CANFD
#      ifndef CONFIG_CAN_CANFD
#        error CONFIG_CAN_CANFD must be selected
#      endif
#    endif
#  endif

#endif

// Progs checks *************************************************************
