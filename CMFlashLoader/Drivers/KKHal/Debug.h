#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdarg.h>
#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define DEBUG_PRINT DebugPrint
// #define DEBUG_PRINT(...)

void DebugPrint(const char* format, ...);

#ifdef	__cplusplus
}
#endif


#endif /* DEBUG_H_ */
