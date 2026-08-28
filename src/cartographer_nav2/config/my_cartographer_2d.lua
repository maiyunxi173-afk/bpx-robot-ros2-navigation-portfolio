include "map_builder.lua"
include "trajectory_builder.lua"

options = {
  map_builder = MAP_BUILDER,
  trajectory_builder = TRAJECTORY_BUILDER,
  
  -- 坐标系配置
  map_frame = "map",
  tracking_frame = "base_link",       
  published_frame = "odom",           
  odom_frame = "odom",                
  
  -- 运行模式
  provide_odom_frame = false,         
  publish_frame_projected_to_2d = true,
  
  -- 传感器利用
  use_odometry = true,
  use_nav_sat = false,
  use_landmarks = false,
  
  num_laser_scans = 1,                
  num_multi_echo_laser_scans = 0,     
  num_subdivisions_per_laser_scan = 1,
  num_point_clouds = 0,
  
  lookup_transform_timeout_sec = 0.2,
  submap_publish_period_sec = 0.3,
  pose_publish_period_sec = 5e-3,
  trajectory_publish_period_sec = 30e-3,

  -- =====================================
  -- 采样率参数 (Humble 版本强制要求)
  rangefinder_sampling_ratio = 1.0,
  -- EKF 以 100Hz 发布里程计，只采样 20% 降低偶发跳变帧对建图的影响
  odometry_sampling_ratio = 0.2,
  fixed_frame_pose_sampling_ratio = 1.0,
  imu_sampling_ratio = 1.0,
  landmarks_sampling_ratio = 1.0,
  -- =====================================
}

MAP_BUILDER.use_trajectory_builder_2d = true

-- 轨迹构建器核心参数
TRAJECTORY_BUILDER_2D.use_imu_data = false
-- min_range 调大：避免激光扫到机器人自身的腿/底盘（<0.3m 内的近距返回几乎都是自体干扰）
TRAJECTORY_BUILDER_2D.min_range = 0.3
TRAJECTORY_BUILDER_2D.max_range = 12.0
-- 无回波射线长度：远小于 max_range，消除地图上的放射状伪影
TRAJECTORY_BUILDER_2D.missing_data_ray_length = 1.0

-- 体素滤波：0.05m 体素合并近邻点，平滑墙壁轮廓、去除地面/天花板散点
TRAJECTORY_BUILDER_2D.voxel_filter_size = 0.05

-- 子图帧数：90帧建一张子图，积累更多激光帧取平均，抑制倾斜步态引起的点云抖动
TRAJECTORY_BUILDER_2D.submaps.num_range_data = 90

-- 扫描匹配：开启在线 CSM 提升旋转/平移精度
TRAJECTORY_BUILDER_2D.use_online_correlative_scan_matching = true
TRAJECTORY_BUILDER_2D.real_time_correlative_scan_matcher.linear_search_window = 0.1
TRAJECTORY_BUILDER_2D.real_time_correlative_scan_matcher.angular_search_window = math.rad(20.)

-- 运动过滤：仅当位移>2cm 或转角>0.1° 时插入新帧，减少静止时的重复帧
TRAJECTORY_BUILDER_2D.motion_filter.max_distance_meters = 0.02
TRAJECTORY_BUILDER_2D.motion_filter.max_angle_radians = math.rad(0.1)
TRAJECTORY_BUILDER_2D.motion_filter.max_time_seconds = 5.

-- Ceres 扫描匹配权重：加大旋转权重抑制偏航漂移
TRAJECTORY_BUILDER_2D.ceres_scan_matcher.rotation_weight = 40.0

-- 全局位姿图优化
POSE_GRAPH.optimize_every_n_nodes = 35
-- 回环检测阈值调高：防止幽灵子图被错误合并成假回环
POSE_GRAPH.constraint_builder.min_score = 0.65
POSE_GRAPH.constraint_builder.global_localization_min_score = 0.70
-- 减少搜索窗口：只在合理范围内找回环约束，避免跨越幽灵位置的假匹配
POSE_GRAPH.constraint_builder.max_constraint_distance = 15.0

return options