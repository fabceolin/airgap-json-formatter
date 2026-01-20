# Airgap JSON Formatter

**Airgap JSON Formatter** is a security-first tool for manipulating JSON data. It combines the performance and safety of **Rust** with a robust **Qt** interface, delivered entirely via the browser using **WebAssembly**.

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

* **Offline Capability:** Fully functional without an internet connection after initial load.
* **High Performance:** Rust-based parsing engine handles large JSON files efficiently.
* **Strict Validation:** Instant syntax checking and error reporting.
* **Privacy-Guaranteed:** Designed specifically for developers handling sensitive production data.

## 🛠️ Build & Development

### Prerequisites

* Rust toolchain (`stable`)
* Qt 6 Development Libraries
* `wasm32-unknown-unknown` target

### Local Build (Native)

To run the Qt application as a native desktop binary:

```bash
cargo run --release
