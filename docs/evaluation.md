# Evidence, Results and Evaluation Protocol

## Current evidence register

| Claim | Evidence in this repository | Status |
| --- | --- | --- |
| BPX SDK data and commands are exposed through ROS 2 interfaces | Driver source, launch files and package manifest | Code evidence |
| Velocity commands are gated by stand state and timeout | `cmdVelCallback`, `velTimerCallback`, sit/damping callbacks | Code evidence |
| Three odometry modes are selectable | Conditional launch paths for `bpx`, `ekf` and `rf2o` | Code/config evidence |
| TF ownership is separated by mode | `publish_odom_tf` settings and conditional node launch | Code/config evidence |
| SLAM Toolbox and Cartographer routes are configured | Launch/configuration packages and saved map artefacts | Integration evidence |
| A learning-navigation workflow was recorded in Gazebo | Edited video and raw-clip-derived timeline | Simulation video evidence only |
| BPX navigation achieves a specific success rate or localisation error | No fixed-route bags or ground truth included | **Not established** |
| The learning policy transfers to the BPX robot | No deployment evidence included | **Not established** |

## Quantitative results

No numerical BPX localisation or navigation performance claim is made in the current release. Saved maps and screen recordings are useful development evidence, but they are not substitutes for a controlled benchmark.

The RMSE change from 1.662 m to 0.313 m and P90 below 0.46 m stated in my application materials belongs to a separate IMU/GIS indoor-navigation thesis experiment. It must not be attributed to this BPX stack.

## Observed integration failures and responses

| Failure mode | Observable effect | Engineering response | Remaining limitation |
| --- | --- | --- | --- |
| Competing TF publishers | Jumping or inconsistent `odom -> base_link` | Conditional launch paths assign one owner per odometry mode | Requires runtime verification after launch-file changes |
| Frozen/unavailable SDK leg odometry | Position stops changing despite a command | Detect frozen x/y and use documented fallbacks | Command integration cannot observe slip |
| Robot not standing | Velocity command is unsafe or ignored by hardware | Gate forwarding on motion state `6` | State code remains vendor specific |
| Stale `/cmd_vel` | Robot may continue the last command | Stop and disable velocity control after timeout | Not a certified emergency-stop mechanism |
| Inconsistent LiDAR extrinsics | Map distortion or poor scan alignment | Expose transforms in launch and document calibration need | Historical configurations still require physical measurement |
| Duplicate/invalid startup IMU sample | Invalid attitude propagation | Reject invalid quaternion and capture a startup yaw reference | Noise/covariance values are not yet experimentally identified |

## Proposed reproducible benchmark

Before adding performance claims, run the following protocol for each odometry mode:

1. Fix the software commit, map, parameter files, ROS domain and robot payload.
2. Measure LiDAR/IMU extrinsics and record the measurement method.
3. Define at least two indoor routes with fixed start/goal poses and repeated trials.
4. Record `/tf`, `/tf_static`, `/odom`, `/odom/filtered`, `/scan`, `/cmd_vel`, Nav2 status and timestamps in rosbag2.
5. Use motion-capture, surveyed landmarks or another independent reference for trajectory error.
6. Report absolute trajectory error, relative pose error, navigation success rate, collision/safety-intervention count, completion time and path-length ratio.
7. Publish failed trials and parameter files alongside successful runs.

For the learning-navigation experiment, additionally report training seed, evaluation seeds, checkpoint, environment version, success/collision definitions, episode count and a classical-controller baseline.
