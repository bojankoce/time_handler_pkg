#include "rclcpp/rclcpp.hpp"
#include "time_handler.hpp"
#include "custom_interfaces_pkg/srv/get_unix_timestamp.hpp"
#include "custom_interfaces_pkg/srv/set_unix_timestamp.hpp"
#include "custom_interfaces_pkg/srv/adjust_time.hpp"
#include <memory>
#include <time.h>

using std::placeholders::_1;
using std::placeholders::_2;

class RtcNodeROSWrapper : public rclcpp::Node
{
public:
    RtcNodeROSWrapper() : Node("rtc_node")
    {        
        //TODO(bojankoce): Take the RTC path ("/dev/rtc0") from parameter list
        rtc_.reset(new TimeHandler("/dev/rtc0"));  
               
        get_unix_time_server_ = this->create_service<custom_interfaces_pkg::srv::GetUnixTimestamp>(
            "get_unix_timestamp",
            std::bind(&RtcNodeROSWrapper::callbackGetUnixTimestamp, this, _1, _2));              
        RCLCPP_INFO(this->get_logger(), "Get Unix Timestamp Service server has been started.");

        set_unix_time_server_ = this->create_service<custom_interfaces_pkg::srv::SetUnixTimestamp>(
            "set_unix_timestamp",
            std::bind(&RtcNodeROSWrapper::callbackSetUnixTimestamp, this, _1, _2));              
        RCLCPP_INFO(this->get_logger(), "Set Unix Timestamp Service server has been started.");

        //TODO(bojankoce): Add AdjustTime Server that will be used to adjust time time between Harwdare Clock (RTC) and System Clock
        // one way to do it is through std::system("/sbin/hwclock -X"); command: https://man7.org/linux/man-pages/man8/hwclock.8.html

    }

private:
    std::unique_ptr<TimeHandler> rtc_;
    
    int callbackGetUnixTimestamp(const custom_interfaces_pkg::srv::GetUnixTimestamp::Request::SharedPtr,
                            const custom_interfaces_pkg::srv::GetUnixTimestamp::Response::SharedPtr response)
    {        
        int ret = 0;
        time_t unix_time;
        
        ret = rtc_->GetTime(&unix_time);
        response->timestamp = static_cast<uint32_t>(unix_time);
        response->status = ret;
        response->message = "This is GetUnixTimestamp status message";
        RCLCPP_INFO(this->get_logger(), "Service GetUnixTimestamp is called");
        
        return ret;
    }    

    int callbackSetUnixTimestamp(const custom_interfaces_pkg::srv::SetUnixTimestamp::Request::SharedPtr request,
                            const custom_interfaces_pkg::srv::SetUnixTimestamp::Response::SharedPtr response)
    {        
        int ret = 0;
        time_t unix_time;
        unix_time = static_cast<time_t>(request->timestamp);        
        ret = rtc_->SetTime(unix_time);
        response->status = ret;
        response->message = "This is SetUnixTimestamp status message";
        RCLCPP_INFO(this->get_logger(), "Service SetUnixTimestamp is called");
        
        return ret;
    }  

    rclcpp::Service<custom_interfaces_pkg::srv::GetUnixTimestamp>::SharedPtr get_unix_time_server_;
    rclcpp::Service<custom_interfaces_pkg::srv::SetUnixTimestamp>::SharedPtr set_unix_time_server_;
    rclcpp::Service<custom_interfaces_pkg::srv::AdjustTime>::SharedPtr adjust_time_server_;
};

// FIXME(bojankoce): When we spin ROS2 node with rclcpp::spin(), it become single thread process which might cause an undeterminism in behaviour 
// when we have multiple topics, services, that can block each other's callbacks (especially if there are nested calls). That's why it is
// recommended to spin the node in multithread node. ROS1 is using ros::AsyncSpinner for that. This concept does not exist in ROS2.
// ROS2 contains MultiThreadedExecutor.
// References:
// -> https://roboticsbackend.com/ros-asyncspinner-example/
// -> https://nicolovaligi.com/articles/concurrency-and-parallelism-in-ros1-and-ros2-application-apis/
// -> https://github.com/TheRoboticsClub/colab-gsoc2019-Pankhuri_Vanjani/issues/6
// -> https://discourse.ros.org/t/async-executor-in-ros2/1575

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<RtcNodeROSWrapper>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}