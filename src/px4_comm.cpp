//          Copyright Emil Fresk 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// User includes
#include "px4_comm/px4_comm.hpp"
#include "mav_msgs/conversions.h"
#include <Eigen/Dense>
#include <mavros/frame_tf.h>

// Callback functions for subscribed messages
void px4_communication::callback_roll_pitch_yawrate_thrust(
    const mav_msgs::RollPitchYawrateThrustConstPtr& msg)
{
  using namespace mavros;

  auto q = ftf::quaternion_from_rpy(msg->roll, msg->pitch, current_yaw);

  mavros_msgs::AttitudeTarget target;

  target.type_mask = mavros_msgs::AttitudeTarget::IGNORE_ROLL_RATE |
                     mavros_msgs::AttitudeTarget::IGNORE_PITCH_RATE;

  target.orientation.x = q.x();
  target.orientation.y = q.y();
  target.orientation.z = q.z();
  target.orientation.w = q.w();

  target.body_rate.x = 0;
  target.body_rate.y = 0;
  target.body_rate.z = msg->yaw_rate;
    scaled_thrust = 0.030 * (msg->thrust.z);
  //target.thrust = (1/30) * (msg->thrust.z);
  target.thrust = scaled_thrust;

  target.header.stamp = ros::Time::now();
  command_pub_.publish(target);
}

void px4_communication::callback_rollrate_pitchrate_yawrate_thrust(
    const mav_msgs::RateThrustConstPtr& msg)
{
  ROS_ERROR_ONCE("Got Rate Thrust command, not implemented");

  mavros_msgs::AttitudeTarget target;

  target.type_mask = mavros_msgs::AttitudeTarget::IGNORE_ATTITUDE;

  target.orientation.x = 0;
  target.orientation.y = 0;
  target.orientation.z = 0;
  target.orientation.w = 1;

  target.body_rate.x = msg->angular_rates.x;
  target.body_rate.y = msg->angular_rates.y;
  target.body_rate.z = msg->angular_rates.z;

  target.thrust = msg->thrust.z;

  target.header.stamp = ros::Time::now();
  command_pub_.publish(target);
}

void px4_communication::callback_mavros_rc_value(
    const mavros_msgs::RCInConstPtr& msg)
{
  sensor_msgs::Joy joy;
  for (auto ch : msg->channels)
    joy.axes.push_back(ch);

  joy.header.stamp = ros::Time::now();
  rc_pub_.publish(joy);
}

void px4_communication::callback_mavros_state(
    const mavros_msgs::StateConstPtr& msg)
{
  status_msg_.header.stamp = ros::Time::now();

  if (msg->mode == "OFFBOARD")
    status_msg_.command_interface_enabled = true;
  else
    status_msg_.command_interface_enabled = false;

  if (msg->armed == true)
  {
    status_msg_.in_air = true;
    status_msg_.motor_status = mav_msgs::Status::MOTOR_STATUS_RUNNING;
  }
  else
  {
    status_msg_.in_air = false;
    status_msg_.motor_status = mav_msgs::Status::MOTOR_STATUS_STOPPED;
  }

  status_pub_.publish(status_msg_);
}

void px4_communication::callback_mavros_battery(
    const sensor_msgs::BatteryStateConstPtr& msg)
{
  status_msg_.battery_voltage = msg->voltage;
}

void px4_communication::callback_mavros_altitude(
    const mavros_msgs::AltitudeConstPtr& msg)
{
  ROS_ERROR_ONCE("Got MAVROS Altitude, not implemented (50x)");
}

void px4_communication::callback_mavros_imu(const sensor_msgs::ImuConstPtr& msg)
{
  current_yaw = mavros::ftf::quaternion_get_yaw(
      mav_msgs::quaternionFromMsg(msg->orientation));
}

px4_communication::px4_communication(ros::NodeHandle& pub_nh,
                                     ros::NodeHandle& priv_nh)
    : public_nh_(pub_nh), priv_nh_(priv_nh)
{
  //
  // Local variables
  //
  status_msg_.battery_voltage = 0;
  status_msg_.command_interface_enabled = false;
  status_msg_.cpu_load = 0;
  status_msg_.flight_time = 0;
  status_msg_.gps_num_satellites = 0;
  status_msg_.gps_status = mav_msgs::Status::GPS_STATUS_NO_LOCK;
  status_msg_.in_air = false;
  status_msg_.motor_status = mav_msgs::Status::MOTOR_STATUS_STOPPED;
  status_msg_.rc_command_mode = mav_msgs::Status::RC_COMMAND_ATTITUDE;
  status_msg_.system_uptime = 0;
  status_msg_.vehicle_name = "no type";
  status_msg_.vehicle_type = "no name";
  status_msg_.header.stamp = ros::Time::now();

  current_yaw = 0;

  //
  // UAV comm topics
  //
  ratethrust_sub_ = public_nh_.subscribe(
      mav_msgs::default_topics::COMMAND_RATE_THRUST, 5,
      &px4_communication::callback_rollrate_pitchrate_yawrate_thrust, this,
      ros::TransportHints().tcpNoDelay());

  rpyrt_sub_ = public_nh_.subscribe(
      "/shafter3/command/roll_pitch_yawrate_thrust", 5,
      &px4_communication::callback_roll_pitch_yawrate_thrust, this,
      ros::TransportHints().tcpNoDelay());

  status_pub_ = public_nh_.advertise< mav_msgs::Status >(
      mav_msgs::default_topics::STATUS, 1);

  rc_pub_ =
      public_nh_.advertise< sensor_msgs::Joy >(mav_msgs::default_topics::RC, 1);

  //
  // mavros topics
  //
  command_pub_ = public_nh_.advertise< mavros_msgs::AttitudeTarget >(
      "/shafter3/mavros/setpoint_raw/attitude", 10);

  rc_sub_ = public_nh_.subscribe("/shafter3/mavros/rc/in", 5,
                                 &px4_communication::callback_mavros_rc_value,
                                 this, ros::TransportHints().tcpNoDelay());

  state_sub_ = public_nh_.subscribe("/shafter3/mavros/state", 5,
                                    &px4_communication::callback_mavros_state,
                                    this, ros::TransportHints().tcpNoDelay());

  battery_sub_ = public_nh_.subscribe(
      "/shafter3/mavros/battery", 5, &px4_communication::callback_mavros_battery, this,
      ros::TransportHints().tcpNoDelay());

  altitude_sub_ = public_nh_.subscribe(
      "/shafter3/mavros/altitude", 5, &px4_communication::callback_mavros_altitude, this,
      ros::TransportHints().tcpNoDelay());

  imu_sub_ = public_nh_.subscribe("/shafter3/mavros/imu/data", 5,
                                  &px4_communication::callback_mavros_imu, this,
                                  ros::TransportHints().tcpNoDelay());
}

void px4_communication::ros_loop()
{
  ros::spin();
}
