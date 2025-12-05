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

# Select application by editing makefile APP_TYPE variable, e.g.:
# APP_TYPE = tflm_yolov8_od

# Generate flashable image (from we2_image_gen_local directory)
cd we2_image_gen_local
cp ../EPII_CM55M_APP_S/obj_epii_evb_icv30_bdv10/gnu_epii_evb_WLCSP65/EPII_CM55M_gnu_epii_evb_WLCSP65_s.elf input_case1_secboot/
./we2_local_image_gen project_case1_blp_wlcsp.json
# Output: output_case1_sec_wlcsp/output.img (max 1MB)

# Flash firmware via xmodem (Python)
cd xmodem
pip install -r requirements.txt
python xmodem_send.py <COM_PORT> <firmware.img>
```

**Toolchain:** ARM GNU Toolchain v13.2.rel1 (`arm-gnu-toolchain-13.2.rel1`)

## Architecture

```
EPII_CM55M_APP_S/           # Main firmware
├── app/
│   ├── main.c              # Entry point
│   └── scenario_app/       # Application examples (15+ apps)
│       ├── tflm_yolov8_od/       # YOLOv8 object detection
│       ├── tflm_yolov8_pose/     # Pose estimation
│       ├── tflm_fd_fm/           # Face detection/mesh
│       ├── kws_pdm_record/       # Keyword spotting
│       ├── allon_sensor_tflm/    # Sensor + inference
│       └── ei_standalone_*/      # Edge Impulse apps
├── library/
│   ├── inference/          # TFLite Micro (two versions)
│   ├── cmsis_nn/           # CMSIS-NN v7.0.0
│   ├── cmsis_dsp/          # Signal processing
│   └── cmsis_cv/           # Computer vision (submodule)
├── drivers/                # Hardware drivers
├── os/rtos2_freertos/      # FreeRTOS 10.5.1
└── makefile                # Build configuration

we2_image_gen_local/        # Image generation tool (binaries for Linux/Win/macOS)
model_zoo/                  # Pre-trained ML models per application
xmodem/                     # Python flashing utility
swd_debugging/              # SWD/pyOCD debugging setup
```

## Key Build Configuration

In `EPII_CM55M_APP_S/makefile`:
- `APP_TYPE`: Select scenario app (e.g., `tflm_yolov8_od`, `tflm_fd_fm`)
- `OS_SEL`: `freertos` or blank for bare-metal
- `TOOLCHAIN`: `gnu` (default) or `arm`
- `EPII_USECASE_SEL`: IC package (`WLCSP65`, `LQFP128`, `QFN88`, `BGA64`)

## Memory Layout

- **0x000000 - 0x200000**: Firmware (2MB reserved)
- **0x200000+**: ML models (must be 4KB aligned)
- Model addresses configured in each app's `common_config.h`

## Adding/Modifying ML Models

1. Place `.tflite` model in appropriate `model_zoo/` subdirectory
2. Update `common_config.h` in the scenario app with flash address
3. Flash model separately after firmware using xmodem

## Hardware Support

- **Cameras**: OV5647, IMX219, IMX477, IMX708 (MIPI CSI)
- **Audio**: PDM microphone
- **IMU**: Motion sensors
- **Serial**: 921600 baud UART

## Debugging

- **Serial output**: Primary debug method (921600 baud)
- **SWD debugging**: See `swd_debugging/README.md` for VS Code + pyOCD setup
- **SVD files**: `WE2_S.svd`, `WE2_S_NS.svd` for register view

## VS Code Integration

Pre-configured in `EPII_CM55M_APP_S/.vscode/`:
- `tasks.json`: Build/clean tasks
- `launch.json`: Debugger config
- `settings.json`: IntelliSense paths
