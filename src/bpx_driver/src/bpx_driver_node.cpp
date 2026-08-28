#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <std_msgs/msg/u_int8.hpp>
#include <std_msgs/msg/float32.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_ros/transform_broadcaster.h>
#include <geometry_msgs/msg/transform_stamped.hpp>

#include <motion_level_control.h>

#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

using namespace std::chrono_literals;

class BpxDriverNode : public rclcpp::Node {
public:
    BpxDriverNode() : Node("bpx_driver_node") {
        // Declare parameters
        declare_parameter("robot_ip", std::string(bpx_sdk::DEFAULT_SERVER_IP));
        declare_parameter("state_upload_rate_hz", 100);
        declare_parameter("motion_command_rate_hz", 50);
        declare_parameter("state_publish_rate_hz", 50);
        declare_parameter("cmd_vel_timeout_sec", 0.5);
        declare_parameter("odom_frame", std::string("odom"));
        declare_parameter("base_frame", std::string("base_link"));
        declare_parameter("imu_frame", std::string("imu_link"));
        declare_parameter("publish_odom_tf", true);
        declare_parameter("use_ekf_odom", true);
        declare_parameter("prefer_leg_odom", true);
        declare_parameter("zero_yaw_on_start", true);

        // Get parameters
        robot_ip_ = get_parameter("robot_ip").as_string();
        int state_rate = get_parameter("state_upload_rate_hz").as_int();
        int cmd_rate = get_parameter("motion_command_rate_hz").as_int();
        int pub_rate = get_parameter("state_publish_rate_hz").as_int();
        cmd_vel_timeout_ = std::chrono::duration<double>(
            get_parameter("cmd_vel_timeout_sec").as_double());
        odom_frame_ = get_parameter("odom_frame").as_string();
        base_frame_ = get_parameter("base_frame").as_string();
        imu_frame_ = get_parameter("imu_frame").as_string();
        publish_odom_tf_ = get_parameter("publish_odom_tf").as_bool();
        use_ekf_odom_ = get_parameter("use_ekf_odom").as_bool();
        prefer_leg_odom_ = get_parameter("prefer_leg_odom").as_bool();
        zero_yaw_on_start_ = get_parameter("zero_yaw_on_start").as_bool();

        // Initialize SDK
        motion_.setRobotIp(robot_ip_.c_str());
        motion_.setRobotStateUploadPort(bpx_sdk::DEFAULT_CLIENT_ROBOT_STATE_UDP_PORT);
        motion_.setTcpLocalPort(0);
        motion_.setRobotStateUploadRate(static_cast<uint16_t>(state_rate));
        motion_.setMotionCommandRate(static_cast<uint16_t>(cmd_rate));

        if (!motion_.connect()) {
            RCLCPP_ERROR(get_logger(), "Failed to connect to robot at %s — driver running without robot data", robot_ip_.c_str());
        } else {
            RCLCPP_INFO(get_logger(), "Connected to BPX robot at %s", robot_ip_.c_str());
        }

        // Publishers
        joint_state_pub_ = create_publisher<sensor_msgs::msg::JointState>("joint_states", 10);
        imu_pub_ = create_publisher<sensor_msgs::msg::Imu>("imu/data", 10);
        odom_pub_ = create_publisher<nav_msgs::msg::Odometry>("odom", 10);
        battery_pub_ = create_publisher<std_msgs::msg::UInt8>("bpx/battery", 10);
        battery_current_pub_ = create_publisher<std_msgs::msg::Float32>("bpx/battery_current", 10);
        motion_state_pub_ = create_publisher<std_msgs::msg::UInt8>("bpx/motion_state", 10);
        gait_pub_ = create_publisher<std_msgs::msg::UInt8>("bpx/gait", 10);

        // EKF raw data publishers
        imu_raw_pub_ = create_publisher<sensor_msgs::msg::Imu>("bpx/imu/data_raw", 10);
        joint_state_raw_pub_ = create_publisher<sensor_msgs::msg::JointState>("bpx/joint_states", 10);
        joint_torque_pub_ = create_publisher<std_msgs::msg::Float64MultiArray>("bpx/joint_torques", 10);

        // TF broadcaster
        tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

        // Subscriber
        cmd_vel_sub_ = create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel", 10,
            std::bind(&BpxDriverNode::cmdVelCallback, this, std::placeholders::_1));

        // Services
        stand_up_srv_ = create_service<std_srvs::srv::Trigger>(
            "bpx/stand_up",
            std::bind(&BpxDriverNode::standUpCallback, this,
                      std::placeholders::_1, std::placeholders::_2));
        sit_down_srv_ = create_service<std_srvs::srv::Trigger>(
            "bpx/sit_down",
            std::bind(&BpxDriverNode::sitDownCallback, this,
                      std::placeholders::_1, std::placeholders::_2));
        damping_srv_ = create_service<std_srvs::srv::Trigger>(
            "bpx/damping",
            std::bind(&BpxDriverNode::dampingCallback, this,
                      std::placeholders::_1, std::placeholders::_2));

        // State publish timer
        auto pub_period = std::chrono::milliseconds(1000 / pub_rate);
        state_timer_ = create_wall_timer(
            pub_period, std::bind(&BpxDriverNode::stateTimerCallback, this));

        // Velocity command timer (send at motion command rate)
        auto cmd_period = std::chrono::milliseconds(1000 / cmd_rate);
        vel_timer_ = create_wall_timer(
            cmd_period, std::bind(&BpxDriverNode::velTimerCallback, this));

        RCLCPP_INFO(get_logger(), "BPX driver node started");
        if (use_ekf_odom_) {
            RCLCPP_INFO(get_logger(), "EKF odometry enabled - publishing raw data for EKF fusion");
        }
    }

    ~BpxDriverNode() override {
        motion_.setVelocityControlFlag(false);
        motion_.disconnect();
    }

private:
    // ---------- Callbacks ----------

    void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(vel_mutex_);
        last_cmd_vel_ = *msg;
        last_cmd_vel_time_ = now();
        velocity_active_ = true;
        motion_.setVelocityControlFlag(true);
    }

    void velTimerCallback() {
        std::lock_guard<std::mutex> lock(vel_mutex_);
        if (!velocity_active_) return;

        // Check timeout
        if ((now() - last_cmd_vel_time_) > cmd_vel_timeout_) {
            // Timeout: send zero velocity and disable velocity control
            motion_.setVelocity(0.0f, 0.0f, 0.0f);
            motion_.setVelocityControlFlag(false);
            velocity_active_ = false;
            RCLCPP_WARN(get_logger(), "cmd_vel timeout, stopping robot");
            return;
        }

        // Robot must be in stand-up state (6) to accept velocity commands
        uint8_t motion_state;
        if (motion_.getCurrentMotionState(&motion_state)) {
            if (motion_state != 6) {
                RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 3000,
                    "Robot motion state is %d, not 6 (stand up). "
                    "Velocity commands will be ignored. Call /bpx/stand_up service first.",
                    motion_state);
                return;
            }
        }

        float vx = static_cast<float>(last_cmd_vel_.linear.x);
        float vy = static_cast<float>(last_cmd_vel_.linear.y);
        float wz = static_cast<float>(last_cmd_vel_.angular.z);
        motion_.setVelocity(vx, vy, wz);
    }

    void stateTimerCallback() {
        publishJointStates();
        publishImu();
        publishBattery();
        publishMotionState();

        // Publish EKF raw data if enabled
        if (use_ekf_odom_) {
            publishEKFRawData();
        } else {
            publishOdometry();
        }
    }

    // ---------- Publish helpers ----------

    void publishJointStates() {
        float pos[12], vel[12], tau[12];
        bool has_pos = motion_.getJointPosition(pos);
        bool has_vel = motion_.getJointVelocity(vel);
        bool has_tau = motion_.getJointTorque(tau);

        if (!has_pos) return;

        auto msg = sensor_msgs::msg::JointState();
        msg.header.stamp = now();
        msg.header.frame_id = "";

        msg.name = {
            "fl_hip_roll_joint", "fl_hip_pitch_joint", "fl_knee_joint",
            "fr_hip_roll_joint", "fr_hip_pitch_joint", "fr_knee_joint",
            "hl_hip_roll_joint", "hl_hip_pitch_joint", "hl_knee_joint",
            "hr_hip_roll_joint", "hr_hip_pitch_joint", "hr_knee_joint",
        };

        msg.position.assign(pos, pos + 12);
        if (has_vel) msg.velocity.assign(vel, vel + 12);
        if (has_tau) msg.effort.assign(tau, tau + 12);

        joint_state_pub_->publish(msg);

        // Store for EKF
        std::copy(pos, pos + 12, joint_pos_);
        std::copy(vel, vel + 12, joint_vel_);
        std::copy(tau, tau + 12, joint_tau_);
    }

    // Correct the raw IMU sample into the imu_link frame.
    //
    // The BPX SDK reports the orientation quaternion in (x, y, z, w) order
    // (see request_robot_state.h). The accelerometer already reads gravity on
    // +Z with roll/pitch ~0 when the robot is level, i.e. the sensor axes are
    // already aligned with imu_link (REP-103: x-fwd, y-left, z-up), so NO axis
    // remapping / compensation rotation is applied to acc/omega.
    //
    // Optionally the initial heading (yaw) is captured on the first valid
    // sample and subtracted, so a static robot at startup reports an (almost)
    // identity orientation while roll/pitch stay gravity-referenced (absolute).
    void transformImuData(const float raw_quat[4], const float raw_acc[3], const float raw_omega[3],
                          geometry_msgs::msg::Quaternion& out_q,
                          geometry_msgs::msg::Vector3& out_acc,
                          geometry_msgs::msg::Vector3& out_omega)
    {
        // SDK quaternion order is (x, y, z, w). Guard against invalid
        // (all-zero / NaN) samples that the SDK can emit right after connect,
        // before real IMU data has arrived.
        double qn = std::sqrt(
            raw_quat[0] * raw_quat[0] + raw_quat[1] * raw_quat[1] +
            raw_quat[2] * raw_quat[2] + raw_quat[3] * raw_quat[3]);

        tf2::Quaternion q_corrected(0.0, 0.0, 0.0, 1.0);  // identity fallback
        if (std::isfinite(qn) && qn > 1e-3) {
            tf2::Quaternion q_raw(raw_quat[0], raw_quat[1], raw_quat[2], raw_quat[3]);
            q_raw.normalize();

            // Capture the startup yaw as a reference so heading starts at 0.
            if (zero_yaw_on_start_ && !yaw_ref_set_) {
                double yaw = std::atan2(
                    2.0 * (q_raw.w() * q_raw.z() + q_raw.x() * q_raw.y()),
                    1.0 - 2.0 * (q_raw.y() * q_raw.y() + q_raw.z() * q_raw.z()));
                tf2::Quaternion q_ref;
                q_ref.setRPY(0.0, 0.0, yaw);   // yaw-only reference
                yaw_ref_inv_ = q_ref.inverse();
                yaw_ref_set_ = true;
                RCLCPP_INFO(get_logger(),
                    "IMU startup yaw reference captured: %.2f deg (zeroed)",
                    yaw * 180.0 / M_PI);
            }

            q_corrected =
                (zero_yaw_on_start_ && yaw_ref_set_) ? (yaw_ref_inv_ * q_raw) : q_raw;
            q_corrected.normalize();
        }

        out_q.x = q_corrected.x();
        out_q.y = q_corrected.y();
        out_q.z = q_corrected.z();
        out_q.w = q_corrected.w();

        // Accelerometer and gyro are already expressed in the imu_link frame;
        // pass them through unchanged (they are body-frame quantities and must
        // NOT be rotated by the yaw reference).
        out_acc.x = raw_acc[0];
        out_acc.y = raw_acc[1];
        out_acc.z = raw_acc[2];

        out_omega.x = raw_omega[0];
        out_omega.y = raw_omega[1];
        out_omega.z = raw_omega[2];
    }

    void publishImu() {
        float quat[4], acc[3], omega[3];
        bool has_quat = motion_.getImuQuat(quat);
        bool has_acc = motion_.getImuAcc(acc);
        bool has_omega = motion_.getImuOmega(omega);

        if (!has_quat && !has_acc && !has_omega) return;

        auto msg = sensor_msgs::msg::Imu();
        msg.header.stamp = now();
        msg.header.frame_id = imu_frame_; // 依旧发布给你的正规 imu_link

        // 全局转换
        geometry_msgs::msg::Quaternion q_clean;
        geometry_msgs::msg::Vector3 acc_clean;
        geometry_msgs::msg::Vector3 omega_clean;
        transformImuData(quat, acc, omega, q_clean, acc_clean, omega_clean);

        if (has_quat) {
            msg.orientation = q_clean;
            msg.orientation_covariance = {0.01, 0.0, 0.0, 0.0, 0.01, 0.0, 0.0, 0.0, 0.01};
        } else {
            msg.orientation_covariance[0] = -1.0;
        }

        if (has_omega) {
            msg.angular_velocity = omega_clean;
            msg.angular_velocity_covariance = {0.02, 0.0, 0.0, 0.0, 0.02, 0.0, 0.0, 0.0, 0.02};
        }

        if (has_acc) {
            msg.linear_acceleration = acc_clean;
            msg.linear_acceleration_covariance = {0.04, 0.0, 0.0, 0.0, 0.04, 0.0, 0.0, 0.0, 0.04};
        }

        imu_pub_->publish(msg);

        // 存下来用于 EKF 发射的数据也必须是洗干净的数据
        if (has_quat)  std::copy(quat, quat + 4, imu_quat_);
        if (has_acc)   std::copy(acc, acc + 3, imu_acc_);
        if (has_omega) std::copy(omega, omega + 3, imu_omega_);
    }

    void publishEKFRawData() {
        // 1. Publish raw IMU data for EKF
        auto imu_msg = sensor_msgs::msg::Imu();
        imu_msg.header.stamp = now();
        imu_msg.header.frame_id = imu_frame_; // 发布给正规的 imu_link
        
        // 调用转换函数，获取全套校正后的数据
        geometry_msgs::msg::Quaternion q_clean;
        geometry_msgs::msg::Vector3 acc_clean;
        geometry_msgs::msg::Vector3 omega_clean;
        transformImuData(imu_quat_, imu_acc_, imu_omega_, q_clean, acc_clean, omega_clean);

        // 完美对齐的三轴数据直接赋值
        imu_msg.orientation = q_clean;   
        imu_msg.angular_velocity = omega_clean;
        imu_msg.linear_acceleration = acc_clean;
        
        imu_raw_pub_->publish(imu_msg);

        // 2. Publish joint states for EKF (这部分逻辑完全正确，不需要变动)
        auto joint_msg = sensor_msgs::msg::JointState();
        joint_msg.header.stamp = now();
        joint_msg.header.frame_id = "";
        
        joint_msg.name = {
            "fl_hip_roll_joint", "fl_hip_pitch_joint", "fl_knee_joint",
            "fr_hip_roll_joint", "fr_hip_pitch_joint", "fr_knee_joint",
            "hl_hip_roll_joint", "hl_hip_pitch_joint", "hl_knee_joint",
            "hr_hip_roll_joint", "hr_hip_pitch_joint", "hr_knee_joint",
        };
        
        joint_msg.position.assign(joint_pos_, joint_pos_ + 12);
        joint_msg.velocity.assign(joint_vel_, joint_vel_ + 12);
        joint_msg.effort.assign(joint_tau_, joint_tau_ + 12);
        
        joint_state_raw_pub_->publish(joint_msg);

        // 3. Publish joint torques separately (这部分完全正确)
        auto torque_msg = std_msgs::msg::Float64MultiArray();
        torque_msg.data.assign(joint_tau_, joint_tau_ + 12);
        joint_torque_pub_->publish(torque_msg);
    }

    void publishOdometry() {
        float vel_body[3], quat[4], leg_odom[3];
        bool has_vel = motion_.getCurrentVelocityBody(vel_body);
        bool has_quat = motion_.getImuQuat(quat);
        bool has_leg_odom = prefer_leg_odom_ && motion_.getLegOdom(leg_odom);

        if ((!has_leg_odom && !has_vel) || !has_quat) {
            RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000,
                "Odometry data unavailable: leg_odom=%d, velocity=%d, imu_quat=%d.",
                has_leg_odom, has_vel, has_quat);
            return;
        }

        auto current_time = now();
        double dt = (last_odom_time_.nanoseconds() > 0) ? (current_time - last_odom_time_).seconds() : 0.0;
        last_odom_time_ = current_time;
        if (dt < 0.0 || dt > 1.0) dt = 0.0;

        // A. 拿到转换干净后的机器人全局四元数姿态
        geometry_msgs::msg::Quaternion q_clean;
        geometry_msgs::msg::Vector3 dummy_acc, dummy_omega;
        float dummy_a[3] = {0}, dummy_o[3] = {0};
        transformImuData(quat, dummy_a, dummy_o, q_clean, dummy_acc, dummy_omega);

        double last_pose_x = pose_x_;
        double last_pose_y = pose_y_;
        geometry_msgs::msg::Twist active_cmd_vel;
        bool has_active_cmd_vel = false;
        {
            std::lock_guard<std::mutex> lock(vel_mutex_);
            has_active_cmd_vel =
                velocity_active_ && ((current_time - last_cmd_vel_time_) <= cmd_vel_timeout_);
            active_cmd_vel = last_cmd_vel_;
        }

        if (has_leg_odom &&
            std::isfinite(leg_odom[0]) &&
            std::isfinite(leg_odom[1]) &&
            std::isfinite(leg_odom[2])) {
            if (!leg_odom_ref_set_) {
                leg_odom_ref_[0] = leg_odom[0];
                leg_odom_ref_[1] = leg_odom[1];
                leg_odom_ref_[2] = leg_odom[2];
                leg_odom_ref_set_ = true;
                RCLCPP_INFO(get_logger(),
                    "BPX leg odom reference captured: x=%.3f y=%.3f yaw=%.3f",
                    leg_odom_ref_[0], leg_odom_ref_[1], leg_odom_ref_[2]);
            }

            double leg_pose_x = leg_odom[0] - leg_odom_ref_[0];
            double leg_pose_y = leg_odom[1] - leg_odom_ref_[1];
            double leg_delta_xy = std::hypot(leg_pose_x - pose_x_, leg_pose_y - pose_y_);
            double cmd_linear = std::hypot(active_cmd_vel.linear.x, active_cmd_vel.linear.y);

            if (dt > 0.0 &&
                leg_delta_xy < 1e-5 &&
                has_active_cmd_vel &&
                cmd_linear > 1e-3) {
                leg_odom_xy_unreliable_ = true;
            }

            if (leg_odom_xy_unreliable_) {
                tf2::Quaternion q_tf(q_clean.x, q_clean.y, q_clean.z, q_clean.w);
                tf2::Vector3 v_body(active_cmd_vel.linear.x, active_cmd_vel.linear.y, 0.0);
                tf2::Vector3 v_world = tf2::quatRotate(q_tf, v_body);

                if (dt > 0.0 && has_active_cmd_vel && cmd_linear > 1e-3) {
                    pose_x_ += v_world.x() * dt;
                    pose_y_ += v_world.y() * dt;
                }

                RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 3000,
                    "BPX leg odom xy is unreliable; using commanded linear velocity for odom xy.");
            } else {
                pose_x_ = leg_pose_x;
                pose_y_ = leg_pose_y;
            }
        } else {
            // B. 使用 tf2 将机体速度（Body Frame）投影到世界坐标系（World Frame）下
            // This fallback is less reliable on robots whose SDK velocity field stays zero.
            RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000,
                "BPX leg odom unavailable; falling back to velocity integration.");
            if (!has_vel) {
                if (!has_active_cmd_vel) {
                    return;
                }

                tf2::Quaternion q_tf(q_clean.x, q_clean.y, q_clean.z, q_clean.w);
                tf2::Vector3 v_body(active_cmd_vel.linear.x, active_cmd_vel.linear.y, 0.0);
                tf2::Vector3 v_world = tf2::quatRotate(q_tf, v_body);

                pose_x_ += v_world.x() * dt;
                pose_y_ += v_world.y() * dt;

                RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 3000,
                    "BPX SDK velocity unavailable; integrating commanded velocity for odom.");
            } else {
                tf2::Quaternion q_tf(q_clean.x, q_clean.y, q_clean.z, q_clean.w);
                tf2::Vector3 v_body(vel_body[0], vel_body[1], 0.0);
                tf2::Vector3 v_world = tf2::quatRotate(q_tf, v_body);

                pose_x_ += v_world.x() * dt;
                pose_y_ += v_world.y() * dt;
            }
        }

        double derived_vx = (dt > 1e-6) ? (pose_x_ - last_pose_x) / dt : 0.0;
        double derived_vy = (dt > 1e-6) ? (pose_y_ - last_pose_y) / dt : 0.0;

        // C. 实例化 Odometry 消息（必须在赋值前实例化！）
        auto msg = nav_msgs::msg::Odometry();
        msg.header.stamp = current_time;
        msg.header.frame_id = odom_frame_;
        msg.child_frame_id = base_frame_;

        msg.pose.pose.position.x = pose_x_;
        msg.pose.pose.position.y = pose_y_;
        msg.pose.pose.position.z = 0.0;
        
        // 直接使用转换干净后的四元数
        msg.pose.pose.orientation = q_clean;

        msg.twist.twist.linear.x = has_leg_odom ? derived_vx : vel_body[0];
        msg.twist.twist.linear.y = has_leg_odom ? derived_vy : vel_body[1];
        msg.twist.twist.angular.z = has_vel ? vel_body[2] : 0.0;

        // Pose covariance (x, y, yaw dominant)
        msg.pose.covariance = {
            0.01, 0.0,  0.0, 0.0, 0.0, 0.0,
            0.0,  0.01, 0.0, 0.0, 0.0, 0.0,
            0.0,  0.0,  1e-9,0.0, 0.0, 0.0,
            0.0,  0.0,  0.0, 1e-9,0.0, 0.0,
            0.0,  0.0,  0.0, 0.0, 1e-9,0.0,
            0.0,  0.0,  0.0, 0.0, 0.0, 0.01};
        // Twist covariance
        msg.twist.covariance = {
            0.01, 0.0,  0.0, 0.0, 0.0, 0.0,
            0.0,  0.01, 0.0, 0.0, 0.0, 0.0,
            0.0,  0.0,  1e-9,0.0, 0.0, 0.0,
            0.0,  0.0,  0.0, 1e-9,0.0, 0.0,
            0.0,  0.0,  0.0, 0.0, 1e-9,0.0,
            0.0,  0.0,  0.0, 0.0, 0.0, 0.02};

        odom_pub_->publish(msg);

        if (publish_odom_tf_) {
            geometry_msgs::msg::TransformStamped tf;
            tf.header.stamp = current_time;
            tf.header.frame_id = odom_frame_;
            tf.child_frame_id = base_frame_;
            tf.transform.translation.x = pose_x_;
            tf.transform.translation.y = pose_y_;
            tf.transform.translation.z = 0.0;
            // 这里同步使用干净的四元数
            tf.transform.rotation = q_clean;
            tf_broadcaster_->sendTransform(tf);
        }

        RCLCPP_DEBUG(get_logger(),
            "BPX odom pose x=%.3f y=%.3f source=%s vel=[%.3f %.3f %.3f]",
            pose_x_, pose_y_, has_leg_odom ? "leg_odom" : "velocity",
            has_leg_odom ? derived_vx : vel_body[0],
            has_leg_odom ? derived_vy : vel_body[1],
            has_vel ? vel_body[2] : 0.0);
    }

    void publishBattery() {
        uint8_t level;
        if (motion_.getBatteryLevel(&level)) {
            auto msg = std_msgs::msg::UInt8();
            msg.data = level;
            battery_pub_->publish(msg);
        }

        float current;
        if (motion_.getBatteryCurrent(&current)) {
            auto msg = std_msgs::msg::Float32();
            msg.data = current;
            battery_current_pub_->publish(msg);
        }
    }

    void publishMotionState() {
        uint8_t state;
        if (motion_.getCurrentMotionState(&state)) {
            auto msg = std_msgs::msg::UInt8();
            msg.data = state;
            motion_state_pub_->publish(msg);
        }

        uint8_t gait;
        if (motion_.getCurrentGait(&gait)) {
            auto msg = std_msgs::msg::UInt8();
            msg.data = gait;
            gait_pub_->publish(msg);
        }
    }

    // ---------- Service callbacks ----------

    void standUpCallback(const std_srvs::srv::Trigger::Request::SharedPtr /*req*/,
                         std_srvs::srv::Trigger::Response::SharedPtr res) {
        if (motion_.setStandUp()) {
            pose_x_ = 0.0;
            pose_y_ = 0.0;
            leg_odom_ref_set_ = false;
            leg_odom_xy_unreliable_ = false;
            res->success = true;
            res->message = "Stand up command sent";
        } else {
            res->success = false;
            res->message = "Failed to send stand up command";
        }
    }

    void sitDownCallback(const std_srvs::srv::Trigger::Request::SharedPtr /*req*/,
                         std_srvs::srv::Trigger::Response::SharedPtr res) {
        {
            std::lock_guard<std::mutex> lock(vel_mutex_);
            motion_.setVelocityControlFlag(false);
            velocity_active_ = false;
        }
        if (motion_.setSitDown()) {
            res->success = true;
            res->message = "Sit down command sent";
        } else {
            res->success = false;
            res->message = "Failed to send sit down command";
        }
    }

    void dampingCallback(const std_srvs::srv::Trigger::Request::SharedPtr /*req*/,
                         std_srvs::srv::Trigger::Response::SharedPtr res) {
        {
            std::lock_guard<std::mutex> lock(vel_mutex_);
            motion_.setVelocityControlFlag(false);
            velocity_active_ = false;
        }
        if (motion_.setDamping()) {
            res->success = true;
            res->message = "Damping command sent";
        } else {
            res->success = false;
            res->message = "Failed to send damping command";
        }
    }

    // ---------- Members ----------
    bpx_sdk::MotionLevelControl motion_;

    std::string robot_ip_;
    std::string odom_frame_;
    std::string base_frame_;
    std::string imu_frame_;
    bool publish_odom_tf_ = true;
    bool use_ekf_odom_ = true;
    bool prefer_leg_odom_ = true;
    bool zero_yaw_on_start_ = true;

    // IMU startup yaw reference (so a static robot starts at identity heading)
    bool yaw_ref_set_ = false;
    tf2::Quaternion yaw_ref_inv_{0.0, 0.0, 0.0, 1.0};

    // Odometry integration
    double pose_x_ = 0.0;
    double pose_y_ = 0.0;
    rclcpp::Time last_odom_time_;
    bool leg_odom_ref_set_ = false;
    bool leg_odom_xy_unreliable_ = false;
    float leg_odom_ref_[3] = {0.0f, 0.0f, 0.0f};

    // Velocity control
    std::mutex vel_mutex_;
    geometry_msgs::msg::Twist last_cmd_vel_;
    rclcpp::Time last_cmd_vel_time_;
    bool velocity_active_ = false;
    std::chrono::duration<double> cmd_vel_timeout_;

    // Sensor data storage
    float joint_pos_[12] = {0};
    float joint_vel_[12] = {0};
    float joint_tau_[12] = {0};
    float imu_quat_[4] = {0};
    float imu_acc_[3] = {0};
    float imu_omega_[3] = {0};

    // Publishers
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_pub_;
    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
    rclcpp::Publisher<std_msgs::msg::UInt8>::SharedPtr battery_pub_;
    rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr battery_current_pub_;
    rclcpp::Publisher<std_msgs::msg::UInt8>::SharedPtr motion_state_pub_;
    rclcpp::Publisher<std_msgs::msg::UInt8>::SharedPtr gait_pub_;

    // EKF raw data publishers
    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_raw_pub_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_raw_pub_;
    rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr joint_torque_pub_;

    // Subscriber
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;

    // Services
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr stand_up_srv_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr sit_down_srv_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr damping_srv_;

    // Timers
    rclcpp::TimerBase::SharedPtr state_timer_;
    rclcpp::TimerBase::SharedPtr vel_timer_;

    // TF
    std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<BpxDriverNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
