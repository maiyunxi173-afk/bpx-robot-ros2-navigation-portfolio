# Source Attribution

The navigation packages in this repository were extracted from the collaborative project:

- Source repository: `https://github.com/Master-Wei1201/Hrs_Web_Console.git`
- Source commit used locally: `ac603b8`
- Extracted source area: `nav_ros/src`

This portfolio keeps the navigation subsystem independent from the original web console so my BPX ROS2 driver, odometry, EKF bringup, SLAM, and Nav2 work can be reviewed as a focused project.

The source repository is a team project. Its web console, Android client, inspection-task workflow, and work authored by collaborators are not presented here as my individual work. This repository retains only the navigation material needed to review my stated subsystem contribution and adds portfolio-specific organisation and documentation.

Third-party components retained from the collaborative workspace include:

- `sllidar_ros2`: SLAMTEC/RPLIDAR ROS2 driver package.
- `rf2o_laser_odometry`: laser scan-matching odometry package.
- BPX SDK binary/header files under `src/bpx_driver/third_party/bpx_sdk`.

The `robot_localization`, Navigation2, SLAM Toolbox and Cartographer projects are runtime dependencies and are not vendored in this repository.

The top-level MIT license applies to the portfolio's original material only. Vendored third-party components retain their own notices and terms; in particular, `rf2o_laser_odometry` is GPL-3.0 and `sllidar_ros2` is BSD-licensed. The BPX SDK files remain subject to the vendor's terms.
