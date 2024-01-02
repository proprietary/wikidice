FROM debian:12 as base

RUN apt update && \
    apt install -y --no-install-recommends \
    libc6-dev \
    pkg-config \
    make \
    cmake \
    ca-certificates \
    locales \
    curl \
    git \
    python3 \
    gzip \
    python3-dev \
    libboost-all-dev \
    libzstd-dev \
    cppcheck

# Install latest LLVM toolchain

RUN apt update && \
    apt install -y --no-install-recommends \
    wget \
    gnupg2 \
    lsb-release && \
    wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc && \
    echo "deb http://apt.llvm.org/bookworm/ llvm-toolchain-bookworm main" >> /etc/apt/sources.list && \
    echo "deb-src http://apt.llvm.org/bookworm/ llvm-toolchain-bookworm main" >> /etc/apt/sources.list && \
    apt update && \
    apt install -y --no-install-recommends \
    clang-18 clang-tools-18 clang-format-18 python3-clang-18 clangd-18 clang-tidy-18 \
    libc++-18-dev libc++abi-18-dev \
    libclang-rt-18-dev \
    lld-18 \
    lldb-18 \
    libomp-18-dev \
    libfuzzer-18-dev

# Set toolchain as default in Debian

RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang-18 100 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-18 100 && \
    update-alternatives --install /usr/bin/ld ld /usr/bin/lld-18 100

FROM base as builder

RUN mkdir -p /wikidice

WORKDIR /wikidice

COPY . .

RUN mkdir build && \
    cd build && \
    cmake -S .. \
        -G"Unix Makefiles" \
        -DCMAKE_BUILD_TYPE=Release && \
    make -j12 && \
    ctest

# FROM base as runner

# WORKDIR /usr/bin

# COPY --from=builder /usr/src/app/build/ .

# VOLUME [ "/var/db" ]

# CMD [ "python3" "-m" "wikidice" ]