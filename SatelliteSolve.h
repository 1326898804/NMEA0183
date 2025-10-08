//
// Created by Konodoki on 2025/10/7.
//

#ifndef NMEA0183_SATELLITESOLVE_H
#define NMEA0183_SATELLITESOLVE_H
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
// GPS GSA 数据结构体当前连接的卫星
typedef struct {
    // 模式设置
    int mode1;                 // 模式1：1=手动，2=自动，-1=未知
    int mode2;                 // 模式2：1=无定位，2=2D定位，3=3D定位，-1=未知

    // 卫星PRN列表（最多12颗卫星）
    int satellites[12];        // 卫星PRN编号
    int satellite_count;       // 实际使用的卫星数量

    // 精度因子
    double pdop;               // 位置精度因子
    double hdop;               // 水平精度因子
    double vdop;               // 垂直精度因子

    // 系统标识（NMEA 4.10+）
    char system_id;            // 系统标识：'G'=GPS，'P'=GPS/PPS，'L'=GLONASS，'A'=Galileo，'B'=BeiDou，'N'=GNSS

    // 数据有效性标志
    int has_mode1;
    int has_mode2;
    int has_pdop;
    int has_hdop;
    int has_vdop;
    int has_system_id;
} gps_gsa_t;

// 单个卫星信息结构体
typedef struct {
    int prn;                   // PRN码（卫星编号）
    int elevation;             // 仰角（00～90度）
    int azimuth;               // 方位角（000～359度）
    int snr;                   // 信噪比（00～99dB），-1表示无效
    int is_valid;              // 数据是否有效：1=有效，0=无效
} satellite_info_t;

// GPS GSV 数据结构体（单条语句）当前观测到的卫星
typedef struct {
    // 语句信息
    int total_messages;        // 总的GSV语句电文数
    int message_number;        // 当前GSV语句号
    int total_satellites;      // 可视卫星总数

    // 卫星信息（当前语句中的卫星，最多4颗）
    satellite_info_t satellites[4];
    int satellite_count;       // 当前语句中的卫星数量

    // 系统标识
    char system_id;            // 系统标识：'G'=GPS，'P'=GPS/PPS，'L'=GLONASS，'A'=Galileo，'B'=BeiDou，'N'=GNSS

    // 数据有效性标志
    int has_total_messages;
    int has_message_number;
    int has_total_satellites;
    int has_system_id;
} gps_gsv_t;

//gps当前的卫星情况
#define MAX_KIND_OF_SATELLITE 3 //最大的卫星种类，像我只有GPS，北斗，GLONASS
#define EACH_KIND_OF_SATELLITE 4 //每个种类多少条信息。像我差不多有四条
typedef struct {
    gps_gsa_t gsa[MAX_KIND_OF_SATELLITE];//当前连接的
    gps_gsv_t gsv[MAX_KIND_OF_SATELLITE][EACH_KIND_OF_SATELLITE];//观测到的
}gps_satellites;
char* strtok_my(char *rest,char* c,char **dest);
int parse_gpgsa(const char* sentence, gps_gsa_t* gsa);
void print_gpgsa_info(const gps_gsa_t* gsa);
int parse_gpgsv_single(const char* sentence, gps_gsv_t* gsv);
void print_gpgsv_single_info(const gps_gsv_t* gsv);

#endif // NMEA0183_SATELLITESOLVE_H
