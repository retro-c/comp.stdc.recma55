
/**********************************************************************************
 *   ____      _                    ____
 *  |  _ \ ___| |_ _ __ ___        / ___|
 *  | |_) / _ \ __| '__/ _ \ _____| |
 *  |  _ <  __/ |_| | | (_) |_____| |___
 *  |_| \_\___|\__|_|  \___/       \____|
 *
 *
 *  RECMA55 - Retro ECMA-55-compliant Minimal BASIC Interpreter
 *
 *  Source File
 *
 *  Repository:    <http://source.retro-c.net/comp.stdc.recma55>
 *  File:          /src/recma55.c//
 *  Version:       01.01!00
 *  Environments:  C90, ASCII-CP
 *  Compliance:    Retro-Frame 1.1
 *  License:       MIT
 *
 *  Copyright (c) 2026 Ingo Boehmer <ingo@retro-leisure.net>
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 **********************************************************************************/


#define _CRT_SECURE_NO_WARNINGS  /* be careful (make sure that RECMA55_NUM will never have an exponent of more than RECMA55_NUM_EXP_LEN_MAX decimal digits in order to avoid sprintf overflow) */

#include <assert.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


/*****************************************************************************************
 *
 *  P A R A M E T R I Z A T I O N   M A C R O S
 *
 *****************************************************************************************/


#ifndef RECMA55_LINE_NUM_LEN_MAX
#define RECMA55_LINE_NUM_LEN_MAX          4   /* see ECMA-55 chapter 5.2 */
#endif

#ifndef RECMA55_LINE_LEN_MAX
#define RECMA55_LINE_LEN_MAX              72  /* see ECMA-55 chapter 5.4 */
#endif

#ifndef RECMA55_STR_VAR_LEN_MAX
#define RECMA55_STR_VAR_LEN_MAX           18  /* see ECMA-55 chapter 7.4 */
#endif

#ifndef RECMA55_PRINT_ZONE
#define RECMA55_PRINT_ZONE                15  /* see ECMA-55 chapters 14.4 and 14.6 */
#endif

#ifndef RECMA55_PRINT_MARGIN
#define RECMA55_PRINT_MARGIN              75  /* see ECMA-55 chapters 14.4 and 14.6 */
#endif

#ifndef RECMA55_NUM_PRECISION
#define RECMA55_NUM_PRECISION             6   /* see ECMA-55 chapters 6.4 and 14.4 */
#endif

#ifndef RECMA55_EXRAD_WIDTH
#define RECMA55_EXRAD_WIDTH               2   /* see ECMA-55 chapter 14.4 */
#endif

#if ((RECMA55_LINE_NUM_LEN_MAX < 1) || (RECMA55_LINE_NUM_LEN_MAX > 15))
#error RECMA55_LINE_NUM_LEN_MAX must be in the range 1..15
#endif

#if RECMA55_LINE_LEN_MAX < RECMA55_LINE_NUM_LEN_MAX
#error RECMA55_LINE_LEN_MAX must be >= RECMA55_LINE_NUM_LEN_MAX
#endif

#if RECMA55_STR_VAR_LEN_MAX < 2
#error RECMA55_STR_VAR_LEN_MAX must be >= 2
#endif

#if RECMA55_PRINT_ZONE < 2
#error RECMA55_PRINT_ZONE must be >= 2
#endif

#if (RECMA55_PRINT_MARGIN < RECMA55_PRINT_ZONE) || (RECMA55_PRINT_MARGIN < RECMA55_STR_VAR_LEN_MAX) || (RECMA55_PRINT_MARGIN < RECMA55_NUM_PRECISION + RECMA55_EXRAD_WIDTH + 4)
#error RECMA55_PRINT_MARGIN must be >= max(RECMA55_PRINT_ZONE, RECMA55_STR_VAR_LEN_MAX, RECMA55_NUM_PRECISION + RECMA55_EXRAD_WIDTH + 4)
#endif

#if RECMA55_NUM_PRECISION < 6
#error RECMA55_NUM_PRECISION must be >= 6
#endif

#if RECMA55_EXRAD_WIDTH < 2
#error RECMA55_EXRAD_WIDTH must be >= 2
#endif


/*****************************************************************************************
 *
 *  M A C R O   D E F I N I T I O N S
 *
 *****************************************************************************************/


#define RECMA55_COPYRIGHT                 "Copyright (c) 2026 Ingo Boehmer <ingo@retro-leisure.net>"
#define RECMA55_VERSION                   "1.1!0 (dev)"

#define RECMA55_INPUT_PROMPT              "? "

#define RECMA55_INPUT_BUF_LEN             15

#define RECMA55_NUM_EXP_LEN_MAX           5  /* should be sufficient for all common floating point types (exponent must fit into an int value) */

#define RECMA55_UINT_MAX                  (~(RECMA55_UINT)0)

#if !defined (RECMA55_DOUBLE) && !defined (RECMA55_HUGE_VAL)
#ifndef INFINITY
#error INFINITY not defined (consider using -DRECMA55_DOUBLE or see C90 chapter 7.5 and try -DRECMA55_HUGE_VAL)
#endif
#define RECMA55_NUM_INFINITY              ((RECMA55_NUM)INFINITY)
#else
#define RECMA55_NUM_INFINITY              ((RECMA55_NUM)HUGE_VAL)
#endif


/*****************************************************************************************
 *
 *  T Y P E   D E F I N I T I O N S
 *
 *****************************************************************************************/


#if !defined (RECMA55_DOUBLE)
typedef float RECMA55_NUM;          /* ECMA-55 compliant numeric type (see ECMA-55 chapter 6.4) */
#else
typedef double RECMA55_NUM;         /* define RECMA55_DOUBLE in order to use double instead of float */
#endif

typedef unsigned int RECMA55_UINT;  /* general, internally used unsigned integer (e.g. to represent line numbers) */

typedef int RECMA55_BOOL;           /* C90 does not provide type _Bool */


typedef enum RECMA55_STATE {
	RECMA55_STATE_CONTINUE,
	RECMA55_STATE_HALT,
	RECMA55_STATE_FATAL_ERROR
} RECMA55_STATE;


typedef enum RECMA55_EXPR_MODE {
	RECMA55_EXPR_MODE_ANY,  /* no expression constraint */
	RECMA55_EXPR_MODE_STR,  /* string expression required */
	RECMA55_EXPR_MODE_NUM,  /* numeric expression required */
	RECMA55_EXPR_MODE_VAR   /* variable ("l-value") required */
} RECMA55_EXPR_MODE;


typedef enum RECMA55_EXPR_TYPE {
	RECMA55_EXPR_TYPE_OP_ADD,     /* operator + (see ECMA-55 chapter 8) */
	RECMA55_EXPR_TYPE_OP_SUB,     /* operator - (see ECMA-55 chapter 8), must be RECMA55_EXPR_TYPE_OP_ADD + 1 */
	RECMA55_EXPR_TYPE_OP_MUL,     /* operator * (see ECMA-55 chapter 8), must be RECMA55_EXPR_TYPE_OP_ADD + 2 */
	RECMA55_EXPR_TYPE_OP_DIV,     /* operator / (see ECMA-55 chapter 8), must be RECMA55_EXPR_TYPE_OP_ADD + 3 */
	RECMA55_EXPR_TYPE_OP_INV,     /* operator ^ (see ECMA-55 chapter 8), must be RECMA55_EXPR_TYPE_OP_ADD + 4 */
	RECMA55_EXPR_TYPE_FUNC,       /* implementation supplied function (see ECMA-55 chapter 9) */
	RECMA55_EXPR_TYPE_USER_FUNC,  /* user defined function (see ECMA-55 chapter 10) */
	RECMA55_EXPR_TYPE_STR_CONST,  /* string constant (see ECMA-55 chapter 6, allocated by expression) */
	RECMA55_EXPR_TYPE_STR_VAR,    /* string variable reference (see ECMA-55 chapter 7) */
	RECMA55_EXPR_TYPE_NUM_CONST,  /* numeric constant (see ECMA-55 chapter 6) */
	RECMA55_EXPR_TYPE_NUM_VAR,    /* simple numeric variable reference (see ECMA-55 chapter 7) */
	RECMA55_EXPR_TYPE_ARRAY_BASE  /* numeric array element reference (see ECMA-55 chapter 7, index of variant will be added) */
} RECMA55_EXPR_TYPE;


typedef enum RECMA55_EXPR_PRIO {
	RECMA55_EXPR_PRIO_PRIMARY,
	RECMA55_EXPR_PRIO_INV,
	RECMA55_EXPR_PRIO_MUL_DIV,
	RECMA55_EXPR_PRIO_ADD_SUB
} RECMA55_EXPR_PRIO;


typedef enum RECMA55_RELATION {
	RECMA55_RELATION_EQUAL,         /* = */
	RECMA55_RELATION_NOT_EQUAL,     /* <> */
	RECMA55_RELATION_LESS_THAN,     /* < */
	RECMA55_RELATION_NOT_GREATER,   /* <= */
	RECMA55_RELATION_GREATER_THAN,  /* > */
	RECMA55_RELATION_NOT_LESS       /* >= */
} RECMA55_RELATION;


struct RECMA55_VARIANT {
	RECMA55_UINT tDim[2];   /* tDim[0] == 0 on simple numeric variables, tDim[1] == 0 on one-dimensional arrays or unreferenced variables */
	union {
		RECMA55_NUM num;    /* valid if tDim[0] == 0 */
		RECMA55_NUM *tNum;  /* valid if tDim[0] > 0 (allocated during prepare) */
	} value;
};


struct RECMA55_STR {
	char *sStr;
	RECMA55_UINT len;
};


struct RECMA55_DATA {
	struct RECMA55_DATA *pNext;
	struct RECMA55_STR str;
	RECMA55_BOOL fQuoted;
};


struct RECMA55_LINE;


union RECMA55_LINE_REF {
	char sLineNum[RECMA55_LINE_NUM_LEN_MAX];  /* valid before prepare */
	struct RECMA55_LINE *pLine;               /* valid after prepare */
};


struct RECMA55_RETURN_STACK {
	struct RECMA55_RETURN_STACK *pPrev;
	struct RECMA55_LINE *pLine;
};


struct RECMA55_EXPR;


struct RECMA55_PARAM_FOR {
	RECMA55_NUM *pNum;               /* pointer to control variable */
	struct RECMA55_EXPR *tpExpr[3];  /* [0] = initial value, [1] = limit, [2] = increment (may be NULL) */
	struct RECMA55_LINE *pLine;      /* line of NEXT statement */
	union {
		struct RECMA55_LINE *pLinePrev;  /* temporary pointer used during build and assertion */
		RECMA55_NUM tNum[2];             /* [0] = limit (RECMA55_NUM_INFINITY indicates non-active FOR-block), [1] = increment */
	} state;
};


struct RECMA55_PARAM_IF {
	struct RECMA55_EXPR *tpExpr[2];
	RECMA55_RELATION relation;
	union RECMA55_LINE_REF ref;
};


struct RECMA55_PARAM_LET {
	struct RECMA55_EXPR *tpExpr[2];
};


struct RECMA55_PARAM_ON {
	struct RECMA55_EXPR *pExpr;
	RECMA55_UINT count;
	union RECMA55_LINE_REF *tRef;
};


struct RECMA55_PARAM_PRINT_ITEM {
	struct RECMA55_PARAM_PRINT_ITEM *pNext;
	RECMA55_UINT commaCount;
	RECMA55_BOOL fTab;
	struct RECMA55_EXPR *pExpr;
};


struct RECMA55_PARAM_PRINT {
	struct RECMA55_PARAM_PRINT_ITEM *pItemList;
	RECMA55_UINT commaCount;
	RECMA55_BOOL fNewLine;
};


struct RECMA55_PARAM_VAR {
	struct RECMA55_PARAM_VAR *pNext;  /* pointer to next variable in list */
	struct RECMA55_EXPR *pExpr;       /* string variable, numeric variable or array element (subscript to be evaluated) */
	RECMA55_NUM num;                  /* temporary numeric value */
};


union RECMA55_PARAM {
	union RECMA55_LINE_REF ref;
	struct RECMA55_PARAM_FOR *pFor;
	struct RECMA55_PARAM_IF *pIf;
	struct RECMA55_PARAM_LET *pLet;
	struct RECMA55_PARAM_ON *pOn;
	struct RECMA55_PARAM_PRINT *pPrint;
	struct RECMA55_PARAM_VAR *pVarList;
};


struct RECMA55_STATEMENT;


struct RECMA55_LINE {
	struct RECMA55_LINE *pNext;
	const struct RECMA55_STATEMENT *pStatement;  /* may be NULL during load */
	union RECMA55_PARAM param;
	char sLineNum[RECMA55_LINE_NUM_LEN_MAX];
};


struct RECMA55_LINE_NODE {
	struct RECMA55_LINE *pLine;
	struct RECMA55_LINE_NODE *pLeft;
	struct RECMA55_LINE_NODE *pRight;
	RECMA55_UINT depth;                /* if zero, pLeft is full but pRight is not */
};


struct RECMA55_USER_FUNC {
	struct RECMA55_EXPR *pExpr;
	RECMA55_NUM arg;
	char sArgVar[2];
};


struct RECMA55_RUN_CTX {
	RECMA55_BOOL fBatchMode;
	RECMA55_BOOL fFullChar;
	struct RECMA55_LINE *pLineList;
	struct RECMA55_LINE *pLineCur;
	struct RECMA55_LINE *pLineNext;
	struct RECMA55_DATA *pDataList;
	struct RECMA55_DATA *pDataCur;
	RECMA55_UINT inputRow;
	RECMA55_UINT outputCol;
	RECMA55_UINT fOutput;       /* set on first output */
	RECMA55_BOOL fNoIncrement;  /* used temporarily on FOR/NEXT execution */
	RECMA55_BOOL fAbort;
	struct RECMA55_USER_FUNC tUserFunc[26];
	struct {
		RECMA55_UINT base;
		struct RECMA55_VARIANT tVariant[26];          /* variables A..Z */
		RECMA55_NUM ttNum[26][10];                    /* variables A0..Z9 */
		char tsStr[26][RECMA55_STR_VAR_LEN_MAX + 1];  /* variables A$..Z$ */
	} var;
	struct RECMA55_RETURN_STACK *pStack;
};


struct RECMA55_PREPARE_CTX {
	struct RECMA55_RUN_CTX *pRunCtx;
	struct RECMA55_LINE_NODE *pLineTree;
	struct RECMA55_LINE *pLineStack;      /* stack of FOR statements with open NEXT statement */
};


struct RECMA55_LOAD_CTX {
	struct RECMA55_PREPARE_CTX *pPrepareCtx;
	char sLine[RECMA55_LINE_LEN_MAX + 1];
	RECMA55_UINT fileLineNum;              /* this is for information purposes only (error messages) so we do not prevent overflow */
	RECMA55_BOOL fPRNG;                    /* set if RND function and/or RANDOMIZE function is used by the program */
	RECMA55_BOOL fNoOptionBase;            /* set after first reference to an element of an array or OPTION statement */
	RECMA55_BOOL fEnd;
	const char *sLineNum;                  /* last line number (initially set to NULL) */
	struct RECMA55_LINE **ppLine;
	struct RECMA55_DATA **ppDataTail;
};


struct RECMA55_FUNC {
	const char *sName;
	RECMA55_BOOL fArg;
	RECMA55_BOOL fPRNG;
	RECMA55_NUM (*Handler)(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg);  /* if !fArg, 0.0 is passed as arg */
};


struct RECMA55_EXPR {
	RECMA55_EXPR_TYPE type;
	RECMA55_EXPR_PRIO prio;
	union {
		struct RECMA55_STR str;  /* valid if type == RECMA55_EXPR_TYPE_STR_ (str.len == 0 if type == RECMA55_EXPR_TYPE_STR_VAR) */
		RECMA55_NUM num;         /* valid if type == RECMA55_EXPR_TYPE_NUM_CONST */
		RECMA55_NUM *pNum;       /* valid if type == RECMA55_EXPR_TYPE_NUM_VAR */
		struct {
			union {
				const struct RECMA55_FUNC *pFunc;     /* valid if type == RECMA55_EXPR_TYPE_FUNC */
				struct RECMA55_USER_FUNC *pUserFunc;  /* valid if type == RECMA55_EXPR_TYPE_USER_FUNC */
				struct RECMA55_EXPR *pExpr;           /* valid otherwise (second operand or second subscript of numeric array element) */
			} u;
			struct RECMA55_EXPR *pExpr;  /* first operand, function argument or first subscript of a numeric array element */
		} eval;
	} value;
};


struct RECMA55_STATEMENT {
	const char *sName;
	RECMA55_BOOL fPRNG;
	struct {
		RECMA55_STATE (*Exec)(struct RECMA55_RUN_CTX *pCtx);                                                 /* optional (may be NULL) */
		RECMA55_BOOL (*Prepare)(struct RECMA55_PREPARE_CTX *pCtx, struct RECMA55_LINE *pLine);               /* optional (may be NULL) */
		RECMA55_BOOL (*Build)(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos);  /* required */
		void (*Clear)(union RECMA55_PARAM *pParam);                                                          /* optional (may be NULL) */
	} handler;
};


struct RECMA55_INPUT_BUF {
	struct RECMA55_INPUT_BUF *pNext;
	char sStr[RECMA55_INPUT_BUF_LEN + 1];
	RECMA55_UINT len;
};


/*****************************************************************************************
 *
 *  E R R O R   H A N D L E R
 *
 *****************************************************************************************/


static void L_LineErrMsg(const char sLineNum[RECMA55_LINE_NUM_LEN_MAX], const char *sErrType, const char *sMsg)
{
	char sBuf[RECMA55_LINE_NUM_LEN_MAX + 1];
	RECMA55_UINT len;
	
	assert(sMsg != NULL);
	
	len = 0;
	
	while ((len < RECMA55_LINE_NUM_LEN_MAX) && (sLineNum[len] != '\0'))
	{
		sBuf[len] = sLineNum[len];
		++len;
	}
	
	sBuf[len] = '\0';
	
	if (sMsg[0] != '\0')
	{
		fprintf(stderr, "\n%s ERROR @ LINE NUMBER %s: %s\n", sErrType, sBuf, sMsg);
	}
}


static void L_GenericErrorMsg(const char *sMsg)
{
	if (sMsg == NULL)
	{
		fprintf(stderr, "\nINTERNAL ERROR\n");
	}
	else if (sMsg[0] != '\0')
	{
		fprintf(stderr, "\nERROR: %s\n", sMsg);
	}
}


static void L_InputErrorMsg(RECMA55_UINT inputRow, const char *sMsg)
{
	if (inputRow)
	{
		fprintf(stderr, "\nINPUT ERROR @ INPUT LINE %u: %s\n", inputRow, sMsg);
	}
	else
	{
		fprintf(stderr, "INPUT ERROR: %s\n\n", sMsg);
	}
}


static void L_RunTimeErrorMsg(const char sLineNum[RECMA55_LINE_NUM_LEN_MAX], const char *sMsg)
{
	L_LineErrMsg(sLineNum, "RUN-TIME", sMsg);
}


static void L_ProgramErrorMsg(const char sLineNum[RECMA55_LINE_NUM_LEN_MAX], const char *sMsg)
{
	L_LineErrMsg(sLineNum, "PROGRAM", sMsg);
}


static void L_SyntaxErrorMsg(const char sLineNum[RECMA55_LINE_NUM_LEN_MAX], const char *sMsg)
{
	if (sLineNum != NULL)
	{
		L_LineErrMsg(sLineNum, "SYNTAX", sMsg);
	}
	else if (sMsg[0] != '\0')
	{
		fprintf(stderr, "\nINPUT ERROR: %s\n", sMsg);
	}
}


static void L_LoadErrorMsg(RECMA55_UINT fileLineNum, const char *sMsg)
{
	fprintf(stderr, "\nLOAD ERROR @ INPUT LINE %u: %s\n", fileLineNum, sMsg);
}


static void L_ErrorMsgExprMode(const char sLineNum[RECMA55_LINE_NUM_LEN_MAX], RECMA55_EXPR_MODE mode)
{
	switch (mode)
	{
	case RECMA55_EXPR_MODE_STR:
		
		L_SyntaxErrorMsg(sLineNum, "STRING EXPRESSION EXPECTED");
		break;
		
	case RECMA55_EXPR_MODE_NUM:
		
		L_SyntaxErrorMsg(sLineNum, "NUMERIC EXPRESSION EXPECTED");
		break;
		
	case RECMA55_EXPR_MODE_VAR:
		
		L_SyntaxErrorMsg(sLineNum, "VARIABLE EXPECTED");
		break;
		
	default:
		
		L_SyntaxErrorMsg(sLineNum, "UNKNOWN");
		break;
	}
}


/*****************************************************************************************
 *
 *  E X P R E S S I O N   H A N D L E R
 *
 *****************************************************************************************/


static RECMA55_NUM L_ExprOpAdd(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg1, RECMA55_NUM arg2)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	return arg1 + arg2;
}


static RECMA55_NUM L_ExprOpSub(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg1, RECMA55_NUM arg2)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	return arg1 - arg2;
}


static RECMA55_NUM L_ExprOpMul(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg1, RECMA55_NUM arg2)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	return arg1 * arg2;
}


static RECMA55_NUM L_ExprOpDiv(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg1, RECMA55_NUM arg2)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	if (arg2 == 0.0)
	{
		return (arg1 >= 0.0) ? RECMA55_NUM_INFINITY : -RECMA55_NUM_INFINITY;
	}
	
	return arg1 / arg2;
}


static RECMA55_NUM L_ExprOpInv(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg1, RECMA55_NUM arg2)
{
	RECMA55_UINT value;
	RECMA55_UINT mask;
	
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	if (arg1 == 0.0)
	{
		if (arg2 == 0.0)
		{
			return 1.0;  /* see ECMA-55 chapter 8.4 */
		}
		
		if (arg2 < 0.0)
		{
			return RECMA55_NUM_INFINITY;  /* see ECMA-55 chapter 8.5 */
		}
		
		return 0.0;
	}
	
	value = (RECMA55_UINT)arg2;
	
	if ((RECMA55_NUM)value != arg2)
	{
		if (arg1 < 0.0)
		{
			L_RunTimeErrorMsg(pCtx->pLineCur->sLineNum, "INVALID EXPONENTIATION");  /* see ECMA-55 chapter 8.5 */
			pCtx->fAbort = 1;
			return RECMA55_NUM_INFINITY;
		}
		
		return (RECMA55_NUM)exp(log(arg1) * arg2);
	}
	
	/* integer exponent (see ECMA-55 chapter 8.6): */
	
	mask = 1;
	
	while (mask < value)
	{
		mask = (mask << 1) | 1;
	}
	
	arg2 = 1.0;
	
	while (mask)
	{
		arg2 = arg2 * arg2;
		
		mask >>= 1;
		
		if (value & ~mask)
		{
			arg2 *= arg1;
		}
		
		value &= mask;
	}
	
	return arg2;
}


static RECMA55_NUM* L_GetArrayNum(struct RECMA55_RUN_CTX *pCtx, RECMA55_UINT index, RECMA55_NUM subscript1, RECMA55_NUM subscript2)
{
	struct RECMA55_VARIANT *pVariant;
	RECMA55_UINT index2;
	
	assert(index < 26);
	
	pVariant = &pCtx->var.tVariant[index];
	
	subscript1 += 0.5;  /* round, see ECMA-55 chapter 7.4 */
	
	index = (RECMA55_UINT)subscript1;
	
	if ((subscript1 >= -0.5) && (subscript1 <= RECMA55_UINT_MAX) && (index >= pCtx->var.base) && (index <= pVariant->tDim[0]))
	{
		if (!pVariant->tDim[1])
		{
			return &pVariant->value.tNum[index - pCtx->var.base];
		}
		
		subscript2 += 0.5;  /* round, see ECMA-55 chapter 7.4 */
		
		index2 = (RECMA55_UINT)subscript2;
		
		if ((subscript2 >= -0.5) && (subscript2 <= RECMA55_UINT_MAX) && (index2 >= pCtx->var.base) && (index2 <= pVariant->tDim[1]))
		{
			index2 += index * (pVariant->tDim[1] + (1 - pCtx->var.base));
			
			return &pVariant->value.tNum[index2 - pCtx->var.base];
		}
	}
	
	L_RunTimeErrorMsg(pCtx->pLineCur->sLineNum, "SUBSCRIPT OUT OF RANGE");
	
	return NULL;
}


/*****************************************************************************************
 *
 *  U T I L I T Y   F U N C T I O N S
 *
 *****************************************************************************************/


static const char* L_GetStr(const char *sStr)
{
	return (sStr != NULL) ? sStr : "";
}


static RECMA55_BOOL L_StrCmp(const char *sStr1, const char *sStr2)
{
	while ((*sStr1 != '\0') || (*sStr2 != '\0'))
	{
		if (toupper(*sStr1++) != toupper(*sStr2++))
		{
			return 0;
		}
	}
	
	return 1;
}


static int L_LineNumCmp(const char *sLineNum1, const char *sLineNum2)
{
	RECMA55_UINT len;
	RECMA55_UINT pos;
	
	if (sLineNum1 == NULL)
	{
		return -1;
	}
	
	len = RECMA55_LINE_NUM_LEN_MAX;
	
	while ((sLineNum1[len - 1] == '\0') || (sLineNum2[len - 1] == '\0'))
	{
		if (sLineNum1[len - 1] != sLineNum2[len - 1])
		{
			return (sLineNum1[len - 1] == '\0') ? -1 : 1;
		}
		
		if (!--len)
		{
			return 0;
		}
	}
	
	for (pos = 0; pos < len; ++pos)
	{
		if (sLineNum1[pos] != sLineNum2[pos])
		{
			return (sLineNum1[pos] < sLineNum2[pos]) ? -1 : 1;
		}
	}
	
	return 0;
}


static RECMA55_BOOL L_IsPlainStrChar(char c)
{
	if (c <= 0)
	{
		return 0;
	}
	
	return ((c == '+') || (c == '-') || (c == '.') || isalnum(c));
}


static RECMA55_UINT L_GetUserFuncArgLen(const char *sLine, struct RECMA55_USER_FUNC *pUserFunc)
{
	if (pUserFunc == NULL)
	{
		return 0;
	}
	
	if (sLine[0] != pUserFunc->sArgVar[0])
	{
		return 0;
	}
	
	if (pUserFunc->sArgVar[1] == '\0')
	{
		return 1;
	}
	
	if (sLine[1] != pUserFunc->sArgVar[1])
	{
		return 0;
	}
	
	return 2;
}


static const char* L_GetQuotedStrLen(const char *sStr, RECMA55_UINT *pPos, RECMA55_UINT *pLen)
{
	RECMA55_UINT pos;
	RECMA55_UINT len;
	
	pos = *pPos;
	
	assert(sStr[pos] == '\"');
	
	len = 0;
	
	while (sStr[++pos] != '\"')
	{
		if (len > RECMA55_PRINT_MARGIN)
		{
			return "QUOTED STRING CONSTANT EXCEEDS PRINT MARGIN";
		}
		
		if (sStr[pos] == '\0')
		{
			return "QUOTED STRING CONSTANT NOT TERMINATED";
		}
		
		++len;
	}
	
	/* skip quotation mark and trailing spaces: */

	do
	{
		++pos;
	}
	while (sStr[pos] == ' ');
	
	*pPos = pos;
	*pLen = len;
	
	return NULL;
}


static RECMA55_UINT L_PreparePrint(RECMA55_UINT outputCol, RECMA55_UINT len)
{
	/* see ECMA-55 chapter 14.4: */
	
	if (len <= RECMA55_PRINT_MARGIN - outputCol)
	{
		return outputCol + len;
	}
	
	printf("\n");
	
	return len % RECMA55_PRINT_MARGIN;
}


static RECMA55_UINT L_PrintComma(RECMA55_UINT outputCol, RECMA55_UINT commaCount)
{
	while (commaCount)
	{
		/* see ECMA-55 chapter 14.4: */
		
		if ((outputCol + 1) / RECMA55_PRINT_ZONE  >= RECMA55_PRINT_MARGIN / RECMA55_PRINT_ZONE)
		{
			printf("\n");
			outputCol = 0;
		}
		else
		{
			do
			{
				printf(" ");
				++outputCol;
			}
			while (outputCol % RECMA55_PRINT_ZONE);
		}
		
		--commaCount;
	}
	
	return outputCol;
}


static RECMA55_UINT L_PrintNum(RECMA55_UINT outputCol, RECMA55_NUM num)
{
	char sBuf[RECMA55_NUM_PRECISION + RECMA55_NUM_EXP_LEN_MAX + 5];  /* sign + digit + full-stop + (RECMA55_NUM_PRECISION - 1) digits + 'E' + sign + up to RECMA55_NUM_EXP_LEN_MAX digits + '\0' */
	int exp;
	int pos;
	int len;
	
	pos = 0;
	
	if ((num == RECMA55_NUM_INFINITY) || (num == -RECMA55_NUM_INFINITY))
	{
		sprintf(sBuf, "%cINFINITY", (num < 0) ? '-' : ' ');
	}
	else
	{
		sprintf(sBuf, "% .*E", RECMA55_NUM_PRECISION - 1, num);
		
		if (sBuf[1] == '0')
		{
			sprintf(sBuf, " 0 ");  /* avoid negative zeros */
		}
		else
		{
			exp = 0;
			
			pos = RECMA55_NUM_PRECISION + 4;  /* should refer to the most significant digit of the exponent */
			
			while (sBuf[pos] == '0')
			{
				++pos;  /* skip leading zeros of exponent */
			}
			
			len = pos;
			
			while ((sBuf[len] != '\0') && (len - pos <= RECMA55_EXRAD_WIDTH))
			{
				assert(isdigit((int)sBuf[len]));
				
				exp = (exp * 10) + (sBuf[len++] - '0');
			}
			
			if (len - pos > RECMA55_EXRAD_WIDTH)
			{
				sprintf(sBuf, "%cHUGE-VAL", (num < 0) ? '-' : ' ');
			}
			else
			{
				assert((sBuf[RECMA55_NUM_PRECISION + 3] == '+') || (sBuf[RECMA55_NUM_PRECISION + 3] == '-'));
				
				if (sBuf[RECMA55_NUM_PRECISION + 3] == '-')
				{
					exp = -exp;
				}
				
				assert((sBuf[RECMA55_NUM_PRECISION + 2] == 'E') || (sBuf[RECMA55_NUM_PRECISION + 2] == 'e'));
				
				len = RECMA55_NUM_PRECISION + 1;  /* should refer to the least significant digit of the significand */
				
				while (sBuf[len] == '0')
				{
					assert(len > 2);  /* full-stop expected if (len == 2) */
					
					--len;
				}
				
				if ((exp + 2 >= len) && (exp < RECMA55_NUM_PRECISION))
				{
					/* integer output using implicit point unscaled representation: */
					
					for (pos = 0; pos < exp; ++pos)
					{
						sBuf[pos + 2] = sBuf[pos + 3];
					}
					
					sBuf[exp + 2] = '\0';
				}
				else if ((len - 2 - exp <= RECMA55_NUM_PRECISION) && (exp < RECMA55_NUM_PRECISION))
				{
					/* fractional output using explicit point unscaled representation: */
					
					if (exp >= 0)
					{
						for (pos = 0; pos < exp; ++pos)
						{
							sBuf[pos + 2] = sBuf[pos + 3];
						}
						
						sBuf[exp + 2] = '.';
						sBuf[len + 1] = '\0';
					}
					else if (exp == -1)
					{
						sBuf[2] = sBuf[1];
						sBuf[1] = '.';
						sBuf[len + 1] = '\0';
					}
					else
					{
						sBuf[2] = sBuf[1];
						sBuf[1] = '.';
						
						pos = len - exp;
						
						sBuf[pos--] = '\0';
						
						while (len > 1)
						{
							sBuf[pos--] = sBuf[len--];
						}
						
						while (pos > 1)
						{
							sBuf[pos--] = '0';
						}
					}
				}
			}
		}
	}
	
	outputCol = L_PreparePrint(outputCol, (RECMA55_UINT)strlen(sBuf) + 1);
	
	printf("%s ", sBuf);
	
	return outputCol;
}


static RECMA55_BOOL L_CheckEndOfLine(const char *sLine, const char sLineNum[RECMA55_LINE_NUM_LEN_MAX])
{
	if (*sLine != '\0')
	{
		L_SyntaxErrorMsg(sLineNum, "END OF LINE EXPECTED");
		return 0;
	}
	
	return 1;
}


static RECMA55_BOOL L_CheckKeyword(const char *sLine, RECMA55_UINT *pPos, const char *sKeywordExpected, const char sLineNum[RECMA55_LINE_NUM_LEN_MAX])
{
	RECMA55_UINT pos;
	RECMA55_UINT len;
	RECMA55_UINT count;
	RECMA55_BOOL fSpaceErr;
	
	/* sKeywordExpected must have the format "\'<KEYWORD>\' EXPECTED" */
	
	pos = *pPos;
	len = 0;
	count = ((sKeywordExpected[1] == 'G') && (sKeywordExpected[2] == 'O')) ? 2 : 0;
	fSpaceErr = (pos > 0) && (sLine[pos - 1] != ' ');
	
	while (sKeywordExpected[len + 1] != '\'')
	{
		if (sLine[pos + len] != sKeywordExpected[len + 1])
		{
			L_SyntaxErrorMsg(sLineNum, sKeywordExpected);
			return 0;
		}
		
		++len;
		
		if (count)
		{
			if (!--count)
			{
				while (sLine[pos + len] == ' ')
				{
					++pos;
				}
			}
		}
	}
	
	if (fSpaceErr)
	{
		L_SyntaxErrorMsg(sLineNum, "KEYWORD SHALL BE PRECEDED BY AT LEAST ONE SPACE");  /* See ECMA-55 chapter 5.4: */
		return 0;
	}
	
	pos += len;
	
	if ((sLine[pos] != '\0') && (sLine[pos] != ' '))
	{
		L_SyntaxErrorMsg(sLineNum, "KEYWORD SHALL BE FOLLOWED BY AT LEAST ONE SPACE");  /* See ECMA-55 chapter 5.4: */
		return 0;
	}
	
	while (sLine[pos] == ' ')
	{
		++pos;
	}
	
	*pPos = pos;
	
	return 1;
}


static RECMA55_BOOL L_TranslateInputChar(int *pC, RECMA55_BOOL fString, RECMA55_BOOL fFullChar)
{
	char c;
	
	c = *pC;
	
	if (((c >= ' ') && (c <= '?')) || ((c >= 'A') && (c <= 'Z')) || (c == '^') || (c == '_'))  /* ASCII assumed */
	{
		return 1;  /* conforming character set, see ECMA-55 chapter 4 */
	}
	
	if (!fFullChar)
	{
		return 0;
	}
	
	if ((((c >= 0) && c < 32) && (c != '\a') && (c != '\b') && (c != '\t')) || (c == 127))
	{
		*pC = '\0';  /* ignore all ASCII control characters but BEL, BS and HT */
	}
	
	if (!fString)
	{
		*pC = (c == '\t') ? ' ' : toupper(c);  /* translate code characters to upper case and HT to space if -FULLCHAR argument is given */
	}
	
	return 1;
}


static void* L_MemAlloc(size_t size)
{
	void *pPtr;
	
	if (size)
	{
		pPtr = malloc(size);
	}
	else
	{
		pPtr = NULL;
	}
	
	if (pPtr == NULL)
	{
		L_GenericErrorMsg("OUT OF MEMORY");
	}
	
	return pPtr;
}


static void L_MemFree(void *pPtr)
{
	free(pPtr);
}


static void L_DestroyInputBuf(struct RECMA55_INPUT_BUF **ppInputBuf)
{
	struct RECMA55_INPUT_BUF *pInputBuf;
	
	while (*ppInputBuf != NULL)
	{
		pInputBuf = *ppInputBuf;
		*ppInputBuf = pInputBuf->pNext;
		
		L_MemFree(pInputBuf);
	}
}


static void L_DestroyData(struct RECMA55_DATA **ppData)
{
	struct RECMA55_DATA *pData;
	
	while (*ppData != NULL)
	{
		pData = *ppData;
		*ppData = pData->pNext;
		
		L_MemFree(pData);
	}
}


static void L_DestroyExpr(struct RECMA55_EXPR *pExpr)
{
	struct RECMA55_EXPR *pNext;
	
	while (pExpr != NULL)
	{
		switch (pExpr->type)
		{
		case RECMA55_EXPR_TYPE_STR_CONST:
			
			L_MemFree(pExpr->value.str.sStr);
			pNext = NULL;
			break;
			
		case RECMA55_EXPR_TYPE_STR_VAR:
		case RECMA55_EXPR_TYPE_NUM_CONST:
		case RECMA55_EXPR_TYPE_NUM_VAR:
			
			pNext = NULL;
			break;
			
		case RECMA55_EXPR_TYPE_FUNC:
		case RECMA55_EXPR_TYPE_USER_FUNC:
			
			pNext = pExpr->value.eval.pExpr;
			break;
			
		default:
			
			if (pExpr->value.eval.u.pExpr != NULL)
			{
				L_DestroyExpr(pExpr->value.eval.u.pExpr);
			}
			
			pNext = pExpr->value.eval.pExpr;
		}
		
		L_MemFree(pExpr);
		pExpr = pNext;
	}
}


static RECMA55_BOOL L_CreateArray(struct RECMA55_VARIANT *pVariant, RECMA55_UINT base)
{
	RECMA55_UINT count;
	
	assert(base <= 1);
	assert(pVariant->tDim[0]);
	assert(pVariant->value.tNum == NULL);
	
	if (pVariant->tDim[0] > RECMA55_UINT_MAX - (1 - base))
	{
		L_MemAlloc(0);  /* report index overflow as out of memory error */
		return 0;
	}
	
	count = pVariant->tDim[0] + (1 - base);
	
	if (pVariant->tDim[1])
	{
		if (pVariant->tDim[1] > (RECMA55_UINT_MAX / count) - (1 - base))
		{
			L_MemAlloc(0);  /* report index overflow as out of memory error */
			return 0;
		}
		
		count *= pVariant->tDim[1] + (1 - base);
	}
	
	if (count > ~(size_t)0 / sizeof (RECMA55_NUM))
	{
		L_MemAlloc(0);
		return 0;
	}
	
	pVariant->value.tNum = L_MemAlloc((size_t)count * sizeof (RECMA55_NUM));
	
	if (pVariant->value.tNum == NULL)
	{
		return 0;
	}
	
	do
	{
		pVariant->value.tNum[--count] = 0.0;
	}
	while (count);
	
	return 1;
}


static RECMA55_BOOL L_CreateExpr(struct RECMA55_EXPR **ppExpr, RECMA55_EXPR_TYPE type)
{
	struct RECMA55_EXPR *pExpr;
	
	pExpr = L_MemAlloc(sizeof (struct RECMA55_EXPR));
	
	if (pExpr == NULL)
	{
		return 0;
	}
	
	pExpr->type = type;
	pExpr->prio = RECMA55_EXPR_PRIO_PRIMARY;
	
	switch (type)
	{
	case RECMA55_EXPR_TYPE_STR_CONST:
	case RECMA55_EXPR_TYPE_STR_VAR:
		
		pExpr->value.str.sStr = NULL;
		pExpr->value.str.len = 0;
		break;
		
	case RECMA55_EXPR_TYPE_NUM_CONST:
		
		pExpr->value.num = 0.0;
		break;
		
	case RECMA55_EXPR_TYPE_NUM_VAR:
		
		pExpr->value.pNum = NULL;
		break;
		
	case RECMA55_EXPR_TYPE_FUNC:
		
		pExpr->value.eval.pExpr = NULL;
		pExpr->value.eval.u.pFunc = NULL;
		break;
		
	case RECMA55_EXPR_TYPE_USER_FUNC:
		
		pExpr->value.eval.pExpr = NULL;
		pExpr->value.eval.u.pUserFunc = NULL;
		break;
		
	default:
		
		pExpr->value.eval.pExpr = NULL;
		pExpr->value.eval.u.pExpr = NULL;
	}
	
	*ppExpr = pExpr;
	
	return 1;
}


static RECMA55_NUM L_EvalArray(struct RECMA55_RUN_CTX *pCtx, RECMA55_UINT index, RECMA55_NUM subscript1, RECMA55_NUM subscript2)
{
	RECMA55_NUM *pNum;
	
	pNum = L_GetArrayNum(pCtx, index, subscript1, subscript2);
	
	if (pNum == NULL)
	{
		pCtx->fAbort = 1;
		return RECMA55_NUM_INFINITY;
	}
	
	return *pNum;
}


static RECMA55_NUM L_EvalNumExpr(struct RECMA55_RUN_CTX *pCtx, struct RECMA55_EXPR *pExpr)
{
	if (pExpr == NULL)
	{
		return 0.0;
	}
	
	switch (pExpr->type)
	{
	case RECMA55_EXPR_TYPE_STR_CONST:
	case RECMA55_EXPR_TYPE_STR_VAR:
		
		L_RunTimeErrorMsg(pCtx->pLineCur->sLineNum, NULL);  /* internal error (should never happen) */
		pCtx->fAbort = 1;
		return RECMA55_NUM_INFINITY;
		
	case RECMA55_EXPR_TYPE_NUM_CONST:
		
		return pExpr->value.num;
		
	case RECMA55_EXPR_TYPE_NUM_VAR:
		
		return (RECMA55_NUM)((pExpr->value.pNum != NULL) ? *pExpr->value.pNum : 0.0);
		
	case RECMA55_EXPR_TYPE_FUNC:
		
		if (pExpr->value.eval.u.pFunc == NULL)
		{
			return L_EvalNumExpr(pCtx, pExpr->value.eval.pExpr);
		}
		
		return pExpr->value.eval.u.pFunc->Handler(pCtx, L_EvalNumExpr(pCtx, pExpr->value.eval.pExpr));
		
	case RECMA55_EXPR_TYPE_USER_FUNC:
		
		pExpr->value.eval.u.pUserFunc->arg = L_EvalNumExpr(pCtx, pExpr->value.eval.pExpr);
		
		if (pCtx->fAbort)
		{
			return RECMA55_NUM_INFINITY;
		}
		
		return L_EvalNumExpr(pCtx, pExpr->value.eval.u.pUserFunc->pExpr);
		
	case RECMA55_EXPR_TYPE_OP_ADD:
		
		return L_ExprOpAdd(pCtx, L_EvalNumExpr(pCtx, pExpr->value.eval.pExpr), L_EvalNumExpr(pCtx, pExpr->value.eval.u.pExpr));
		
	case RECMA55_EXPR_TYPE_OP_SUB:
		
		return L_ExprOpSub(pCtx, L_EvalNumExpr(pCtx, pExpr->value.eval.pExpr), L_EvalNumExpr(pCtx, pExpr->value.eval.u.pExpr));
		
	case RECMA55_EXPR_TYPE_OP_MUL:
		
		return L_ExprOpMul(pCtx, L_EvalNumExpr(pCtx, pExpr->value.eval.pExpr), L_EvalNumExpr(pCtx, pExpr->value.eval.u.pExpr));
		
	case RECMA55_EXPR_TYPE_OP_DIV:
		
		return L_ExprOpDiv(pCtx, L_EvalNumExpr(pCtx, pExpr->value.eval.pExpr), L_EvalNumExpr(pCtx, pExpr->value.eval.u.pExpr));
		
	case RECMA55_EXPR_TYPE_OP_INV:
		
		return L_ExprOpInv(pCtx, L_EvalNumExpr(pCtx, pExpr->value.eval.pExpr), L_EvalNumExpr(pCtx, pExpr->value.eval.u.pExpr));
		
	default:
		
		return L_EvalArray(pCtx, (RECMA55_UINT)(pExpr->type - RECMA55_EXPR_TYPE_ARRAY_BASE), L_EvalNumExpr(pCtx, pExpr->value.eval.pExpr), L_EvalNumExpr(pCtx, pExpr->value.eval.u.pExpr));
	}
}


static RECMA55_BOOL L_SetNumVar(struct RECMA55_RUN_CTX *pCtx, struct RECMA55_EXPR *pExpr, RECMA55_NUM num)
{
	RECMA55_NUM *pNum;
	
	if (pCtx->fAbort)
	{
		return 0;  /* errors have been handled already */
	}
	
	if (pExpr->type == RECMA55_EXPR_TYPE_NUM_VAR)
	{
		pNum = pExpr->value.pNum;
	}
	else
	{
		assert(pExpr->type >= RECMA55_EXPR_TYPE_ARRAY_BASE);
		
		pNum = L_GetArrayNum(pCtx, (RECMA55_UINT)(pExpr->type - RECMA55_EXPR_TYPE_ARRAY_BASE), L_EvalNumExpr(pCtx, pExpr->value.eval.pExpr), L_EvalNumExpr(pCtx, pExpr->value.eval.u.pExpr));
		
		if (pNum == NULL)
		{
			pCtx->fAbort = 1;
			return 0;
		}
	}
	
	*pNum = num;
	
	return 1;
}


static void L_ClearLineTree(struct RECMA55_LINE_NODE *pTree)
{
	if (pTree != NULL)
	{
		L_ClearLineTree(pTree->pLeft);
		L_ClearLineTree(pTree->pRight);
		L_MemFree(pTree);
	}
}


static void L_InsertLineTree(struct RECMA55_LINE_NODE **ppTree, struct RECMA55_LINE_NODE *pNode)
{
	if (*ppTree == NULL)
	{
		*ppTree = pNode;
		pNode->depth = 1;
	}
	else if ((*ppTree)->depth)
	{
		pNode->pLeft = *ppTree;
		*ppTree = pNode;
	}
	else
	{
		L_InsertLineTree(&(*ppTree)->pRight, pNode);
		
		if ((*ppTree)->pLeft->depth == (*ppTree)->pRight->depth)
		{
			(*ppTree)->depth = (*ppTree)->pLeft->depth + 1;
		}
	}
}

static RECMA55_BOOL L_LookupLineTree(struct RECMA55_LINE_NODE *pTree, union RECMA55_LINE_REF *pLineRef, const char sLineNum[RECMA55_LINE_NUM_LEN_MAX])
{
	while (pTree != NULL)
	{
		switch (L_LineNumCmp(pTree->pLine->sLineNum, pLineRef->sLineNum))
		{
		case 1:
			
			pTree = pTree->pLeft;
			break;
			
		case -1:
			
			pTree = pTree->pRight;
			break;
			
		default:
			
			pLineRef->pLine = pTree->pLine;
			return 1;
		}
	}
	
	L_ProgramErrorMsg(sLineNum, "LINE NUMBER NOT FOUND");
	
	return 0;
}


/*****************************************************************************************
 *
 *  F U N C T I O N   H A N D L E R
 *
 *****************************************************************************************/


static RECMA55_NUM RECMA55_FuncABS(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	return (RECMA55_NUM)fabs(arg);
}


static RECMA55_NUM RECMA55_FuncATN(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	return (RECMA55_NUM)atan(arg);
}


static RECMA55_NUM RECMA55_FuncCOS(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	return (RECMA55_NUM)cos(arg);
}


static RECMA55_NUM RECMA55_FuncEXP(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	return (RECMA55_NUM)exp(arg);
}


static RECMA55_NUM RECMA55_FuncINT(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	return (RECMA55_NUM)floor(arg);
}


static RECMA55_NUM RECMA55_FuncLOG(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	if (arg <= 0.0)
	{
		L_RunTimeErrorMsg(pCtx->pLineCur->sLineNum, "LOG FUNC ARGUMENT IS LESS OR EQUAL THAN ZERO");  /* see ECMA-55 chapter 9.5 */
		pCtx->fAbort = 1;
		return RECMA55_NUM_INFINITY;
	}
	
	return (RECMA55_NUM)log(arg);
}


static RECMA55_NUM RECMA55_FuncRND(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	return (RECMA55_NUM)rand() / ((RECMA55_NUM)RAND_MAX + 1);  /* SECURITY WARNING: rand() is not secure */
}


static RECMA55_NUM RECMA55_FuncSGN(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	if (arg == 0.0)
	{
		return 0.0;
	}
	
	return (RECMA55_NUM)((arg > 0.0) ? 1.0 : -1.0);
}


static RECMA55_NUM RECMA55_FuncSIN(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	return (RECMA55_NUM)sin(arg);
}


static RECMA55_NUM RECMA55_FuncSQR(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	if (arg < 0.0)
	{
		L_RunTimeErrorMsg(pCtx->pLineCur->sLineNum, "LOG FUNC ARGUMENT IS LESS OR EQUAL THAN ZERO");  /* see ECMA-55 chapter 9.5 */
		pCtx->fAbort = 1;
		return RECMA55_NUM_INFINITY;
	}
	
	return (RECMA55_NUM)sqrt(arg);
}


static RECMA55_NUM RECMA55_FuncTAN(struct RECMA55_RUN_CTX *pCtx, RECMA55_NUM arg)
{
	if (pCtx->fAbort)
	{
		return RECMA55_NUM_INFINITY;
	}
	
	return (RECMA55_NUM)tan(arg);
}


static const struct RECMA55_FUNC ltFunc[] = {
	/* array must be sorted by sName */
	{ "ABS", 1, 0, RECMA55_FuncABS },
	{ "ATN", 1, 0, RECMA55_FuncATN },
	{ "COS", 1, 0, RECMA55_FuncCOS },
	{ "EXP", 1, 0, RECMA55_FuncEXP },
	{ "INT", 1, 0, RECMA55_FuncINT },
	{ "LOG", 1, 0, RECMA55_FuncLOG },
	{ "RND", 0, 1, RECMA55_FuncRND },
	{ "SGN", 1, 0, RECMA55_FuncSGN },
	{ "SIN", 1, 0, RECMA55_FuncSIN },
	{ "SQR", 1, 0, RECMA55_FuncSQR },
	{ "TAN", 1, 0, RECMA55_FuncTAN }
};


/*****************************************************************************************
 *
 *  P A R S I N G   F U N C T I O N S
 *
 *****************************************************************************************/


static const struct RECMA55_FUNC* L_LookupFunc(const char *sLine, RECMA55_UINT *pPos)
{
	const char *sName;
	RECMA55_UINT namePos;
	RECMA55_UINT linePos;
	size_t l;
	size_t r;
	size_t m;
	
	l = 1;
	r = sizeof ltFunc / sizeof ltFunc[0];
	
	while (l <= r)
	{
		m = (l + r) / 2;
		
		sName = ltFunc[m - 1].sName;
		
		namePos = 0;
		linePos = *pPos;
		
		while (sName[namePos] == sLine[linePos])
		{
			++linePos;
			
			if (sName[++namePos] == '\0')
			{
				*pPos = linePos;
				
				return &ltFunc[m - 1];
			}
		}
		
		if (sName[namePos] < sLine[linePos])
		{
			l = m + 1;
		}
		else
		{
			r = m - 1;
		}
	}
	
	return NULL;
}


static const char* L_ParseNumRep(const char *sLine, RECMA55_UINT *pPos, RECMA55_NUM *pNum)
{
	char *sEndPtr;
	double num;
	RECMA55_UINT pos;
	
	pos = *pPos;
	
	if ((sLine[pos] == '0') && (!isdigit((int)sLine[pos + 1]) && (sLine[pos + 1] != ' ') && (sLine[pos + 1] != '.') && (sLine[pos + 1] != 'E')))
	{
		/* numeric representation is 0 (prevent valid C translation of 0x..., NAN... or INFINITY): */
		
		num = 0.0;
		
		++pos;
	}
	else
	{
		/* in any other case, use strtod() for numeric representation: */
		
		num = strtod(&sLine[pos], &sEndPtr);
		
		if (sEndPtr == &sLine[pos])
		{
			return "NUMERIC CONSTANT EXPECTED";
		}
		
		pos += (RECMA55_UINT)(sEndPtr - &sLine[pos]);
	}
	
	/* skip trailing spaces: */
	
	while (sLine[pos] == ' ')
	{
		++pos;
	}
	
	*pPos = pos;
	
	/* explicitly set RECMA55_NUM_INFINITY on overflow: */
	
	if (num >= HUGE_VAL)
	{
		*pNum = RECMA55_NUM_INFINITY;
	}
	else if (num <= -HUGE_VAL)
	{
		*pNum = -RECMA55_NUM_INFINITY;
	}
	else
	{
		*pNum = (RECMA55_NUM)num;
	}
	
	return NULL;
}


static struct RECMA55_EXPR* L_ParseStrConst(const char *sLine, RECMA55_UINT *pPos, const char sLineNum[RECMA55_LINE_NUM_LEN_MAX])
{
	const char *sErrMsg;
	struct RECMA55_EXPR *pExpr;
	RECMA55_UINT pos;
	RECMA55_UINT len;
	
	pos = *pPos;
	
	sErrMsg = L_GetQuotedStrLen(sLine, pPos, &len);  /* skips trailing spaces, keep pos for the start of the string constant */
	
	if (sErrMsg != NULL)
	{
		L_SyntaxErrorMsg(sLineNum, sErrMsg);
		return NULL;
	}
	
	if (!L_CreateExpr(&pExpr, RECMA55_EXPR_TYPE_STR_CONST))
	{
		return NULL;
	}
	
	if (len)
	{
		/* allocate and copy string if not empty: */
		
		pExpr->value.str.sStr = L_MemAlloc(len + 1);
		
		if (pExpr->value.str.sStr == NULL)
		{
			L_DestroyExpr(pExpr);
			return NULL;
		}
		
		strncpy(pExpr->value.str.sStr, &sLine[pos + 1], len);
		
		pExpr->value.str.sStr[len] = '\0';
		pExpr->value.str.len = len;
	}
	
	return pExpr;
}


static RECMA55_UINT L_ParseDim(struct RECMA55_LOAD_CTX *pCtx, RECMA55_UINT *pPos, const char sLineNum[RECMA55_LINE_NUM_LEN_MAX])
{
	RECMA55_UINT digit;
	RECMA55_UINT value;
	RECMA55_UINT pos;
	
	pos = *pPos;
	
	if (!isdigit((int)pCtx->sLine[pos]))
	{
		L_SyntaxErrorMsg(sLineNum, "INTEGER CONSTANT EXPECTED");
		return 0;
	}
	
	value = 0;
	
	do
	{
		digit = (RECMA55_UINT)(pCtx->sLine[pos] - '0');
		
		if (value > (RECMA55_UINT_MAX - digit) / 10)
		{
			L_SyntaxErrorMsg(sLineNum, "INTEGER CONSTANT OVERFLOW");
			return 0;
		}
		
		value = (value * 10) + digit;
		
		++pos;
	}
	while (isdigit((int)pCtx->sLine[pos]));
	
	if (value < pCtx->pPrepareCtx->pRunCtx->var.base)
	{
		L_SyntaxErrorMsg(sLineNum, "SUBSCRIPT BOUND MUST BE GREATER OR EQUAL THAN BASE");  /* see ECMA-55 chapter 18.4 */
		return 0;
	}
	
	while (pCtx->sLine[pos] == ' ')
	{
		++pos;
	}
	
	*pPos = pos;
	
	return value;
}


static struct RECMA55_EXPR* L_ParseNumVar(struct RECMA55_LOAD_CTX *pCtx, RECMA55_UINT *pPos, RECMA55_UINT *pArgCount, struct RECMA55_USER_FUNC *pUserFunc, const char sLineNum[RECMA55_LINE_NUM_LEN_MAX])
{
	struct RECMA55_VARIANT *pVariant;
	struct RECMA55_EXPR *pExpr;
	RECMA55_UINT argCount;
	RECMA55_UINT pos;
	RECMA55_UINT len;
	int index;
	
	argCount = 0;
	
	pos = *pPos;
	
	assert(isupper((int)pCtx->sLine[pos]));
	
	len = L_GetUserFuncArgLen(&pCtx->sLine[pos], pUserFunc);
	
	if (len)
	{
		/* simple numeric variable is an argument of a user defined function: */
		
		if (!L_CreateExpr(&pExpr, RECMA55_EXPR_TYPE_NUM_VAR))
		{
			return NULL;
		}
		
		pExpr->value.pNum = &pUserFunc->arg;
		
		pos += len;
	}
	else if (isdigit((int)pCtx->sLine[pos + 1]))
	{
		/* simple numeric variable (letter + digit): */
		
		if (!L_CreateExpr(&pExpr, RECMA55_EXPR_TYPE_NUM_VAR))
		{
			return NULL;
		}
		
		pExpr->value.pNum = &pCtx->pPrepareCtx->pRunCtx->var.ttNum[(int)pCtx->sLine[pos]][(int)pCtx->sLine[pos + 1]];
		
		pos += 2;
	}
	else
	{
		/* variant variable: */
		
		index = pCtx->sLine[pos++] - 'A';
		
		pVariant = &pCtx->pPrepareCtx->pRunCtx->var.tVariant[index];
		
		if (pVariant->tDim[0] || pVariant->tDim[1])
		{
			/* variant type has already been determined: */
			
			if (!pVariant->tDim[0])
			{
				if (!L_CreateExpr(&pExpr, RECMA55_EXPR_TYPE_NUM_VAR))
				{
					return NULL;
				}
				
				pExpr->value.pNum = &pVariant->value.num;
			}
			else if (pArgCount != NULL)
			{
				if (!L_CreateExpr(&pExpr, RECMA55_EXPR_TYPE_ARRAY_BASE + index))
				{
					return NULL;
				}
				
				argCount = pVariant->tDim[1] ? 2 : 1;
			}
			else
			{
				L_ProgramErrorMsg(sLineNum, "VARIABLE MUST NOT BE AN ARRAY");
				return NULL;
			}
		}
		else
		{
			/* skip spaces: */
			
			while (pCtx->sLine[pos] == ' ')
			{
				++pos;
			}
			
			if ((pCtx->sLine[pos] != '(') || (pArgCount == NULL))
			{
				/* variant implicitly becomes simple numeric variable: */
				
				pVariant->tDim[1] = 1;
				
				if (!L_CreateExpr(&pExpr, RECMA55_EXPR_TYPE_NUM_VAR))
				{
					return NULL;
				}
				
				pExpr->value.pNum = &pVariant->value.num;
			}
			else
			{
				/* variant implicitly becomes with upper bound(s) of 10 (see ECMA-55 chapter 7.4): */
				
				len = pos;
				
				do
				{
					++len;
				}
				while (isdigit((int)pCtx->sLine[len]) || (pCtx->sLine[len] == ' '));
				
				pCtx->fNoOptionBase = 1;
				
				pVariant->tDim[0] = 10;
				argCount = 1;
				
				if (pCtx->sLine[len] == ',')
				{
					pVariant->tDim[1] = 10;
					argCount = 2;
				}
				
				pVariant->value.tNum = NULL;
				
				if (!L_CreateArray(pVariant, pCtx->pPrepareCtx->pRunCtx->var.base))
				{
					return NULL;
				}
				
				if (!L_CreateExpr(&pExpr, RECMA55_EXPR_TYPE_ARRAY_BASE + index))
				{
					return NULL;
				}
			}
		}
	}
	
	/* skip spaces: */
	
	while (pCtx->sLine[pos] == ' ')
	{
		++pos;
	}
	
	if (pArgCount != NULL)
	{
		*pArgCount = argCount;
	}
	
	*pPos = pos;
	
	return pExpr;
}


static struct RECMA55_EXPR* L_ParseExpr(struct RECMA55_LOAD_CTX *pCtx, RECMA55_UINT *pPos, RECMA55_EXPR_MODE mode, struct RECMA55_USER_FUNC *pUserFunc, const char sLineNum[RECMA55_LINE_NUM_LEN_MAX])
{
	const char *sErrMsg;
	const struct RECMA55_FUNC *pFunc;
	struct RECMA55_EXPR **ppExpr;
	struct RECMA55_EXPR *pExpr;
	struct RECMA55_EXPR *pTemp;
	RECMA55_UINT argCount;
	RECMA55_NUM num;
	RECMA55_UINT index;
	RECMA55_UINT pos;
	RECMA55_EXPR_TYPE type;
	RECMA55_EXPR_PRIO prio;
	
	pExpr = NULL;
	ppExpr = &pExpr;
	
	pos = *pPos;
	
	/* optional sign: */
	
	if ((pCtx->sLine[pos] == '+') || (pCtx->sLine[pos] == '-'))
	{
		if ((mode != RECMA55_EXPR_MODE_NUM) && (mode != RECMA55_EXPR_MODE_ANY))
		{
			L_ErrorMsgExprMode(sLineNum, mode);
			return NULL;
		}
		
		if (pCtx->sLine[pos] == '-')
		{
			/* leading minus sign (first operand pointing to NULL which will be evaluated to 0.0): */
			
			if (!L_CreateExpr(&pExpr, RECMA55_EXPR_TYPE_OP_SUB))
			{
				L_DestroyExpr(pExpr);
				return NULL;
			}
			
			pExpr->prio = RECMA55_EXPR_PRIO_ADD_SUB;
			
			ppExpr = &(*ppExpr)->value.eval.u.pExpr;
		}
		
		/* skip leading sign and subsequent spaces: */
		
		do
		{
			++pos;
		}
		while (pCtx->sLine[pos] == ' ');
	}
	
	/* main loop: */
	
	do
	{
		argCount = 0;
		
		if (isdigit((int)pCtx->sLine[pos]) || (pCtx->sLine[pos] == '.'))
		{
			/* numeric representation: */
			
			if ((mode != RECMA55_EXPR_MODE_NUM) && (mode != RECMA55_EXPR_MODE_ANY))
			{
				L_ErrorMsgExprMode(sLineNum, mode);
				return NULL;
			}
			
			sErrMsg = L_ParseNumRep(pCtx->sLine, &pos, &num);
			
			if (sErrMsg != NULL)
			{
				L_SyntaxErrorMsg(sLineNum, sErrMsg);
				return NULL;
			}
			
			if (!L_CreateExpr(ppExpr, RECMA55_EXPR_TYPE_NUM_CONST))
			{
				L_DestroyExpr(pExpr);
				return NULL;
			}
			
			(*ppExpr)->value.num = num;
		}
		else if (pCtx->sLine[pos] == '\"')
		{
			/* string constant: */
			
			if ((mode != RECMA55_EXPR_MODE_STR) && (mode != RECMA55_EXPR_MODE_ANY))
			{
				L_ErrorMsgExprMode(sLineNum, mode);
				return NULL;
			}
			
			*ppExpr = L_ParseStrConst(pCtx->sLine, &pos, sLineNum);
			
			if (*ppExpr == NULL)
			{
				L_DestroyExpr(pExpr);
				return NULL;
			}
		}
		else if (isupper((int)pCtx->sLine[pos]) && (pCtx->sLine[pos + 1] == '$'))
		{
			/* string variable: */
			
			if ((mode != RECMA55_EXPR_MODE_STR) && (mode != RECMA55_EXPR_MODE_VAR) && (mode != RECMA55_EXPR_MODE_ANY))
			{
				L_ErrorMsgExprMode(sLineNum, mode);
				return NULL;
			}
			
			if (!L_CreateExpr(ppExpr, RECMA55_EXPR_TYPE_STR_VAR))
			{
				L_DestroyExpr(pExpr);
				return NULL;
			}
			
			pExpr->value.str.sStr = pCtx->pPrepareCtx->pRunCtx->var.tsStr[pCtx->sLine[pos] - 'A'];
			
			pos += 2;
		}
		else if (isupper((int)pCtx->sLine[pos]) && !isalpha((int)pCtx->sLine[pos + 1]))
		{
			/* numeric variable: */
			
			if ((mode != RECMA55_EXPR_MODE_NUM) && (mode != RECMA55_EXPR_MODE_VAR) && (mode != RECMA55_EXPR_MODE_ANY))
			{
				L_ErrorMsgExprMode(sLineNum, mode);
				return NULL;
			}
			
			*ppExpr = L_ParseNumVar(pCtx, &pos, &argCount, pUserFunc, sLineNum);
			
			if (*ppExpr == NULL)
			{
				L_DestroyExpr(pExpr);
				return NULL;
			}
		}
		else if ((pCtx->sLine[pos] == 'F') && (pCtx->sLine[pos + 1] == 'N') && isupper((int)pCtx->sLine[pos + 2]))
		{
			/* user defined function: */
			
			if ((mode != RECMA55_EXPR_MODE_NUM) && (mode != RECMA55_EXPR_MODE_ANY))
			{
				L_ErrorMsgExprMode(sLineNum, mode);
				return NULL;
			}
			
			index = (RECMA55_UINT)(pCtx->sLine[pos + 2] - 'A');
			
			if (pCtx->pPrepareCtx->pRunCtx->tUserFunc[index].pExpr == NULL)
			{
				L_ProgramErrorMsg(sLineNum, "USER DEFINED FUNCTION NOT DEFINED");  /* see ECMA-55 chapter 10.4 (function definition shall occur in a lower numbered line) */
				L_DestroyExpr(pExpr);
				return NULL;
			}
			
			if (!L_CreateExpr(ppExpr, RECMA55_EXPR_TYPE_USER_FUNC))
			{
				L_DestroyExpr(pExpr);
				return NULL;
			}
			
			(*ppExpr)->value.eval.u.pUserFunc = &pCtx->pPrepareCtx->pRunCtx->tUserFunc[index];
			
			pos += 3;
			
			if ((*ppExpr)->value.eval.u.pUserFunc->sArgVar[0] != '\0')
			{
				argCount = 1;
			}
		}
		else if (isupper((int)pCtx->sLine[pos]))
		{
			/* implementation supplied function: */
			
			if ((mode != RECMA55_EXPR_MODE_NUM) && (mode != RECMA55_EXPR_MODE_ANY))
			{
				L_ErrorMsgExprMode(sLineNum, mode);
				return NULL;
			}
			
			pFunc = L_LookupFunc(pCtx->sLine, &pos);
			
			if (pFunc == NULL)
			{
				L_ProgramErrorMsg(sLineNum, "INVALID IDENTIFIER IN EXPRESSION");
				L_DestroyExpr(pExpr);
				return NULL;
			}
			
			if (pFunc->fPRNG)
			{
				pCtx->fPRNG = 1;
			}
			
			if (!L_CreateExpr(ppExpr, RECMA55_EXPR_TYPE_FUNC))
			{
				L_DestroyExpr(pExpr);
				return NULL;
			}
			
			(*ppExpr)->value.eval.u.pFunc = pFunc;  /* as L_CreateExpr() did not return 0, *ppExpr is not NULL */
			
			if (pFunc->fArg)
			{
				argCount = 1;
			}
		}
		
		/* skip spaces: */
		
		while (pCtx->sLine[pos] == ' ')
		{
			++pos;
		}
		
		if ((*ppExpr == NULL) || argCount)
		{
			/* left parenthesis expected (array subscript, function argument or primary expression): */
			
			if (pCtx->sLine[pos] != '(')
			{
				if (argCount)
				{
					L_SyntaxErrorMsg(sLineNum, "\'(\' EXPECTED");
				}
				else if (pCtx->sLine[pos] == '\0')
				{
					L_SyntaxErrorMsg(sLineNum, "OPERAND EXPECTED");
				}
				else
				{
					L_SyntaxErrorMsg(sLineNum, "INVALID CHARACTER IN EXPRESSION");
				}
				
				L_DestroyExpr(pExpr);
				return NULL;
			}
			
			/* skip left parenthesis and subsequent spaces: */
			
			do
			{
				++pos;
			}
			while (pCtx->sLine[pos] == ' ');
			
			if (argCount)
			{
				/* array subscript or function argument: */
				
				assert(*ppExpr != NULL);
				
				(*ppExpr)->value.eval.pExpr = L_ParseExpr(pCtx, &pos, RECMA55_EXPR_MODE_NUM, pUserFunc, sLineNum);
				
				if ((*ppExpr)->value.eval.pExpr == NULL)
				{
					L_DestroyExpr(pExpr);
					return NULL;
				}
				
				if (argCount >= 2)
				{
					/* subscript of two-dimensional array: */
					
					if (pCtx->sLine[pos] != ',')
					{
						L_ProgramErrorMsg(sLineNum, "\',\' EXPECTED IN SUBSCRIPT FOR TWO-DIMENSIONAL ARRAY");
						L_DestroyExpr(pExpr);
						return NULL;
					}
					
					do
					{
						++pos;
					}
					while (pCtx->sLine[pos] == ' ');
					
					(*ppExpr)->value.eval.u.pExpr = L_ParseExpr(pCtx, &pos, RECMA55_EXPR_MODE_NUM, pUserFunc, sLineNum);
					
					if ((*ppExpr)->value.eval.u.pExpr == NULL)
					{
						L_DestroyExpr(pExpr);
						return NULL;
					}
				}
			}
			else
			{
				/* primary numeric expression: */
				
				if ((mode != RECMA55_EXPR_MODE_NUM) && (mode != RECMA55_EXPR_MODE_ANY))
				{
					L_ErrorMsgExprMode(sLineNum, mode);
					return NULL;
				}
				
				*ppExpr = L_ParseExpr(pCtx, &pos, mode, pUserFunc, sLineNum);
				
				if (*ppExpr == NULL)
				{
					L_DestroyExpr(pExpr);
					return NULL;
				}
				
				(*ppExpr)->prio = RECMA55_EXPR_PRIO_PRIMARY;
			}
			
			if (pCtx->sLine[pos] != ')')
			{
				L_SyntaxErrorMsg(sLineNum, "\')\' EXPECTED");
				L_DestroyExpr(pExpr);
				return NULL;
			}
			
			/* skip right parenthesis and subsequent spaces: */
			
			do
			{
				++pos;
			}
			while (pCtx->sLine[pos] == ' ');
		}
		
		assert(*ppExpr != NULL);
		
		if ((pExpr->prio != RECMA55_EXPR_PRIO_PRIMARY) && pExpr->value.eval.pExpr != NULL)
		{
			/* rotate tree depending on the operator's priority: */
			
			ppExpr = &pExpr;
			
			while ((*ppExpr)->value.eval.pExpr->prio > (*ppExpr)->prio)
			{
				pTemp = (*ppExpr)->value.eval.pExpr;                      /* save first operand which is an operator of less priority */
				(*ppExpr)->value.eval.pExpr = pTemp->value.eval.u.pExpr;  /* set first operand to second operand of first operand */
				pTemp->value.eval.u.pExpr = *ppExpr;                      /* set second operand of first operand to operator of higher priority */
				*ppExpr = pTemp;                                          /* make operand of less priority new root of the expression */
				ppExpr = &pTemp->value.eval.u.pExpr;
				
				assert(*ppExpr != NULL);
			}
		}
		
		if ((mode == RECMA55_EXPR_MODE_NUM) || (mode == RECMA55_EXPR_MODE_ANY))
		{
			/* binary numeric operator (if given, *ppExpr is NULL in order to loop for the second operand): */
			
			type = RECMA55_EXPR_TYPE_OP_ADD;
			prio = RECMA55_EXPR_PRIO_ADD_SUB;
			
			switch (pCtx->sLine[pos])
			{
			case '^':
				
				++type;
				--prio;
				/* no break */
				
			case '/':
				
				++type;
				/* no break */
				
			case '*':
				
				++type;
				--prio;
				/* no break */
				
			case '-':
				
				++type;
				/* no break */
				
			case '+':
				
				/* skip operator and subsequent spaces: */
				
				do
				{
					++pos;
				}
				while (pCtx->sLine[pos] == ' ');
				
				if (!L_CreateExpr(&pTemp, type))
				{
					L_DestroyExpr(pExpr);
					return NULL;
				}
				
				pTemp->prio = prio;
				pTemp->value.eval.pExpr = pExpr;
				pExpr = pTemp;
				ppExpr = &pExpr->value.eval.u.pExpr;
				break;
			}
		}
	}
	while (*ppExpr == NULL);
	
	*pPos = pos;
	
	return pExpr;
}


static const char* L_ParseLineNumber(const char *sLine, RECMA55_UINT *pPos, char sLineNum[RECMA55_LINE_NUM_LEN_MAX], RECMA55_BOOL fAllowComma)
{
	RECMA55_UINT pos;
	RECMA55_UINT len;
	
	pos = *pPos;
	
	if (!isdigit((int)sLine[pos]))
	{
		return "LINE NUMBER EXPECTED";
	}
	
	while (sLine[pos] == '0')
	{
		++pos;  /* see ECMA-55 chapter 5.4 (ignore leading zeros) */
	}
	
	if (!isdigit((int)sLine[pos]))
	{
		return "BAD LINE NUMBER (NON-ZERO DECIMAL VALUE EXPECTED)";
	}
	
	len = 0;
	
	do
	{
		if (len >= RECMA55_LINE_NUM_LEN_MAX)
		{
			return "BAD LINE NUMBER (LINE NUMBER HAS TOO MANY DIGITS)";
		}
		
		sLineNum[len++] = sLine[pos++];
	}
	while ((sLine[pos] >= '0') && (sLine[pos] <= '9'));
	
	while (len < RECMA55_LINE_NUM_LEN_MAX)
	{
		sLineNum[len++] = '\0';
	}
	
	if ((sLine[pos] != '\0') && (sLine[pos] != ' ') && ((sLine[pos] != ',') || !fAllowComma))
	{
		return "BAD LINE NUMBER (INVALID CHARACTER)";
	}
	
	while (sLine[pos] == ' ')
	{
		++pos;
	}
	
	*pPos = pos;
	
	return NULL;
}


static const char* RECMA55_ParseData(const char *sStr, struct RECMA55_DATA ***pppData)
{
	const char *sErrMsg;
	struct RECMA55_DATA *pData;
	RECMA55_UINT pos;
	RECMA55_UINT len;
	RECMA55_UINT fQuoted;
	
	/* see ECMA-55 chapters 15 and 17: */
	
	assert(pppData != NULL);
	assert(*pppData != NULL);
	assert(**pppData == NULL);
	
	pos = 0;
	
	do
	{
		while (sStr[pos] == ' ')
		{
			++pos;  /* skip leading spaces */
		}
		
		sStr += pos;
		
		pos = 0;
		len = 0;
		fQuoted = 0;
		
		if (sStr[pos] == '\"')
		{
			fQuoted = 1;
			
			sErrMsg = L_GetQuotedStrLen(sStr, &pos, &len);
			
			if (sErrMsg != NULL)
			{
				return sErrMsg;
			}
			
			if ((sStr[pos] != '\0') && (sStr[pos] != ','))
			{
				return "\',\' EXPECTED";
			}
		}
		else if (L_IsPlainStrChar(sStr[pos]))
		{
			len = ++pos;
			
			while ((sStr[pos] != '\0') && (sStr[pos] != ','))
			{
				if (sStr[pos] == ' ')
				{
					++pos;
				}
				else if (L_IsPlainStrChar(sStr[pos]))
				{
					len = ++pos;
				}
				else
				{
					return "INVALID DATUM";
				}
			}
		}
		else
		{
			return "DATUM EXPECTED";
		}
		
		pData = L_MemAlloc(sizeof (struct RECMA55_DATA));
		
		if (pData == NULL)
		{
			return "";  /* errors already have been handled */
		}
		
		pData->pNext = NULL;
		pData->str.sStr = NULL;
		pData->str.len = len;
		pData->fQuoted = fQuoted;
		
		if (len)
		{
			pData->str.sStr = L_MemAlloc(len + 1);
			
			if (pData->str.sStr == NULL)
			{
				L_MemFree(pData);
				return "";  /* errors already have been handled */
			}
			
			strncpy(pData->str.sStr, &sStr[fQuoted], len);
			
			pData->str.sStr[len] = '\0';
		}
		
		**pppData = pData;
		*pppData = &pData->pNext;
	}
	while (sStr[pos++] == ',');
	
	return NULL;
}


static const char* RECMA55_ParseInputData(char *sStr, struct RECMA55_DATA ***pppData, RECMA55_BOOL fFullChar)
{
	RECMA55_UINT pos;
	int c;
	RECMA55_BOOL fString;
	
	fString = 0;
	
	for (pos = 0; sStr[pos] != '\0'; ++pos)
	{
		c = sStr[pos];
		
		if (!L_TranslateInputChar(&c, fString, fFullChar))
		{
			return "CHARACTER(S) NOT ALLOWED";
		}
		
		sStr[pos] = c;
		
		if (c == '\"')
		{
			fString = 1 - fString;
		}
	}
	
	return RECMA55_ParseData(sStr, pppData);
}


static const char* L_ParseVarList(struct RECMA55_RUN_CTX *pCtx, struct RECMA55_PARAM_VAR *pVarList, const struct RECMA55_DATA *pData, RECMA55_BOOL fInput)
{
	const char *sErrMsg;
	struct RECMA55_PARAM_VAR *pVar;
	RECMA55_UINT pos;
	
	/* validate data: */
	
	for (pVar = pVarList; pVar != NULL; pVar = pVar->pNext)
	{
		if (pData == NULL)
		{
			return "DATA MISSING";
		}
		
		if (pVar->pExpr->type != RECMA55_EXPR_TYPE_STR_VAR)
		{
			if (pData->fQuoted)
			{
				return "NUMERIC EXPECTED";
			}
			
			pos = 0;
			
			sErrMsg = L_ParseNumRep(pData->str.sStr, &pos, &pVar->num);
			
			if (sErrMsg != NULL)
			{
				return sErrMsg;
			}
			
			if ((pVar->num == RECMA55_NUM_INFINITY) || (pVar->num == -RECMA55_NUM_INFINITY))
			{
				return "NUMERIC OVERFLOW";
			}
			
			if (pData->str.sStr[pos] != '\0')
			{
				return "INVALID NUMERIC";
			}
		}
		else if (pData->str.len > RECMA55_STR_VAR_LEN_MAX)
		{
			return "STRING CONTAINS TOO MAY CHARACTERS";  /* see ECMA-55 chapter 11.5 */
		}
		
		pData = pData->pNext;
	}
	
	if (fInput)
	{
		/* validate number of input items before assigning variables (see ECMA-55 chapter 15.4): */
		
		if (pData != NULL)
		{
			return "UNEXPECTED DATA";
		}
	}
	
	return NULL;
}


static RECMA55_BOOL L_AssignVarList(struct RECMA55_RUN_CTX *pCtx, struct RECMA55_PARAM_VAR *pVarList, const struct RECMA55_DATA **ppData)
{
	const struct RECMA55_DATA *pData;
	struct RECMA55_PARAM_VAR *pVar;
	
	pData = *ppData;
	
	for (pVar = pVarList; pVar != NULL; pVar = pVar->pNext)
	{
		assert(pData != NULL);
		
		if (pVar->pExpr->type == RECMA55_EXPR_TYPE_STR_VAR)
		{
			strcpy(pVar->pExpr->value.str.sStr, L_GetStr(pData->str.sStr));
		}
		else if (!L_SetNumVar(pCtx, pVar->pExpr, pVar->num))
		{
			return 0;
		}
		
		pData = pData->pNext;
	}
	
	*ppData = pData;
	
	return 1;
}


static RECMA55_BOOL L_Input(struct RECMA55_RUN_CTX *pCtx, struct RECMA55_PARAM_VAR *pVarList, const char *sPrompt)
{
	const char *sErrMsg;
	struct RECMA55_DATA *pList;
	struct RECMA55_DATA *pTemp;
	struct RECMA55_DATA **ppTail;
	struct RECMA55_INPUT_BUF inputBuf;
	struct RECMA55_INPUT_BUF *pInputBuf;
	char *sStr;
	RECMA55_UINT len;
	
	/* see ECMA-55 chapter 15: */
	
	inputBuf.pNext = NULL;
	
	do
	{
		assert(inputBuf.pNext == NULL);
		
		if (!pCtx->fBatchMode)
		{
			if (!pCtx->fOutput)
			{
				printf("\n");
				pCtx->fOutput = 1;
			}
			
			printf("%s%s", sPrompt, RECMA55_INPUT_PROMPT);
		}
		
		len = 0;
		pInputBuf = &inputBuf;
		
		do
		{
			if (fgets(pInputBuf->sStr, RECMA55_INPUT_BUF_LEN + 1, stdin) == NULL)
			{
				L_GenericErrorMsg("INPUT READ FAILED");
				L_DestroyInputBuf(&inputBuf.pNext);
				return 0;
			}
			
			pInputBuf->len = (RECMA55_UINT)strlen(pInputBuf->sStr);
			
			while (pInputBuf->len && ((pInputBuf->sStr[pInputBuf->len - 1] == '\r') || (pInputBuf->sStr[pInputBuf->len - 1] == '\n')))
			{
				--pInputBuf->len;
			}
			
			if (!pInputBuf->len || (pInputBuf->sStr[pInputBuf->len] != '\0'))
			{
				pInputBuf->sStr[pInputBuf->len] = '\0';
			}
			else
			{
				pInputBuf->pNext = L_MemAlloc(sizeof(struct RECMA55_INPUT_BUF));
				
				if (pInputBuf->pNext == NULL)
				{
					L_DestroyInputBuf(&inputBuf.pNext);
					return 0;
				}
				
				pInputBuf->pNext->pNext = NULL;
			}
			
			len += pInputBuf->len;
			
			pInputBuf = pInputBuf->pNext;
		}
		while (pInputBuf != NULL);
		
		if (pCtx->fBatchMode)
		{
			++pCtx->inputRow;
		}
		
		pList = NULL;
		ppTail = &pList;
		
		if (inputBuf.pNext == NULL)
		{
			/* single input buffer: */
			
			sErrMsg = RECMA55_ParseInputData(inputBuf.sStr, &ppTail, pCtx->fFullChar);
		}
		else
		{
			/* build string from multiple input buffers: */
			
			sStr = L_MemAlloc(len + 1);
			
			if (sStr == NULL)
			{
				L_DestroyInputBuf(&inputBuf.pNext);
				return 0;
			}
			
			len = 0;
			
			for (pInputBuf = &inputBuf; pInputBuf != NULL; pInputBuf = pInputBuf->pNext)
			{
				strcpy(&sStr[len], pInputBuf->sStr);
				len += pInputBuf->len;
			}
			
			L_DestroyInputBuf(&inputBuf.pNext);
			
			sErrMsg = RECMA55_ParseInputData(sStr, &ppTail, pCtx->fFullChar);
			
			L_MemFree(sStr);
		}
		
		if (sErrMsg == NULL)
		{
			sErrMsg = L_ParseVarList(pCtx, pVarList, pList, 1);
		}
		
		if (sErrMsg != NULL)
		{
			L_InputErrorMsg(pCtx->inputRow, sErrMsg);
			
			if (pCtx->fBatchMode)
			{
				return 0;
			}
		}
		else
		{
			pTemp = pList;
			
			if (!L_AssignVarList(pCtx, pVarList, (const struct RECMA55_DATA**)&pTemp))
			{
				if (pCtx->fBatchMode)
				{
					return 0;
				}
				
				sErrMsg = "";  /* loop */
			}
		}
		
		L_DestroyData(&pList);
	}
	while (sErrMsg != NULL);
	
	return 1;
}


/*****************************************************************************************
 *
 *  S T A T E M E N T   H A N D L E R
 *
 *****************************************************************************************/


static RECMA55_BOOL RECMA55_BuildStatementDATA(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	const char *sErrMsg;
	
	sErrMsg = RECMA55_ParseData(&pCtx->sLine[pos], &pCtx->ppDataTail);
	
	if (sErrMsg != NULL)
	{
		L_SyntaxErrorMsg(pLine->sLineNum, sErrMsg);
		return 0;
	}
	
	return 1;
}


static RECMA55_BOOL RECMA55_BuildStatementDEF(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	struct RECMA55_USER_FUNC *pUserFunc;
	RECMA55_UINT index;
	
	/* see ECMA-55 chapter 10: */
	
	if ((pCtx->sLine[pos] != 'F') && (pCtx->sLine[pos + 1] != 'N') && !isupper((int)pCtx->sLine[pos + 2]))
	{
		L_SyntaxErrorMsg(pLine->sLineNum, "USER DEFINED FUNCTION NAME (\'FN\' LETTER) EXPECTED");
		return 0;
	}
	
	index = (RECMA55_UINT)(pCtx->sLine[pos + 2] - 'A');
	
	pUserFunc = &pCtx->pPrepareCtx->pRunCtx->tUserFunc[index];
	
	if (pUserFunc->pExpr != NULL)
	{
		L_ProgramErrorMsg(pLine->sLineNum, "USER DEFINED FUNCTION HAS ALREADY BEEN DEFINED");
		return 0;
	}
	
	pos += 3;
	
	while (pCtx->sLine[pos] == ' ')
	{
		++pos;
	}
	
	assert(pUserFunc->sArgVar[0] == '\0');
	
	if (pCtx->sLine[pos] == '(')
	{
		do
		{
			++pos;
		}
		while (pCtx->sLine[pos] == ' ');
		
		if (!isupper((int)pCtx->sLine[pos]))
		{
			L_SyntaxErrorMsg(pLine->sLineNum, "SIMPLE NUMERIC VARIABLE AS ARGUMENT EXPECTED");
			return 0;
		}
		
		pUserFunc->sArgVar[0] = pCtx->sLine[pos++];
		
		if (isdigit((int)pCtx->sLine[pos]))
		{
			pUserFunc->sArgVar[1] = pCtx->sLine[pos++];
		}
		else
		{
			pUserFunc->sArgVar[1] = '\0';
		}
		
		while (pCtx->sLine[pos] == ' ')
		{
			++pos;
		}
		
		if (pCtx->sLine[pos] != ')')
		{
			L_SyntaxErrorMsg(pLine->sLineNum, "\')\' EXPECTED");
			return 0;
		}
		
		do
		{
			++pos;
		}
		while (pCtx->sLine[pos] == ' ');
	}
	
	if (pCtx->sLine[pos] != '=')
	{
		L_SyntaxErrorMsg(pLine->sLineNum, "\'=\' EXPECTED");
		return 0;
	}
	
	do
	{
		++pos;
	}
	while (pCtx->sLine[pos] == ' ');
	
	pUserFunc->pExpr = L_ParseExpr(pCtx, &pos, RECMA55_EXPR_MODE_NUM, pUserFunc, pLine->sLineNum);
	
	if (pUserFunc->pExpr == NULL)
	{
		return 0;
	}
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static RECMA55_BOOL RECMA55_BuildStatementDIM(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	struct RECMA55_VARIANT *pVariant;
	
	/* see ECMA-55 chapter 18: */
	
	pCtx->fNoOptionBase = 1;
	
	--pos;
	
	do
	{
		do
		{
			++pos;
		}
		while (pCtx->sLine[pos] == ' ');
		
		if (!isupper((int)pCtx->sLine[pos]))
		{
			L_SyntaxErrorMsg(pLine->sLineNum, "LETTER EXPECTED");
			return 0;
		}
		
		pVariant = &pCtx->pPrepareCtx->pRunCtx->var.tVariant[(RECMA55_UINT)(pCtx->sLine[pos] - 'A')];
		
		if (pVariant->tDim[0] || pVariant->tDim[1])
		{
			L_SyntaxErrorMsg(pLine->sLineNum, "DIM NO MORE ALLOWED");
			return 0;
		}
		
		do
		{
			++pos;
		}
		while (pCtx->sLine[pos] == ' ');
		
		if (pCtx->sLine[pos] != '(')
		{
			L_SyntaxErrorMsg(pLine->sLineNum, "\'(\' EXPECTED");
			return 0;
		}
		
		do
		{
			++pos;
		}
		while (pCtx->sLine[pos] == ' ');
		
		pVariant->tDim[0] = L_ParseDim(pCtx, &pos, pLine->sLineNum);
		
		if (!pVariant->tDim[0])
		{
			return 0;
		}
		
		pVariant->value.tNum = NULL;
		
		if (pCtx->sLine[pos] == ',')
		{
			do
			{
				++pos;
			}
			while (pCtx->sLine[pos] == ' ');
			
			pVariant->tDim[1] = L_ParseDim(pCtx, &pos, pLine->sLineNum);
			
			if (!pVariant->tDim[1])
			{
				return 0;
			}
		}
		
		if (pCtx->sLine[pos] != ')')
		{
			L_SyntaxErrorMsg(pLine->sLineNum, "\')\' EXPECTED");
			return 0;
		}
		
		do
		{
			++pos;
		}
		while (pCtx->sLine[pos] == ' ');
		
		if (!L_CreateArray(pVariant, pCtx->pPrepareCtx->pRunCtx->var.base))
		{
			return 0;
		}
	}
	while (pCtx->sLine[pos] == ',');
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static RECMA55_BOOL RECMA55_BuildStatementEND(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	/* see ECMA-55 chapter 5: */
	
	pCtx->fEnd = 1;
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static RECMA55_STATE RECMA55_ExecStatementFOR(struct RECMA55_RUN_CTX *pCtx)
{
	struct RECMA55_PARAM_FOR *pParam;
	
	/* see ECMA-55 chapter 13.4: */
	
	pParam = pCtx->pLineCur->param.pFor;
	
	pParam->state.tNum[0] = L_EvalNumExpr(pCtx, pParam->tpExpr[1]);  /* limit */
	
	if (pCtx->fAbort)
	{
		return RECMA55_STATE_FATAL_ERROR;
	}
	
	if (pParam->tpExpr[2] == NULL)
	{
		pParam->state.tNum[1] = 1.0;  /* increment (implicit) */
	}
	else
	{
		pParam->state.tNum[1] = L_EvalNumExpr(pCtx, pParam->tpExpr[2]);  /* increment (explicit) */
		
		if (pCtx->fAbort)
		{
			return RECMA55_STATE_FATAL_ERROR;
		}
	}
	
	*pParam->pNum = L_EvalNumExpr(pCtx, pParam->tpExpr[0]);  /* initial-value */
	
	if (pCtx->fAbort)
	{
		return RECMA55_STATE_FATAL_ERROR;
	}
	
	pCtx->fNoIncrement = 1;  /* prevent NEXT statement to increment */
	
	pCtx->pLineNext = pParam->pLine;  /* continue with NEXT statement in order to check limit */
	
	return RECMA55_STATE_CONTINUE;
}


static RECMA55_BOOL RECMA55_PrepareStatementFOR(struct RECMA55_PREPARE_CTX *pCtx, struct RECMA55_LINE *pLine)
{
	struct RECMA55_PARAM_FOR *pParam;
	
	pParam = pLine->param.pFor;
	
	if (pParam->pLine == NULL)
	{
		L_ProgramErrorMsg(pLine->sLineNum, "FOR WITHOUT NEXT");
		return 0;
	}
	
	pParam->state.tNum[0] = RECMA55_NUM_INFINITY;
	
	return 1;
}


static RECMA55_BOOL RECMA55_BuildStatementFOR(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	struct RECMA55_PARAM_FOR *pParam;
	struct RECMA55_LINE *pTemp;
	struct RECMA55_EXPR *pExpr;
	
	pLine->param.pFor = NULL;
	
	/* determine control variable: */
	
	pExpr = L_ParseNumVar(pCtx, &pos, NULL, NULL, pLine->sLineNum);
	
	if (pExpr == NULL)
	{
		return 0;
	}
	
	assert(pExpr->type == RECMA55_EXPR_TYPE_NUM_VAR);
	
	for (pTemp = pCtx->pPrepareCtx->pLineStack; pTemp != NULL; pTemp = pTemp->param.pFor->state.pLinePrev)
	{
		if (pExpr->value.pNum == pTemp->param.pFor->pNum)
		{
			L_ProgramErrorMsg(pLine->sLineNum, "CONTROL VARIABLE IS ALREADY USED");  /* see ECMA-55 chapter 13.4 */
			L_DestroyExpr(pExpr);
			return 0;
		}
	}
	
	/* check and skip equal sign and subsequent spaces: */
	
	if (pCtx->sLine[pos] != '=')
	{
		L_SyntaxErrorMsg(pLine->sLineNum, "\'=\' EXPECTED");
		L_DestroyExpr(pLine->param.pFor->tpExpr[0]);
		return 0;
	}
	
	do
	{
		++pos;
	}
	while (pCtx->sLine[pos] == ' ');
	
	/* create and initialize param: */
	
	pLine->param.pFor = L_MemAlloc(sizeof(struct RECMA55_PARAM_FOR));
	
	if (pLine->param.pFor == NULL)
	{
		return 0;
	}
	
	pParam = pLine->param.pFor;
	
	pParam->pNum = pExpr->value.pNum;
	pParam->tpExpr[0] = NULL;
	pParam->tpExpr[1] = NULL;
	pParam->tpExpr[2] = NULL;
	pParam->pLine = NULL;
	pParam->state.pLinePrev = pCtx->pPrepareCtx->pLineStack;
	
	pCtx->pPrepareCtx->pLineStack = pLine;
	
	L_DestroyExpr(pExpr);
	
	pParam->tpExpr[0] = L_ParseExpr(pCtx, &pos, RECMA55_EXPR_MODE_NUM, NULL, pLine->sLineNum);
	
	if (pParam->tpExpr[0] == NULL)
	{
		return 0;
	}
	
	if (!L_CheckKeyword(pCtx->sLine, &pos, "\'TO\' EXPECTED", pLine->sLineNum))
	{
		return 0;
	}
	
	pParam->tpExpr[1] = L_ParseExpr(pCtx, &pos, RECMA55_EXPR_MODE_NUM, NULL, pLine->sLineNum);
	
	if (pParam->tpExpr[1] == NULL)
	{
		return 0;
	}
	
	if (pCtx->sLine[pos] == '\0')
	{
		return 1;  /* no STEP clause */
	}
	
	if (!L_CheckKeyword(pCtx->sLine, &pos, "\'STEP\' EXPECTED", pLine->sLineNum))
	{
		return 0;
	}
	
	pParam->tpExpr[2] = L_ParseExpr(pCtx, &pos, RECMA55_EXPR_MODE_NUM, NULL, pLine->sLineNum);
	
	if (pParam->tpExpr[2] == NULL)
	{
		return 0;
	}
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static void RECMA55_ClearStatementFOR(union RECMA55_PARAM *pParam)
{
	if (pParam->pFor != NULL)
	{
		L_DestroyExpr(pParam->pFor->tpExpr[0]);
		L_DestroyExpr(pParam->pFor->tpExpr[1]);
		L_DestroyExpr(pParam->pFor->tpExpr[2]);
		L_MemFree(pParam->pFor);
	}
}


static RECMA55_STATE RECMA55_ExecStatementGOSUB(struct RECMA55_RUN_CTX *pCtx)
{
	struct RECMA55_RETURN_STACK *pStack;
	
	pStack = L_MemAlloc(sizeof (struct RECMA55_RETURN_STACK));
	
	if (pStack == NULL)
	{
		return RECMA55_STATE_FATAL_ERROR;
	}
	
	pStack->pPrev = pCtx->pStack;
	pStack->pLine = pCtx->pLineNext;
	
	pCtx->pStack = pStack;
	
	pCtx->pLineNext = pCtx->pLineCur->param.ref.pLine;
	
	return RECMA55_STATE_CONTINUE;
}


static RECMA55_STATE RECMA55_ExecStatementGOTO(struct RECMA55_RUN_CTX *pCtx)
{
	pCtx->pLineNext = pCtx->pLineCur->param.ref.pLine;
	
	return RECMA55_STATE_CONTINUE;
}


static RECMA55_BOOL RECMA55_PrepareStatementGO(struct RECMA55_PREPARE_CTX *pCtx, struct RECMA55_LINE *pLine)
{
	return L_LookupLineTree(pCtx->pLineTree, &pLine->param.ref, pLine->sLineNum);
}


static RECMA55_BOOL RECMA55_BuildStatementGO(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	const char *sErrMsg;
	
	/* see ECMA-55 chapter 12: */
	
	sErrMsg = L_ParseLineNumber(pCtx->sLine, &pos, pLine->param.ref.sLineNum, 0);
	
	if (sErrMsg != NULL)
	{
		L_SyntaxErrorMsg(pLine->sLineNum, sErrMsg);
		return 0;
	}
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static RECMA55_STATE RECMA55_ExecStatementIF(struct RECMA55_RUN_CTX *pCtx)
{
	struct RECMA55_PARAM_IF *pParam;
	RECMA55_NUM tNum[2];
	
	pParam = pCtx->pLineCur->param.pIf;
	
	if ((pParam->tpExpr[0]->type == RECMA55_EXPR_TYPE_STR_CONST) || (pParam->tpExpr[0]->type == RECMA55_EXPR_TYPE_STR_VAR))
	{
		/* string compare: */
		
		if (strcmp(L_GetStr(pParam->tpExpr[0]->value.str.sStr), L_GetStr(pParam->tpExpr[1]->value.str.sStr)))
		{
			if (pParam->relation == RECMA55_RELATION_NOT_EQUAL)
			{
				pCtx->pLineNext = pParam->ref.pLine;
			}
		}
		else
		{
			if (pParam->relation == RECMA55_RELATION_EQUAL)
			{
				pCtx->pLineNext = pParam->ref.pLine;
			}
		}
		
		return RECMA55_STATE_CONTINUE;
	}
	
	/* numeric compare: */
	
	tNum[0] = L_EvalNumExpr(pCtx, pParam->tpExpr[0]);
	
	if (pCtx->fAbort)
	{
		return RECMA55_STATE_FATAL_ERROR;
	}
	
	tNum[1] = L_EvalNumExpr(pCtx, pParam->tpExpr[1]);
	
	if (pCtx->fAbort)
	{
		return RECMA55_STATE_FATAL_ERROR;
	}
	
	switch (pParam->relation)
	{
	case RECMA55_RELATION_EQUAL:
		
		if (tNum[0] == tNum[1])
		{
			pCtx->pLineNext = pParam->ref.pLine;
		}
		break;
		
	case RECMA55_RELATION_NOT_EQUAL:
		
		if (tNum[0] != tNum[1])
		{
			pCtx->pLineNext = pParam->ref.pLine;
		}
		break;
		
	case RECMA55_RELATION_LESS_THAN:
		
		if (tNum[0] < tNum[1])
		{
			pCtx->pLineNext = pParam->ref.pLine;
		}
		break;
		
	case RECMA55_RELATION_NOT_GREATER:
		
		if (tNum[0] <= tNum[1])
		{
			pCtx->pLineNext = pParam->ref.pLine;
		}
		break;
		
	case RECMA55_RELATION_GREATER_THAN:
		
		if (tNum[0] > tNum[1])
		{
			pCtx->pLineNext = pParam->ref.pLine;
		}
		break;
		
	case RECMA55_RELATION_NOT_LESS:
		
		if (tNum[0] >= tNum[1])
		{
			pCtx->pLineNext = pParam->ref.pLine;
		}
		break;
		
	default:
		
		break;
	}
	
	return RECMA55_STATE_CONTINUE;
}


static RECMA55_BOOL RECMA55_PrepareStatementIF(struct RECMA55_PREPARE_CTX *pCtx, struct RECMA55_LINE *pLine)
{
	return L_LookupLineTree(pCtx->pLineTree, &pLine->param.pIf->ref, pLine->sLineNum);
}


static RECMA55_BOOL RECMA55_BuildStatementIF(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	const char *sErrMsg;
	struct RECMA55_PARAM_IF *pParam;
	RECMA55_EXPR_MODE mode;
	
	/* see ECMA-55 chapter 12: */
	
	pLine->param.pIf = L_MemAlloc(sizeof(struct RECMA55_PARAM_IF));
	
	if (pLine->param.pIf == NULL)
	{
		return 0;
	}
	
	pParam = pLine->param.pIf;
	
	pParam->tpExpr[0] = NULL;
	pParam->tpExpr[1] = NULL;
	
	pParam->tpExpr[0] = L_ParseExpr(pCtx, &pos, RECMA55_EXPR_MODE_ANY, NULL, pLine->sLineNum);
	
	if (pParam->tpExpr[0] == NULL)
	{
		return 0;
	}
	
	mode = ((pParam->tpExpr[0]->type == RECMA55_EXPR_TYPE_STR_CONST) || (pParam->tpExpr[0]->type == RECMA55_EXPR_TYPE_STR_VAR)) ? RECMA55_EXPR_MODE_STR : RECMA55_EXPR_MODE_NUM;
	
	if (pCtx->sLine[pos] == '=')
	{
		pParam->relation = RECMA55_RELATION_EQUAL;
	}
	else if ((pCtx->sLine[pos] == '<') && (pCtx->sLine[pos + 1] == '>'))
	{
		pParam->relation = RECMA55_RELATION_NOT_EQUAL;
		++pos;
	}
	else if (mode == RECMA55_EXPR_MODE_NUM)
	{
		switch (pCtx->sLine[pos])
		{
		case '<':
			
			if (pCtx->sLine[pos + 1] == '=')
			{
				pParam->relation = RECMA55_RELATION_NOT_GREATER;
				++pos;
			}
			else
			{
				pParam->relation = RECMA55_RELATION_LESS_THAN;
			}
			break;
			
		case '>':
			
			if (pCtx->sLine[pos + 1] == '=')
			{
				pParam->relation = RECMA55_RELATION_NOT_LESS;
				++pos;
			}
			else
			{
				pParam->relation = RECMA55_RELATION_GREATER_THAN;
			}
			break;
			
		default:
			
			L_SyntaxErrorMsg(pLine->sLineNum, "RELATION OPERATOR EXPECTED");
			return 0;
		}
	}
	else
	{
		L_SyntaxErrorMsg(pLine->sLineNum, "EQUALITY RELATION OPERATOR EXPECTED");
		return 0;
	}
	
	do
	{
		++pos;
	}
	while (pCtx->sLine[pos] == ' ');
	
	pParam->tpExpr[1] = L_ParseExpr(pCtx, &pos, mode, NULL, pLine->sLineNum);
	
	if (pParam->tpExpr[1] == NULL)
	{
		return 0;
	}
	
	if (!L_CheckKeyword(pCtx->sLine, &pos, "\'THEN\' EXPECTED", pLine->sLineNum))
	{
		return 0;
	}
	
	sErrMsg = L_ParseLineNumber(pCtx->sLine, &pos, pParam->ref.sLineNum, 0);
	
	if (sErrMsg != NULL)
	{
		L_SyntaxErrorMsg(pLine->sLineNum, sErrMsg);
		return 0;
	}
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static void RECMA55_ClearStatementIF(union RECMA55_PARAM *pParam)
{
	if (pParam->pIf != NULL)
	{
		L_DestroyExpr(pParam->pIf->tpExpr[0]);
		L_DestroyExpr(pParam->pIf->tpExpr[1]);
		L_MemFree(pParam->pIf);
	}
}


static RECMA55_STATE RECMA55_ExecStatementINPUT(struct RECMA55_RUN_CTX *pCtx)
{
	/* see ECMA-55 chapter 15: */
	
	if (!L_Input(pCtx, pCtx->pLineCur->param.pVarList, ""))
	{
		return RECMA55_STATE_FATAL_ERROR;
	}
	
	return RECMA55_STATE_CONTINUE;
}


static RECMA55_STATE RECMA55_ExecStatementLET(struct RECMA55_RUN_CTX *pCtx)
{
	struct RECMA55_PARAM_LET *pParam;
	
	pParam = pCtx->pLineCur->param.pLet;
	
	switch (pParam->tpExpr[0]->type)
	{
	case RECMA55_EXPR_TYPE_STR_CONST:
	case RECMA55_EXPR_TYPE_STR_VAR:
		
		strcpy(pParam->tpExpr[0]->value.str.sStr, pParam->tpExpr[1]->value.str.sStr);  /* length of constant string has been asserted on build */
		return RECMA55_STATE_CONTINUE;
		
	default:
		
		break;
	}
	
	if (!L_SetNumVar(pCtx, pParam->tpExpr[0], L_EvalNumExpr(pCtx, pParam->tpExpr[1])))
	{
		return RECMA55_STATE_FATAL_ERROR;
	}
	
	return RECMA55_STATE_CONTINUE;
}


static RECMA55_BOOL RECMA55_BuildStatementLET(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	struct RECMA55_PARAM_LET *pParam;
	
	/* see ECMA-55 chapter 11: */
	
	pLine->param.pLet = L_MemAlloc(sizeof (struct RECMA55_PARAM_LET));
	
	if (pLine->param.pLet == NULL)
	{
		return 0;
	}
	
	pParam = pLine->param.pLet;
	
	pParam->tpExpr[0] = NULL;
	pParam->tpExpr[1] = NULL;
	
	pParam->tpExpr[0] = L_ParseExpr(pCtx, &pos, RECMA55_EXPR_MODE_VAR, NULL, pLine->sLineNum);
	
	if (pParam->tpExpr[0] == NULL)
	{
		return 0;
	}
	
	if (pCtx->sLine[pos] != '=')
	{
		L_SyntaxErrorMsg(pLine->sLineNum, "\'=\' EXPECTED");
		return 0;
	}
	
	do
	{
		++pos;
	}
	while (pCtx->sLine[pos] == ' ');
	
	pParam->tpExpr[1] = L_ParseExpr(pCtx, &pos, (pParam->tpExpr[0]->type == RECMA55_EXPR_TYPE_STR_VAR) ? RECMA55_EXPR_MODE_STR : RECMA55_EXPR_MODE_NUM, NULL, pLine->sLineNum);
	
	if (pParam->tpExpr[1] == NULL)
	{
		return 0;
	}
	
	if ((pParam->tpExpr[1]->type == RECMA55_EXPR_TYPE_STR_CONST) && (pParam->tpExpr[1]->value.str.len > RECMA55_STR_VAR_LEN_MAX))
	{
		L_SyntaxErrorMsg(pLine->sLineNum, "STRING CONTAINS TOO MAY CHARACTERS");  /* see ECMA-55 chapter 11.5 */
		return 0;
	}
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static void RECMA55_ClearStatementLET(union RECMA55_PARAM *pParam)
{
	if (pParam->pLet != NULL)
	{
		L_DestroyExpr(pParam->pLet->tpExpr[0]);
		L_DestroyExpr(pParam->pLet->tpExpr[1]);
		L_MemFree(pParam->pLet);
	}
}


static RECMA55_STATE RECMA55_ExecStatementNEXT(struct RECMA55_RUN_CTX *pCtx)
{
	struct RECMA55_PARAM_FOR *pParam;
	
	/* see ECMA-55 chapter 13.4: */
	
	pParam = pCtx->pLineCur->param.ref.pLine->param.pFor;
	
	if (pParam->state.tNum[0] == RECMA55_NUM_INFINITY)
	{
		L_RunTimeErrorMsg(pCtx->pLineCur->sLineNum, "NEXT STATEMENT NOT ALLOWED");  /* program has transferred control into the for-body, see ECMA-55 chapter 13.4 */
		return RECMA55_STATE_FATAL_ERROR;
	}
	
	if (pCtx->fNoIncrement)
	{
		pCtx->fNoIncrement = 0;  /* no increment after jump from FOR statement */
	}
	else
	{
		*pParam->pNum += pParam->state.tNum[1];  /* increment */
	}
	
	if (pParam->state.tNum[1] >= 0.0)
	{
		/* stepping forward: */
		
		if (*pParam->pNum <= pParam->state.tNum[0])
		{
			pCtx->pLineNext = pCtx->pLineCur->param.ref.pLine->pNext;
		}
		else
		{
			pParam->state.tNum[0] = RECMA55_NUM_INFINITY;
		}
	}
	else
	{
		/* stepping backward: */
		
		if (*pParam->pNum >= pParam->state.tNum[0])
		{
			pCtx->pLineNext = pCtx->pLineCur->param.ref.pLine->pNext;
		}
		else
		{
			pParam->state.tNum[0] = RECMA55_NUM_INFINITY;
		}
	}
	
	return RECMA55_STATE_CONTINUE;
}


static RECMA55_BOOL RECMA55_BuildStatementNEXT(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	struct RECMA55_EXPR *pExpr;
	
	/* matching FOR statement must exist (see ECMA-55 chapters 5.2, 13.2 and 13.4): */
	
	if (pCtx->pPrepareCtx->pLineStack == NULL)
	{
		L_ProgramErrorMsg(pLine->sLineNum, "NEXT WITHOUT FOR");
		return 0;
	}
	
	pExpr = L_ParseNumVar(pCtx, &pos, NULL, NULL, pLine->sLineNum);
	
	if (pExpr == NULL)
	{
		return 0;
	}
	
	assert(pExpr->type == RECMA55_EXPR_TYPE_NUM_VAR);
	
	if (pExpr->value.pNum != pCtx->pPrepareCtx->pLineStack->param.pFor->pNum)
	{
		L_ProgramErrorMsg(pLine->sLineNum, "FOR/NEXT CONTROL VARIABLE MISMATCH");  /* see ECMA-55 chapter 13.4 */
		return 0;
	}
	
	pLine->param.ref.pLine = pCtx->pPrepareCtx->pLineStack;
	
	pLine->param.ref.pLine->param.pFor->pLine = pLine;
	
	pCtx->pPrepareCtx->pLineStack = pCtx->pPrepareCtx->pLineStack->param.pFor->state.pLinePrev;
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static RECMA55_STATE RECMA55_ExecStatementON(struct RECMA55_RUN_CTX *pCtx)
{
	struct RECMA55_PARAM_ON *pParam;
	RECMA55_NUM num;
	RECMA55_UINT index;
	
	/* see ECMA-55 chapter 12: */
	
	pParam = pCtx->pLineCur->param.pOn;
	
	num = L_EvalNumExpr(pCtx, pParam->pExpr);
	
	if (pCtx->fAbort)
	{
		return RECMA55_STATE_FATAL_ERROR;  /* errors have been handled already */
	}
	
	num += 0.5;  /* rounding */
	
	index = ((num >= 1.0) && (num != RECMA55_NUM_INFINITY) && (num != -RECMA55_NUM_INFINITY)) ? (RECMA55_UINT)num : 0;
	
	if ((index < 1) || (index > pParam->count))
	{
		L_RunTimeErrorMsg(pCtx->pLineCur->sLineNum, "ON EXPRESSION OUT OF BOUND");  /* see ECMA-55 chapter 12.5 */
		return RECMA55_STATE_FATAL_ERROR;
	}
	
	pCtx->pLineNext = pParam->tRef[index - 1].pLine;
	
	return RECMA55_STATE_CONTINUE;
}


static RECMA55_BOOL RECMA55_PrepareStatementON(struct RECMA55_PREPARE_CTX *pCtx, struct RECMA55_LINE *pLine)
{
	struct RECMA55_PARAM_ON *pParam;
	RECMA55_UINT index;
	
	pParam = pLine->param.pOn;
	
	for (index = 0; index < pParam->count; ++index)
	{
		if (!L_LookupLineTree(pCtx->pLineTree, &pParam->tRef[index], pLine->sLineNum))
		{
			return 0;
		}
	}
	
	return 1;
}


static RECMA55_BOOL RECMA55_BuildStatementON(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	const char *sErrMsg;
	const char *sLine;
	struct RECMA55_PARAM_ON *pParam;
	RECMA55_UINT index;
	
	/* see ECMA-55 chapter 12: */
	
	pLine->param.pOn = L_MemAlloc(sizeof (struct RECMA55_PARAM_ON));
	
	if (pLine->param.pOn == NULL)
	{
		return 0;
	}
	
	pParam = pLine->param.pOn;
	
	pParam->count = 1;
	pParam->tRef = NULL;
	
	pParam->pExpr = L_ParseExpr(pCtx, &pos, RECMA55_EXPR_MODE_NUM, NULL, pLine->sLineNum);
	
	if (pParam->pExpr == NULL)
	{
		return 0;
	}
	
	if (!L_CheckKeyword(pCtx->sLine, &pos, "\'GOTO\' EXPECTED", pLine->sLineNum))
	{
		return 0;
	}
	
	/* determine number of line numbers: */
	
	for (sLine = &pCtx->sLine[pos] ; *sLine != '\0'; ++sLine)
	{
		if (*sLine == ',')
		{
			++pParam->count;
		}
	}
	
	pParam->tRef = L_MemAlloc(pParam->count * sizeof (union RECMA55_LINE_REF));
	
	if (pParam->tRef == NULL)
	{
		return 0;
	}
	
	/* parse line numbers: */
	
	index = 0;
	
	--pos;  /* we start one character before */
	
	do
	{
		assert(index < pParam->count);
		
		do
		{
			++pos;
		}
		while (pCtx->sLine[pos] == ' ');
		
		sErrMsg = L_ParseLineNumber(pCtx->sLine, &pos, pParam->tRef[index].sLineNum, 1);
		
		if (sErrMsg != NULL)
		{
			L_SyntaxErrorMsg(pLine->sLineNum, sErrMsg);
			return 0;
		}
		
		++index;
	}
	while (pCtx->sLine[pos] == ',');
	
	assert(index == pParam->count);
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static void RECMA55_ClearStatementON(union RECMA55_PARAM *pParam)
{
	if (pParam->pOn != NULL)
	{
		L_DestroyExpr(pParam->pOn->pExpr);
		L_MemFree(pParam->pOn->tRef);
		L_MemFree(pParam->pOn);
	}
}


static RECMA55_BOOL RECMA55_BuildStatementOPTION(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	/* see ECMA-55 chapter 18: */
	
	if (pCtx->fNoOptionBase)
	{
		L_ProgramErrorMsg(pLine->sLineNum, "OPTION STATEMENT NO MORE ALLOWED");
		return 0;
	}
	
	pCtx->fNoOptionBase = 1;
	
	if (!L_CheckKeyword(pCtx->sLine, &pos, "\'BASE\' EXPECTED", pLine->sLineNum))
	{
		return 0;
	}
	
	if ((pCtx->sLine[pos] < '0') || (pCtx->sLine[pos] > '1'))
	{
		L_SyntaxErrorMsg(pLine->sLineNum, "OPTION BASE MUST BE EITHER 0 OR 1");
		return 0;
	}
	
	pCtx->pPrepareCtx->pRunCtx->var.base = (RECMA55_UINT)(pCtx->sLine[pos++] - '0');
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static RECMA55_STATE RECMA55_ExecStatementPRINT(struct RECMA55_RUN_CTX *pCtx)
{
	const struct RECMA55_PARAM_PRINT_ITEM *pItem;
	struct RECMA55_PARAM_PRINT *pParam;
	RECMA55_NUM num;
	RECMA55_UINT outputCol;
	
	pParam = pCtx->pLineCur->param.pPrint;
	
	if (!pCtx->fOutput)
	{
		printf("\n");
		pCtx->fOutput = 1;
	}
	
	if (pParam == NULL)
	{
		printf("\n");
		pCtx->outputCol = 0;
		
		return RECMA55_STATE_CONTINUE;
	}
	
	pCtx->outputCol = L_PrintComma(pCtx->outputCol, pParam->commaCount);
	
	for (pItem = pParam->pItemList;  pItem != NULL; pItem = pItem->pNext)
	{
		pCtx->outputCol = L_PrintComma(pCtx->outputCol, pItem->commaCount);
		
		switch (pItem->pExpr->type)
		{
		case RECMA55_EXPR_TYPE_STR_CONST:
			
			pCtx->outputCol = L_PreparePrint(pCtx->outputCol, pItem->pExpr->value.str.len);
			printf("%s", L_GetStr(pItem->pExpr->value.str.sStr));
			break;
			
		case RECMA55_EXPR_TYPE_STR_VAR:
			
			pCtx->outputCol = L_PreparePrint(pCtx->outputCol, (RECMA55_UINT)strlen(pItem->pExpr->value.str.sStr));
			printf("%s", pItem->pExpr->value.str.sStr);
			break;
			
		default:
			
			num = L_EvalNumExpr(pCtx, pItem->pExpr);
			
			if (pCtx->fAbort)
			{
				return RECMA55_STATE_FATAL_ERROR;
			}
			
			if (pItem->fTab)
			{
				if (num < 1.0)
				{
					outputCol = 0;  /* see ECMA-55 chapter 14.5 (outputCol is zero-based) */
				}
				else
				{
					num = (RECMA55_NUM)fmod(num - 0.5, RECMA55_PRINT_MARGIN);  /* see ECMA-55 chapter 14.4 (outputCol is zero-based) */
					
					if (num < RECMA55_PRINT_MARGIN)
					{
						outputCol = (RECMA55_UINT)num;
					}
					else
					{
						outputCol = 0;  /* may happen on infinity */
					}
				}
				
				if (outputCol >= pCtx->outputCol)  /* see ECMA-55 chapter 14.4 */
				{
					printf("\n");
					pCtx->outputCol = 0;
				}
				
				while (pCtx->outputCol < outputCol)
				{
					printf(" ");
					++pCtx->outputCol;
				}
			}
			else
			{
				pCtx->outputCol = L_PrintNum(pCtx->outputCol, num);
			}
		}
	}
	
	pCtx->outputCol = L_PrintComma(pCtx->outputCol, pParam->commaCount);
	
	if (pParam->fNewLine)
	{
		printf("\n");
		pCtx->outputCol = 0;
	}
	
	return RECMA55_STATE_CONTINUE;
}


static RECMA55_BOOL RECMA55_BuildStatementPRINT(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	struct RECMA55_PARAM_PRINT_ITEM **ppItem;
	struct RECMA55_PARAM_PRINT *pParam;
	
	/* see ECMA-55 chapter 14: */
	
	if (pCtx->sLine[pos] == '\0')
	{
		pLine->param.pPrint = NULL;
		return 1;
	}
	
	pLine->param.pPrint = L_MemAlloc(sizeof(struct RECMA55_PARAM_PRINT));
	
	if (pLine->param.pPrint == NULL)
	{
		return 0;
	}
	
	pParam = pLine->param.pPrint;
	
	pParam->pItemList = 0;
	pParam->commaCount = 0;
	pParam->fNewLine = 0;
	
	while ((pCtx->sLine[pos] == ',') || (pCtx->sLine[pos] == ';'))
	{
		if (pCtx->sLine[pos++] == ',')
		{
			++pParam->commaCount;
		}
		
		while (pCtx->sLine[pos] == ' ')
		{
			++pos;
		}
	}
	
	ppItem = &pParam->pItemList;
	
	while (pCtx->sLine[pos] != '\0')
	{
		*ppItem = L_MemAlloc(sizeof (struct RECMA55_PARAM_PRINT_ITEM));
		
		if (*ppItem == NULL)
		{
			return 0;
		}
		
		(*ppItem)->pNext = NULL;
		(*ppItem)->commaCount = pParam->commaCount;
		(*ppItem)->fTab = 0;
		(*ppItem)->pExpr = NULL;
		
		if ((pCtx->sLine[pos] == 'T') && (pCtx->sLine[pos + 1] == 'A') && (pCtx->sLine[pos + 2] == 'B') && !isalnum((int)pCtx->sLine[pos + 3]))
		{
			/* TAB-call (see ECMA-55 chapters 14.2): */
			
			(*ppItem)->fTab = 1;
			
			pos += 3;
			
			while (pCtx->sLine[pos] == ' ')
			{
				++pos;
			}
			
			if (pCtx->sLine[pos] != '(')
			{
				L_SyntaxErrorMsg(pLine->sLineNum, "\'(\' EXPECTED");
				return 0;
			}
			
			while (pCtx->sLine[pos] == ' ')
			{
				++pos;
			}
			
			(*ppItem)->pExpr = L_ParseExpr(pCtx, &pos, RECMA55_EXPR_MODE_NUM, NULL, pLine->sLineNum);
			
			if ((*ppItem)->pExpr == NULL)
			{
				return 0;
			}
			
			if (pCtx->sLine[pos] != ')')
			{
				L_SyntaxErrorMsg(pLine->sLineNum, "\')\' EXPECTED");
				return 0;
			}
			
			while (pCtx->sLine[pos] == ' ')
			{
				++pos;
			}
		}
		else
		{
			(*ppItem)->pExpr = L_ParseExpr(pCtx, &pos, RECMA55_EXPR_MODE_ANY, NULL, pLine->sLineNum);
			
			if ((*ppItem)->pExpr == NULL)
			{
				return 0;
			}
		}
		
		pParam->commaCount = 0;
		pParam->fNewLine = 1;
		
		while ((pCtx->sLine[pos] == ',') || (pCtx->sLine[pos] == ';'))
		{
			pParam->fNewLine = 0;
			
			if (pCtx->sLine[pos++] == ',')
			{
				++pParam->commaCount;
			}
			
			while (pCtx->sLine[pos] == ' ')
			{
				++pos;
			}
		}
		
		ppItem = &(*ppItem)->pNext;
	}
	
	return 1;
}


static void RECMA55_ClearStatementPRINT(union RECMA55_PARAM *pParam)
{
	struct RECMA55_PARAM_PRINT_ITEM *pItem;
	
	if (pParam->pPrint != NULL)
	{
		while (pParam->pPrint->pItemList != NULL)
		{
			pItem = pParam->pPrint->pItemList;
			pParam->pPrint->pItemList = pItem->pNext;
			L_DestroyExpr(pItem->pExpr);
			L_MemFree(pItem);
		}
		
		L_MemFree(pParam->pPrint);
	}
}


static RECMA55_STATE RECMA55_ExecStatementRANDOMIZE(struct RECMA55_RUN_CTX *pCtx)
{
	struct RECMA55_EXPR expr;
	struct RECMA55_PARAM_VAR var;
	time_t seed;
	
	seed = time(NULL);
	
	if (seed <= 0)
	{
		expr.type = RECMA55_EXPR_TYPE_NUM_VAR;
		expr.prio = RECMA55_EXPR_PRIO_PRIMARY;
		expr.value.pNum = &var.num;
		
		var.pNext = NULL;
		var.pExpr = &expr;
		
		if (!L_Input(pCtx, &var, "SEED"))
		{
			return RECMA55_STATE_FATAL_ERROR;
		}
		
		while (var.num < RAND_MAX)
		{
			var.num *= 2;
		}
		
		seed = (time_t)fmod(var.num, RAND_MAX);
	}
	
	srand((unsigned int)(seed % RAND_MAX));
	
	rand();  /* skip first pseudo-random value (ignore return value) */
	
	return RECMA55_STATE_CONTINUE;
}


static RECMA55_BOOL RECMA55_BuildStatementRANDOMIZE(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	/* see ECMA-55 chapter 20: */
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static RECMA55_BOOL RECMA55_BuildStatementREM(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	/* see ECMA-55 chapter 19: */
	
	return 1;
}


static RECMA55_STATE RECMA55_ExecStatementREAD(struct RECMA55_RUN_CTX *pCtx)
{
	const char *sErrMsg;
	
	/* see ECMA-55 chapter 16: */
	
	sErrMsg = L_ParseVarList(pCtx, pCtx->pLineCur->param.pVarList, pCtx->pDataCur, 0);
	
	if (sErrMsg != NULL)
	{
		L_RunTimeErrorMsg(pCtx->pLineCur->sLineNum, sErrMsg);
		return RECMA55_STATE_FATAL_ERROR;
	}
	
	if (!L_AssignVarList(pCtx, pCtx->pLineCur->param.pVarList, (const struct RECMA55_DATA**)&pCtx->pDataCur))
	{
		return RECMA55_STATE_FATAL_ERROR;
	}
	
	return RECMA55_STATE_CONTINUE;
}


static RECMA55_STATE RECMA55_ExecStatementRESTORE(struct RECMA55_RUN_CTX *pCtx)
{
	/* see ECMA-55 chapter 16: */
	
	pCtx->pDataCur = pCtx->pDataList;
	
	return RECMA55_STATE_CONTINUE;
}


static RECMA55_BOOL RECMA55_BuildStatementRESTORE(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	/* see ECMA-55 chapter 16: */
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static RECMA55_STATE RECMA55_ExecStatementRETURN(struct RECMA55_RUN_CTX *pCtx)
{
	struct RECMA55_RETURN_STACK *pStack;
	
	pStack = pCtx->pStack;
	
	if (pStack == NULL)
	{
		L_RunTimeErrorMsg(pCtx->pLineCur->sLineNum, "RETURN WITHOUT GOSUB");  /* see ECMA-55 chapter 12.5 */
		return RECMA55_STATE_FATAL_ERROR;
	}
	
	pCtx->pLineNext = pStack->pLine;
	pCtx->pStack = pStack->pPrev;
	
	L_MemFree(pStack);
	
	return RECMA55_STATE_CONTINUE;
}


static RECMA55_BOOL RECMA55_BuildStatementRETURN(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	/* see ECMA-55 chapter 12: */
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static RECMA55_STATE RECMA55_ExecStatementSTOP(struct RECMA55_RUN_CTX *pCtx)
{
	return RECMA55_STATE_HALT;
}


static RECMA55_BOOL RECMA55_BuildStatementSTOP(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	/* see ECMA-55 chapter 12: */
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static RECMA55_BOOL RECMA55_BuildStatementVAR(struct RECMA55_LOAD_CTX *pCtx, struct RECMA55_LINE *pLine, RECMA55_UINT pos)
{
	struct RECMA55_PARAM_VAR **ppVar;
	
	/* see ECMA-55 chapters 15 and 16: */
	
	pLine->param.pVarList = NULL;
	ppVar = &pLine->param.pVarList;
	
	--pos;
	
	do
	{
		do
		{
			++pos;
		}
		while (pCtx->sLine[pos] == ' ');
		
		*ppVar = L_MemAlloc(sizeof (struct RECMA55_PARAM_VAR));
		
		if (*ppVar == NULL)
		{
			return 0;
		}
		
		(*ppVar)->pNext = NULL;
		(*ppVar)->pExpr = L_ParseExpr(pCtx, &pos, RECMA55_EXPR_MODE_VAR, NULL, pLine->sLineNum);
		
		if ((*ppVar)->pExpr == NULL)
		{
			return 0;
		}
		
		ppVar = &(*ppVar)->pNext;
	}
	while (pCtx->sLine[pos] == ',');
	
	return L_CheckEndOfLine(&pCtx->sLine[pos], pLine->sLineNum);
}


static void RECMA55_ClearStatementVAR(union RECMA55_PARAM *pParam)
{
	struct RECMA55_PARAM_VAR *pVar;
	
	while (pParam->pVarList != NULL)
	{
		pVar = pParam->pVarList;
		pParam->pVarList = pVar->pNext;
		L_DestroyExpr(pVar->pExpr);
		L_MemFree(pVar);
	}
}


static const struct RECMA55_STATEMENT ltStatement[] = {
	/* array must be sorted by sName */
	{
		"DATA", 0,
		{ NULL, NULL, RECMA55_BuildStatementDATA, NULL }
	},
	{
		"DEF", 0,
		{ NULL, NULL, RECMA55_BuildStatementDEF, NULL }
	},
	{
		"DIM", 0,
		{ NULL, NULL, RECMA55_BuildStatementDIM, NULL }
	},
	{
		"END", 0,
		{ NULL, NULL, RECMA55_BuildStatementEND, NULL }
	},
	{
		"FOR", 0,
		{ RECMA55_ExecStatementFOR, RECMA55_PrepareStatementFOR, RECMA55_BuildStatementFOR, RECMA55_ClearStatementFOR }
	},
	{
		"GOSUB", 0,
		{ RECMA55_ExecStatementGOSUB, RECMA55_PrepareStatementGO, RECMA55_BuildStatementGO, NULL }
	},
	{
		"GOTO", 0,
		{ RECMA55_ExecStatementGOTO, RECMA55_PrepareStatementGO, RECMA55_BuildStatementGO, NULL }
	},
	{
		"IF", 0,
		{ RECMA55_ExecStatementIF, RECMA55_PrepareStatementIF, RECMA55_BuildStatementIF, RECMA55_ClearStatementIF }
	},
	{
		"INPUT", 0,
		{ RECMA55_ExecStatementINPUT, NULL, RECMA55_BuildStatementVAR, RECMA55_ClearStatementVAR }
	},
	{
		"LET", 0,
		{ RECMA55_ExecStatementLET, NULL, RECMA55_BuildStatementLET, RECMA55_ClearStatementLET }
	},
	{
		"NEXT", 0,
		{ RECMA55_ExecStatementNEXT, NULL, RECMA55_BuildStatementNEXT, NULL }
	},
	{
		"ON", 0,
		{ RECMA55_ExecStatementON, RECMA55_PrepareStatementON, RECMA55_BuildStatementON, RECMA55_ClearStatementON }
	},
	{
		"OPTION", 0,
		{ NULL, NULL, RECMA55_BuildStatementOPTION, NULL }
	},
	{
		"PRINT", 0,
		{ RECMA55_ExecStatementPRINT, NULL, RECMA55_BuildStatementPRINT, RECMA55_ClearStatementPRINT }
	},
	{
		"RANDOMIZE", 1,
		{ RECMA55_ExecStatementRANDOMIZE, NULL, RECMA55_BuildStatementRANDOMIZE, NULL }
	},
	{
		"READ", 0,
		{ RECMA55_ExecStatementREAD, NULL, RECMA55_BuildStatementVAR, RECMA55_ClearStatementVAR }
	},
	{
		"REM", 0,
		{ NULL, NULL, RECMA55_BuildStatementREM, NULL }
	},
	{
		"RESTORE", 0,
		{ RECMA55_ExecStatementRESTORE, NULL, RECMA55_BuildStatementRESTORE, NULL }
	},
	{
		"RETURN", 0,
		{ RECMA55_ExecStatementRETURN, NULL, RECMA55_BuildStatementRETURN, NULL }
	},
	{
		"STOP", 0,
		{ RECMA55_ExecStatementSTOP, NULL, RECMA55_BuildStatementSTOP, NULL }
	}
};


/*****************************************************************************************
 *
 *  I N T E R N A L   F U N C T I O N S
 *
 *****************************************************************************************/


static void L_Init(struct RECMA55_RUN_CTX *pCtx)
{
	RECMA55_UINT index;
	RECMA55_UINT digit;
	
	pCtx->fBatchMode = 0;
	pCtx->fFullChar = 0;
	pCtx->pLineList = NULL;
	pCtx->pLineCur = NULL;
	pCtx->pLineNext = NULL;
	pCtx->pDataList = NULL;
	pCtx->pDataCur = NULL;
	pCtx->inputRow = 0;
	pCtx->outputCol = 0;
	pCtx->fOutput = 0;
	pCtx->fNoIncrement = 0;
	pCtx->fAbort = 0;
	pCtx->var.base = 0;
	
	for (index = 0; index < 26; ++index)
	{
		pCtx->tUserFunc[index].pExpr = NULL;
		pCtx->tUserFunc[index].sArgVar[0] = '\0';
	}
	
	for (index = 0; index < 26; ++index)
	{
		pCtx->var.tVariant[index].tDim[0] = 0;
		pCtx->var.tVariant[index].tDim[1] = 0;
		pCtx->var.tVariant[index].value.num = 0.0;
		
		for (digit = 0; digit < 10; ++digit)
		{
			pCtx->var.ttNum[index][digit] = 0.0;
		}
		
		pCtx->var.tsStr[index][0] = '\0';
	}
	
	pCtx->pStack = NULL;
}


static void L_Clear(struct RECMA55_RUN_CTX *pCtx)
{
	struct RECMA55_LINE *pLine;
	struct RECMA55_RETURN_STACK *pStack;
	RECMA55_UINT index;
	
	while (pCtx->pStack != NULL)
	{
		pStack = pCtx->pStack;
		pCtx->pStack = pStack->pPrev;
		L_MemFree(pStack);
	}
	
	for (index = 0; index < 26; ++index)
	{
		if (pCtx->var.tVariant[index].tDim[0])
		{
			L_MemFree(pCtx->var.tVariant[index].value.tNum);
		}
	}
	
	for (index = 0; index < 26; ++index)
	{
		L_DestroyExpr(pCtx->tUserFunc[index].pExpr);
	}
	
	while (pCtx->pLineList != NULL)
	{
		pLine = pCtx->pLineList;
		pCtx->pLineList = pLine->pNext;
		
		if (pLine->pStatement != NULL)
		{
			if (pLine->pStatement->handler.Clear != NULL)
			{
				pLine->pStatement->handler.Clear(&pLine->param);
			}
		}
		
		L_MemFree(pLine);
	}
}


static const struct RECMA55_STATEMENT* L_LookupStatement(const char *sLine, RECMA55_UINT *pPos)
{
	const char *sName;
	RECMA55_UINT pos;
	size_t l;
	size_t r;
	size_t m;
	
	l = 1;
	r = sizeof ltStatement / sizeof ltStatement[0];
	
	while (l <= r)
	{
		m = (l + r) / 2;
		
		sName = ltStatement[m - 1].sName;
		pos = *pPos;
		
		if ((sName[0] == 'G') && (sLine[pos] == 'G') && (sName[1] == 'O') && (sLine[pos + 1] == 'O'))
		{
			/* allow arbitrary number of spaces between 'GO' and the subsequent keyword (e.g. 'TO' or 'SUB'), see ECMA-55 chapter 12.2: */
			
			sName += 2;
			pos += 2;
			
			while (sLine[pos] == ' ')
			{
				++pos;
			}
		}
		
		while ((*sName != '\0') && (*sName == sLine[pos]))
		{
			++sName;
			++pos;
		}
		
		if ((*sName == '\0') && ((sLine[pos] == '\0') || (sLine[pos] == ' ')))
		{
			while (sLine[pos] == ' ')
			{
				++pos;
			}
			
			*pPos = pos;
			
			return &ltStatement[m - 1];
		}
		
		if (*sName < sLine[pos])
		{
			l = m + 1;
		}
		else
		{
			r = m - 1;
		}
	}
	
	return NULL;
}


static RECMA55_BOOL L_LoadLine(struct RECMA55_LOAD_CTX *pCtx)
{
	const char *sErrMsg;
	struct RECMA55_LINE_NODE *pNode;
	RECMA55_UINT pos;
	
	pos = 0;
	
	/* create line object (becomes tail of pCtx->pPrepareCtx->pRunCtx->pLineList and does thus not need to be destroyed here on error): */
	
	*pCtx->ppLine = L_MemAlloc(sizeof (struct RECMA55_LINE));
	
	if (*pCtx->ppLine == NULL)
	{
		return 0;
	}
	
	(*pCtx->ppLine)->pNext = NULL;
	(*pCtx->ppLine)->pStatement = NULL;
	
	/* parse and verify line number: */
	
	sErrMsg = L_ParseLineNumber(pCtx->sLine, &pos, (*pCtx->ppLine)->sLineNum, 0);
	
	if (sErrMsg != NULL)
	{
		L_LoadErrorMsg(pCtx->fileLineNum, sErrMsg);
		return 0;
	}
	
	if (L_LineNumCmp(pCtx->sLineNum, (*pCtx->ppLine)->sLineNum) >= 0)
	{
		L_LoadErrorMsg(pCtx->fileLineNum, "NON-ASCENDING LINE NUMBER");  /* see ECMA-55 chapter 5.4 */
		return 0;
	}
	
	pCtx->sLineNum = (*pCtx->ppLine)->sLineNum;
	
	/* create line node for fast line number lookup: */
	
	pNode = L_MemAlloc(sizeof (struct RECMA55_LINE_NODE));
	
	if (pNode == NULL)
	{
		return 0;
	}
	
	pNode->pLine = *pCtx->ppLine;
	pNode->pLeft = NULL;
	pNode->pRight = NULL;
	pNode->depth = 0;
	
	L_InsertLineTree(&pCtx->pPrepareCtx->pLineTree, pNode);
	
	/* build statement: */
	
	pCtx->ppLine = &pNode->pLine->pNext;
	
	if (pCtx->sLine[pos] == '\0')
	{
		L_SyntaxErrorMsg(pNode->pLine->sLineNum, "STATEMENT EXPECTED");
		return 0;
	}
	
	assert(pCtx->sLine[pos - 1] == ' ');
	
	pNode->pLine->pStatement = L_LookupStatement(pCtx->sLine, &pos);
	
	if (pNode->pLine->pStatement == NULL)
	{
		L_SyntaxErrorMsg(pNode->pLine->sLineNum, "INVALID OR UNKNOWN STATEMENT");
		return 0;
	}
	
	if (pNode->pLine->pStatement->fPRNG)
	{
		pCtx->fPRNG = 1;
	}
	
	return pNode->pLine->pStatement->handler.Build(pCtx, pNode->pLine, pos);
}


static RECMA55_BOOL L_Load(struct RECMA55_PREPARE_CTX *pPrepareCtx, FILE *pFile, RECMA55_BOOL fSecurityLow)
{
	struct RECMA55_LOAD_CTX ctx;
	RECMA55_UINT len;
	RECMA55_BOOL fString;
	RECMA55_BOOL fEOF;
	int prev;
	int c;
	
	ctx.pPrepareCtx = pPrepareCtx;
	ctx.fileLineNum = 1;
	ctx.fPRNG = 0;
	ctx.fNoOptionBase = 0;
	ctx.fEnd = 0;
	ctx.sLineNum = NULL;
	ctx.ppLine = &pPrepareCtx->pRunCtx->pLineList;
	ctx.ppDataTail = &pPrepareCtx->pRunCtx->pDataList;
	
	len = 0;
	fString = 0;
	fEOF = 0;
	c = '\0';
	
	do
	{
		prev = c;
		
		c = fgetc(pFile);
		
		switch (c)
		{
		case EOF:
			
			if (ferror(pFile))
			{
				L_GenericErrorMsg("READ FROM INPUT FILE FAILED");
				return 0;
			}
			
			fEOF = 1;
			
			/* no break */
			
		case '\n':
			
			if (prev == '\r')
			{
				break;  /* ignore LF or EOF after CR */
			}
			
			/* no break */
			
		case '\r':
			
			if (len)
			{
				ctx.sLine[len] = '\0';
				
				if (!L_LoadLine(&ctx))
				{
					return 0;
				}
				
				len = 0;
			}
			else if (!pPrepareCtx->pRunCtx->fFullChar && !fEOF && !ctx.fEnd)
			{
				L_LoadErrorMsg(ctx.fileLineNum, "LINE IS EMPTY");
				return 0;
			}
			
			++ctx.fileLineNum;
			break;
			
		default:
			
			if (!L_TranslateInputChar(&c, fString, pPrepareCtx->pRunCtx->fFullChar))
			{
				L_LoadErrorMsg(ctx.fileLineNum, "INVALID CHARACTER");
				return 0;
			}
			
			if (c != '\0')
			{
				if (ctx.fEnd)
				{
					L_LoadErrorMsg(ctx.fileLineNum, "CHARACTER AFTER END STATEMENT");
					return 0;
				}
				
				if (len >= RECMA55_LINE_LEN_MAX)
				{
					L_LoadErrorMsg(ctx.fileLineNum, "LINE EXCEEDS BOUNDARY");
					return 0;
				}
				
				if (c == '\"')
				{
					fString = !fString;
				}
				
				ctx.sLine[len++] = c;
			}
		}
	}
	while (!fEOF);
	
	if (ctx.fileLineNum <= 1)
	{
		L_GenericErrorMsg("INPUT FILE IS EMPTY");
		return 0;
	}
	
	if (!ctx.fEnd)
	{
		L_GenericErrorMsg("END STATEMENT IS MISSING");
		return 0;
	}
	
	if (ctx.fPRNG && !fSecurityLow)
	{
		fprintf(stderr, "\nSECURITY ALERT:\n\n");
		fprintf(stderr, "This program uses the insecure built-in pseudo-random number generator\n");
		fprintf(stderr, "(function RND and/or statement RANDOMIZE). You may use the command line\n");
		fprintf(stderr, "option \'-SECURITY=LOW\' to run the program anyway but do not rely on the\n");
		fprintf(stderr, "randomness of the output and consider that the generated random numbers\n");
		fprintf(stderr, "may reveal the system time.\n");
		
		return 0;
	}
	
	return 1;
}


static RECMA55_BOOL L_Prepare(struct RECMA55_RUN_CTX *pRunCtx, const char *sPath, RECMA55_BOOL fSecurityLow)
{
	struct RECMA55_PREPARE_CTX ctx;
	struct RECMA55_LINE *pLine;
	FILE *pFile;
	
	/* initialize context: */
	
	ctx.pRunCtx = pRunCtx;
	ctx.pLineTree = NULL;
	ctx.pLineStack = NULL;
	
	/* input file: */
	
	pFile = fopen(sPath, "rt");
	
	if (pFile == NULL)
	{
		L_GenericErrorMsg("CANNOT OPEN INPUT FILE");
		return 0;
	}
	
	if (!L_Load(&ctx, pFile, fSecurityLow))
	{
		L_ClearLineTree(ctx.pLineTree);
		fclose(pFile);
		return 0;
	}
	
	if (fclose(pFile))
	{
		L_GenericErrorMsg("CLOSING INPUT FILE FAILED");
		L_ClearLineTree(ctx.pLineTree);
		return 0;
	}
	
	/* prepare program: */
	
	for (pLine = ctx.pRunCtx->pLineList; pLine != NULL; pLine = pLine->pNext)
	{
		assert(pLine->pStatement != NULL);
		
		if (pLine->pStatement->handler.Prepare != NULL)
		{
			if (!pLine->pStatement->handler.Prepare(&ctx, pLine))
			{
				L_ClearLineTree(ctx.pLineTree);
				return 0;
			}
		}
	}
	
	L_ClearLineTree(ctx.pLineTree);
	
	return 1;
}


static RECMA55_BOOL L_Run(struct RECMA55_RUN_CTX *pCtx)
{
	pCtx->pDataCur = pCtx->pDataList;
	pCtx->pLineCur = pCtx->pLineList;
	
	while (pCtx->pLineCur != NULL)
	{
		if (pCtx->pLineCur->pStatement->handler.Exec == NULL)
		{
			pCtx->pLineCur = pCtx->pLineCur->pNext;
		}
		else
		{
			pCtx->pLineNext = pCtx->pLineCur->pNext;
			
			switch (pCtx->pLineCur->pStatement->handler.Exec(pCtx))
			{
			case RECMA55_STATE_CONTINUE:
				
				pCtx->pLineCur = pCtx->pLineNext;
				break;
				
			case RECMA55_STATE_HALT:
				
				pCtx->pLineCur = NULL;
				break;
				
			default:
				
				return 0;
			}
		}
	}
	
	return 1;
}


/*****************************************************************************************
 *
 *  P R O G R A M   E N T R Y   P O I N T
 *
 *****************************************************************************************/


int main(int argc, char *argv[])
{
	struct RECMA55_RUN_CTX ctx;
	RECMA55_BOOL fSecurityLow;
	RECMA55_BOOL fNoBanner;
	RECMA55_BOOL fLicense;
	RECMA55_BOOL fNoArg;
	int exitCode;
	int argi;
	
	L_Init(&ctx);
	
	fSecurityLow = 0;
	fNoBanner = 0;
	fLicense = 0;
	fNoArg = 0;
	exitCode = EXIT_FAILURE;
	argi = 1;
	
	while ((argi < argc) && !fNoArg)
	{
		if (L_StrCmp(argv[argi], "-SECURITY=LOW") || L_StrCmp(argv[argi], "-SECURITY:LOW"))
		{
			fSecurityLow = 1;
		}
		else if (L_StrCmp(argv[argi], "-FULLCHAR"))
		{
			ctx.fFullChar = 1;
		}
		else if (L_StrCmp(argv[argi], "-BATCH"))
		{
			ctx.fBatchMode = 1;
		}
		else if (L_StrCmp(argv[argi], "-NOBANNER"))
		{
			fNoBanner = 1;
			ctx.fOutput = 1;
		}
		else if (L_StrCmp(argv[argi], "-LICENSE"))
		{
			fLicense = 1;
		}
		else
		{
			fNoArg = 1;
		}
		
		++argi;
	}
	
	if (!fNoBanner)
	{
		printf("RECMA55 - Retro ECMA-55-compliant Minimal BASIC Interpreter v%s\n", RECMA55_VERSION);
	}
	
	if (fLicense)
	{
		printf("\nMIT License\n\n");
	}
	
	if (!fNoBanner || fLicense)
	{
		printf("%s\n", RECMA55_COPYRIGHT);
	}

	if (fLicense)
	{
		printf("\nPermission is hereby granted, free of charge, to any person obtaining a copy\n");
		printf("of this software and associated documentation files (the \"Software\"), to deal\n");
		printf("in the Software without restriction, including without limitation the rights\n");
		printf("to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n");
		printf("copies of the Software, and to permit persons to whom the Software is\n");
		printf("furnished to do so, subject to the following conditions:\n\n");
		printf("The above copyright notice and this permission notice shall be included in all\n");
		printf("copies or substantial portions of the Software.\n\n");
		printf("THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n");
		printf("IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n");
		printf("FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n");
		printf("AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n");
		printf("LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n");
		printf("OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n");
		printf("SOFTWARE.\n\n");
		printf("Source: <http://source.retro-c.net/comp.stdc.recma55>\n");
	}
	
	if ((argc <= 1) || (argi != argc))
	{
		if (fNoArg)
		{
			L_GenericErrorMsg("Invalid argument");
		}
		
		printf("\nUsage:\n\n");
		printf("  RECMA55 [ <option> ]* [ <input-file> ]\n");
		printf("\nOptions:\n\n");
		printf("  -SECURITY=LOW  Allow usage of insecure statements and functions\n");
		printf("  -FULLCHAR      Allow usage of full character set\n");
		printf("  -BATCH         Batch processing (non-interactive mode)\n");
		printf("  -NOBANNER      Inhibit output of the banner at program start\n");
		printf("  -LICENSE       Output license text\n");
		printf("\nArguments:\n\n");
		printf("  <input-file>   ECMA-55 compliant Minimal BASIC program\n");
	}
	else if (fNoArg)
	{
		if (L_Prepare(&ctx, argv[argc - 1], fSecurityLow))
		{
			if (L_Run(&ctx))
			{
				exitCode = EXIT_SUCCESS;
			}
		}
		
		L_Clear(&ctx);
	}
	
	return exitCode;
}
