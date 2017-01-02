#include <stdio.h>
#include <stdlib.h>
#include "strutil.h"
#include "parse.h"

//栈区起始位置
#define RAM_STACK_ADDR (0x0040)

//#define EXIT_ON_ERROR
#define RELEASE

//逻辑行号计数器（指令地址计数器）
static int instAddr = 0;
//数据段起始地址
static int dataAddr = 0;
//物理行号计数器（含伪指令的计数器）
static int count = 0;

int main(int argc, char **argv)
{
	char c = 0;
	char line[200];     //当前行

	int dataFlag = 0;   //.data语句计数器
	int textFlag = 0;   //.text语句计数器

	//打开汇编源文件
#ifdef DEBUG
	FILE *input = fopen("D:\\asm.txt", "r");
#elif defined(RELEASE)
	FILE *input = fopen(argv[1], "r");
#endif
	if(!input){
		printf("ERROR: Cannot open source file \"D:\\asm.txt\".\n");
		return (-1);
	}
	//打开二进制代码文件
	FILE *output = fopen("D:\\out.coe", "w");
	if(!output){
		printf("ERROR : Cannot open output file \"D:\\out.coe\".\n");
		return (-1);
	}
	//打开数据代码文件
#ifdef DEBUG
	FILE *data_output = fopen("D:\\data.txt", "w");
#elif defined(RELEASE)
	FILE *data_output = fopen(argv[2], "w");
#endif
	if(!data_output){
		printf("ERROR : Cannot open output file \"D:\\data.coe\".\n");
		return (-1);
	}

	//打开符号表文件
	idTable = fopen("D:\\id_table.txt", "w+");
	if(!output){
		printf("ERROR : Cannot open id_table file \"D:\\id_table.txt\".\n");
		return (-1);
	}

	//向目标文件写入
	fputs("memory_initialization_radix=16;\n", output);
	fputs("memory_initialization_vector=\n", output);

	fputs("memory_initialization_radix=16;\n", data_output);
	fputs("memory_initialization_vector=\n", data_output);

	printf("第一遍扫描：语法分析与预生成\n");

	//预处理开始
	//循环读入，对每一行判断语句类型，根据语句类型：
	//  - 修改段标记sectionFlag，计算指令地址；
	//  - 提取词法元素，包括助记符、寄存器名、标签、立即数、标识符等；
	//  - 将助记符、寄存器名、立即数翻译成二进制代码，标签和标识符保留原文暂不翻译；
	//  - 按照 [Addr]:[SemiBinCode] 的格式输出预处理文件
	//数据段有两个作用：
	//  - 向idTable中写入id-Addr的映射关系；
	//  - 生成RAM初始化代码
	//【RAM初始化代码用法】
	//  - 读入VAR_DEF语句时，即生成初始化指令，保存在临时文件中；
	//  - 待全部指令预处理完毕后，向预处理文件追加初始化指令；
	//  - 这样，预处理输出的总体结构如下：
	//  0x0000:   j @initialize      #首先跳转到@initialize(0x0124)
	//  0x0004: add $1,$2,$3         #此为@start标签对应的逻辑上的首条指令
	//  ......  ...
	//  # NUM .word 0x12345678       #假设NUM的地址是0x22223333
	//  0x0124: lui $at,0x1234       #高位0x1234存入$at(本句标签@initialize)
	//  0x0128: ori $t8,$at,0x5678   #低位0x5678或上$at存入$t8
	//  0x0132: lui $at,0x2222       #地址高位存入$at
	//  0x0136:  sw $t8,0x3333($at)  #Mem[0x22220000+0x00003333]<-$t8
	//  0x0136:andi $at,$zero,0x0000 #$at清零
	//  0x0136:andi $t8,$zero,0x0000 #$t8清零
	//  0x0140:   j @start           #跳转到开始语句(0x0000)执行主程序

	do{
		int ccnt = 0;
		//读取一行 line
		do{ c = fgetc(input); line[ccnt++] = c; } while(c != '\n' && c != EOF); line[ccnt-1] = 0;

		//预处理开始

		//语句类型 - 这直接决定后续的解释方式（文法）
		int type = GetInstructionType(line);
		
		int start = 0;		//起始下标
		int end = 0;		//结尾下标
		char temp[50];		//通用临时字符串
		char labelTemp[50];	//标签临时字符串
		char addrTemp[50];  //指令地址临时字符串
		char idTemp[50];	//标识符临时字符串
		char typeTemp[50];	//类型符临时字符串
		char reg1Temp[50];	//寄存器1临时字符串
		char reg2Temp[50];	//寄存器2临时字符串
		char reg3Temp[50];	//寄存器3临时字符串
		char shamtTemp[50];	//shamt临时字符串
		char immeTemp[50];	//立即数（含标签、标识符引用）临时字符串

		char operTemp[50];
		char funcTemp[50];

		char binCode[100];  //生成的二进制指令字
		char hexCode[100];  //生成的16进制指令字

		int spaceAddr = 0;  //跳过地址

		int imme = 0;       //立即数临时值
		int immeType = 0;   //立即数类型

		////////////////////////////////////
		// 根据语句类型对每种语句进行翻译 //
		////////////////////////////////////

		//【数据段伪指令 .data】
		//  需要执行的工作：
		//    1. 设定数据段起始地址
		//    2. 检查地址是否合法
		//    3. 向data coe中输出0数据
		if(type == PSEUDO_DATA) {
			//printf("DATA : %s\n", line);
			start = getFirstIndexOverBlank(line, start);   //第一个有效字符的下标
			start = getIndexBeforeBlank(line, start);      //Token ".data" 最后一个字符的下标
			start = getFirstIndexOverBlank(line, start+1); //程序段起始地址的起始下标
			end   = getIndexBeforeBlank(line, start);      //程序段起始地址的最后一个字符的下标
			strmid(line, start, end, temp);
			//设定数据段起始地址
			dataAddr = HexStrToHexNum(temp);
			//检查地址范围
			if(!(dataAddr >= 0 && dataAddr <= 65535)) {
				printf("ERROR[%u] : 起始数据地址 %s 超出64k范围\n", count, temp);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			if( dataAddr % 4 != 0) {
				printf("ERROR[%u] : 起始数据地址 %s 没有按字对齐\n", count, temp);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//printf("  数据段起始地址 : %d\n", dataAddr);
			//向data_output写入空白数据
			for(int i = 0; i < dataAddr; i+=4) {
				fputs("cccccccc,", data_output);
				fputs("\n", data_output);
			}
			//flag加一
			dataFlag++;
			//行号加一
			count++;
		}//.data结束

		//【地址控制伪指令 .space】
		//  需要执行的工作：
		//    1. 修改当前数据地址计数器dataAddr
		//    2. 检查当前数据地址和跳过数据地址是否合法
		//    3. 向data coe中输出0数据
		else if(type == PSEUDO_SPACE) {
			//printf("SPACE : %s\n", line);
			start = getFirstIndexOverBlank(line, start);   //第一个有效字符的下标
			start = getIndexBeforeBlank(line, start);      //Token ".data" 最后一个字符的下标
			start = getFirstIndexOverBlank(line, start+1); //程序段起始地址的起始下标
			end   = getIndexBeforeBlank(line, start);      //程序段起始地址的最后一个字符的下标
			strmid(line, start, end, temp);
			//检查当前数据地址
			if( dataAddr % 4 != 0) {
				printf("ERROR[%u] : 当前数据地址 %s 没有按字对齐\n", count, temp);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//设定跳过地址数
			spaceAddr = HexStrToHexNum(temp);
			dataAddr += spaceAddr;
			//printf("  当前数据段地址 : %d\n", dataAddr);
			//检查地址范围
			if(!(dataAddr >= 0 && dataAddr <= 65535)) {
				printf("ERROR[%u] : 超出64k范围\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}

			if( spaceAddr % 4 != 0) {
				printf("ERROR[%u] : Space字节数 %s 不是4的倍数\n", count, temp);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//写入0数据
			for(int i = 0; i < spaceAddr; i+=4) {
				fputs("cccccccc,", data_output);
				fputs("\n", data_output);
			}
			//行号加一
			count++;
		}//.space结束

		//【数据定义】
		//  需要执行的工作：
		//    1. 【词法语法分析】提取标识符、类型符和立即数字符串
		//    2. 【合规检验】判断数据类型
		//    3. 【写入】写入符号表和data coe
		else if(type == VAR_DEF){
			//printf("[%u(0x%x)]DEF : %s\n", dataAddr, dataAddr, line);
			//【词法语法分析】
			//提取标识符
			start = getFirstIndexOverBlank(line, start);  //第一个有效字符的下标
			end   = getIndexBeforeBlank(line, start);     //标识符最后一个字符的下标
			strmid(line, start, end, idTemp);
			//printf("  提取到的标识符 : %s\n", idTemp);
			//提取类型符
			start = getFirstIndexOverBlank(line, end+1);  //类型符第一个有效字符的下标
			end   = getIndexBeforeBlank(line, start);     //类型符最后一个字符的下标
			strmid(line, start, end, typeTemp);
			//printf("  提取到的类型符 : %s\n", typeTemp);
			//提取初始值
			start = getFirstIndexOverBlank(line, end+1);  //类型符第一个有效字符的下标
			end   = getIndexBeforeBlank(line, start);     //类型符最后一个字符的下标
			strmid(line, start, end, immeTemp);
			//printf("  提取到的初始值 : %s\n", immeTemp);
			//计算初始值
			if(immeTemp[0] == '0') { //十六进制无符号整数
				imme = HexStrToHexNum(immeTemp);
			}
			else if(immeTemp[0] == '+' || immeTemp[0] == '-') {
				imme = SDecStrToSDec(immeTemp);
			}
			else if(immeTemp[0] >= '1' && immeTemp[0] <= '9') {
				imme = UDecStrToUDec(immeTemp);
			}
			else {
				printf("ERROR[%u] : 只允许有符号十进制和0x开头的十六进制数。\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//【写入符号表文件】
			//检查id是否重复，若不重复则写入
			if(!WriteIdentifier(idTemp)) {
				UDecToHexStr(dataAddr, temp, 16);
				fputs(" ", idTable);
				fputs(temp, idTable); fputs(" ", idTable);
				fputs(immeTemp, idTable); fputs("\n", idTable);
			}
			//【判断数据宽度并写入coe】
			//32位字必须地址对齐，一次性输出32位
			if(!_stricmp(typeTemp, ".word")) {
				//检查地址
				if(dataAddr % 4 != 0) {
					printf("ERROR[%u] : 当前数据地址 %d 没有按字对齐\n", count, dataAddr);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				} else {
					immeType = DATATYPE_WORD;
					UDecToHexStr((uint32)imme, immeTemp, 32); //立即数转换为十六进制字符串
					Delete0x(immeTemp);
					fputs(immeTemp, data_output);
					fputs(",\n", data_output);
					dataAddr += 4; //数据地址加4
				}
			}//32位字结束
			//16位半字必须在偶数地址，一次性输出16位，可能在字头（前加0x），可能在字尾（后加分号换行）
			else if(!_stricmp(typeTemp, ".half")) {
				//检查地址
				if(dataAddr % 2 != 0) {
					printf("ERROR[%u] : 当前数据地址 %d 没有按半字对齐\n", count, dataAddr);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				} else {
					immeType = DATATYPE_HALF;
					//输出高位字节
					UDecToHexStr(((uint32)imme)>>8 , immeTemp, 8); //高字节转换为十六进制字符串
					HexStrToBinStr(immeTemp, temp, 8); //十六进制串取八位
					BinStrToHexStr(temp, immeTemp); //八位二进制串转换为十六进制串
					strmid(immeTemp, 2, 3, temp); //去掉开头的0x
					fputs(temp, data_output); //输出高位字节
					//输出低位字节
					UDecToHexStr(((uint32)imme) & 0x00ff , immeTemp, 8); //低字节转换为十六进制字符串
					HexStrToBinStr(immeTemp, temp, 8); //十六进制串取八位
					BinStrToHexStr(temp, immeTemp); //八位二进制串转换为十六进制串
					strmid(immeTemp, 2, 3, temp); //去掉开头的0x
					fputs(temp, data_output); //输出低位字节
					if(dataAddr % 4 != 0) {
						//输出行尾分号和换行
						fputs(",\n", data_output);
					}
					dataAddr += 2; //数据地址加2
				}//地址检查结束
			}//16位字结束
			//8位字节，地址无要求（因为系统按字节编址），可能在一个字单元的第0个、第1个、第2个、第3个字节。
			else if(!_stricmp(typeTemp, ".byte")) {
				immeType = DATATYPE_BYTE;
				//字单元第3字节，则添加分号和行尾
				if(dataAddr % 4 == 3){
					//输出字节
					UDecToHexStr(((uint32)imme) & 0x00ff , immeTemp, 8); //高字节转换为十六进制字符串
					HexStrToBinStr(immeTemp, temp, 8); //十六进制串取八位
					BinStrToHexStr(temp, immeTemp); //八位二进制串转换为十六进制串
					strmid(immeTemp, 2, 3, temp); //去掉开头的0x
					fputs(temp, data_output); //输出高位字节
					//输出行尾分号和换行
					fputs(",\n", data_output);
				}
				//字单元第0、1、2字节，无添加
				else if(dataAddr % 4 == 0 || dataAddr % 4 == 1 || dataAddr % 4 == 2){
					//输出字节
					UDecToHexStr(((uint32)imme) & 0x00ff , immeTemp, 8); //高字节转换为十六进制字符串
					HexStrToBinStr(immeTemp, temp, 8); //十六进制串取八位
					BinStrToHexStr(temp, immeTemp); //八位二进制串转换为十六进制串
					strmid(immeTemp, 2, 3, temp); //去掉开头的0x
					fputs(temp, data_output); //输出高位字节
				}
				dataAddr += 1; //数据地址加1
			}//8位字节结束
			else {
				printf("ERROR[%u] : Unknown data type.\n", count, dataAddr);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//判断初始值是否符合类型要求（略）
			//行号加一
			count++;			
		}//var_def结束

		//【代码段伪指令 .text】
		//  需要执行的工作：
		//    0. 将代码段剩余的地址以cc填充到data coe
		//    1. 设定代码段起始地址
		//    2. 检查地址是否合法
		//    3. 向data coe中输出0数据
		else if(type == PSEUDO_TEXT){
			//词法分析
			start = getFirstIndexOverBlank(line, start);   //第一个有效字符的下标
			start = getIndexBeforeBlank(line, start);      //Token ".text" 最后一个字符的下标
			start = getFirstIndexOverBlank(line, start+1); //程序段起始地址的起始下标
			end   = getIndexBeforeBlank(line, start);      //程序段起始地址的最后一个字符的下标
			strmid(line, start, end, temp);
			//起始地址
			int startAddr = HexStrToHexNum(temp);

			//检查地址范围
			if(!(startAddr >= 0 && startAddr <= 65535)) {
				printf("ERROR[%u] : 指令地址超出64k范围\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			if(startAddr % 4 != 0) {
				printf("ERROR[%u] : Instruction Address must be aligned to WORD.\n", count, dataAddr);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//printf("TEXT : %s\n", line);
			printf("NOTICE[%u] : Code section start addr: %d\n", count, instAddr);

			//flag标记用于记录text出现次数。
			//若为0，说明第一次出现，应当生成data coe，如果为1以上，则应补全空余代码空间
			//若flag==0，意味着数据段结束，向data_output写入空白数据
			if(textFlag==0) {
				for(int i = dataAddr; i < 65531; i+=4) {
					fputs("cccccccc,\n", data_output);
				}
				//最后一个字，分号结尾
				fputs("cccccccc;\n", data_output);
			}
			else {
				printf("NOTICE[%u] : Fill NOP instruction from [%u(0x%x)] to [%u(0x%x)].\n", count, instAddr, instAddr, startAddr-4, startAddr-4);
				//写入nop指令
				for(int i = instAddr; i < startAddr; i+=4) {
					fputs("00000000,\n", output);
				}
			}
			//设定程序段起始地址
			instAddr = startAddr;
			//flag加一
			textFlag++;
			//行号加一
			count++;	
		}//.text结束

		//【R0型指令】
		//  三条指令，各自分别处理。
		//  2017.1.2 加入两条宏指令：[60]pushall和[61]popall
		else if(type == INST_TYPE_R0) {
			//printf("[%u(0x%x)]R0 : %s\n", instAddr, instAddr, line);
			//取标签
			if(!GetLabel(line, labelTemp)) {
				//printf("【标签】 : %s\n", labelTemp);
				//检测重复label后写入label及其地址
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //取得指令地址十六进制字符串
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//【指令二进制码生成】计算助记符index，这决定了指令码的字段格式
			int memonicIndex = GetMemonicIndex(line);
			//[28]break指令
			if(memonicIndex == 28){
				strcpy(binCode, "00000000000000000000000000001101");
			}
			//[29]syscall指令
			else if(memonicIndex == 29){
				strcpy(binCode, "00000000000000000000000000001100");
			}
			//[30]eret指令
			else if(memonicIndex == 30){
				strcpy(binCode, "01000010000000000000000000011000");
			}
			//[60]pushall指令
			else if(memonicIndex == 60) {
				uint32 stack_addr = RAM_STACK_ADDR;
				uint32 reg_i = 0;
				UDecToHexStr(stack_addr, immeTemp, 16);
				HexStrToBinStr(immeTemp, temp, 16);
				// sw $reg_i, RAM_STACK_ADDR($sp)
				// addi $sp,$sp,4
				for(reg_i = 0; reg_i < 32; reg_i++) {
					UDecToHexStr(reg_i, reg1Temp, 8);
					HexStrToBinStr(reg1Temp, reg2Temp, 5); //$x的二进制码
					strcpy(binCode, "101011"); //SW-oper
					strcat(binCode, "11101");  //rs = $sp($29)
					strcat(binCode, reg2Temp);  //rt = $x
					strcat(binCode, temp);     //imme = stack_addr
					BinStrToHexStr(binCode, hexCode); Delete0x(hexCode);
					fputs(hexCode, output); fputs(",\n", output); // SW
					fputs("23bd0004,\n", output); // addi $sp,$sp,4
					//修改指令地址计数器
					instAddr += 8;
				}
				//行号加一
				count++;
				continue;
			}
			//[61]popall指令
			else if(memonicIndex == 61) {
				uint32 stack_addr = RAM_STACK_ADDR;
				uint32 reg_i = 0;
				UDecToHexStr(stack_addr, immeTemp, 16);
				HexStrToBinStr(immeTemp, temp, 16);
				// addi $sp,$sp,-4
				// lw $reg_i, RAM_STACK_ADDR($sp)
				for(reg_i = 0; reg_i < 32; reg_i++) {
					fputs("23bdfffc,\n", output); // addi $sp,$sp,-4
					UDecToHexStr(reg_i, reg1Temp, 8);
					HexStrToBinStr(reg1Temp, reg2Temp, 5); //$x的二进制码
					strcpy(binCode, "100011"); //LW-oper
					strcat(binCode, "11101");  //rs = $sp($29)
					strcat(binCode, reg2Temp);  //rt = $x
					strcat(binCode, temp);     //imme = stack_addr
					BinStrToHexStr(binCode, hexCode); Delete0x(hexCode);
					fputs(hexCode, output); fputs(",\n", output); // LW
					//修改指令地址计数器
					instAddr += 8;
				}
				//行号加一
				count++;
				continue;
			}
			//转换成十六进制【写入output文件】
			BinStrToHexStr(binCode, hexCode);//printf("  [二进制码] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//追加nop
			printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
			fputs("00000000,\n", output);
			//修改指令地址计数器
			instAddr += 8;
			//行号加一
			count++;
		}//R0结束

		//【R1型指令】
		// 五条指令，各自分别处理。
		// 2017.1.2 追加[58]push\[59]pop两条指令
		else if(type == INST_TYPE_R1) {
			//printf("[%u(0x%x)]R1 : %s\n", instAddr, instAddr, line);
			//取标签
			if(!GetLabel(line, labelTemp)) {
				//printf("【标签】 : %s\n", labelTemp);
				//检测重复label后写入label及其地址
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //取得指令地址十六进制字符串
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//取Oper和Func的二进制代码
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			GetFuncBinCode(line, funcTemp);//printf(" FUNC : %s\n", funcTemp);
			//取寄存器号的二进制代码
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			//【指令二进制码生成】计算助记符index，这决定了指令码的字段格式
			int memonicIndex = GetMemonicIndex(line);
			//[20]mfhi指令
			//[21]mflo指令
			if(memonicIndex == 20 || memonicIndex == 21 ){
				//reg1(rd)只读警告
				if(!_stricmp(reg1Temp, "00000") ) {
					printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
				}
				strcpy(binCode, operTemp); //OPER
				strcat(binCode, "00000");  //RS
				strcat(binCode, "00000");  //RT
				strcat(binCode, reg1Temp); //RD
				strcat(binCode, "00000");  //Shamt
				strcat(binCode, funcTemp); //FUNC
			}
			//[22]mthi指令
			//[23]mtlo指令
			else if(memonicIndex == 22 || memonicIndex == 23){
				strcpy(binCode, operTemp); //OPER
				strcat(binCode, reg1Temp); //RS
				strcat(binCode, "00000");  //RT
				strcat(binCode, "00000");  //RD
				strcat(binCode, "00000");  //Shamt
				strcat(binCode, funcTemp); //FUNC
			}
			//[26]jr指令
			//【跳转指令应当追加一句nop】
			else if(memonicIndex == 26){
				printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
				//reg1推荐用$ra（11111）
				if(_stricmp(reg1Temp, "11111") ) {
					printf("NOTICE[%u] : Register $ra or $31 is recommended for instruction 'jr'.\n", count);
				}
				strcpy(binCode, operTemp); //OPER
				strcat(binCode, reg1Temp); //RS
				strcat(binCode, "00000");  //RT
				strcat(binCode, "00000");  //RD
				strcat(binCode, "00000");  //Shamt
				strcat(binCode, funcTemp); //FUNC(应为001001)
				//转换成十六进制【写入output文件】
				BinStrToHexStr(binCode, hexCode);//printf("  [二进制码] %s\n", hexCode);
				Delete0x(hexCode);
				fputs(hexCode, output);
				fputs(",\n", output);
				//追加nop
				fputs("00000000,\n", output);
				//修改指令地址计数器
				instAddr += 8;
				//行号加一
				count++;
				//直接结束此分支
				continue;
			}
			//[58]push $x
			// sw $x, RAM_STACK_ADDR($sp)
			// addi $sp,$sp,4
			else if(memonicIndex == 58){
				uint32 stack_addr = RAM_STACK_ADDR;
				UDecToHexStr(stack_addr, immeTemp, 16);
				HexStrToBinStr(immeTemp, temp, 16);
				strcpy(binCode, "101011"); //SW-oper
				strcat(binCode, "11101");  //rs = $sp($29)
				strcat(binCode, reg1Temp);  //rt = $x
				strcat(binCode, temp);     //imme = stack_addr
				BinStrToHexStr(binCode, hexCode); Delete0x(hexCode);
				fputs(hexCode, output); fputs(",\n", output); // SW
				fputs("23bd0004,\n", output); // addi $sp,$sp,4
				//修改指令地址计数器
				instAddr += 8;
				//行号加一
				count++;
				continue;
			}
			//[58]pop $x
			// addi $sp,$sp,-4
			// lw $x, RAM_STACK_ADDR($sp)
			else if(memonicIndex == 59){
				uint32 stack_addr = RAM_STACK_ADDR;
				UDecToHexStr(stack_addr, immeTemp, 16);
				HexStrToBinStr(immeTemp, temp, 16);
				fputs("23bdfffc,\n", output); // addi $sp,$sp,-4
				strcpy(binCode, "100011"); //LW-oper
				strcat(binCode, "11101");  //rs = $sp($29)
				strcat(binCode, reg1Temp);  //rt = $x
				strcat(binCode, temp);     //imme = stack_addr
				BinStrToHexStr(binCode, hexCode); Delete0x(hexCode);
				fputs(hexCode, output); fputs(",\n", output); // LW
				//修改指令地址计数器
				instAddr += 8;
				//行号加一
				count++;
				continue;
			}
			//意外指令
			else {
				printf("ERROR[%u] : Unexpected Type-R1 Instruction.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//转换成十六进制【写入output文件】
			BinStrToHexStr(binCode, hexCode);//printf("  [二进制码] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//指令地址计数器+4B
			instAddr += 4;
			//行号加一
			count++;
		}//R1结束

		//【R2型指令】
		// 分为以下三种：
		//   [16-19]乘除指令 mult $1, $2 -- OPER $1 $2 00000 00000 FUNC
		//   [24-25]  C0指令 mfc0 $1, $2 -- OPER 00000 $1 $2 00000 000000
		//   [ 27  ]JALR指令 jalr $1, $2 -- OPER $2 00000 $1 00000 001001
		else if(type == INST_TYPE_R2) {
			//printf("[%u(0x%x)]R2 : %s\n", instAddr, instAddr, line);
			//取标签
			if(!GetLabel(line, labelTemp)) {
				//printf("【标签】 : %s\n", labelTemp);
				//检测重复label后写入label及其地址
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //取得指令地址十六进制字符串
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//取Oper和Func的二进制代码
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			GetFuncBinCode(line, funcTemp);//printf(" FUNC : %s\n", funcTemp);
			//取寄存器号的二进制代码
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			GetReg2BinCode(line, reg2Temp);//printf(" Reg2 : %s\n", reg2Temp);
			//【指令二进制码生成】计算助记符index，这决定了指令码的字段格式
			int memonicIndex = GetMemonicIndex(line);
			//乘除指令
			if(memonicIndex >= 16 && memonicIndex <= 19) {
				strcpy(binCode, operTemp); //OPER
				strcat(binCode, reg1Temp); //RS
				strcat(binCode, reg2Temp); //RT
				strcat(binCode, "00000");  //RD
				strcat(binCode, "00000");  //Shamt
				strcat(binCode, funcTemp); //FUNC
			}
			//c0指令
			else if(memonicIndex >= 24 && memonicIndex <= 25){
				//reg2(rd)只读警告
				if(!_stricmp(reg2Temp, "00000") ) {
					printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
				}
				strcpy(binCode, operTemp); //OPER
				strcat(binCode, "00000");  //RS
				strcat(binCode, reg1Temp); //RT
				strcat(binCode, reg2Temp); //RD
				strcat(binCode, "00000");  //Shamt
				strcat(binCode, funcTemp); //FUNC(应为000000)
			}
			//jalr指令
			//【跳转指令应当追加一句nop】
			else if(memonicIndex == 27){
				printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
				//reg1推荐用$ra（11111）
				if(_stricmp(reg1Temp, "11111") ) {
					printf("NOTICE[%u] : Register $ra or $31 is recommended for instruction 'jr'.\n", count);
				}
				//reg1(rd)只读警告
				if(!_stricmp(reg1Temp, "00000") ) {
					printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
				}
				strcpy(binCode, operTemp); //OPER
				strcat(binCode, reg2Temp); //RS
				strcat(binCode, "00000");  //RT
				strcat(binCode, reg1Temp); //RD
				strcat(binCode, "00000");  //Shamt
				strcat(binCode, funcTemp); //FUNC(应为001001)
				//转换成十六进制【写入output文件】
				BinStrToHexStr(binCode, hexCode);//printf("  [二进制码] %s\n", hexCode);
				Delete0x(hexCode);
				fputs(hexCode, output);
				fputs(",\n", output);
				//追加nop
				fputs("00000000,\n", output);
				//修改指令地址计数器
				instAddr += 8;
				//行号加一
				count++;
				//直接结束此分支
				continue;
			}
			//意外指令
			else {
				printf("ERROR[%u] : Unexpected Type-R2 Instruction.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//转换成十六进制【写入output文件】
			BinStrToHexStr(binCode, hexCode);//printf("  [二进制码] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//指令地址计数器+4B
			instAddr += 4;
			//行号加一
			count++;
		}//R2结束

		//【R3型指令】
		//mem $1, $2, $3  --  oper $2 $3 $1 shamt func
		//  需要执行的工作：
		//    1. 【词法分析】解析出label、memonic、reg1、reg2、reg3
		//    2. 【地址标记】将label和对应的代码地址登记到id_table
		//    3. 【机器码翻译】根据指令不同，将memonic、reg1、reg2、reg3、shamt翻译成不同的二进制字符串并组合成十六进制指令字
		//    4. 【写入】向output写入指令字
		//    5. 【指令地址加4】
		else if(type == INST_TYPE_R3) {
			//printf("[%u(0x%x)]R3 : %s\n", instAddr, instAddr, line);
			//取标签
			if(!GetLabel(line, labelTemp)) {
				//printf("【标签】 : %s\n", labelTemp);
				//检测重复label后写入label及其地址
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //取得指令地址十六进制字符串
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//取Oper和Func的二进制代码
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			GetFuncBinCode(line, funcTemp);//printf(" FUNC : %s\n", funcTemp);
			//取寄存器号的二进制代码
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			GetReg2BinCode(line, reg2Temp);//printf(" Reg2 : %s\n", reg2Temp);
			GetReg3BinCode(line, reg3Temp);//printf(" Reg3 : %s\n", reg3Temp);
			//设定shamt的二进制代码
			strcpy(shamtTemp, "00000");
			//reg1(rd)只读警告
			if(!_stricmp(reg1Temp, "00000") ) {
				printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
			}
			//生成最终的指令二进制码
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, reg2Temp); //RS
			strcat(binCode, reg3Temp); //RT
			strcat(binCode, reg1Temp); //RD
			strcat(binCode, shamtTemp);//Shamt
			strcat(binCode, funcTemp); //FUNC
			//转换成十六进制写入output文件
			BinStrToHexStr(binCode, hexCode);//printf("  [二进制码] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);

			//指令地址计数器+4B
			instAddr += 4;
			//行号加一
			count++;
		}//R3结束

		//【RS型指令】
		else if(type == INST_TYPE_RS) {
			//printf("[%u(0x%x)]RS : %s\n", instAddr, instAddr, line);
			//取标签
			if(!GetLabel(line, labelTemp)) {
				//printf("【标签】 : %s\n", labelTemp);
				//检测重复label后写入label及其地址
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //取得指令地址十六进制字符串
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//取Oper和Func的二进制代码
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			GetFuncBinCode(line, funcTemp);//printf(" FUNC : %s\n", funcTemp);
			//取寄存器号的二进制代码
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			GetReg2BinCode(line, reg2Temp);//printf(" Reg2 : %s\n", reg2Temp);
			//取shamt立即数的字符串
			GetImme(line, immeTemp);
			//计算shamt的二进制编码
			uint32 sh = UDecStrToUDec(immeTemp);
			//Shamt移位数警告
			if(sh >= 32) {
				printf("WARNING[%u] : Shift amount greater than 31. Use least 5 bits.\n", count);
			}
			UDecToHexStr(sh, temp, 8);
			HexStrToBinStr(temp, shamtTemp, 5);//printf(" Shamt : %s\n", shamtTemp);
			//reg1(rd)只读警告
			if(!_stricmp(reg1Temp, "00000") ) {
				printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
			}
			//生成最终的指令二进制码
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, "00000");  //RS
			strcat(binCode, reg2Temp); //RT
			strcat(binCode, reg1Temp); //RD
			strcat(binCode, shamtTemp);//Shamt
			strcat(binCode, funcTemp); //FUNC
			//转换成十六进制写入output文件
			BinStrToHexStr(binCode, hexCode);//printf("  [二进制码] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//指令地址计数器+4B
			instAddr += 4;
			//行号加一
			count++;
		}//RS结束

		//【I1型指令】（仅lui，立即数只能是数字，不可以是id或label）
		else if(type == INST_TYPE_I1) {
			//printf("[%u(0x%x)]I1 : %s\n", instAddr, instAddr, line);
			//取标签
			if(!GetLabel(line, labelTemp)) {
				//printf("【标签】 : %s\n", labelTemp);
				//检测重复label后写入label及其地址
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //取得指令地址十六进制字符串
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//取Oper的二进制代码
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			//取寄存器号的二进制代码
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			//取立即数的字符串
			GetImme(line, immeTemp);
			//计算immeTemp的二进制编码
			//首先判断立即数类型
			//十六进制
			if(immeTemp[0] == '0' && immeTemp[1] == 'x') {
				uint32 num = HexStrToUDec(immeTemp);
				if(num > 65535) {
					printf("ERROR[%u] : IMME out of range.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				HexStrToBinStr(immeTemp, temp, 16);
				strcpy(immeTemp, temp);
			}
			//有符号十进制
			else if(immeTemp[0] == '+' || immeTemp[0] == '-') {
				sint32 num = SDecStrToSDec(immeTemp);
				if( num > 32767 || num < -32768 ) {
					printf("ERROR[%u] : IMME out of range.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				SDecToHexStr(num, temp, 16);
				HexStrToBinStr(temp, immeTemp, 16);
			}
			//无符号十进制
			else if(immeTemp[0] >= '0' && immeTemp[0] <= '9') {
				uint32 num = UDecStrToUDec(immeTemp);
				if(num > 65535) {
					printf("ERROR[%u] : IMME out of range.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				UDecToHexStr(num, temp, 16);
				HexStrToBinStr(temp, immeTemp, 16);
			}
			//其他情况报错
			else {
				printf("ERROR[%u] : Dec(signed or unsigned) or Hex number expected for IMME. No other identifier or value allowed.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//生成最终的指令二进制码
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, "00000");  //RS
			strcat(binCode, reg1Temp); //RT
			strcat(binCode, immeTemp); //IMME
			//reg1(rt)只读警告
			if(!_stricmp(reg1Temp, "00000") ) {
				printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
			}
			//转换成十六进制写入output文件
			BinStrToHexStr(binCode, hexCode);//printf("  [二进制码] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//指令地址计数器+4B
			instAddr += 4;
			//行号加一
			count++;
		}//I1结束

		//【I2型指令】（涉及两个寄存器的立即数指令，并不涉及访存和跳转，因此imme字段不能是id和label）
		//含[31-35][53][54]共七条指令
		//  imme字段必须是【数字】！
		else if(type == INST_TYPE_I2) {
			//printf("[%u(0x%x)]I2 : %s\n", instAddr, instAddr, line);
			//取标签
			if(!GetLabel(line, labelTemp)) {
				//printf("【标签】 : %s\n", labelTemp);
				//检测重复label后写入label及其地址
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //取得指令地址十六进制字符串
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//取Oper的二进制代码
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			//取寄存器号的二进制代码
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			GetReg2BinCode(line, reg2Temp);//printf(" Reg2 : %s\n", reg2Temp);
			//取立即数的字符串
			GetImme(line, immeTemp);
			//计算immeTemp的二进制编码
			//首先判断立即数类型
			//十六进制
			if(immeTemp[0] == '0' && immeTemp[1] == 'x') {
				uint32 num = HexStrToUDec(immeTemp);
				if(num > 65535) {
					printf("ERROR[%u] : IMME out of range.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				HexStrToBinStr(immeTemp, temp, 16);
				strcpy(immeTemp, temp);
			}
			//有符号十进制
			else if(immeTemp[0] == '+' || immeTemp[0] == '-') {
				sint32 num = SDecStrToSDec(immeTemp);
				if( num > 32767 || num < -32768 ) {
					printf("ERROR[%u] : IMME out of range.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				SDecToHexStr(num, temp, 16);
				HexStrToBinStr(temp, immeTemp, 16);
			}
			//无符号十进制
			else if(immeTemp[0] >= '0' && immeTemp[0] <= '9') {
				uint32 num = UDecStrToUDec(immeTemp);
				if(num > 65535) {
					printf("ERROR[%u] : IMME out of range.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				UDecToHexStr(num, temp, 16);
				HexStrToBinStr(temp, immeTemp, 16);
			}
			//其他情况报错
			else {
				printf("ERROR[%u] : Dec(signed or unsigned) or Hex number expected for IMME. No other identifier or value allowed.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//生成最终的指令二进制码
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, reg2Temp); //RS
			strcat(binCode, reg1Temp); //RT
			strcat(binCode, immeTemp); //IMME
			//reg1(rt)只读警告
			if(!_stricmp(reg1Temp, "00000") ) {
				printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
			}
			//转换成十六进制写入output文件
			BinStrToHexStr(binCode, hexCode);//printf("  [二进制码] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//指令地址计数器+4B
			instAddr += 4;
			//行号加一
			count++;
		}//I2结束

		//【I1B型指令】
		//[47-52]共六条指令
		else if(type == INST_TYPE_I1B) {
			//printf("[%u(0x%x)]I1B : %s\n", instAddr, instAddr, line);
			//取标签
			if(!GetLabel(line, labelTemp)) {
				//printf("【标签】 : %s\n", labelTemp);
				//检测重复label后写入label及其地址
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //取得指令地址十六进制字符串
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//取Oper的二进制代码
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			//取寄存器号的二进制代码
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			//生成reg2字段
			int memonicIndex = GetMemonicIndex(line);
			if(memonicIndex >= 48 && memonicIndex <= 50) {
				strcpy(reg2Temp, "00000");
			}
			else if(memonicIndex == 47) {
				strcpy(reg2Temp, "00001");
			}
			else if(memonicIndex == 51) {
				strcpy(reg2Temp, "10001");
			}
			else if(memonicIndex == 52) {
				strcpy(reg2Temp, "10000");
			}
			//取立即数的字符串
			GetImme(line, immeTemp);//printf(" IDENTIFIER : %s\n", immeTemp);
			//【判断立即数类型】
			//  这里对于立即数的判断决定了将本条指令翻译成BinCode还是中间编码
			//  中间编码格式如下：
			//    bne $22222, $33333, @label #本条指令十六进制地址0x1234
			//    L0001012222233333@label:0x1234
			//  作为数字字符串的立即数可以是有符号数
			//  【立即数单位是指令相对条数（即相对字节数/4）】
			//数字：十六进制（补码）
			if(immeTemp[0] == '0' && immeTemp[1] == 'x') {
				uint32 num = HexStrToUDec(immeTemp);
				if(num > 65535) {
					printf("ERROR[%u] : IMME out of range. Please check if the offset is measured in WORD(4B).\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				HexStrToBinStr(immeTemp, temp, 16);
				strcpy(immeTemp, temp);
			}
			//数字：有符号十进制
			else if(immeTemp[0] == '+' || immeTemp[0] == '-') {
				sint32 num = SDecStrToSDec(immeTemp);
				if( num > 32767 || num < -32768 ) {
					printf("ERROR[%u] : IMME out of range.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				SDecToHexStr(num, temp, 16);
				HexStrToBinStr(temp, immeTemp, 16);
			}
			//数字：无符号十进制
			else if(immeTemp[0] >= '0' && immeTemp[0] <= '9') {
				printf("ERROR[%u] : Please use Signed Dec for branch instructions.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//标签
			else if(immeTemp[0] == '@'){
				//reg1(rt)只读警告
				if(!_stricmp(reg1Temp, "00000") ) {
					printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
				}
				//生成当前指令地址的十六进制字符串
				UDecToHexStr(instAddr, addrTemp, 16);
				//生成含变量标识符的中间编码
				strcpy(binCode, "B");      //"B"
				strcat(binCode, operTemp); //OPER
				strcat(binCode, reg1Temp); //RS
				strcat(binCode, reg2Temp); //RT
				strcat(binCode, immeTemp); //@label
				strcat(binCode, ":");      //":"
				strcat(binCode, addrTemp); //当前指令地址
				//中间编码输出到文件
				fputs(binCode, output);//printf("  [中间编码] %s\n", binCode);
				fputs(",\n", output);
				//追加nop
				printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
				fputs("00000000", output);
				fputs(",\n", output);
				//指令地址计数器+8B
				instAddr += 8;
				//行号加一
				count++;
				continue;
			}
			//生成最终的指令二进制码
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, reg1Temp); //RS
			strcat(binCode, reg2Temp); //RT
			strcat(binCode, immeTemp); //IMME
			//reg1(rt)只读警告
			if(!_stricmp(reg1Temp, "00000") ) {
				printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
			}
			//转换成十六进制写入output文件
			BinStrToHexStr(binCode, hexCode);//printf("  [二进制码] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//追加nop
			printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
			fputs("00000000", output);
			fputs(",\n", output);
			//指令地址计数器+8B
			instAddr += 8;
			//行号加一
			count++;
		}//I1B结束

		//【I2B型指令】
		//含[45][46]两条指令，一般格式：
		//  mem $1, $2, imme/@label
		else if(type == INST_TYPE_I2B) {
			//printf("[%u(0x%x)]I2B : %s\n", instAddr, instAddr, line);
			//取标签
			if(!GetLabel(line, labelTemp)) {
				//printf("【标签】 : %s\n", labelTemp);
				//检测重复label后写入label及其地址
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //取得指令地址十六进制字符串
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//取Oper的二进制代码
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			//取寄存器号的二进制代码
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			GetReg2BinCode(line, reg2Temp);//printf(" Reg2 : %s\n", reg2Temp);
			//取立即数的字符串
			GetImme(line, immeTemp);//printf(" IDENTIFIER : %s\n", immeTemp);
			//【判断立即数类型】
			//  这里对于立即数的判断决定了将本条指令翻译成BinCode还是中间编码
			//  中间编码格式如下：
			//    bne $22222, $33333, @label #本条指令十六进制地址0x1234
			//    L0001012222233333@label:0x1234
			//  作为数字字符串的立即数可以是有符号数
			//  【立即数单位是指令相对条数（即相对字节数/4）】
			//数字：十六进制（补码）
			if(immeTemp[0] == '0' && immeTemp[1] == 'x') {
				uint32 num = HexStrToUDec(immeTemp);
				if(num > 65535) {
					printf("ERROR[%u] : IMME out of range. Please check if the offset is measured in WORD(4B).\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				HexStrToBinStr(immeTemp, temp, 16);
				strcpy(immeTemp, temp);
			}
			//数字：有符号十进制
			else if(immeTemp[0] == '+' || immeTemp[0] == '-') {
				sint32 num = SDecStrToSDec(immeTemp);
				if( num > 32767 || num < -32768 ) {
					printf("ERROR[%u] : IMME out of range.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				SDecToHexStr(num, temp, 16);
				HexStrToBinStr(temp, immeTemp, 16);
			}
			//数字：无符号十进制
			else if(immeTemp[0] >= '0' && immeTemp[0] <= '9') {
				printf("ERROR[%u] : Please use Signed Dec for branch instructions.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//标签
			else if(immeTemp[0] == '@'){
				//reg1(rt)只读警告
				if(!_stricmp(reg1Temp, "00000") ) {
					printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
				}
				//生成当前指令地址的十六进制字符串
				UDecToHexStr(instAddr, addrTemp, 16);
				//生成含变量标识符的中间编码
				strcpy(binCode, "B");      //"B"
				strcat(binCode, operTemp); //OPER
				strcat(binCode, reg1Temp); //RS
				strcat(binCode, reg2Temp); //RT
				strcat(binCode, immeTemp); //@label
				strcat(binCode, ":");      //":"
				strcat(binCode, addrTemp); //当前指令地址
				//中间编码输出到文件
				fputs(binCode, output);//printf("  [中间编码] %s\n", binCode);
				fputs(",\n", output);
				//追加nop
				printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
				fputs("00000000", output);
				fputs(",\n", output);
				//指令地址计数器+8B
				instAddr += 8;
				//行号加一
				count++;
				continue;
			}
			//生成最终的指令二进制码
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, reg1Temp); //RS
			strcat(binCode, reg2Temp); //RT
			strcat(binCode, immeTemp); //IMME
			//reg1(rt)只读警告
			if(!_stricmp(reg1Temp, "00000") ) {
				printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
			}
			//转换成十六进制写入output文件
			BinStrToHexStr(binCode, hexCode);//printf("  [二进制码] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//追加nop
			printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
			fputs("00000000", output);
			fputs(",\n", output);
			//指令地址计数器+8B
			instAddr += 8;
			//行号加一
			count++;
		}//I2B结束

		//【IR型指令】
		//含[37-44]共8条指令，一般格式：
		//  mem $1 , ADDR ( $2 )
		else if(type == INST_TYPE_IR) {
			//printf("[%u(0x%x)]IR : %s\n", instAddr, instAddr, line);
			//取标签
			if(!GetLabel(line, labelTemp)) {
				//printf("【标签】 : %s\n", labelTemp);
				//检测重复label后写入label及其地址
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //取得指令地址十六进制字符串
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//取Oper的二进制代码
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			//取寄存器号的二进制代码
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			GetBaseAddrRegBinCode(line, reg2Temp);//printf(" Reg2 : %s\n", reg2Temp);
			//取偏移地址的字符串
			GetImme(line, immeTemp);//printf(" IDENTIFIER : %s\n", immeTemp);
			//【判断偏移地址类型】
			//  这里对于偏移地址的判断决定了将本条指令翻译成BinCode还是中间编码
			//  中间编码格式如下：
			//    lw $11111,ADDR($22222)
			//    V1000112222211111ADDR
			//  作为数字字符串的偏移地址可以是有符号数
			//数字：十六进制
			if(immeTemp[0] == '0' && immeTemp[1] == 'x') {
				uint32 num = HexStrToUDec(immeTemp);
				if(num > 65535) {
					printf("ERROR[%u] : IMME out of range.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				HexStrToBinStr(immeTemp, temp, 16);
				strcpy(immeTemp, temp);
			}
			//数字：有符号十进制
			else if(immeTemp[0] == '+' || immeTemp[0] == '-') {
				sint32 num = SDecStrToSDec(immeTemp);
				if((uint32)num > 65535) {
					printf("ERROR[%u] : IMME out of range.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				SDecToHexStr(num, temp, 16);
				HexStrToBinStr(temp, immeTemp, 16);
			}
			//数字：无符号十进制
			else if(immeTemp[0] >= '0' && immeTemp[0] <= '9') {
				uint32 num = UDecStrToUDec(immeTemp);
				if(num > 65535) {
					printf("ERROR[%u] : IMME out of range.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				UDecToHexStr(num, temp, 16);
				HexStrToBinStr(temp, immeTemp, 16);
			}
			//ID标识符
			else if(immeTemp[0] >= 'a' && immeTemp[0] <= 'z' || immeTemp[0] >= 'A' && immeTemp[0] <= 'Z' || immeTemp[0] == '_'){
				//reg1(rt)只读警告
				if(!_stricmp(reg1Temp, "00000") ) {
					printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
				}
				//生成含变量标识符的中间编码
				strcpy(binCode, "V");      //"V"
				strcat(binCode, operTemp); //OPER
				strcat(binCode, reg2Temp); //RS
				strcat(binCode, reg1Temp); //RT
				strcat(binCode, immeTemp); //immeID
				//中间编码输出到文件
				fputs(binCode, output);//printf("  [中间编码] %s\n", binCode);
				fputs(",\n", output);
				//指令地址计数器+4B
				instAddr += 4;
				//行号加一
				count++;
				continue;
			}
			//生成最终的指令二进制码
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, reg2Temp); //RS
			strcat(binCode, reg1Temp); //RT
			strcat(binCode, immeTemp); //IMME
			//reg1(rt)只读警告
			if(!_stricmp(reg1Temp, "00000") ) {
				printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
			}
			//转换成十六进制写入output文件
			BinStrToHexStr(binCode, hexCode);//printf("  [二进制码] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//指令地址计数器+4B
			instAddr += 4;
			//行号加一
			count++;
		}//IR结束

		//J型指令
		else if(type == INST_TYPE_J) {
			//printf("[%u(0x%x)]J : %s\n", instAddr, instAddr, line);
			//取标签
			if(!GetLabel(line, labelTemp)) {
				//printf("【标签】 : %s\n", labelTemp);
				//检测重复label后写入label及其地址
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //取得指令地址十六进制字符串
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//取Oper的二进制代码
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			//判断nop
			int memonicIndex = GetMemonicIndex(line);
			if(memonicIndex == 57) {
				fputs("00000000", output);
				fputs(",\n", output);
				//指令地址计数器+4B
				instAddr += 4;
				//行号加一
				count++;
				continue;
			}
			//取立即数的字符串
			GetImme(line, immeTemp);//printf(" IDENTIFIER : %s\n", immeTemp);
			//【判断立即数类型】
			//  这里对于立即数的判断决定了将本条指令翻译成BinCode还是中间编码
			//  中间编码格式如下：
			//    bne $22222, $33333, @label #本条指令十六进制地址0x1234
			//    L0001012222233333@label:0x1234
			//  作为数字字符串的立即数可以是有符号数
			//  【立即数单位是指令相对条数（即相对字节数/4）】
			//数字：十六进制（原码）
			if(immeTemp[0] == '0' && immeTemp[1] == 'x') {
				uint32 num = HexStrToUDec(immeTemp);
				if(num > 65535) {
					printf("ERROR[%u] : IMME out of range. Please check if the offset is measured in WORD(4B).\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				HexStrToBinStr(immeTemp, temp, 26);
				strcpy(immeTemp, temp);
			}
			//数字：有符号十进制
			else if(immeTemp[0] == '+' || immeTemp[0] == '-') {
				printf("ERROR[%u] : Please use Unsigned Dec for Jump instructions.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//数字：无符号十进制
			else if(immeTemp[0] >= '0' && immeTemp[0] <= '9') {
				uint32 num = UDecStrToUDec(immeTemp);
				if(num > 65535) {
					printf("ERROR[%u] : IMME out of range.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				}
				UDecToHexStr(num, temp, 26);
				HexStrToBinStr(temp, immeTemp, 26);
			}
			//标签
			else if(immeTemp[0] == '@'){
				//生成当前指令地址的十六进制字符串
				UDecToHexStr(instAddr, addrTemp, 16);
				//生成含变量标识符的中间编码
				strcpy(binCode, "J");      //"J"
				strcat(binCode, operTemp); //OPER
				strcat(binCode, immeTemp); //@label
				strcat(binCode, ":");      //":"
				strcat(binCode, addrTemp); //当前指令地址
				//中间编码输出到文件
				fputs(binCode, output);//printf("  [中间编码] %s\n", binCode);
				fputs(",\n", output);
				//追加nop
				printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
				fputs("00000000", output);
				fputs(",\n", output);
				//指令地址计数器+8B
				instAddr += 8;
				//行号加一
				count++;
				continue;
			}
			//生成最终的指令二进制码
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, immeTemp); //IMME
			//转换成十六进制写入output文件
			BinStrToHexStr(binCode, hexCode);//printf("  [二进制码] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//追加nop
			printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
			fputs("00000000", output);
			fputs(",\n", output);
			//指令地址计数器+8B
			instAddr += 8;
			//行号加一
			count++;
		}//J结束

		//注释行
		else if(type == COMMENT) {
			//printf("COMMENT : %s\n", line);
			//行号加一
			count++;
		}

		//空行
		else if(type == NULL_LINE) {
			//printf("NULL_LINE : %s\n", line);
			//行号加一
			count++;
		}

		//不符合语法规范的语句
		else{
			printf("ERROR[%u] : Unknown instruction type.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
		}
	}
	while(c != EOF);
	fclose(output);
	fclose(data_output);
	fclose(input);
	fclose(idTable);

	///////////////////////////////////////////////////////////////
	//  开始第二遍汇编
	//    扫描机器码，计算地址，替换标签和标识符；
	//    空余地址补全。
	///////////////////////////////////////////////////////////////

	printf("第二遍扫描：标识符替换\n");

	//打开二进制代码文件
	output = fopen("D:\\out.coe", "r");
	if(!output){
		printf("ERROR : Cannot open output file \"D:\\out.coe\".\n");
		return (-1);
	}
	//打开符号表文件
	idTable = fopen("D:\\id_table.txt", "r");
	if(!output){
		printf("ERROR : Cannot open id_table file \"D:\\id_table.txt\".\n");
		return (-1);
	}
	//打开输出文件
#ifdef DEBUG
	FILE *binfile = fopen("D:\\code.txt", "w+");
#elif defined(RELEASE)
	FILE *binfile = fopen(argv[3], "w+");
#endif
	if(!binfile){
		printf("ERROR : Cannot open output file \"D:\\binfile.coe\".\n");
		return (-1);
	}
	//行号计数器，注意是按字计数（即指令条数）
	//跳过前两行，因此从-2开始
	int lineNumber = -2;
	//开始逐行读取out.coe
	do{
		uint32 count = 0;
		//读取一行line
		do{ c = fgetc(output); line[count++] = c; } while(c != '\n' && c != EOF); line[count-1] = 0;
		//printf("[%u]: %s\n", lineNumber, line);
		//需要替换标识符的语句
		//  格式例 V1010000001000001NUM1,
		//                          a   b
		if(line[0] == 'V') {
			//语句解析
			int a = getLetterIndex(line, 1); //标识符的第一个字母位置
			int b = getIndex(line, ',', a);  //末尾逗号位置
			char prefix[50]; //二进制码前缀
			char identifier[50];
			char idAddr[50];
			char idBin[50];
			char bincode[100]; //输出的二进制码
			char hexcode[100]; //输出的16进制码
			strmid(line, 1, a-1, prefix); //已有的指令码
			strmid(line, a, b-1, identifier);  //id
			//从idTable中查找id
			char ch = 0;
			char item[100];
			//找到标识（用于报错）
			char seekflag = 0;
			rewind(idTable);
			do{
				int cnt = 0;
				seekflag = 0;
				do{ ch = fgetc(idTable); item[cnt++] = ch; } while(ch != '\n' && ch != EOF); item[cnt-1] = 0;
				//解析idTable条目
				// NUM 0xaabb ...
				// 0 x y    z
				if(item[0] >= 'a' && item[0] <= 'z' || item[0] >= 'A' && item[0] <= 'Z' || item[0] == '_') {
					//取id
					char idtmp[50];
					int x = getIndex(item, ' ', 0) - 1;
					int y = x + 2;
					int z = getIndexBeforeBlank(item, y);
					strmid(item, 0, x, idtmp);
					//判断是否一致
					if(!_stricmp(identifier, idtmp)){
						seekflag = 1;
						//取地址字符串
						strmid(item, y, z, idAddr);
						rewind(idTable);
						break;
					}
				}
				else {
					continue;
				}
			}
			while(ch != EOF);
			if(seekflag == 0) {
				printf("ERROR[%u] : Undefined identifier '%s'.\n", lineNumber, identifier);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//计算id地址的二进制字符串并与prefix连接
			HexStrToBinStr(idAddr, idBin, 16);
			strcpy(bincode, prefix);
			strcat(bincode, idBin);
			//转成hex
			BinStrToHexStr(bincode, hexcode);
			Delete0x(hexcode);
			fputs(hexcode, binfile);
			fputs(",\n", binfile);
		}
		//需要替换标签的语句
		//  格式例 L000010@label:0x0078,
		//                a     b      c
		else if(line[0] == 'J' || line[0] == 'B') {
			//语句解析
			int a = getIndex(line, '@', 0);
			int b = getIndex(line, ':', a);
			int c = getIndex(line, ',', b);
			char prefix[50]; //二进制码前缀
			char label[50];
			char labelAddr[50];
			char addr[50];
			char bincode[100]; //输出的二进制码
			char hexcode[100]; //输出的16进制码
			strmid(line, 1, a-1, prefix);
			strmid(line, a, b-1, label);
			strmid(line, b+1, c-1, addr);
			uint32 currentAddr = HexStrToUDec(addr);
			//从idTable中查找@label
			char ch = 0;
			char item[100];
			char seekflag = 0;
			rewind(idTable);
			do{
				int cnt = 0;
				seekflag = 0;
				do{ ch = fgetc(idTable); item[cnt++] = ch; } while(ch != '\n' && ch != EOF); item[cnt-1] = 0;
				//解析idTable条目
				if(item[0] == '@') {
					//取标签
					char lbltmp[50];
					int lastindex = getIndexBeforeBlank(item, 0);
					strmid(item, 0, lastindex, lbltmp);
					//判断是否一致
					if(!_stricmp(label, lbltmp)){
						seekflag++;
						strmid(item, lastindex+2, (int)strlen(item), labelAddr);
						rewind(idTable);
						break;
					}
				}
				else {
					continue;
				}
			}
			while(ch != EOF);
			if(seekflag == 0) {
				printf("ERROR[%u] : Label '%s' not found.\n", lineNumber, label);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//Branch型指令：计算相对偏移量（以字为单位）
			sint32 offset = 0;
			if(line[0] == 'B') {
				//计算label地址和当前地址的差，并转化为行号
				uint32 destAddr = HexStrToUDec(labelAddr);
				sint32 offset = (sint32)destAddr - ( (sint32)currentAddr + 1);
				offset >>= 2;
			}
			//Jump型指令：计算绝对位置（也是以字为单位）
			else if(line[0] == 'J') {
				offset = (sint32)HexStrToUDec(labelAddr);
				offset >>= 2;
			}
			//offset转为二进制，并与prefix连接成指令字
			char hextmp[50];
			char bintmp[50];
			SDecToHexStr(offset, hextmp, 32);
			//J型指令和IB型指令所需的imme长度不一样
			HexStrToBinStr(hextmp, bintmp, 32 - strlen(prefix));
			strcpy(bincode, prefix);
			strcat(bincode, bintmp);
			BinStrToHexStr(bincode, hexcode);
			Delete0x(hexcode);
			fputs(hexcode, binfile);
			fputs(",\n", binfile);
		}
		//文件末尾空行
		else if(!strcmp(line, ""))
		{
			continue;
		}
		else {
			fputs(line, binfile);
			fputs("\n", binfile);
		}
		//行号加一
		lineNumber++;
	}
	while(c != EOF);
	//补全剩余地址
	printf("[%u]",lineNumber);
	for(int i = lineNumber; i < 16383; i++) {
		fputs("00000000,\n", binfile);
	}
	fputs("00000000;\n", binfile);

	printf("汇编结束。\n");
	//printf("  数据RAM输出：%s\n", argv[2]);
	//printf("  程序ROM输出：%s\n", argv[3]);
	return 0;
}