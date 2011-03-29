//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

#pragma pack(push, 1)

typedef int	func_t;
typedef int	string_t;

enum etype_t
{
	ev_void,
	ev_string,
	ev_float,
	ev_vector,
	ev_entity,
	ev_field,
	ev_function,
	ev_pointer
};

#define	OFS_NULL		0
#define	OFS_RETURN		1
#define	OFS_PARM0		4		// leave 3 ofs for each parm to hold vectors
#define	OFS_PARM1		7
#define	OFS_PARM2		10
#define	OFS_PARM3		13
#define	OFS_PARM4		16
#define	OFS_PARM5		19
#define	OFS_PARM6		22
#define	OFS_PARM7		25
#define	RESERVED_OFS	28

enum
{
	OP_DONE,
	OP_MUL_F,
	OP_MUL_V,
	OP_MUL_FV,
	OP_MUL_VF,
	OP_DIV_F,
	OP_ADD_F,
	OP_ADD_V,
	OP_SUB_F,
	OP_SUB_V,

	OP_EQ_F,
	OP_EQ_V,
	OP_EQ_S,
	OP_EQ_E,
	OP_EQ_FNC,

	OP_NE_F,
	OP_NE_V,
	OP_NE_S,
	OP_NE_E,
	OP_NE_FNC,

	OP_LE,
	OP_GE,
	OP_LT,
	OP_GT,

	OP_LOAD_F,
	OP_LOAD_V,
	OP_LOAD_S,
	OP_LOAD_ENT,
	OP_LOAD_FLD,
	OP_LOAD_FNC,

	OP_ADDRESS,

	OP_STORE_F,
	OP_STORE_V,
	OP_STORE_S,
	OP_STORE_ENT,
	OP_STORE_FLD,
	OP_STORE_FNC,

	OP_STOREP_F,
	OP_STOREP_V,
	OP_STOREP_S,
	OP_STOREP_ENT,
	OP_STOREP_FLD,
	OP_STOREP_FNC,

	OP_RETURN,
	OP_NOT_F,
	OP_NOT_V,
	OP_NOT_S,
	OP_NOT_ENT,
	OP_NOT_FNC,
	OP_IF,
	OP_IFNOT,
	OP_CALL0,
	OP_CALL1,
	OP_CALL2,
	OP_CALL3,
	OP_CALL4,
	OP_CALL5,
	OP_CALL6,
	OP_CALL7,
	OP_CALL8,
	OP_STATE,
	OP_GOTO,
	OP_AND,
	OP_OR,

	OP_BITAND,
	OP_BITOR,

	//	Start of Hexen 2 added instructions.
	OP_MULSTORE_F,
	OP_MULSTORE_V,
	OP_MULSTOREP_F,
	OP_MULSTOREP_V,

	OP_DIVSTORE_F,
	OP_DIVSTOREP_F,

	OP_ADDSTORE_F,
	OP_ADDSTORE_V,
	OP_ADDSTOREP_F,
	OP_ADDSTOREP_V,

	OP_SUBSTORE_F,
	OP_SUBSTORE_V,
	OP_SUBSTOREP_F,
	OP_SUBSTOREP_V,

	OP_FETCH_GBL_F,
	OP_FETCH_GBL_V,
	OP_FETCH_GBL_S,
	OP_FETCH_GBL_E,
	OP_FETCH_GBL_FNC,

	OP_CSTATE,
	OP_CWSTATE,

	OP_THINKTIME,

	OP_BITSET,
	OP_BITSETP,
	OP_BITCLR,
	OP_BITCLRP,

	OP_RAND0,
	OP_RAND1,
	OP_RAND2,
	OP_RANDV0,
	OP_RANDV1,
	OP_RANDV2,

	OP_SWITCH_F,
	OP_SWITCH_V,
	OP_SWITCH_S,
	OP_SWITCH_E,
	OP_SWITCH_FNC,

	OP_CASE,
	OP_CASERANGE
};

struct dstatement_t
{
	quint16		op;
	qint16		a;
	qint16		b;
	qint16		c;
};

struct ddef_t
{
	quint16		type;		// if DEF_SAVEGLOBGAL bit is set
							// the variable needs to be saved in savegames
	quint16		ofs;
	qint32		s_name;
};

#define DEF_SAVEGLOBAL	(1 << 15)

#define MAX_PARMS		8

struct dfunction_t
{
	qint32		first_statement;	// negative numbers are builtins
	qint32		parm_start;
	qint32		locals;				// total ints of parms + locals
	
	qint32		profile;		// runtime
	
	qint32		s_name;
	qint32		s_file;			// source file defined in
	
	qint32		numparms;
	quint8		parm_size[MAX_PARMS];
};


#define PROG_VERSION	6

struct dprograms_t
{
	qint32		version;
	qint32		crc;			// check of header file

	qint32		ofs_statements;
	qint32		numstatements;	// statement 0 is an error

	qint32		ofs_globaldefs;
	qint32		numglobaldefs;

	qint32		ofs_fielddefs;
	qint32		numfielddefs;

	qint32		ofs_functions;
	qint32		numfunctions;	// function 0 is an empty

	qint32		ofs_strings;
	qint32		numstrings;		// first string is a null string

	qint32		ofs_globals;
	qint32		numglobals;

	qint32		entityfields;
};

#pragma pack(pop)
