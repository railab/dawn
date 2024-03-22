# syntax=docker/dockerfile:1
#
# Ubuntu-based Dawn container image.
#
# The image contains the host toolchain and Docker QA entrypoints. Dawn source
# checkout, repo_init.sh, virtualenv creation, and Python editable installs are
# performed when the container starts.

FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ENV DAWN_HOME=/workspace/dawn
ENV DAWN_REPO=https://github.com/railab/dawn.git
ENV DAWN_REF=master
ENV NUTTX_BRANCH=dawn

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        bison \
        build-essential \
        binutils-arm-none-eabi \
        ca-certificates \
        can-utils \
        cmake \
        cppcheck \
        flex \
        g++-14 \
        gcc-arm-none-eabi \
        gcc-14 \
        genromfs \
        git \
        gnupg \
        gperf \
        iproute2 \
        iputils-ping \
        libnewlib-arm-none-eabi \
        libstdc++-arm-none-eabi-dev \
        libstdc++-arm-none-eabi-newlib \
        lsb-release \
        make \
        net-tools \
        ninja-build \
        pkg-config \
        python-is-python3 \
        python3 \
        python3-pip \
        python3-venv \
        qemu-system-x86 \
        rsync \
        software-properties-common \
        socat \
        sudo \
        wget \
    && wget -O /tmp/llvm.sh https://apt.llvm.org/llvm.sh \
    && chmod +x /tmp/llvm.sh \
    && /tmp/llvm.sh 21 \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update \
    && apt-get install -y --no-install-recommends clang-format-21 \
    && ln -sf /usr/bin/clang-format-21 /usr/local/bin/clang-format \
    && rm -rf /var/lib/apt/lists/*

COPY install-dawn-python-tools.py /usr/local/bin/install-dawn-python-tools
COPY run-dawnpy-tests.sh /usr/local/bin/dawn-docker-qa
RUN chmod +x /usr/local/bin/install-dawn-python-tools /usr/local/bin/dawn-docker-qa

WORKDIR /workspace

CMD ["/bin/bash"]
