# Dawn Docker Container

This directory contains Docker helpers for users who want to build, run, or QA
Dawn without installing the full host toolchain directly.

Build the Ubuntu Dawn container image from the Dawn repository root:

```bash
docker build -f tools/docker/ubuntu-container.Dockerfile \
  -t dawn-container \
  tools/docker
```

The image build installs the host toolchain, Docker QA entrypoint, and helper
scripts only. Dawn source checkout, `repo_init.sh`, `.venv` creation, editable
Python package installation, and QA builds happen when the container starts.

Run the full QA suite from Docker:

```bash
tools/docker/run-qa.sh
```

By default, QA clones `DAWN_REPO` at `DAWN_REF` inside the container and runs
`repo_init.sh` there. To test the current checkout instead, use local source
mode:

```bash
tools/docker/run-qa.sh --local
```

Local source mode bind-mounts the current checkout read-only, copies it to an
in-container workspace, runs `repo_init.sh`, creates the virtual environment,
installs the required local Python tooling from that copy, and runs QA there.
This tests local changes without writing root-owned build artifacts back into
the host checkout.

The wrapper intentionally runs the container with bounded resources:

- `DAWN_DOCKER_CPUS`, default `4`
- `DAWN_DOCKER_MEMORY`, default `8g`
- `DAWN_DOCKER_QA_JOBS`, default `2`
- Docker local log rotation, default `20m` x `2`
- Docker PID limit, default `4096`

The QA container is privileged because the integration tests need kernel-backed
network devices. The wrapper creates `/dev/net/tun` inside the container before
starting `testenv_init.sh`; no manual host `/dev/net/tun` setup is required.

Pass `dawnpy-tests` arguments after the wrapper name:

```bash
tools/docker/run-qa.sh --jobs 1
```

Select upstream/image source explicitly:

```bash
tools/docker/run-qa.sh --image
```

Select a different local checkout:

```bash
DAWN_DOCKER_LOCAL_SOURCE=/path/to/dawn tools/docker/run-qa.sh --local
```

The same source selection can be made with `DAWN_DOCKER_SOURCE=image` or
`DAWN_DOCKER_SOURCE=local`.

Select an explicit Dawn ref for upstream/image mode:

```bash
DAWN_REF=master tools/docker/run-qa.sh --image
```

Use a fork or local mirror:

```bash
DAWN_REPO=https://github.com/railab/dawn.git \
DAWN_REF=master \
tools/docker/run-qa.sh --image
```

Use a different NuttX branch:

```bash
NUTTX_BRANCH=dawn tools/docker/run-qa.sh --image
```

The repository also contains a GitHub Actions workflow that publishes the image
to GitHub Container Registry as `ghcr.io/<owner>/<repo>/dawn-container`.
It builds on pull requests that touch Docker files, publishes automatically on
pushes to `main` or `master` that touch Docker files, and can also be run
manually from the Actions tab.
