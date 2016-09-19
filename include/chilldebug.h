
// a central place to turn on debugging messages

// enable the next line to get lots of output 
//#define DEBUGCHILL
#ifndef DEBUGCHILL_H
#define DEBUGCHILL_H

#ifdef DEBUGCHILL 
#define DEBUG_PRINT(args...) fprintf(stderr, args )
#else
#define DEBUG_PRINT(args...) do {} while(0) /* Don't do anything  */
#endif

#endif
