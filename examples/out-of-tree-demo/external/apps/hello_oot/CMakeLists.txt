# examples/out-of-tree-demo/external/apps/hello_oot/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_DAWN_OOT_DEMO_APP_HELLO)
  nuttx_add_application(
    NAME hello_oot
    SRCS hello_oot_main.c
    STACKSIZE ${CONFIG_DAWN_OOT_DEMO_APP_HELLO_STACKSIZE})
endif()
