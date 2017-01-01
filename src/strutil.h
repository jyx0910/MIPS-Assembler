#ifndef __STRUTIL_H__
#define __STRUTIL_H__

#define _CRT_SECURE_NO_DEPRECATE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//数据类型声明
//32位有符号整数
#ifndef sint32
#define sint32 int
#endif
//32位无符号整数
#ifndef uint32
#define uint32 unsigned int
#endif

//位宽声明
#define BITWIDTH_BYTE (8)
#define BITWIDTH_HALF (16)
#define BITWIDTH_WORD (32)

/*
 * 基础工具函数：字符串截取
 */
void strmid(char *src, int a, int b, char *dst)
{
	int len = (int)strlen(src);
	int cnt = 0;

	if(a<=b && a>=0 && b<=len)
	{
		for(int i = a; i <= b; i++)
		{
			dst[cnt++] = src[i];
		}
		dst[cnt] = 0;
	}
	else
	{
		printf("ERROR A0000 : function STRMID: Illegel Upper and/or Lower. Operation STRMID cancelled.\n");
		//exit(-1);
	}
}

/*
 * 基础工具函数：从某个index起，寻找字符c在字符串src中出现的第一个位置下标
 */
int getIndex(char *src, char c, int fromindex)
{
	for(int i = fromindex; i < (int)strlen(src); i++)
	{
		if(src[i]==c)
		{
			return i;
		}
	}
	return (int)strlen(src);
}
/*
 * 寻找大小写字母或者下划线在字符串src中出现的第一个位置下标
 */
int getLetterIndex(char *src, int fromindex)
{
	for(int i = fromindex; i < (int)strlen(src); i++)
	{
		if((src[i] >= 'a' && src[i] <= 'z' )|| (src[i] >= 'A' && src[i] <= 'Z') || (src[i] == '_'))
		{
			return i;
		}
	}
	return (int)strlen(src);
}

/*
 * 计算汇编语句行的第一个有效下标
 * 输入：src
 * 输出：index
 */
int getFirstIndex(char *src)
{
	int isLabeled = 0;
	int lastIndex = 0;
	for(int i = 0; i < (int)strlen(src); i++)
	{

		if(src[i]=='@')
		{
			isLabeled = 1;
		}
		if(isLabeled==0)
		{
			if(src[i]>='A' && src[i]<='Z' || src[i]>='a' && src[i]<='z')
			{
				return i;
			}
		}
		else
		{
			if(src[lastIndex]==':' && (src[i]>='A' && src[i]<='Z' || src[i]>='a' && src[i]<='z'))
			{
				return i;
			}
		}
		if(src[i]!=' ' && src[i]!='\t')
		{
			lastIndex = i;
		}
	}
	return (int)strlen(src);
}

int getOperandIndex(char *src)
{
	for(int i = 1; i < (int)strlen(src); i++)
	{
		if( (src[i-1]==' ' || src[i-1]=='\t') &&
			(src[i]=='R' || src[i]=='@' || src[i]=='$' || (src[i]>='0' && src[i]<='9')))
		{
			return i;
		}
	}
	return (int)strlen(src);
}

// 跳过空格的第一个有效字符下标
int getFirstIndexOverBlank(char *src, int fromindex) {
	for(int i = fromindex; i < (int)strlen(src); i++)
	{
		if(src[i]!=' ' && src[i]!='\t') {
			return i;
		}
	}
	return (int)strlen(src);
}
// 空格(含换行、#号)前有效字符下标
int getIndexBeforeBlank(char *src, int fromindex) {
	for(int i = fromindex; i < (int)strlen(src); i++)
	{
		if(src[i+1]==' ' || src[i+1]=='\t' || src[i+1]=='\n' || src[i+1]=='#' || src[i+1]=='\0') {
			return i;
		}
	}
	return (int)strlen(src);
}

#ifndef uint32
#define uint32 unsigned int
#endif

/*
 * 注意：本文件中所有函数的指针参数都必须在主调函数处分配空间再传入。
 *       即，不允许传入野指针。请保证传入指针所指向的空间足够大。
 */


//进制转换
//可以实现下列几种转换：
//  1. 有符号整数 到 有符号十进制字符串 [-+][0-9]+ （例如-1 到 "-1"）
//  2. 有符号十进制字符串 [-+][0-9]+ 到 有符号整数 （例如"-1" 到 -1）
//  3. 有符号整数 到 十六进制字符串 0x[0-9a-f]+    （例如-1 到 "0xffffffff"）
//  4. 十六进制字符串 0x[0-9a-f]+ 到 有符号整数    （例如"0xffffffff" 到 -1）

//  5. 无符号整数 到 无符号十进制字符串 [0-9]+     （例如65535 到 "65535"）
//  6. 无符号十进制字符串 [0-9]+ 到 无符号整数     （例如"65535" 到 65535）
//  7. 无符号整数 到 十六进制字符串 0x[0-9a-f]+    （例如65535 到 "0x00ff"）
//  8. 十六进制字符串 0x[0-9a-f]+ 到 无符号整数    （例如"0x00ff" 到 65535）

//  9. 十六进制字符串 到 二进制字符串  （例如"0xffffffff" 到 "1111...11"）
// 10. 二进制字符串 到 十六进制字符串  （例如"1111" 到 "0xf"）


//下面是函数首部声明

//  1. 有符号整数 到 有符号十进制字符串 [-+][0-9]+ （例如-1 到 "-1"）
//  参数num：有符号整数
//  参数str：输出字符串
//  返回值 ：输出字符串
char *SDecToSDecStr(sint32 num, char * str)
{
	char tmp[33]={'\0'};//用于存储num的每一位
	sint32 tmpval;         //存储num进行计算过程中的中间量
	if(num>=0)          //对num取绝对值
		tmpval=num;
	else
		tmpval=num*(-1);
	int i,j;
	for(i=0;i<33;i++)   //得到num的每一位上的数并转换成单个字符
	{
		tmp[i]=(tmpval%10)+'0';
		tmpval=tmpval/10;
		if(tmpval==0)
			break;
	}
	if(num<0)
		tmp[++i]='-';
	else
		tmp[++i]='+';
	for(j=0;i>=0;i--)   //将得到的单个字符拼成字符串
		str[j++]=tmp[i];
	str[j]='\0';
	return str;
}

//  2. 有符号十进制字符串 [-+][0-9]+ 到 有符号整数 （例如"-1" 到 -1）
//  参数str：输入字符串
//  返回值 ：转换得到的有符号整数
sint32 SDecStrToSDec(char * str)
{
	int flag;       //用于判断正负
	int tmpval=1;   //用于计算整数时，每一位数对应的权重，个十百千万。。。
	if(str[0]=='-')
		flag=-1;
	else
		flag=1;
	sint32 val=0;   //用于存储计算整数时得到的中间变量
	char tmp[33]={'\0'};//用于存储从字符串上截下来的单个字符
	int i;
	for(i=1;i<33;i++)//得到单个字符并存入tmp中
	{
		if(str[i]=='\0')
			break;
		tmp[i-1]=str[i];
	}
	for(i=i-2;i>=0;i--)
	{
		val=val+(tmp[i]-'0')*tmpval;
		tmpval=tmpval*10;
	}
	val=val*flag;
	return val;
}

//  3. 有符号整数 到 十六进制字符串 0x[0-9a-f]+    （例如-1 到 "0xffffffff"）
//  参数num：有符号整数
//  参数str：输出字符串
//  参数bitwidth：整数位宽（有可能输入short、char等）
//  返回值 ：输出字符串
char *SDecToHexStr(sint32 num, char * str, int bitwidth)
{
	if(bitwidth%4!=0)
	{
		return NULL;
	}
	int len = bitwidth/4;
	char hs[9];
	char h[9];
	_ltoa(num,h,16);   //十进制转十六进制
	sprintf(hs,"%08s",h);//十六进制转字符串
	str[0]='0';
	str[1]='x';
	int i;
    for(i=2;i<len+2;i++)
	{
		str[i]=hs[8-len+i-2];
	}
	//i++;
	str[i]='\0';
	return str;	
}

//  4. 十六进制字符串 0x[0-9a-f]+ 到 有符号整数    （例如"0xffffffff" 到 -1）
//  参数str：输入字符串
//  返回值 ：转换得到的有符号整数
sint32 HexStrToSDec(char * str)
{
	char tmp[33]={'\0'};
	for(int i=2;i<10;i++)   //去掉十六进制前面的0x
	{
		if(str[i]=='\0')
			break;
		tmp[i-2]=str[i];
	}
	sint32 tmpval;
	sscanf(tmp,"%x",&tmpval);//十六进制转十进制
	return tmpval;
}

//  5. 无符号整数 到 无符号十进制字符串 [0-9]+
//  参数num：无符号整数
//  参数str：输出字符串
//  返回值 ：输出字符串
char *UDecToUDecStr(uint32 num, char * str)
{
	char tmp[33]={'\0'};//用于存储num的每一位
	uint32 tmpval=num;         //存储num进行计算过程中的中间量
	int i,j;
	for(i=0;i<33;i++)   //得到num的每一位上的数并转换成单个字符
	{
		tmp[i]=(tmpval%10)+'0';
		tmpval=tmpval/10;
		if(tmpval==0)
			break;
	}
	for(j=0;i>=0;i--)   //将得到的单个字符拼成字符串
		str[j++]=tmp[i];
	str[j]='\0';
	return str;
}

//  6. 无符号十进制字符串 [0-9]+ 到 无符号整数
//  参数str：输入字符串
//  返回值 ：转换得到的无符号整数
uint32 UDecStrToUDec(char * str)
{
	int tmpval=1;   //用于计算整数时，每一位数对应的权重，个十百千万。。。
	uint32 val=0;   //用于存储计算整数时得到的中间变量
	char tmp[33]={'\0'};//用于存储从字符串上截下来的单个字符
	int i;
	for(i=0;i<33;i++)//得到单个字符并存入tmp中
	{
		if(str[i]=='\0')
			break;
		tmp[i]=str[i];
	}
	for(i=i-1;i>=0;i--)
	{
		val=val+(tmp[i]-'0')*tmpval;
		tmpval=tmpval*10;
	}
	return val;
}

//  7. 无符号整数 到 十六进制字符串 0x[0-9a-f]+
//  参数num：无符号整数
//  参数str：输出字符串
//  参数bitwidth：整数位宽（有可能输入short、char等）
//  返回值 ：输出字符串
char *UDecToHexStr(uint32 num, char * str, int bitwidth)
{
	if(bitwidth%4!=0)
	{
		return NULL;
	}
	int len = bitwidth/4;
	char hs[9];
	char h[9];
	_ltoa(num,h,16);   //十进制转十六进制
	sprintf(hs,"%08s",h);//十六进制转字符串
	str[0]='0';
	str[1]='x';
	int i;
    for(i=2;i<len+2;i++)
	{
		str[i]=hs[8-len+i-2];
	}
	//i++;
	str[i]='\0';
	return str;	
}

//  8. 十六进制字符串 0x[0-9a-f]+ 到 无符号整数
//  参数str：输入字符串
//  返回值 ：转换得到的无符号整数
uint32 HexStrToUDec(char * str)
{
	char tmp[33]={'\0'};
	for(int i=2;i<10;i++)   //去掉十六进制前面的0x
	{
		if(str[i]=='\0')
			break;
		tmp[i-2]=str[i];
	}
	uint32 tmpval;
	sscanf(tmp,"%x",&tmpval);//十六进制转十进制
	return tmpval;
}

//  9. 十六进制字符串 到 二进制字符串  （例如"0xffffffff" 到 "1111...11"）
//  参数hex：输入的十六进制字符串
//  参数bin：输出的二进制字符串
//  参数bitnum：二进制字符串位数，高位0填充
char *HexStrToBinStr(char *hex, char *bin, int bitnum)
{
	int i,j=0,k=0;
	char bins[100]={'\0'};
	char tmp[100]={'\0'};
	for(i=2;i<10;i++)   //去掉十六进制前面的0x
	{
		if(hex[i]=='\0')
			break;
		tmp[i-2]=hex[i];
	}
	tmp[i-2]='\0';
	for(j=0;j<20;j++)
	{
		if(tmp[j]=='\0')
			break;
		if(tmp[j]=='0')
		{
			bin[k]='0';k++;
			bin[k]='0';k++;
			bin[k]='0';k++;
			bin[k]='0';k++;
		}else if(tmp[j]=='1')
		{
			bin[k]='0';k++;
			bin[k]='0';k++;
			bin[k]='0';k++;
			bin[k]='1';k++;
		}else if(tmp[j]=='2')
		{
			bin[k]='0';k++;
			bin[k]='0';k++;
			bin[k]='1';k++;
			bin[k]='0';k++;
		}else if(tmp[j]=='3')
		{
			bin[k]='0';k++;
			bin[k]='0';k++;
			bin[k]='1';k++;
			bin[k]='1';k++;
		}else if(tmp[j]=='4')
		{
			bin[k]='0';k++;
			bin[k]='1';k++;
			bin[k]='0';k++;
			bin[k]='0';k++;
		}else if(tmp[j]=='5')
		{
			bin[k]='0';k++;
			bin[k]='1';k++;
			bin[k]='0';k++;
			bin[k]='1';k++;
		}else if(tmp[j]=='6')
		{
			bin[k]='0';k++;
			bin[k]='1';k++;
			bin[k]='1';k++;
			bin[k]='0';k++;
		}else if(tmp[j]=='7')
		{
			bin[k]='0';k++;
			bin[k]='1';k++;
			bin[k]='1';k++;
			bin[k]='1';k++;
		}else if(tmp[j]=='8')
		{
			bin[k]='1';k++;
			bin[k]='0';k++;
			bin[k]='0';k++;
			bin[k]='0';k++;
		}else if(tmp[j]=='9')
		{
			bin[k]='1';k++;
			bin[k]='0';k++;
			bin[k]='0';k++;
			bin[k]='1';k++;
		}else if(tmp[j]=='a'||tmp[j]=='A')
		{
			bin[k]='1';k++;
			bin[k]='0';k++;
			bin[k]='1';k++;
			bin[k]='0';k++;
		}else if(tmp[j]=='b'||tmp[j]=='B')
		{
			bin[k]='1';k++;
			bin[k]='0';k++;
			bin[k]='1';k++;
			bin[k]='1';k++;
		}else if(tmp[j]=='c'||tmp[j]=='C')
		{
			bin[k]='1';k++;
			bin[k]='1';k++;
			bin[k]='0';k++;
			bin[k]='0';k++;
		}else if(tmp[j]=='d'||tmp[j]=='D')
		{
			bin[k]='1';k++;
			bin[k]='1';k++;
			bin[k]='0';k++;
			bin[k]='1';k++;
		}else if(tmp[j]=='e'||tmp[j]=='E')
		{
			bin[k]='1';k++;
			bin[k]='1';k++;
			bin[k]='1';k++;
			bin[k]='0';k++;
		}else if(tmp[j]=='f'||tmp[j]=='F')
		{
			bin[k]='1';k++;
			bin[k]='1';k++;
			bin[k]='1';k++;
			bin[k]='1';k++;
		}
	}
	bin[k]='\0';
	sprintf(bins,"%064s",bin);
	int m=0;
	for(int n=64-bitnum;n<64;n++,m++)
		bin[m]=bins[n];
	bin[m]='\0';
	return bin;
}

// 10. 二进制字符串 到 十六进制字符串  （例如"1111" 到 "0xf"）
//  参数bin：输入的二进制字符串
//  参数hex：输出的十六进制字符串
char *BinStrToHexStr(char *bin, char *hex)
{
	int blen=strlen(bin);
	int hlen=blen/4;
	int j=0,k=0;
	char cobin[100]={'\0'};
	if(blen%4!=0)
	{
		hlen++;
		int mod=blen%4;
		int modlen=4-mod;//当出现不是整四位二进制时，前面用0补齐
		int i;
		for(i=0;i<modlen;i++)
			cobin[i]='0';
		cobin[i]='\0';
	}
	strcat(cobin,bin);	
	char tmp[10]={'\0'};
	for(j=0;j<hlen;j++)
	{
		int m;
		for(m=0;m<4;m++)
		{
			tmp[m]=cobin[k];
			k++;
		}
		tmp[m]='\0';
		if(strcmp(tmp,"0000")==0)
		{
			hex[j]='0';
			continue;
		}else if(strcmp(tmp,"0001")==0)
		{
			hex[j]='1';
			continue;
		}else if(strcmp(tmp,"0010")==0)
		{
			hex[j]='2';
			continue;
		}else if(strcmp(tmp,"0011")==0)
		{
			hex[j]='3';
			continue;
		}else if(strcmp(tmp,"0100")==0)
		{
			hex[j]='4';
			continue;
		}else if(strcmp(tmp,"0101")==0)
		{
			hex[j]='5';
			continue;
		}else if(strcmp(tmp,"0110")==0)
		{
			hex[j]='6';
			continue;
		}else if(strcmp(tmp,"0111")==0)
		{
			hex[j]='7';
			continue;
		}else if(strcmp(tmp,"1000")==0)
		{
			hex[j]='8';
			continue;
		}else if(strcmp(tmp,"1001")==0)
		{
			hex[j]='9';
			continue;
		}else if(strcmp(tmp,"1010")==0)
		{
			hex[j]='a';
			continue;
		}else if(strcmp(tmp,"1011")==0)
		{
			hex[j]='b';
			continue;
		}else if(strcmp(tmp,"1100")==0)
		{
			hex[j]='c';
			continue;
		}else if(strcmp(tmp,"1101")==0)
		{
			hex[j]='d';
			continue;
		}else if(strcmp(tmp,"1110")==0)
		{
			hex[j]='e';
			continue;
		}else if(strcmp(tmp,"1111")==0)
		{
			hex[j]='f';
			continue;
		}
	}
	hex[j]='\0';
	char tmp2[100]={'\0'};
	tmp2[0]='0';
	tmp2[1]='x';
	tmp2[2]='\0';
	strcat(tmp2,hex);
	strcpy(hex,tmp2);
	return hex;
}

/*
 * Hex串 - Hex数字
 */
uint32 HexStrToHexNum(char *hex)
{
	int length = (int)strlen(hex);
	int i = length - 1;
	int weight = 1;
	uint32 sum = 0;
	while(hex[i] != 'X' && hex[i] != 'x') {
		char c = hex[i];
		if(hex[i] >= 'a' && hex[i] <= 'f') {
			sum += (hex[i]-'a'+10) * weight;
		}
		else if(hex[i] >= 'A' && hex[i] <= 'F') {
			sum += (hex[i]-'A'+10) * weight;
		}
		else if(hex[i] >= '0' && hex[i] <= '9') {
			sum += (hex[i]-'0') * weight;
		}
		else {
			printf("ERROR A0000 : 十六进制数字里面混入奇怪的字符了呢。:%c\n", *hex);
			exit(-1);
		}
		weight *= 16;
		i--;
	}
	return sum;
}

/*
 * 每8个字符添加一个空格
 * 再次强调：请保证src指向的空间足够大。
 */
void InsertSpace(char *src) {
	int len = (int)strlen(src);
	int spaceNum = len >> 3; // 插入的空格数
	char *temp = (char *)malloc((len+spaceNum+1) * sizeof(char));
	int i = 0;
	int count = 0;
	
	memset(temp, '\0', (len+spaceNum+1) * sizeof(char));

	temp[count] = src[0]; count++;
	for(int i = 1; i < len; i++) {
		if(i % 8 != 0) {
			temp[count] = src[i]; count++;
		}
		else{
			temp[count] = ' ';    count++;
			temp[count] = src[i]; count++;
		}
	}
	if(len % 8 == 0) {
		temp[count] = ' ';  count++;
	}
	temp[count] = '\0';

	strcpy(src, temp);
}
/*去掉十六进制前面的0x*/
void Delete0x(char *hex) {
	char temp[100];
	strmid(hex, 2, (int)strlen(hex)-1, temp);
	strcpy(hex, temp);
}

#endif