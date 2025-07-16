# UCX/OS with LLTF Real-Time Scheduler (by Bruno)

This project extends the UCX/OS microkernel (by Prof. Sérgio Johann Filho) with a Least Laxity Time First (LLTF) scheduler, adding real-time capabilities to an embedded system running on the HF-RISCV (RV32E) platform and simulator.

The goal was to solve the real-time scheduling problem, ensuring tasks meet deadlines under periodic constraints.

## Implemented Changes

### LLTF Scheduler (`rtsched.c`)

* Implements LLTF: selects the task with the smallest slack (time before deadline minus remaining computation).
* Handles periodic tasks, resetting remaining time each cycle.
* Detects and reports deadline misses.
* Uses STOP\_TIME as a simulation limit.

### Kernel Modifications (`ucx.c`, `kernel.h`)

* Added `lstf_parameter` struct with computation, period, deadline, slack, remaining.
* Extended TCB (`struct tcb_s`) with pointer to RT parameters.
* Added `ucx_task_ifno()` to let tasks access their RT parameters.
* Modified `ucx_task_rt_priority()` to register RT parameters.

### Application Example (`app_main`)

* Spawns multiple tasks (task0, task1, task2, task\_idle).
* Assigns RT parameters to tasks.
* Prints task status: ID, computation, period, deadline, slack, remaining.

## How to Compile and Run

### Requirements

* RISC-V toolchain: riscv64-unknown-elf-gcc
* HF-RISCV simulator: [https://github.com/sjohann81/hf-risc](https://github.com/sjohann81/hf-risc)
* UCX/OS kernel: [https://github.com/sjohann81/ucx-os](https://github.com/sjohann81/ucx-os)

### Commands

```bash
git clone https://github.com/sjohann81/hf-risc
git clone https://github.com/sjohann81/ucx-os

cd hf-risc/tools/sim/hf_riscv_sim
make
cd ../../../

cd ucx-os
make ucx ARCH=riscv/hf-riscv-e

# Place rtsched.c in app/
make rtsched

../hf-risc/tools/sim/hf_riscv_sim/hf_riscv_sim build/target/image.bin
```

For debug output:

```bash
../hf-risc/tools/sim/hf_riscv_sim/hf_riscv_sim build/target/image.bin out.txt
```

## Summary

* Added real-time scheduling (LLTF) to UCX/OS.
* Enhanced kernel structures and APIs for RT tasks.
* Provided an example application showing RT behavior.
* Detailed instructions for compilation and simulation.
