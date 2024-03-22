==========
Dawn Class
==========

``CDawn`` - a class that integrate all Dawn components and can be easily used
to create applications. It uses Dawn features based on dynamic allocation
(``CIOFactory``, ``CProgFactory`` and ``CProtoFactory``), which may not be
suitable for a certain class of applications.

Class/API Reference
===================

- `dawn::CDawn <../../doxygen/classdawn_1_1CDawn.html>`_

Objects life cycle
------------------

.. uml::
   :scale: 80%
   :align: center

   start

   : create all IOs;
   : initialize all IOs;
   : create all Programs;
   : initialize all Programs;
   : create all Protocols;
   : initialize all Protocols;
   : bind configuration items;
   : start all IOs;
   : start all Programs;
   : start all Protocols;

   stop

.. end of uml
