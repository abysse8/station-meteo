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
    /* TODO Boubacar (B1): generate the next reading; see docs/repartition.md.
     * Add a clamp helper if useful. Return 1 only after adding a reading.
     * Placeholder: no reading generated, so callers cannot read uninitialized data. */
    (void)day;
    return 0;
}

int weather_day_export_txt(const WeatherDay *day, const char *path)
{
    /* TODO Chahid (C1): write the TXT file; see docs/repartition.md.
     * Placeholder: report "not implemented" without creating or truncating a file. */
    (void)day;
    (void)path;
    errno = ENOSYS;
    return -1;
}
