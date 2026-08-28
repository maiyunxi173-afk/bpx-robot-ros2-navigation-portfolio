import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import EqualsSubstitution, LaunchConfiguration


def generate_launch_description():
    pkg_cartographer_nav2 = get_package_share_directory('cartographer_nav2')

    mode_arg = DeclareLaunchArgument(
        'mode', default_value='mapping',
        description='Mode: "mapping", "localization", or "nav"')
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

    mapping_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_cartographer_nav2, 'launch', 'cartographer_nav.launch.py')),
        launch_arguments={
            'robot_ip': LaunchConfiguration('robot_ip'),
            'lidar_port': LaunchConfiguration('lidar_port'),
            'lidar_baud': LaunchConfiguration('lidar_baud'),
            'odom_source': LaunchConfiguration('odom_source'),
            'rviz': LaunchConfiguration('rviz'),
        }.items(),
        condition=IfCondition(
            EqualsSubstitution(LaunchConfiguration('mode'), 'mapping')),
    )

    localization_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_cartographer_nav2, 'launch', 'cartographer_localization.launch.py')),
        launch_arguments={
            'robot_ip': LaunchConfiguration('robot_ip'),
            'lidar_port': LaunchConfiguration('lidar_port'),
            'lidar_baud': LaunchConfiguration('lidar_baud'),
            'pbstream': LaunchConfiguration('pbstream'),
            'odom_source': LaunchConfiguration('odom_source'),
            'rviz': LaunchConfiguration('rviz'),
        }.items(),
        condition=IfCondition(
            EqualsSubstitution(LaunchConfiguration('mode'), 'localization')),
    )

    navigation_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_cartographer_nav2, 'launch', 'navigation.launch.py')),
        launch_arguments={
            'robot_ip': LaunchConfiguration('robot_ip'),
            'lidar_port': LaunchConfiguration('lidar_port'),
            'lidar_baud': LaunchConfiguration('lidar_baud'),
            'pbstream': LaunchConfiguration('pbstream'),
            'map': LaunchConfiguration('map'),
            'odom_source': LaunchConfiguration('odom_source'),
            'rviz': LaunchConfiguration('rviz'),
        }.items(),
        condition=IfCondition(
            EqualsSubstitution(LaunchConfiguration('mode'), 'nav')),
    )

    return LaunchDescription([
        mode_arg,
        robot_ip_arg,
        lidar_port_arg,
        lidar_baud_arg,
        pbstream_arg,
        map_arg,
        rviz_arg,
        odom_source_arg,
        mapping_launch,
        localization_launch,
        navigation_launch,
    ])
