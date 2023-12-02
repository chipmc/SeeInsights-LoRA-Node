#ifndef CONFIG_H
#define CONFIG_H

#define DEFAULT_PEOPLE_LIMIT 5             // Values above this will generate an alert
#define PEOPLECOUNTER_DEBUG 1              // Prints occupancy numbers as well as the resulting stateStack with visual representations
#define OCCUPANCYSTATE_DEBUG 0             // Prints the occupancy state using ASCII characters to produce a large number that can be read at a distance.
#define TENFOOTDISPLAY 1                   // Prints the personCount using ASCII characters to produce a large number that can be read at a distance.
#define SINGLE_ENTRANCE 1                  // If this is the only entrance, negative occupancy values are not allowed
#define MOUNTED_INSIDE 0                   // Reverses the directions

#endif