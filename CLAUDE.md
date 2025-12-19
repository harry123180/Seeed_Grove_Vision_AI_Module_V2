# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Embedded AI/ML firmware for the Seeed Grove Vision AI Module V2 (Himax WiseEye2 HX6538 Cortex-M55). Supports computer vision and audio inference with TensorFlow Lite Micro, CMSIS-NN acceleration, and Edge Impulse integration.

## Build Commands

```bash
# Build firmware (from EPII_CM55M_APP_S directory)
cd EPII_CM55M_APP_S
make clean
make

# Generate flashable image (from we2_image_gen_local directory)
cd we2_image_gen_local
cp ../EPII_CM55M_APP_S/obj_epii_evb_icv30_bdv10/gnu_epii_evb_WLCSP65/EPII_CM55M_gnu_epii_evb_WLCSP65_s.elf input_case1_secboot/

# Linux/macOS:
./we2_local_image_gen project_case1_blp_wlcsp.json
# macOS ARM64:
./we2_local_image_gen_macOS_arm64 project_case1_blp_wlcsp.json
# Windows:
we2_local_image_gen project_case1_blp_wlcsp.json

# Output: output_case1_sec_wlcsp/output.img (max 1MB)
```

**Toolchain:** ARM GNU Toolchain v13.2.rel1 (`arm-gnu-toolchain-13.2.rel1`)

## Flashing Firmware

```bash
# Install dependencies
pip install -r xmodem/requirements.txt

# Flash firmware only
python xmodem/xmodem_send.py --port=COM3 --baudrate=921600 --protocol=xmodem --file=we2_image_gen_local/output_case1_sec_wlcsp/output.img

# Flash firmware + model (model address from app's common_config.h)
python xmodem/xmodem_send.py --port=COM3 --baudrate=921600 --protocol=xmodem \
  --file=we2_image_gen_local/output_case1_sec_wlcsp/output.img \
  --model="model_zoo/tflm_yolov8_od/yolov8n_od_192_delete_transpose_0xB7B000.tflite 0xB7B000 0x00000"
```

## Architecture

```
EPII_CM55M_APP_S/           # Main firmware
├── app/
│   ├── main.c              # Entry point
│   └── scenario_app/       # Application examples (20+ apps)
├── library/
│   ├── inference/          # TFLite Micro
│   ├── cmsis_nn/           # CMSIS-NN (v7.0.0 default)
│   ├── cmsis_dsp/          # Signal processing
│   └── cmsis_cv/           # Computer vision (git submodule)
├── drivers/                # Hardware drivers
├── os/rtos2_freertos/      # FreeRTOS 10.5.1
└── makefile                # Build configuration

we2_image_gen_local/        # Image generation (platform-specific binaries)
model_zoo/                  # Pre-trained ML models per application
xmodem/                     # Python flashing utility
swd_debugging/              # SWD/pyOCD debugging setup
```

## Key Build Configuration

Edit `EPII_CM55M_APP_S/makefile`:

| Variable | Options | Description |
|----------|---------|-------------|
| `APP_TYPE` | `allon_sensor_tflm`, `tflm_yolov8_od`, `tflm_fd_fm`, `kws_pdm_record`, etc. | Select scenario app |
| `OS_SEL` | `freertos` or blank | FreeRTOS vs bare-metal |
| `TOOLCHAIN` | `gnu` (default), `arm` | Compiler toolchain |
| `IC_PACKAGE_SEL` | `WLCSP65`, `LQFP128`, `QFN88`, `BGA64` | IC package variant |
| `LIB_CMSIS_NN_ENALBE` | `1` or `0` | Enable CMSIS-NN acceleration |
| `LIB_CMSIS_NN_VERSION` | `7_0_0` or blank | CMSIS-NN version |

Available `APP_TYPE` values: `allon_sensor_tflm`, `allon_sensor_tflm_freertos`, `allon_sensor_tflm_cmsis_nn`, `tflm_yolov8_od`, `tflm_yolov8_pose`, `tflm_yolov8_gender_cls`, `tflm_yolo11_od`, `tflm_fd_fm`, `tflm_peoplenet`, `kws_pdm_record`, `pdm_record`, `imu_read`, `ei_standalone_inferencing`, `ei_standalone_inferencing_camera`, `edge_impulse_firmware`, `hello_world_cmsis_dsp`, `hello_world_cmsis_cv`

## Memory Layout

- **0x000000 - 0x200000**: Firmware (2MB reserved, max 1MB image)
- **0x200000+**: ML models (must be 4KB aligned)
- Model flash addresses defined in each app's `common_config.h`

## Adding Custom Camera Support

Edit the app's `.mk` file (e.g., `allon_sensor_tflm.mk`):
```makefile
CIS_SUPPORT_INAPP_MODEL = cis_ov5647   # Default
# CIS_SUPPORT_INAPP_MODEL = cis_imx219  # RPi Camera V2
# CIS_SUPPORT_INAPP_MODEL = cis_imx477  # RPi HQ Camera
# CIS_SUPPORT_INAPP_MODEL = cis_imx708  # RPi Camera V3
```

## Hardware

- **Serial**: 921600 baud UART for debug output and xmodem flashing
- **Cameras**: OV5647, IMX219, IMX477, IMX708 (MIPI CSI)
- **Audio**: PDM microphone
- **Debugging**: SWD via CMSIS-DAP compatible probe (see `swd_debugging/README.md`)

## Debugging

1. **Serial output**: Primary method at 921600 baud
2. **SWD debugging**: VS Code + pyOCD setup in `swd_debugging/`
   - Requires custom pyOCD wheel: `pyocd_hx-0.34.3.dev0+dirty`
   - SVD files: `WE2_S.svd`, `WE2_S_NS.svd` for register view
3. **Boot menu**: Hold any key (except Enter) while pressing reset to enter boot options
