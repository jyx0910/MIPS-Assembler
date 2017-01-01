#ifndef __STRUTIL_H__
#define __STRUTIL_H__

#define _CRT_SECURE_NO_DEPRECATE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//������������
//32λ�з�������
#ifndef sint32
#define sint32 int
#endif
//32λ�޷�������
#ifndef uint32
#define uint32 unsigned int
#endif

//λ������
#define BITWIDTH_BYTE (8)
#define BITWIDTH_HALF (16)
#define BITWIDTH_WORD (32)

/*
 * �������ߺ������ַ�����ȡ
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
 * �������ߺ�������ĳ��index��Ѱ���ַ�c���ַ���src�г��ֵĵ�һ��λ���±�
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
 * Ѱ�Ҵ�Сд��ĸ�����»������ַ���src�г��ֵĵ�һ��λ���±�
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
 * ����������еĵ�һ����Ч�±�
 * ���룺src
 * �����index
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

// �����ո�ĵ�һ����Ч�ַ��±�
int getFirstIndexOverBlank(char *src, int fromindex) {
	for(int i = fromindex; i < (int)strlen(src); i++)
	{
		if(src[i]!=' ' && src[i]!='\t') {
			return i;
		}
	}
	return (int)strlen(src);
}
// �ո�(�����С�#��)ǰ��Ч�ַ��±�
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
 * ע�⣺���ļ������к�����ָ���������������������������ռ��ٴ��롣
 *       ������������Ұָ�롣�뱣֤����ָ����ָ��Ŀռ��㹻��
 */


//����ת��
//����ʵ�����м���ת����
//  1. �з������� �� �з���ʮ�����ַ��� [-+][0-9]+ ������-1 �� "-1"��
//  2. �з���ʮ�����ַ��� [-+][0-9]+ �� �з������� ������"-1" �� -1��
//  3. �з������� �� ʮ�������ַ��� 0x[0-9a-f]+    ������-1 �� "0xffffffff"��
//  4. ʮ�������ַ��� 0x[0-9a-f]+ �� �з�������    ������"0xffffffff" �� -1��

//  5. �޷������� �� �޷���ʮ�����ַ��� [0-9]+     ������65535 �� "65535"��
//  6. �޷���ʮ�����ַ��� [0-9]+ �� �޷�������     ������"65535" �� 65535��
//  7. �޷������� �� ʮ�������ַ��� 0x[0-9a-f]+    ������65535 �� "0x00ff"��
//  8. ʮ�������ַ��� 0x[0-9a-f]+ �� �޷�������    ������"0x00ff" �� 65535��

//  9. ʮ�������ַ��� �� �������ַ���  ������"0xffffffff" �� "1111...11"��
// 10. �������ַ��� �� ʮ�������ַ���  ������"1111" �� "0xf"��


//�����Ǻ����ײ�����

//  1. �з������� �� �з���ʮ�����ַ��� [-+][0-9]+ ������-1 �� "-1"��
//  ����num���з�������
//  ����str������ַ���
//  ����ֵ ������ַ���
char *SDecToSDecStr(sint32 num, char * str)
{
	char tmp[33]={'\0'};//���ڴ洢num��ÿһλ
	sint32 tmpval;         //�洢num���м�������е��м���
	if(num>=0)          //��numȡ����ֵ
		tmpval=num;
	else
		tmpval=num*(-1);
	int i,j;
	for(i=0;i<33;i++)   //�õ�num��ÿһλ�ϵ�����ת���ɵ����ַ�
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
	for(j=0;i>=0;i--)   //���õ��ĵ����ַ�ƴ���ַ���
		str[j++]=tmp[i];
	str[j]='\0';
	return str;
}

//  2. �з���ʮ�����ַ��� [-+][0-9]+ �� �з������� ������"-1" �� -1��
//  ����str�������ַ���
//  ����ֵ ��ת���õ����з�������
sint32 SDecStrToSDec(char * str)
{
	int flag;       //�����ж�����
	int tmpval=1;   //���ڼ�������ʱ��ÿһλ����Ӧ��Ȩ�أ���ʮ��ǧ�򡣡���
	if(str[0]=='-')
		flag=-1;
	else
		flag=1;
	sint32 val=0;   //���ڴ洢��������ʱ�õ����м����
	char tmp[33]={'\0'};//���ڴ洢���ַ����Ͻ������ĵ����ַ�
	int i;
	for(i=1;i<33;i++)//�õ������ַ�������tmp��
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

//  3. �з������� �� ʮ�������ַ��� 0x[0-9a-f]+    ������-1 �� "0xffffffff"��
//  ����num���з�������
//  ����str������ַ���
//  ����bitwidth������λ���п�������short��char�ȣ�
//  ����ֵ ������ַ���
char *SDecToHexStr(sint32 num, char * str, int bitwidth)
{
	if(bitwidth%4!=0)
	{
		return NULL;
	}
	int len = bitwidth/4;
	char hs[9];
	char h[9];
	_ltoa(num,h,16);   //ʮ����תʮ������
	sprintf(hs,"%08s",h);//ʮ������ת�ַ���
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

//  4. ʮ�������ַ��� 0x[0-9a-f]+ �� �з�������    ������"0xffffffff" �� -1��
//  ����str�������ַ���
//  ����ֵ ��ת���õ����з�������
sint32 HexStrToSDec(char * str)
{
	char tmp[33]={'\0'};
	for(int i=2;i<10;i++)   //ȥ��ʮ������ǰ���0x
	{
		if(str[i]=='\0')
			break;
		tmp[i-2]=str[i];
	}
	sint32 tmpval;
	sscanf(tmp,"%x",&tmpval);//ʮ������תʮ����
	return tmpval;
}

//  5. �޷������� �� �޷���ʮ�����ַ��� [0-9]+
//  ����num���޷�������
//  ����str������ַ���
//  ����ֵ ������ַ���
char *UDecToUDecStr(uint32 num, char * str)
{
	char tmp[33]={'\0'};//���ڴ洢num��ÿһλ
	uint32 tmpval=num;         //�洢num���м�������е��м���
	int i,j;
	for(i=0;i<33;i++)   //�õ�num��ÿһλ�ϵ�����ת���ɵ����ַ�
	{
		tmp[i]=(tmpval%10)+'0';
		tmpval=tmpval/10;
		if(tmpval==0)
			break;
	}
	for(j=0;i>=0;i--)   //���õ��ĵ����ַ�ƴ���ַ���
		str[j++]=tmp[i];
	str[j]='\0';
	return str;
}

//  6. �޷���ʮ�����ַ��� [0-9]+ �� �޷�������
//  ����str�������ַ���
//  ����ֵ ��ת���õ����޷�������
uint32 UDecStrToUDec(char * str)
{
	int tmpval=1;   //���ڼ�������ʱ��ÿһλ����Ӧ��Ȩ�أ���ʮ��ǧ�򡣡���
	uint32 val=0;   //���ڴ洢��������ʱ�õ����м����
	char tmp[33]={'\0'};//���ڴ洢���ַ����Ͻ������ĵ����ַ�
	int i;
	for(i=0;i<33;i++)//�õ������ַ�������tmp��
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

//  7. �޷������� �� ʮ�������ַ��� 0x[0-9a-f]+
//  ����num���޷�������
//  ����str������ַ���
//  ����bitwidth������λ���п�������short��char�ȣ�
//  ����ֵ ������ַ���
char *UDecToHexStr(uint32 num, char * str, int bitwidth)
{
	if(bitwidth%4!=0)
	{
		return NULL;
	}
	int len = bitwidth/4;
	char hs[9];
	char h[9];
	_ltoa(num,h,16);   //ʮ����תʮ������
	sprintf(hs,"%08s",h);//ʮ������ת�ַ���
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

//  8. ʮ�������ַ��� 0x[0-9a-f]+ �� �޷�������
//  ����str�������ַ���
//  ����ֵ ��ת���õ����޷�������
uint32 HexStrToUDec(char * str)
{
	char tmp[33]={'\0'};
	for(int i=2;i<10;i++)   //ȥ��ʮ������ǰ���0x
	{
		if(str[i]=='\0')
			break;
		tmp[i-2]=str[i];
	}
	uint32 tmpval;
	sscanf(tmp,"%x",&tmpval);//ʮ������תʮ����
	return tmpval;
}

//  9. ʮ�������ַ��� �� �������ַ���  ������"0xffffffff" �� "1111...11"��
//  ����hex�������ʮ�������ַ���
//  ����bin������Ķ������ַ���
//  ����bitnum���������ַ���λ������λ0���
char *HexStrToBinStr(char *hex, char *bin, int bitnum)
{
	int i,j=0,k=0;
	char bins[100]={'\0'};
	char tmp[100]={'\0'};
	for(i=2;i<10;i++)   //ȥ��ʮ������ǰ���0x
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

// 10. �������ַ��� �� ʮ�������ַ���  ������"1111" �� "0xf"��
//  ����bin������Ķ������ַ���
//  ����hex�������ʮ�������ַ���
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
		int modlen=4-mod;//�����ֲ�������λ������ʱ��ǰ����0����
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
 * Hex�� - Hex����
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
			printf("ERROR A0000 : ʮ�������������������ֵ��ַ����ء�:%c\n", *hex);
			exit(-1);
		}
		weight *= 16;
		i--;
	}
	return sum;
}

/*
 * ÿ8���ַ����һ���ո�
 * �ٴ�ǿ�����뱣֤srcָ��Ŀռ��㹻��
 */
void InsertSpace(char *src) {
	int len = (int)strlen(src);
	int spaceNum = len >> 3; // ����Ŀո���
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
/*ȥ��ʮ������ǰ���0x*/
void Delete0x(char *hex) {
	char temp[100];
	strmid(hex, 2, (int)strlen(hex)-1, temp);
	strcpy(hex, temp);
}

#endif