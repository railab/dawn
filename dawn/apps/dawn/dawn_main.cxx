// dawn/apps/dawn/dawn_main.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/board.hxx"
#include "dawn/porting/config.hxx"

#include "dawn/dawn.hxx"
#include "dawn/dev/descriptor.hxx"
#include "dawn/oot.hxx"

#include "dawn/debug.hxx"

#include "dawn_debug.hxx"
#include "dawn_parseargs.hxx"

using namespace dawn;

#if defined(CONFIG_DAWN_DESC_SLOTS) && CONFIG_DAWN_DESC_SLOTS > 1
#  define DAWN_DESC_CRC_REQUIRED 1
#elif defined(CONFIG_DAWN_DESC_VALID_CRC32)
#  define DAWN_DESC_CRC_REQUIRED 1
#else
#  define DAWN_DESC_CRC_REQUIRED 0
#endif

// descriptor

extern uint32_t g_dawn_desc[];
extern size_t g_dawn_desc_size;

// Weak externs for FLASH-baked extra slots (multi-descriptor YAML).
// Resolve to null/0 when the generated descriptor has only one slot.

extern uint32_t *const g_dawn_flash_descs[] __attribute__((weak));
extern const size_t g_dawn_flash_desc_sizes[] __attribute__((weak));
extern const size_t g_dawn_flash_desc_count __attribute__((weak));

// Default args

static struct args_s g_args = {
  // for future usage
};

// dawn_main

extern "C" int main(int argc, char *argv[])
{
  CDevDescriptor::SDescriptorReg dreg = {};
  CDawn *pDawn = nullptr;
  CDevDescriptor *devdesc = nullptr;
  uint32_t *boot_desc_bin = nullptr;
  size_t boot_desc_len = 0;
  uint32_t *desc_bin = nullptr;
  size_t desc_len = 0;
  size_t flash_desc_count = 0;
  size_t desc_slot = 0;
  int ret = OK;

  // create dawn handler

  pDawn = new CDawn(
    dawn::oot::user_io_factory(), dawn::oot::user_prog_factory(), dawn::oot::user_proto_factory());

  // Parse command-line arguments

#ifdef CONFIG_BUILTIN
  // Parse the command line

  parse_args(&g_args, argc, argv);
#endif

  // Validate arguments

  ret = validate_args(&g_args);
  if (ret < 0)
    {
      DAWNERR("ERROR: validate args failed %d\n", ret);
      goto errout;
    }

  // Board initialize

  ret = dawn_board_init();
  if (ret < 0)
    {
      DAWNERR("ERROR: dawn_board_init failed %d\n", ret);
      goto errout;
    }

  // User-supplied early init hook (weak default is a no-op)

  ret = dawn::oot::user_init();
  if (ret < 0)
    {
      DAWNERR("ERROR: user_init failed %d\n", ret);
      goto errout;
    }

  // Initialize descriptor slot 0 (boot descriptor)

  boot_desc_bin = g_dawn_desc;
  boot_desc_len = g_dawn_desc_size;
  desc_bin = boot_desc_bin;
  desc_len = boot_desc_len;

#if DAWN_DESC_CRC_REQUIRED
  // Fill with check sum for now
  ret = CDescriptor::binCheckFill(desc_bin, desc_len);
  if (ret < 0)
    {
      DAWNERR("binCheckFill %d\n", ret);
      goto errout;
    }
#endif

  devdesc = CDevDescriptor::getInst();
  dreg.ptr = desc_bin;
  dreg.len = desc_len;
  ret = devdesc->regDescriptor(0, dreg);
  if (ret < 0)
    {
      DAWNERR("regDescriptor(slot0) %d\n", ret);
      goto errout;
    }

  // Register additional FLASH descriptor slots (multi-descriptor YAML).
  // Weak symbol addresses resolve to null when the generated descriptor
  // has only one slot.

  if (&g_dawn_flash_desc_count != nullptr && &g_dawn_flash_descs != nullptr &&
      &g_dawn_flash_desc_sizes != nullptr)
    {
      flash_desc_count = g_dawn_flash_desc_count;
    }

  for (desc_slot = 0; desc_slot < flash_desc_count; desc_slot++)
    {
      desc_bin = g_dawn_flash_descs[desc_slot];
      desc_len = g_dawn_flash_desc_sizes[desc_slot];

#if DAWN_DESC_CRC_REQUIRED
      ret = CDescriptor::binCheckFill(desc_bin, desc_len);
      if (ret < 0)
        {
          DAWNERR("binCheckFill slot%zu %d\n", desc_slot + 1, ret);
          goto errout;
        }
#endif

      dreg.ptr = desc_bin;
      dreg.len = desc_len;
      ret = devdesc->regDescriptor(desc_slot + 1, dreg);
      if (ret < 0)
        {
          DAWNERR("regDescriptor(slot%zu) %d\n", desc_slot + 1, ret);
          goto errout;
        }
    }

  // Load device descriptor

  DAWNINFO("load descriptor\n");

  ret = pDawn->load_descriptor(boot_desc_bin, boot_desc_len);
  if (ret < 0)
    {
      DAWNERR("Dawn->load_config %d\n", ret);
      goto errout;
    }

  // User-supplied post-load hook (weak default is a no-op).
  // Objects exist and are wired up; framework has not started yet.

  ret = dawn::oot::user_post_load(pDawn);
  if (ret < 0)
    {
      DAWNERR("ERROR: user_post_load failed %d\n", ret);
      goto errout;
    }

  // Start

  ret = pDawn->start();
  if (ret < 0)
    {
      DAWNERR("dawn->start failed %d\n", ret);
      goto errout;
    }

errout:
  delete pDawn;
  return 0;
}
