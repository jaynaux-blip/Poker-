// Included by every engine .cpp: no fused multiply-add in this translation unit, whatever the
// build flags. A fused a * b + c rounds once instead of twice, which changes results in the last
// bit and breaks parity with the TypeScript build (179 golden vectors fail with it).
// Clang contracts by default on ARM64 (Apple Silicon Macs), and this pragma turns that off
// without relying on the toolchain's flags. It cannot override -ffp-contract=fast or
// fast-math, so never build this module with those. GCC has no reliable pragma: the Standalone
// build passes -ffp-contract=off.
#pragma once

#if defined(__clang__)
#pragma clang fp contract(off)
#elif defined(_MSC_VER)
#pragma fp_contract(off)
#endif
