//
// Created by Konodoki on 2025/10/7.
//

#include "NMEA0183Solve.h"

// 将度分格式转换为度
double dm_to_degree(double dm) {
    int degrees = (int)(dm / 100);
    double minutes = dm - degrees * 100;
    return degrees + minutes / 60.0;
}

// 解析GPRMC语句
int parse_gprmc(const char* sentence, gps_rmc_t* rmc) {
    if (sentence == NULL || rmc == NULL) {
        return -1;
    }

    // 检查语句格式
    if (strncmp(sentence+3, "RMC,", 4) != 0) { // 支持GNSS RMC
        return -2;
    }

    // 查找校验和分隔符
    const char* checksum_start = strchr(sentence, '*');
    if (checksum_start == NULL) {
        return -3;
    }

    // 复制语句用于解析（不包含校验和部分）
    char buffer[256];
    int len = checksum_start - sentence - 1; // 从$后到*前的内容
    if (len >= sizeof(buffer)) {
        return -4;
    }

    strncpy(buffer, sentence + 1, len);
    buffer[len] = '\0';

    // 初始化结构体
    memset(rmc, 0, sizeof(gps_rmc_t));
    rmc->utc_time = -1.0;
    rmc->status = -1;
    rmc->latitude = NAN;
    rmc->longitude = NAN;
    rmc->speed_over_ground = -1.0;
    rmc->course_over_ground = -1.0;
    rmc->day = -1;
    rmc->month = -1;
    rmc->year = -1;
    rmc->magnetic_variation = NAN;
    rmc->is_magnetic_east = -1;
    rmc->mode_indicator = -1;
    rmc->nav_status[0] = '\0';

    // 解析各个字段
    char* token;
    char* rest = buffer;
    int field_count = 0;

    // 临时存储字符串字段
    char time_str[16] = {0};
    char status_str[2] = {0};
    char lat_str[16] = {0};
    char lat_hemi[2] = {0};
    char lon_str[16] = {0};
    char lon_hemi[2] = {0};
    char speed_str[16] = {0};
    char course_str[16] = {0};
    char date_str[16] = {0};
    char mag_var_str[16] = {0};
    char mag_dir[2] = {0};
    char mode_str[2] = {0};
    char nav_status_str[2] = {0};

    while ((token = strtok_my(rest, ",", &rest))) {
        field_count++;

        switch (field_count) {
            case 1: // 语句类型，已验证
                break;

            case 2: // UTC时间
                strncpy(time_str, token, sizeof(time_str)-1);
                break;

            case 3: // 定位状态
                strncpy(status_str, token, sizeof(status_str)-1);
                break;

            case 4: // 纬度
                strncpy(lat_str, token, sizeof(lat_str)-1);
                break;

            case 5: // 纬度半球
                strncpy(lat_hemi, token, sizeof(lat_hemi)-1);
                break;

            case 6: // 经度
                strncpy(lon_str, token, sizeof(lon_str)-1);
                break;

            case 7: // 经度半球
                strncpy(lon_hemi, token, sizeof(lon_hemi)-1);
                break;

            case 8: // 地面速率
                strncpy(speed_str, token, sizeof(speed_str)-1);
                break;

            case 9: // 地面航向
                strncpy(course_str, token, sizeof(course_str)-1);
                break;

            case 10: // UTC日期
                strncpy(date_str, token, sizeof(date_str)-1);
                break;

            case 11: // 磁偏角
                strncpy(mag_var_str, token, sizeof(mag_var_str)-1);
                break;

            case 12: // 磁偏角方向
                strncpy(mag_dir, token, sizeof(mag_dir)-1);
                break;

            case 13: // 模式指示
                strncpy(mode_str, token, sizeof(mode_str)-1);
                break;

            case 14: // 导航状态（NMEA 4.10+）
                strncpy(nav_status_str, token, sizeof(nav_status_str)-1);
                break;
        }
    }

    // 解析时间 hhmmss.sss
    if (strlen(time_str) >= 6) {
        int hours, minutes;
        double seconds;
        if (sscanf(time_str, "%2d%2d%lf", &hours, &minutes, &seconds) >= 2) {
            if (hours >= 0 && hours <= 23 && minutes >= 0 && minutes <= 59 &&
                seconds >= 0.0 && seconds < 60.0) {
                rmc->hour = hours;
                rmc->minute = minutes;
                rmc->second = seconds;
                rmc->utc_time = hours + minutes/60.0 + seconds/3600.0;
                rmc->has_time = 1;
            }
        }
    }

    // 解析定位状态
    if (strlen(status_str) > 0) {
        if (status_str[0] == 'A' || status_str[0] == 'V') {
            rmc->status = (status_str[0] == 'A') ? 1 : 0;
            rmc->has_status = 1;
        }
    }

    // 解析纬度
    if (strlen(lat_str) > 0) {
        double lat_dm = atof(lat_str);
        if (lat_dm > 0) {
            rmc->latitude_degrees = (int)(lat_dm / 100);
            rmc->latitude_minutes = lat_dm - rmc->latitude_degrees * 100;
            rmc->latitude = dm_to_degree(lat_dm);

            if (strlen(lat_hemi) > 0 && (lat_hemi[0] == 'N' || lat_hemi[0] == 'S')) {
                if (lat_hemi[0] == 'S') {
                    rmc->latitude = -rmc->latitude;
                }
                rmc->is_north = (lat_hemi[0] == 'N') ? 1 : 0;
            }
            rmc->has_latitude = 1;
        }
    }

    // 解析经度
    if (strlen(lon_str) > 0) {
        double lon_dm = atof(lon_str);
        if (lon_dm > 0) {
            rmc->longitude_degrees = (int)(lon_dm / 100);
            rmc->longitude_minutes = lon_dm - rmc->longitude_degrees * 100;
            rmc->longitude = dm_to_degree(lon_dm);

            if (strlen(lon_hemi) > 0 && (lon_hemi[0] == 'E' || lon_hemi[0] == 'W')) {
                if (lon_hemi[0] == 'W') {
                    rmc->longitude = -rmc->longitude;
                }
                rmc->is_east = (lon_hemi[0] == 'E') ? 1 : 0;
            }
            rmc->has_longitude = 1;
        }
    }

    // 解析速度和航向
    if (strlen(speed_str) > 0) {
        double speed = atof(speed_str);
        if (speed >= 0.0 && speed <= 999.9) {
            rmc->speed_over_ground = speed;
            rmc->has_speed = 1;
        }
    }

    if (strlen(course_str) > 0) {
        double course = atof(course_str);
        if (course >= 0.0 && course < 360.0) {
            rmc->course_over_ground = course;
            rmc->has_course = 1;
        }
    }

    // 解析日期 ddmmyy
    if (strlen(date_str) >= 6) {
        int day, month, year_short;
        if (sscanf(date_str, "%2d%2d%2d", &day, &month, &year_short) == 3) {
            if (day >= 1 && day <= 31 && month >= 1 && month <= 12) {
                rmc->day = day;
                rmc->month = month;
                rmc->year = 2000 + year_short; // 假设是2000年之后的年份
                if (rmc->year < 2000 || rmc->year > 2100) {
                    rmc->year = 1900 + year_short; // 回退到1900年代
                }
                rmc->has_date = 1;
            }
        }
    }

    // 解析磁偏角
    if (strlen(mag_var_str) > 0) {
        double mag_var = atof(mag_var_str);
        if (mag_var >= 0.0 && mag_var <= 180.0) {
            rmc->magnetic_variation = mag_var;

            if (strlen(mag_dir) > 0 && (mag_dir[0] == 'E' || mag_dir[0] == 'W')) {
                if (mag_dir[0] == 'W') {
                    rmc->magnetic_variation = -rmc->magnetic_variation;
                }
                rmc->is_magnetic_east = (mag_dir[0] == 'E') ? 1 : 0;
            }
            rmc->has_magnetic_variation = 1;
        }
    }

    // 解析模式指示
    if (strlen(mode_str) > 0) {
        switch (toupper(mode_str[0])) {
            case 'A': rmc->mode_indicator = 0; break; // 自主定位
            case 'D': rmc->mode_indicator = 1; break; // 差分
            case 'E': rmc->mode_indicator = 2; break; // 估算
            case 'N': rmc->mode_indicator = 3; break; // 数据无效
            case 'M': rmc->mode_indicator = 4; break; // 手动输入
            case 'S': rmc->mode_indicator = 5; break; // 模拟器
            default:  rmc->mode_indicator = -1; break;
        }
        if (rmc->mode_indicator != -1) {
            rmc->has_mode = 1;
        }
    }

    // 解析导航状态
    if (strlen(nav_status_str) > 0) {
        strncpy(rmc->nav_status, nav_status_str, sizeof(rmc->nav_status)-1);
        rmc->has_nav_status = 1;
    }

    return 0;
}
// 打印解析结果的辅助函数
void print_gprmc_info(const gps_rmc_t* rmc) {
    printf("=== GPS RMC Data ===\n");

    // 时间信息
    if (rmc->has_time) {
        printf("UTC Time: %.6f (%02d:%02d:%06.3f)\n",
               rmc->utc_time, rmc->hour, rmc->minute, rmc->second);
    } else {
        printf("UTC Time: Not Available\n");
    }

    // 状态信息
    if (rmc->has_status) {
        printf("Status: %d\n", rmc->status);
    } else {
        printf("Status: Not Provided\n");
    }

    // 位置信息
    if (rmc->has_latitude) {
        printf("Latitude: %.8f° (%s)\n", rmc->latitude,
               rmc->is_north == 1 ? "N" : (rmc->is_north == 0 ? "S" : "Unknown"));
        printf("  Raw: %02.0f° %06.3f'\n", rmc->latitude_degrees, rmc->latitude_minutes);
    } else {
        printf("Latitude: Not Available\n");
    }

    if (rmc->has_longitude) {
        printf("Longitude: %.8f° (%s)\n", rmc->longitude,
               rmc->is_east == 1 ? "E" : (rmc->is_east == 0 ? "W" : "Unknown"));
        printf("  Raw: %03.0f° %06.3f'\n", rmc->longitude_degrees, rmc->longitude_minutes);
    } else {
        printf("Longitude: Not Available\n");
    }

    // 运动信息
    if (rmc->has_speed) {
        printf("Speed: %.1f knots", rmc->speed_over_ground);
    } else {
        printf("Speed: Not Available\n");
    }

    if (rmc->has_course) {
        printf("Course: %05.1f°\n", rmc->course_over_ground);
    } else {
        printf("Course: Not Available\n");
    }

    // 日期信息
    if (rmc->has_date) {
        printf("Date: %04d-%02d-%02d\n", rmc->year, rmc->month, rmc->day);
    } else {
        printf("Date: Not Available\n");
    }

    // 磁偏角信息
    if (rmc->has_magnetic_variation) {
        printf("Magnetic Variation: %.1f° (%s)\n",
               fabs(rmc->magnetic_variation),
               rmc->is_magnetic_east == 1 ? "East" :
               (rmc->is_magnetic_east == 0 ? "West" : "Unknown"));
    } else {
        printf("Magnetic Variation: Not Available\n");
    }

    // 模式信息
    if (rmc->has_mode) {
        printf("Mode: %d\n", rmc->mode_indicator);
    } else {
        printf("Mode: Not Provided\n");
    }

    // 导航状态
    if (rmc->has_nav_status) {
        printf("Navigation Status: %s\n", rmc->nav_status);
    }

    // 数据有效性汇总
    printf("\nData Availability:\n");
    printf("  Time: %s\n", rmc->has_time ? "Available" : "Not Available");
    printf("  Status: %s\n", rmc->has_status ? "Available" : "Not Available");
    printf("  Latitude: %s\n", rmc->has_latitude ? "Available" : "Not Available");
    printf("  Longitude: %s\n", rmc->has_longitude ? "Available" : "Not Available");
    printf("  Speed: %s\n", rmc->has_speed ? "Available" : "Not Available");
    printf("  Course: %s\n", rmc->has_course ? "Available" : "Not Available");
    printf("  Date: %s\n", rmc->has_date ? "Available" : "Not Available");
    printf("  Magnetic Variation: %s\n", rmc->has_magnetic_variation ? "Available" : "Not Available");
    printf("  Mode: %s\n", rmc->has_mode ? "Available" : "Not Available");
    printf("  Nav Status: %s\n", rmc->has_nav_status ? "Available" : "Not Available");

    // 位置质量检查
    printf("\n");

    printf("====================\n");
}
// 解析GPGGA语句
int parse_gpgga(const char* sentence, gps_gga_t* gga) {
    if (sentence == NULL || gga == NULL) {
        return -1;
    }

    // 检查语句格式
    if (strncmp(sentence+3, "GGA,", 4) != 0) { // 支持GNSS GGA
        return -2;
    }

    // 查找校验和分隔符
    const char* checksum_start = strchr(sentence, '*');
    if (checksum_start == NULL) {
        return -3;
    }

    // 复制语句用于解析（不包含校验和部分）
    char buffer[256];
    int len = checksum_start - sentence - 1; // 从$后到*前的内容
    if (len >= sizeof(buffer)) {
        return -4;
    }

    strncpy(buffer, sentence + 1, len);
    buffer[len] = '\0';

    // 初始化结构体
    memset(gga, 0, sizeof(gps_gga_t));
    gga->utc_time = -1.0;
    gga->latitude = NAN;
    gga->longitude = NAN;
    gga->fix_quality = -1;
    gga->satellites_used = -1;
    gga->hdop = -1.0;
    gga->altitude = NAN;
    gga->geoid_height = NAN;
    gga->diff_age = -1.0;
    gga->diff_station_id = -1;

    // 解析各个字段
    char* token;
    char* rest = buffer;
    int field_count = 0;

    // 临时存储字符串字段
    char time_str[16] = {0};
    char lat_str[16] = {0};
    char lat_hemi[2] = {0};
    char lon_str[16] = {0};
    char lon_hemi[2] = {0};
    char fix_quality_str[2] = {0};
    char satellites_str[4] = {0};
    char hdop_str[16] = {0};
    char altitude_str[16] = {0};
    char altitude_units[2] = {0};
    char geoid_height_str[16] = {0};
    char geoid_units[2] = {0};
    char diff_age_str[16] = {0};
    char diff_station_str[8] = {0};

    while ((token = strtok_my(rest, ",", &rest))) {
        field_count++;

        switch (field_count) {
            case 1: // 语句类型，已验证
                break;

            case 2: // UTC时间 hhmmss.sss
                strncpy(time_str, token, sizeof(time_str)-1);
                break;

            case 3: // 纬度
                strncpy(lat_str, token, sizeof(lat_str)-1);
                break;

            case 4: // 纬度半球
                strncpy(lat_hemi, token, sizeof(lat_hemi)-1);
                break;

            case 5: // 经度
                strncpy(lon_str, token, sizeof(lon_str)-1);
                break;

            case 6: // 经度半球
                strncpy(lon_hemi, token, sizeof(lon_hemi)-1);
                break;

            case 7: // 定位质量指示
                strncpy(fix_quality_str, token, sizeof(fix_quality_str)-1);
                break;

            case 8: // 使用卫星数量
                strncpy(satellites_str, token, sizeof(satellites_str)-1);
                break;

            case 9: // 水平精度因子
                strncpy(hdop_str, token, sizeof(hdop_str)-1);
                break;

            case 10: // 天线高度
                strncpy(altitude_str, token, sizeof(altitude_str)-1);
                break;

            case 11: // 高度单位
                strncpy(altitude_units, token, sizeof(altitude_units)-1);
                break;

            case 12: // 大地水准面高度
                strncpy(geoid_height_str, token, sizeof(geoid_height_str)-1);
                break;

            case 13: // 大地水准面高度单位
                strncpy(geoid_units, token, sizeof(geoid_units)-1);
                break;

            case 14: // 差分数据期限
                strncpy(diff_age_str, token, sizeof(diff_age_str)-1);
                break;

            case 15: // 差分基站标号
                strncpy(diff_station_str, token, sizeof(diff_station_str)-1);
                break;
        }
    }

    // 解析时间 hhmmss.sss
    if (strlen(time_str) >= 6) {
        int hours, minutes;
        double seconds;
        if (sscanf(time_str, "%2d%2d%lf", &hours, &minutes, &seconds) >= 2) {
            if (hours >= 0 && hours <= 23 && minutes >= 0 && minutes <= 59 &&
                seconds >= 0.0 && seconds < 60.0) {
                gga->hour = hours;
                gga->minute = minutes;
                gga->second = seconds;
                gga->utc_time = hours + minutes/60.0 + seconds/3600.0;
                gga->has_time = 1;
            }
        }
    }

    // 解析纬度
    if (strlen(lat_str) > 0) {
        double lat_dm = atof(lat_str);
        if (lat_dm > 0) {
            gga->latitude_degrees = (int)(lat_dm / 100);
            gga->latitude_minutes = lat_dm - gga->latitude_degrees * 100;
            gga->latitude = dm_to_degree(lat_dm);

            if (strlen(lat_hemi) > 0 && (lat_hemi[0] == 'N' || lat_hemi[0] == 'S')) {
                if (lat_hemi[0] == 'S') {
                    gga->latitude = -gga->latitude;
                }
                gga->is_north = (lat_hemi[0] == 'N') ? 1 : 0;
            }
            gga->has_latitude = 1;
        }
    }

    // 解析经度
    if (strlen(lon_str) > 0) {
        double lon_dm = atof(lon_str);
        if (lon_dm > 0) {
            gga->longitude_degrees = (int)(lon_dm / 100);
            gga->longitude_minutes = lon_dm - gga->longitude_degrees * 100;
            gga->longitude = dm_to_degree(lon_dm);

            if (strlen(lon_hemi) > 0 && (lon_hemi[0] == 'E' || lon_hemi[0] == 'W')) {
                if (lon_hemi[0] == 'W') {
                    gga->longitude = -gga->longitude;
                }
                gga->is_east = (lon_hemi[0] == 'E') ? 1 : 0;
            }
            gga->has_longitude = 1;
        }
    }

    // 解析定位质量
    if (strlen(fix_quality_str) > 0) {
        int quality = atoi(fix_quality_str);
        if (quality >= 0 && quality <= 8) {
            gga->fix_quality = quality;
            gga->has_fix_quality = 1;
        }
    }

    // 解析使用卫星数量
    if (strlen(satellites_str) > 0) {
        int satellites = atoi(satellites_str);
        if (satellites >= 0 && satellites <= 99) {
            gga->satellites_used = satellites;
            gga->has_satellites = 1;
        }
    }

    // 解析水平精度因子
    if (strlen(hdop_str) > 0) {
        double hdop = atof(hdop_str);
        if (hdop >= 0.0 && hdop <= 99.9) {
            gga->hdop = hdop;
            gga->has_hdop = 1;
        }
    }

    // 解析天线高度
    if (strlen(altitude_str) > 0) {
        double altitude = atof(altitude_str);
        if (altitude >= -9999.9 && altitude <= 9999.9) {
            gga->altitude = altitude;

            // 验证单位（应该是'M'）
            if (strlen(altitude_units) > 0 && altitude_units[0] != 'M') {
                // 单位不是米，但数据仍然保留
            }
            gga->has_altitude = 1;
        }
    }

    // 解析大地水准面高度
    if (strlen(geoid_height_str) > 0) {
        double geoid_height = atof(geoid_height_str);
        if (geoid_height >= -9999.9 && geoid_height <= 9999.9) {
            gga->geoid_height = geoid_height;

            // 验证单位（应该是'M'）
            if (strlen(geoid_units) > 0 && geoid_units[0] != 'M') {
                // 单位不是米，但数据仍然保留
            }
            gga->has_geoid_height = 1;
        }
    }

    // 解析差分数据期限
    if (strlen(diff_age_str) > 0) {
        double diff_age = atof(diff_age_str);
        if (diff_age >= 0.0) {
            gga->diff_age = diff_age;
            gga->has_diff_age = 1;
        }
    }

    // 解析差分基站标号
    if (strlen(diff_station_str) > 0) {
        int station_id = atoi(diff_station_str);
        if (station_id >= 0 && station_id <= 4095) { // 扩展范围
            gga->diff_station_id = station_id;
            gga->has_diff_station = 1;
        }
    }

    return 0;
}
// 打印解析结果的辅助函数
void print_gpgga_info(const gps_gga_t* gga) {
    printf("=== GPS GGA Data ===\n");

    // 时间信息
    if (gga->has_time) {
        printf("UTC Time: %.6f (%02d:%02d:%06.3f)\n",
               gga->utc_time, gga->hour, gga->minute, gga->second);
    } else {
        printf("UTC Time: Not Available\n");
    }

    // 位置信息
    if (gga->has_latitude) {
        printf("Latitude: %.8f° (%s)\n", gga->latitude,
               gga->is_north == 1 ? "N" : (gga->is_north == 0 ? "S" : "Unknown"));
        printf("  Raw: %02.0f° %06.3f'\n", gga->latitude_degrees, gga->latitude_minutes);
    } else {
        printf("Latitude: Not Available\n");
    }

    if (gga->has_longitude) {
        printf("Longitude: %.8f° (%s)\n", gga->longitude,
               gga->is_east == 1 ? "E" : (gga->is_east == 0 ? "W" : "Unknown"));
        printf("  Raw: %03.0f° %06.3f'\n", gga->longitude_degrees, gga->longitude_minutes);
    } else {
        printf("Longitude: Not Available\n");
    }

    // 定位质量信息
    if (gga->has_fix_quality) {
        printf("Fix Quality: %d\n", gga->fix_quality);
    } else {
        printf("Fix Quality: Not Available\n");
    }

    if (gga->has_satellites) {
        printf("Satellites Used: %d\n", gga->satellites_used);
    } else {
        printf("Satellites Used: Not Available\n");
    }

    if (gga->has_hdop) {
        printf("HDOP: %.1f\n", gga->hdop);
    } else {
        printf("HDOP: Not Available\n");
    }

    // 高程信息
    if (gga->has_altitude) {
        printf("Altitude: %.1f meters\n", gga->altitude);
    } else {
        printf("Altitude: Not Available\n");
    }

    if (gga->has_geoid_height) {
        printf("Geoid Height: %.1f meters\n", gga->geoid_height);
    } else {
        printf("Geoid Height: Not Available\n");
    }

    // 差分信息
    if (gga->has_diff_age) {
        printf("Differential Age: %.1f seconds\n", gga->diff_age);
    } else {
        printf("Differential Age: Not Available\n");
    }

    if (gga->has_diff_station) {
        printf("Differential Station: %04d\n", gga->diff_station_id);
    } else {
        printf("Differential Station: Not Available\n");
    }

    // 数据有效性汇总
    printf("\nData Availability:\n");
    printf("  Time: %s\n", gga->has_time ? "Available" : "Not Available");
    printf("  Latitude: %s\n", gga->has_latitude ? "Available" : "Not Available");
    printf("  Longitude: %s\n", gga->has_longitude ? "Available" : "Not Available");
    printf("  Fix Quality: %s\n", gga->has_fix_quality ? "Available" : "Not Available");
    printf("  Satellites: %s\n", gga->has_satellites ? "Available" : "Not Available");
    printf("  HDOP: %s\n", gga->has_hdop ? "Available" : "Not Available");
    printf("  Altitude: %s\n", gga->has_altitude ? "Available" : "Not Available");
    printf("  Geoid Height: %s\n", gga->has_geoid_height ? "Available" : "Not Available");
    printf("  Differential Age: %s\n", gga->has_diff_age ? "Available" : "Not Available");
    printf("  Differential Station: %s\n", gga->has_diff_station ? "Available" : "Not Available");

    printf("====================\n");
}

// 解析GPVTG语句
int parse_gpvtg(const char* sentence, gps_vtg_t* vtg) {
    if (sentence == NULL || vtg == NULL) {
        return -1;
    }

    // 检查语句格式
    if (strncmp(sentence+3, "VTG,", 4) != 0) {
        return -2;
    }

    // 查找校验和分隔符
    const char* checksum_start = strchr(sentence, '*');
    if (checksum_start == NULL) {
        return -3;
    }

    // 复制语句用于解析（不包含校验和部分）
    char buffer[256];
    int len = checksum_start - sentence - 1; // 从$后到*前的内容
    if (len >= sizeof(buffer)) {
        return -4;
    }

    strncpy(buffer, sentence + 1, len);
    buffer[len] = '\0';

    // 初始化结构体
    memset(vtg, 0, sizeof(gps_vtg_t));
    vtg->course_true = -1.0;        // 用负值表示无效
    vtg->course_magnetic = -1.0;
    vtg->speed_knots = -1.0;
    vtg->speed_kmh = -1.0;
    vtg->mode = -1;

    // 解析各个字段
    char* token;
    char* rest = buffer;
    int field_count = 0;

    // 临时存储字符串字段
    char course_true_str[16] = {0};
    char true_indicator[2] = {0};      // "T"
    char course_magnetic_str[16] = {0};
    char magnetic_indicator[2] = {0};  // "M"
    char speed_knots_str[16] = {0};
    char knots_indicator[2] = {0};     // "N"
    char speed_kmh_str[16] = {0};
    char kmh_indicator[2] = {0};       // "K"
    char mode_str[2] = {0};

    while ((token = strtok_my(rest, ",", &rest))) {
        field_count++;

        switch (field_count) {
            case 1: // 语句类型，已验证
                break;

            case 2: // 真北航向
                strncpy(course_true_str, token, sizeof(course_true_str)-1);
                break;

            case 3: // 真北指示符 "T"
                strncpy(true_indicator, token, sizeof(true_indicator)-1);
                break;

            case 4: // 磁北航向
                strncpy(course_magnetic_str, token, sizeof(course_magnetic_str)-1);
                break;

            case 5: // 磁北指示符 "M"
                strncpy(magnetic_indicator, token, sizeof(magnetic_indicator)-1);
                break;

            case 6: // 地面速率（节）
                strncpy(speed_knots_str, token, sizeof(speed_knots_str)-1);
                break;

            case 7: // 节指示符 "N"
                strncpy(knots_indicator, token, sizeof(knots_indicator)-1);
                break;

            case 8: // 地面速率（公里/小时）
                strncpy(speed_kmh_str, token, sizeof(speed_kmh_str)-1);
                break;

            case 9: // 公里/小时指示符 "K"
                strncpy(kmh_indicator, token, sizeof(kmh_indicator)-1);
                break;

            case 10: // 模式指示
                strncpy(mode_str, token, sizeof(mode_str)-1);
                break;
        }
    }

    // 解析真北航向
    if (strlen(course_true_str) > 0) {
        vtg->course_true = atof(course_true_str);
        vtg->has_true_course = 1;

        // 验证指示符
        if (strlen(true_indicator) > 0 && true_indicator[0] != 'T') {
            // 指示符不正确，但数据仍然保留
        }
    }

    // 解析磁北航向
    if (strlen(course_magnetic_str) > 0) {
        vtg->course_magnetic = atof(course_magnetic_str);
        vtg->has_magnetic_course = 1;

        // 验证指示符
        if (strlen(magnetic_indicator) > 0 && magnetic_indicator[0] != 'M') {
            // 指示符不正确，但数据仍然保留
        }
    }

    // 解析节速度
    if (strlen(speed_knots_str) > 0) {
        vtg->speed_knots = atof(speed_knots_str);
        vtg->has_speed_knots = 1;

        // 验证指示符
        if (strlen(knots_indicator) > 0 && knots_indicator[0] != 'N') {
            // 指示符不正确，但数据仍然保留
        }
    }

    // 解析公里/小时速度
    if (strlen(speed_kmh_str) > 0) {
        vtg->speed_kmh = atof(speed_kmh_str);
        vtg->has_speed_kmh = 1;

        // 验证指示符
        if (strlen(kmh_indicator) > 0 && kmh_indicator[0] != 'K') {
            // 指示符不正确，但数据仍然保留
        }
    }

    // 解析模式指示
    if (strlen(mode_str) > 0) {
        switch (mode_str[0]) {
            case 'A': vtg->mode = 0; break; // 自主定位
            case 'D': vtg->mode = 1; break; // 差分
            case 'E': vtg->mode = 2; break; // 估算
            case 'N': vtg->mode = 3; break; // 数据无效
            default:  vtg->mode = -1; break; // 未知
        }
        vtg->has_mode = 1;
    }

    return 0;
}

// 打印解析结果的辅助函数
void print_gpvtg_info(const gps_vtg_t* vtg) {
    printf("=== GPS VTG Data ===\n");

    // 航向信息
    if (vtg->has_true_course) {
        printf("True Course: %06.1f°\n", vtg->course_true);
    } else {
        printf("True Course: Not Available\n");
    }

    if (vtg->has_magnetic_course) {
        printf("Magnetic Course: %06.1f°\n", vtg->course_magnetic);
    } else {
        printf("Magnetic Course: Not Available\n");
    }

    // 速度信息
    if (vtg->has_speed_knots) {
        printf("Speed: %05.1f knots\n", vtg->speed_knots);
    } else {
        printf("Speed (knots): Not Available\n");
    }

    if (vtg->has_speed_kmh) {
        printf("Speed: %06.1f km/h\n", vtg->speed_kmh);
    } else {
        printf("Speed (km/h): Not Available\n");
    }

    // 模式信息
    printf("Mode: %d\n", vtg->mode);

    printf("====================\n");
}


// 解析GPGLL语句
int parse_gpgll(const char* sentence, gps_gll_t* gll) {
    if (sentence == NULL || gll == NULL) {
        return -1;
    }

    // 检查语句格式
    if (strncmp(sentence+3, "GLL,", 4) != 0) {
        return -2;
    }

    // 查找校验和分隔符
    const char* checksum_start = strchr(sentence, '*');
    if (checksum_start == NULL) {
        return -3;
    }

    // 复制语句用于解析（不包含校验和部分）
    char buffer[256];
    int len = checksum_start - sentence - 1; // 从$后到*前的内容
    if (len >= sizeof(buffer)) {
        return -4;
    }

    strncpy(buffer, sentence + 1, len);
    buffer[len] = '\0';

    // 初始化结构体
    memset(gll, 0, sizeof(gps_gll_t));
    gll->latitude = NAN;
    gll->longitude = NAN;
    gll->utc_time = -1.0;
    gll->data_valid = -1;
    gll->mode_indicator = -1;

    // 解析各个字段
    char* token;
    char* rest = buffer;
    int field_count = 0;

    // 临时存储字符串字段
    char lat_str[16] = {0};
    char lat_hemi[2] = {0};
    char lon_str[16] = {0};
    char lon_hemi[2] = {0};
    char time_str[16] = {0};
    char data_valid_str[2] = {0};
    char mode_str[2] = {0};

    while ((token = strtok_my(rest, ",", &rest))) {
        field_count++;

        switch (field_count) {
            case 1: // 语句类型，已验证
                break;

            case 2: // 纬度
                strncpy(lat_str, token, sizeof(lat_str)-1);
                break;

            case 3: // 纬度半球
                strncpy(lat_hemi, token, sizeof(lat_hemi)-1);
                break;

            case 4: // 经度
                strncpy(lon_str, token, sizeof(lon_str)-1);
                break;

            case 5: // 经度半球
                strncpy(lon_hemi, token, sizeof(lon_hemi)-1);
                break;

            case 6: // UTC时间
                strncpy(time_str, token, sizeof(time_str)-1);
                break;

            case 7: // 数据有效性
                strncpy(data_valid_str, token, sizeof(data_valid_str)-1);
                break;

            case 8: // 模式指示（可选字段）
                strncpy(mode_str, token, sizeof(mode_str)-1);
                break;
        }
    }

    // 解析纬度
    if (strlen(lat_str) > 0) {
        double lat_dm = atof(lat_str);
        if (lat_dm > 0) {
            gll->latitude_degrees = (int)(lat_dm / 100);
            gll->latitude_minutes = lat_dm - gll->latitude_degrees * 100;
            gll->latitude = dm_to_degree(lat_dm);

            if (strlen(lat_hemi) > 0) {
                if (lat_hemi[0] == 'S') {
                    gll->latitude = -gll->latitude;
                }
                gll->is_north = (lat_hemi[0] == 'N') ? 1 : 0;
            }
            gll->has_latitude = 1;
        }
    }

    // 解析经度
    if (strlen(lon_str) > 0) {
        double lon_dm = atof(lon_str);
        if (lon_dm > 0) {
            gll->longitude_degrees = (int)(lon_dm / 100);
            gll->longitude_minutes = lon_dm - gll->longitude_degrees * 100;
            gll->longitude = dm_to_degree(lon_dm);

            if (strlen(lon_hemi) > 0) {
                if (lon_hemi[0] == 'W') {
                    gll->longitude = -gll->longitude;
                }
                gll->is_east = (lon_hemi[0] == 'E') ? 1 : 0;
            }
            gll->has_longitude = 1;
        }
    }

    // 解析时间 hhmmss.sss
    if (strlen(time_str) >= 6) {
        int hours, minutes;
        double seconds;
        if (sscanf(time_str, "%2d%2d%lf", &hours, &minutes, &seconds) >= 2) {
            gll->hour = hours;
            gll->minute = minutes;
            gll->second = seconds;
            gll->utc_time = hours + minutes/60.0 + seconds/3600.0;
            gll->has_time = 1;
        }
    }

    // 解析数据有效性
    if (strlen(data_valid_str) > 0) {
        gll->data_valid = (data_valid_str[0] == 'A') ? 1 : 0;
        gll->has_data_valid = 1;
    }

    // 解析模式指示
    if (strlen(mode_str) > 0) {
        switch (mode_str[0]) {
            case 'A': gll->mode_indicator = 0; break; // 自主定位
            case 'D': gll->mode_indicator = 1; break; // 差分
            case 'E': gll->mode_indicator = 2; break; // 估算
            case 'N': gll->mode_indicator = 3; break; // 数据无效
            case 'M': gll->mode_indicator = 4; break; // 手动输入模式
            case 'S': gll->mode_indicator = 5; break; // 模拟器模式
            default:  gll->mode_indicator = -1; break; // 未知
        }
        gll->has_mode = 1;
    }

    return 0;
}

// 打印解析结果的辅助函数
void print_gpgll_info(const gps_gll_t* gll) {
    printf("=== GPS GLL Data ===\n");

    // 位置信息
    if (gll->has_latitude) {
        printf("Latitude: %.8f° (%s)\n", gll->latitude, gll->is_north ? "N" : "S");
        printf("  Raw: %02.0f° %06.3f'\n", gll->latitude_degrees, gll->latitude_minutes);
    } else {
        printf("Latitude: Not Available\n");
    }

    if (gll->has_longitude) {
        printf("Longitude: %.8f° (%s)\n", gll->longitude, gll->is_east ? "E" : "W");
        printf("  Raw: %03.0f° %06.3f'\n", gll->longitude_degrees, gll->longitude_minutes);
    } else {
        printf("Longitude: Not Available\n");
    }

    // 时间信息
    if (gll->has_time) {
        printf("UTC Time: %.6f (%02d:%02d:%06.3f)\n",
               gll->utc_time, gll->hour, gll->minute, gll->second);
    } else {
        printf("UTC Time: Not Available\n");
    }

    // 状态信息
    if (gll->has_data_valid) {
        printf("Data Valid: %d\n", gll->data_valid);
    } else {
        printf("Data Valid: Not Provided\n");
    }

    if (gll->has_mode) {
        printf("Mode: %d\n", gll->mode_indicator);
    } else {
        printf("Mode: Not Provided\n");
    }

    // 数据有效性汇总
    printf("\nData Availability:\n");
    printf("  Latitude: %s\n", gll->has_latitude ? "Available" : "Not Available");
    printf("  Longitude: %s\n", gll->has_longitude ? "Available" : "Not Available");
    printf("  Time: %s\n", gll->has_time ? "Available" : "Not Available");
    printf("  Data Valid Flag: %s\n", gll->has_data_valid ? "Available" : "Not Available");
    printf("  Mode Indicator: %s\n", gll->has_mode ? "Available" : "Not Available");

    // 位置质量检查
    printf("\n");
    printf("====================\n");
}

// 计算两个GLL位置之间的距离（ Haversine公式）
double calculate_distance(const gps_gll_t* gll1, const gps_gll_t* gll2) {
    if (!gll1->has_latitude || !gll1->has_longitude ||
        !gll2->has_latitude || !gll2->has_longitude) {
        return NAN;
    }

    double lat1 = gll1->latitude * M_PI / 180.0;
    double lon1 = gll1->longitude * M_PI / 180.0;
    double lat2 = gll2->latitude * M_PI / 180.0;
    double lon2 = gll2->longitude * M_PI / 180.0;

    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    double a = sin(dlat/2) * sin(dlat/2) +
               cos(lat1) * cos(lat2) * sin(dlon/2) * sin(dlon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));

    return 6371000.0 * c; // 地球半径6371km，返回米
}

// 解析GPZDA语句
int parse_gpzda(const char* sentence, gps_zda_t* zda) {
    if (sentence == NULL || zda == NULL) {
        return -1;
    }

    // 检查语句格式
    if (strncmp(sentence+3, "ZDA,", 4) != 0) {
        return -2;
    }

    // 查找校验和分隔符
    const char* checksum_start = strchr(sentence, '*');
    if (checksum_start == NULL) {
        return -3;
    }

    // 复制语句用于解析（不包含校验和部分）
    char buffer[256];
    int len = checksum_start - sentence - 1; // 从$后到*前的内容
    if (len >= sizeof(buffer)) {
        return -4;
    }

    strncpy(buffer, sentence + 1, len);
    buffer[len] = '\0';

    // 初始化结构体
    memset(zda, 0, sizeof(gps_zda_t));
    zda->utc_time = -1.0;
    zda->day = -1;
    zda->month = -1;
    zda->year = -1;
    zda->local_timezone_hours = 0;
    zda->local_timezone_minutes = 0;

    // 解析各个字段
    char* token;
    char* rest = buffer;
    int field_count = 0;

    // 临时存储字符串字段
    char time_str[16] = {0};
    char day_str[4] = {0};
    char month_str[4] = {0};
    char year_str[6] = {0};
    char tz_hours_str[4] = {0};
    char tz_minutes_str[4] = {0};

    while ((token = strtok_my(rest, ",", &rest))) {
        field_count++;

        switch (field_count) {
            case 1: // 语句类型，已验证
                break;

            case 2: // UTC时间 hhmmss.sss
                strncpy(time_str, token, sizeof(time_str)-1);
                break;

            case 3: // 日
                strncpy(day_str, token, sizeof(day_str)-1);
                break;

            case 4: // 月
                strncpy(month_str, token, sizeof(month_str)-1);
                break;

            case 5: // 年
                strncpy(year_str, token, sizeof(year_str)-1);
                break;

            case 6: // 本地时区小时偏移
                strncpy(tz_hours_str, token, sizeof(tz_hours_str)-1);
                break;

            case 7: // 本地时区分钟偏移
                strncpy(tz_minutes_str, token, sizeof(tz_minutes_str)-1);
                break;
        }
    }

    // 解析时间 hhmmss.sss
    if (strlen(time_str) >= 6) {
        int hours, minutes;
        double seconds;
        if (sscanf(time_str, "%2d%2d%lf", &hours, &minutes, &seconds) >= 2) {
            if (hours >= 0 && hours <= 23 && minutes >= 0 && minutes <= 59 &&
                seconds >= 0.0 && seconds < 60.0) {
                zda->hour = hours;
                zda->minute = minutes;
                zda->second = seconds;
                zda->utc_time = hours + minutes/60.0 + seconds/3600.0;
                zda->has_time = 1;
            }
        }
    }

    // 解析日期
    if (strlen(day_str) > 0) {
        zda->day = atoi(day_str);
    }
    if (strlen(month_str) > 0) {
        zda->month = atoi(month_str);
    }
    if (strlen(year_str) > 0) {
        zda->year = atoi(year_str);
        // 处理2位数年份（假设是2000年之后的年份）
        if (zda->year < 100) {
            zda->year += 2000;
        }
    }

    // 检查日期完整性
    if (zda->day > 0 && zda->month > 0 && zda->year > 0) {
        zda->has_date = 1;
    }

    // 解析时区信息
    if (strlen(tz_hours_str) > 0) {
        zda->local_timezone_hours = atoi(tz_hours_str);
    }
    if (strlen(tz_minutes_str) > 0) {
        zda->local_timezone_minutes = atoi(tz_minutes_str);
    }
    if (strlen(tz_hours_str) > 0 || strlen(tz_minutes_str) > 0) {
        zda->has_timezone = 1;
    }

    return 0;
}

// 打印解析结果的辅助函数
void print_gpzda_info(const gps_zda_t* zda) {
    printf("=== GPS ZDA Data ===\n");

    // 时间信息
    if (zda->has_time) {
        printf("UTC Time: %.6f (%02d:%02d:%06.3f)\n",
               zda->utc_time, zda->hour, zda->minute, zda->second);
    } else {
        printf("UTC Time: Not Available\n");
    }

    // 日期信息
    if (zda->has_date) {
        printf("Date: %04d-%02d-%02d\n", zda->year, zda->month, zda->day);
    } else {
        printf("Date: Not Available\n");
    }

    // 时区信息
    if (zda->has_timezone) {
        printf("Local Timezone: UTC%s%02d:%02d\n",
               zda->local_timezone_hours >= 0 ? "+" : "",
               zda->local_timezone_hours, zda->local_timezone_minutes);
    } else {
        printf("Local Timezone: Not Provided\n");
    }

    // 数据有效性汇总
    printf("\nData Availability:\n");
    printf("  Time: %s\n", zda->has_time ? "Available" : "Not Available");
    printf("  Date: %s\n", zda->has_date ? "Available" : "Not Available");
    printf("  Timezone: %s\n", zda->has_timezone ? "Available" : "Not Available");
    // 日期时间验证
    printf("\n");

    printf("====================\n");
}