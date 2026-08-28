import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    pkg_cartographer_nav2 = get_package_share_directory('cartographer_nav2')
    pkg_nav2_bringup = get_package_share_directory('nav2_bringup')

    robot_ip_arg = DeclareLaunchArgument(
        'robot_ip', default_value='10.21.20.1',
        description='BPX robot IP address')
    lidar_port_arg = DeclareLaunchArgument(
        'lidar_port', default_value='/dev/ttyUSB0',
        description='RPLIDAR serial port')
    lidar_baud_arg = DeclareLaunchArgument(
        'lidar_baud', default_value='460800',
        description='RPLIDAR C1 baud rate')
    pbstream_arg = DeclareLaunchArgument(
        'pbstream',
        default_value=os.path.join(pkg_cartographer_nav2, 'maps', 'map5291.pbstream'),
        description='Full path to Cartographer .pbstream state file')
    map_arg = DeclareLaunchArgument(
        'map',
        default_value=os.path.join(pkg_cartographer_nav2, 'maps', 'map5291.yaml'),
        description='Full path to Nav2 occupancy grid map.yaml')
    rviz_arg = DeclareLaunchArgument(
        'rviz', default_value='true',
        description='Launch RViz')
    odom_source_arg = DeclareLaunchArgument(
        'odom_source', default_value='bpx',
        description='Odometry source: "bpx", "ekf", or "rf2o"')

    cartographer_localization = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_cartographer_nav2, 'launch', 'cartographer_localization.launch.py')),
        launch_arguments={
            'robot_ip': LaunchConfiguration('robot_ip'),
            'lidar_port': LaunchConfiguration('lidar_port'),
            'lidar_baud': LaunchConfiguration('lidar_baud'),
            'pbstream': LaunchConfiguration('pbstream'),
            'odom_source': LaunchConfiguration('odom_source'),
            'rviz': 'false',
        }.items(),
    )

    map_server_node = Node(
        package='nav2_map_server',
        executable='map_server',
        name='map_server',
        parameters=[
            os.path.join(pkg_cartographer_nav2, 'config', 'nav2_params.yaml'),
            {'yaml_filename': LaunchConfiguration('map')},
        ],
        output='screen',
    )

    lifecycle_map_server = Node(
        package='nav2_lifecycle_manager',
        executable='lifecycle_manager',
        name='lifecycle_manager_map_server',
        parameters=[{
            'use_sim_time': False,
            'autostart': True,
            'node_names': ['map_server'],
        }],
        output='screen',
    )

    nav2_bringup = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_nav2_bringup, 'launch', 'navigation_launch.py')),
        launch_arguments={
            'params_file': os.path.join(pkg_cartographer_nav2, 'config', 'nav2_params.yaml'),
            'use_sim_time': 'false',
            'autostart': 'true',
        }.items(),
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
        pbstream_arg,
        map_arg,
        rviz_arg,
        odom_source_arg,
        cartographer_localization,
        map_server_node,
        lifecycle_map_server,
        nav2_bringup,
        rviz_node,
    ])
