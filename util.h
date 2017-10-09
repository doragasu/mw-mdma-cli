#ifndef _UTIL_H_
#define _UTIL_H_

#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
#define __OS_WIN
#include <windows.h>
#else
#include <unistd.h>
#endif

//=============================================================================
// TYPES
//=============================================================================
typedef char  s8;
typedef short s16;
typedef long  s32;
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/// 16-bit byte swap macro
#define ByteSwapWord(word)	do{(word) = ((word)>>8) | ((word)<<8);}while(0)

#ifndef MAX
#define MAX(a,b)	((a)>(b)?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b)	((a)<(b)?(a):(b))
#endif

/// printf-like macro that writes on stderr instead of stdout
#define PrintErr(...)	do{fprintf(stderr, __VA_ARGS__);}while(0)

// Delay ms function, compatible with both Windows and Unix
#ifdef __OS_WIN
#define DelayMs(ms) Sleep(ms)
#else
#define DelayMs(ms) usleep((ms)*1000)
#endif

#endif //_UTIL_H_

