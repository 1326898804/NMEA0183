//
// Created by Konodoki on 2025/10/7.
//

#ifndef NMEA0183_GPSSOLVE_H
#define NMEA0183_GPSSOLVE_H
#include "NMEA0183Solve.h"
void add_sentence(char *sentence);
void solve_once();
gps_data_t* get_gps_data();
#endif // NMEA0183_GPSSOLVE_H
