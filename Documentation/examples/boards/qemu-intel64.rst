============
QEMU Intel64
============

``/boards/x86_64/qemu/qemu-intel64``

Configs
=======

nsh_modbus_tcp
--------------

Modbus TCP reference configuration using ``qemu_modbus_tcp_dummy_map.yaml``.

.. code:: bash

   qemu-system-x86_64 -m 2G -smp 4 -cpu host -enable-kvm -nographic -serial mon:stdio -device e1000,netdev=mynet0 -netdev tap,id=mynet0,ifname=tap0,script=no,downscript=no -kernel <path_to_nuttx_elf>

nsh_nxscope_udp
---------------

NXScope UDP reference configuration using ``qemu_nxscope_udp.yaml``.

.. code:: bash

   qemu-system-x86_64 -m 2G -smp 4 -cpu host -enable-kvm -nographic -serial mon:stdio -device e1000,netdev=mynet0 -netdev tap,id=mynet0,ifname=tap0,script=no,downscript=no -kernel <path_to_nuttx_elf>

nsh_udp
-------

Basic UDP reference configuration using ``udp_basic.yaml``.

.. code:: bash

   qemu-system-x86_64 -m 2G -smp 4 -cpu host -enable-kvm -nographic -serial mon:stdio -device e1000,netdev=mynet0 -netdev tap,id=mynet0,ifname=tap0,script=no,downscript=no -kernel <path_to_nuttx_elf>
