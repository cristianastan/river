#include "SymbolicEnvironment.h"

#include "LargeStack.h"

namespace sym {
	inline bool SymbolicEnvironment::SetExecutor(SymbolicExecutor *e) {
		exec = e;
		return true;
	}

	bool ScopedSymbolicEnvironment::_SetCurrentInstruction(RiverInstruction *instruction, void *opBuffer) {
		return true;
	}

	ScopedSymbolicEnvironment::ScopedSymbolicEnvironment() {
		subEnv = nullptr;
	}

	inline bool ScopedSymbolicEnvironment::SetExecutor(SymbolicExecutor *e) {
		exec = e;
		return subEnv->SetExecutor(e);
	}

	void ScopedSymbolicEnvironment::PushState(stk::LargeStack &stack) {
		subEnv->PushState(stack);
		_PushState(stack);
	}

	void ScopedSymbolicEnvironment::PopState(stk::LargeStack &stack) {
		_PopState(stack);
		subEnv->PopState(stack);
	}

	bool ScopedSymbolicEnvironment::SetSubEnvironment(SymbolicEnvironment *env) {
		subEnv = env;
		return true;
	}

	bool ScopedSymbolicEnvironment::SetCurrentInstruction(RiverInstruction *instruction, void *opBuffer) {
		if (!subEnv->SetCurrentInstruction(instruction, opBuffer)) {
			return false;
		}

		return _SetCurrentInstruction(instruction, opBuffer);
	}

	bool ScopedSymbolicEnvironment::GetOperand(rev::BYTE opIdx, rev::BOOL &isTracked, rev::DWORD &concreteValue, void *&symbolicValue) {
		return subEnv->GetOperand(opIdx, isTracked, concreteValue, symbolicValue);
	}

	bool ScopedSymbolicEnvironment::GetFlgValue(rev::BYTE flg, rev::BOOL &isTracked, rev::BYTE &concreteValue, void *&symbolicValue) {
		return subEnv->GetFlgValue(flg, isTracked, concreteValue, symbolicValue);
	}

	bool ScopedSymbolicEnvironment::SetOperand(rev::BYTE opIdx, void *symbolicValue) {
		return subEnv->SetOperand(opIdx, symbolicValue);
	}

	bool ScopedSymbolicEnvironment::UnsetOperand(rev::BYTE opIdx) {
		return subEnv->UnsetOperand(opIdx);
	}

	void ScopedSymbolicEnvironment::SetFlgValue(rev::BYTE flg, void *symbolicValue) {
		return subEnv->SetFlgValue(flg, symbolicValue);
	}

	void ScopedSymbolicEnvironment::UnsetFlgValue(rev::BYTE flg) {
		return subEnv->UnsetFlgValue(flg);
	}
};