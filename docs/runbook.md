# Runbook

## Environment

- ROS2 Humble on Ubuntu 22.04
- BPX robot reachable at `10.21.20.1`
- SLAMTEC/RPLIDAR connected as `/dev/ttyUSB0`
- Recommended: keep every terminal on the same `ROS_DOMAIN_ID`

## Build

```bash
cd bpx-robot-ros2-navigation-portfolio
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source install/setup.bash
```

Build only the navigation path:

```bash
colcon build --symlink-install --packages-select \
  bpx_driver bpx_ekf_bringup sllidar_ros2 rf2o_laser_odometry slam_toolbox_nav2
```

## BPX Driver Only

```bash
ros2 launch bpx_driver bpx_driver.launch.py robot_ip:=10.21.20.1
```

Stand the robot before sending navigation velocity:

```bash
ros2 service call /bpx/stand_up std_srvs/srv/Trigger
```

## EKF Odometry Fusion

```bash
ros2 launch bpx_ekf_bringup ekf_localization.launch.py robot_ip:=10.21.20.1
```

Expected frame ownership:

```text
bpx_driver publishes: /odom, /imu/data, /joint_states, /bpx/*
ekf_filter_node publishes: /odom/filtered and odom -> base_link
```

Only one node should publish `odom -> base_link`.

## SLAM Toolbox Mapping

```bash
ros2 launch slam_toolbox_nav2 bringup.launch.py mode:=slam odom_source:=ekf
```

With scan-matching odometry fallback:

```bash
ros2 launch slam_toolbox_nav2 bringup.launch.py mode:=slam odom_source:=rf2o
```

Save the map:

```bash
ros2 run nav2_map_server map_saver_cli -f src/slam_toolbox_nav2/maps/my_map
```

## SLAM Toolbox + Nav2

```bash
ros2 launch slam_toolbox_nav2 bringup.launch.py \
  mode:=nav \
  map:=src/slam_toolbox_nav2/maps/my_map.yaml \
  odom_source:=ekf
```

## Cartographer Mapping

```bash
ros2 launch cartographer_nav2 bringup.launch.py mode:=mapping odom_source:=ekf
```

## Cartographer Localization + Nav2

```bash
ros2 launch cartographer_nav2 bringup.launch.py \
  mode:=nav \
  pbstream:=src/cartographer_nav2/maps/map5291.pbstream \
  map:=src/cartographer_nav2/maps/map5291.yaml \
  odom_source:=ekf
```

## Quick Checks

```bash
ros2 topic echo /odom --once
ros2 topic echo /imu/data --once
ros2 topic echo /scan --once
ros2 run tf2_tools view_frames
ros2 action list | rg navigate
```

If the web console or another ROS process is attached, verify the same domain:

```bash
echo $ROS_DOMAIN_ID
ros2 node list
```
