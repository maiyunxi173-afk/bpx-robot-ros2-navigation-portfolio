# cartographer_nav2

BPX 机器狗的 `Cartographer + Nav2` 方案。

## 功能

- `mode:=mapping`：启动 BPX 驱动、RPLIDAR、Cartographer 建图和 occupancy grid。
- `mode:=localization`：加载 `.pbstream`，使用 Cartographer 纯定位。
- `mode:=nav`：加载 `.pbstream` 做定位，同时启动 Nav2 执行导航。
- `odom_source:=bpx`：默认使用 `bpx_driver` 发布的 `/odom` 与 `odom -> base_link`。
- `odom_source:=rf2o`：使用 `rf2o_laser_odometry` 根据 `/scan` 发布 `/odom` 与 `odom -> base_link`。

## 启动

建图：

```bash
ros2 launch cartographer_nav2 bringup.launch.py mode:=mapping
```

纯定位：

```bash
ros2 launch cartographer_nav2 bringup.launch.py mode:=localization \
  pbstream:=/path/to/map.pbstream
```

导航：

```bash
ros2 launch cartographer_nav2 bringup.launch.py mode:=nav \
  pbstream:=/path/to/map.pbstream \
  map:=/path/to/map.yaml
```

改用 rf2o 里程计：

```bash
ros2 launch cartographer_nav2 bringup.launch.py mode:=nav odom_source:=rf2o
```

## 地图格式

- Cartographer 定位需要 `.pbstream`。
- Nav2 代价地图和全局规划使用 `.yaml + .pgm`。
- 当前默认使用 `maps/map5291.pbstream` 和 `maps/map5291.yaml`。

## TF

```text
map -> odom -> base_link -> laser
```

- Cartographer 发布 `map -> odom`。
- `odom_source:=bpx` 时，`bpx_driver` 发布 `odom -> base_link`。
- `odom_source:=rf2o` 时，`rf2o_laser_odometry` 发布 `odom -> base_link`。
