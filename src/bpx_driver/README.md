# BPX ROS 2 Driver

`bpx_driver` adapts the BPX quadruped SDK to standard ROS 2 control, state, IMU and odometry interfaces. It targets ROS 2 Humble on Ubuntu 22.04 and supports the bundled x86_64 and aarch64 SDK libraries.

## Portfolio relevance

This is the platform-specific layer of the portfolio. The code demonstrates C++ ROS 2 node development, vendor-SDK integration, message/frame handling and basic motion-safety logic. It does not implement a custom state estimator; fusion is handled separately by `bpx_ekf_bringup` through `robot_localization`.

## Interfaces

### Subscribed topic

| Topic | Type | Purpose |
| --- | --- | --- |
| `/cmd_vel` | `geometry_msgs/msg/Twist` | Body-frame planar velocity command. |

### Published topics

| Topic | Type | Purpose |
| --- | --- | --- |
| `/joint_states` | `sensor_msgs/msg/JointState` | Position, velocity and effort for 12 joints. |
| `/imu/data` | `sensor_msgs/msg/Imu` | Normalised orientation, angular velocity and acceleration. |
| `/odom` | `nav_msgs/msg/Odometry` | BPX leg/body odometry or configured fallback integration. |
| `/bpx/battery` | `std_msgs/msg/UInt8` | Battery percentage. |
| `/bpx/battery_current` | `std_msgs/msg/Float32` | Battery current. |
| `/bpx/motion_state` | `std_msgs/msg/UInt8` | Current platform motion-state code. |
| `/bpx/gait` | `std_msgs/msg/UInt8` | Current gait code. |
| `/bpx/imu/data_raw` | `sensor_msgs/msg/Imu` | IMU stream exposed for downstream estimation. |
| `/bpx/joint_states` | `sensor_msgs/msg/JointState` | Joint stream exposed for downstream estimation. |
| `/bpx/joint_torques` | `std_msgs/msg/Float64MultiArray` | Joint-torque stream. |

### Services

| Service | Type | Behaviour |
| --- | --- | --- |
| `/bpx/stand_up` | `std_srvs/srv/Trigger` | Requests stand-up and resets the local odometry reference. |
| `/bpx/sit_down` | `std_srvs/srv/Trigger` | Stops velocity control before requesting sit-down. |
| `/bpx/damping` | `std_srvs/srv/Trigger` | Stops velocity control before requesting damping mode. |

## Safety behaviour

- Commands are forwarded only when the SDK reports motion state `6` (standing).
- If `/cmd_vel` is not refreshed within `cmd_vel_timeout_sec` (0.5 s by default), the driver sends zero velocity and disables velocity control.
- Sit-down and damping service calls disable velocity control first.
- Invalid startup IMU quaternions are rejected; optional startup-yaw zeroing preserves roll and pitch.

These are integration safeguards, not a certified safety controller. Keep an independent emergency stop available during hardware tests.

## Odometry behaviour

With `prefer_leg_odom:=true`, the driver uses finite BPX SDK leg-odometry x/y values relative to the startup reference and IMU orientation for attitude. If leg x/y remains frozen while a non-zero command is active, the driver marks it unreliable and falls back to integrating the active command. If leg odometry is unavailable, SDK body velocity is integrated; commanded velocity is a final fallback when the SDK velocity field is also unavailable.

The command-integration fallback is intentionally documented as low-confidence behaviour. It is useful for interface continuity, not quantitative localisation evaluation.

## Build and run

```bash
source /opt/ros/humble/setup.bash
colcon build --symlink-install --packages-select bpx_driver
source install/setup.bash

ros2 launch bpx_driver bpx_driver.launch.py robot_ip:=10.21.20.1
ros2 service call /bpx/stand_up std_srvs/srv/Trigger
```

Check the key streams before commanding motion:

```bash
ros2 topic echo /imu/data --once
ros2 topic echo /odom --once
ros2 topic echo /bpx/motion_state --once
```

## Parameters

| Parameter | Default | Meaning |
| --- | --- | --- |
| `robot_ip` | `10.21.20.1` | BPX SDK endpoint on the robot network. |
| `cmd_vel_timeout_sec` | `0.5` | Maximum age of an active velocity command. |
| `odom_frame` | `odom` | Odometry parent frame. |
| `base_frame` | `base_link` | Robot base frame. |
| `imu_frame` | `imu_link` | IMU frame. |
| `publish_odom_tf` | `true` | Whether this node owns `odom -> base_link`. |
| `use_ekf_odom` | `true` in the node, overridden by launch | Enables additional estimator-facing streams instead of direct odometry publication. |
| `prefer_leg_odom` | `true` | Prefer the SDK leg-odometry field when valid. |
| `zero_yaw_on_start` | `true` | Remove the initial yaw offset. |

## Known limitations

- Covariance values are configured constants rather than identified noise models.
- Command integration cannot observe slip and should not be reported as measured odometry.
- The bundled SDK is vendor/platform dependent; check redistribution and version compatibility before reuse.
- No raw rosbag or calibrated ground-truth trajectory is included in this repository.
