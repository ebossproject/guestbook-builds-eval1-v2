# Copyright (c) 2026 Cromulence
# Uses environmental variables (ENV): 
#   - VARIANT - specifies the directory of the desired build
#   - VCPKG_OVERLAY_TRIPLETS - specifies directory to look for target triplets
#   - VCPKG_OVERLAY_PORTS - specifies location to search ports (takes precedence over vcpkg repo)
#   - TOOLCHAIN_FILE - specifies toolchain path and file
#   - TRIPLET - specifies the triplet to use for the cmake configuration
#   - VCPKG_ROOT - specifies the root of the vcpkg install location and other contents

# 1/2: Build
FROM ubuntu:24.04 AS build

ARG VARIANT
ENV VCPKG_OVERLAY_TRIPLETS=/opt/vcpkg-overlays/triplets
ENV VCPKG_OVERLAY_PORTS=/opt/vcpkg-overlays/ports
ENV TRIPLET=x64-linux
ENV VCPKG_ROOT=/opt/vcpkg
ENV TOOLCHAIN_FILE=/opt/toolchains/toolchain.cmake

RUN echo $TOOLCHAIN_FILE && echo $TRIPLET && echo $VCPKG_OVERLAY_PORTS && echo $VCPKG_OVERLAY_TRIPLETS

# Disable interactive prompts during package installs
ENV DEBIAN_FRONTEND=noninteractive

# Install build tools
RUN apt-get update && \
    apt-get install -y \
        clang \
        lld \
        cmake \
        ninja-build \
        git \
        make \
        wget \
        curl \
        zip \
        unzip \
        time \
        ca-certificates \
        pkg-config \
        autoconf libtool \
        && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# Pin vcpkg version for package stability
RUN git clone --depth 1 --branch 2025.03.19 https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} && \
    ${VCPKG_ROOT}/bootstrap-vcpkg.sh -disableMetrics

# Copy everything needed into the image
COPY variant-builds/${VARIANT}/app /challenge/app
COPY variant-builds/${VARIANT}/vcpkg-overlays/ports /opt/vcpkg-overlays/ports
COPY variant-builds/${VARIANT}/vcpkg-overlays/ports ${VCPKG_OVERLAY_PORTS}
COPY variant-builds/${VARIANT}/app/vcpkg.json variant-builds/${VARIANT}/app/CMakeLists.txt /src/guestbook/
COPY variant-builds/${VARIANT}/views /opt/tempviews/
COPY dependencies/ /challenge/dependencies
COPY toolchains/toolchain.cmake ${TOOLCHAIN_FILE}
COPY vcpkg-triplets/x64-linux.cmake ${VCPKG_OVERLAY_TRIPLETS}/x64-linux.cmake

WORKDIR /challenge

RUN echo $TOOLCHAIN_FILE && echo $TRIPLET && echo $VCPKG_OVERLAY_PORTS && echo $VCPKG_OVERLAY_TRIPLETS

# Configure and build using the values from base stage
RUN cmake -S /challenge/app -B /challenge/build \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=${TOOLCHAIN_FILE} \
    -DVCPKG_OVERLAY_PORTS=${VCPKG_OVERLAY_PORTS} \
    -DVCPKG_TARGET_TRIPLET=${TRIPLET} \
    -DCMAKE_BUILD_TYPE=Release && \
    cmake --build /challenge/build --target guestbook --verbose

# Stage 2: Runtime Stage
FROM ubuntu:24.04 AS runtime

ENV CHALLENGE_BINARY="/challenge/build/bin/guestbook"

RUN useradd -m user

COPY --from=build /challenge/build  /challenge/build
COPY --from=build /opt/vcpkg/buildtrees  /challenge/buildtrees
COPY --from=build /opt/tempviews/ /challenge/views/

RUN chown -R user:user /challenge

WORKDIR /challenge

USER user

# Launch the parser
CMD ["/challenge/build/bin/guestbook"]
