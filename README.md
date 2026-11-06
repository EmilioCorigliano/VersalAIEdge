# 

llvm-build:

```
cmake ../llvm \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CROSSCOMPILING=True \
  -DLLVM_TARGETS_TO_BUILD="AArch64;ARM" \
  -G Ninja \
  -DLLVM_PARALLEL_LINK_JOBS=2 \
  -DLLVM_ENABLE_PROJECTS='clang;lld;clang-tools-extra' \
  -DCMAKE_C_COMPILER_TARGET=arm-none-eabi

ninja
```


clean project:

```
rm -rf freertos_hello_world/build
rm -rf TE0950/psv_cortexr5_0/freertos_psv_cortexr5_0/bsp/libsrc/build_configs
```

compile project
```
export VERBOSE=1 # if we want to see verbose output for debugging

vitis -s Build.py
```


program device in xsdb (open xsdb first)
```
connect
targets -set -filter {name =~ "Versal*"}
ta 1
device program <path_to_pdi>
```

How to flip a bit in xsdb (open xsdb first):
```
stop
mrd 0xffE00010 1 // read
mwr 0xffe00010 0x55555555 1 // write
target 3 // context of the third component (should be r5-0)
rrd pc
rrd
con
```

## GPIOs
C_LSI1
 1  2  3  4  5  6
|x |x  |Tx|2 |1 |x |
|x |GND|4 |3 |x |Rx|

1 = 0b0001 -> WSCP_READY_PIN (8 - brown solid)
2 = 0b0010 -> WSCP_FAULT_PIN (7 - brown striped)
3 = 0b0100 -> WSCP_TX_SIG (3 - green striped)
4 = 0b1000 -> WSCP_RUN_PIN (5 - blue striped)
Tx = Versal -> SFERAv2 (2 - orange solid)
Rx = Versal <- SFERAv2 (1 - orange striped)
GND = (6 - green solid)
RST = (4 - blue solid)

## Note on lscript.ld:
- psv_r5_0_atcm_MEM_0, psv_r5_0_btcm_MEM_0 are in the context of the core:
    - r5-0 reaches psv_r5_0_atcm_global_MEM_0 and psv_r5_0_btcm_global_MEM_0
    - r5-1 reaches psv_r5_1_atcm_global_MEM_0 and psv_r5_1_btcm_global_MEM_0
- accessing psv_r5_0_atcm_MEM_0 is a lot faster then psv_r5_0_atcm_global_MEM_0 (same for btcm)
- use two different zones for the cores in the linkerscript, otherwise it does a mess. Using two different areas led to working UART in both cores.

## Useful commands

- get sizes and addresses of the different sections: `llvm-size --radix=16 -A /home/streben/Documents/POLIMI/PhD/Projects/VersalAIEdge/freertos_hello_world/build/freertos_hello_world_aspis` (for position and sizes of single variables see the .map file)

## SelDestroyer
in "/home/streben/Documents/POLIMI/PhD/SFERA/SELDestroyer/host/cli"
./build/seldestroyer --device /dev/ttyUSB0 --config_hold_us 1000000 --config_max_I 0.585 --output A

./build/seldestroyer --device /dev/ttyUSB0 --getCONFIG
./build/seldestroyer --device /dev/ttyUSB0 --monitor


## Program Flash
`program_flash -f /home/streben/Documents/POLIMI/PhD/Projects/VersalAIEdge/boot_output/boot.pdi -pdi TE0950/hw/sdt/vpl_gen_fixed.pdi -flash_type qspi-x8-dual_parallel -blank_check -verify`

Otherwise, for using the SD, just take the boot.pdi and upload on the FAT formatted SD as BOOT.BIN


## SFERAv2

