# BPX EKF Bring-up

`bpx_ekf_bringup` configures the standard ROS 2 `robot_localization/ekf_node` for planar BPX odometry and IMU fusion.

## What this package owns

- Starts `bpx_driver` with direct odometry TF publication disabled.
- Starts `ekf_filter_node` at 50 Hz.
- Fuses `/odom` with `/imu/data` under `two_d_mode`.
- Publishes `/odom/filtered` and owns `odom -> base_link`.

This package is configuration and orchestration around `robot_localization`; it is not a custom EKF implementation.

## Run

```bash
source /opt/ros/humble/setup.bash
colcon build --symlink-install --packages-select bpx_driver bpx_ekf_bringup
source install/setup.bash

ros2 launch bpx_ekf_bringup ekf_localization.launch.py \
  robot_ip:=10.21.20.1
```

## Check output and frames

```bash
ros2 topic echo /odom/filtered --once
ros2 run tf2_ros tf2_echo odom base_link
ros2 run tf2_tools view_frames
```

## Configuration notes

The current YAML uses fixed process noise and sensor covariances selected during integration. These values are starting points rather than identified statistical models. Before reporting localisation accuracy, collect timestamped bags and ground truth, validate frame conventions, and tune the filter against a documented route and metric set.
