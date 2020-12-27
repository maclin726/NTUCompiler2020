#!/bin/bash
./parser $1
riscv64-linux-gnu-gcc -O0 -static main.S
qemu-riscv64 a.out