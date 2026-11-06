import vitis
import os
import subprocess
import shutil


WORKSPACE    = "/home/streben/Documents/POLIMI/PhD/Projects/VersalAIEdge/"
PLATFORM     = "TE0950"
PLM          = "versal_plm"
APP          = "freertos_hello_world_a72"
HW_DESIGN    = "TE0950_DDR_TIM_400MHz"
CORE         = "a72-0"          # or a72-1 for the second A72 core
BOOTGEN      = "/opt/Xilinx/2025.2/Vitis/bin/bootgen"
SDTGEN       = "/opt/Xilinx/2025.2/Vitis/bin/sdtgen"
OUTPUT_DIR   = os.path.join(WORKSPACE, "boot_output")
BIF_PATH     = os.path.join(OUTPUT_DIR, "boot.bif")
PDI_PATH     = os.path.join(OUTPUT_DIR, "boot.pdi")
XSA_PATH     = os.path.join(WORKSPACE, PLATFORM, "hw", f"{HW_DESIGN}.xsa")
SDT_OUT      = os.path.join(WORKSPACE, PLATFORM, "hw/sdt")


# Clean project before building
subprocess.run(
    ["rm", "-rf", "freertos_hello_world_a72/build"],
    check=True
)

subprocess.run(
    ["rm", "-rf", "TE0950/psv_cortexa72_0/freertos_psv_cortexa72_0/bsp/libsrc/build_configs"],
    check=True
)

HW_PDI = os.path.join(SDT_OUT, "hw_design.pdi")
HW_PDI_MODIFIED = os.path.join(SDT_OUT, "hw_modified_plm.pdi")
PDI_FILES_PATH = os.path.join(SDT_OUT, "extracted/design_1_wrapper_1/pdi_files")
HW_BIF_PATH = os.path.join(PDI_FILES_PATH, "TE0950_DDR_TIM.bif")

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

# plm = client.get_component(name=PLM)
# plm.build()

comp = client.get_component(name=APP)
comp.build()


# ----------------------------------------------------------------
# 2. Regenerate hardware PDI with modified PLM firmware
# ----------------------------------------------------------------

plm_firmware = os.path.join(WORKSPACE, PLM, "build", f"{PLM}.elf")
plm_firmware_target = os.path.join(SDT_OUT, f"extracted/design_1_wrapper_1/pdi_files/gen_files/plm.elf")

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

# Application ELF produced by the build
elf = os.path.join(
    WORKSPACE, APP, "build", f"{APP}_aspis"
)

if not os.path.isfile(HW_PDI_MODIFIED):
    raise FileNotFoundError(f"Hardware PDI not found: {HW_PDI_MODIFIED}")
if not os.path.isfile(elf):
    raise FileNotFoundError(f"Application ELF not found: {elf}")

# ----------------------------------------------------------------
# 4. Write Application BIF file
# ----------------------------------------------------------------
os.makedirs(OUTPUT_DIR, exist_ok=True)

bif_content = f"""
all:
{{
    image {{
        partition {{ type=bootimage, file={HW_PDI_MODIFIED} }}
    }}

    image {{
        name=user_elfs_subsystem
        partition {{ core={CORE}, file={elf} }}
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
