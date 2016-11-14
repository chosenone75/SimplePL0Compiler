/************  PL0.c  *************/
// pl0 compiler source code

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "set.h"
#include "pl0.h"

//////////////////////////////////////////////////////////////////////
// print error message.
//输出报错信息
void error(int n)
{
	int i;
	printf("      ");
	for (i = 1; i <= cc - 1; i++)
		printf(" ");
	printf("^\n");
	printf("Error %3d: %s\n", n, err_msg[n]);
	err++;
} // error

//////////////////////////////////////////////////////////////////////
//读取一个字符
//每次读入一行进行处理  对应于输入缓冲区？ ：（
void getch(void)
{
	if (cc == ll)
	{

		linenum++;

		if (feof(infile))
		{
			printf("\nPROGRAM INCOMPLETE\n");
			exit(1);
		}
		ll = cc = 0;
		printf("%5d  ", linenum);
		while (!feof(infile) && (ch = getc(infile))!='\n')
		{
			printf("%c", ch);
			line[++ll] = ch;
		} // while
		printf("\n");
		line[++ll] = ' ';	
	}
	ch = line[++cc];
} // getch

/////////////////////////////////////////////
//回退一个字符
void back(char pre){
	cc--;
	ch = pre;
}

//////////////////////////////////////////////////////////////////////
// gets a symbol from input stream.
int isComment = 0;
void getsym(void)
{
	//词法分析器，实现识别各个词法单元的有限自动机
	//以下

	int i, k;
	char a[MAXIDLEN + 1];
	char pre;


	while (ch == ' ' || ch == '\t' || (int)ch == 9) //changedbyran 略过一些奇怪的非打印字符：)，可能是记事本的原因
		getch();

	// 添加注释功能 changedbyran
	while(ch == '(' || isComment){
		pre = ch;
		getch();
		if(pre == '(' && !isComment){
			if(ch == '*')
				isComment = 1;//标志进入注释
			else{
				//回退字符 (
				back(pre);
				break;//跳出此循环
			}
		}
		if(isComment && (pre == '*' && ch == ')')){
			isComment = 0;
			//printf("%s","Comment Complete");
			getch();
		}	 
	}


	while (ch == ' ' || ch == '\t' || (int)ch == 9) //changedbyran 略过一些奇怪的非打印字符：)，可能是记事本的原因
		getch();

	if (isalpha(ch))
	{ 
		// symbol is a reserved word or an identifier.
		k = 0;
		do
		{
			if (k < MAXIDLEN)
				a[k++] = ch;
			getch();
		}
		while (isalpha(ch) || isdigit(ch));//字母或下划线开头的标识符，可能是保留字也可能是标识符
		a[k] = 0;

		//至本方法结尾判断具体是标识符或者保留字

		strcpy(id, a);//将标识符内容赋予全局变量id
		word[0] = id;
		i = NRW;
		while (strcmp(id, word[i--]));
		if (++i)
			sym = wsym[i]; // symbol is a reserved word
		else
			sym = SYM_IDENTIFIER;   // symbol is an identifier
	}
	else if (isdigit(ch))
	{ 
		// symbol is a number.
		//当前词法单元为常数值
		//全局变量num保存当前读取的常数的十进制数值

		k = num = 0;
		sym = SYM_NUMBER;
		do
		{
			num = num * 10 + ch - '0';
			k++;
			getch();
		}
		while (isdigit(ch));
		if (k > MAXNUMLEN)
			error(25);     // The number is too great.
	}
	else if (ch == ':')
	{
		//判断：=    手动微笑：）
		getch();
		if (ch == '=')
		{
			sym = SYM_BECOMES; // :=
			getch();
		}
		else
		{
			sym = SYM_COLON;       // illegal? changedbyran
		}
	}
	else if (ch == '>')
	{
		//识别 >=,或> 最长的原则
		getch();
		if (ch == '=')
		{
			sym = SYM_GEQ;     // >=
			getch();
		}
		else
		{
			sym = SYM_GTR;     // >
		}
	}
	else if (ch == '<')
	{
		//同上，识别<或者<=,<>
		getch();
		if (ch == '=')
		{
			sym = SYM_LEQ;     // <=
			getch();
		}
		else if (ch == '>')
		{
			sym = SYM_NEQ;     // <>
			getch();
		}
		else
		{
			sym = SYM_LES;     // <
		}
	}
	else
	{ // other tokens
		//其他符号，诸如运算符等
		i = NSYM;
		csym[0] = ch;
		while (csym[i--] != ch);
		if (++i)
		{
			sym = ssym[i];
			getch();

			//changedbyran 判断++ 或者 --
			if(sym == SYM_PLUS && ch == '+'){
				sym = SYM_PLUSPLUS;
				getch();
			}
			if(sym == SYM_MINUS && ch == '-'){
				sym = SYM_MINUSMINUS;
				getch();
			}
		}
		else
		{
			printf("%d",ch);//changedbyran  输出不能识别的字符
			printf("Fatal Error: Unknown character.\n");
			exit(1);
		}
	}
} // getsym

//////////////////////////////////////////////////////////////////////
// generates (assembles) an instruction.
void gen(int x, int y, int z)
{
	if (cx > CXMAX)
	{
		printf("Fatal Error: Program too long.\n");
		exit(1);
	}
	code[cx].f = x;
	code[cx].l = y;
	code[cx++].a = z;//增加cx的值 指示下一条指令的地址
} // gen

//////////////////////////////////////////////////////////////////////
// tests if error occurs and skips all symbols that do not belongs to s1 or s2.
void test(symset s1, symset s2, int n)
{
	symset s;

	if (! inset(sym, s1))
	{
		error(n);
		printf("%s %d\n",id,sym);
		s = uniteset(s1, s2);
		while(! inset(sym, s))
			getsym();
		destroyset(s);
	}
} // test

//////////////////////////////////////////////////////////////////////
int dx;  // data allocation index

// enter object(constant, variable or procedre) into table.
void enter(int kind)
{
	mask* mk;
	tx++;
	strcpy(table[tx].name, id);
	//printf("%s\n",id);
	table[tx].kind = kind;
	switch (kind)
	{
	case ID_CONSTANT:
		if (num > MAXADDRESS)
		{
			error(25); // The number is too great.
			num = 0;
		}
		table[tx].value = num;
		break;
	case ID_INTEGER:
		mk = (mask*) &table[tx];
		mk->level = level;
		mk->address = dx++;
		break;
	case ID_BOOLEAN:
		mk = (mask*) &table[tx];
		mk->level = level;
		mk->address = dx++;
		break;
	case ID_PROCEDURE:
		mk = (mask*) &table[tx];
		mk->level = level;
		break;
	} // switch
} // enter

//////////////////////////////////////////////////////////////////////
//在符号表执行查找id的lookup操作
// locates identifier in symbol table.
int position(char* id)
{
	int i;
	strcpy(table[0].name, id);
	i = tx + 1;
	while (strcmp(table[--i].name, id) != 0);
	return i;
} // position

//////////////////////////////////////////////////////////////////////
void constdeclaration()
{
	if (sym == SYM_IDENTIFIER)
	{
		getsym();
		if (sym == SYM_EQU || sym == SYM_BECOMES)
		{
			if (sym == SYM_BECOMES)
				error(1); // Found ':=' when expecting '='.
			getsym();
			if (sym == SYM_NUMBER)
			{
				enter(ID_CONSTANT);
				getsym();
			}
			else
			{
				error(2); // There must be a number to follow '='.
			}
		}
		else
		{
			error(3); // There must be an '=' to follow the identifier.
		}
	}
	else
		error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
} // constdeclaration

//////////////////////////////////////////////////////////////////////
/*
VarDecl  -> var VarDecl  {VarDecl } 
VarDec  -> ident : Type//此处我的理解与原文有出入，不想再改了：（
*/
void vardeclaration(void)
{
	if (sym == SYM_IDENTIFIER)
	{
		//保存当前id
		char idTobeEntered[MAXIDLEN + 1];
		strcpy(idTobeEntered,id);

		//enter(ID_VARIABLE);
		getsym();
		if(sym == SYM_COLON){
			getsym();
			if(sym == SYM_BOOLEAN || sym == SYM_INTEGER){
				//恢复id 并填入
				strcpy(id,idTobeEntered);

				if(sym == SYM_BOOLEAN) enter(ID_BOOLEAN);
				else enter(ID_INTEGER);
				getsym();
			}
			else{
				error(27);
				getsym();//changedbyran just for test 错误提示
			}
		}
		else{
			error(26);//必须声明变量的类型
			getsym();//changedbyran just for test 错误提示
		}
	}
	else
	{
		error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
	}
} // vardeclaration

//////////////////////////////////////////////////////////////////////
void listcode(int from, int to)
{
	int i;
	printf("\n");
	for (i = from; i < to; i++)
	{
		printf("%5d %s\t%d\t%d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
	}
	printf("\n");
} // listcode

//////////////////////////////////////////////////////////////////////

/*
文法表示：
Factor ->id | number | ( Exp ) | not Factor | ident [ ( ActParal ) ] | true | false| ++id | --id;
*/

void factor(symset fsys)
{
	void expression(symset);//changedbyran MDZZ
	int i;
	symset set;


	test(facbegsys, fsys, 24); // The symbol can not be as the beginning of an expression.

	while (inset(sym, facbegsys))
	{
		if (sym == SYM_IDENTIFIER)
		{
			if ((i = position(id)) == 0)
			{
				error(11); // Undeclared identifier.
			}
			else
			{
				switch (table[i].kind)
				{
					mask* mk;
				case ID_CONSTANT:
					gen(LIT, 0, table[i].value);
					break;
				case ID_INTEGER:
					mk = (mask*) &table[i];
					gen(LOD, level - mk->level, mk->address);
					break;
				case ID_BOOLEAN:
					mk = (mask*) &table[i];
					gen(LOD, level - mk->level, mk->address);
					break;
				case ID_PROCEDURE:
					error(21); // Procedure identifier can not be in an expression.
					break;
				} // switch
			}
			getsym();
		}
		else if (sym == SYM_NUMBER)
		{
			if (num > MAXADDRESS)
			{
				error(25); // The number is too great.
				num = 0;
			}
			gen(LIT, 0, num);
			getsym();
		}else if(sym == SYM_TRUE){//true
			gen(LIT, 0, 1);
			getsym();
		}else if(sym == SYM_FALSE){
			gen(LIT, 0, 0);
			getsym();
		}else if(sym == SYM_NOT){
			getsym();
			factor(fsys);
			gen(OPR,0,OPR_NOT);
			getsym();
		}
		else if (sym == SYM_LPAREN)
		{
			getsym();
			set = uniteset(createset(SYM_RPAREN, SYM_NULL), fsys);
			expression(set);
			destroyset(set);
			if (sym == SYM_RPAREN)
			{
				getsym();
			}
			else
			{
				error(22); // Missing ')'.
			}
		}else if(sym == SYM_PLUSPLUS){
			mask* mk;
			getsym();
			if(sym != SYM_IDENTIFIER) error(30);//maybebug
			else if(!(i = position(id))) error(11);
			else{
				mk = (mask*) &table[i];
				if(mk->kind == ID_PROCEDURE) error(21);
				else{//标识符为变量的情况
					gen(LOD,level - mk->level,mk->address);//++id的指令流程;
					gen(LIT,0,1);
					gen(OPR,0,OPR_ADD);
					gen(STO,level - mk->level,mk->address);
					gen(LOD,level - mk->level,mk->address);
					getsym();
				}
			}
		}else if(sym == SYM_MINUSMINUS){
		    mask* mk;
			getsym();
			if(sym != SYM_IDENTIFIER) error(30);//maybebug
			else if(!(i = position(id))) error(11);
			else{
				mk = (mask*) &table[i];
				if(mk->kind == ID_PROCEDURE) error(21);
				else{//标识符为变量的情况
					gen(LOD,level - mk->level,mk->address);//--id的指令流程;
					gen(LIT,0,1);
					gen(OPR,0,OPR_MIN);
					gen(STO,level - mk->level,mk->address);
					gen(LOD,level - mk->level,mk->address);
					getsym();
				}
			}
		}
		test(fsys, createset(SYM_LPAREN, SYM_NULL), 23);
	} // while
} // factor

//////////////////////////////////////////////////////////////////////
/*
文法表示:
Term->Factor{*Factor | / Factor | div Factor | mod Factor | and Factor}   
*/
void term(symset fsys)
{
	int mulop;
	symset set;

	set = uniteset(fsys, createset(SYM_TIMES, SYM_SLASH, SYM_AND,SYM_NULL));//changedbyran
	factor(set);
	while (sym == SYM_TIMES || sym == SYM_SLASH || sym == SYM_AND)
	{
		while (sym == SYM_TIMES || sym == SYM_SLASH)
		{
			mulop = sym;
			getsym();
			factor(set);
			if (mulop == SYM_TIMES)
			{
				gen(OPR, 0, OPR_MUL);
			}
			else
			{
				gen(OPR, 0, OPR_DIV);
			}
		}
		if(sym == SYM_AND){
			getsym();
			factor(set);
			while (sym == SYM_TIMES || sym == SYM_SLASH)
			{
				mulop = sym;
				getsym();
				factor(set);
				if (mulop == SYM_TIMES)
				{
					gen(OPR, 0, OPR_MUL);
				}
				else
				{
					gen(OPR, 0, OPR_DIV);
				}
			}//whileinif
			gen(OPR,0,OPR_AND);
		}//andif
	} // while
	destroyset(set);
} // term

//////////////////////////////////////////////////////////////////////
/*
文法表示:

Condition -> odd Exp | Exp RelOp Exp

Exp  ->  [+ | - ] Term {+ Term | - Term | or Term}

Term->Factor{*Factor | / Factor | div Factor | mod Factor | and Factor}   
Factor ->id | number | ( Exp ) | not Factor | ident [ ( ActParal ) ] | true | false

*/   
void expression(symset fsys)
{
	int addop;
	symset set;

	set = uniteset(fsys, createset(SYM_PLUS, SYM_MINUS, SYM_OR,SYM_NULL));//changedbyran
	if (sym == SYM_PLUS || sym == SYM_MINUS)
	{
		addop = sym;
		getsym();
		term(set);
		if (addop == SYM_MINUS)
		{
			gen(OPR, 0, OPR_NEG);
		}
	}
	else
	{
		term(set);
	}//文法第一部分

	while (sym == SYM_PLUS || sym == SYM_MINUS || sym == SYM_OR)
	{
		while(sym == SYM_PLUS || sym == SYM_MINUS){
			addop = sym;
			getsym();
			term(set);
			if (addop == SYM_PLUS)
			{
				gen(OPR, 0, OPR_ADD);
			}
			else
			{
				gen(OPR, 0, OPR_MIN);
			}
		}//while1
		if(sym == SYM_OR){
			getsym();
			if (sym == SYM_PLUS || sym == SYM_MINUS)
			{
				addop = sym;
				getsym();
				term(set);
				if (addop == SYM_MINUS)
				{
					gen(OPR, 0, OPR_NEG);
				}
			}
			else
			{
				term(set);
			}//文法第一部分
			while(sym == SYM_PLUS || sym == SYM_MINUS){
				addop = sym;
				getsym();
				term(set);
				if (addop == SYM_PLUS)
				{
					gen(OPR, 0, OPR_ADD);
				}
				else
				{
					gen(OPR, 0, OPR_MIN);
				}
			}//while in if
			//生成or运算
			gen(OPR,0,OPR_OR);
		}
	} // while
	destroyset(set);
} // expression

//////////////////////////////////////////////////////////////////////
/*
Condition -> odd Exp | Exp RelOp Exp //不用改动
Exp  ->  [+ | - ] Term {+ Term | - Term | or Term}
Term->Factor{*Factor | / Factor | div Factor | mod Factor | and Factor}   
Factor ->id | number | ( Exp ) | not Factor | ident [ ( ActParal ) ] | true | false
*/

void condition(symset fsys)
{
	int relop;
	symset set;

	if (sym == SYM_ODD)
	{
		getsym();
		expression(fsys);
		gen(OPR, 0, OPR_ODD);
	}
	else
	{
		set = uniteset(relset, fsys);//添加终结符
		expression(set);
		destroyset(set);
		if (! inset(sym, relset))
		{
			error(20);
		}
		else
		{
			relop = sym;
			getsym();
			expression(fsys);
			switch (relop)
			{
			case SYM_EQU:
				gen(OPR, 0, OPR_EQU);
				break;
			case SYM_NEQ:
				gen(OPR, 0, OPR_NEQ);
				break;
			case SYM_LES:
				gen(OPR, 0, OPR_LES);
				break;
			case SYM_GEQ:
				gen(OPR, 0, OPR_GEQ);
				break;
			case SYM_GTR:
				gen(OPR, 0, OPR_GTR);
				break;
			case SYM_LEQ:
				gen(OPR, 0, OPR_LEQ);
				break;
			} // switch
		} // else
	} // else
} // condition
//ε
//////////////////////////////////////////////////////////////////////
/*
文法表示：
Stmt   -> id := Exp 
| if Exp then Stmt 
| if Exp then Stmt else Stmt 
| begin Stmt {; Stmt } end 
| while Exp do Stmt 
| exit 
| call ident [ ( ActParal ) ] 
| write ( Exp [, Exp ] ) 
| read (IdentRef [, IdentRef ] )
| for id := Exp to Exp do Stmt
| for id := Exp downto Exp do Stmt //各自循环步长为1或-1
| ++id|--id|id++|id--;
*/
void statement(symset fsys)
{
	int i, cx1, cx2;
	symset set1, set;

	if (sym == SYM_IDENTIFIER)
	{ 
		// variable assignment
		mask* mk;
		if (! (i = position(id)))
		{
			error(11); // Undeclared identifier.
		}
		else if (table[i].kind != ID_INTEGER && table[i].kind != ID_BOOLEAN)
		{
			error(12); // Illegal assignment.
			i = 0;
		}
		getsym();
		if (sym == SYM_BECOMES)
		{
			getsym();
		}
		else
		{
			error(13); // ':=' expected.
		}

		expression(fsys);

		//此处gen目标代码 增加类型转换语句 强转 例如 int->boolean 根据id的类型进行判断
		//类型检查

		mk = (mask*) &table[i];
		if (i)
		{
			gen(STO, level - mk->level, mk->address);//level - mk->level 即为 level_diff 层次差
		}
	}
	else if (sym == SYM_CALL)
	{ 
		// procedure call
		getsym();
		if (sym != SYM_IDENTIFIER)
		{
			error(14); // There must be an identifier to follow the 'call'.
		}
		else
		{
			if (! (i = position(id)))
			{
				error(11); // Undeclared identifier.
			}
			else if (table[i].kind == ID_PROCEDURE)
			{
				mask* mk;
				mk = (mask*) &table[i];
				gen(CAL, level - mk->level, mk->address);
			}
			else
			{
				error(15); // A constant or variable can not be called. 
			}
			getsym();
		}
	} 
	else if (sym == SYM_IF)
	{ // if statement
		getsym();
		set1 = createset(SYM_THEN, SYM_DO, SYM_NULL);
		set = uniteset(set1, fsys);
		condition(set);
		destroyset(set1);
		destroyset(set);
		if (sym == SYM_THEN)
		{
			getsym();
		}
		else
		{
			error(16); // 'then' expected.
		}

		cx1 = cx;
		gen(JPC, 0, 0);
		set1 = createset(SYM_ELSE,SYM_NULL);//changedbyran
		set = uniteset(set1, fsys);
		statement(set);

		if(sym == SYM_ELSE){
			getsym();
			cx2 = cx;
			code[cx1].a = cx+1;//回填
			gen(JMP,0,0);
			statement(fsys);
			code[cx2].a = cx;//回填
		}else{
			code[cx1].a = cx;//进行回填
		}
		destroyset(set1);
		destroyset(set);
	}
	else if (sym == SYM_BEGIN)
	{ 
		// block 语句序列
		getsym();
		set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
		set = uniteset(set1, fsys);
		statement(set);
		while (sym == SYM_SEMICOLON || inset(sym, statbegsys))
		{
			if (sym == SYM_SEMICOLON)
			{
				getsym();
			}
			else
			{
				error(10);
			}
			statement(set);
		} // while
		destroyset(set1);
		destroyset(set);
		if (sym == SYM_END)
		{
			getsym();
		}
		else
		{
			error(17); // ';' or 'end' expected.
		}
	}
	else if (sym == SYM_WHILE)
	{ // while statement
		cx1 = cx;
		getsym();
		set1 = createset(SYM_DO, SYM_NULL);
		set = uniteset(set1, fsys);
		condition(set);
		destroyset(set1);
		destroyset(set);
		cx2 = cx;
		gen(JPC, 0, 0);
		if (sym == SYM_DO)
		{
			getsym();
		}
		else
		{
			error(18); // 'do' expected.
		}

		statement(fsys);
		gen(JMP, 0, cx1);
		code[cx2].a = cx;
	}else if(sym == SYM_PLUSPLUS)
	{
		//++运算
		mask* mk;
		getsym();
		if(sym == SYM_IDENTIFIER){
			if (! (i = position(id)))
			{
				error(11); // Undeclared identifier.
			}
			else if (table[i].kind != ID_INTEGER)
			{
				error(28); // Illegal assignment.
				i = 0;
			}else{
				getsym();
				mk = (mask*) &table[i];
				if (i)
				{
					gen(LOD,level - mk->level,mk->address);//找到变量地址，将其值入栈
					gen(LIT,0,1);//将常数1取到栈顶
					gen(OPR,0,OPR_ADD);
					gen(STO, level - mk->level, mk->address);//level - mk->level 即为 level_diff 层次差
				}
			}	
		}else{
			error(29);
		}
	}else if(sym == SYM_MINUSMINUS){
		//--运算
		mask* mk;
		getsym();
		if(sym == SYM_IDENTIFIER){
			if (!(i = position(id)))
			{
				error(11); // Undeclared identifier.
			}
			else if (table[i].kind != ID_INTEGER)
			{
				error(28); // Illegal assignment.
				i = 0;
			}else{
				getsym();
				mk = (mask*) &table[i];
				if (i)
				{
					gen(LOD,level - mk->level,mk->address);//找到变量地址，将其值入栈
					gen(LIT,0,1);//将常数1取到栈顶
					gen(OPR,0,OPR_MIN);
					gen(STO, level - mk->level, mk->address);//level - mk->level 即为 level_diff 层次差
				}
			}	
		}else{
			error(29);
		}
	}else if(sym == SYM_FOR){
		mask* mk;
		getsym();
		if(sym == SYM_IDENTIFIER){
			if(!(i = position(id))){
				error(11);
			}else if(table[i].kind != ID_BOOLEAN && table[i].kind != ID_INTEGER){
				error(31);
				i = 0;
			}else{
				getsym();
				mk = (mask*)&table[i];
				/*
				for id := Exp to Exp do Stmt
				for id := Exp downto Exp do Stmt
				*/
				if(sym != SYM_BECOMES) error(13);
				getsym();//maybebug
				set1 = createset(SYM_DOWNTO, SYM_TO, SYM_NULL);
				set = uniteset(set1, fsys);
				expression(set);
				destroyset(set1);
				destroyset(set);
				gen(STO,level - mk->level,mk->address);//保存变量的初值

				if(sym != SYM_TO && sym != SYM_DOWNTO){//这种方式对应错误处理方法中的将终结符出栈
					error(33); 
				}
				else if(sym == SYM_TO){
					getsym();
					cx1 = cx;
					gen(LOD,level - mk->level,mk->address);//将变量移至栈顶
					set1 = createset(SYM_DO, SYM_NULL);
					set = uniteset(set1, fsys);
					expression(set);
					destroyset(set1);
					destroyset(set);
					gen(OPR,0,OPR_LEQ);//比较
					cx2 = cx;
					gen(JPC,0,0);//跳出循环指令

					if(sym != SYM_DO) error(34);
					else{
						getsym();
						statement(fsys);
						//递增循环变量的值
						gen(LOD,level - mk->level,mk->address);
						gen(LIT,0,1);
						gen(OPR,0,OPR_ADD);
						gen(STO,level - mk->level,mk->address);
						gen(JMP,0,cx1);//跳转到循环开头进行测试
						code[cx2].a=cx;
					}
				}else if(sym == SYM_DOWNTO){
					getsym();
					cx1 = cx;
					gen(LOD,level - mk->level,mk->address);//将变量移至栈顶
					set1 = createset(SYM_DO, SYM_NULL);
					set = uniteset(set1, fsys);
					expression(set);
					destroyset(set1);
					destroyset(set);
					gen(OPR,0,OPR_GEQ);//比较
					cx2 = cx;
					gen(JPC,0,0);//跳出循环指令

					if(sym != SYM_DO) error(34);
					else{
						getsym();
						statement(fsys);
						//递减循环变量的值
						gen(LOD,level - mk->level,mk->address);
						gen(LIT,0,1);
						gen(OPR,0,OPR_MIN);
						gen(STO,level - mk->level,mk->address);
						gen(JMP,0,cx1);//跳转到循环开头进行测试
						code[cx2].a=cx;
					}
				}
			}
		}else{
			error(30);
		}
	}else if(sym == SYM_EXIT){

	}
	test(fsys, phi, 19);
} // statement

//////////////////////////////////////////////////////////////////////

void block(symset fsys)
{
	int cx0; // initial code index
	mask* mk;
	int block_dx;//当前block待分配的空间(即声明的变量的个数)
	int savedTx;//顾名思义 用于嵌套过程
	symset set1, set;

	dx = 3;//三个地址单元 分别存放 栈顶位置，前一个栈顶指针，前一个过程的PC值 DL,SL,DA
	block_dx = dx;
	mk = (mask*) &table[tx];
	mk->address = cx;
	gen(JMP, 0, 0);
	if (level > MAXLEVEL)
	{
		error(32); // There are too many levels.
	}
	do
	{
		if (sym == SYM_CONST)
		{ 
			// constant declarations
			getsym();
			do
			{
				constdeclaration();
				while (sym == SYM_COMMA)
				{
					getsym();
					constdeclaration();
				}
				if (sym == SYM_SEMICOLON)
				{
					getsym();
				}

				else
				{
					error(5); // Missing ',' or ';'.
				}
			}
			while (sym == SYM_IDENTIFIER);
		} // if

		if (sym == SYM_VAR)
		{ 
			// variable declarations
			getsym();
			do
			{
				vardeclaration();
				while (sym == SYM_COMMA)
				{
					getsym();
					vardeclaration();
				}
				if (sym == SYM_SEMICOLON)
				{
					getsym();
				}
				else
				{
					error(5); // Missing ',' or ';'.
				}
			}
			while (sym == SYM_IDENTIFIER);//蜜汁循环 现在还不能理解

			block_dx = dx;//changedbyran 分配声明变量的地址空间
		} // if

		while (sym == SYM_PROCEDURE)
		{ 
			// procedure declarations
			getsym();
			if (sym == SYM_IDENTIFIER)
			{
				enter(ID_PROCEDURE);
				getsym();
			}
			else
			{
				error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
			}

			if (sym == SYM_SEMICOLON)
			{
				getsym();
			}
			else
			{
				error(5); // Missing ',' or ';'.
			}

			level++;//嵌套层次

			savedTx = tx;
			set1 = createset(SYM_SEMICOLON, SYM_NULL);
			set = uniteset(set1, fsys);//随着子程序的深入，添加一些符号进行错误处理
			block(set);
			destroyset(set1);
			destroyset(set);
			tx = savedTx;
			level--;

			if (sym == SYM_SEMICOLON)
			{
				getsym();
				/*
				set1 = createset(SYM_PROCEDURE, SYM_NULL);
				set = uniteset(statbegsys, set1);
				test(set, fsys, 6);

				destroyset(set1);
				destroyset(set);
				*/
			}
			else
			{
				error(5); // Missing ',' or ';'.
			}
		} // while
		set1 = createset(SYM_IDENTIFIER, SYM_NULL);
		set = uniteset(statbegsys, set1);
		test(set, declbegsys, 7);
		destroyset(set1);
		destroyset(set);
	}
	while (inset(sym, declbegsys));

	code[mk->address].a = cx;
	mk->address = cx;
	cx0 = cx;
	gen(INT, 0, block_dx);
	set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
	set = uniteset(set1, fsys);
	statement(set);
	destroyset(set1);
	destroyset(set);
	gen(OPR, 0, OPR_RET); // return
	test(fsys, phi, 8); // test for error: Follow the statement is an incorrect symbol.
	listcode(cx0, cx);
} // block

//////////////////////////////////////////////////////////////////////

/*
解释运行相关
*/

//寻找基地址 （栈顶地址）
int base(int stack[], int currentLevel, int levelDiff)
{
	int b = currentLevel;
	while (levelDiff--)
		b = stack[b];
	return b;
} // base

//////////////////////////////////////////////////////////////////////
// interprets and executes codes.
/*
（1）LIT           将常数置于栈顶 
（2）LOD        将变量值置于栈顶 
（3）STO         将栈顶的值赋与某变量 
（4）CAL         用于过程调用的指令 
（5）INT          在数据栈中分配存贮空间 
（6）JMP, JPC  用于if, while语句的条件或无条件控制转移指令
（7）OPR         一组算术或逻辑运算指令 
*/

void interpret()
{
	int pc;        // program counter 程序计数器

	//自定义栈结构
	int stack[STACKSIZE];
	int top;       // top of stack

	int b;         // program, base, and top-stack register //基址寄存器
	instruction i; // instruction register // 指令寄存器

	printf("Begin executing PL/0 program.\n");

	pc = 0;
	b = 1;
	top = 3;
	stack[1] = stack[2] = stack[3] = 0;
	do
	{
		i = code[pc++];
		switch (i.f)
		{
		case LIT:
			stack[++top] = i.a;
			break;
		case OPR:
			switch (i.a) // operator
			{
			case OPR_RET:
				top = b - 1;
				pc = stack[top + 3];
				b = stack[top + 2];
				break;
			case OPR_NEG:
				stack[top] = -stack[top];
				break;
			case OPR_ADD:
				top--;
				stack[top] += stack[top + 1];
				break;
			case OPR_MIN:
				top--;
				stack[top] -= stack[top + 1];
				break;
			case OPR_MUL:
				top--;
				stack[top] *= stack[top + 1];
				break;
			case OPR_DIV:
				top--;
				if (stack[top + 1] == 0)
				{
					fprintf(stderr, "Runtime Error: Divided by zero.\n");
					fprintf(stderr, "Program terminated.\n");
					continue;
				}
				stack[top] /= stack[top + 1];
				break;
			case OPR_ODD:
				stack[top] %= 2;
				break;
			case OPR_EQU:
				top--;
				stack[top] = stack[top] == stack[top + 1];
				break;
			case OPR_NEQ:
				top--;
				stack[top] = stack[top] != stack[top + 1];
				break;
			case OPR_LES:
				top--;
				stack[top] = stack[top] < stack[top + 1];
				break;
			case OPR_GEQ:
				top--;
				stack[top] = stack[top] >= stack[top + 1];
				break;
			case OPR_GTR:
				top--;
				stack[top] = stack[top] > stack[top + 1];
				break;
			case OPR_LEQ:
				top--;
				stack[top] = stack[top] <= stack[top + 1];
				break;
				//changedbyran
			case OPR_AND:
				top--;
				stack[top] = stack[top] && stack[top+1];
				break;
			case OPR_OR:
				top--;
				stack[top] = stack[top] || stack[top+1];
				break;
			case OPR_NOT:
				stack[top] = stack[top]?0:1;
				break;
			} // switch
			break;
		case LOD:
			stack[++top] = stack[base(stack, b, i.l) + i.a];
			break;
		case STO:
			stack[base(stack, b, i.l) + i.a] = stack[top];
			printf("%d\n", stack[top]);
			top--;
			break;
		case CAL:
			stack[top + 1] = base(stack, b, i.l);
			// generate new block mark
			stack[top + 2] = b;// 上一个栈帧的首地址
			stack[top + 3] = pc;//上一个过程的执行位置
			b = top + 1;//更新当前栈顶指针
			pc = i.a;//更新程序计数器内容
			break;
		case INT:
			top += i.a;
			break;
		case JMP:
			pc = i.a;
			break;
		case JPC:
			if (stack[top] == 0)
				pc = i.a;
			top--;
			break;
		} // switch
	}
	while (pc);

	printf("End executing PL/0 program.\n");
} // interpret

//////////////////////////////////////////////////////////////////////
void main()
{
	FILE* hbin;
	char s[80];
	int i;
	symset set, set1, set2;

	printf("Please input source file name: "); // get file name to be compiled
	scanf("%s", s);
	if ((infile = fopen(s, "r")) == NULL)
	{
		printf("File %s can't be opened.\n", s);
		exit(1);
	}

	phi = createset(SYM_NULL);
	relset = createset(SYM_EQU, SYM_NEQ, SYM_LES, SYM_LEQ, SYM_GTR, SYM_GEQ,SYM_NULL);

	// create begin symbol sets
	declbegsys = createset(SYM_CONST, SYM_VAR, SYM_PROCEDURE, SYM_NULL);
	statbegsys = createset(SYM_BEGIN, SYM_CALL, SYM_IF, SYM_WHILE, SYM_NULL,SYM_PLUSPLUS,SYM_MINUSMINUS);//changedbyran
	facbegsys = createset(SYM_IDENTIFIER, SYM_NUMBER, SYM_LPAREN, SYM_TRUE,SYM_FALSE,SYM_NOT,SYM_PLUSPLUS,SYM_MINUSMINUS,SYM_NULL);

	err = cc = cx = ll = linenum = 0; // initialize global variables
	ch = ' ';
	kk = MAXIDLEN;

	getsym();

	set1 = createset(SYM_PERIOD, SYM_NULL);//follow集
	set2 = uniteset(declbegsys, statbegsys);//重要的不可忽视的符号集
	set = uniteset(set1, set2);
	block(set);//语法分析开始
	destroyset(set1);
	destroyset(set2);
	destroyset(set);
	destroyset(phi);
	destroyset(relset);
	destroyset(declbegsys);
	destroyset(statbegsys);
	destroyset(facbegsys);

	if (sym != SYM_PERIOD)
		error(9); // '.' expected.
	if (err == 0)
	{
		hbin = fopen("hbin.txt", "w");
		for (i = 0; i < cx; i++)
			fwrite(&code[i], sizeof(instruction), 1, hbin);
		fclose(hbin);
	}
	if (err == 0)
		interpret();
	else
		printf("There are %d error(s) in PL/0 program.\n", err);
	listcode(0, cx);
} // main    END OF PL0.c
