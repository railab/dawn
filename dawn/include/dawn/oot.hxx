// dawn/include/dawn/oot.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/factory.hxx"
#include "dawn/prog/factory.hxx"
#include "dawn/proto/factory.hxx"

/**
 * @brief Out-of-tree user-extension hooks for Dawn.
 *
 * Two groups of free functions form the entire user-extension API:
 *
 *   * Factory hooks: apps_dawn calls these at startup and passes the results
 *     into CDawn. Built-in factories handle anything the user factories
 *     return nullptr for.
 *
 *   * Lifecycle hooks: optional callbacks invoked at well-known points in
 *     the default app and the CDawn main loop. Default impls are no-ops
 *     (or, for user_on_idle, just sleep(1) so cadence is unchanged).
 *
 * Default implementations in dawn/src/oot.cxx are weak. Out-of-tree projects
 * override them in <oot>/external/dawn_oot_hooks.cxx (compiled into the
 * application target by dawn/apps/dawn/CMakeLists.txt). Strong overrides
 * beat weak defaults during the final link.
 */

namespace dawn
{
class CDawn;

namespace oot
{

// Factory hooks.

IIOFactory *user_io_factory();
IProgFactory *user_prog_factory();
IProtoFactory *user_proto_factory();

/**
 * @brief Lifecycle hooks.
 *
 *   * user_init - Called once after dawn_board_init() succeeds, before
 *     any descriptor work. Negative return aborts startup.
 *
 *   * user_post_load - Called after CDawn::load_descriptor() succeeds, before
 *     CDawn::start(). Negative return aborts startup.
 *
 *   * user_on_idle - Called from CDawn::start()'s main loop in place of
 *     sleep(1). Hook owns the cadence, so it must return
 *     promptly (the default sleeps 1 s) -- otherwise
 *     shutdown / descriptor-switch flags will not be
 *     re-checked in time.
 *
 *   * user_pre_shutdown - Called from CDawn::start() after the main loop exits,
 *     before the framework starts tearing handlers down.
 *     Only present when CONFIG_DAWN_LIFECYCLE_TEARDOWN=y.
 *     Negative return is logged but does not abort teardown.
 *
 * user_init / user_post_load are tied to the default apps_dawn entry point.
 * user_on_idle / user_pre_shutdown fire from CDawn::start() and therefore
 * also apply to any external app that drives CDawn directly.
 */

int user_init();
int user_post_load(CDawn *dawn);
void user_on_idle(CDawn *dawn);
int user_pre_shutdown(CDawn *dawn);

} // namespace oot
} // namespace dawn
