//
// Created by Konodoki on 2025/10/7.
//

#include "GPSSolve.h"
#define BUFF_SIZE 1024
static char sovle_buff[BUFF_SIZE]={0};
static uint32_t buff_pointer=0;
static gps_data_t gps_data_preview;
static gps_data_t gps_data;
gps_data_t* get_gps_data() {
    return  &gps_data;
}
void add_sentence(char *sentence) {
    uint32_t len = strlen(sentence);
    if (buff_pointer>=BUFF_SIZE||len>BUFF_SIZE-buff_pointer)return;
    strcpy(sovle_buff+buff_pointer,sentence);
    buff_pointer+=len+1;
    sovle_buff[buff_pointer-1]='\n';
}
void solve_once() {
    char *token=0;
    char *rest=sovle_buff;
    uint32_t gsa_pointer=0;

    char last_gsv_system[2]="00";
    int gsv_pointer=-1;
    uint32_t gsv_child_pointer=0;
    while ((token=strtok_my(rest,"\n",&rest))) {
        if (strncmp(token+3,"GGA,",4)==0) {
            parse_gpgga(token,&gps_data_preview.gga);
        }else if (strncmp(token+3,"GLL,",4)==0) {
            parse_gpgll(token,&gps_data_preview.gll);
        }else if (strncmp(token+3,"GSA,",4)==0) {
            if (gsa_pointer>=MAX_KIND_OF_SATELLITE)continue;
            parse_gpgsa(token,&gps_data_preview.satellites.gsa[gsa_pointer]);
            gsa_pointer++;
        }else if (strncmp(token+3,"GSV,",4)==0) {
            if (strncmp(token+1,last_gsv_system,2)==0) {
                gsv_child_pointer++;
            }else {
                gsv_pointer++;
                gsv_child_pointer=0;
            }
            parse_gpgsv_single(token,&gps_data_preview.satellites.gsv[gsv_pointer][gsv_child_pointer]);
            strncpy(last_gsv_system,token+1,2);
        }else if (strncmp(token+3,"RMC,",4)==0) {
            parse_gprmc(token,&gps_data_preview.rmc);
        }else if (strncmp(token+3,"VTG,",4)==0) {
            parse_gpvtg(token,&gps_data_preview.vtg);
        }else if (strncmp(token+3,"ZDA,",4)==0) {
            parse_gpzda(token,&gps_data_preview.zda);
        }else if (strncmp(token+3,"TXT,",4)==0) {
            //这个在我这个模块好像就是每帧的结尾信息
        }
    }
    //清理工作
    memcpy(&gps_data,&gps_data_preview,sizeof(gps_data_t));
    memset(sovle_buff,0,buff_pointer);
    buff_pointer=0;
}