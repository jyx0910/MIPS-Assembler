/* 2016.12.22 重写 */

#ifndef __ASMPARSER_H__
#define __ASMPARSER_H__

//#define DEBUG
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include "strutil.h"

#define ERROR_NOTFOUND (-1)
#define ERROR_SYNTAX (-2)

#define INST_TYPE_R0	10 // R型指令 @label:MMN
#define INST_TYPE_R1	11 // R型指令 @label:MMN $1
#define INST_TYPE_R2	12 // R型指令 @label:MMN $1, $2
#define INST_TYPE_R3	13 // R型指令 @label:MMN $1, $2, $3
#define INST_TYPE_RS	14 // R型指令 @label:MMN $1, $2, shamt(0~31dec)

#define INST_TYPE_I0	20 // I型指令(保留此宏不用)
#define INST_TYPE_I1	21 // I型指令 @label:MMN $1, imme
#define INST_TYPE_I2	22 // I型指令 @label:MMN $1, $2, imme
#define INST_TYPE_I1B	23 // I型指令 @label:MMN $1, imme/@label
#define INST_TYPE_I2B	24 // I型指令 @label:MMN $1, $2, imme/@label
#define INST_TYPE_IR	25 // I型指令 @label:MMN $1, imme($2) 【imme为16bit以内整数，支持十进制和十六进制】

#define INST_TYPE_J		30 // J型指令 @label:MMN [addr|label] 【addr为26bit以内无符号数，支持十进制和十六进制 label为@[a-zA-Z][a-zA-Z0-9]*】

#define PSEUDO_DATA		40 // 伪指令 .data [addr] 【addr为32bit以内无符号数，支持十进制和十六进制】 
#define PSEUDO_TEXT		50 // 伪指令 .text [addr] 【addr为32bit以内无符号数，支持十进制和十六进制】 
#define PSEUDO_SPACE	60 // 伪指令 .space num 【num为32bit以内十进制正整数】 

#define VAR_DEF			70 // 变量定义 var .type value:dup 【var为[a-zA-Z][a-zA-Z0-9]* type为[byte|half|word] value为相应类型，支持十进制和十六进制 dup为32bit以内十进制正整数】

#define COMMENT			80
#define NULL_LINE		90

//数据类型
#define DATATYPE_WORD	10
#define DATATYPE_HALF	20
#define DATATYPE_BYTE	30

// 【符号表文件】
// 将符号表放在外部文件里，符号表文件格式：
//   @label1=0xabcd1234
//   @label2=0x12345666
//   var1=0x12345678
//   var2=-123
//   ...
FILE *idTable;

extern int count;

// 【指令助记符列表、指令总数、指令类型】
#define INSTRUCTION_NUMBER 62
static char MemonicList[][10] = {
// R型指令（31）
	"add",        "addu",       "sub",        "subu",       "and",        "or",         "xor",        "nor",\
	"slt",        "sltu",       "sll",        "srl",        "sra",        "sllv",       "srlv",       "srav",\
	"mult",       "multu",      "div",        "divu",       "mfhi",       "mflo",       "mthi",       "mtlo",\
	"mfc0",       "mtc0",       "jr",         "jalr",       "break",      "syscall",    "eret",\
// I型指令（24）
	"addi",       "addiu",      "andi",       "ori",        "xori",       "lui",        "lb",         "lbu",\
	"lh",         "lhu",        "sb",         "sh",         "lw",         "sw",         "beq",        "bne",\
	"bgez",       "bgtz",       "blez",       "bltz",       "bgezal",     "bltzal",     "slti",       "sltiu",\
// J型指令（3）
	"j",          "jal",        "nop",\
// 宏指令
	"push",       "pop",        "pushall",    "popall"
};
static int MemonicType[] = {
// R型指令
	INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_R3,\
	INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_RS, INST_TYPE_RS, INST_TYPE_RS, INST_TYPE_R3, INST_TYPE_R3, INST_TYPE_R3,\
	INST_TYPE_R2, INST_TYPE_R2, INST_TYPE_R2, INST_TYPE_R2, INST_TYPE_R1, INST_TYPE_R1, INST_TYPE_R1, INST_TYPE_R1,\
	INST_TYPE_R2, INST_TYPE_R2, INST_TYPE_R1, INST_TYPE_R2, INST_TYPE_R0, INST_TYPE_R0, INST_TYPE_R0,\
// I型指令
	INST_TYPE_I2, INST_TYPE_I2, INST_TYPE_I2, INST_TYPE_I2, INST_TYPE_I2, INST_TYPE_I1, INST_TYPE_IR, INST_TYPE_IR,\
	INST_TYPE_IR, INST_TYPE_IR, INST_TYPE_IR, INST_TYPE_IR, INST_TYPE_IR, INST_TYPE_IR, INST_TYPE_I2B,INST_TYPE_I2B,\
	INST_TYPE_I1B,INST_TYPE_I1B,INST_TYPE_I1B,INST_TYPE_I1B,INST_TYPE_I1B,INST_TYPE_I1B,INST_TYPE_I2, INST_TYPE_I2,\
// J型指令
	INST_TYPE_J , INST_TYPE_J , INST_TYPE_J,
// 宏指令
	INST_TYPE_R1, INST_TYPE_R1, INST_TYPE_R0, INST_TYPE_R0
};
static unsigned char OperBinary[] = {
// R型指令（除[30]eret指令外均为0x00）
	0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,\
	0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,\
	0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x00,\
	0x00,         0x00,         0x00,         0x00,         0x00,         0x00,         0x10,\
// I型指令
	0x08,         0x09,         0x0c,         0x0d,         0x0e,         0x0f,         0x20,         0x24,\
	0x21,         0x25,         0x28,         0x29,         0x23,         0x2b,         0x04,         0x05,\
	0x01,         0x07,         0x06,         0x01,         0x01,         0x01,         0x0a,         0x0b,\
// J型指令
	0x02,         0x03,         0x00,
// 宏指令
	0xff,         0xff,         0xff,         0xff
};
static unsigned char FuncBinary[] = {
// R型指令
	0x20,         0x21,         0x22,         0x23,         0x24,         0x25,         0x26,         0x27,\
	0x2a,         0x2b,         0x00,         0x02,         0x03,         0x04,         0x06,         0x07,\
	0x18,         0x19,         0x1a,         0x1b,         0x10,         0x12,         0x11,         0x13,\
	0x00,         0x00,         0x08,         0x09,         0x0d,         0x0c,         0x18,\
// I型指令（字段无意义，均为0xff）
	0xff,         0xff,         0xff,         0xff,         0xff,         0xff,         0xff,         0xff,\
	0xff,         0xff,         0xff,         0xff,         0xff,         0xff,         0xff,         0xff,\
	0xff,         0xff,         0xff,         0xff,         0xff,         0xff,         0xff,         0xff,\
// J型指令（字段无意义，均为0xff）
	0xff,         0xff,         0xff,
// 宏指令
	0xff,         0xff,         0xff,         0xff
};

// 【寄存器名】
static char RegisterName[][10] = {
	"$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",\
	"$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
};
// 【寄存器号】
static char RegisterNumber[][10] = {
	"$0", "$1", "$2", "$3", "$4", "$5", "$6", "$7", "$8", "$9", "$10", "$11", "$12", "$13", "$14", "$t15",\
	"$16", "$17", "$18", "$19", "$20", "$21", "$22", "$23", "$24", "$25", "$26", "$27", "$28", "$29", "$30", "$31"
};
// 【用于同名检测的临时字符串】
static char IDTemp[100][50];
static int IDCount = 0;

// 【读取状态】
#define SECTION_DATA 10
#define SECTION_TEXT 20
// 全局状态标记：指示目前读到的是数据段还是代码段
//   本flag用于区分INST和VAR_DEF两类语句的解析
static int sectionFlag = 0;


/*
 * 取助记符
 * 输入：line
 * 输出：memonic
 * 返回：0
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
 * 取标签
 * 输入：line
 * 输出：label（以@开头）
 * 返回：有标签返回0，无标签返回1
 */
int GetLabel(char *line, char *label)
{
	//下标变量说明
	//   @label:   add  $t1  ,  $t2  ,  $t3   # vvv
	//   a     b   c
	int a = getFirstIndexOverBlank(line, 0); //第一个有效字符（非空格字符）下标
	int b = 0;
	//如果第一个有效字符即为@，则截取之
	if(line[a] == '@') {
		b = getIndex(line, ':', a); //取得:下标
		strmid(line, a, b-1, label);
		return 0;
	}
	else{
		return (-1);
	}
}
/*
 * 计算语句助记符编号
 * 输入：line
 * 被调用函数：GetMemonic
 * 返回：助记符index
 */
int GetMemonicIndex(char *line) {
	char memonic[10];
	// 取得助记符
	GetMemonic(line, memonic);
	// 查找助记符序号
	for(int i = 0; i < INSTRUCTION_NUMBER; i++) {
		if(!_stricmp(memonic, MemonicList[i])) {
			return i;
		}
	}
	// 如果未找到，返回-1
	printf("ERROR[%u] : Memonic not found.\n", count);
	return ERROR_NOTFOUND;
}

/*
 * 计算语句类型（不会修改sectionFlag）
 * 输入：line
 * 被调用函数：GetMemonic
 * 返回：0
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
	// 代码段：语句解析为指令
	if(sectionFlag == SECTION_TEXT) {
		char memonic[10];
		int memonicIndex = -1;
		// 取得助记符
		GetMemonic(line, memonic);
		// 查找助记符序号
		for(int i = 0; i < INSTRUCTION_NUMBER; i++) {
			if(!_stricmp(memonic, MemonicList[i])) {
				memonicIndex = i;
				break;
			}
		}
		// 如果找到了，返回类型
		if(memonicIndex != -1) {
			return MemonicType[memonicIndex];
		}
		// 如果未找到，返回-1
		else {
			printf("ERROR[%u] : Memonic not found.\n", count);
			return ERROR_NOTFOUND;
		}
	}
	// 代码段：语句解析为变量声明
	else if(sectionFlag == SECTION_DATA)
	{
		if(line[first] >= 'a' && line[first] <= 'z' || line[first] >= 'A' && line[first] <= 'Z' || line[first] == '_') {
			return VAR_DEF;
		}
		else {
			return ERROR_SYNTAX; //标识符语法错误（不以数字、下划线或字母开头）
		}
	}
	else {
		return -99;//不可能
	}
	return ERROR_NOTFOUND;
}

/*
 * 从指令行中取助记符对应的oper二进制字符串
 * 输入：line（全部指令）
 * 输出：operbin（6位）
 * 返回：正常返回0
 */
int GetOperBinCode(char *line, char *operbin) {
	char memonic[10];
	int memonicIndex = -1;
	uint32 oper = 0;
	char hex[20];
	//查找助记符index
	GetMemonic(line, memonic);
	for(int i = 0; i < INSTRUCTION_NUMBER; i++) {
		if(!_stricmp(memonic, MemonicList[i])) {
			memonicIndex = i;
		}
	}
	//检查是否找到
	if(memonicIndex < 0) {
		printf("ERROR[%u] : Memonic not found.\n", count);
		return ERROR_NOTFOUND;
	}
	//查oper表并转换成二进制输出
	else {
		oper = (uint32)OperBinary[memonicIndex];
		UDecToHexStr(oper, hex, 8);
		HexStrToBinStr(hex, operbin, 6);
		return 0;
	}
	return (-1);
}

/*
 * 从指令行中取助记符对应的func二进制字符串
 * 输入：line（全部指令）
 * 输出：funcbin（6位）
 * 返回：正常返回0
 */
int GetFuncBinCode(char *line, char *funcbin) {
	char memonic[10];
	int memonicIndex = -1;
	uint32 oper = 0;
	char hex[20];
	//查找助记符index
	GetMemonic(line, memonic);
	for(int i = 0; i < INSTRUCTION_NUMBER; i++) {
		if(!_stricmp(memonic, MemonicList[i])) {
			memonicIndex = i;
		}
	}
	//检查是否找到
	if(memonicIndex < 0) {
		printf("ERROR[%u] : Memonic not found.\n", count);
		return ERROR_NOTFOUND;
	}
	//查oper表并转换成二进制输出
	else {
		oper = (uint32)FuncBinary[memonicIndex];
		UDecToHexStr(oper, hex, 8);
		HexStrToBinStr(hex, funcbin, 6);
		return 0;
	}
	return (-1);
}

/*
 * 从指令行中取第一寄存器名，输出其二进制码
 * 输入：line（仅R、I型指令，本函数不检查）
 * 输出：reg1bin（5位）
 * 返回：正常返回0
 */
int GetReg1BinCode(char *line, char *reg1bin)
{
	//下标变量说明
	//   @label:   add  $t1  ,  $t2  ,  $t3   # vvv
	//                  a b  c
	int a = getIndex(line, '$', 0);        //第一个'$'下标
	int b = getIndexBeforeBlank(line, a);  //寄存器名最后一个字符的下标(空格前有效字符)
	int c = getIndex(line, ',', a);        //逗号下标
	char reg[10];
	char hex[20];
	//语法检查
	int len = (int)strlen(line);
	if(a == len) {
		printf("ERROR[%u] : Missing '$'. More operand needed probably.\n", count);
		return ERROR_SYNTAX;
	}
	//提取寄存器名
	//没有逗号，可能是R1型指令
	if(c == len) {
		strmid(line, a, b, reg);
	}
	else{
		//逗号前没有空格 或者根本没有逗号
		if(b >= c) {
			strmid(line, a, c-1, reg);
		}
		//逗号前有空格
		else {
			strmid(line, a, b, reg);
		}
	}
	//翻译寄存器名
	for(int i = 0; i < 32; i++) {
		if( !_stricmp(reg, RegisterName[i]) ) {
			UDecToHexStr((uint32)i, hex, 8);
			HexStrToBinStr(hex, reg1bin, 5);
			return 0;
		}
	}
	//翻译寄存器号
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
 * 从指令行中取第2寄存器名，输出其二进制码
 * 输入：line（仅R、I型指令，本函数不检查）
 * 输出：reg2bin（5位）
 * 返回：正常返回0
 */
int GetReg2BinCode(char *line, char *reg2bin)
{
	//下标变量说明
	//   @label:   add  $t1  ,  $t2  ,  $t3   # vvv
	//                       p  a b  c
	int p = getIndex(line, ',', 0);        //第一个','下标
	int a = getIndex(line, '$', p);        //第二个'$'下标
	int b = getIndexBeforeBlank(line, a);  //寄存器名最后一个字符的下标(空格前有效字符)
	int c = getIndex(line, ',', a);
	char reg[10];
	char hex[20];
	//语法检查
	int len = (int)strlen(line);
	if(p == len || a == len) {
		printf("ERROR[%u] : Missing ',' and/or '$'. More operand needed probably.\n", count);
		return ERROR_SYNTAX;
	}
	//提取寄存器名
	//没有第二个逗号，可能是R2型指令
	if(c == len) {
		strmid(line, a, b, reg);
	}
	else{
		//逗号前没有空格 或者根本没有逗号
		if(b >= c) {
			strmid(line, a, c-1, reg);
		}
		//逗号前有空格
		else {
			strmid(line, a, b, reg);
		}
	}
	//翻译寄存器名
	for(int i = 0; i < 32; i++) {
		if( !_stricmp(reg, RegisterName[i]) ) {
			UDecToHexStr((uint32)i, hex, 8);
			HexStrToBinStr(hex, reg2bin, 5);
			return 0;
		}
	}
	//翻译寄存器号
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
 * 从指令行中取第3寄存器名，输出其二进制码
 * 输入：line（仅R型指令，本函数不检查）
 * 输出：reg3bin（5位）
 * 返回：正常返回0
 */
int GetReg3BinCode(char *line, char *reg3bin)
{
	//下标变量说明
	//   @label:   add  $t1  ,  $t2  ,  $t3   # vvv
	//                       p       p  a b
	int p = getIndex(line, ',', 0);        //第一个','下标
	    p = getIndex(line, ',', p+1);        //第二个','下标
	int a = getIndex(line, '$', p);        //第三个'$'下标
	int b = getIndexBeforeBlank(line, a);  //寄存器名最后一个字符的下标(空格前有效字符)
	char reg[10];
	char hex[20];
	//语法检查
	int len = (int)strlen(line);
	if(p == len || a == len || b == len) {
		printf("ERROR[%u] : Missing ',' and/or '$'. More operand needed probably.\n", count);
		return ERROR_SYNTAX;
	}
	//提取寄存器名
	strmid(line, a, b, reg);
	//翻译寄存器名
	for(int i = 0; i < 32; i++) {
		if( !_stricmp(reg, RegisterName[i]) ) {
			UDecToHexStr((uint32)i, hex, 8);
			HexStrToBinStr(hex, reg3bin, 5);
			return 0;
		}
	}
	//翻译寄存器号
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
 * 从指令行中取立即数
 * 输入：line（部分R、I型指令，本函数不检查）
 * 输出：imme（字符串，可能是立即数、变量或标签）
 * 返回：正常返回0
 */
int GetImme(char *line, char *imme)
{
	int len = (int)strlen(line);
	int a = 0;
	int b = 0;
	int p = getIndex(line, ',', 0); //第一个','下标
	int pp = 0; //第2个','下标
	int leftb = 0; //左括号下标
	//检查逗号分隔符，判断语句类型
	//若有第一个逗号，可能是I1、I2、RS、IR
	if(p != len) {
	    pp = getIndex(line, ',', p+1); //检查第二个','下标
		//若有第二个逗号，则为I2或RS，处理方法相同
		//mem $1 , $2 , imme
		//            p a  b
		if(pp != len) {
			a = getFirstIndexOverBlank(line, pp+1);   //imme字段起始下标
			b = getIndexBeforeBlank(line, a);      //imme最后一个字符的下标(空格前有效字符)
			//提取imme
			strmid(line, a, b, imme);
			return 0;
		}
		//若无第二个逗号，则为I1或IR
		else if(pp == len) {
			leftb = getIndex(line, '(', 0); //左括号下标
			//若有左括号，则认为是IR
			//mem $1 , imme ( $2 )
			//       p a  b ^leftb
			if(leftb != len) {
				a = getFirstIndexOverBlank(line, p+1);   //imme字段起始下标
				b = getIndexBeforeBlank(line, a);      //imme最后一个字符的下标(空格前有效字符)
				//如果imme与(间没有空格
				if(b >= leftb) {
					strmid(line, a, leftb-1, imme);
					return 0;
				}
				//如果imme与(间有空格
				else{
					strmid(line, a, b, imme);
					return 0;
				}
			}
			//若无左括号，则认为是I1
			//mem $1 , imme
			//       p a  b
			else {
				a = getFirstIndexOverBlank(line, p+1);   //imme字段起始下标
				b = getIndexBeforeBlank(line, a);      //imme最后一个字符的下标(空格前有效字符)
				//提取imme
				strmid(line, a, b, imme);
				return 0;
			}
		}
	}
	//若无第一个逗号，则可能是J
	//mem imme
	//    a  b
	else if(p == len){
		a = getFirstIndexOverBlank(line, 0);   //第一个有效字符的下标
		//如果有标签
		if(line[a] == '@') {
			a = getIndex(line, ':', a+1);
			a = getFirstIndexOverBlank(line, a+1); //memonic字段起始下标
		}
		a = getIndexBeforeBlank(line, a);   //imme字段起始下标
		a = getFirstIndexOverBlank(line, a+1);   //imme字段起始下标
		b = getIndexBeforeBlank(line, a);      //imme最后一个字符的下标(空格前有效字符)
		//提取imme
		strmid(line, a, b, imme);
		return 0;
	}
	printf("ERROR[%u] : Register name not found.\n", count);
	return ERROR_NOTFOUND;
}
/*
 * 从IR型指令行中取基址寄存器二进制编码
 * 输入：line（IR型指令，本函数不检查）
 * 输出：baseregbin（字符串，可能是立即数、变量或标签）
 * 返回：正常返回0
 */
int GetBaseAddrRegBinCode(char *line, char *baseregbin) {
	//下标变量说明
	//   @label:   add  $t1  ,  ADDR ( $base )   # vvv
	//                               p a   b c
	int len = (int)strlen(line);
	int p = getIndex(line, '(', 0);           //左括号位置
	int a = getFirstIndexOverBlank(line, p+1); //寄存器名第一个有效字符的下标
	int b = getIndexBeforeBlank(line, a);      //最后一个有效字符的下标
	int c = getIndex(line, ')', p);           //右括号位置
	char reg[20];
	char hex[50];
	//语法检查
	if(p == len || c == len) {
		printf("ERROR[%u] : Missing '(' and/or ')'.\n", count);
		return ERROR_SYNTAX;
	}
	//提取寄存器名
	//如果$base与)间没有空格
	if(b >= c) {
		strmid(line, a, c-1, reg);
	}
	//如果$base与)间有空格
	else {
		strmid(line, a, b, reg);
	}
	//翻译寄存器名
	for(int i = 0; i < 32; i++) {
		if( !_stricmp(reg, RegisterName[i]) ) {
			UDecToHexStr((uint32)i, hex, 8);
			HexStrToBinStr(hex, baseregbin, 5);
			return 0;
		}
	}
	//翻译寄存器号
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





//【标识符重复性检查】
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