//
// Created by Konodoki on 2025/10/7.
//

#ifndef NMEA0183_NMEA0183_H
#define NMEA0183_NMEA0183_H
#include "SatelliteSolve.h"

// GPS RMC 数据结构体
typedef struct {
    // 时间信息
    double utc_time;           // UTC时间（小时+分钟/60+秒/3600）
    int hour;                  // 小时 (00-23)
    int minute;                // 分钟 (00-59)
    double second;             // 秒（含小数部分，00.000-59.999）

    // 定位状态
    int status;                // 定位状态：1=有效定位，0=无效定位，-1=未知

    // 纬度信息
    double latitude;           // 纬度（度，北纬为正，南纬为负）
    double latitude_degrees;   // 纬度度数部分
    double latitude_minutes;   // 纬度分钟部分
    int is_north;              // 是否北半球：1=北半球，0=南半球，-1=未知

    // 经度信息
    double longitude;          // 经度（度，东经为正，西经为负）
    double longitude_degrees;  // 经度度数部分
    double longitude_minutes;  // 经度分钟部分
    int is_east;               // 是否东经：1=东经，0=西经，-1=未知

    // 运动信息
    double speed_over_ground;  // 地面速率（节，000.0~999.9）
    double course_over_ground; // 地面航向（度，000.0~359.9）

    // 日期信息
    int day;                   // 日 (01-31)
    int month;                 // 月 (01-12)
    int year;                  // 年（完整年份，如2023）

    // 磁偏角信息
    double magnetic_variation; // 磁偏角（度，000.0~180.0）
    int is_magnetic_east;      // 磁偏角方向：1=东，0=西，-1=未知

    // 模式指示
    int mode_indicator;        // 模式：0=自主定位，1=差分，2=估算，3=数据无效，4=未知

    // 导航状态（NMEA 4.10+）
    char nav_status[2];        // 导航状态：A=有效，V=无效，其他=未知

    // 数据有效性标志
    int has_time;
    int has_status;
    int has_latitude;
    int has_longitude;
    int has_speed;
    int has_course;
    int has_date;
    int has_magnetic_variation;
    int has_mode;
    int has_nav_status;
} gps_rmc_t;
// GPS GGA 数据结构体
typedef struct {
    // 时间信息
    double utc_time;           // UTC时间（小时+分钟/60+秒/3600）
    int hour;                  // 小时 (00-23)
    int minute;                // 分钟 (00-59)
    double second;             // 秒（含小数部分，00.000-59.999）

    // 位置信息
    double latitude;           // 纬度（度，北纬为正，南纬为负）
    double latitude_degrees;   // 纬度度数部分
    double latitude_minutes;   // 纬度分钟部分
    int is_north;              // 是否北半球：1=北半球，0=南半球，-1=未知

    double longitude;          // 经度（度，东经为正，西经为负）
    double longitude_degrees;  // 经度度数部分
    double longitude_minutes;  // 经度分钟部分
    int is_east;               // 是否东经：1=东经，0=西经，-1=未知

    // 定位质量信息
    int fix_quality;           // 定位质量：0=无效，1=GPS定位，2=差分GPS定位，3=PPS定位，4=RTK，5=浮点RTK，6=估算，7=手动，8=模拟
    int satellites_used;       // 使用卫星数量（00-12）
    double hdop;               // 水平精度因子（0.5-99.9）

    // 高程信息
    double altitude;           // 天线离海平面的高度（米，-9999.9到9999.9）
    double geoid_height;       // 大地水准面高度（米，-9999.9到9999.9）

    // 差分信息
    double diff_age;           // 差分GPS数据期限（秒）
    int diff_station_id;       // 差分参考基站标号（0000-1023）

    // 数据有效性标志
    int has_time;
    int has_latitude;
    int has_longitude;
    int has_fix_quality;
    int has_satellites;
    int has_hdop;
    int has_altitude;
    int has_geoid_height;
    int has_diff_age;
    int has_diff_station;
} gps_gga_t;

// GPS VTG 数据结构体
typedef struct {
    // 航向信息
    double course_true;        // 以正北为参考基准的地面航向（000~359度）
    double course_magnetic;    // 以磁北为参考基准的地面航向（000~359度）

    // 地面速率信息
    double speed_knots;        // 地面速率（节，000.0~999.9）
    double speed_kmh;          // 地面速率（公里/小时，0000.0~1851.8）

    // 模式指示
    int mode;                  // 模式：0=自主定位，1=差分，2=估算，3=数据无效，-1=未知/未提供

    // 数据有效性标志
    int has_true_course;       // 是否有真北航向数据：1=有，0=无
    int has_magnetic_course;   // 是否有磁北航向数据：1=有，0=无
    int has_speed_knots;       // 是否有节速度数据：1=有，0=无
    int has_speed_kmh;         // 是否有公里/小时速度数据：1=有，0=无
    int has_mode;              // 是否有模式指示：1=有，0=无
} gps_vtg_t;

// GPS GLL 数据结构体
typedef struct {
    // 位置信息
    double latitude;           // 纬度（度，北纬为正，南纬为负）
    double latitude_degrees;   // 纬度度数部分
    double latitude_minutes;   // 纬度分钟部分
    int is_north;              // 是否北半球：1=北半球，0=南半球

    double longitude;          // 经度（度，东经为正，西经为负）
    double longitude_degrees;  // 经度度数部分
    double longitude_minutes;  // 经度分钟部分
    int is_east;               // 是否东经：1=东经，0=西经

    // 时间信息
    double utc_time;           // UTC时间（小时+分钟/60+秒/3600）
    int hour;                  // 小时
    int minute;                // 分钟
    double second;             // 秒（含小数部分）

    // 数据状态
    int data_valid;            // 数据有效性：1=有效，0=无效
    int mode_indicator;        // 模式指示：0=自主定位，1=差分，2=估算，3=数据无效，4=未知

    // 数据有效性标志
    int has_latitude;          // 是否有纬度数据
    int has_longitude;         // 是否有经度数据
    int has_time;              // 是否有时间数据
    int has_data_valid;        // 是否有数据有效性标志
    int has_mode;              // 是否有模式指示
} gps_gll_t;


// GPS ZDA 数据结构体
typedef struct {
    // 时间信息
    double utc_time;           // UTC时间（小时+分钟/60+秒/3600）
    int hour;                  // 小时 (00-23)
    int minute;                // 分钟 (00-59)
    double second;             // 秒（含小数部分，00.000-59.999）

    // 日期信息
    int day;                   // 日 (01-31)
    int month;                 // 月 (01-12)
    int year;                  // 年 (4位数，如2023)

    // 本地时区信息
    int local_timezone_hours;  // 本地时区小时偏移 (-13 to +13)
    int local_timezone_minutes;// 本地时区分钟偏移 (00 or 30, 通常为00)

    // 数据有效性标志
    int has_time;              // 是否有时间数据
    int has_date;              // 是否有日期数据
    int has_timezone;          // 是否有时区数据

} gps_zda_t;

typedef struct {
    gps_gga_t gga;
    gps_gll_t gll;
    gps_satellites satellites;
    gps_rmc_t rmc;
    gps_vtg_t vtg;
    gps_zda_t zda;
}gps_data_t;
int parse_gprmc(const char* sentence, gps_rmc_t* rmc);
void print_gprmc_info(const gps_rmc_t* rmc);

int parse_gpgga(const char* sentence, gps_gga_t* gga);
void print_gpgga_info(const gps_gga_t* gga);

int parse_gpvtg(const char* sentence, gps_vtg_t* vtg);
void print_gpvtg_info(const gps_vtg_t* vtg);

int parse_gpgll(const char* sentence, gps_gll_t* gll);
void print_gpgll_info(const gps_gll_t* gll);
double calculate_distance(const gps_gll_t* gll1, const gps_gll_t* gll2);

int parse_gpzda(const char* sentence, gps_zda_t* zda);
void print_gpzda_info(const gps_zda_t* zda);


#endif // NMEA0183_NMEA0183_H
