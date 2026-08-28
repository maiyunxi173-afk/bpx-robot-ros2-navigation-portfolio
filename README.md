# BPX Robot ROS2 Navigation Portfolio

Standalone portfolio of my BPX quadruped ROS2 navigation work from a collaborative robot console project. The repository focuses on the parts I owned and integrated: BPX driver adaptation, odometry output, EKF localization bringup, LiDAR SLAM mapping, and Nav2 navigation.

## What This Shows

- ROS2 driver bridge for BPX SDK: `/cmd_vel`, robot services, status topics, IMU, joint states, and odometry.
- EKF localization bringup with `robot_localization`, fusing BPX odometry and IMU while keeping TF ownership clean.
- Two mapping/navigation routes: `slam_toolbox + AMCL + Nav2` and `Cartographer + Nav2`.
- Switchable odometry source: BPX leg/body odometry or `rf2o_laser_odometry` fallback from LiDAR scans.
- Real-robot integration notes for TF conflicts, motion-state safety, ROS domain consistency, LiDAR frames, and map persistence.

## Architecture

```text
BPX SDK -> bpx_driver -> /odom + /imu/data -> bpx_ekf_bringup -> odom->base_link
LiDAR   -> sllidar_ros2 -> /scan -> SLAM Toolbox / Cartographer -> map->odom
Nav2    -> global/local planning -> /cmd_vel -> bpx_driver -> BPX robot
```

See [project_showcase.md](docs/project_showcase.md) for the portfolio write-up and [runbook.md](docs/runbook.md) for build/run commands.

## Repository Layout

```text
src/
├── bpx_driver/              # BPX SDK ROS2 driver, odometry, IMU, services
├── bpx_ekf_bringup/         # robot_localization EKF config and launch
├── sllidar_ros2/            # LiDAR driver
├── rf2o_laser_odometry/     # fallback scan-matching odometry
├── slam_toolbox_nav2/       # slam_toolbox mapping + AMCL/Nav2 navigation
└── cartographer_nav2/       # Cartographer mapping/localization + Nav2
```

## Quick Start

```bash
colcon build --symlink-install
source install/setup.bash
ros2 launch bpx_ekf_bringup ekf_localization.launch.py robot_ip:=10.21.20.1
```

Mapping with slam_toolbox:

```bash
ros2 launch slam_toolbox_nav2 bringup.launch.py mode:=slam odom_source:=ekf
```

Navigation with a saved map:

```bash
ros2 launch slam_toolbox_nav2 bringup.launch.py \
  mode:=nav \
  map:=src/slam_toolbox_nav2/maps/my_map.yaml \
  odom_source:=ekf
```

## My Role

I extracted this subsystem from the broader HRS Web Console collaboration and reorganized it as a focused engineering portfolio. My work centers on the ROS2 navigation stack: making the BPX robot expose standard ROS interfaces, producing usable odometry, wiring localization/SLAM/Nav2 launch paths, and documenting the practical integration decisions needed for real-robot operation.

## Source

Extracted and reorganized from `Maker1201/Hrs_Web_Console` at local source commit `ac603b8`. See [source_attribution.md](docs/source_attribution.md).
