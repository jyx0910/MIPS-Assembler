# MIPS-Assembler

一个MIPS汇编器。

An MIPS Assembler designed for Computer System Design Course.

## 支持指令集

支持以下58条指令：

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
"j",          "jal",        "nop"

## 汇编语法说明


示例：
\#注释可以单独占一行，也可以置于行尾
\#所有地址均为字节地址，采用大端模式
\#数据段定义伪指令如下
.data 0x0004 # 数据段起始地址必须是十六进制数，且保证在64kB范围内，否则汇编器报错
    # 标识符模式 [a-zA-Z_][a-zA-Z_0-9]+
    num1 .word 0x12345678 # 地址0x04，字定义，要求地址必须按字对齐
    num2 .half 0x22223333 # 地址0x08，半字定义，要求地址必须为偶数，取立即数低位两个字节
    num3 .byte 0x44556677 # 地址0x0a，字节定义，地址无要求，取立即数低位字节
    dumm .byte 0xffffffff # 地址0x0b，字节定义，用来向字对齐
    .space 0x01 # 从0x0c开始空出一个字的空间，当前地址必须按字对齐，且终止地址在64k范围内

\#代码段定义伪指令如下
.text 0x0004 # 代码段起始地址必须是十六进制数，且保证在64kB范围内，否则汇编器报错
\#标签必须以@开始，紧跟:，后面可以跟若干空格开始指令行。
\#首条指令必须有@start标签，否则报错
\#助记符、标识符、十六进制数字（除0x的x外）均不区分大小写
\#寄存器名只允许使用寄存器名形式，不允许使用寄存器号
\#下面示例中的空格均代表[ |\t]+
@start:  add  $t0 , $t1 , $t2       # R型指令（三个寄存器）
         div  $t2 , $t3             # R型指令（两个寄存器）
          jr  $ra                   # R型指令（一个寄存器）
         sll  $t1 , $t2 , 2         # R型指令（移位型，Shamt必须是无符号十进制数）
@type_i: ori  $t1 , $t2 , 0xffff    # I型指令（两个寄存器，imme为原码十六进制或无符号十进制）
        bgez  $t1 , -123            # I型指令（一个寄存器，imme为补码十六进制或有符号十进制）
        bgez  $t1 , @start          # I型指令（跳转到标签）
          lw  $t1 , num1( $t2 )     # I型指令（相对寻址，num1为标识符，原码十六进制或无符号十进制）
@type_j:   j  @type_i               # J型指令（跳转到标签）
         jal  num1                  # J型指令（num1为标识符，原码十六进制或无符号十进制）
\#可继续定义代码段
.text 0xFFF8	
          jr  $k0

## 权利声明

Copyright (c) 2016-2017 Mikukonai.

草率之作，本人不对任何人使用本代码造成的任何后果负责。