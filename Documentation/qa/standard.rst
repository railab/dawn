===============
Coding Standard
===============

The rules at the moment are:

* Project written in C++, only the porting layer, external libraries and tests
  are allowed to use C. I'm not a fan of C++, but for this particular project,
  object-oriented design is strongly recommended.

* Only light C++ features allowed.

* No C++ exceptions allowed.

* CMake is the only build system supported.

* Use Mozilla style.
  Enforce with ``clang-format`` (configured in ``.clang-format``).
  Run ``tools/scripts/check-format.sh fix`` to auto-format all files.

* All C++ code follows Doxygen-compatible documentation standards for API clarity.

* max 100 characters per line for Documentation and for code.

* Use ``#pragma once`` in headers instead of manual include guards.

* **Error handling:** Always prefer returning error codes (negative errno values)
  over assertions. Use ``DAWNERR()`` to log errors before returning.

  * **Preferred:** Return error codes with logging (``-EINVAL``, ``-ENOMEM``, ``-EIO``, etc.)
  * ``DAWNASSERT`` should **only** be used for constructor argument validation
    at API boundaries (e.g., validating class ID ranges).
  * ``DEBUGASSERT`` is **not recommended** for general validation. While acceptable
    for NuttX-specific debug checks (compiled out in release builds), prefer
    explicit error returns for better production diagnostics.
  * Runtime errors (null pointers, invalid data, unsupported operations, resource
    allocation failures) **must** return error codes, not assert.

* Dynamic allocations are not allowed except in the initialization stage.
  Protocols based on external stacks must document any violation explicitly and
  must not allocate from the general system heap in runtime request paths.

* Unit testing as much as possible.

* code and configuration should be free of OS-specific code and options.
  OS-specific code goes to ``/dawn/src/porting`` and ``/dawn/include/porting``.
