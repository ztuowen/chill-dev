#ifndef DEBUGCHILL_H
#define DEBUGCHILL_H

#include <libgen.h>
#include <string.h>
#include <stdlib.h>

#ifndef NDEBUG // means that CMAKE_BUILD_TYPE=Debug
#define DEBUGCHILL
#endif
// This thing below potentially create leaks
#define FILENAME basename(strdup(__FILE__))

#ifdef DEBUGCHILL 
#define CHILL_DEBUG_PRINT(format,args...) fprintf(stderr,"%s, %s, LN%d:\t" format,FILENAME,__FUNCTION__, \
        __LINE__, ##args )
#define CHILL_DEBUG_BEGIN   { \
                                fprintf(stderr,"=========\t%s, %s, LN%d\t=========\n",FILENAME,__FUNCTION__,__LINE__);
#define CHILL_DEBUG_END     fprintf(stderr,"===========================\n");}
#else
#define CHILL_DEBUG_PRINT(format,args...) do {} while(0) /* Don't do anything  */
#define CHILL_DEBUG_BEGIN   while(0) {
#define CHILL_DEBUG_END     }
#endif

// TODO below should be substituted by some error throwing? to be more consistent with cpp style
#define CHILL_ERROR(format,args...) fprintf(stderr,"ERROR:\t%s, %s, LN%d:\t" format,FILENAME,__FUNCTION__, \
        __LINE__, ##args )


#endif
