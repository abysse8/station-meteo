#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "meteo.h"

#define PI 3.14159265358979

/* Random number between min and max. */
static double random_between(double min, double max)
{
    return min + (max - min) * ((double)rand() / RAND_MAX);
}

/* Keep value inside [min, max]. */
static double clamp(double value, double min, double max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

void weather_day_init(WeatherDay *day)
{
    srand((unsigned int)time(NULL));

    day->count = 0;
    day->mean_temperature = random_between(8.0, 24.0);
    day->temperature_amplitude = random_between(3.0, 7.0);
    day->mean_humidity = random_between(50.0, 75.0);
    day->temperature_drift = 0.0;
    day->humidity_drift = 0.0;
}

int weather_day_add_reading(WeatherDay *day)
{
    Reading *reading;
    double hour;
    double daily_cycle;

    if (day->count >= READINGS_PER_DAY) {
        return 0;
    }

    reading = &day->readings[day->count];
    reading->minute = day->count * MINUTES_BETWEEN_READINGS;
    hour = reading->minute / 60.0;

    /* Daily cycle: +1 at 15:00 (warmest), -1 at 03:00 (coldest). */
    daily_cycle = cos(2.0 * PI * (hour - 15.0) / 24.0);

    /*
     * The drift changes only a little between two readings (random walk),
     * so the curves stay smooth instead of jumping randomly.
     */
    day->temperature_drift = clamp(day->temperature_drift + random_between(-0.3, 0.3), -2.0, 2.0);
    day->humidity_drift = clamp(day->humidity_drift + random_between(-1.5, 1.5), -8.0, 8.0);

    reading->temperature = day->mean_temperature
                         + day->temperature_amplitude * daily_cycle
                         + day->temperature_drift;

    /* Humidity goes down when the air warms up, so it follows the opposite cycle. */
    reading->humidity = day->mean_humidity
                      - 2.5 * day->temperature_amplitude * daily_cycle
                      + day->humidity_drift;

    reading->temperature = clamp(reading->temperature, TEMPERATURE_MIN, TEMPERATURE_MAX);
    reading->humidity = clamp(reading->humidity, HUMIDITY_MIN, HUMIDITY_MAX);

    day->count++;
    return 1;
}

int weather_day_export_txt(const WeatherDay *day, const char *path)
{
    FILE *file;
    time_t now;
    struct tm *today;
    char date[16] = "unknown";
    int i;

    now = time(NULL);
    today = localtime(&now);
    if (today != NULL) {
        strftime(date, sizeof date, "%d/%m/%Y", today);
    }

    file = fopen(path, "w");
    if (file == NULL) {
        return -1;
    }

    fprintf(file, "WEATHER STATION\n");
    fprintf(file, "Date: %s\n", date);
    fprintf(file, "Simulation: 24 hours\n");
    fprintf(file, "\n");
    fprintf(file, "Time     Temperature (C)    Humidity (%%)\n");

    for (i = 0; i < day->count; i++) {
        const Reading *reading = &day->readings[i];
        fprintf(file, "%02d:%02d%11.1f%18.0f\n",
                reading->minute / 60, reading->minute % 60,
                reading->temperature, reading->humidity);
    }

    /* A write error (disk full, ...) is only reported by ferror or fclose. */
    if (ferror(file)) {
        fclose(file);
        errno = EIO;
        return -1;
    }
    if (fclose(file) != 0) {
        return -1;
    }
    return 0;
}
