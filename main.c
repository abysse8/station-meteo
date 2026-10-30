#include <stdio.h>
#include <gtk/gtk.h>

#include "interface.h"

int main(int argc, char *argv[])
{
    /* gtk_init_check returns FALSE instead of crashing when no display is available. */
    if (!gtk_init_check(&argc, &argv)) {
        fprintf(stderr, "Error: could not start GTK (no graphical display?)\n");
        return 1;
    }

    interface_create();
    gtk_main(); /* runs until the window is closed */

    return 0;
}
