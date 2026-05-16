.. _dawnpy-lwm2m:

============
dawnpy-lwm2m
============

LwM2M transport extension for :doc:`dawnpy`. It provides an
``aiocoap``-backed LwM2M server used by the Wakaama NTFC suite and a
``dawnpy-lwm2m`` console for descriptor-backed resource access.

Source: `github.com/railab/dawnpy-lwm2m
<https://github.com/railab/dawnpy-lwm2m>`_

Installation
============

Requires the core ``dawnpy`` package to be installed first::

    pip install -e tools/dawnpy-lwm2m

Dependencies (installed automatically): ``dawnpy``, ``aiocoap >= 0.4.17``,
and ``click >= 8.1``.

Command
========

::

    dawnpy-lwm2m descriptors/ntfc/ntfc_wakaama.yaml
    dawnpy-lwm2m descriptors/ntfc/ntfc_wakaama.yaml --endpoint ntfc-wakaama --timeout 30
    dawnpy-lwm2m --help

The command binds a UDP LwM2M server, waits for one registration on
``/rd``, acknowledges it, and enters an interactive console. With a
descriptor path, resources can be addressed by IO ID or absolute LwM2M path.
The console supports listing descriptor bindings plus resource read, write,
and monitor commands.

Tests
=====

::

    cd tools/dawnpy-lwm2m && tox
    tox -e py
