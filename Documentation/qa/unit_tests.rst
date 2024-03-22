.. _unit_tests:

==========
Unit Tests
==========

Dawn unit tests live in ``dawn/tests/`` and are built into a standalone
``dawntest`` application that runs inside the NuttX simulator. Tests use
a Unity-based harness with lightweight local mocks. Every source file
under ``dawn/src/`` has a corresponding test file.

They run automatically as part of ``dawnpy-tests`` (the
*Simulator unit tests* step, ``nsh_tests`` config).

Organization
============

.. code:: text

   dawn/tests/
   ├── common/     - object system, descriptor, bindable, handler
   ├── io/         - all IO type implementations
   ├── prog/       - all PROG type implementations
   ├── proto/      - all PROTO type implementations
   ├── dev/        - descriptor and inspector subsystems
   ├── porting/    - NuttX platform abstraction layer
   └── mocks/      - shared fake and spy helpers

Running Selected Tests
======================

.. code:: shell

   dawntest --list                              # list modules
   dawntest --module io                         # run one module
   dawntest --test io.test_io_handler_user_factory  # run one case
   dawntest --prefix proto.                     # run by prefix

Mocking Conventions
===================

Mocks in ``dawn/tests/mocks/`` follow two conventions:

* **Fake** - deterministic behaviour, no call tracking.
* **Spy** - fake behaviour plus call recording via ``MockTrace``
  (``dawn/tests/include/test_mock_expect.hxx``).

Common spy assertions:

.. code:: cpp

   ASSERT_CALLS(trace, "create", N);
   ASSERT_CALL_ORDER(trace, expected_order);
   ASSERT_CALL_AT(trace, idx, "create", id);

Test Style
==========

A test case exercises **one behaviour**. If you find yourself writing
"first do X, then do Y, then do Z", write three test cases instead of
one. Keeping cases small means a failure points at the exact behaviour
that broke instead of a 200-line lifecycle, and re-running a single
scenario is one ``dawntest --test ...`` invocation.

Guidelines:

* **One behaviour per case.** A read-back test is separate from a
  write-back test. A bool case is separate from a uint32 case. An
  out-of-range error path is its own case, not an extra assert glued
  onto the happy path.
* **Soft size target: ~50 lines per case.** Boilerplate setup is
  excluded - factor repeated configure/init/bind sequences into
  file-local static helpers. A case over 100 lines must carry a comment
  explaining why splitting is impractical.
* **Naming:** ``test_<module>_<feature>_<scenario>`` - e.g.
  ``test_io_limits_validate_uint32_boundaries``,
  ``test_proto_can_push_bool``. The scenario is the differentiator;
  prefer concrete names (``read_out_of_range``) over generic ones
  (``error``).
* **One Description block per case.** The existing
  ``// Description: ...`` banner above the function is a one-line
  summary of what the case asserts, in plain English.
* **Helpers stay file-local.** Promote a helper to a shared header in
  ``dawn/tests/include/`` only when three or more files reuse it.
