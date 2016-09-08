#include "RevSymbolicEnvironment.h"

#include "../revtracer/execenv.h"
#include "../revtracer/Tracking.h"
#include "../Execution/Execution.h"

static rev::BYTE GetFundamentalRegister(rev::BYTE reg) {
	if (reg < 0x20) {
		return reg & 0x07;
	}
	return reg;
}

RevSymbolicEnvironment::RevSymbolicEnvironment(void *revEnv, ExecutionController *ctl) {
	ctrl = ctl;
	pEnv = revEnv;
}

void RevSymbolicEnvironment::GetOperandLayout(const RiverInstruction &rIn) {
	unsigned int trackIndex = 0;

	for (unsigned int i = 0; i < 4; ++i) {
		addressOffsets[i] = 0xFFFFFFFF;
	}

	for (unsigned int i = 0; i < 4; ++i) {
		valueOffsets[i] = 0xFFFFFFFF;
	}

	flagOffset = 0xFFFFFFFF;

	if ((0 == (RIVER_SPEC_IGNORES_FLG & rIn.specifiers)) || (rIn.modFlags)) {
		flagOffset = trackIndex;
		trackIndex++;
	}

	for (int i = 3; i >= 0; --i) {
		if ((RIVER_OPTYPE(rIn.opTypes[i]) == RIVER_OPTYPE_MEM) && (0 != rIn.operands[i].asAddress->type)) {
			addressOffsets[i] = trackIndex;
			trackIndex += rIn.operands[i].asAddress->HasSegment() ? 2 : 1;

			if (0 == (rIn.specifiers & RIVER_SPEC_IGNORES_MEMORY)) {
				valueOffsets[i] = trackIndex + 1; // look at the layout one more time
				trackIndex += 2;
			}
		}
	}

	for (int i = 3; i >= 0; --i) {
		if (0 == (RIVER_SPEC_IGNORES_OP(i) & rIn.specifiers)) {
			switch (RIVER_OPTYPE(rIn.opTypes[i])) {
			case RIVER_OPTYPE_NONE:
			case RIVER_OPTYPE_IMM:
				break;
			case RIVER_OPTYPE_MEM: // holy shit this is an effed up switch
				if (0 == rIn.operands[i].asAddress->type) {
			case RIVER_OPTYPE_REG:
					valueOffsets[i] = trackIndex;
					trackIndex++;
				}
				break;
			/*case RIVER_OPTYPE_REG:
				valueOffsets[i] = trackIndex;
				trackIndex++;
				break;*/
			default:
				_asm int 3;
			}
		}
	}
}

template <rev::BYTE offset, rev::BYTE size> void *RevSymbolicEnvironment::GetSubexpression(rev::DWORD address) {
	void *symExpr = (void *)TrackAddr(pEnv, address, 0);

	return exec->ExtractBits(symExpr, (3 - offset) << 3, size << 3);
}

template <> void *RevSymbolicEnvironment::GetSubexpression<0, 4>(rev::DWORD address) {
	return (void *)TrackAddr(pEnv, address, 0);
}

void *RevSymbolicEnvironment::GetSubexpressionInvalid(rev::DWORD address) {
	__asm int 3;
}

RevSymbolicEnvironment::GetSubExpFunc RevSymbolicEnvironment::subExpsGet[4][5] = {
	{ nullptr, &RevSymbolicEnvironment::GetSubexpression<0, 1>, &RevSymbolicEnvironment::GetSubexpression<0, 2>,  &RevSymbolicEnvironment::GetSubexpression<0, 3>,  &RevSymbolicEnvironment::GetSubexpression<0, 4>  }, 
	{ nullptr, &RevSymbolicEnvironment::GetSubexpression<1, 1>, &RevSymbolicEnvironment::GetSubexpression<1, 2>,  &RevSymbolicEnvironment::GetSubexpression<1, 3>,  &RevSymbolicEnvironment::GetSubexpressionInvalid },
	{ nullptr, &RevSymbolicEnvironment::GetSubexpression<2, 1>, &RevSymbolicEnvironment::GetSubexpression<2, 2>,  &RevSymbolicEnvironment::GetSubexpressionInvalid, &RevSymbolicEnvironment::GetSubexpressionInvalid },
	{ nullptr, &RevSymbolicEnvironment::GetSubexpression<3, 1>, &RevSymbolicEnvironment::GetSubexpressionInvalid, &RevSymbolicEnvironment::GetSubexpressionInvalid, &RevSymbolicEnvironment::GetSubexpressionInvalid },
};

void *RevSymbolicEnvironment::GetExpression(rev::DWORD address, rev::DWORD size) {
	static const rev::DWORD sizes[3] = { 4, 2, 1 };
	rev::DWORD sz = sizes[RIVER_OPSIZE(size)];
	
	void *ret = nullptr;

	while (sz) {
		rev::DWORD fa = address & (~0x03), fo = address & 0x03;
		rev::DWORD copy = 4 - fo;

		if (sz < copy) {
			copy = sz;
		}

		void *tmp = (this->*subExpsGet[fo][sz])(address);

		if (nullptr == ret) {
			ret = tmp;
		} else {
			ret = exec->ConcatBits(ret, tmp);
		}

		address += copy;
		sz -= copy;
	}

	return ret;
}

template<rev::BYTE offset, rev::BYTE size> void RevSymbolicEnvironment::SetSubexpression(void *expr, rev::DWORD address, void *value) {
	rev::DWORD rSz = (4 - offset - size) << 3;
	rev::DWORD lSz = offset << 3;
	rev::DWORD lLsb = (4 - offset) << 3;

	void *ret = exec->ExtractBits(value, lLsb, lSz);
	ret = exec->ConcatBits(ret, expr);
	ret = exec->ConcatBits(ret, exec->ExtractBits(value, 0, rSz));

	MarkAddr(pEnv, address, (rev::DWORD)ret, 0);
}

template<rev::BYTE size> void RevSymbolicEnvironment::SetSubexpressionOffM(void *expr, rev::DWORD address, void *value) {
	rev::DWORD rSz = (4 - size) << 3;

	void *ret = exec->ConcatBits(exec->ExtractBits(value, size << 3, rSz), expr);
	
	MarkAddr(pEnv, address, (rev::DWORD)ret, 0);
}

template<rev::BYTE size> void RevSymbolicEnvironment::SetSubexpressionOff0(void *expr, rev::DWORD address, void *value) {
	void *ret = exec->ExtractBits(value, 0, (4 - size) << 3);
	ret = exec->ConcatBits(expr, ret);

	MarkAddr(pEnv, address, (rev::DWORD)ret, 0);
}

template <> void RevSymbolicEnvironment::SetSubexpression<0, 4>(void *expr, rev::DWORD address, void *value) {
	MarkAddr(pEnv, address, (rev::DWORD)expr, 0);
}

void RevSymbolicEnvironment::SetSubexpressionInvalid(void *expr, rev::DWORD address, void *value) {
	__asm int 3;
}

RevSymbolicEnvironment::SetSubExpFunc RevSymbolicEnvironment::subExpsSet[4][5] = {
	{ nullptr, &RevSymbolicEnvironment::SetSubexpressionOff0<1>, &RevSymbolicEnvironment::SetSubexpressionOff0<2>, &RevSymbolicEnvironment::SetSubexpressionOff0<3>, &RevSymbolicEnvironment::SetSubexpression<0, 4>  },
	{ nullptr, &RevSymbolicEnvironment::SetSubexpression<1, 1>,  &RevSymbolicEnvironment::SetSubexpression<1, 2>,  &RevSymbolicEnvironment::SetSubexpressionOffM<3>,  &RevSymbolicEnvironment::SetSubexpressionInvalid },
	{ nullptr, &RevSymbolicEnvironment::SetSubexpression<2, 1>,  &RevSymbolicEnvironment::SetSubexpressionOffM<2>,  &RevSymbolicEnvironment::SetSubexpressionInvalid, &RevSymbolicEnvironment::SetSubexpressionInvalid },
	{ nullptr, &RevSymbolicEnvironment::SetSubexpressionOffM<1>, &RevSymbolicEnvironment::SetSubexpressionInvalid, &RevSymbolicEnvironment::SetSubexpressionInvalid, &RevSymbolicEnvironment::SetSubexpressionInvalid }
};

void RevSymbolicEnvironment::SetExpression(void *exp, rev::DWORD address, rev::DWORD size, rev::DWORD *values) {
	static const rev::DWORD sizes[3] = { 4, 2, 1 };
	rev::DWORD sz = sizes[RIVER_OPSIZE(size)];
	rev::DWORD osz = sz;
	rev::DWORD ptr = 0;

	while (sz) {
		rev::DWORD fa = address & (~0x03), fo = address & 0x03;
		rev::DWORD copy = 4 - fo;

		if (sz < copy) {
			copy = sz;
		}

		void *ext = (osz == copy) ? exp : exec->ExtractBits(exp, (sz - copy) << 3, copy << 3);

		void *val = (void *)TrackAddr(pEnv, fa, 0);
		if (nullptr == val) {
			val = exec->MakeConst(values[ptr], 32);
		}


		(this->*subExpsSet[fo][sz])(ext, address, val);
		ptr++;

		address += copy;
		sz -= copy;
	}
}

bool RevSymbolicEnvironment::SetCurrentInstruction(RiverInstruction *rIn, void *context) {
	opBase = (rev::DWORD *)context;
	current = rIn;

	GetOperandLayout(*current);
	return true;
}

void RevSymbolicEnvironment::PushState(stk::LargeStack &stack) { }
void RevSymbolicEnvironment::PopState(stk::LargeStack &stack) { }

bool RevSymbolicEnvironment::GetOperand(rev::BYTE opIdx, rev::BOOL &isTracked, rev::DWORD &concreteValue, void *&symbolicValue) {
	void *symExpr;

	if (RIVER_SPEC_IGNORES_OP(opIdx) & current->specifiers) {
		return false;
	}

	switch (RIVER_OPTYPE(current->opTypes[opIdx])) {
	case RIVER_OPTYPE_NONE:
		return false;

	case RIVER_OPTYPE_IMM:
		isTracked = false;
		switch (RIVER_OPSIZE(current->opTypes[opIdx])) {
		case RIVER_OPSIZE_32:
			concreteValue = current->operands[opIdx].asImm32;
			break;
		case RIVER_OPSIZE_16:
			concreteValue = current->operands[opIdx].asImm16;
			break;
		case RIVER_OPSIZE_8:
			concreteValue = current->operands[opIdx].asImm8;
			break;
		}
		return true;

	case RIVER_OPTYPE_REG:
		symExpr = (void *)((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[GetFundamentalRegister(current->operands[opIdx].asRegister.name)];

		isTracked = (symExpr != NULL);
		symbolicValue = symExpr;
		concreteValue = opBase[-(valueOffsets[opIdx])];
		return true;

	case RIVER_OPTYPE_MEM:
		// how do we handle dereferenced symbolic values?

		if (0 == current->operands[opIdx].asAddress->type) {
			symExpr = (void *)((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[GetFundamentalRegister(current->operands[opIdx].asAddress->base.name)];

			isTracked = (symExpr != NULL);
			symbolicValue = symExpr;
			concreteValue = opBase[-(valueOffsets[opIdx])];
			return true;
		}

		if (RIVER_SPEC_IGNORES_MEMORY & current->specifiers) {
			return false; // for now!
		}

		//symExpr = get from opBase[opOffset[opIdx]]

		if (opBase[-(addressOffsets[opIdx])] < 0x1000) {
			__asm int 3;
		}

		symExpr = GetExpression(opBase[-(addressOffsets[opIdx])], RIVER_OPSIZE(current->opTypes[opIdx])); //(void *)TrackAddr(pEnv, opBase[-(addressOffsets[opIdx])], 0);
		isTracked = (symExpr != NULL);
		symbolicValue = symExpr;
		concreteValue = opBase[-(valueOffsets[opIdx])];
		return true;
	}

	return false;
}

unsigned int BinLog2(unsigned int v) {
	register unsigned int r; // result of log2(v) will go here
	register unsigned int shift;

	r = (v > 0xFFFF) << 4; v >>= r;
	shift = (v > 0xFF) << 3; v >>= shift; r |= shift;
	shift = (v > 0x0F) << 2; v >>= shift; r |= shift;
	shift = (v > 0x03) << 1; v >>= shift; r |= shift;
	r |= (v >> 1);

	return r;
}

unsigned int flagShifts[] = {
	0, //RIVER_SPEC_FLAG_CF
	2, //RIVER_SPEC_FLAG_PF
	4, //RIVER_SPEC_FLAG_AF
	6, //RIVER_SPEC_FLAG_ZF
	7, //RIVER_SPEC_FLAG_SF
	11, //RIVER_SPEC_FLAG_OF
	10 //RIVER_SPEC_FLAG_DF
};

bool RevSymbolicEnvironment::GetFlgValue(rev::BYTE flg, rev::BOOL &isTracked, rev::BYTE &concreteValue, void *&symbolicValue) {
	if (0 == (flg & current->testFlags)) {
		return false;
	}

	unsigned int flgIdx = BinLog2(flg);
	//void *expr = pEnv->runtimeContext.taintedFlags[flgIdx];

	rev::DWORD val = ((ExecutionEnvironment *)pEnv)->runtimeContext.taintedFlags[flgIdx];

	isTracked = (0 != val);
	concreteValue = (opBase[flagOffset] >> flagShifts[flgIdx]) & 1;
	if (isTracked) {
		symbolicValue = (void *)val;
	}

	return true;
}

bool RevSymbolicEnvironment::SetOperand(rev::BYTE opIdx, void *symbolicValue) {
	switch (RIVER_OPTYPE(current->opTypes[opIdx])) {
	case RIVER_OPTYPE_NONE:
	case RIVER_OPTYPE_IMM:
		return false;

	case RIVER_OPTYPE_REG:
		((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[GetFundamentalRegister(current->operands[opIdx].asRegister.name)] = (rev::DWORD)symbolicValue;
		return true;

	case RIVER_OPTYPE_MEM:
		if (0 == current->operands[opIdx].asAddress->type) {
			((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[GetFundamentalRegister(current->operands[opIdx].asAddress->base.name)] = (rev::DWORD)symbolicValue;
		}
		else {
			//MarkAddr(pEnv, opBase[-(addressOffsets[opIdx])], (rev::DWORD)symbolicValue, 0);
			SetExpression(symbolicValue, opBase[-(addressOffsets[opIdx])], RIVER_OPSIZE(current->opTypes[opIdx]), &opBase[-(valueOffsets[opIdx])]);
		}
		return true;

	}

	return false;
}

bool RevSymbolicEnvironment::UnsetOperand(rev::BYTE opIdx) {
	return SetOperand(opIdx, nullptr);
}

void RevSymbolicEnvironment::SetFlgValue(rev::BYTE flg, void *symbolicValue) {
	unsigned int flgIdx = BinLog2(flg);

	((ExecutionEnvironment *)pEnv)->runtimeContext.taintedFlags[flgIdx] = (rev::DWORD)symbolicValue;
}

void RevSymbolicEnvironment::UnsetFlgValue(rev::BYTE flg) {
	SetFlgValue(flg, nullptr);
}

void RevSymbolicEnvironment::SetSymbolicVariable(const char *name, rev::ADDR_TYPE addr, rev::DWORD size) {
	rev::DWORD buff[2];
	ctrl->ReadProcessMemory((unsigned int)addr & ~0x03, 8, (unsigned char *)buff);

	rev::DWORD oSize = 0;

	switch (size) {
	case 4:
		oSize = RIVER_OPSIZE_32;
		break;
	case 2:
		oSize = RIVER_OPSIZE_16;
		break;
	case 1 : 
		oSize = RIVER_OPSIZE_8;
		break;
	default :
		__asm int 3;
	}

	SetExpression(
		exec->CreateVariable(name, size),
		(rev::DWORD)addr,
		oSize,
		buff
	);
}
