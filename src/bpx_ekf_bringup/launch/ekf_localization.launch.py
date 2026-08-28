import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    pkg_bpx_driver = get_package_share_directory('bpx_driver')
    pkg_ekf = get_package_share_directory('bpx_ekf_bringup')

    ekf_config = os.path.join(pkg_ekf, 'config', 'ekf.yaml')

    robot_ip_arg = DeclareLaunchArgument(
        'robot_ip',
        default_value='10.21.20.1',
        description='BPX robot IP address')

    bpx_driver = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_bpx_driver, 'launch', 'bpx_driver.launch.py')),
        launch_arguments={
            'robot_ip': LaunchConfiguration('robot_ip'),
            'publish_odom_tf': 'false',
            'use_ekf_odom': 'false',
        }.items(),
    )

    ekf_node = Node(
        package='robot_localization',
        executable='ekf_node',
        name='ekf_filter_node',
        output='screen',
        parameters=[ekf_config],
        remappings=[
            ('/odometry/filtered', '/odom/filtered'),
        ],
    )

    return LaunchDescription([
        robot_ip_arg,
        bpx_driver,
        ekf_node,
    ])
