# Programming Statement — Mai Yunxi

## Summary

My programming experience is centred on robotics integration, localisation and navigation. I use Python and MATLAB for algorithm development and data analysis, and C/C++ for ROS 2 interfaces and platform integration. I describe C/C++ as working proficiency rather than advanced systems expertise; the evidence below distinguishes code visible in this repository from experience documented elsewhere in my application.

## Languages and evidence

| Language | Level and use | Verifiable evidence |
| --- | --- | --- |
| C++ | Working proficiency for ROS 2 nodes, SDK adaptation, messages, services, TF and safety gating. | [`bpx_driver_node.cpp`](../src/bpx_driver/src/bpx_driver_node.cpp), package/CMake configuration, and ROS interface documentation. |
| Python | Primary language for research prototypes, data processing and ROS 2 launch orchestration. | ROS 2 launch files in [`slam_toolbox_nav2`](../src/slam_toolbox_nav2/launch), [`cartographer_nav2`](../src/cartographer_nav2/launch) and [`bpx_ekf_bringup`](../src/bpx_ekf_bringup/launch). |
| MATLAB | Algorithm prototyping and evaluation for navigation, filtering and signal-processing coursework/thesis work. | Described in the application CV and thesis materials; MATLAB source is not included in this repository. |
| C | Formal introductory programming training and certificate-backed coursework. | C programming certificate in the private application evidence set; it is intentionally not published here. |

## Relevant academic preparation

The official transcript supports the following selected results:

- Integrated Navigation — 94/100
- Indoor Positioning Technology — 95/100
- Pattern Recognition and Artificial Intelligence — 89/100
- Digital Signal Processing — 85/100
- Embedded Systems and Programming — 84/100

These subjects support my work with coordinate frames, sensor data, estimation, embedded interfaces and autonomous navigation. The transcript—not this page—remains the authoritative academic record.

## Representative engineering work

### BPX SDK to ROS 2 bridge

I worked on the C++ driver layer that turns vendor SDK data and commands into ROS 2 topics and services. The implementation handles joint/IMU/odometry publication, quaternion normalisation, command timeouts, platform-state gating and TF ownership options. This required reasoning across vendor data semantics, ROS message conventions and physical-platform behaviour.

### Launch and configuration integration

I used Python launch descriptions to make three odometry sources selectable (`bpx`, `ekf`, `rf2o`) while preventing duplicate `odom -> base_link` publishers. I also connected the selected odometry path to SLAM Toolbox or Cartographer and Nav2.

### Research programming outside this repository

My undergraduate navigation work used Python/MATLAB to implement gait detection, Weinberg step-length estimation, heading smoothing, particle-filter map matching and EKF fusion. The reported RMSE reduction from 1.662 m to 0.313 m belongs to that thesis experiment and is not presented as a BPX result.

## Engineering habits

- Keep verified implementation evidence and unverified claims explicitly separated.
- Prefer reproducible launch/configuration paths over undocumented terminal sequences.
- Record frame ownership and timing assumptions because localisation failures are often interface failures.
- Disable physical actuation when calibration or state validity is unresolved.
- Report missing benchmarks as missing rather than inferring performance from screenshots or saved maps.

## Current growth areas

I am developing deeper capability in C++ software design, automated ROS 2 testing, statistically grounded covariance tuning and controlled navigation benchmarking. These are stated as growth areas because the current repository demonstrates integration competence but does not yet provide a complete ground-truth evaluation suite.
