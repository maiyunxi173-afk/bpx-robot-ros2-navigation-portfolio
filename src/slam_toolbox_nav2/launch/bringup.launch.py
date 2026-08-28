import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import EqualsSubstitution, LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    pkg_bpx_driver = get_package_share_directory('bpx_driver')
    pkg_bpx_ekf = get_package_share_directory('bpx_ekf_bringup')
    pkg_slam_nav = get_package_share_directory('slam_toolbox_nav2')

    # ---- Arguments ----
    mode_arg = DeclareLaunchArgument(
        'mode', default_value='slam',
        description='Navigation mode: "slam" or "nav"')

    robot_ip_arg = DeclareLaunchArgument(
        'robot_ip', default_value='10.21.20.1',
        description='BPX robot IP address')

    rviz_arg = DeclareLaunchArgument(
        'rviz', default_value='true',
        description='Launch RViz')

    map_arg = DeclareLaunchArgument(
        'map', default_value=os.path.join(pkg_slam_nav, 'maps', 'my_map.yaml'),
        description='Full path to the navigation map YAML file')

    lidar_port_arg = DeclareLaunchArgument(
        'lidar_port', default_value='/dev/ttyUSB0',
        description='RPLIDAR serial port')

    lidar_baud_arg = DeclareLaunchArgument(
        'lidar_baud', default_value='460800',
        description='RPLIDAR baud rate (460800 for C1)')

    odom_source_arg = DeclareLaunchArgument(
        'odom_source', default_value='bpx',
        description='Odometry source: "bpx", "ekf", or "rf2o"')

    # ---- BPX driver, using its own odometry ----
    bpx_driver_with_odom = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_bpx_driver, 'launch', 'bpx_driver.launch.py')),
        launch_arguments={
            'robot_ip': LaunchConfiguration('robot_ip'),
            'publish_odom_tf': 'true',
            'use_ekf_odom': 'false',
        }.items(),
        condition=IfCondition(
            EqualsSubstitution(LaunchConfiguration('odom_source'), 'bpx')),
    )

    # ---- BPX driver, control/status only; rf2o provides /odom and TF ----
    bpx_driver_control_only = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_bpx_driver, 'launch', 'bpx_driver.launch.py')),
        launch_arguments={
            'robot_ip': LaunchConfiguration('robot_ip'),
            'publish_odom_tf': 'false',
            'use_ekf_odom': 'true',
        }.items(),
        condition=IfCondition(
            EqualsSubstitution(LaunchConfiguration('odom_source'), 'rf2o')),
    )

    bpx_ekf_bringup = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_bpx_ekf, 'launch', 'ekf_localization.launch.py')),
        launch_arguments={
            'robot_ip': LaunchConfiguration('robot_ip'),
        }.items(),
        condition=IfCondition(
            EqualsSubstitution(LaunchConfiguration('odom_source'), 'ekf')),
    )

    rf2o_odom_node = Node(
        package='rf2o_laser_odometry',
        executable='rf2o_laser_odometry_node',
        name='rf2o_laser_odometry',
        output='screen',
        parameters=[{
            'laser_scan_topic': '/scan',
            'odom_topic': '/odom',
            'publish_tf': True,
            'base_frame_id': 'base_link',
            'odom_frame_id': 'odom',
            'init_pose_from_topic': '',
            'freq': 20.0,
        }],
        condition=IfCondition(
            EqualsSubstitution(LaunchConfiguration('odom_source'), 'rf2o')),
    )

    # ---- SLAM mode ----
    slam_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_slam_nav, 'launch', 'slam.launch.py')),
        launch_arguments={
            'rviz': LaunchConfiguration('rviz'),
            'lidar_port': LaunchConfiguration('lidar_port'),
            'lidar_baud': LaunchConfiguration('lidar_baud'),
        }.items(),
        condition=IfCondition(
            EqualsSubstitution(LaunchConfiguration('mode'), 'slam')),
    )

    # ---- Navigation mode ----
    nav_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_slam_nav, 'launch', 'navigation.launch.py')),
        launch_arguments={
            'rviz': LaunchConfiguration('rviz'),
            'map': LaunchConfiguration('map'),
            'lidar_port': LaunchConfiguration('lidar_port'),
            'lidar_baud': LaunchConfiguration('lidar_baud'),
        }.items(),
        condition=IfCondition(
            EqualsSubstitution(LaunchConfiguration('mode'), 'nav')),
    )

    return LaunchDescription([
        mode_arg,
        robot_ip_arg,
        rviz_arg,
        map_arg,
        lidar_port_arg,
        lidar_baud_arg,
        odom_source_arg,
        bpx_driver_with_odom,
        bpx_driver_control_only,
        bpx_ekf_bringup,
        rf2o_odom_node,
        slam_launch,
        nav_launch,
    ])
