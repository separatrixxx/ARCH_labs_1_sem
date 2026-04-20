FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential ccache cmake git ninja-build pkg-config \
    libpq-dev \
    postgresql-server-dev-14 \
    libldap2-dev \
    libkrb5-dev \
    libmongoc-dev libbson-dev \
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

COPY lab2/CMakeLists.txt ./
COPY lab2/cmake/ cmake/
COPY lab2/DownloadUserver.cmake ./
COPY lab2/src/ src/
COPY lab2/openapi.yaml ./
COPY lab4/configs/ configs/

RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DFETCHCONTENT_UPDATES_DISCONNECTED=ON \
    -DUSERVER_FEATURE_TESTSUITE=OFF \
    -DUSERVER_FEATURE_POSTGRESQL=ON \
    -DUSERVER_FEATURE_MONGODB=ON \
    -DUSE_MIXED=ON \
    -DPostgreSQLInternal_ROOT=/usr/lib/postgresql/14 \
    && cmake --build build --target user_service catalog_service order_service -- -j1

EXPOSE 8081 8082 8083
