//
// Created by Konodoki on 2025/10/7.
//

#include "SatelliteSolve.h"
char* strtok_my(char *rest,char* c,char **dest) {
    char *pointer=rest;
    while (*pointer!='\0') {
        if (*pointer==c[0]) {
            *pointer='\0';
            *dest=pointer+1;
            return rest;
        }
        pointer++;
    }
    return 0;
}

// 解析GPGSA语句
int parse_gpgsa(const char* sentence, gps_gsa_t* gsa) {
    if (sentence == NULL || gsa == NULL) {
        return -1;
    }

    // 检查语句格式（支持多种GNSS系统）
    if (strncmp(sentence+3, "GSA,", 4) != 0) {
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
    memset(gsa, 0, sizeof(gps_gsa_t));
    gsa->mode1 = -1;
    gsa->mode2 = -1;
    gsa->pdop = -1.0;
    gsa->hdop = -1.0;
    gsa->vdop = -1.0;
    gsa->satellite_count = 0;

    // 从语句头获取系统标识
    gsa->system_id = sentence[2]; // $GP->'P', $GN->'N', $GL->'L', etc.

    // 初始化卫星数组为无效值
    for (int i = 0; i < 12; i++) {
        gsa->satellites[i] = -1;
    }

    // 解析各个字段
    char* token;
    char* rest = buffer;
    int field_count = 0;

    // 临时存储字符串字段
    char mode1_str[2] = {0};
    char mode2_str[2] = {0};
    char satellite_strs[12][4] = {0}; // 最多12颗卫星
    char pdop_str[16] = {0};
    char hdop_str[16] = {0};
    char vdop_str[16] = {0};

    while ((token = strtok_my(rest, ",", &rest))) {
        field_count++;

        if (field_count == 1) {
            // 语句类型，已处理
            continue;
        }

        switch (field_count) {
            case 2: // 模式1
                strncpy(mode1_str, token, sizeof(mode1_str)-1);
                break;

            case 3: // 模式2
                strncpy(mode2_str, token, sizeof(mode2_str)-1);
                break;

            case 4: case 5: case 6: case 7: case 8: case 9:
            case 10: case 11: case 12: case 13: case 14: case 15:
                // 卫星PRN (字段4-15)
                if (strlen(token) > 0) {
                    strncpy(satellite_strs[field_count - 4], token,
                           sizeof(satellite_strs[field_count - 4])-1);
                }
                break;

            case 16: // PDOP
                strncpy(pdop_str, token, sizeof(pdop_str)-1);
                break;

            case 17: // HDOP
                strncpy(hdop_str, token, sizeof(hdop_str)-1);
                break;

            case 18: // VDOP
                strncpy(vdop_str, token, sizeof(vdop_str)-1);
                break;
        }
    }

    // 解析模式1
    if (strlen(mode1_str) > 0) {
        if (mode1_str[0] == 'M' || mode1_str[0] == 'A') {
            gsa->mode1 = (mode1_str[0] == 'M') ? 1 : 2;
            gsa->has_mode1 = 1;
        }
    }

    // 解析模式2
    if (strlen(mode2_str) > 0) {
        int mode2 = atoi(mode2_str);
        if (mode2 >= 1 && mode2 <= 3) {
            gsa->mode2 = mode2;
            gsa->has_mode2 = 1;
        }
    }

    // 解析卫星PRN列表
    for (int i = 0; i < 12; i++) {
        if (strlen(satellite_strs[i]) > 0) {
            int prn = atoi(satellite_strs[i]);
            if (prn > 0) {
                gsa->satellites[gsa->satellite_count] = prn;
                gsa->satellite_count++;
            }
        }
    }

    // 解析精度因子
    if (strlen(pdop_str) > 0) {
        double pdop = atof(pdop_str);
        if (pdop >= 0.0 && pdop <= 99.9) {
            gsa->pdop = pdop;
            gsa->has_pdop = 1;
        }
    }

    if (strlen(hdop_str) > 0) {
        double hdop = atof(hdop_str);
        if (hdop >= 0.0 && hdop <= 99.9) {
            gsa->hdop = hdop;
            gsa->has_hdop = 1;
        }
    }

    if (strlen(vdop_str) > 0) {
        double vdop = atof(vdop_str);
        if (vdop >= 0.0 && vdop <= 99.9) {
            gsa->vdop = vdop;
            gsa->has_vdop = 1;
        }
    }

    gsa->has_system_id = 1;

    return 0;
}
// 打印解析结果的辅助函数
void print_gpgsa_info(const gps_gsa_t* gsa) {
    printf("=== GPS GSA Data ===\n");

    // 系统信息
    if (gsa->has_system_id) {
        printf("System: %c\n", gsa->system_id);
    }

    // 模式信息
    if (gsa->has_mode1) {
        printf("Mode 1: %d\n", gsa->mode1);
    } else {
        printf("Mode 1: Not Available\n");
    }

    if (gsa->has_mode2) {
        printf("Mode 2: %d\n", gsa->mode2);
    } else {
        printf("Mode 2: Not Available\n");
    }

    // 卫星信息
    printf("Satellites Used: %d\n", gsa->satellite_count);
    if (gsa->satellite_count > 0) {
        printf("PRN List: ");
        for (int i = 0; i < gsa->satellite_count; i++) {
            printf("%d", gsa->satellites[i]);
            if (i < gsa->satellite_count - 1) {
                printf(", ");
            }
        }
        printf("\n");
    }

    // 精度因子信息
    if (gsa->has_pdop) {
        printf("PDOP: %.1f\n", gsa->pdop);
    } else {
        printf("PDOP: Not Available\n");
    }

    if (gsa->has_hdop) {
        printf("HDOP: %.1f\n", gsa->hdop);
    } else {
        printf("HDOP: Not Available\n");
    }

    if (gsa->has_vdop) {
        printf("VDOP: %.1f\n", gsa->vdop);
    } else {
        printf("VDOP: Not Available\n");
    }


    // 数据有效性汇总
    printf("\nData Availability:\n");
    printf("  System ID: %s\n", gsa->has_system_id ? "Available" : "Not Available");
    printf("  Mode 1: %s\n", gsa->has_mode1 ? "Available" : "Not Available");
    printf("  Mode 2: %s\n", gsa->has_mode2 ? "Available" : "Not Available");
    printf("  Satellites: %s\n", gsa->satellite_count > 0 ? "Available" : "Not Available");
    printf("  PDOP: %s\n", gsa->has_pdop ? "Available" : "Not Available");
    printf("  HDOP: %s\n", gsa->has_hdop ? "Available" : "Not Available");
    printf("  VDOP: %s\n", gsa->has_vdop ? "Available" : "Not Available");

    printf("====================\n");
}

// 解析单个GPGSV语句
int parse_gpgsv_single(const char* sentence, gps_gsv_t* gsv) {
    if (sentence == NULL || gsv == NULL) {
        return -1;
    }

    // 检查语句格式（支持多种GNSS系统）
    if (strncmp(sentence+3, "GSV,", 4) != 0) {
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
    memset(gsv, 0, sizeof(gps_gsv_t));
    gsv->total_messages = -1;
    gsv->message_number = -1;
    gsv->total_satellites = -1;
    gsv->satellite_count = 0;

    // 从语句头获取系统标识
    gsv->system_id = sentence[2]; // $GP->'P', $GN->'N', $GL->'L', etc.

    // 初始化卫星数据为无效值
    for (int i = 0; i < 4; i++) {
        gsv->satellites[i].prn = -1;
        gsv->satellites[i].elevation = -1;
        gsv->satellites[i].azimuth = -1;
        gsv->satellites[i].snr = -1;
        gsv->satellites[i].is_valid = 0;
    }

    // 解析各个字段
    char* token;
    char* rest = buffer;
    int field_count = 0;
    int satellite_data_index = 0;

    // 临时存储字符串字段
    char total_msg_str[4] = {0};
    char msg_num_str[4] = {0};
    char total_sat_str[4] = {0};

    // 卫星数据临时存储（4颗卫星，每颗4个字段）
    char sat_data[4][4][8] = {0};

    while ((token = strtok_my(rest, ",", &rest))) {
        field_count++;

        if (field_count == 1) {
            // 语句类型，已处理
            continue;
        }

        if (field_count <= 4) {
            // 前3个字段是语句信息（字段2-4）
            switch (field_count) {
                case 2: // 总语句数
                    strncpy(total_msg_str, token, sizeof(total_msg_str)-1);
                    break;
                case 3: // 当前语句号
                    strncpy(msg_num_str, token, sizeof(msg_num_str)-1);
                    break;
                case 4: // 总卫星数
                    strncpy(total_sat_str, token, sizeof(total_sat_str)-1);
                    break;
            }
        } else {
            // 后面的字段是卫星数据（每颗卫星4个数据项）
            // 字段索引计算：field_count从5开始，每4个字段一颗卫星
            int sat_index = (field_count - 5) / 4;      // 卫星索引（0-3）
            int data_index = (field_count - 5) % 4;     // 数据项索引（0-3）

            if (sat_index < 4 && data_index < 4) {
                strncpy(sat_data[sat_index][data_index], token,
                       sizeof(sat_data[sat_index][data_index])-1);
            }
        }
    }

    // 解析语句信息
    if (strlen(total_msg_str) > 0) {
        int total_msgs = atoi(total_msg_str);
        if (total_msgs >= 1 && total_msgs <= 9) {
            gsv->total_messages = total_msgs;
            gsv->has_total_messages = 1;
        }
    }

    if (strlen(msg_num_str) > 0) {
        int msg_num = atoi(msg_num_str);
        if (msg_num >= 1 && msg_num <= 9) {
            gsv->message_number = msg_num;
            gsv->has_message_number = 1;
        }
    }

    if (strlen(total_sat_str) > 0) {
        int total_sats = atoi(total_sat_str);
        if (total_sats >= 0 && total_sats <= 99) {
            gsv->total_satellites = total_sats;
            gsv->has_total_satellites = 1;
        }
    }

    gsv->has_system_id = 1;

    // 解析卫星数据
    for (int i = 0; i < 4; i++) {
        // 检查是否有PRN数据（第一个数据项）
        if (strlen(sat_data[i][0]) > 0) {
            int prn = atoi(sat_data[i][0]);
            if (prn > 0) {
                gsv->satellites[gsv->satellite_count].prn = prn;
                gsv->satellites[gsv->satellite_count].is_valid = 1;

                // 解析仰角
                if (strlen(sat_data[i][1]) > 0) {
                    int elevation = atoi(sat_data[i][1]);
                    if (elevation >= 0 && elevation <= 90) {
                        gsv->satellites[gsv->satellite_count].elevation = elevation;
                    }
                }

                // 解析方位角
                if (strlen(sat_data[i][2]) > 0) {
                    int azimuth = atoi(sat_data[i][2]);
                    if (azimuth >= 0 && azimuth <= 359) {
                        gsv->satellites[gsv->satellite_count].azimuth = azimuth;
                    }
                }

                // 解析信噪比
                if (strlen(sat_data[i][3]) > 0) {
                    int snr = atoi(sat_data[i][3]);
                    if (snr >= 0 && snr <= 99) {
                        gsv->satellites[gsv->satellite_count].snr = snr;
                    }
                }

                gsv->satellite_count++;
            }
        }
    }

    return 0;
}

// 打印解析结果的辅助函数
void print_gpgsv_single_info(const gps_gsv_t* gsv) {
    printf("=== GPS GSV Data (Single Message) ===\n");

    // 系统信息
    if (gsv->has_system_id) {
        printf("System: %c \n", gsv->system_id);
    }

    // 语句信息
    if (gsv->has_total_messages) {
        printf("Total Messages: %d\n", gsv->total_messages);
    } else {
        printf("Total Messages: Not Available\n");
    }

    if (gsv->has_message_number) {
        printf("Message Number: %d\n", gsv->message_number);
    } else {
        printf("Message Number: Not Available\n");
    }

    if (gsv->has_total_satellites) {
        printf("Total Satellites: %d\n", gsv->total_satellites);
    } else {
        printf("Total Satellites: Not Available\n");
    }

    // 卫星信息
    printf("Satellites in this message: %d\n", gsv->satellite_count);

    for (int i = 0; i < gsv->satellite_count; i++) {
        const satellite_info_t* sat = &gsv->satellites[i];
        printf("  Satellite #%d:\n", i + 1);
        printf("    PRN: %d\n", sat->prn);

        if (sat->elevation >= 0) {
            printf("    Elevation: %d°\n", sat->elevation);
        } else {
            printf("    Elevation: No data\n");
        }

        if (sat->azimuth >= 0) {
            printf("    Azimuth: %03d°\n", sat->azimuth);
        } else {
            printf("    Azimuth: No data\n");
        }

        if (sat->snr >= 0) {
            printf("    SNR: %02d dB\n", sat->snr);
        } else {
            printf("    SNR: No data\n");
        }
    }

    // 数据有效性汇总
    printf("\nData Availability:\n");
    printf("  System ID: %s\n", gsv->has_system_id ? "Available" : "Not Available");
    printf("  Total Messages: %s\n", gsv->has_total_messages ? "Available" : "Not Available");
    printf("  Message Number: %s\n", gsv->has_message_number ? "Available" : "Not Available");
    printf("  Total Satellites: %s\n", gsv->has_total_satellites ? "Available" : "Not Available");
    printf("  Satellite Data: %s\n", gsv->satellite_count > 0 ? "Available" : "Not Available");

    printf("=====================================\n");
}