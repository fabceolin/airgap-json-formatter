# Dockerfile for Airgap JSON Formatter local development
# Ubuntu 24.04 with Qt 6.8.0 WASM, Emscripten 3.1.56, and Rust

FROM ubuntu:24.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Version configuration (matching GitHub Actions workflow)
ENV QT_VERSION=6.8.0
ENV EMSDK_VERSION=3.1.56
ENV RUST_VERSION=stable

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    wget \
    python3 \
    python3-pip \
    python3-venv \
    pipx \
    pkg-config \
    libssl-dev \
    xz-utils \
    p7zip-full \
    libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

# Install Rust
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain ${RUST_VERSION}
ENV PATH="/root/.cargo/bin:${PATH}"

# Add WASM target and install wasm-bindgen-cli
RUN rustup target add wasm32-unknown-unknown && \
    cargo install wasm-bindgen-cli

# Install Emscripten SDK
WORKDIR /opt
RUN git clone https://github.com/emscripten-core/emsdk.git && \
    cd emsdk && \
    ./emsdk install ${EMSDK_VERSION} && \
    ./emsdk activate ${EMSDK_VERSION}

# Set up Emscripten environment
ENV EMSDK=/opt/emsdk
ENV PATH="${EMSDK}:${EMSDK}/upstream/emscripten:${PATH}"
ENV EM_CONFIG=/opt/emsdk/.emscripten

# Download and install Qt for WebAssembly using pipx
WORKDIR /opt
RUN pipx install aqtinstall && \
    /root/.local/bin/aqt install-qt linux desktop ${QT_VERSION} -m qtshadertools && \
    /root/.local/bin/aqt install-qt all_os wasm ${QT_VERSION} wasm_singlethread -m qtshadertools

# Set Qt environment variables and fix permissions
ENV QT_ROOT_DIR=/opt/${QT_VERSION}
ENV QT_HOST_PATH=/opt/${QT_VERSION}/gcc_64
ENV QT_WASM_PATH=/opt/${QT_VERSION}/wasm_singlethread
ENV PATH="${QT_HOST_PATH}/bin:${QT_WASM_PATH}/bin:${PATH}"

# Fix permissions for Qt binaries
RUN chmod +x /opt/${QT_VERSION}/wasm_singlethread/bin/* && \
    chmod +x /opt/${QT_VERSION}/gcc_64/bin/*

# Create working directory
WORKDIR /workspace

# Copy project files
COPY . .

# Build script
RUN echo '#!/bin/bash\n\
set -e\n\
\n\
# Source Emscripten environment\n\
source /opt/emsdk/emsdk_env.sh\n\
\n\
echo "=== Building Rust WASM ==="\n\
cargo build --target wasm32-unknown-unknown --release\n\
mkdir -p dist/pkg\n\
wasm-bindgen target/wasm32-unknown-unknown/release/airgap_json_formatter.wasm \\\n\
    --out-dir dist/pkg \\\n\
    --typescript \\\n\
    --target web\n\
\n\
echo "=== Building Qt WASM ==="\n\
cd qt\n\
mkdir -p build && cd build\n\
${QT_WASM_PATH}/bin/qt-cmake .. -DCMAKE_BUILD_TYPE=Release\n\
cmake --build . --parallel $(nproc)\n\
\n\
# Copy outputs to dist\n\
for ext in wasm js html; do\n\
    for file in *.$ext; do\n\
        if [ -f "$file" ]; then\n\
            cp "$file" ../../dist/\n\
            echo "Copied: $file"\n\
        fi\n\
    done\n\
done\n\
\n\
if [ -f "qtloader.js" ]; then\n\
    cp qtloader.js ../../dist/\n\
fi\n\
\n\
cd ../..\n\
\n\
echo "=== Copying static assets ==="\n\
cp public/index.html dist/\n\
cp public/bridge.js dist/\n\
cp public/history-storage.js dist/\n\
cp public/sw.js dist/\n\
cp public/manifest.json dist/\n\
cp public/404.html dist/\n\
\n\
echo "=== Build complete! ==="\n\
echo "Output in ./dist/"\n\
ls -la dist/\n\
' > /usr/local/bin/build.sh && chmod +x /usr/local/bin/build.sh

# Development helper scripts
RUN echo '#!/bin/bash\n\
# Run Rust tests\n\
cargo test --verbose\n\
' > /usr/local/bin/test.sh && chmod +x /usr/local/bin/test.sh

RUN echo '#!/bin/bash\n\
# Serve the dist directory\n\
cd /workspace/dist\n\
python3 -m http.server 8080\n\
' > /usr/local/bin/serve.sh && chmod +x /usr/local/bin/serve.sh

# Default command
CMD ["bash"]

# Labels
LABEL maintainer="Airgap JSON Formatter"
LABEL version="1.0"
LABEL description="Development environment for Airgap JSON Formatter with Qt WASM and Rust"
