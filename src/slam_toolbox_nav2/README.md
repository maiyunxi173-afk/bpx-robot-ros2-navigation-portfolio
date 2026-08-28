# slam_toolbox_nav2

BPX 机器狗的 `slam_toolbox + AMCL + Nav2` 方案。

## 功能

- `mode:=slam`：启动 BPX 驱动、RPLIDAR、`slam_toolbox` 建图和可选 RViz。
- `mode:=nav`：启动 BPX 驱动、RPLIDAR、`map_server`、AMCL、Nav2 和可选 RViz。
- `odom_source:=bpx`：默认使用 `bpx_driver` 发布的 `/odom` 与 `odom -> base_link`。
- `odom_source:=rf2o`：使用 `rf2o_laser_odometry` 根据 `/scan` 发布 `/odom` 与 `odom -> base_link`，`bpx_driver` 只负责控制和状态。

## 启动

建图：

```bash
ros2 launch slam_toolbox_nav2 bringup.launch.py mode:=slam
```

建图并改用 rf2o 里程计：

```bash
ros2 launch slam_toolbox_nav2 bringup.launch.py mode:=slam odom_source:=rf2o
```

导航：

```bash
ros2 launch slam_toolbox_nav2 bringup.launch.py mode:=nav map:=/path/to/map.yaml
```

默认地图为：

```text
share/slam_toolbox_nav2/maps/my_map.yaml
```

## TF

```text
map -> odom -> base_link -> laser
```

- 建图时 `slam_toolbox` 发布 `map -> odom`。
- 导航时 AMCL 发布 `map -> odom`。
- `odom_source:=bpx` 时，`bpx_driver` 发布 `odom -> base_link`。
- `odom_source:=rf2o` 时，`rf2o_laser_odometry` 发布 `odom -> base_link`。

## 保存地图

```bash
ros2 run nav2_map_server map_saver_cli -f ~/nav_ros/src/slam_toolbox_nav2/maps/new_map
```
