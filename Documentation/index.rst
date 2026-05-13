.. Dawn Documentation master file, created by
   sphinx-quickstart on Thu Jun  6 14:57:26 2024.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. note::
   This project is in an early development stage.
   Any help or suggestions are welcome.

.. image:: assets/logo.svg
   :alt: Dawn Project logo
   :align: center
   :width: 280px

====================
Dawn's Documentation
====================

**Dawn** is a lightweight, modular, open-source data acquisition framework that
simplifies the creation of embedded node devices, whether they're smart,
edge-processing units or simple data relays. It enables developers to create
sensor nodes, actuator nodes, and complete DAQ systems.

It's a versatile solution for digital acquisition in embedded systems,
enabling rapid deployment of reliable, scalable hardware nodes, all built on
top of the `Apache NuttX <https://nuttx.apache.org/>`_ RTOS.

The core concept behind **Dawn** is that most embedded devices implement some
combination of the following functionalities:

* Reading data from inputs,

* Writing data to outputs,

* Performing edge processing, and

* Communicating with the external world using a protocol.

This project aims to simplify the implementation of these features, with the
potential to automatically generate ready-to-use projects.
Some effort (but not much) has been made to provide a porting abstraction
layer, which may, in the future, allow other POSIX-compatible systems to be
supported as well.

Many crucial components in Dawn are based on 32-bit variables, so it is
recommended to use it on at least 32-bit architectures, and those are its main
target.

For those curious about the origin of the project's name, **Dawn** is
an acronym for:

Data Acquisition With NuttX :)

Motivation
==========

Dawn was created to fill a practical gap: an easier way to build embedded
devices that share similar features, without rewriting the same patterns for
each project.

Many embedded projects need the same core features again and again.
Dawn helps build those devices faster, especially for prototyping and custom
dev/lab tools.

Design Tradeoffs
================

Dawn tries to minimize FLASH and RAM footprint.
At the same time, as a build-time and runtime abstraction layer, Dawn adds
some resource cost compared to a single-purpose hand-written firmware.

The practical goal is to maximize feature coverage while minimizing resource
usage. To support this, Dawn exposes many Kconfig options for fine tuning.
Configuration complexity is the tradeoff, but it can be managed with helper
tools provided by project.

Last Updated: |today|

`Dawn repository <https://github.com/railab/dawn>`_

.. toctree::
   :maxdepth: 1
   :caption: Contents:

   Home <self>
   quickstart.rst
   guides/index.rst
   Implementation <components/index.rst>
   examples/index.rst
   tools/index.rst
   api/index.rst
   QA <qa/index.rst>
   customization.rst
   roadmap.rst
   glossary.rst
