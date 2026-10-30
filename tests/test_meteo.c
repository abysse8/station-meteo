#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "meteo.h"

static int failures;

#define CHECK(condition, message) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL: %s\n", message); \
        failures++; \
    } \
} while (0)

static void test_simulation(void)
{
    WeatherDay day;
    weather_day_init(&day);
    CHECK(day.count == 0, "initial day must be empty");
    for (int i = 0; i < READINGS_PER_DAY; i++) {
        int added = weather_day_add_reading(&day);
        CHECK(added == 1, "a reading must be added until the day is full");
        CHECK(day.count == i + 1, "count must increase exactly once");
        if (added != 1 || day.count != i + 1) {
            return; /* Do not inspect a reading that was not created. */
        }
        const Reading *reading = &day.readings[i];
        CHECK(reading->minute == i * MINUTES_BETWEEN_READINGS, "reading time must advance by ten minutes");
        CHECK(isfinite(reading->temperature) && reading->temperature >= TEMPERATURE_MIN
              && reading->temperature <= TEMPERATURE_MAX, "temperature must stay within sensor limits");
        CHECK(isfinite(reading->humidity) && reading->humidity >= HUMIDITY_MIN
              && reading->humidity <= HUMIDITY_MAX, "humidity must stay within sensor limits");
    }
    Reading last = day.readings[READINGS_PER_DAY - 1];
    CHECK(last.minute == 1430, "last reading must represent 23:50");
    CHECK(weather_day_add_reading(&day) == 0, "a full day must reject another reading");
    CHECK(day.count == READINGS_PER_DAY, "a full day must keep its count");
    CHECK(day.readings[READINGS_PER_DAY - 1].temperature == last.temperature
          && day.readings[READINGS_PER_DAY - 1].humidity == last.humidity
          && day.readings[READINGS_PER_DAY - 1].minute == last.minute,
          "rejecting a reading must preserve the final reading");
}

static void test_export(void)
{
    /* Independent of the simulation task: use two known measurements. */
    WeatherDay day = {0};
    day.count = 2;
    day.readings[0] = (Reading){0, 12.5, 65};
    day.readings[1] = (Reading){10, 13.0, 60};
    char path[] = "/tmp/station-meteo-test-XXXXXX";
    int fd = mkstemp(path);
    CHECK(fd >= 0, "temporary export file must be created");
    if (fd < 0) return;
    close(fd);

    CHECK(weather_day_export_txt(&day, path) == 0, "export must succeed for a writable file");
    FILE *file = fopen(path, "r");
    CHECK(file != NULL, "export file must be readable");
    if (file != NULL) {
        char line[256];
        int rows = 0, header = 0;
        while (fgets(line, sizeof line, file) != NULL) {
            if (strstr(line, "WEATHER STATION") != NULL) header = 1;
            int hour, minute;
            double temperature, humidity;
            if (sscanf(line, "%d:%d %lf %lf", &hour, &minute, &temperature, &humidity) == 4) {
                CHECK(rows < day.count, "export must not contain extra readings");
                if (rows < day.count) {
                    const Reading *expected = &day.readings[rows];
                    CHECK(hour * 60 + minute == expected->minute, "exported time must match the reading");
                    CHECK(fabs(temperature - expected->temperature) < 0.01, "exported temperature must match");
                    CHECK(fabs(humidity - expected->humidity) < 0.01, "exported humidity must match");
                }
                rows++;
            }
        }
        CHECK(header, "export must include the station header");
        CHECK(rows == day.count, "export must contain every reading");
        fclose(file);
    }
    unlink(path);
    errno = 0;
    CHECK(weather_day_export_txt(&day, "/") == -1, "exporting to a directory must fail");
    CHECK(errno != 0, "export failure must set errno");
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "simulation") == 0) {
        test_simulation();
    } else if (argc == 2 && strcmp(argv[1], "export") == 0) {
        test_export();
    } else {
        fprintf(stderr, "Usage: %s simulation|export\n", argv[0]);
        return 2;
    }
    if (failures != 0) return 1;
    puts("PASS");
    return 0;
}
