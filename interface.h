#ifndef INTERFACE_H
#define INTERFACE_H

/*
 * Graphical interface of the weather station (GTK 3 + Cairo).
 * Call gtk_init() before interface_create(), then gtk_main().
 */

/* Build the main window and start the simulation timer. */
void interface_create(void);

#endif
