=====================
Resource Optimization
=====================

This page will describe how to optimize Dawn for small systems with low
FLASH, RAM, and CPU budgets.

For now, use the Apache NuttX documentation as the primary reference:

* `NuttX documentation <https://nuttx.apache.org/docs/latest>`__

NuttX tuning is also a major part of optimizing Dawn-based firmware. Dawn
follows the same general approach used for memory-constrained NuttX systems:
disable as many unused framework and operating-system features as possible,
then enable only the pieces required by the target application.

Dawn configuration
==================

Many Dawn features can be disabled through Kconfig. Ideally, every optional
feature should be configurable so users can fine tune the framework for a
specific target and application.

Dawn uses a factory model for IO, Program, and Protocol object creation. This
keeps the framework modular, but it can prevent the compiler and linker from
optimizing unused code as aggressively as they could in a single-purpose
firmware. For small systems, it is therefore the user's responsibility to tune
the Dawn configuration for the required feature set.

In the future, this process should be automated, or at least assisted, by
``dawnpy`` tooling.

Future Dawn-specific content should cover:

* choosing minimal board configurations,
* disabling unused Dawn IO, Program, and Protocol classes,
* reducing shell, logging, debug, and protocol features,
* measuring binary size and RAM use, and
* maintaining small reference configurations.
