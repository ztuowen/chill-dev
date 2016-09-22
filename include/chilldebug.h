#ifndef DEBUGCHILL_H
#define DEBUGCHILL_H

#ifndef NDEBUG // means that CMAKE_BUILD_TYPE=Debug
#define DEBUGCHILL
#endif

#ifdef DEBUGCHILL 
#define CHILL_DEBUG_PRINT(format,args...) fprintf(stderr,"%s,%s,LN%d:\n\t" format,__FILE__,__FUNCTION__,__LINE__, ##args )
#define CHILL_DEBUG_BEGIN   { \
                                fprintf(stderr,"%s,%s,LN%d:\n",__FILE__,__FUNCTION__,__LINE__);
#define CHILL_DEBUG_END     }
#else
#define CHILL_DEBUG_PRINT(format,args...) do {} while(0) /* Don't do anything  */
#define CHILL_DEBUG_BEGIN   do {
#define CHILL_DEBUG_END     } while (0);
#endif


#endif
