# 🌸 Lotus Coin (LOTUS)

[![Build Status](https://github.com/Pawan0810-ops/Lotus_Coin/actions/workflows/build.yml/badge.svg)](https://github.com/Pawan0810-ops/Lotus_Coin/actions)
[![Latest Release](https://img.shields.io/github/v/release/Pawan0810-ops/Lotus_Coin?color=pink)](https://github.com/Pawan0810-ops/Lotus_Coin/releases)
[![Live Web Explorer](https://img.shields.io/badge/Web_Explorer-Live-brightgreen)](https://pawan0810-ops.github.io/Lotus_Coin/)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

**Lotus Coin** is a high-performance, modular, hybrid Proof-of-Work (PoW) / Proof-of-Stake (PoS) cryptocurrency engine built from scratch in C++. It features persistent state storage via LevelDB, cryptographic transaction signing with OpenSSL, a peer-to-peer (P2P) auto-syncing protocol, an embedded HTTP REST API engine, and a web-based blockchain explorer dashboard.

🌐 **Live Blockchain Explorer:** [https://pawan0810-ops.github.io/Lotus_Coin/](https://pawan0810-ops.github.io/Lotus_Coin/)

---

## ⚡ Core Features

- **Hybrid PoW/PoS Consensus:** Proof-of-Stake gate mechanism enforcing active validator stake before high-priority PoW block mining.
- **LevelDB State Engine:** High-throughput local key-value store for immutable chain state and block ledger persistence.
- **OpenSSL SHA-256 Cryptography:** Secure block header hashing, target difficulty adjustment, and cryptographic validation.
- **Automated P2P Syncing:** Asynchronous TCP socket protocol for multi-node block propagation and real-time state synchronization.
- **Embedded REST API:** Native C++ HTTP API endpoints serving real-time node metrics, mempool state, and ledger history.
- **Docker Multi-Node Infrastructure:** Containerized environment supporting local multi-node network simulation out of the box.
- **CI/CD & Pre-Compiled Releases:** GitHub Actions automated compilation tests and cross-platform binary release distribution (`v1.0.0`).

---

## 🏗️ System Architecture

```text
[ Web Explorer / Client ] 
           │
           │ (HTTP REST APIs)
           ▼
┌─────────────────────────────────────────────────────────┐
│                      Lotus Node                         │
│  ┌───────────────────┐        ┌──────────────────────┐  │
│  │ HTTP API Server   │        │ P2P Sync Engine      │  │
│  │ (Port 8080/8081)  │        │ (Port 9333/9334)     │  │
│  └─────────┬─────────┘        └──────────┬───────────┘  │
│            │                             │              │
│            ▼                             ▼              │
│  ┌───────────────────────────────────────────────────┐  │
│  │ Hybrid PoW/PoS Consensus Engine                   │  │
│  └─────────────────────────┬─────────────────────────┘  │
│                            │                            │
│                            ▼                            │
│  ┌───────────────────────────────────────────────────┐  │
│  │ LevelDB Blockchain State Storage                  │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘