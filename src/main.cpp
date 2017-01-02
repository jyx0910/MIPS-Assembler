#include <stdio.h>
#include <stdlib.h>
#include "strutil.h"
#include "parse.h"

//ջ����ʼλ��
#define RAM_STACK_ADDR (0x0040)

//#define EXIT_ON_ERROR
#define RELEASE

//�߼��кż�������ָ���ַ��������
static int instAddr = 0;
//���ݶ���ʼ��ַ
static int dataAddr = 0;
//�����кż���������αָ��ļ�������
static int count = 0;

int main(int argc, char **argv)
{
	char c = 0;
	char line[200];     //��ǰ��

	int dataFlag = 0;   //.data��������
	int textFlag = 0;   //.text��������

	//�򿪻��Դ�ļ�
#ifdef DEBUG
	FILE *input = fopen("D:\\asm.txt", "r");
#elif defined(RELEASE)
	FILE *input = fopen(argv[1], "r");
#endif
	if(!input){
		printf("ERROR: Cannot open source file \"D:\\asm.txt\".\n");
		return (-1);
	}
	//�򿪶����ƴ����ļ�
	FILE *output = fopen("D:\\out.coe", "w");
	if(!output){
		printf("ERROR : Cannot open output file \"D:\\out.coe\".\n");
		return (-1);
	}
	//�����ݴ����ļ�
#ifdef DEBUG
	FILE *data_output = fopen("D:\\data.txt", "w");
#elif defined(RELEASE)
	FILE *data_output = fopen(argv[2], "w");
#endif
	if(!data_output){
		printf("ERROR : Cannot open output file \"D:\\data.coe\".\n");
		return (-1);
	}

	//�򿪷��ű��ļ�
	idTable = fopen("D:\\id_table.txt", "w+");
	if(!output){
		printf("ERROR : Cannot open id_table file \"D:\\id_table.txt\".\n");
		return (-1);
	}

	//��Ŀ���ļ�д��
	fputs("memory_initialization_radix=16;\n", output);
	fputs("memory_initialization_vector=\n", output);

	fputs("memory_initialization_radix=16;\n", data_output);
	fputs("memory_initialization_vector=\n", data_output);

	printf("��һ��ɨ�裺�﷨������Ԥ����\n");

	//Ԥ����ʼ
	//ѭ�����룬��ÿһ���ж�������ͣ�����������ͣ�
	//  - �޸Ķα��sectionFlag������ָ���ַ��
	//  - ��ȡ�ʷ�Ԫ�أ��������Ƿ����Ĵ���������ǩ������������ʶ���ȣ�
	//  - �����Ƿ����Ĵ�����������������ɶ����ƴ��룬��ǩ�ͱ�ʶ������ԭ���ݲ����룻
	//  - ���� [Addr]:[SemiBinCode] �ĸ�ʽ���Ԥ�����ļ�
	//���ݶ����������ã�
	//  - ��idTable��д��id-Addr��ӳ���ϵ��
	//  - ����RAM��ʼ������
	//��RAM��ʼ�������÷���
	//  - ����VAR_DEF���ʱ�������ɳ�ʼ��ָ���������ʱ�ļ��У�
	//  - ��ȫ��ָ��Ԥ������Ϻ���Ԥ�����ļ�׷�ӳ�ʼ��ָ�
	//  - ������Ԥ�������������ṹ���£�
	//  0x0000:   j @initialize      #������ת��@initialize(0x0124)
	//  0x0004: add $1,$2,$3         #��Ϊ@start��ǩ��Ӧ���߼��ϵ�����ָ��
	//  ......  ...
	//  # NUM .word 0x12345678       #����NUM�ĵ�ַ��0x22223333
	//  0x0124: lui $at,0x1234       #��λ0x1234����$at(�����ǩ@initialize)
	//  0x0128: ori $t8,$at,0x5678   #��λ0x5678����$at����$t8
	//  0x0132: lui $at,0x2222       #��ַ��λ����$at
	//  0x0136:  sw $t8,0x3333($at)  #Mem[0x22220000+0x00003333]<-$t8
	//  0x0136:andi $at,$zero,0x0000 #$at����
	//  0x0136:andi $t8,$zero,0x0000 #$t8����
	//  0x0140:   j @start           #��ת����ʼ���(0x0000)ִ��������

	do{
		int ccnt = 0;
		//��ȡһ�� line
		do{ c = fgetc(input); line[ccnt++] = c; } while(c != '\n' && c != EOF); line[ccnt-1] = 0;

		//Ԥ����ʼ

		//������� - ��ֱ�Ӿ��������Ľ��ͷ�ʽ���ķ���
		int type = GetInstructionType(line);
		
		int start = 0;		//��ʼ�±�
		int end = 0;		//��β�±�
		char temp[50];		//ͨ����ʱ�ַ���
		char labelTemp[50];	//��ǩ��ʱ�ַ���
		char addrTemp[50];  //ָ���ַ��ʱ�ַ���
		char idTemp[50];	//��ʶ����ʱ�ַ���
		char typeTemp[50];	//���ͷ���ʱ�ַ���
		char reg1Temp[50];	//�Ĵ���1��ʱ�ַ���
		char reg2Temp[50];	//�Ĵ���2��ʱ�ַ���
		char reg3Temp[50];	//�Ĵ���3��ʱ�ַ���
		char shamtTemp[50];	//shamt��ʱ�ַ���
		char immeTemp[50];	//������������ǩ����ʶ�����ã���ʱ�ַ���

		char operTemp[50];
		char funcTemp[50];

		char binCode[100];  //���ɵĶ�����ָ����
		char hexCode[100];  //���ɵ�16����ָ����

		int spaceAddr = 0;  //������ַ

		int imme = 0;       //��������ʱֵ
		int immeType = 0;   //����������

		////////////////////////////////////
		// ����������Ͷ�ÿ�������з��� //
		////////////////////////////////////

		//�����ݶ�αָ�� .data��
		//  ��Ҫִ�еĹ�����
		//    1. �趨���ݶ���ʼ��ַ
		//    2. ����ַ�Ƿ�Ϸ�
		//    3. ��data coe�����0����
		if(type == PSEUDO_DATA) {
			//printf("DATA : %s\n", line);
			start = getFirstIndexOverBlank(line, start);   //��һ����Ч�ַ����±�
			start = getIndexBeforeBlank(line, start);      //Token ".data" ���һ���ַ����±�
			start = getFirstIndexOverBlank(line, start+1); //�������ʼ��ַ����ʼ�±�
			end   = getIndexBeforeBlank(line, start);      //�������ʼ��ַ�����һ���ַ����±�
			strmid(line, start, end, temp);
			//�趨���ݶ���ʼ��ַ
			dataAddr = HexStrToHexNum(temp);
			//����ַ��Χ
			if(!(dataAddr >= 0 && dataAddr <= 65535)) {
				printf("ERROR[%u] : ��ʼ���ݵ�ַ %s ����64k��Χ\n", count, temp);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			if( dataAddr % 4 != 0) {
				printf("ERROR[%u] : ��ʼ���ݵ�ַ %s û�а��ֶ���\n", count, temp);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//printf("  ���ݶ���ʼ��ַ : %d\n", dataAddr);
			//��data_outputд��հ�����
			for(int i = 0; i < dataAddr; i+=4) {
				fputs("cccccccc,", data_output);
				fputs("\n", data_output);
			}
			//flag��һ
			dataFlag++;
			//�кż�һ
			count++;
		}//.data����

		//����ַ����αָ�� .space��
		//  ��Ҫִ�еĹ�����
		//    1. �޸ĵ�ǰ���ݵ�ַ������dataAddr
		//    2. ��鵱ǰ���ݵ�ַ���������ݵ�ַ�Ƿ�Ϸ�
		//    3. ��data coe�����0����
		else if(type == PSEUDO_SPACE) {
			//printf("SPACE : %s\n", line);
			start = getFirstIndexOverBlank(line, start);   //��һ����Ч�ַ����±�
			start = getIndexBeforeBlank(line, start);      //Token ".data" ���һ���ַ����±�
			start = getFirstIndexOverBlank(line, start+1); //�������ʼ��ַ����ʼ�±�
			end   = getIndexBeforeBlank(line, start);      //�������ʼ��ַ�����һ���ַ����±�
			strmid(line, start, end, temp);
			//��鵱ǰ���ݵ�ַ
			if( dataAddr % 4 != 0) {
				printf("ERROR[%u] : ��ǰ���ݵ�ַ %s û�а��ֶ���\n", count, temp);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//�趨������ַ��
			spaceAddr = HexStrToHexNum(temp);
			dataAddr += spaceAddr;
			//printf("  ��ǰ���ݶε�ַ : %d\n", dataAddr);
			//����ַ��Χ
			if(!(dataAddr >= 0 && dataAddr <= 65535)) {
				printf("ERROR[%u] : ����64k��Χ\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}

			if( spaceAddr % 4 != 0) {
				printf("ERROR[%u] : Space�ֽ��� %s ����4�ı���\n", count, temp);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//д��0����
			for(int i = 0; i < spaceAddr; i+=4) {
				fputs("cccccccc,", data_output);
				fputs("\n", data_output);
			}
			//�кż�һ
			count++;
		}//.space����

		//�����ݶ��塿
		//  ��Ҫִ�еĹ�����
		//    1. ���ʷ��﷨��������ȡ��ʶ�������ͷ����������ַ���
		//    2. ���Ϲ���顿�ж���������
		//    3. ��д�롿д����ű��data coe
		else if(type == VAR_DEF){
			//printf("[%u(0x%x)]DEF : %s\n", dataAddr, dataAddr, line);
			//���ʷ��﷨������
			//��ȡ��ʶ��
			start = getFirstIndexOverBlank(line, start);  //��һ����Ч�ַ����±�
			end   = getIndexBeforeBlank(line, start);     //��ʶ�����һ���ַ����±�
			strmid(line, start, end, idTemp);
			//printf("  ��ȡ���ı�ʶ�� : %s\n", idTemp);
			//��ȡ���ͷ�
			start = getFirstIndexOverBlank(line, end+1);  //���ͷ���һ����Ч�ַ����±�
			end   = getIndexBeforeBlank(line, start);     //���ͷ����һ���ַ����±�
			strmid(line, start, end, typeTemp);
			//printf("  ��ȡ�������ͷ� : %s\n", typeTemp);
			//��ȡ��ʼֵ
			start = getFirstIndexOverBlank(line, end+1);  //���ͷ���һ����Ч�ַ����±�
			end   = getIndexBeforeBlank(line, start);     //���ͷ����һ���ַ����±�
			strmid(line, start, end, immeTemp);
			//printf("  ��ȡ���ĳ�ʼֵ : %s\n", immeTemp);
			//�����ʼֵ
			if(immeTemp[0] == '0') { //ʮ�������޷�������
				imme = HexStrToHexNum(immeTemp);
			}
			else if(immeTemp[0] == '+' || immeTemp[0] == '-') {
				imme = SDecStrToSDec(immeTemp);
			}
			else if(immeTemp[0] >= '1' && immeTemp[0] <= '9') {
				imme = UDecStrToUDec(immeTemp);
			}
			else {
				printf("ERROR[%u] : ֻ�����з���ʮ���ƺ�0x��ͷ��ʮ����������\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//��д����ű��ļ���
			//���id�Ƿ��ظ��������ظ���д��
			if(!WriteIdentifier(idTemp)) {
				UDecToHexStr(dataAddr, temp, 16);
				fputs(" ", idTable);
				fputs(temp, idTable); fputs(" ", idTable);
				fputs(immeTemp, idTable); fputs("\n", idTable);
			}
			//���ж����ݿ�Ȳ�д��coe��
			//32λ�ֱ����ַ���룬һ�������32λ
			if(!_stricmp(typeTemp, ".word")) {
				//����ַ
				if(dataAddr % 4 != 0) {
					printf("ERROR[%u] : ��ǰ���ݵ�ַ %d û�а��ֶ���\n", count, dataAddr);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				} else {
					immeType = DATATYPE_WORD;
					UDecToHexStr((uint32)imme, immeTemp, 32); //������ת��Ϊʮ�������ַ���
					Delete0x(immeTemp);
					fputs(immeTemp, data_output);
					fputs(",\n", data_output);
					dataAddr += 4; //���ݵ�ַ��4
				}
			}//32λ�ֽ���
			//16λ���ֱ�����ż����ַ��һ�������16λ����������ͷ��ǰ��0x������������β����ӷֺŻ��У�
			else if(!_stricmp(typeTemp, ".half")) {
				//����ַ
				if(dataAddr % 2 != 0) {
					printf("ERROR[%u] : ��ǰ���ݵ�ַ %d û�а����ֶ���\n", count, dataAddr);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
				} else {
					immeType = DATATYPE_HALF;
					//�����λ�ֽ�
					UDecToHexStr(((uint32)imme)>>8 , immeTemp, 8); //���ֽ�ת��Ϊʮ�������ַ���
					HexStrToBinStr(immeTemp, temp, 8); //ʮ�����ƴ�ȡ��λ
					BinStrToHexStr(temp, immeTemp); //��λ�����ƴ�ת��Ϊʮ�����ƴ�
					strmid(immeTemp, 2, 3, temp); //ȥ����ͷ��0x
					fputs(temp, data_output); //�����λ�ֽ�
					//�����λ�ֽ�
					UDecToHexStr(((uint32)imme) & 0x00ff , immeTemp, 8); //���ֽ�ת��Ϊʮ�������ַ���
					HexStrToBinStr(immeTemp, temp, 8); //ʮ�����ƴ�ȡ��λ
					BinStrToHexStr(temp, immeTemp); //��λ�����ƴ�ת��Ϊʮ�����ƴ�
					strmid(immeTemp, 2, 3, temp); //ȥ����ͷ��0x
					fputs(temp, data_output); //�����λ�ֽ�
					if(dataAddr % 4 != 0) {
						//�����β�ֺźͻ���
						fputs(",\n", data_output);
					}
					dataAddr += 2; //���ݵ�ַ��2
				}//��ַ������
			}//16λ�ֽ���
			//8λ�ֽڣ���ַ��Ҫ����Ϊϵͳ���ֽڱ�ַ����������һ���ֵ�Ԫ�ĵ�0������1������2������3���ֽڡ�
			else if(!_stricmp(typeTemp, ".byte")) {
				immeType = DATATYPE_BYTE;
				//�ֵ�Ԫ��3�ֽڣ�����ӷֺź���β
				if(dataAddr % 4 == 3){
					//����ֽ�
					UDecToHexStr(((uint32)imme) & 0x00ff , immeTemp, 8); //���ֽ�ת��Ϊʮ�������ַ���
					HexStrToBinStr(immeTemp, temp, 8); //ʮ�����ƴ�ȡ��λ
					BinStrToHexStr(temp, immeTemp); //��λ�����ƴ�ת��Ϊʮ�����ƴ�
					strmid(immeTemp, 2, 3, temp); //ȥ����ͷ��0x
					fputs(temp, data_output); //�����λ�ֽ�
					//�����β�ֺźͻ���
					fputs(",\n", data_output);
				}
				//�ֵ�Ԫ��0��1��2�ֽڣ������
				else if(dataAddr % 4 == 0 || dataAddr % 4 == 1 || dataAddr % 4 == 2){
					//����ֽ�
					UDecToHexStr(((uint32)imme) & 0x00ff , immeTemp, 8); //���ֽ�ת��Ϊʮ�������ַ���
					HexStrToBinStr(immeTemp, temp, 8); //ʮ�����ƴ�ȡ��λ
					BinStrToHexStr(temp, immeTemp); //��λ�����ƴ�ת��Ϊʮ�����ƴ�
					strmid(immeTemp, 2, 3, temp); //ȥ����ͷ��0x
					fputs(temp, data_output); //�����λ�ֽ�
				}
				dataAddr += 1; //���ݵ�ַ��1
			}//8λ�ֽڽ���
			else {
				printf("ERROR[%u] : Unknown data type.\n", count, dataAddr);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//�жϳ�ʼֵ�Ƿ��������Ҫ���ԣ�
			//�кż�һ
			count++;			
		}//var_def����

		//�������αָ�� .text��
		//  ��Ҫִ�еĹ�����
		//    0. �������ʣ��ĵ�ַ��cc��䵽data coe
		//    1. �趨�������ʼ��ַ
		//    2. ����ַ�Ƿ�Ϸ�
		//    3. ��data coe�����0����
		else if(type == PSEUDO_TEXT){
			//�ʷ�����
			start = getFirstIndexOverBlank(line, start);   //��һ����Ч�ַ����±�
			start = getIndexBeforeBlank(line, start);      //Token ".text" ���һ���ַ����±�
			start = getFirstIndexOverBlank(line, start+1); //�������ʼ��ַ����ʼ�±�
			end   = getIndexBeforeBlank(line, start);      //�������ʼ��ַ�����һ���ַ����±�
			strmid(line, start, end, temp);
			//��ʼ��ַ
			int startAddr = HexStrToHexNum(temp);

			//����ַ��Χ
			if(!(startAddr >= 0 && startAddr <= 65535)) {
				printf("ERROR[%u] : ָ���ַ����64k��Χ\n", count);
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

			//flag������ڼ�¼text���ִ�����
			//��Ϊ0��˵����һ�γ��֣�Ӧ������data coe�����Ϊ1���ϣ���Ӧ��ȫ�������ռ�
			//��flag==0����ζ�����ݶν�������data_outputд��հ�����
			if(textFlag==0) {
				for(int i = dataAddr; i < 65531; i+=4) {
					fputs("cccccccc,\n", data_output);
				}
				//���һ���֣��ֺŽ�β
				fputs("cccccccc;\n", data_output);
			}
			else {
				printf("NOTICE[%u] : Fill NOP instruction from [%u(0x%x)] to [%u(0x%x)].\n", count, instAddr, instAddr, startAddr-4, startAddr-4);
				//д��nopָ��
				for(int i = instAddr; i < startAddr; i+=4) {
					fputs("00000000,\n", output);
				}
			}
			//�趨�������ʼ��ַ
			instAddr = startAddr;
			//flag��һ
			textFlag++;
			//�кż�һ
			count++;	
		}//.text����

		//��R0��ָ�
		//  ����ָ����Էֱ���
		//  2017.1.2 ����������ָ�[60]pushall��[61]popall
		else if(type == INST_TYPE_R0) {
			//printf("[%u(0x%x)]R0 : %s\n", instAddr, instAddr, line);
			//ȡ��ǩ
			if(!GetLabel(line, labelTemp)) {
				//printf("����ǩ�� : %s\n", labelTemp);
				//����ظ�label��д��label�����ַ
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //ȡ��ָ���ַʮ�������ַ���
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//��ָ������������ɡ��������Ƿ�index���������ָ������ֶθ�ʽ
			int memonicIndex = GetMemonicIndex(line);
			//[28]breakָ��
			if(memonicIndex == 28){
				strcpy(binCode, "00000000000000000000000000001101");
			}
			//[29]syscallָ��
			else if(memonicIndex == 29){
				strcpy(binCode, "00000000000000000000000000001100");
			}
			//[30]eretָ��
			else if(memonicIndex == 30){
				strcpy(binCode, "01000010000000000000000000011000");
			}
			//[60]pushallָ��
			else if(memonicIndex == 60) {
				uint32 stack_addr = RAM_STACK_ADDR;
				uint32 reg_i = 0;
				UDecToHexStr(stack_addr, immeTemp, 16);
				HexStrToBinStr(immeTemp, temp, 16);
				// sw $reg_i, RAM_STACK_ADDR($sp)
				// addi $sp,$sp,4
				for(reg_i = 0; reg_i < 32; reg_i++) {
					UDecToHexStr(reg_i, reg1Temp, 8);
					HexStrToBinStr(reg1Temp, reg2Temp, 5); //$x�Ķ�������
					strcpy(binCode, "101011"); //SW-oper
					strcat(binCode, "11101");  //rs = $sp($29)
					strcat(binCode, reg2Temp);  //rt = $x
					strcat(binCode, temp);     //imme = stack_addr
					BinStrToHexStr(binCode, hexCode); Delete0x(hexCode);
					fputs(hexCode, output); fputs(",\n", output); // SW
					fputs("23bd0004,\n", output); // addi $sp,$sp,4
					//�޸�ָ���ַ������
					instAddr += 8;
				}
				//�кż�һ
				count++;
				continue;
			}
			//[61]popallָ��
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
					HexStrToBinStr(reg1Temp, reg2Temp, 5); //$x�Ķ�������
					strcpy(binCode, "100011"); //LW-oper
					strcat(binCode, "11101");  //rs = $sp($29)
					strcat(binCode, reg2Temp);  //rt = $x
					strcat(binCode, temp);     //imme = stack_addr
					BinStrToHexStr(binCode, hexCode); Delete0x(hexCode);
					fputs(hexCode, output); fputs(",\n", output); // LW
					//�޸�ָ���ַ������
					instAddr += 8;
				}
				//�кż�һ
				count++;
				continue;
			}
			//ת����ʮ�����ơ�д��output�ļ���
			BinStrToHexStr(binCode, hexCode);//printf("  [��������] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//׷��nop
			printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
			fputs("00000000,\n", output);
			//�޸�ָ���ַ������
			instAddr += 8;
			//�кż�һ
			count++;
		}//R0����

		//��R1��ָ�
		// ����ָ����Էֱ���
		// 2017.1.2 ׷��[58]push\[59]pop����ָ��
		else if(type == INST_TYPE_R1) {
			//printf("[%u(0x%x)]R1 : %s\n", instAddr, instAddr, line);
			//ȡ��ǩ
			if(!GetLabel(line, labelTemp)) {
				//printf("����ǩ�� : %s\n", labelTemp);
				//����ظ�label��д��label�����ַ
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //ȡ��ָ���ַʮ�������ַ���
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//ȡOper��Func�Ķ����ƴ���
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			GetFuncBinCode(line, funcTemp);//printf(" FUNC : %s\n", funcTemp);
			//ȡ�Ĵ����ŵĶ����ƴ���
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			//��ָ������������ɡ��������Ƿ�index���������ָ������ֶθ�ʽ
			int memonicIndex = GetMemonicIndex(line);
			//[20]mfhiָ��
			//[21]mfloָ��
			if(memonicIndex == 20 || memonicIndex == 21 ){
				//reg1(rd)ֻ������
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
			//[22]mthiָ��
			//[23]mtloָ��
			else if(memonicIndex == 22 || memonicIndex == 23){
				strcpy(binCode, operTemp); //OPER
				strcat(binCode, reg1Temp); //RS
				strcat(binCode, "00000");  //RT
				strcat(binCode, "00000");  //RD
				strcat(binCode, "00000");  //Shamt
				strcat(binCode, funcTemp); //FUNC
			}
			//[26]jrָ��
			//����תָ��Ӧ��׷��һ��nop��
			else if(memonicIndex == 26){
				printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
				//reg1�Ƽ���$ra��11111��
				if(_stricmp(reg1Temp, "11111") ) {
					printf("NOTICE[%u] : Register $ra or $31 is recommended for instruction 'jr'.\n", count);
				}
				strcpy(binCode, operTemp); //OPER
				strcat(binCode, reg1Temp); //RS
				strcat(binCode, "00000");  //RT
				strcat(binCode, "00000");  //RD
				strcat(binCode, "00000");  //Shamt
				strcat(binCode, funcTemp); //FUNC(ӦΪ001001)
				//ת����ʮ�����ơ�д��output�ļ���
				BinStrToHexStr(binCode, hexCode);//printf("  [��������] %s\n", hexCode);
				Delete0x(hexCode);
				fputs(hexCode, output);
				fputs(",\n", output);
				//׷��nop
				fputs("00000000,\n", output);
				//�޸�ָ���ַ������
				instAddr += 8;
				//�кż�һ
				count++;
				//ֱ�ӽ����˷�֧
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
				//�޸�ָ���ַ������
				instAddr += 8;
				//�кż�һ
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
				//�޸�ָ���ַ������
				instAddr += 8;
				//�кż�һ
				count++;
				continue;
			}
			//����ָ��
			else {
				printf("ERROR[%u] : Unexpected Type-R1 Instruction.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//ת����ʮ�����ơ�д��output�ļ���
			BinStrToHexStr(binCode, hexCode);//printf("  [��������] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//ָ���ַ������+4B
			instAddr += 4;
			//�кż�һ
			count++;
		}//R1����

		//��R2��ָ�
		// ��Ϊ�������֣�
		//   [16-19]�˳�ָ�� mult $1, $2 -- OPER $1 $2 00000 00000 FUNC
		//   [24-25]  C0ָ�� mfc0 $1, $2 -- OPER 00000 $1 $2 00000 000000
		//   [ 27  ]JALRָ�� jalr $1, $2 -- OPER $2 00000 $1 00000 001001
		else if(type == INST_TYPE_R2) {
			//printf("[%u(0x%x)]R2 : %s\n", instAddr, instAddr, line);
			//ȡ��ǩ
			if(!GetLabel(line, labelTemp)) {
				//printf("����ǩ�� : %s\n", labelTemp);
				//����ظ�label��д��label�����ַ
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //ȡ��ָ���ַʮ�������ַ���
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//ȡOper��Func�Ķ����ƴ���
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			GetFuncBinCode(line, funcTemp);//printf(" FUNC : %s\n", funcTemp);
			//ȡ�Ĵ����ŵĶ����ƴ���
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			GetReg2BinCode(line, reg2Temp);//printf(" Reg2 : %s\n", reg2Temp);
			//��ָ������������ɡ��������Ƿ�index���������ָ������ֶθ�ʽ
			int memonicIndex = GetMemonicIndex(line);
			//�˳�ָ��
			if(memonicIndex >= 16 && memonicIndex <= 19) {
				strcpy(binCode, operTemp); //OPER
				strcat(binCode, reg1Temp); //RS
				strcat(binCode, reg2Temp); //RT
				strcat(binCode, "00000");  //RD
				strcat(binCode, "00000");  //Shamt
				strcat(binCode, funcTemp); //FUNC
			}
			//c0ָ��
			else if(memonicIndex >= 24 && memonicIndex <= 25){
				//reg2(rd)ֻ������
				if(!_stricmp(reg2Temp, "00000") ) {
					printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
				}
				strcpy(binCode, operTemp); //OPER
				strcat(binCode, "00000");  //RS
				strcat(binCode, reg1Temp); //RT
				strcat(binCode, reg2Temp); //RD
				strcat(binCode, "00000");  //Shamt
				strcat(binCode, funcTemp); //FUNC(ӦΪ000000)
			}
			//jalrָ��
			//����תָ��Ӧ��׷��һ��nop��
			else if(memonicIndex == 27){
				printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
				//reg1�Ƽ���$ra��11111��
				if(_stricmp(reg1Temp, "11111") ) {
					printf("NOTICE[%u] : Register $ra or $31 is recommended for instruction 'jr'.\n", count);
				}
				//reg1(rd)ֻ������
				if(!_stricmp(reg1Temp, "00000") ) {
					printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
				}
				strcpy(binCode, operTemp); //OPER
				strcat(binCode, reg2Temp); //RS
				strcat(binCode, "00000");  //RT
				strcat(binCode, reg1Temp); //RD
				strcat(binCode, "00000");  //Shamt
				strcat(binCode, funcTemp); //FUNC(ӦΪ001001)
				//ת����ʮ�����ơ�д��output�ļ���
				BinStrToHexStr(binCode, hexCode);//printf("  [��������] %s\n", hexCode);
				Delete0x(hexCode);
				fputs(hexCode, output);
				fputs(",\n", output);
				//׷��nop
				fputs("00000000,\n", output);
				//�޸�ָ���ַ������
				instAddr += 8;
				//�кż�һ
				count++;
				//ֱ�ӽ����˷�֧
				continue;
			}
			//����ָ��
			else {
				printf("ERROR[%u] : Unexpected Type-R2 Instruction.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//ת����ʮ�����ơ�д��output�ļ���
			BinStrToHexStr(binCode, hexCode);//printf("  [��������] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//ָ���ַ������+4B
			instAddr += 4;
			//�кż�һ
			count++;
		}//R2����

		//��R3��ָ�
		//mem $1, $2, $3  --  oper $2 $3 $1 shamt func
		//  ��Ҫִ�еĹ�����
		//    1. ���ʷ�������������label��memonic��reg1��reg2��reg3
		//    2. ����ַ��ǡ���label�Ͷ�Ӧ�Ĵ����ַ�Ǽǵ�id_table
		//    3. �������뷭�롿����ָ�ͬ����memonic��reg1��reg2��reg3��shamt����ɲ�ͬ�Ķ������ַ�������ϳ�ʮ������ָ����
		//    4. ��д�롿��outputд��ָ����
		//    5. ��ָ���ַ��4��
		else if(type == INST_TYPE_R3) {
			//printf("[%u(0x%x)]R3 : %s\n", instAddr, instAddr, line);
			//ȡ��ǩ
			if(!GetLabel(line, labelTemp)) {
				//printf("����ǩ�� : %s\n", labelTemp);
				//����ظ�label��д��label�����ַ
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //ȡ��ָ���ַʮ�������ַ���
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//ȡOper��Func�Ķ����ƴ���
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			GetFuncBinCode(line, funcTemp);//printf(" FUNC : %s\n", funcTemp);
			//ȡ�Ĵ����ŵĶ����ƴ���
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			GetReg2BinCode(line, reg2Temp);//printf(" Reg2 : %s\n", reg2Temp);
			GetReg3BinCode(line, reg3Temp);//printf(" Reg3 : %s\n", reg3Temp);
			//�趨shamt�Ķ����ƴ���
			strcpy(shamtTemp, "00000");
			//reg1(rd)ֻ������
			if(!_stricmp(reg1Temp, "00000") ) {
				printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
			}
			//�������յ�ָ���������
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, reg2Temp); //RS
			strcat(binCode, reg3Temp); //RT
			strcat(binCode, reg1Temp); //RD
			strcat(binCode, shamtTemp);//Shamt
			strcat(binCode, funcTemp); //FUNC
			//ת����ʮ������д��output�ļ�
			BinStrToHexStr(binCode, hexCode);//printf("  [��������] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);

			//ָ���ַ������+4B
			instAddr += 4;
			//�кż�һ
			count++;
		}//R3����

		//��RS��ָ�
		else if(type == INST_TYPE_RS) {
			//printf("[%u(0x%x)]RS : %s\n", instAddr, instAddr, line);
			//ȡ��ǩ
			if(!GetLabel(line, labelTemp)) {
				//printf("����ǩ�� : %s\n", labelTemp);
				//����ظ�label��д��label�����ַ
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //ȡ��ָ���ַʮ�������ַ���
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//ȡOper��Func�Ķ����ƴ���
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			GetFuncBinCode(line, funcTemp);//printf(" FUNC : %s\n", funcTemp);
			//ȡ�Ĵ����ŵĶ����ƴ���
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			GetReg2BinCode(line, reg2Temp);//printf(" Reg2 : %s\n", reg2Temp);
			//ȡshamt���������ַ���
			GetImme(line, immeTemp);
			//����shamt�Ķ����Ʊ���
			uint32 sh = UDecStrToUDec(immeTemp);
			//Shamt��λ������
			if(sh >= 32) {
				printf("WARNING[%u] : Shift amount greater than 31. Use least 5 bits.\n", count);
			}
			UDecToHexStr(sh, temp, 8);
			HexStrToBinStr(temp, shamtTemp, 5);//printf(" Shamt : %s\n", shamtTemp);
			//reg1(rd)ֻ������
			if(!_stricmp(reg1Temp, "00000") ) {
				printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
			}
			//�������յ�ָ���������
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, "00000");  //RS
			strcat(binCode, reg2Temp); //RT
			strcat(binCode, reg1Temp); //RD
			strcat(binCode, shamtTemp);//Shamt
			strcat(binCode, funcTemp); //FUNC
			//ת����ʮ������д��output�ļ�
			BinStrToHexStr(binCode, hexCode);//printf("  [��������] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//ָ���ַ������+4B
			instAddr += 4;
			//�кż�һ
			count++;
		}//RS����

		//��I1��ָ�����lui��������ֻ�������֣���������id��label��
		else if(type == INST_TYPE_I1) {
			//printf("[%u(0x%x)]I1 : %s\n", instAddr, instAddr, line);
			//ȡ��ǩ
			if(!GetLabel(line, labelTemp)) {
				//printf("����ǩ�� : %s\n", labelTemp);
				//����ظ�label��д��label�����ַ
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //ȡ��ָ���ַʮ�������ַ���
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//ȡOper�Ķ����ƴ���
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			//ȡ�Ĵ����ŵĶ����ƴ���
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			//ȡ���������ַ���
			GetImme(line, immeTemp);
			//����immeTemp�Ķ����Ʊ���
			//�����ж�����������
			//ʮ������
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
			//�з���ʮ����
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
			//�޷���ʮ����
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
			//�����������
			else {
				printf("ERROR[%u] : Dec(signed or unsigned) or Hex number expected for IMME. No other identifier or value allowed.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//�������յ�ָ���������
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, "00000");  //RS
			strcat(binCode, reg1Temp); //RT
			strcat(binCode, immeTemp); //IMME
			//reg1(rt)ֻ������
			if(!_stricmp(reg1Temp, "00000") ) {
				printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
			}
			//ת����ʮ������д��output�ļ�
			BinStrToHexStr(binCode, hexCode);//printf("  [��������] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//ָ���ַ������+4B
			instAddr += 4;
			//�кż�һ
			count++;
		}//I1����

		//��I2��ָ����漰�����Ĵ�����������ָ������漰�ô����ת�����imme�ֶβ�����id��label��
		//��[31-35][53][54]������ָ��
		//  imme�ֶα����ǡ����֡���
		else if(type == INST_TYPE_I2) {
			//printf("[%u(0x%x)]I2 : %s\n", instAddr, instAddr, line);
			//ȡ��ǩ
			if(!GetLabel(line, labelTemp)) {
				//printf("����ǩ�� : %s\n", labelTemp);
				//����ظ�label��д��label�����ַ
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //ȡ��ָ���ַʮ�������ַ���
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//ȡOper�Ķ����ƴ���
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			//ȡ�Ĵ����ŵĶ����ƴ���
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			GetReg2BinCode(line, reg2Temp);//printf(" Reg2 : %s\n", reg2Temp);
			//ȡ���������ַ���
			GetImme(line, immeTemp);
			//����immeTemp�Ķ����Ʊ���
			//�����ж�����������
			//ʮ������
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
			//�з���ʮ����
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
			//�޷���ʮ����
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
			//�����������
			else {
				printf("ERROR[%u] : Dec(signed or unsigned) or Hex number expected for IMME. No other identifier or value allowed.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//�������յ�ָ���������
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, reg2Temp); //RS
			strcat(binCode, reg1Temp); //RT
			strcat(binCode, immeTemp); //IMME
			//reg1(rt)ֻ������
			if(!_stricmp(reg1Temp, "00000") ) {
				printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
			}
			//ת����ʮ������д��output�ļ�
			BinStrToHexStr(binCode, hexCode);//printf("  [��������] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//ָ���ַ������+4B
			instAddr += 4;
			//�кż�һ
			count++;
		}//I2����

		//��I1B��ָ�
		//[47-52]������ָ��
		else if(type == INST_TYPE_I1B) {
			//printf("[%u(0x%x)]I1B : %s\n", instAddr, instAddr, line);
			//ȡ��ǩ
			if(!GetLabel(line, labelTemp)) {
				//printf("����ǩ�� : %s\n", labelTemp);
				//����ظ�label��д��label�����ַ
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //ȡ��ָ���ַʮ�������ַ���
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//ȡOper�Ķ����ƴ���
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			//ȡ�Ĵ����ŵĶ����ƴ���
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			//����reg2�ֶ�
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
			//ȡ���������ַ���
			GetImme(line, immeTemp);//printf(" IDENTIFIER : %s\n", immeTemp);
			//���ж����������͡�
			//  ����������������жϾ����˽�����ָ����BinCode�����м����
			//  �м�����ʽ���£�
			//    bne $22222, $33333, @label #����ָ��ʮ�����Ƶ�ַ0x1234
			//    L0001012222233333@label:0x1234
			//  ��Ϊ�����ַ������������������з�����
			//  ����������λ��ָ�����������������ֽ���/4����
			//���֣�ʮ�����ƣ����룩
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
			//���֣��з���ʮ����
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
			//���֣��޷���ʮ����
			else if(immeTemp[0] >= '0' && immeTemp[0] <= '9') {
				printf("ERROR[%u] : Please use Signed Dec for branch instructions.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//��ǩ
			else if(immeTemp[0] == '@'){
				//reg1(rt)ֻ������
				if(!_stricmp(reg1Temp, "00000") ) {
					printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
				}
				//���ɵ�ǰָ���ַ��ʮ�������ַ���
				UDecToHexStr(instAddr, addrTemp, 16);
				//���ɺ�������ʶ�����м����
				strcpy(binCode, "B");      //"B"
				strcat(binCode, operTemp); //OPER
				strcat(binCode, reg1Temp); //RS
				strcat(binCode, reg2Temp); //RT
				strcat(binCode, immeTemp); //@label
				strcat(binCode, ":");      //":"
				strcat(binCode, addrTemp); //��ǰָ���ַ
				//�м����������ļ�
				fputs(binCode, output);//printf("  [�м����] %s\n", binCode);
				fputs(",\n", output);
				//׷��nop
				printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
				fputs("00000000", output);
				fputs(",\n", output);
				//ָ���ַ������+8B
				instAddr += 8;
				//�кż�һ
				count++;
				continue;
			}
			//�������յ�ָ���������
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, reg1Temp); //RS
			strcat(binCode, reg2Temp); //RT
			strcat(binCode, immeTemp); //IMME
			//reg1(rt)ֻ������
			if(!_stricmp(reg1Temp, "00000") ) {
				printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
			}
			//ת����ʮ������д��output�ļ�
			BinStrToHexStr(binCode, hexCode);//printf("  [��������] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//׷��nop
			printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
			fputs("00000000", output);
			fputs(",\n", output);
			//ָ���ַ������+8B
			instAddr += 8;
			//�кż�һ
			count++;
		}//I1B����

		//��I2B��ָ�
		//��[45][46]����ָ�һ���ʽ��
		//  mem $1, $2, imme/@label
		else if(type == INST_TYPE_I2B) {
			//printf("[%u(0x%x)]I2B : %s\n", instAddr, instAddr, line);
			//ȡ��ǩ
			if(!GetLabel(line, labelTemp)) {
				//printf("����ǩ�� : %s\n", labelTemp);
				//����ظ�label��д��label�����ַ
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //ȡ��ָ���ַʮ�������ַ���
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//ȡOper�Ķ����ƴ���
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			//ȡ�Ĵ����ŵĶ����ƴ���
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			GetReg2BinCode(line, reg2Temp);//printf(" Reg2 : %s\n", reg2Temp);
			//ȡ���������ַ���
			GetImme(line, immeTemp);//printf(" IDENTIFIER : %s\n", immeTemp);
			//���ж����������͡�
			//  ����������������жϾ����˽�����ָ����BinCode�����м����
			//  �м�����ʽ���£�
			//    bne $22222, $33333, @label #����ָ��ʮ�����Ƶ�ַ0x1234
			//    L0001012222233333@label:0x1234
			//  ��Ϊ�����ַ������������������з�����
			//  ����������λ��ָ�����������������ֽ���/4����
			//���֣�ʮ�����ƣ����룩
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
			//���֣��з���ʮ����
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
			//���֣��޷���ʮ����
			else if(immeTemp[0] >= '0' && immeTemp[0] <= '9') {
				printf("ERROR[%u] : Please use Signed Dec for branch instructions.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//��ǩ
			else if(immeTemp[0] == '@'){
				//reg1(rt)ֻ������
				if(!_stricmp(reg1Temp, "00000") ) {
					printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
				}
				//���ɵ�ǰָ���ַ��ʮ�������ַ���
				UDecToHexStr(instAddr, addrTemp, 16);
				//���ɺ�������ʶ�����м����
				strcpy(binCode, "B");      //"B"
				strcat(binCode, operTemp); //OPER
				strcat(binCode, reg1Temp); //RS
				strcat(binCode, reg2Temp); //RT
				strcat(binCode, immeTemp); //@label
				strcat(binCode, ":");      //":"
				strcat(binCode, addrTemp); //��ǰָ���ַ
				//�м����������ļ�
				fputs(binCode, output);//printf("  [�м����] %s\n", binCode);
				fputs(",\n", output);
				//׷��nop
				printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
				fputs("00000000", output);
				fputs(",\n", output);
				//ָ���ַ������+8B
				instAddr += 8;
				//�кż�һ
				count++;
				continue;
			}
			//�������յ�ָ���������
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, reg1Temp); //RS
			strcat(binCode, reg2Temp); //RT
			strcat(binCode, immeTemp); //IMME
			//reg1(rt)ֻ������
			if(!_stricmp(reg1Temp, "00000") ) {
				printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
			}
			//ת����ʮ������д��output�ļ�
			BinStrToHexStr(binCode, hexCode);//printf("  [��������] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//׷��nop
			printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
			fputs("00000000", output);
			fputs(",\n", output);
			//ָ���ַ������+8B
			instAddr += 8;
			//�кż�һ
			count++;
		}//I2B����

		//��IR��ָ�
		//��[37-44]��8��ָ�һ���ʽ��
		//  mem $1 , ADDR ( $2 )
		else if(type == INST_TYPE_IR) {
			//printf("[%u(0x%x)]IR : %s\n", instAddr, instAddr, line);
			//ȡ��ǩ
			if(!GetLabel(line, labelTemp)) {
				//printf("����ǩ�� : %s\n", labelTemp);
				//����ظ�label��д��label�����ַ
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //ȡ��ָ���ַʮ�������ַ���
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//ȡOper�Ķ����ƴ���
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			//ȡ�Ĵ����ŵĶ����ƴ���
			GetReg1BinCode(line, reg1Temp);//printf(" Reg1 : %s\n", reg1Temp);
			GetBaseAddrRegBinCode(line, reg2Temp);//printf(" Reg2 : %s\n", reg2Temp);
			//ȡƫ�Ƶ�ַ���ַ���
			GetImme(line, immeTemp);//printf(" IDENTIFIER : %s\n", immeTemp);
			//���ж�ƫ�Ƶ�ַ���͡�
			//  �������ƫ�Ƶ�ַ���жϾ����˽�����ָ����BinCode�����м����
			//  �м�����ʽ���£�
			//    lw $11111,ADDR($22222)
			//    V1000112222211111ADDR
			//  ��Ϊ�����ַ�����ƫ�Ƶ�ַ�������з�����
			//���֣�ʮ������
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
			//���֣��з���ʮ����
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
			//���֣��޷���ʮ����
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
			//ID��ʶ��
			else if(immeTemp[0] >= 'a' && immeTemp[0] <= 'z' || immeTemp[0] >= 'A' && immeTemp[0] <= 'Z' || immeTemp[0] == '_'){
				//reg1(rt)ֻ������
				if(!_stricmp(reg1Temp, "00000") ) {
					printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
				}
				//���ɺ�������ʶ�����м����
				strcpy(binCode, "V");      //"V"
				strcat(binCode, operTemp); //OPER
				strcat(binCode, reg2Temp); //RS
				strcat(binCode, reg1Temp); //RT
				strcat(binCode, immeTemp); //immeID
				//�м����������ļ�
				fputs(binCode, output);//printf("  [�м����] %s\n", binCode);
				fputs(",\n", output);
				//ָ���ַ������+4B
				instAddr += 4;
				//�кż�һ
				count++;
				continue;
			}
			//�������յ�ָ���������
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, reg2Temp); //RS
			strcat(binCode, reg1Temp); //RT
			strcat(binCode, immeTemp); //IMME
			//reg1(rt)ֻ������
			if(!_stricmp(reg1Temp, "00000") ) {
				printf("WARNING[%u] : Register $zero or $0 is read-only and always set to constant 0.\n", count);
			}
			//ת����ʮ������д��output�ļ�
			BinStrToHexStr(binCode, hexCode);//printf("  [��������] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//ָ���ַ������+4B
			instAddr += 4;
			//�кż�һ
			count++;
		}//IR����

		//J��ָ��
		else if(type == INST_TYPE_J) {
			//printf("[%u(0x%x)]J : %s\n", instAddr, instAddr, line);
			//ȡ��ǩ
			if(!GetLabel(line, labelTemp)) {
				//printf("����ǩ�� : %s\n", labelTemp);
				//����ظ�label��д��label�����ַ
				if( !WriteIdentifier(labelTemp)) {
					UDecToHexStr(instAddr, addrTemp, 16); //ȡ��ָ���ַʮ�������ַ���
					fputs(" ", idTable);
					fputs(addrTemp, idTable);
					fputs("\n", idTable);
				}
			}
			//ȡOper�Ķ����ƴ���
			GetOperBinCode(line, operTemp);//printf(" OPER : %s\n", operTemp);
			//�ж�nop
			int memonicIndex = GetMemonicIndex(line);
			if(memonicIndex == 57) {
				fputs("00000000", output);
				fputs(",\n", output);
				//ָ���ַ������+4B
				instAddr += 4;
				//�кż�һ
				count++;
				continue;
			}
			//ȡ���������ַ���
			GetImme(line, immeTemp);//printf(" IDENTIFIER : %s\n", immeTemp);
			//���ж����������͡�
			//  ����������������жϾ����˽�����ָ����BinCode�����м����
			//  �м�����ʽ���£�
			//    bne $22222, $33333, @label #����ָ��ʮ�����Ƶ�ַ0x1234
			//    L0001012222233333@label:0x1234
			//  ��Ϊ�����ַ������������������з�����
			//  ����������λ��ָ�����������������ֽ���/4����
			//���֣�ʮ�����ƣ�ԭ�룩
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
			//���֣��з���ʮ����
			else if(immeTemp[0] == '+' || immeTemp[0] == '-') {
				printf("ERROR[%u] : Please use Unsigned Dec for Jump instructions.\n", count);
#ifdef EXIT_ON_ERROR
				exit(-1);
#endif
			}
			//���֣��޷���ʮ����
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
			//��ǩ
			else if(immeTemp[0] == '@'){
				//���ɵ�ǰָ���ַ��ʮ�������ַ���
				UDecToHexStr(instAddr, addrTemp, 16);
				//���ɺ�������ʶ�����м����
				strcpy(binCode, "J");      //"J"
				strcat(binCode, operTemp); //OPER
				strcat(binCode, immeTemp); //@label
				strcat(binCode, ":");      //":"
				strcat(binCode, addrTemp); //��ǰָ���ַ
				//�м����������ļ�
				fputs(binCode, output);//printf("  [�м����] %s\n", binCode);
				fputs(",\n", output);
				//׷��nop
				printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
				fputs("00000000", output);
				fputs(",\n", output);
				//ָ���ַ������+8B
				instAddr += 8;
				//�кż�һ
				count++;
				continue;
			}
			//�������յ�ָ���������
			strcpy(binCode, operTemp); //OPER
			strcat(binCode, immeTemp); //IMME
			//ת����ʮ������д��output�ļ�
			BinStrToHexStr(binCode, hexCode);//printf("  [��������] %s\n", hexCode);
			Delete0x(hexCode);
			fputs(hexCode, output);
			fputs(",\n", output);
			//׷��nop
			printf("NOTICE[%u] : A NOP instruction is appended at [%u(0x%x)] for adapting the CPU architecture.\n", count, instAddr+4, instAddr+4);
			fputs("00000000", output);
			fputs(",\n", output);
			//ָ���ַ������+8B
			instAddr += 8;
			//�кż�һ
			count++;
		}//J����

		//ע����
		else if(type == COMMENT) {
			//printf("COMMENT : %s\n", line);
			//�кż�һ
			count++;
		}

		//����
		else if(type == NULL_LINE) {
			//printf("NULL_LINE : %s\n", line);
			//�кż�һ
			count++;
		}

		//�������﷨�淶�����
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
	//  ��ʼ�ڶ�����
	//    ɨ������룬�����ַ���滻��ǩ�ͱ�ʶ����
	//    �����ַ��ȫ��
	///////////////////////////////////////////////////////////////

	printf("�ڶ���ɨ�裺��ʶ���滻\n");

	//�򿪶����ƴ����ļ�
	output = fopen("D:\\out.coe", "r");
	if(!output){
		printf("ERROR : Cannot open output file \"D:\\out.coe\".\n");
		return (-1);
	}
	//�򿪷��ű��ļ�
	idTable = fopen("D:\\id_table.txt", "r");
	if(!output){
		printf("ERROR : Cannot open id_table file \"D:\\id_table.txt\".\n");
		return (-1);
	}
	//������ļ�
#ifdef DEBUG
	FILE *binfile = fopen("D:\\code.txt", "w+");
#elif defined(RELEASE)
	FILE *binfile = fopen(argv[3], "w+");
#endif
	if(!binfile){
		printf("ERROR : Cannot open output file \"D:\\binfile.coe\".\n");
		return (-1);
	}
	//�кż�������ע���ǰ��ּ�������ָ��������
	//����ǰ���У���˴�-2��ʼ
	int lineNumber = -2;
	//��ʼ���ж�ȡout.coe
	do{
		uint32 count = 0;
		//��ȡһ��line
		do{ c = fgetc(output); line[count++] = c; } while(c != '\n' && c != EOF); line[count-1] = 0;
		//printf("[%u]: %s\n", lineNumber, line);
		//��Ҫ�滻��ʶ�������
		//  ��ʽ�� V1010000001000001NUM1,
		//                          a   b
		if(line[0] == 'V') {
			//������
			int a = getLetterIndex(line, 1); //��ʶ���ĵ�һ����ĸλ��
			int b = getIndex(line, ',', a);  //ĩβ����λ��
			char prefix[50]; //��������ǰ׺
			char identifier[50];
			char idAddr[50];
			char idBin[50];
			char bincode[100]; //����Ķ�������
			char hexcode[100]; //�����16������
			strmid(line, 1, a-1, prefix); //���е�ָ����
			strmid(line, a, b-1, identifier);  //id
			//��idTable�в���id
			char ch = 0;
			char item[100];
			//�ҵ���ʶ�����ڱ���
			char seekflag = 0;
			rewind(idTable);
			do{
				int cnt = 0;
				seekflag = 0;
				do{ ch = fgetc(idTable); item[cnt++] = ch; } while(ch != '\n' && ch != EOF); item[cnt-1] = 0;
				//����idTable��Ŀ
				// NUM 0xaabb ...
				// 0 x y    z
				if(item[0] >= 'a' && item[0] <= 'z' || item[0] >= 'A' && item[0] <= 'Z' || item[0] == '_') {
					//ȡid
					char idtmp[50];
					int x = getIndex(item, ' ', 0) - 1;
					int y = x + 2;
					int z = getIndexBeforeBlank(item, y);
					strmid(item, 0, x, idtmp);
					//�ж��Ƿ�һ��
					if(!_stricmp(identifier, idtmp)){
						seekflag = 1;
						//ȡ��ַ�ַ���
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
			//����id��ַ�Ķ������ַ�������prefix����
			HexStrToBinStr(idAddr, idBin, 16);
			strcpy(bincode, prefix);
			strcat(bincode, idBin);
			//ת��hex
			BinStrToHexStr(bincode, hexcode);
			Delete0x(hexcode);
			fputs(hexcode, binfile);
			fputs(",\n", binfile);
		}
		//��Ҫ�滻��ǩ�����
		//  ��ʽ�� L000010@label:0x0078,
		//                a     b      c
		else if(line[0] == 'J' || line[0] == 'B') {
			//������
			int a = getIndex(line, '@', 0);
			int b = getIndex(line, ':', a);
			int c = getIndex(line, ',', b);
			char prefix[50]; //��������ǰ׺
			char label[50];
			char labelAddr[50];
			char addr[50];
			char bincode[100]; //����Ķ�������
			char hexcode[100]; //�����16������
			strmid(line, 1, a-1, prefix);
			strmid(line, a, b-1, label);
			strmid(line, b+1, c-1, addr);
			uint32 currentAddr = HexStrToUDec(addr);
			//��idTable�в���@label
			char ch = 0;
			char item[100];
			char seekflag = 0;
			rewind(idTable);
			do{
				int cnt = 0;
				seekflag = 0;
				do{ ch = fgetc(idTable); item[cnt++] = ch; } while(ch != '\n' && ch != EOF); item[cnt-1] = 0;
				//����idTable��Ŀ
				if(item[0] == '@') {
					//ȡ��ǩ
					char lbltmp[50];
					int lastindex = getIndexBeforeBlank(item, 0);
					strmid(item, 0, lastindex, lbltmp);
					//�ж��Ƿ�һ��
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
			//Branch��ָ��������ƫ����������Ϊ��λ��
			sint32 offset = 0;
			if(line[0] == 'B') {
				//����label��ַ�͵�ǰ��ַ�Ĳ��ת��Ϊ�к�
				uint32 destAddr = HexStrToUDec(labelAddr);
				sint32 offset = (sint32)destAddr - ( (sint32)currentAddr + 1);
				offset >>= 2;
			}
			//Jump��ָ��������λ�ã�Ҳ������Ϊ��λ��
			else if(line[0] == 'J') {
				offset = (sint32)HexStrToUDec(labelAddr);
				offset >>= 2;
			}
			//offsetתΪ�����ƣ�����prefix���ӳ�ָ����
			char hextmp[50];
			char bintmp[50];
			SDecToHexStr(offset, hextmp, 32);
			//J��ָ���IB��ָ�������imme���Ȳ�һ��
			HexStrToBinStr(hextmp, bintmp, 32 - strlen(prefix));
			strcpy(bincode, prefix);
			strcat(bincode, bintmp);
			BinStrToHexStr(bincode, hexcode);
			Delete0x(hexcode);
			fputs(hexcode, binfile);
			fputs(",\n", binfile);
		}
		//�ļ�ĩβ����
		else if(!strcmp(line, ""))
		{
			continue;
		}
		else {
			fputs(line, binfile);
			fputs("\n", binfile);
		}
		//�кż�һ
		lineNumber++;
	}
	while(c != EOF);
	//��ȫʣ���ַ
	printf("[%u]",lineNumber);
	for(int i = lineNumber; i < 16383; i++) {
		fputs("00000000,\n", binfile);
	}
	fputs("00000000;\n", binfile);

	printf("��������\n");
	//printf("  ����RAM�����%s\n", argv[2]);
	//printf("  ����ROM�����%s\n", argv[3]);
	return 0;
}