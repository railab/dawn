// dawn/apps/dawn/dawn_descriptor.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/config.hxx"

// Include descriptor file

#ifdef CONFIG_DAWN_APPS_EXAMPLE_DESC_FORMAT_YAML
#  include DAWN_APPS_EXAMPLE_DESC_GENERATED_PATH
#else
#  include CONFIG_DAWN_APPS_EXAMPLE_DESC_PATH
#endif
