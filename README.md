# 🌸 Lotus Coin (`LOTUS`)

**A High-Performance, Hybrid PoW/PoS Modular C++ Cryptocurrency Engine**

---

## 📸 System Architecture

```text
       +-----------------------------------------------------------------------+
       |                           LOTUS COIN NODE                             |
       |                                                                       |
       |  +------------------+   +-------------------+   +------------------+  |
       |  |  HTTP REST API   |   |   P2P SERVER      |   |  LEVELDB ENGINE  |  |
       |  |   (Port 8080)    |   |   (Port 9333)     |   |   (Disk Ledger)  |  |
       |  +--------+---------+   +---------+---------+   +--------+---------+  |
       |           |                       |                      |            |
       |           v                       v                      v            |
       |  +-------------------------------------------------------------+      |
       |  |                  LOTUS NETWORK CORE STATE                   |      |
       |  |  [Mempool Priority Queue]  --> [PoS Validator Check]        |      |
       |  |  [PoW Miner Engine]        --> [Dynamic Target Adjuster]    |      |
       |  |  [Slashing & Treasury Pool] --> [Longest Chain Fork Sync]   |      |
       |  +-------------------------------------------------------------+      |
       +-----------------------------------------------------------------------+