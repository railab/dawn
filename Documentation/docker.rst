===============
Docker Workflow
===============

The Dawn container provides an Ubuntu-based toolchain environment for building
and testing Dawn without installing the full host toolchain directly.

Build the image from the Dawn repository root::

  docker build -f tools/docker/ubuntu-container.Dockerfile \
    -t dawn-container \
    tools/docker

The image build installs the host toolchain, Docker QA entrypoint, and helper
scripts only. Dawn source checkout, ``repo_init.sh``, ``.venv`` creation,
editable Python package installation, and QA builds happen when the container
starts.

Full QA Suite
=============

Build the image first, then run the complete ``dawnpy-tests`` QA flow through
the protected wrapper::

  tools/docker/run-qa.sh

By default, this clones ``DAWN_REPO`` at ``DAWN_REF`` inside the container and
runs ``repo_init.sh`` there. To test the current host checkout instead, use
local source mode::

  tools/docker/run-qa.sh --local

Local source mode bind-mounts the host checkout read-only, copies it into an
in-container workspace, runs ``repo_init.sh``, creates the virtual environment,
installs the required local Python tooling from that copy, and runs QA there.
This keeps the host checkout isolated from root-owned build artifacts while
still testing local Dawn sources.

The wrapper refuses to start if Docker storage accounting is unhealthy, because
that state can leave stale container layers behind. It then runs the QA
container with bounded resources:

* ``DAWN_DOCKER_CPUS`` defaults to ``4``.
* ``DAWN_DOCKER_MEMORY`` defaults to ``8g`` and is also used for
  ``--memory-swap``.
* ``DAWN_DOCKER_QA_JOBS`` defaults to ``2``.
* Docker local log rotation defaults to ``20m`` x ``2``.
* the PID limit defaults to ``4096``.

The QA container uses ``--privileged`` because the integration tests create
kernel-backed network devices. The in-container entrypoint creates
``/dev/net/tun`` before running ``testenv_init.sh start``, so no manual host
``/dev/net/tun`` setup is required.

Pass additional ``dawnpy-tests`` arguments after the wrapper name, for example::

  tools/docker/run-qa.sh --jobs 1

Select upstream/image source explicitly::

  tools/docker/run-qa.sh --image

Select a different local checkout::

  DAWN_DOCKER_LOCAL_SOURCE=/path/to/dawn tools/docker/run-qa.sh --local

The same selection is available through ``DAWN_DOCKER_SOURCE=image`` or
``DAWN_DOCKER_SOURCE=local``.

Runtime Source Selection
========================

``DAWN_REPO``
  Git repository URL to clone in upstream/image mode. Defaults to
  ``https://github.com/railab/dawn.git``.

``DAWN_REF``
  Branch, tag, or commit to check out after cloning. Defaults to ``master``.

``NUTTX_BRANCH``
  Branch to use for both NuttX repositories during ``repo_init.sh``. Defaults
  to ``dawn``.

Example using an explicit ref::

  DAWN_REF=master tools/docker/run-qa.sh --image

Example using an explicit NuttX branch::

  NUTTX_BRANCH=dawn tools/docker/run-qa.sh --image

Publishing
==========

The ``Docker image`` GitHub Actions workflow publishes the Dawn container to
GitHub Container Registry as::

  ghcr.io/<owner>/<repo>/dawn-container

The workflow:

* builds on pull requests that touch Docker files, without publishing;
* publishes automatically on pushes to ``main`` or ``master`` that touch
  ``tools/docker/**`` or the workflow itself;
* can be started manually from the Actions tab.

Notes
=====

The Docker QA path is intended for simulator and virtual integration coverage.
Physical hardware access still depends on host device availability and explicit
Docker device permissions.
