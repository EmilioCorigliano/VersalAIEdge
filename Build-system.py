import vitis
import os
import subprocess
import shutil


WORKSPACE    = "/home/streben/Documents/POLIMI/PhD/Projects/VersalAIEdge/"
PLATFORM     = "TE0950"
PLM          = "versal_plm"
HW_DESIGN    = "binary_container_1"
BOOTGEN      = "/opt/Xilinx/2025.2/Vitis/bin/bootgen"
SDTGEN       = "/opt/Xilinx/2025.2/Vitis/bin/sdtgen"
OUTPUT_DIR   = os.path.join(WORKSPACE, "boot_output")
BIF_PATH     = os.path.join(OUTPUT_DIR, "boot.bif")
PDI_PATH     = os.path.join(OUTPUT_DIR, "boot.pdi")
XSA_PATH     = os.path.join(WORKSPACE, PLATFORM, "hw", f"{HW_DESIGN}.xsa")
SDT_OUT      = os.path.join(WORKSPACE, PLATFORM, "hw/sdt")
PDI_FILES_PATH = os.path.join(SDT_OUT, "extracted/PFM_EUSBD_wrapper_1/pdi_files")
HW_BIF_PATH  = os.path.join(PDI_FILES_PATH, "vpl_gen_fixed.bif")
# PDI_FILES_PATH = os.path.join(SDT_OUT, "extracted/system_wrapper_1/pdi_files")
# HW_BIF_PATH  = os.path.join(PDI_FILES_PATH, "standalone_wrapper.bif")

## R5
Enable_R5       = True
APP_R5          = "freertos_hello_world"
CORE_R5         = "r5-0"

## A72
Enable_A72      = False
APP_A72         = "freertos_hello_world_a72"
CORE_A72        = "a72-0"


ELF_R5_0     = os.path.join(WORKSPACE, PLATFORM, "dummy_r5-0")
ELF_R5_1     = os.path.join(WORKSPACE, PLATFORM, "dummy_r5-1")
ELF_A72_0     = os.path.join(WORKSPACE, PLATFORM, "dummy_a72-0")
ELF_A72_1     = os.path.join(WORKSPACE, PLATFORM, "dummy_a72-1")


# Clean project before building
subprocess.run(
    ["rm", "-rf", "freertos_hello_world/build"],
    check=True
)

subprocess.run(
    ["rm", "-rf", "freertos_hello_world_a72/build"],
    check=True
)

subprocess.run(
    ["rm", "-rf", "TE0950/psv_cortexr5_0/freertos_psv_cortexr5_0/bsp/libsrc/build_configs"],
    check=True
)

subprocess.run(
    ["rm", "-rf", "TE0950/psv_cortexa72_0/freertos_psv_cortexa72_0/bsp/libsrc/build_configs"],
    check=True
)

subprocess.run(
    ["rm", "-rf", f"{SDT_OUT}/extracted"],
    check=True
)

subprocess.run(
    "rm -rf TE0950/hw/sdt/*.pdi TE0950/hw/sdt/*.mmi TE0950/hw/sdt/*.isd TE0950/hw/sdt/*.dts",
    shell=True,
    check=True
)

HW_PDI_MODIFIED = os.path.join(SDT_OUT, f"{HW_DESIGN}_modified_plm.pdi")
HW_PDI = os.path.join(SDT_OUT, f"vpl_gen_fixed.pdi")

# Step 0: Generate SDT
subprocess.run(
    [SDTGEN, "-xsa", XSA_PATH, "-dir", SDT_OUT],
    check=True
)


# ----------------------------------------------------------------
# 1. Build platform and application
# ----------------------------------------------------------------
client = vitis.create_client()
client.set_workspace(path=WORKSPACE)

platform = client.get_component(name=PLATFORM)
platform.build()

plm = client.get_component(name=PLM)
plm.build()

if Enable_R5:
    comp_r5 = client.get_component(name=APP_R5)
    comp_r5.build()
    if(CORE_R5 == "r5-0"):
        ELF_R5_0 = os.path.join(WORKSPACE, APP_R5, "build", f"{APP_R5}_aspis")
    elif(CORE_R5 == "r5-1"):
        ELF_R5_1 = os.path.join(WORKSPACE, APP_R5, "build", f"{APP_R5}_aspis")
    else:
        print("ERROR! core not found!")
        exit()

if Enable_A72:
    comp_a72 = client.get_component(name=APP_A72)
    comp_a72.build()
    if(CORE_A72 == "a72-0"):
        ELF_A72_0 = os.path.join(WORKSPACE, APP_A72, "build", f"{APP_A72}_aspis")
    elif(CORE_A72 == "a72-1"):
        ELF_A72_1 = os.path.join(WORKSPACE, APP_A72, "build", f"{APP_A72}_aspis")
    else:
        print("ERROR! core not found!")
        exit()



# ----------------------------------------------------------------
# 2. Regenerate hardware PDI with modified PLM firmware
# ----------------------------------------------------------------

plm_firmware = os.path.join(WORKSPACE, PLM, "build", f"{PLM}.elf")
plm_firmware_target = os.path.join(PDI_FILES_PATH, f"gen_files/plm.elf")

# Copy the PLM firmware to the target directory
subprocess.run(
    [f"cp -f {plm_firmware} {plm_firmware_target}"],
    shell=True
)

# regenerate the PDI file with the modified PLM firmware
cmd_hw_pdi = [
    BOOTGEN,
    "-image", HW_BIF_PATH,
    "-arch", "versal",
    "-o", HW_PDI_MODIFIED,
    "-w",                # overwrite existing output
]

print(f"Running bootgen: {' '.join(cmd_hw_pdi)}")
result = subprocess.run(cmd_hw_pdi, capture_output=True, text=True, cwd=PDI_FILES_PATH)

# ----------------------------------------------------------------
# 3. Locate required files
# ----------------------------------------------------------------

if not os.path.isfile(HW_PDI_MODIFIED):
    raise FileNotFoundError(f"Hardware PDI not found: {HW_PDI_MODIFIED}")
if not os.path.isfile(ELF_R5_0):
    raise FileNotFoundError(f"Application ELF0 not found: {ELF_R5_0}")
if not os.path.isfile(ELF_R5_1):
    raise FileNotFoundError(f"Application ELF1 not found: {ELF_R5_1}")
if not os.path.isfile(ELF_A72_0):
    raise FileNotFoundError(f"Application ELF0 not found: {ELF_A72_0}")
if not os.path.isfile(ELF_A72_1):
    raise FileNotFoundError(f"Application ELF1 not found: {ELF_A72_1}")

# ----------------------------------------------------------------
# 4. Write Application BIF file
# ----------------------------------------------------------------
os.makedirs(OUTPUT_DIR, exist_ok=True)

AIE_CDO = "TE0950/hw/aie_debug"

bif_content = f"""
all:
{{
    image {{
        partition {{ type=bootimage, file={HW_PDI_MODIFIED} }}
    }}

    image
    {{
        name=aie_dev_part, id=0x18800000
        {{
            type=cdo, file={AIE_CDO}/aie.cdo.device.partition.reset.bin
        }}
    }}
    image
    {{
        name=aie_image, id=0x18800000
        {{
            type=cdo, file={AIE_CDO}/aie.merged.cdo.bin
        }}
    }}

    image {{
        id = 0x1c000000, name=rpu_subsystem
        partition {{ core=r5-0, file={ELF_R5_0} }}
        partition {{ core=r5-1, file={ELF_R5_1} }}
    }}
    image {{
        name=apu_subsystem
        partition {{ core=a72-0, file={ELF_A72_0} }}
        partition {{ core=a72-1, file={ELF_A72_1} }}
    }}
}}
"""

with open(BIF_PATH, "w") as f:
    f.write(bif_content)

# ----------------------------------------------------------------
# 5. Run bootgen
# ----------------------------------------------------------------
cmd = [
    BOOTGEN,
    "-image", BIF_PATH,
    "-arch", "versal",
    "-o",    PDI_PATH,
    "-w",                # overwrite existing output
]

print(f"Running bootgen: {' '.join(cmd)}")
result = subprocess.run(cmd, capture_output=True, text=True)
print(result.stdout)
if result.returncode != 0:
    print(result.stderr)
    raise RuntimeError(f"bootgen failed with exit code {result.returncode}")

print(f"\nBoot PDI generated: {PDI_PATH}")
