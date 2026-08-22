/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_PARSER_H_INCLUDED
# define YY_YY_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 316,                 /* "invalid token"  */
    AND_SY = 257,                  /* AND_SY  */
    ASM_SY = 258,                  /* ASM_SY  */
    ASSIGNMINUS_SY = 259,          /* ASSIGNMINUS_SY  */
    ASSIGNMOD_SY = 260,            /* ASSIGNMOD_SY  */
    ASSIGNPLUS_SY = 261,           /* ASSIGNPLUS_SY  */
    ASSIGNSLASH_SY = 262,          /* ASSIGNSLASH_SY  */
    ASSIGNSTAR_SY = 263,           /* ASSIGNSTAR_SY  */
    BOOLEAN_SY = 264,              /* BOOLEAN_SY  */
    BREAK_SY = 265,                /* BREAK_SY  */
    CASE_SY = 266,                 /* CASE_SY  */
    CFLOATCONST_SY = 267,          /* CFLOATCONST_SY  */
    COLONCOLON_SY = 268,           /* COLONCOLON_SY  */
    CONST_SY = 269,                /* CONST_SY  */
    CONTINUE_SY = 270,             /* CONTINUE_SY  */
    DEFAULT_SY = 271,              /* DEFAULT_SY  */
    DISCARD_SY = 272,              /* DISCARD_SY  */
    DO_SY = 273,                   /* DO_SY  */
    EQ_SY = 274,                   /* EQ_SY  */
    ELSE_SY = 275,                 /* ELSE_SY  */
    ERROR_SY = 276,                /* ERROR_SY  */
    EXTERN_SY = 277,               /* EXTERN_SY  */
    FLOAT_SY = 278,                /* FLOAT_SY  */
    FLOATCONST_SY = 279,           /* FLOATCONST_SY  */
    FLOATHCONST_SY = 280,          /* FLOATHCONST_SY  */
    FLOATXCONST_SY = 281,          /* FLOATXCONST_SY  */
    FOR_SY = 282,                  /* FOR_SY  */
    GE_SY = 283,                   /* GE_SY  */
    GG_SY = 284,                   /* GG_SY  */
    GOTO_SY = 285,                 /* GOTO_SY  */
    IDENT_SY = 286,                /* IDENT_SY  */
    IF_SY = 287,                   /* IF_SY  */
    IN_SY = 288,                   /* IN_SY  */
    INLINE_SY = 289,               /* INLINE_SY  */
    INOUT_SY = 290,                /* INOUT_SY  */
    INT_SY = 291,                  /* INT_SY  */
    INTCONST_SY = 292,             /* INTCONST_SY  */
    INTERNAL_SY = 293,             /* INTERNAL_SY  */
    LE_SY = 294,                   /* LE_SY  */
    LL_SY = 295,                   /* LL_SY  */
    MINUSMINUS_SY = 296,           /* MINUSMINUS_SY  */
    NE_SY = 297,                   /* NE_SY  */
    OR_SY = 298,                   /* OR_SY  */
    OUT_SY = 299,                  /* OUT_SY  */
    PACKED_SY = 300,               /* PACKED_SY  */
    PLUSPLUS_SY = 301,             /* PLUSPLUS_SY  */
    RETURN_SY = 302,               /* RETURN_SY  */
    STATIC_SY = 303,               /* STATIC_SY  */
    STRCONST_SY = 304,             /* STRCONST_SY  */
    STRUCT_SY = 305,               /* STRUCT_SY  */
    SWITCH_SY = 306,               /* SWITCH_SY  */
    TEXOBJ_SY = 307,               /* TEXOBJ_SY  */
    THIS_SY = 308,                 /* THIS_SY  */
    TYPEDEF_SY = 309,              /* TYPEDEF_SY  */
    TYPEIDENT_SY = 310,            /* TYPEIDENT_SY  */
    UNIFORM_SY = 311,              /* UNIFORM_SY  */
    VARYING_SY = 312,              /* VARYING_SY  */
    VOID_SY = 313,                 /* VOID_SY  */
    WHILE_SY = 314,                /* WHILE_SY  */
    FIRST_USER_TOKEN_SY = 315      /* FIRST_USER_TOKEN_SY  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 56 "parser.y"

    int    sc_token;
    int    sc_int;
    float  sc_fval;
    int    sc_ident;
    spec   sc_specifiers;
    dtype  sc_type;
    Type   *sc_ptype;
    decl   *sc_decl;
    expr   *sc_expr;
    symb   *sc_symb;
    stmt   *sc_stmt;
    /* Dummy palce holder: */
    int    dummy;

#line 141 "parser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_PARSER_H_INCLUDED  */
