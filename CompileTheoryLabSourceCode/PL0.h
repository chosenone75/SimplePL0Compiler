/************  PL0.h  *************/

#include <stdio.h>

#define NRW        27    // number of reserved words
#define TXMAX      500    // length of identifier table
#define MAXNUMLEN  14     // maximum number of digits in numbers
#define NSYM       11     // maximum number of symbols in array ssym and csym
#define MAXIDLEN   10     // length of identifiers

#define MAXADDRESS 32767  // maximum address
#define MAXLEVEL   32     // maximum depth of nesting block ���Ƕ�����
#define CXMAX      500    // size of code array

#define MAXSYM     30     // maximum number of symbols  

#define STACKSIZE  1000   // maximum storage
enum symtype
{
	SYM_NULL,
	SYM_IDENTIFIER,
	SYM_NUMBER,
	SYM_PLUS,
	SYM_MINUS,
	SYM_TIMES,
	SYM_SLASH,
	SYM_ODD,
	SYM_EQU,
	SYM_NEQ,
	SYM_LES,
	SYM_LEQ,
	SYM_GTR,
	SYM_GEQ,
	SYM_LPAREN,
	SYM_RPAREN,
	SYM_COMMA,
	SYM_SEMICOLON,
	SYM_PERIOD,
	SYM_BECOMES,
	SYM_BEGIN,
	SYM_END,
	SYM_IF,
	SYM_THEN,
	SYM_WHILE,
	SYM_DO,
	SYM_CALL,
	SYM_CONST,
	SYM_VAR,
	SYM_PROCEDURE,
	/*changedbyran*/
	SYM_INTEGER,
	SYM_BOOLEAN,
	SYM_COLON,
	SYM_TRUE,
	SYM_FALSE,
	SYM_AND,
	SYM_NOT,
	SYM_OR,
	SYM_ELSE,
	SYM_PLUSPLUS,
	SYM_MINUSMINUS,
	SYM_FOR,
	SYM_DOWNTO,
	SYM_TO,
	SYM_EXIT,
	SYM_READ,
	SYM_WRITE,
	SYM_REPEAT,
	SYM_UNTIL
};

enum idtype
{
	ID_CONSTANT, ID_VARIABLE, ID_PROCEDURE,ID_INTEGER,ID_BOOLEAN
};

enum opcode
{
	LIT, OPR, LOD, STO, CAL, INT, JMP, JPC
};

enum oprcode
{
	OPR_RET, OPR_NEG, OPR_ADD, OPR_MIN,
	OPR_MUL, OPR_DIV, OPR_ODD, OPR_EQU,
	OPR_NEQ, OPR_LES, OPR_LEQ, OPR_GTR,
	OPR_GEQ,/* changed by ran*/OPR_AND,OPR_OR,OPR_NOT,OPR_READ,OPR_WRITE
};


typedef struct
{
	int f; // function code
	int l; // level
	int a; // displacement address
} instruction; //Ŀ�����ָ��,������ f,l,a

//////////////////////////////////////////////////////////////////////
char* err_msg[] =
{
/*  0 */    "",
/*  1 */    "Found ':=' when expecting '='.",
/*  2 */    "There must be a number to follow '='.",
/*  3 */    "There must be an '=' to follow the identifier.",
/*  4 */    "There must be an identifier to follow 'const', 'var', or 'procedure'.",
/*  5 */    "Missing ',' or ';'.",
/*  6 */    "Incorrect procedure name.",
/*  7 */    "Statement expected.",
/*  8 */    "Follow the statement is an incorrect symbol.",
/*  9 */    "'.' expected.",
/* 10 */    "';' expected.",
/* 11 */    "�����ʶ����û����",
/* 12 */    "Illegal assignment.",
/* 13 */    "':=' expected.",
/* 14 */    "There must be an identifier to follow the 'call'.",
/* 15 */    "A constant or variable can not be called.",
/* 16 */    "'then' expected.",
/* 17 */    "';' or 'end' expected.",
/* 18 */    "'do' expected.",
/* 19 */    "Incorrect symbol.",
/* 20 */    "Relative operators expected.",
/* 21 */    "Procedure identifier can not be in an expression.",
/* 22 */    "Missing ')'.",
/* 23 */    "The symbol can not be followed by a factor.",
/* 24 */    "The symbol can not be as the beginning of an expression.",
/* 25 */    "The number is too great.",
/* 26 */    "����������������������~ ",
/* 27 */    "��������֪����˵����ɶ���ͣ���",
/* 28 */    "���ڴ��������ı���Ϊ���ͱ���: )",
/* 29 */    "�����и����ָ���++��--: )",
/* 30 */    "�����и���ʶ��: )",
/* 31 */    "����Ҫ������: )",
/* 32 */    "There are too many levels.",
/* 33 */    "��Ӧ���и�downto����to : ) ",
/* 34 */    "����for��д�˸�do : ) ",
/* 35 */    "ȱ�������� �ֶ�΢Ц: ) ",
/* 36 */    "ֻ�ܶ������������� : ) ",
/* 37 */    "ȱ�������� �ֶ�΢Ц: ) ",
/* 38 */    "����ȷ��������� : ) "
};
//////////////////////////////////////////////////////////////////////

char ch;         // last character read �����ȡ��һ���ַ�
int  sym;        // last symbol read
char id[MAXIDLEN + 1]; // last identifier read
int  num;        // last number read
int  cc;         // character count
int  ll;         // line length
int  kk;
int  err;
int  cx;         // index of current instruction to be generated. ��nextquad
int  level = 0;
int  tx = 0;
int linenum = 0;

char line[80];//���ƻ�����������

instruction code[CXMAX];

//�˴����Ż�����ĳ��۰������߲�ѯЧ�ʡ�
char* word[NRW + 1] =
{
	"", /* place holder */
	"begin", "call", "const", "do", "end","if",
	"odd", "procedure", "then", "var", "while","integer"
	,"boolean","true","false","and","not","or","else","for","to","downto","exit","read","write","repeat","until"
};

int wsym[NRW + 1] =
{
	SYM_NULL, SYM_BEGIN, SYM_CALL, SYM_CONST, SYM_DO, SYM_END,
	SYM_IF, SYM_ODD, SYM_PROCEDURE, SYM_THEN, SYM_VAR, SYM_WHILE,SYM_INTEGER,SYM_BOOLEAN,
	SYM_TRUE,SYM_FALSE,SYM_AND,SYM_NOT,SYM_OR,SYM_ELSE,SYM_FOR,SYM_TO,SYM_DOWNTO,SYM_EXIT,SYM_READ,SYM_WRITE,SYM_REPEAT,SYM_UNTIL
};

int ssym[NSYM + 1] =
{
	SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES, SYM_SLASH,
	SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_PERIOD, SYM_SEMICOLON,SYM_COLON
};

char csym[NSYM + 1] =
{
	' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';',':'
};

#define MAXINS   8

char* mnemonic[MAXINS] =
{
	"LIT", "OPR", "LOD", "STO", "CAL", "INT", "JMP", "JPC"
};



typedef struct
{
	char name[MAXIDLEN + 1];
	int  kind;
	int  value;
} comtab;

comtab table[TXMAX];





typedef struct
{
	char  name[MAXIDLEN + 1];
	int   kind;
	short level;
	short address;
} mask;

FILE* infile;

// EOF PL0.h
