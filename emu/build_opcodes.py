from collections import namedtuple
"""
typedef struct OpcodeOperand {
	int name;
	//int  bytes;
	//int  immediate;
	//int  increment;
	//int  decrement;
} OpcodeOperand;

typedef struct OpcodeDef {
	char mnemonic[20];
	int num_bytes;
	//int cycle_steps;
	//int cycles[2];
	int num_operands;
	OpcodeOperand operands[3];
	//int immediate;
	//char flag_z;
	//char flag_n;
	//char flag_h;
	//char flag_c;
} OpcodeDef;
"""

Opcode = namedtuple(
        'Opcode', [
            'mnemonic',
            'num_bytes',
            'operands',])

Operand = namedtuple(
        'Operand', [
            'name',])

opcodes = [None] * 0x100


opcodes[0x43] = Opcode(
        mnemonic='inc',
        num_bytes=1,
        operands=[
            Operand('bx'),
            ]
        )

opcodes[0x48] = Opcode(
        mnemonic='dec',
        num_bytes=1,
        operands=[
            Operand('ax'),
            ]
        )

opcodes[0x53] = Opcode(
        mnemonic='push',
        num_bytes=1,
        operands=[
            Operand('bx'),
            ]
        )

opcodes[0x5b] = Opcode(
        mnemonic='pop',
        num_bytes=1,
        operands=[
            Operand('bx'),
            ]
        )

opcodes[0x74] = Opcode(
        mnemonic='jz',
        num_bytes=2,
        operands=[
            Operand('e8'),
            ]
        )

opcodes[0x84] = Opcode(
        mnemonic='test',
        num_bytes=1, # TODO
        operands=[
            Operand('?'),
            Operand('?'),
            ]
        )

opcodes[0x8a] = Opcode(
        mnemonic='mov',
        num_bytes=2,
        operands=[
            Operand('al'),
            Operand('[bx]'),
            ]
        )

opcodes[0xb4] = Opcode(
        mnemonic='mov',
        num_bytes=2,
        operands=[
            Operand('ah'),
            Operand('n8'),
            ]
        )


opcodes[0xbb] = Opcode(
        mnemonic='mov',
        num_bytes=3,
        operands=[
            Operand('bx'),
            Operand('n16')
            ]
        )

opcodes[0xcd] = Opcode(
        mnemonic='int',
        num_bytes=2,
        operands=[
            Operand('n8')
            ]
        )

opcodes[0xeb] = Opcode(
        mnemonic='jmp short',
        num_bytes=2,
        operands=[
            Operand('e8')
            ]
        )

print(';')
print('OpcodeDef opcodes[0x100] = {')

lines = []

for i, opcode in enumerate(opcodes):
    if opcode is None:
        #continue
        s = f'''
        {{
            .mnemonic  = "? 0x{i:02x}",
            .num_bytes = 1,
            .num_operands = 0
        }}
        '''
        lines.append(s)
        continue
    #print((f"0x{i:02x}", opcode))
           # {{.name = 'bx'}},
           # {{.name = 'n16'}}

    operand_lines = [f"{{.name='{operand.name}'}}" for operand in opcode.operands]
    operands = ',\n            '.join(operand_lines)

    num_operands = len(opcode.operands)

    s = f'''
    {{
        .mnemonic  = "{opcode.mnemonic}",
        .num_bytes = {opcode.num_bytes},
        .num_operands = {num_operands},
        .operands  = {{
            {operands}
        }}
    }}
    '''
    lines.append(s)


print(',\n'.join(lines))

print('};')

