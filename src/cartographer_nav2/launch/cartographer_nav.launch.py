import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import EqualsSubstitution, LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    pkg_cartographer_nav2 = get_package_share_directory('cartographer_nav2')
    pkg_bpx_driver = get_package_share_directory('bpx_driver')
    pkg_bpx_ekf = get_package_share_directory('bpx_ekf_bringup')
    cartographer_config_dir = os.path.join(pkg_cartographer_nav2, 'config')
    cartographer_config_basename = 'my_cartographer_2d.lua'

    robot_ip_arg = DeclareLaunchArgument(
        'robot_ip', default_value='10.21.20.1',
        description='BPX robot IP address')
    lidar_port_arg = DeclareLaunchArgument(
        'lidar_port', default_value='/dev/ttyUSB0',
        description='RPLIDAR serial port')
    lidar_baud_arg = DeclareLaunchArgument(
        'lidar_baud', default_value='460800',
        description='RPLIDAR C1 baud rate')
    lidar_frame_arg = DeclareLaunchArgument(
        'lidar_frame', default_value='laser',
        description='Laser frame ID')
    rviz_arg = DeclareLaunchArgument(
        'rviz', default_value='false',
        description='Launch RViz')
    odom_source_arg = DeclareLaunchArgument(
        'odom_source', default_value='bpx',
        description='Odometry source: "bpx", "ekf", or "rf2o"')

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

    rplidar_node = Node(
        package='sllidar_ros2',
        executable='sllidar_node',
        name='sllidar_node',
        parameters=[{
            'channel_type': 'serial',
            'serial_port': LaunchConfiguration('lidar_port'),
            'serial_baudrate': LaunchConfiguration('lidar_baud'),
            'frame_id': LaunchConfiguration('lidar_frame'),
            'inverted': False,
            'angle_compensate': True,
            'scan_mode': 'Standard',
        }],
        output='screen',
    )

    cartographer_node = Node(
        package='cartographer_ros',
        executable='cartographer_node',
        name='cartographer_node',
        output='screen',
        arguments=[
            '-configuration_directory', cartographer_config_dir,
            '-configuration_basename', cartographer_config_basename
        ],
        remappings=[
            ('scan', '/scan'),
            ('odom', '/odom'),
        ],
    )

    occupancy_grid_node = Node(
        package='cartographer_ros',
        executable='cartographer_occupancy_grid_node',
        name='cartographer_occupancy_grid_node',
        output='screen',
        arguments=[
            '-resolution', '0.05',
            '-publish_period_sec', '1.0'
        ],
    )

    static_tf_node = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='static_tf_base_to_laser',
        arguments=['0.0', '0.0', '0.2', '0.0', '0.0', '0.0',
                   'base_link', LaunchConfiguration('lidar_frame')],
    )

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        condition=IfCondition(LaunchConfiguration('rviz')),
    )

    return LaunchDescription([
        robot_ip_arg,
        lidar_port_arg,
        lidar_baud_arg,
        lidar_frame_arg,
        rviz_arg,
        odom_source_arg,
        bpx_driver_with_odom,
        bpx_driver_control_only,
        bpx_ekf_bringup,
        rplidar_node,
        rf2o_odom_node,
        static_tf_node,
        cartographer_node,
        occupancy_grid_node,
        rviz_node,
    ])
