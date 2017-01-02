/* 2016.12.22 ��д */

#ifndef __ASMPARSER_H__
#define __ASMPARSER_H__

//#define DEBUG
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include "strutil.h"

#define ERROR_NOTFOUND (-1)
#define ERROR_SYNTAX (-2)

#define INST_TYPE_R0	10 // R��ָ�� @label:MMN
#define INST_TYPE_R1	11 // R��ָ�� @label:MMN $1
#define INST_TYPE_R2	12 // R��ָ�� @label:MMN $1, $2
#define INST_TYPE_R3	13 // R��ָ�� @label:MMN $1, $2, $3
#define INST_TYPE_RS	14 // R��ָ�� @label:MMN $1, $2, shamt(0~31dec)

#define INST_TYPE_I0	20 // I��ָ��(�����˺겻��)
#define INST_TYPE_I1	21 // I��ָ�� @label:MMN $1, imme
#define INST_TYPE_I2	22 // I��ָ�� @label:MMN $1, $2, imme
#define INST_TYPE_I1B	23 // I��ָ�� @label:MMN $1, imme/@label
#define INST_TYPE_I2B	24 // I��ָ�� @label:MMN $1, $2, imme/@label
#define INST_TYPE_IR	25 // I��ָ�� @label:MMN $1, imme($2) ��immeΪ16bit����������֧��ʮ���ƺ�ʮ�����ơ�

#define INST_TYPE_J		30 // J��ָ�� @label:MMN [addr|label] ��addrΪ26bit�����޷�������֧��ʮ���ƺ�ʮ������ labelΪ@[a-zA-Z][a-zA-Z0-9]*��

#define PSEUDO_DATA		40 // αָ�� .data [addr] ��addrΪ32bit�����޷�������֧��ʮ���ƺ�ʮ�����ơ� 
#define PSEUDO_TEXT		50 // αָ�� .text [addr] ��addrΪ32bit�����޷�������֧��ʮ���ƺ�ʮ�����ơ� 
#define PSEUDO_SPACE	60 // αָ�� .space num ��numΪ32bit����ʮ������������ 

#define VAR_DEF			70 // �������� var .type value:dup ��varΪ[a-zA-Z][a-zA-Z0-9]* typeΪ[byte|half|word] valueΪ��Ӧ���ͣ�֧��ʮ���ƺ�ʮ������ dupΪ32bit����ʮ������������

#define COMMENT			80
#define NULL_LINE		90

//��������
#define DATATYPE_WORD	10
#define DATATYPE_HALF	20
#define DATATYPE_BYTE	30

// �����ű��ļ���
// �����ű�����ⲿ�ļ�����ű��ļ���ʽ��
//   @label1=0xabcd1234
//   @label2=0x12345666
//   var1=0x12345678
//   var2=-123
//   ...
FILE *idTable;

extern int count;

// ��ָ�����Ƿ��б�ָ��������ָ�����͡�
#define INSTRUCTION_NUMBER 62
static char MemonicList[][10] = {
// R��ָ�31��
	"add",        "addu",       "sub",        "subu",       "and",        "or",         "xor",        "nor",\
	"slt",        "sltu",       "sll",        "srl",        "sra",        "sllv",       "srlv",       "srav",\
	"mult",       "multu",      "div",        "divu",       "mfhi",       "mflo",       "mthi",       "mtlo",\
	"mfc0",       "mtc0",       "jr",         "jalr",       "break",      "syscall",    "eret",\
// I��ָ�24��
	"addi",       "addiu",      "andi",       "ori",        "xori",       "lui",        "lb",         "lbu",\
	"lh",         "lhu",        "sb",         "sh",         "lw",         "sw",         "beq",        "bne",\
	"bgez",       "bgtz",       "blez",       "bltz",       "bgezal",     "bltzal",     "slti",       "sltiu",\
// J��ָ�3��
	"j",          "jal",        "nop",\
// ��ָ��
	"push",       "pop",        "pushall",    "popall"
};
static int MemonicType[] = {
// R��ָ��
	INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_R3,\
	INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_RS, INST_TYPE_RS, INST_TYPE_RS, INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_R3,\
	INST_TYPE_R2, INST_TYPE_R2, INST_TYPE_R2, INST_TYPE_R2, INST_TYPE_R1, INST_TYPE_R1, INST_TYPE_R1, INST_TYPE_R1,\
	INST_TYPE_R2, INST_TYPE_R2, INST_TYPE_R1, INST_TYPE_R2, INST_TYPE_R0, INST_TYPE_R0, INST_TYPE_R0,\
// I��ָ��
	INST_TYPE_I2, INST_TYPE_I2, INST_TYPE_I2, INST_TYPE_I2, INST_TYPE_I2, INST_TYPE_I1, INST_TYPE_IR, INST_TYPE_IR,\
	INST_TYPE_IR, INST_TYPE_IR, INST_TYPE_IR, INST_TYPE_IR, INST_TYPE_IR, INST_TYPE_IR, INST_TYPE_I2B,INST_TYPE_I2B,\
	INST_TYPE_I1B,INST_TYPE_I1B,INST_TYPE_I1B,INST_TYPE_I1B,INST_TYPE_I1B,INST_TYPE_I1B,INST_TYPE_I2, INST_TYPE_I2,\
// J��ָ��
	INST_TYPE_J , INST_TYPE_J , INST_TYPE_J,
// ��ָ��
	INST_TYPE_R1, INST_TYPE_R1, INST_TYPE_R0, INST_TYPE_R0
};
static unsigned char OperBinary[] = {
// R��ָ���[30]eretָ�����Ϊ0x00��
	0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,\
	0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,\
	0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,\
	0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x10,\
// I��ָ��
	0x08,         0x09,         0x0c,         0x0d,         0x0e,         0x0f,         0x20,         0x24,\
	0x21,         0x25,         0x28,         0x29,         0x23,         0x2b,         0x04,         0x05,\
	0x01,         0x07,         0x06,         0x01,         0x01,         0x01,         0x0a,         0x0b,\
// J��ָ��
	0x02,         0x03,         0x00,
// ��ָ��
	0xff,         0xff,         0xff,         0xff
};
static unsigned char FuncBinary[] = {
// R��ָ��
	0x20,         0x21,         0x22,         0x23,         0x24,         0x25,         0x26,         0x27,\
	0x2a,         0x2b,         0x00,         0x02,         0x03,         0x04,         0x06,         0x07,\
	0x18,         0x19,         0x1a,         0x1b,         0x10,         0x12,         0x11,         0x13,\
	0x00,         0x00,         0x08,         0x09,         0x0d,         0x0c,         0x18,\
// I��ָ��ֶ������壬��Ϊ0xff��
	0xff,         0xff,         0xff,         0xff,         0xff,         0xff,         0xff,         0xff,\
	0xff,         0xff,         0xff,         0xff,         0xff,         0xff,         0xff,         0xff,\
	0xff,         0xff,         0xff,         0xff,         0xff,         0xff,         0xff,         0xff,\
// J��ָ��ֶ������壬��Ϊ0xff��
	0xff,         0xff,         0xff,
// ��ָ��
	0xff,         0xff,         0xff,         0xff
};

// ���Ĵ�������
static char RegisterName[][10] = {
	"$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",\
	"$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
};
// ���Ĵ����š�
static char RegisterNumber[][10] = {
	"$0", "$1", "$2", "$3", "$4", "$5", "$6", "$7", "$8", "$9", "$10", "$11", "$12", "$13", "$14", "$t15",\
	"$16", "$17", "$18", "$19", "$20", "$21", "$22", "$23", "$24", "$25", "$26", "$27", "$28", "$29", "$30", "$31"
};
// ������ͬ��������ʱ�ַ�����
static char IDTemp[100][50];
static int IDCount = 0;

// ����ȡ״̬��
#define SECTION_DATA 10
#define SECTION_TEXT 20
// ȫ��״̬��ǣ�ָʾĿǰ�����������ݶλ��Ǵ����
//   ��flag��������INST��VAR_DEF�������Ľ���
static int sectionFlag = 0;


/*
 * ȡ���Ƿ�
 * ���룺line
 * �����memonic
 * ���أ�0
 */
int GetMemonic(char *line, char *memonic)
{
	int a = getFirstIndex(line);
	int b = 0;
	for(int i = a; i < (int)strlen(line); i++)
	{
		if( (line[i]>='A' && line[i]<='Z' || line[i]>='a' && line[i]<='z' || line[i] == '0') && 
			(line[i+1] == ' ' || line[i+1] == '\t' || line[i+1] == 0 || line[i+1] == '\n'))
		{
			b = i;
			break;
		}
	}
	strmid(line, a, b, memonic);
	return 0;
}
/*
 * ȡ��ǩ
 * ���룺line
 * �����label����@��ͷ��
 * ���أ��б�ǩ����0���ޱ�ǩ����1
 */
int GetLabel(char *line, char *label)
{
	//�±����˵��
	//   @label:   add  $t1  ,  $t2  ,  $t3   # vvv
	//   a     b   c
	int a = getFirstIndexOverBlank(line, 0); //��һ����Ч�ַ����ǿո��ַ����±�
	int b = 0;
	//�����һ����Ч�ַ���Ϊ@�����ȡ֮
	if(line[a] == '@') {
		b = getIndex(line, ':', a); //ȡ��:�±�
		strmid(line, a, b-1, label);
		return 0;
	}
	else{
		return (-1);
	}
}
/*
 * ����������Ƿ����
 * ���룺line
 * �����ú�����GetMemonic
 * ���أ����Ƿ�index
 */
int GetMemonicIndex(char *line) {
	char memonic[10];
	// ȡ�����Ƿ�
	GetMemonic(line, memonic);
	// �������Ƿ����
	for(int i = 0; i < INSTRUCTION_NUMBER; i++) {
		if(!_stricmp(memonic, MemonicList[i])) {
			return i;
		}
	}
	// ���δ�ҵ�������-1
	printf("ERROR[%u] : Memonic not found.\n", count);
	return ERROR_NOTFOUND;
}

/*
 * ����������ͣ������޸�sectionFlag��
 * ���룺line
 * �����ú�����GetMemonic
 * ���أ�0
 */
int GetInstructionType(char *line) {
	int first = getFirstIndexOverBlank(line, 0);
	if(line[first] == '\0') {
		return NULL_LINE;
	}
	if(line[first] == '.') {
		char token[10];
		int to = getIndexBeforeBlank(line, first);
		strmid(line, first+1, to, token);
		if(!_stricmp(token, "data")) {
			sectionFlag = SECTION_DATA;
			return PSEUDO_DATA;
		}
		else if(!_stricmp(token, "text")) {
			sectionFlag = SECTION_TEXT;
			return PSEUDO_TEXT;
		}
		else if(!_stricmp(token, "space")) {
			return PSEUDO_SPACE;
		}
		else {
			return ERROR_NOTFOUND;
		}
	}
	if(line[first] == '#') {
		return COMMENT;
	}
	// ����Σ�������Ϊָ��
	if(sectionFlag == SECTION_TEXT) {
		char memonic[10];
		int memonicIndex = -1;
		// ȡ�����Ƿ�
		GetMemonic(line, memonic);
		// �������Ƿ����
		for(int i = 0; i < INSTRUCTION_NUMBER; i++) {
			if(!_stricmp(memonic, MemonicList[i])) {
				memonicIndex = i;
				break;
			}
		}
		// ����ҵ��ˣ���������
		if(memonicIndex != -1) {
			return MemonicType[memonicIndex];
		}
		// ���δ�ҵ�������-1
		else {
			printf("ERROR[%u] : Memonic not found.\n", count);
			return ERROR_NOTFOUND;
		}
	}
	// ����Σ�������Ϊ��������
	else if(sectionFlag == SECTION_DATA)
	{
		if(line[first] >= 'a' && line[first] <= 'z' || line[first] >= 'A' && line[first] <= 'Z' || line[first] == '_') {
			return VAR_DEF;
		}
		else {
			return ERROR_SYNTAX; //��ʶ���﷨���󣨲������֡��»��߻���ĸ��ͷ��
		}
	}
	else {
		return -99;//������
	}
	return ERROR_NOTFOUND;
}

/*
 * ��ָ������ȡ���Ƿ���Ӧ��oper�������ַ���
 * ���룺line��ȫ��ָ�
 * �����operbin��6λ��
 * ���أ���������0
 */
int GetOperBinCode(char *line, char *operbin) {
	char memonic[10];
	int memonicIndex = -1;
	uint32 oper = 0;
	char hex[20];
	//�������Ƿ�index
	GetMemonic(line, memonic);
	for(int i = 0; i < INSTRUCTION_NUMBER; i++) {
		if(!_stricmp(memonic, MemonicList[i])) {
			memonicIndex = i;
		}
	}
	//����Ƿ��ҵ�
	if(memonicIndex < 0) {
		printf("ERROR[%u] : Memonic not found.\n", count);
		return ERROR_NOTFOUND;
	}
	//��oper��ת���ɶ��������
	else {
		oper = (uint32)OperBinary[memonicIndex];
		UDecToHexStr(oper, hex, 8);
		HexStrToBinStr(hex, operbin, 6);
		return 0;
	}
	return (-1);
}

/*
 * ��ָ������ȡ���Ƿ���Ӧ��func�������ַ���
 * ���룺line��ȫ��ָ�
 * �����funcbin��6λ��
 * ���أ���������0
 */
int GetFuncBinCode(char *line, char *funcbin) {
	char memonic[10];
	int memonicIndex = -1;
	uint32 oper = 0;
	char hex[20];
	//�������Ƿ�index
	GetMemonic(line, memonic);
	for(int i = 0; i < INSTRUCTION_NUMBER; i++) {
		if(!_stricmp(memonic, MemonicList[i])) {
			memonicIndex = i;
		}
	}
	//����Ƿ��ҵ�
	if(memonicIndex < 0) {
		printf("ERROR[%u] : Memonic not found.\n", count);
		return ERROR_NOTFOUND;
	}
	//��oper��ת���ɶ��������
	else {
		oper = (uint32)FuncBinary[memonicIndex];
		UDecToHexStr(oper, hex, 8);
		HexStrToBinStr(hex, funcbin, 6);
		return 0;
	}
	return (-1);
}

/*
 * ��ָ������ȡ��һ�Ĵ�������������������
 * ���룺line����R��I��ָ�����������飩
 * �����reg1bin��5λ��
 * ���أ���������0
 */
int GetReg1BinCode(char *line, char *reg1bin)
{
	//�±����˵��
	//   @label:   add  $t1  ,  $t2  ,  $t3   # vvv
	//                  a b  c
	int a = getIndex(line, '$', 0);        //��һ��'$'�±�
	int b = getIndexBeforeBlank(line, a);  //�Ĵ��������һ���ַ����±�(�ո�ǰ��Ч�ַ�)
	int c = getIndex(line, ',', a);        //�����±�
	char reg[10];
	char hex[20];
	//�﷨���
	int len = (int)strlen(line);
	if(a == len) {
		printf("ERROR[%u] : Missing '$'. More operand needed probably.\n", count);
		return ERROR_SYNTAX;
	}
	//��ȡ�Ĵ�����
	//û�ж��ţ�������R1��ָ��
	if(c == len) {
		strmid(line, a, b, reg);
	}
	else{
		//����ǰû�пո� ���߸���û�ж���
		if(b >= c) {
			strmid(line, a, c-1, reg);
		}
		//����ǰ�пո�
		else {
			strmid(line, a, b, reg);
		}
	}
	//����Ĵ�����
	for(int i = 0; i < 32; i++) {
		if( !_stricmp(reg, RegisterName[i]) ) {
			UDecToHexStr((uint32)i, hex, 8);
			HexStrToBinStr(hex, reg1bin, 5);
			return 0;
		}
	}
	//����Ĵ�����
	for(int i = 0; i < 32; i++) {
		if( !_stricmp(reg, RegisterNumber[i]) ) {
			UDecToHexStr((uint32)i, hex, 8);
			HexStrToBinStr(hex, reg1bin, 5);
			return 0;
		}
	}
	printf("ERROR[%u] : Register name not found.\n", count);
	return ERROR_NOTFOUND;
}
/*
 * ��ָ������ȡ��2�Ĵ�������������������
 * ���룺line����R��I��ָ�����������飩
 * �����reg2bin��5λ��
 * ���أ���������0
 */
int GetReg2BinCode(char *line, char *reg2bin)
{
	//�±����˵��
	//   @label:   add  $t1  ,  $t2  ,  $t3   # vvv
	//                       p  a b  c
	int p = getIndex(line, ',', 0);        //��һ��','�±�
	int a = getIndex(line, '$', p);        //�ڶ���'$'�±�
	int b = getIndexBeforeBlank(line, a);  //�Ĵ��������һ���ַ����±�(�ո�ǰ��Ч�ַ�)
	int c = getIndex(line, ',', a);
	char reg[10];
	char hex[20];
	//�﷨���
	int len = (int)strlen(line);
	if(p == len || a == len) {
		printf("ERROR[%u] : Missing ',' and/or '$'. More operand needed probably.\n", count);
		return ERROR_SYNTAX;
	}
	//��ȡ�Ĵ�����
	//û�еڶ������ţ�������R2��ָ��
	if(c == len) {
		strmid(line, a, b, reg);
	}
	else{
		//����ǰû�пո� ���߸���û�ж���
		if(b >= c) {
			strmid(line, a, c-1, reg);
		}
		//����ǰ�пո�
		else {
			strmid(line, a, b, reg);
		}
	}
	//����Ĵ�����
	for(int i = 0; i < 32; i++) {
		if( !_stricmp(reg, RegisterName[i]) ) {
			UDecToHexStr((uint32)i, hex, 8);
			HexStrToBinStr(hex, reg2bin, 5);
			return 0;
		}
	}
	//����Ĵ�����
	for(int i = 0; i < 32; i++) {
		if( !_stricmp(reg, RegisterNumber[i]) ) {
			UDecToHexStr((uint32)i, hex, 8);
			HexStrToBinStr(hex, reg2bin, 5);
			return 0;
		}
	}
	printf("ERROR[%u] : Register name not found.\n", count);
	return ERROR_NOTFOUND;
}
/*
 * ��ָ������ȡ��3�Ĵ�������������������
 * ���룺line����R��ָ�����������飩
 * �����reg3bin��5λ��
 * ���أ���������0
 */
int GetReg3BinCode(char *line, char *reg3bin)
{
	//�±����˵��
	//   @label:   add  $t1  ,  $t2  ,  $t3   # vvv
	//                       p       p  a b
	int p = getIndex(line, ',', 0);        //��һ��','�±�
	    p = getIndex(line, ',', p+1);        //�ڶ���','�±�
	int a = getIndex(line, '$', p);        //������'$'�±�
	int b = getIndexBeforeBlank(line, a);  //�Ĵ��������һ���ַ����±�(�ո�ǰ��Ч�ַ�)
	char reg[10];
	char hex[20];
	//�﷨���
	int len = (int)strlen(line);
	if(p == len || a == len || b == len) {
		printf("ERROR[%u] : Missing ',' and/or '$'. More operand needed probably.\n", count);
		return ERROR_SYNTAX;
	}
	//��ȡ�Ĵ�����
	strmid(line, a, b, reg);
	//����Ĵ�����
	for(int i = 0; i < 32; i++) {
		if( !_stricmp(reg, RegisterName[i]) ) {
			UDecToHexStr((uint32)i, hex, 8);
			HexStrToBinStr(hex, reg3bin, 5);
			return 0;
		}
	}
	//����Ĵ�����
	for(int i = 0; i < 32; i++) {
		if( !_stricmp(reg, RegisterNumber[i]) ) {
			UDecToHexStr((uint32)i, hex, 8);
			HexStrToBinStr(hex, reg3bin, 5);
			return 0;
		}
	}
	printf("ERROR[%u] : Register name not found.\n", count);
	return ERROR_NOTFOUND;
}
/*
 * ��ָ������ȡ������
 * ���룺line������R��I��ָ�����������飩
 * �����imme���ַ��������������������������ǩ��
 * ���أ���������0
 */
int GetImme(char *line, char *imme)
{
	int len = (int)strlen(line);
	int a = 0;
	int b = 0;
	int p = getIndex(line, ',', 0); //��һ��','�±�
	int pp = 0; //��2��','�±�
	int leftb = 0; //�������±�
	//��鶺�ŷָ������ж��������
	//���е�һ�����ţ�������I1��I2��RS��IR
	if(p != len) {
	    pp = getIndex(line, ',', p+1); //���ڶ���','�±�
		//���еڶ������ţ���ΪI2��RS����������ͬ
		//mem $1 , $2 , imme
		//            p a  b
		if(pp != len) {
			a = getFirstIndexOverBlank(line, pp+1);   //imme�ֶ���ʼ�±�
			b = getIndexBeforeBlank(line, a);      //imme���һ���ַ����±�(�ո�ǰ��Ч�ַ�)
			//��ȡimme
			strmid(line, a, b, imme);
			return 0;
		}
		//���޵ڶ������ţ���ΪI1��IR
		else if(pp == len) {
			leftb = getIndex(line, '(', 0); //�������±�
			//���������ţ�����Ϊ��IR
			//mem $1 , imme ( $2 )
			//       p a  b ^leftb
			if(leftb != len) {
				a = getFirstIndexOverBlank(line, p+1);   //imme�ֶ���ʼ�±�
				b = getIndexBeforeBlank(line, a);      //imme���һ���ַ����±�(�ո�ǰ��Ч�ַ�)
				//���imme��(��û�пո�
				if(b >= leftb) {
					strmid(line, a, leftb-1, imme);
					return 0;
				}
				//���imme��(���пո�
				else{
					strmid(line, a, b, imme);
					return 0;
				}
			}
			//���������ţ�����Ϊ��I1
			//mem $1 , imme
			//       p a  b
			else {
				a = getFirstIndexOverBlank(line, p+1);   //imme�ֶ���ʼ�±�
				b = getIndexBeforeBlank(line, a);      //imme���һ���ַ����±�(�ո�ǰ��Ч�ַ�)
				//��ȡimme
				strmid(line, a, b, imme);
				return 0;
			}
		}
	}
	//���޵�һ�����ţ��������J
	//mem imme
	//    a  b
	else if(p == len){
		a = getFirstIndexOverBlank(line, 0);   //��һ����Ч�ַ����±�
		//����б�ǩ
		if(line[a] == '@') {
			a = getIndex(line, ':', a+1);
			a = getFirstIndexOverBlank(line, a+1); //memonic�ֶ���ʼ�±�
		}
		a = getIndexBeforeBlank(line, a);   //imme�ֶ���ʼ�±�
		a = getFirstIndexOverBlank(line, a+1);   //imme�ֶ���ʼ�±�
		b = getIndexBeforeBlank(line, a);      //imme���һ���ַ����±�(�ո�ǰ��Ч�ַ�)
		//��ȡimme
		strmid(line, a, b, imme);
		return 0;
	}
	printf("ERROR[%u] : Register name not found.\n", count);
	return ERROR_NOTFOUND;
}
/*
 * ��IR��ָ������ȡ��ַ�Ĵ��������Ʊ���
 * ���룺line��IR��ָ�����������飩
 * �����baseregbin���ַ��������������������������ǩ��
 * ���أ���������0
 */
int GetBaseAddrRegBinCode(char *line, char *baseregbin) {
	//�±����˵��
	//   @label:   add  $t1  ,  ADDR ( $base )   # vvv
	//                               p a   b c
	int len = (int)strlen(line);
	int p = getIndex(line, '(', 0);           //������λ��
	int a = getFirstIndexOverBlank(line, p+1); //�Ĵ�������һ����Ч�ַ����±�
	int b = getIndexBeforeBlank(line, a);      //���һ����Ч�ַ����±�
	int c = getIndex(line, ')', p);           //������λ��
	char reg[20];
	char hex[50];
	//�﷨���
	if(p == len || c == len) {
		printf("ERROR[%u] : Missing '(' and/or ')'.\n", count);
		return ERROR_SYNTAX;
	}
	//��ȡ�Ĵ�����
	//���$base��)��û�пո�
	if(b >= c) {
		strmid(line, a, c-1, reg);
	}
	//���$base��)���пո�
	else {
		strmid(line, a, b, reg);
	}
	//����Ĵ�����
	for(int i = 0; i < 32; i++) {
		if( !_stricmp(reg, RegisterName[i]) ) {
			UDecToHexStr((uint32)i, hex, 8);
			HexStrToBinStr(hex, baseregbin, 5);
			return 0;
		}
	}
	//����Ĵ�����
	for(int i = 0; i < 32; i++) {
		if( !_stricmp(reg, RegisterNumber[i]) ) {
			UDecToHexStr((uint32)i, hex, 8);
			HexStrToBinStr(hex, baseregbin, 5);
			return 0;
		}
	}
	printf("ERROR[%u] : BaseAddr Register name not found.\n", count);
	return ERROR_NOTFOUND;
}





//����ʶ���ظ��Լ�顿
int RepeatCheck(char *id) {
	char idTemp[50];
	do {
		fscanf(idTable, "%s", idTemp);
		if( !_stricmp(id, idTemp) ) {
			
			exit(-1);
			return 0;
		}
	}while(_stricmp("\n", idTemp));
	return (-1);
}
int WriteIdentifier(char *id) {
	for(int i = 0; i < IDCount; i++) {
		if( !_stricmp(id, IDTemp[i]) ) {
			printf("ERROR[%u] : Identifier Collision.\n", count);
			exit(-1);
			return (-1);
		}
	}
	fputs(id, idTable);
	strcpy(IDTemp[IDCount++], id);
	return 0;
}
#endif