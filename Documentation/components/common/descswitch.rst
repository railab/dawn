=================
Descriptor Switch
=================

``CDescSwitch`` provides a process-wide signal used for descriptor slot switch
requests.

Overview
========

``CDescSwitch`` is a static coordination utility:

- ``requestSwitch(slot)`` records pending switch target slot.
- ``isSwitchRequested()`` indicates pending switch request.
- ``getSwitchSlot()`` returns requested slot index.
- ``setActiveSlot(slot)`` stores active runtime slot index.
- ``getActiveSlot()`` returns active runtime slot index.
- ``clear()`` clears pending switch request.

``CDawn::start()`` polls this signal in the main loop and performs runtime
descriptor reload when requested.

Doxygen
=======

- `dawn::CDescSwitch <../../doxygen/classdawn_1_1CDescSwitch.html>`_
