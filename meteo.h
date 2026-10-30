#ifndef METEO_H
#define METEO_H

/*
 * Simulation of one day of weather measurements.
 * This part knows nothing about the graphical interface.
 */

/* One reading every 10 simulated minutes over 24 hours = 144 readings. */
#define READINGS_PER_DAY 144
#define MINUTES_BETWEEN_READINGS 10

/* Physical limits of the simulated sensors. */
#define TEMPERATURE_MIN -5.0
#define TEMPERATURE_MAX 40.0
#define HUMIDITY_MIN 0.0
#define HUMIDITY_MAX 100.0

/* One measurement taken by the station. */
typedef struct {
    int minute;         /* simulated time since 00:00, in minutes */
    double temperature; /* degrees Celsius */
    double humidity;    /* percent */
} Reading;

/* All the readings of one simulated day. */
typedef struct {
    Reading readings[READINGS_PER_DAY];
    int count; /* number of readings generated so far */

    /* Weather of the day, chosen at random once at the start. */
    double mean_temperature;
    double temperature_amplitude;
    double mean_humidity;

    /* Slow random drift added to the daily cycle. */
    double temperature_drift;
    double humidity_drift;
} WeatherDay;

/* Prepare an empty day with random weather parameters. */
void weather_day_init(WeatherDay *day);

/* Generate the next reading. Returns 1 if a reading was added, 0 if the day is already complete. */
int weather_day_add_reading(WeatherDay *day);

/* Write every reading to a text file. Returns 0 on success, -1 on error (errno says why). */
int weather_day_export_txt(const WeatherDay *day, const char *path);

#endif
