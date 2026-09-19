# SLAM Toolbox + Nav2 for BPX

This package provides a single launch surface for online mapping with SLAM Toolbox and saved-map navigation with AMCL and Nav2.

## Modes

| Argument | Values | Purpose |
| --- | --- | --- |
| `mode` | `slam`, `nav` | Select online mapping or saved-map navigation. |
| `odom_source` | `bpx`, `ekf`, `rf2o` | Select direct BPX odometry, fused odometry or LiDAR scan matching. |
| `map` | YAML path | Occupancy map used in `nav` mode. |
| `use_rviz` | Boolean | Start the supplied RViz configuration. |

Only one odometry mode is launched at a time so a single component owns `odom -> base_link`.

## Build

```bash
source /opt/ros/humble/setup.bash
colcon build --symlink-install --packages-select \
  bpx_driver bpx_ekf_bringup sllidar_ros2 \
  rf2o_laser_odometry slam_toolbox_nav2
source install/setup.bash
```

## Online mapping

```bash
ros2 launch slam_toolbox_nav2 bringup.launch.py \
  mode:=slam odom_source:=ekf
```

Save the resulting map:

```bash
ros2 run nav2_map_server map_saver_cli \
  -f src/slam_toolbox_nav2/maps/my_map
```

## Saved-map navigation

```bash
ros2 launch slam_toolbox_nav2 bringup.launch.py \
  mode:=nav \
  map:=src/slam_toolbox_nav2/maps/my_map.yaml \
  odom_source:=ekf
```

Use `odom_source:=rf2o` when evaluating LiDAR scan matching instead of platform odometry.

## Frame ownership

```text
map              -> odom        SLAM Toolbox (mapping) or AMCL (navigation)
odom             -> base_link   BPX driver, robot_localization, or RF2O
base_link        -> laser       static transform in the launch configuration
```

## Evidence and limitations

Saved `.yaml + .pgm` maps are included as integration artefacts. They show that maps were produced and retained, but they do not by themselves establish localisation accuracy or navigation success rate. The repository does not currently include bags, fixed-seed evaluation runs or ground-truth trajectories; see [evaluation protocol](../../docs/evaluation.md).
