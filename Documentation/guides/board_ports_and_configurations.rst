==============================
Board Ports and Configurations
==============================

This page will describe how to create board ports and Dawn configurations on
top of Apache NuttX.

For now, use the Apache NuttX documentation as the primary reference:

* `NuttX boards support <https://nuttx.apache.org/docs/latest/components/boards.html>`__
* `NuttX custom boards how-to <https://nuttx.apache.org/docs/latest/guides/customboards.html>`__
* `NuttX configuration guide <https://nuttx.apache.org/docs/latest/quickstart/configuring.html>`__

Board ports in Dawn use basically the same layout as NuttX board ports. Dawn
keeps the familiar ``boards/<arch>/<chip>/<board>/`` structure and layers
Dawn-specific configuration on top of the regular NuttX board configuration.

Future Dawn-specific content should cover:

* ``defconfig`` and ``defconfig.dawn`` pairs,
* descriptor selection from NuttX Kconfig,
* in-tree versus out-of-tree board layouts, and
* board bring-up checks for Dawn applications.
