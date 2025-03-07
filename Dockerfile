FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    git \
    g++ \
    cmake \
    python3 \
    python3-pip \
    zenity \
    libcgal-dev \
    libgl1-mesa-dev \
    libx11-dev \
    wget \
    libssl-dev \
    ccache \
    && rm -rf /var/lib/apt/lists/*

# Install CMake
RUN wget https://github.com/Kitware/CMake/releases/download/v3.27.5/cmake-3.27.5-linux-x86_64.sh -O /tmp/cmake.sh && \
    chmod +x /tmp/cmake.sh && \
    /tmp/cmake.sh --skip-license --prefix=/usr/local && \
    rm /tmp/cmake.sh

# Set workdir
WORKDIR /app/polyfem

# Copy source code into the container
COPY . .

# Update submodules
RUN git submodule update --init --recursive

# Build PolyFEM
WORKDIR /app/polyfem/build

# Configure PolyFEM with ccache enabled
RUN --mount=type=cache,target=/root/.ccache \
    cmake .. \
    -DPOLYFEM_WITH_TESTS=OFF \
    -DPOLYFEM_WITH_CCACHE=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_DEBUGNOSYMBOLS=""

# Build PolyFEM using ccache
RUN --mount=type=cache,target=/root/.ccache \
    make -j $(nproc)

WORKDIR /data
ENTRYPOINT ["/app/polyfem/build/PolyFEM_bin"]
