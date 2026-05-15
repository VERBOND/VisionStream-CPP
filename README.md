# TestTaskCamera

Simple low-latency camera streaming application using C++20 and OpenCV.

---

## Features

* Real-time camera streaming
* FPS monitoring
* Latency measurement
* Dropped frame counter
* Automatic camera reconnect
* Multithreaded frame capture
* Configurable resolution and FPS

---

## Tech Stack

* C++20
* OpenCV
* CMake
* std::jthread
* Multithreading

---

## Why OpenCV

I chose OpenCV because:

* Fast integration with camera devices
* Simple API for frame processing and rendering
* Cross-platform support
* Easy future extension for image processing / AI / CV tasks

Alternative approaches like V4L2 or GStreamer provide lower-level control and potentially lower latency, but require significantly more boilerplate and platform-specific handling.

For this task, OpenCV gives the best balance between:

* development speed
* readability
* maintainability
* functionality

---

## Dependencies

### Ubuntu / Debian

\```bash
sudo apt update

sudo apt install -y \
    build-essential \
    cmake \
    libopencv-dev
\```

---

## Build

### Clone repository

\```bash
git clone <your_repo_url>
cd TestTaskCamera
\```

### Configure and build

\```bash
mkdir build
cd build

cmake ..
make -j$(nproc)
\```

Executable will be generated as:

\```bash
./TestTaskCamera
\```

---

## Run

### Default

\```bash
./TestTaskCamera
\```

Default parameters:

* device = `0`
* resolution = `1280x720`
* fps = `30`

---

### Custom parameters

\```bash
./TestTaskCamera \
    --device 0 \
    --width 1920 \
    --height 1080 \
    --fps 60
\```

---

## Command Line Arguments

| Argument | Description |
|---|---|
| `--device` | Camera index or device path |
| `--width` | Frame width |
| `--height` | Frame height |
| `--fps` | Target FPS |

Examples:

\```bash
./TestTaskCamera --device 1
\```

\```bash
./TestTaskCamera --device /dev/video0
\```

\```bash
./TestTaskCamera --width 640 --height 480 --fps 120
\```

---

## Architecture

### CameraSource

Responsible for:

* opening camera device
* configuring stream
* frame capture
* reconnect handling

---

### Renderer

Responsible for:

* rendering frames
* overlaying statistics:
  * FPS
  * latency
  * dropped frames

---

### App

Main application controller.

Contains:

* capture thread
* shared frame buffer
* synchronization logic
* render loop

---

## Multithreading Model

The application uses two logical threads.

### Capture Thread

Continuously:

* reads frames from camera
* updates shared buffer
* reconnects camera if stream fails

### Main Thread

Continuously:

* receives latest frame
* calculates FPS
* calculates latency
* renders output

---

## Frame Dropping Strategy

The application intentionally drops outdated frames:

\```cpp
if (shared.ready) shared.dropped++;
\```

This minimizes latency and ensures renderer always shows the newest available frame.

Tradeoff:

* lower latency
* possible frame loss

This behavior is preferable for:

* live streaming
* robotics
* real-time CV systems
* teleoperation

---

## Implemented Level

Implemented features:

* [x] Camera initialization
* [x] Real-time frame capture
* [x] FPS measurement
* [x] Latency measurement
* [x] Multithreaded processing
* [x] Frame dropping strategy
* [x] Automatic reconnect
* [x] Command line configuration
* [x] Graceful termination

---

## Bottlenecks / Limitations

### 1. OpenCV VideoCapture overhead

`cv::VideoCapture` is simple but not the most performant backend.

Potential improvement:

* direct V4L2 integration
* GStreamer pipeline

---

### 2. Frame cloning

\```cpp
frame_to_draw = shared.frame.clone();
\```

This introduces extra memory copy overhead.

Potential improvement:

* lock-free queue
* double buffering
* move-only frame ownership

---

### 3. Mutex contention

Current synchronization uses:

\```cpp
std::mutex
\```

Potential improvement:

* lock-free structures
* ring buffer
* atomic frame swapping

---

### 4. Single frame buffer

Only latest frame is stored.

Potential issue:

* unstable rendering under heavy processing

Potential improvement:

* bounded queue/ring buffer

---

## Future Improvements

If continuing development, I would add:

* GStreamer backend
* V4L2 backend
* Hardware accelerated decoding
* Async logging
* Ring-buffer frame queue
* Metrics exporter
* Recording support
* Network stream support (RTSP/UDP)
* GUI controls
* Better error handling
* Unit tests
* Benchmark mode

---

## Example Output

\```text
!!!!CAMERA INITIALIZED: 1280x720
!!!!STATS: FPS=30 LAT=4ms
\```

---

## Exit

Press:

\```text
q
\```

or

\```text
ESC
\```

to terminate application.

---

## Notes

This implementation prioritizes:

* simplicity
* readability
* low latency
* real-time responsiveness

over:

* perfect frame preservation
* maximum throughput
* complex buffering systems
