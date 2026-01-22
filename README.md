# Airgap JSON Formatter

**Airgap JSON Formatter** is a security-first, installable PWA for manipulating JSON data. It combines the performance and safety of **Rust** with a robust **Qt** interface, delivered entirely via the browser using **WebAssembly**. Install it on your device for instant offline access!

## Live Demo

**[Launch Airgap JSON Formatter](https://fabceolin.github.io/airgap-json-formatter/)**

### Quick Start
1. Open the app in your browser
2. **Optional:** Click the install icon in your address bar to add to your device
3. Paste or type JSON in the left pane
4. Click "Format" to prettify or "Minify" to compress
5. Copy the result to clipboard

The app works completely offline once loaded or installed - feel free to disconnect from the internet!

## 🔒 Privacy & Security Architecture

**Critical Notice:**
Despite being hosted on the web, this application executes **100% client-side**.

* **Zero-Network Policy:** Once the Wasm binary loads, the application makes no external API calls. You can disconnect your internet and it will function perfectly.
* **Local Processing:** All parsing, formatting, and validation logic runs within your browser's sandbox using the machine's CPU and memory.
* **Data Safety:** Sensitive JSON payloads (API keys, PII, credentials) never leave your device.

## 🚀 Tech Stack

* **Core Logic:** Rust (Safety, Performance)
* **GUI:** Qt (Cross-platform desktop-grade interface)
* **Platform:** WebAssembly (Wasm)
* **Delivery:** Static hosting via GitHub Pages

## ✨ Features

* **Installable PWA:** Add to your home screen or desktop for native app-like experience. Works offline after installation.
* **Offline Capability:** Fully functional without an internet connection after initial load.
* **High Performance:** Rust-based parsing engine handles large JSON files efficiently.
* **Strict Validation:** Instant syntax checking and error reporting.
* **Privacy-Guaranteed:** Designed specifically for developers handling sensitive production data.

## Build & Development

### Prerequisites

* Rust toolchain (stable, 1.70+)
* `wasm32-unknown-unknown` target

### Development Setup

```bash
# Install Rust (if not already installed)
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Add WebAssembly target
rustup target add wasm32-unknown-unknown
```

### Build Commands

```bash
# Run tests
cargo test

# Build WebAssembly target
cargo build --target wasm32-unknown-unknown --release
```

### Local Build (Native - for testing)

```bash
cargo run --release
