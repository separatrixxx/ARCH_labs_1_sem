FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential ccache cmake git ninja-build pkg-config \
    libabsl-dev \
    libboost-context1.74-dev \
    libboost-coroutine1.74-dev \
    libboost-filesystem1.74-dev \
    libboost-iostreams1.74-dev \
    libboost-locale1.74-dev \
    libboost-program-options1.74-dev \
    libboost-stacktrace1.74-dev \
    libboost1.74-dev \
    libbz2-dev \
    libc-ares-dev \
    libcctz-dev \
    libcrypto++-dev \
    libcurl4-openssl-dev \
    libdouble-conversion-dev \
    libev-dev \
    libfmt-dev \
    libgflags-dev \
    libgmock-dev \
    libgtest-dev \
    libicu-dev \
    libjemalloc-dev \
    libre2-dev \
    liblz4-dev \
    liblzma-dev \
    libnghttp2-dev \
    libpugixml-dev \
    libsnappy-dev \
    libssl-dev \
    libxxhash-dev \
    libyaml-cpp-dev \
    libzstd-dev \
    netbase \
    python3-dev python3-jinja2 python3-venv python3-yaml \
    ragel yasm zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt ./
COPY cmake/ cmake/
COPY DownloadUserver.cmake ./
COPY src/ src/
COPY configs/ configs/
COPY openapi.yaml ./

RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DFETCHCONTENT_UPDATES_DISCONNECTED=ON \
    -DUSERVER_FEATURE_TESTSUITE=OFF \
    && cmake --build build --target user_service catalog_service order_service -- -j1

EXPOSE 8081 8082 8083
