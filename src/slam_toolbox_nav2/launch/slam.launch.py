import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    pkg_slam_nav = get_package_share_directory('slam_toolbox_nav2')

    # ---- Arguments ----
    rviz_arg = DeclareLaunchArgument('rviz', default_value='false',
                                     description='Launch RViz')
    lidar_port_arg = DeclareLaunchArgument(
        'lidar_port', default_value='/dev/ttyUSB0',
        description='RPLIDAR serial port')
    lidar_baud_arg = DeclareLaunchArgument(
        'lidar_baud', default_value='460800',
        description='RPLIDAR baud rate (460800 for C1)')
    lidar_frame_arg = DeclareLaunchArgument(
        'lidar_frame', default_value='laser',
        description='Laser frame ID')

    # ---- RPLIDAR Node (sllidar_ros2 for C1) ----
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

    # ---- slam_toolbox (async online SLAM) ----
    # odom->base_link TF 由 bpx_driver.launch.py 发布。
    slam_node = Node(
        package='slam_toolbox',
        executable='async_slam_toolbox_node',
        name='slam_toolbox',
        parameters=[os.path.join(pkg_slam_nav, 'config', 'slam_toolbox_mapping.yaml')],
        output='screen',
    )

    # ---- Static TF: base_link -> laser ----
    static_tf_node = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='base_to_laser_tf',
        arguments=['0.2', '0.0', '0.25', '0.0', '0.0', '0.0',
                   'base_link', LaunchConfiguration('lidar_frame')],
    )

    # ---- RViz ----
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', os.path.join(pkg_slam_nav, 'rviz', 'navigation.rviz')],
        condition=IfCondition(LaunchConfiguration('rviz')),
    )

    return LaunchDescription([
        rviz_arg,
        lidar_port_arg,
        lidar_baud_arg,
        lidar_frame_arg,
        rplidar_node,
        static_tf_node,
        slam_node,
        rviz_node,
    ])
