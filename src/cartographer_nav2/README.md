# Cartographer + Nav2 for BPX

This package integrates Cartographer mapping/pure localisation with Nav2 on the BPX quadruped.

## Modes

| Mode | Required map input | Result |
| --- | --- | --- |
| `mapping` | None | Builds a Cartographer trajectory and occupancy grid. |
| `localization` | `.pbstream` | Runs Cartographer in pure-localisation mode. |
| `nav` | `.pbstream` plus `.yaml/.pgm` | Localises with Cartographer and runs Nav2. |

All modes accept `odom_source:=bpx|ekf|rf2o`.

## Build

```bash
source /opt/ros/humble/setup.bash
colcon build --symlink-install --packages-select \
  bpx_driver bpx_ekf_bringup sllidar_ros2 \
  rf2o_laser_odometry cartographer_nav2
source install/setup.bash
```

## Mapping

```bash
ros2 launch cartographer_nav2 bringup.launch.py \
  mode:=mapping odom_source:=ekf
```

## Pure localisation

```bash
ros2 launch cartographer_nav2 bringup.launch.py \
  mode:=localization \
  pbstream:=src/cartographer_nav2/maps/map5291.pbstream \
  odom_source:=ekf
```

## Localisation and navigation

```bash
ros2 launch cartographer_nav2 bringup.launch.py \
  mode:=nav \
  pbstream:=src/cartographer_nav2/maps/map5291.pbstream \
  map:=src/cartographer_nav2/maps/map5291.yaml \
  odom_source:=ekf
```

## Frame ownership

Cartographer publishes `map -> odom`. Exactly one selected odometry backend publishes `odom -> base_link`; the launch conditions prevent the BPX driver, EKF and RF2O from publishing that transform simultaneously.

## Evidence and limitations

Several `.pbstream` and occupancy-map snapshots are included as development artefacts. They are not a benchmark set. LiDAR mounting transforms differ between some historical launch configurations and must be measured on the target robot before comparing mapping quality. See [evaluation protocol](../../docs/evaluation.md) for the metrics required before making quantitative claims.
