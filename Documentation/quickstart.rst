===========
Quick Start
===========

.. note::

   Dawn is developed and tested on **Linux only**. Other operating
   systems have not been tested and probably need extra work.
   See :doc:`environment` for the full list of host requirements and
   for what each test suite needs.

   If you prefer an Ubuntu container that automates this setup, see
   :doc:`docker`.

Requirements
============

Install the basic host tools first:

* ``git`` for cloning the repository and initialising submodules.
* Python **3.12 or newer** with ``venv`` and ``pip`` for the ``dawnpy``
  tooling.
* ``cmake`` and ``ninja`` for the NuttX build.
* ``kconfiglib`` for NuttX Kconfig processing. Installing ``dawnpy`` into
  the virtual environment below installs this automatically.
* **GCC 14 or older** for the simulator demo. GCC 15 currently breaks the
  NuttX libcxx build path used by Dawn.

Ubuntu / Debian::

  sudo apt install git python-is-python3 python3 python3-venv python3-pip \
                   cmake ninja-build build-essential gcc-14 g++-14

Arch Linux::

  sudo pacman -S --needed git python python-pip cmake ninja base-devel

Check that your Python is new enough before creating the virtual
environment::

  python --version

The version must be **3.12 or newer**. If ``python`` is unavailable or points
to an older interpreter, use ``python3`` or an explicit version such as
``python3.12`` for the commands below.

On Arch Linux, install the GCC 14 toolchain from the ``gcc14`` AUR package. With
your favorite AUR helper::

  pikaur -S gcc14

If your default host compiler is newer than gcc 14, make sure ``gcc-14`` and
``g++-14`` are available before running the simulator builds below. The
quickstart build commands select them explicitly with
``-e CXX=g++-14 -e CC=gcc-14``.

Clone the repository::

  git clone https://github.com/railab/dawn.git
  cd dawn

Initialise the workspace. By default, this initialises the submodules under
``tools/``, clones the NuttX sources into ``external/``, and creates the
``external/apps/external`` symlink used by NuttX::

  ./repo_init.sh

Run ``./repo_init.sh --help`` for help.

Set up a Python virtual environment (recommended) and install the tooling in
editable mode::

  python -m venv .venv
  source .venv/bin/activate
  pip install -e tools/dawnpy

On Ubuntu / Debian releases, use ``python3`` instead of ``python`` if needed::

  python3 -m venv .venv

Optional transport / QA extensions::

  pip install -e tools/dawnpy-serial tools/dawnpy-can \
                 tools/dawnpy-udp tools/dawnpy-modbus \
                 tools/dawnpy-tests
  pip install -r ntfc/requirements.txt

From this point, ``dawnpy`` is installed in the virtual environment. The
examples use ``python -m dawnpy`` so they also work when the console script is
not on ``PATH``.

Host GCC Version
================

The current NuttX libcxx clean build supports at most GCC 14. GCC 15 currently
is not supported.

Add these options to build commands when your default host compiler is newer
than gcc 14::

  -e CXX=g++-14 -e CC=gcc-14

Dawn Shell on simulator
=======================

This quickstart uses the NuttX simulator with the Dawn shell. It provides a
host-only way to build and inspect a descriptor-defined Dawn node.

Configure and build::

  python -m dawnpy build build_shell boards/sim/sim/sim/configs/nsh_shell \
    -e CXX=g++-14 \
    -e CC=gcc-14

Run Dawn Shell on simulator::

  ./build_shell/nuttx

Start Dawn from the NuttX shell::

  NuttShell (NSH) NuttX-12.12.0
  nsh> dawn

It prints some debug logs and should end up with::

  *Start DAWN Shell*

  abcde> 

Start with the ``help`` command::

    abcde> help
    Proto SHELL help:
       help - print this message
       exit - exit
       info - show shell-bound IO inventory
       getio <IO objectID> - get value for a given objectID
       setio <IO objectID> <v1..vN> - set IO value words
       getioloop - get all objects in loop
       getionotify <objectID> - get io with notification
       getcfg <objectID> [cfgID] - get object configuration
       setcfg <objectID> <cfgID> <v1..vN> - set object config words

     Object Inspector:
       list [io|prog|proto] [verbose] - show object inventory
       inspect <objectID> - detailed object introspection
       tree - show object hierarchy
       stats - show runtime statistics

       NOTE: all values must be in hex format: 0xYYYYYYYY

The demo descriptor is
``descriptors/examples/shell_core_demo.yaml``. It creates many objects; these
are the most useful ones for a first pass through the shell:

.. list-table::
   :header-rows: 1
   :widths: 18 18 64

   * - Object
     - ID
     - What it demonstrates
   * - ``dummy_0``
     - ``0x40a70000``
     - Read/write ``uint32`` IO with an initial value of ``1234``.
   * - ``virt_3``
     - ``0x4c860003``
     - Virtual IO written by the ``sampling_0`` program.
   * - ``config_0``
     - ``0x40270000``
     - Config IO associated with ``dummy_0``.
   * - ``capabilities_0``
     - ``0x46af0000``
     - Seekable capability blob; useful for checking block IO output.
   * - ``sampling_0``
     - ``0xc0c00000``
     - Program that periodically copies dummy IO into virtual IO.
   * - ``shell_0``
     - ``0x81e00000``
     - The shell protocol object.

Use ``info`` for the shortest view of objects that this shell can access with
``getio`` and ``setio``::

  abcde> info
  INFO: shell IO bindings
    Name           ObjID      DType[Dim] Flags
    -------------- ---------- ---------- ------
    dummy_0        0x40a70000 uint32[1] RW--C
    ...
    capabilities_0 0x46af0000 block[512] R----

Use ``list io`` when you want the runtime inspector view with read/write/error
counters. Plain ``list`` shows all sections, and ``list prog`` / ``list proto``
show only programs or protocols::

  abcde> list io
  Object Inventory
  ================

  IOs: 51 objects
    Name           ObjID      DType[Dim]  Cls Flags  R/W/E
    -------------- ---------- ----------- --- ------ -------
    dummy_0        0x40a70000 uint32[1]   5 RW--C  0/0/0
    ...
    virt_3         0x4c860003 int32[1] 100 RWN--  0/8/0
    config_0       0x40270000 uint32[1]   1 RW--C  0/0/0
    capabilities_0 0x46af0000 block[512]  53 R----  0/0/0
    ...

  PROGs: 4 objects
    min_0          0xc0200000   1 RUNNING
    max_0          0xc0400000   2 RUNNING
    sum_0          0xc0800000   4 RUNNING
    sampling_0     0xc0c00000   6 RUNNING

  PROTOs: 1 objects
    shell_0        0x81e00000  15 RUNNING

Read the first dummy IO. The shell requires object IDs in hex, but printed
scalar data is decimal::

  abcde> getio 0x40a70000
  IO 0x40a70000 data:
          1234

Write a new value and read it back. Input data words must also be hex::

  abcde> setio 0x40a70000 0x0000002a
  abcde> getio 0x40a70000
  IO 0x40a70000 data:
          42

Inspect the same object when you need type, class, flags, dimensions, config
state, and runtime counters::

  abcde> inspect 0x40a70000

  Object Details
  ==============

    Name:         dummy_0
    Object ID:    0x40a70000
    Handler:      IO
    Data Type:    0x07 (uint32, 4 bytes)
    Dimension:    1 (item size 4 bytes)
    Config:       Yes

  Statistics
  ----------
    Reads:        2
    Writes:       1
    Errors:       0

Read a virtual IO produced by a running program. In this descriptor,
``sampling_0`` periodically copies ``dummy_3`` into ``virt_3``, so the value is
``-1``::

  abcde> getio 0x4c860003
  IO 0x4c860003 data:
          -1

Use ``stats`` at the end to check that reads, writes, and program-produced
virtual writes are changing::

  abcde> stats

  I/O Statistics
  ==============

  Summary
  -------
    Total I/O Objects: 51
    Total Reads:       ...
    Total Writes:      ...
    Total Errors:      0

Type ``exit`` to stop Dawn and return to the NuttX shell::

  abcde> exit
  nsh>

Type ``poweroff`` to exit NuttX simulator::

  nsh> poweroff

Blinky Demos
============

Dawn ships several blinky-based runtime-control examples. The examples demonstrate
descriptor-defined IO, Program, and Protocol control paths such as shell control,
BLE control, and program chaining. The LED is the visible output.

See :doc:`examples/descriptors` for all blinky descriptors and
:doc:`examples/boards/index` for the full list of board targets.

More Examples
=============

For more example and reference configurations, see :ref:`examples`.

For an overview of running Dawn on the NuttX simulator and QEMU
targets, see :doc:`host_development`.

Next Steps
==========

After the simulator shell demo:

* Use :doc:`examples/descriptors` to try blinky, protocol, gateway, feature,
  and board-oriented descriptors.
* Use :doc:`examples/boards/index` to find simulator, QEMU, and hardware board
  configurations.
* Use :doc:`qa/index` for unit tests and integration testing.
* Use :doc:`customization` for out-of-tree projects and custom Dawn objects.
* Use :doc:`host_development` for simulator and QEMU workflows.
