#ifndef _COMMON_EXECUTION_CONTROLLER_H
#define _COMMON_EXECUTION_CONTROLLER_H

#include "Execution.h"
#include <vector>

using namespace std;

void DebugPrintf(const unsigned int printMask, const char *fmt, ...);
rev::DWORD BranchHandlerFunc(void *context, void *userContext, rev::ADDR_TYPE nextInstruction);

typedef void(*GetCurrentRegistersFunc)(void *ctx, rev::ExecutionRegs *regs);
typedef void *(*GetMemoryInfoFunc)(void *ctx, void *ptr);
typedef void (*SetSymbolicExecutorFunc)(rev::SymbolicExecutorConstructor);

typedef void (*MarkMemoryNameFunc)(void *ctx, rev::ADDR_TYPE addr, const char *name);
typedef void (*MarkMemoryValueFunc)(void *ctx, rev::ADDR_TYPE addr, rev::DWORD value);

class CommonExecutionController : public ExecutionController {
private :
	bool UpdateLayout();

	vector<VirtualMemorySection> sec;
	vector<ModuleInfo> mod;
	uint32_t virtualSize, commitedSize;

protected:

	enum {
		NEW = EXECUTION_NEW,
		INITIALIZED = EXECUTION_INITIALIZED,
		SUSPENDED_AT_START = EXECUTION_SUSPENDED_AT_START,
		RUNNING = EXECUTION_RUNNING,
		SUSPENDED = EXECUTION_SUSPENDED,
		SUSPENDED_AT_TERMINATION = EXECUTION_SUSPENDED_AT_TERMINATION,
		TERMINATED = EXECUTION_TERMINATED,
		ERR = EXECUTION_ERR
	} execState;

	CommonExecutionController();

	wstring path;
	wstring cmdLine;
	void *entryPoint;

	uint32_t featureFlags;

	void *context;
	bool updated;

	ExecutionObserver *observer;

	GetCurrentRegistersFunc gcr;
	GetMemoryInfoFunc gmi;
	MarkMemoryNameFunc mmn;
	MarkMemoryValueFunc mmv;

	rev::SymExeConstructorFunc symbolicConstructor;
	rev::TrackCallbackFunc trackCb;
	rev::MarkCallbackFunc markCb;

public :
	virtual int GetState() const;

	virtual bool SetPath(const wstring &p);
	virtual bool SetCmdLine(const wstring &c);
	virtual bool SetEntryPoint(void *ep);
	virtual bool SetExecutionFeatures(unsigned int feat);

	virtual void SetExecutionObserver(ExecutionObserver *obs);
	virtual void SetTrackingObserver(rev::TrackCallbackFunc track, rev::MarkCallbackFunc mark);

	virtual unsigned int ExecutionBegin(void *address, void *cbCtx);
	virtual unsigned int ExecutionControl(void *address, void *cbCtx);
	virtual unsigned int ExecutionEnd(void *cbCtx);

	virtual void GetCurrentRegisters(void *ctx, rev::ExecutionRegs *registers);
	virtual void *GetMemoryInfo(void *ctx, void *ptr);

	virtual void DebugPrintf(const unsigned long printMask, const char *fmt, ...);

	virtual void SetSymbolicConstructor(rev::SymExeConstructorFunc constr);

	virtual bool GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount);
	virtual bool GetModules(ModuleInfo *&modules, int &moduleCount);
	virtual void MarkMemoryName(void *ctx, rev::ADDR_TYPE addr, const char *name);
	virtual void MarkMemoryValue(void *ctx, rev::ADDR_TYPE addr, rev::DWORD value);
};

#endif // !_COMMON_EXECUTION_CONTROLLER_H