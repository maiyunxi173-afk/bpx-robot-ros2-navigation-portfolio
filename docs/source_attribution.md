# Source Attribution

The navigation packages in this repository were extracted from the collaborative project:

- Source repository: `https://github.com/Maker1201/Hrs_Web_Console.git`
- Source commit used locally: `ac603b8`
- Extracted source area: `nav_ros/src`

This portfolio keeps the navigation subsystem independent from the original web console so my BPX ROS2 driver, odometry, EKF bringup, SLAM, and Nav2 work can be reviewed as a focused project.

Third-party components retained from the collaborative workspace include:

- `sllidar_ros2`: SLAMTEC/RPLIDAR ROS2 driver package.
- `rf2o_laser_odometry`: laser scan-matching odometry package.
- BPX SDK binary/header files under `src/bpx_driver/third_party/bpx_sdk`.
