import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    pkg_slam_nav = get_package_share_directory('slam_toolbox_nav2')
    pkg_nav2_bringup = get_package_share_directory('nav2_bringup')

    # ---- Arguments ----
    rviz_arg = DeclareLaunchArgument('rviz', default_value='true',
                                     description='Launch RViz')
    map_file_arg = DeclareLaunchArgument(
        'map', default_value=os.path.join(pkg_slam_nav, 'maps', 'my_map.yaml'),
        description='Full path to map.yaml file')
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

    # ---- Map Server ----
    # odom->base_link TF 由 bpx_driver.launch.py 发布。
    map_server_node = Node(
        package='nav2_map_server',
        executable='map_server',
        name='map_server',
        parameters=[os.path.join(pkg_slam_nav, 'config', 'nav2_params.yaml'),
                    {'yaml_filename': LaunchConfiguration('map')}],
        output='screen',
    )

    # ---- AMCL (localization) ----
    amcl_node = Node(
        package='nav2_amcl',
        executable='amcl',
        name='amcl',
        parameters=[os.path.join(pkg_slam_nav, 'config', 'nav2_params.yaml')],
        output='screen',
    )

    # ---- Lifecycle Manager: map_server + AMCL ----
    lifecycle_localization = Node(
        package='nav2_lifecycle_manager',
        executable='lifecycle_manager',
        name='lifecycle_manager_localization',
        parameters=[{
            'use_sim_time': False,
            'autostart': True,
            'node_names': ['map_server', 'amcl'],
        }],
        output='screen',
    )

    # ---- Nav2 Bringup (planner, controller, smoother, BT, waypoints, behaviors) ----
    nav2_bringup = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_nav2_bringup, 'launch', 'navigation_launch.py')),
        launch_arguments={
            'params_file': os.path.join(pkg_slam_nav, 'config', 'nav2_params.yaml'),
            'use_sim_time': 'false',
            'autostart': 'true',
        }.items(),
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
        map_file_arg,
        lidar_port_arg,
        lidar_baud_arg,
        lidar_frame_arg,
        rplidar_node,
        static_tf_node,
        map_server_node,
        amcl_node,
        lifecycle_localization,
        nav2_bringup,
        rviz_node,
    ])
