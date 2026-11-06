#include "rtc.h"
#include "io.h"
#include "string.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

static int get_update_in_progress_flag(void) {
    outb(CMOS_ADDRESS, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}

static uint8_t get_rtc_register(int reg) {
    outb(CMOS_ADDRESS, (uint8_t)reg);
    return inb(CMOS_DATA);
}

static uint8_t bcd_to_bin(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

void rtc_get_time(rtc_time_t* t) {
    // Wait until RTC update is not in progress
    while (get_update_in_progress_flag());

    uint8_t sec   = get_rtc_register(0x00);
    uint8_t min   = get_rtc_register(0x02);
    uint8_t hour  = get_rtc_register(0x04);
    uint8_t day   = get_rtc_register(0x07);
    uint8_t month = get_rtc_register(0x08);
    uint8_t year  = get_rtc_register(0x09);
    uint8_t reg_b = get_rtc_register(0x0B);

    // Convert BCD to binary if required (bit 2 of status B is 0 for BCD)
    if (!(reg_b & 0x04)) {
        sec   = bcd_to_bin(sec);
        min   = bcd_to_bin(min);
        hour  = ((hour & 0x7F) != hour) ? (bcd_to_bin(hour & 0x7F) | 0x80) : bcd_to_bin(hour);
        day   = bcd_to_bin(day);
        month = bcd_to_bin(month);
        year  = bcd_to_bin(year);
    }

    // Convert 12 hour clock to 24 hour clock if bit 1 is 0
    if (!(reg_b & 0x02) && (hour & 0x80)) {
        hour = ((hour & 0x7F) + 12) % 24;
    }

    t->second = sec;
    t->minute = min;
    t->hour   = hour;
    t->day    = day;
    t->month  = month;
    t->year   = 2000 + year;
}

void rtc_format(char* buf) {
    rtc_time_t t;
    rtc_get_time(&t);
    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
            t.year, t.month, t.day, t.hour, t.minute, t.second);
}
