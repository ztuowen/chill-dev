
// a central place to turn on debugging messages

#ifndef DEBUGCHILL_H
#define DEBUGCHILL_H

#ifdef DEBUGCHILL 
#define DEBUG_PRINT(args...) fprintf(stderr, args )
#else
#define DEBUG_PRINT(args...) do {} while(0) /* Don't do anything  */
#endif

#endif
