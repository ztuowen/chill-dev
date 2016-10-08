#ifndef CGDEBUG_H
#define CGDEBUG_H

#include <libgen.h>
#include <string.h>
#include <stdlib.h>

#ifndef NDEBUG // means that CMAKE_BUILD_TYPE=Debug
#define DEBUGCODEGEN
#endif
// This thing below potentially create leaks
#define FILENAME basename(strdup(__FILE__))

#ifdef DEBUGCODEGEN
#define CG_DEBUG_PRINT(format,args...) fprintf(stderr,"%15s | %15s | LN%-4d:\t" format,FILENAME,__FUNCTION__, \
        __LINE__, ##args )
#define CG_DEBUG_BEGIN   { \
                                fprintf(stderr,"=========\t%15s, %15s, LN%-4d\t=========\n",FILENAME,__FUNCTION__,__LINE__);
#define CG_DEBUG_END     fprintf(stderr,"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");}
#else
#define CG_DEBUG_PRINT(format,args...) do {} while(0) /* Don't do anything  */
#define CG_DEBUG_BEGIN   while(0) {
#define CG_DEBUG_END     }
#endif

// TODO below should be substituted by some error throwing? to be more consistent with cpp style
#define CG_ERROR(format,args...) fprintf(stderr,"ERROR:\t%s, %s, LN%d:\t" format,FILENAME,__FUNCTION__, \
        __LINE__, ##args )


#endif
