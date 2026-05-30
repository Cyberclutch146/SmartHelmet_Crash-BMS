# Latency Budget Planning

This document details the latency budget for the Smart Helmet System's ANC (Active Noise Cancellation) module.

## Why Latency Matters
In an ANC system, the anti-noise wave must be generated and played back precisely out of phase with the incoming noise. If the total processing latency is too high, the anti-noise will arrive late, causing positive interference (amplification) rather than cancellation.

## Latency Breakdown

| Component | Target Latency | Description |
| :--- | :--- | :--- |
| **UART RX** | `< 1.0 ms` | Command and control overhead. Not in the critical audio path, but interrupts could cause jitter. |
| **DMA RX (Microphone)** | `~2.5 ms` | Latency introduced by I2S RX buffering. E.g., a block size of 64 samples at 48kHz = 1.33 ms + DMA transfer overhead. |
| **FIR Processing (DSP)** | `< 1.0 ms` | Core ANC processing time. Highly dependent on FIR filter length and CMSIS-DSP optimizations. Target is to keep this well under 1ms per block. |
| **I2S TX (Speaker)** | `~2.5 ms` | Latency introduced by I2S TX buffering. Must match RX block size to maintain a consistent pipeline. |
| **Total Audio Latency** | `~6.0 ms` | Target total path latency from mic-in to speaker-out. Low-frequency rumble (typical in helmets, < 250Hz) can tolerate < 10ms. |

## Notes for Implementation
- **Block Size:** Keeping the audio block size small (e.g., 32 or 64 samples) reduces buffering latency but increases interrupt frequency, increasing CPU overhead.
- **Compiler Optimization:** Ensure `-O2` or `-O3` is used to minimize DSP processing time.
- **Interrupt Priorities:** I2S DMA interrupts must have the highest priority in the system (above UART/Bluetooth) to prevent audio jitter and buffer underflows/overflows.
