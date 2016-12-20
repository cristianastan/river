#ifndef _WRAPPER_GLOBAL_H_
#define _WRAPPER_GLOBAL_H_

typedef void *(*AllocateVirtualFunc)(unsigned long);
typedef void (*FreeVirtualFunc)(void *);
typedef void *(*MapMemoryFunc)(unsigned long, unsigned long, unsigned long, unsigned long, void *);

typedef void (*TerminateProcessFunc)(int);
typedef void *(*GetTerminationCodeFunc)();

typedef int (*FormatPrintFunc)(char *buffer, size_t sizeOfBuffer, const char *format, char *);
typedef int (*PrintfFunc)(const char *format, ...);

typedef bool (*WriteFileFunc)(void *, void *, size_t, unsigned long *);
typedef long (*ToErrnoFunc)(long);

typedef long (*YieldExecutionFunc)(void);
typedef int (*InitSemaphoreFunc)(void *, int, int);
typedef int (*WaitSemaphoreFunc)(void *);
typedef int (*PostSemaphoreFunc)(void *);

typedef void (*FlushInstructionCacheFunc)(void);


extern AllocateVirtualFunc allocateVirtual;
extern FreeVirtualFunc freeVirtual;
extern MapMemoryFunc mapMemory;

extern TerminateProcessFunc terminateProcess;
extern GetTerminationCodeFunc getTerminationCode;

extern FormatPrintFunc formatPrint;
extern PrintfFunc Print;

extern WriteFileFunc writeFile;
extern ToErrnoFunc toErrno;

extern YieldExecutionFunc yieldExecution;
extern InitSemaphoreFunc initSemaphore;
extern WaitSemaphoreFunc waitSemaphore;
extern PostSemaphoreFunc postSemaphore;

extern FlushInstructionCacheFunc flushInstructionCache;

#endif
