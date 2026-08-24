/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "parser.y"

/****************************************************************************\
Copyright (c) 2002, NVIDIA Corporation.

NVIDIA Corporation("NVIDIA") supplies this software to you in
consideration of your agreement to the following terms, and your use,
installation, modification or redistribution of this NVIDIA software
constitutes acceptance of these terms.  If you do not agree with these
terms, please do not use, install, modify or redistribute this NVIDIA
software.

In consideration of your agreement to abide by the following terms, and
subject to these terms, NVIDIA grants you a personal, non-exclusive
license, under NVIDIA's copyrights in this original NVIDIA software (the
"NVIDIA Software"), to use, reproduce, modify and redistribute the
NVIDIA Software, with or without modifications, in source and/or binary
forms; provided that if you redistribute the NVIDIA Software, you must
retain the copyright notice of NVIDIA, this notice and the following
text and disclaimers in all such redistributions of the NVIDIA Software.
Neither the name, trademarks, service marks nor logos of NVIDIA
Corporation may be used to endorse or promote products derived from the
NVIDIA Software without specific prior written permission from NVIDIA.
Except as expressly stated in this notice, no other rights or licenses
express or implied, are granted by NVIDIA herein, including but not
limited to any patent rights that may be infringed by your derivative
works or by other works in which the NVIDIA Software may be
incorporated. No hardware is licensed hereunder. 

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
PRODUCTS.

IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN ANY WAY
OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE
NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT,
TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF
NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#define NO_PARSER 1
#include "slglobals.h"
#include "cg_numeric.h"


#line 125 "parser.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parser.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_AND_SY = 3,                     /* AND_SY  */
  YYSYMBOL_ASM_SY = 4,                     /* ASM_SY  */
  YYSYMBOL_ASSIGNMINUS_SY = 5,             /* ASSIGNMINUS_SY  */
  YYSYMBOL_ASSIGNMOD_SY = 6,               /* ASSIGNMOD_SY  */
  YYSYMBOL_ASSIGNPLUS_SY = 7,              /* ASSIGNPLUS_SY  */
  YYSYMBOL_ASSIGNSLASH_SY = 8,             /* ASSIGNSLASH_SY  */
  YYSYMBOL_ASSIGNSTAR_SY = 9,              /* ASSIGNSTAR_SY  */
  YYSYMBOL_BOOLEAN_SY = 10,                /* BOOLEAN_SY  */
  YYSYMBOL_BREAK_SY = 11,                  /* BREAK_SY  */
  YYSYMBOL_CASE_SY = 12,                   /* CASE_SY  */
  YYSYMBOL_CFLOATCONST_SY = 13,            /* CFLOATCONST_SY  */
  YYSYMBOL_COLONCOLON_SY = 14,             /* COLONCOLON_SY  */
  YYSYMBOL_CONST_SY = 15,                  /* CONST_SY  */
  YYSYMBOL_CONTINUE_SY = 16,               /* CONTINUE_SY  */
  YYSYMBOL_DEFAULT_SY = 17,                /* DEFAULT_SY  */
  YYSYMBOL_DISCARD_SY = 18,                /* DISCARD_SY  */
  YYSYMBOL_DO_SY = 19,                     /* DO_SY  */
  YYSYMBOL_EQ_SY = 20,                     /* EQ_SY  */
  YYSYMBOL_ELSE_SY = 21,                   /* ELSE_SY  */
  YYSYMBOL_ERROR_SY = 22,                  /* ERROR_SY  */
  YYSYMBOL_EXTERN_SY = 23,                 /* EXTERN_SY  */
  YYSYMBOL_FLOAT_SY = 24,                  /* FLOAT_SY  */
  YYSYMBOL_FLOATCONST_SY = 25,             /* FLOATCONST_SY  */
  YYSYMBOL_FLOATHCONST_SY = 26,            /* FLOATHCONST_SY  */
  YYSYMBOL_FLOATXCONST_SY = 27,            /* FLOATXCONST_SY  */
  YYSYMBOL_FOR_SY = 28,                    /* FOR_SY  */
  YYSYMBOL_GE_SY = 29,                     /* GE_SY  */
  YYSYMBOL_GG_SY = 30,                     /* GG_SY  */
  YYSYMBOL_GOTO_SY = 31,                   /* GOTO_SY  */
  YYSYMBOL_IDENT_SY = 32,                  /* IDENT_SY  */
  YYSYMBOL_IF_SY = 33,                     /* IF_SY  */
  YYSYMBOL_IN_SY = 34,                     /* IN_SY  */
  YYSYMBOL_INLINE_SY = 35,                 /* INLINE_SY  */
  YYSYMBOL_INOUT_SY = 36,                  /* INOUT_SY  */
  YYSYMBOL_INT_SY = 37,                    /* INT_SY  */
  YYSYMBOL_INTCONST_SY = 38,               /* INTCONST_SY  */
  YYSYMBOL_INTERNAL_SY = 39,               /* INTERNAL_SY  */
  YYSYMBOL_LE_SY = 40,                     /* LE_SY  */
  YYSYMBOL_LL_SY = 41,                     /* LL_SY  */
  YYSYMBOL_MINUSMINUS_SY = 42,             /* MINUSMINUS_SY  */
  YYSYMBOL_NE_SY = 43,                     /* NE_SY  */
  YYSYMBOL_OR_SY = 44,                     /* OR_SY  */
  YYSYMBOL_OUT_SY = 45,                    /* OUT_SY  */
  YYSYMBOL_PACKED_SY = 46,                 /* PACKED_SY  */
  YYSYMBOL_PLUSPLUS_SY = 47,               /* PLUSPLUS_SY  */
  YYSYMBOL_RETURN_SY = 48,                 /* RETURN_SY  */
  YYSYMBOL_STATIC_SY = 49,                 /* STATIC_SY  */
  YYSYMBOL_STRCONST_SY = 50,               /* STRCONST_SY  */
  YYSYMBOL_STRUCT_SY = 51,                 /* STRUCT_SY  */
  YYSYMBOL_SWITCH_SY = 52,                 /* SWITCH_SY  */
  YYSYMBOL_TEXOBJ_SY = 53,                 /* TEXOBJ_SY  */
  YYSYMBOL_THIS_SY = 54,                   /* THIS_SY  */
  YYSYMBOL_TYPEDEF_SY = 55,                /* TYPEDEF_SY  */
  YYSYMBOL_TYPEIDENT_SY = 56,              /* TYPEIDENT_SY  */
  YYSYMBOL_UNIFORM_SY = 57,                /* UNIFORM_SY  */
  YYSYMBOL_VARYING_SY = 58,                /* VARYING_SY  */
  YYSYMBOL_VOID_SY = 59,                   /* VOID_SY  */
  YYSYMBOL_WHILE_SY = 60,                  /* WHILE_SY  */
  YYSYMBOL_CHAR_SY = 61,                   /* CHAR_SY  */
  YYSYMBOL_DOUBLE_SY = 62,                 /* DOUBLE_SY  */
  YYSYMBOL_FIXED_SY = 63,                  /* FIXED_SY  */
  YYSYMBOL_HALF_SY = 64,                   /* HALF_SY  */
  YYSYMBOL_INTERFACE_SY = 65,              /* INTERFACE_SY  */
  YYSYMBOL_LONG_SY = 66,                   /* LONG_SY  */
  YYSYMBOL_SHORT_SY = 67,                  /* SHORT_SY  */
  YYSYMBOL_UNSIGNED_SY = 68,               /* UNSIGNED_SY  */
  YYSYMBOL_RESERVED_SY = 69,               /* RESERVED_SY  */
  YYSYMBOL_FIRST_USER_TOKEN_SY = 70,       /* FIRST_USER_TOKEN_SY  */
  YYSYMBOL_71_ = 71,                       /* ';'  */
  YYSYMBOL_72_ = 72,                       /* ','  */
  YYSYMBOL_73_ = 73,                       /* '='  */
  YYSYMBOL_74_ = 74,                       /* '}'  */
  YYSYMBOL_75_ = 75,                       /* ':'  */
  YYSYMBOL_76_ = 76,                       /* '<'  */
  YYSYMBOL_77_ = 77,                       /* '>'  */
  YYSYMBOL_78_ = 78,                       /* '['  */
  YYSYMBOL_79_ = 79,                       /* ']'  */
  YYSYMBOL_80_ = 80,                       /* ')'  */
  YYSYMBOL_81_ = 81,                       /* '('  */
  YYSYMBOL_82_ = 82,                       /* '{'  */
  YYSYMBOL_83_ = 83,                       /* '.'  */
  YYSYMBOL_84_ = 84,                       /* '+'  */
  YYSYMBOL_85_ = 85,                       /* '-'  */
  YYSYMBOL_86_ = 86,                       /* '!'  */
  YYSYMBOL_87_ = 87,                       /* '~'  */
  YYSYMBOL_88_ = 88,                       /* '*'  */
  YYSYMBOL_89_ = 89,                       /* '/'  */
  YYSYMBOL_90_ = 90,                       /* '%'  */
  YYSYMBOL_91_ = 91,                       /* '&'  */
  YYSYMBOL_92_ = 92,                       /* '^'  */
  YYSYMBOL_93_ = 93,                       /* '|'  */
  YYSYMBOL_94_ = 94,                       /* '?'  */
  YYSYMBOL_YYACCEPT = 95,                  /* $accept  */
  YYSYMBOL_compilation_unit = 96,          /* compilation_unit  */
  YYSYMBOL_external_declaration = 97,      /* external_declaration  */
  YYSYMBOL_declaration = 98,               /* declaration  */
  YYSYMBOL_abstract_declaration = 99,      /* abstract_declaration  */
  YYSYMBOL_declaration_specifiers = 100,   /* declaration_specifiers  */
  YYSYMBOL_abstract_declaration_specifiers = 101, /* abstract_declaration_specifiers  */
  YYSYMBOL_abstract_declaration_specifiers2 = 102, /* abstract_declaration_specifiers2  */
  YYSYMBOL_init_declarator_list = 103,     /* init_declarator_list  */
  YYSYMBOL_init_declarator = 104,          /* init_declarator  */
  YYSYMBOL_type_specifier = 105,           /* type_specifier  */
  YYSYMBOL_type_qualifier = 106,           /* type_qualifier  */
  YYSYMBOL_type_domain = 107,              /* type_domain  */
  YYSYMBOL_storage_class = 108,            /* storage_class  */
  YYSYMBOL_function_specifier = 109,       /* function_specifier  */
  YYSYMBOL_in_out = 110,                   /* in_out  */
  YYSYMBOL_struct_or_connector_specifier = 111, /* struct_or_connector_specifier  */
  YYSYMBOL_struct_compound_header = 112,   /* struct_compound_header  */
  YYSYMBOL_struct_or_connector_header = 113, /* struct_or_connector_header  */
  YYSYMBOL_struct_identifier = 114,        /* struct_identifier  */
  YYSYMBOL_untagged_struct_header = 115,   /* untagged_struct_header  */
  YYSYMBOL_struct_declaration_list = 116,  /* struct_declaration_list  */
  YYSYMBOL_struct_declaration = 117,       /* struct_declaration  */
  YYSYMBOL_annotation = 118,               /* annotation  */
  YYSYMBOL_119_1 = 119,                    /* $@1  */
  YYSYMBOL_annotation_decl_list = 120,     /* annotation_decl_list  */
  YYSYMBOL_declarator = 121,               /* declarator  */
  YYSYMBOL_semantic_declarator = 122,      /* semantic_declarator  */
  YYSYMBOL_basic_declarator = 123,         /* basic_declarator  */
  YYSYMBOL_function_decl_header = 124,     /* function_decl_header  */
  YYSYMBOL_abstract_declarator = 125,      /* abstract_declarator  */
  YYSYMBOL_parameter_list = 126,           /* parameter_list  */
  YYSYMBOL_parameter_declaration = 127,    /* parameter_declaration  */
  YYSYMBOL_abstract_parameter_list = 128,  /* abstract_parameter_list  */
  YYSYMBOL_non_empty_abstract_parameter_list = 129, /* non_empty_abstract_parameter_list  */
  YYSYMBOL_initializer = 130,              /* initializer  */
  YYSYMBOL_initializer_list = 131,         /* initializer_list  */
  YYSYMBOL_variable = 132,                 /* variable  */
  YYSYMBOL_basic_variable = 133,           /* basic_variable  */
  YYSYMBOL_primary_expression = 134,       /* primary_expression  */
  YYSYMBOL_postfix_expression = 135,       /* postfix_expression  */
  YYSYMBOL_actual_argument_list = 136,     /* actual_argument_list  */
  YYSYMBOL_non_empty_argument_list = 137,  /* non_empty_argument_list  */
  YYSYMBOL_expression_list = 138,          /* expression_list  */
  YYSYMBOL_unary_expression = 139,         /* unary_expression  */
  YYSYMBOL_cast_expression = 140,          /* cast_expression  */
  YYSYMBOL_multiplicative_expression = 141, /* multiplicative_expression  */
  YYSYMBOL_additive_expression = 142,      /* additive_expression  */
  YYSYMBOL_shift_expression = 143,         /* shift_expression  */
  YYSYMBOL_relational_expression = 144,    /* relational_expression  */
  YYSYMBOL_equality_expression = 145,      /* equality_expression  */
  YYSYMBOL_AND_expression = 146,           /* AND_expression  */
  YYSYMBOL_exclusive_OR_expression = 147,  /* exclusive_OR_expression  */
  YYSYMBOL_inclusive_OR_expression = 148,  /* inclusive_OR_expression  */
  YYSYMBOL_logical_AND_expression = 149,   /* logical_AND_expression  */
  YYSYMBOL_logical_OR_expression = 150,    /* logical_OR_expression  */
  YYSYMBOL_conditional_expression = 151,   /* conditional_expression  */
  YYSYMBOL_conditional_test = 152,         /* conditional_test  */
  YYSYMBOL_expression = 153,               /* expression  */
  YYSYMBOL_function_definition = 154,      /* function_definition  */
  YYSYMBOL_function_definition_header = 155, /* function_definition_header  */
  YYSYMBOL_statement = 156,                /* statement  */
  YYSYMBOL_balanced_statement = 157,       /* balanced_statement  */
  YYSYMBOL_dangling_statement = 158,       /* dangling_statement  */
  YYSYMBOL_discard_statement = 159,        /* discard_statement  */
  YYSYMBOL_jump_statement = 160,           /* jump_statement  */
  YYSYMBOL_if_statement = 161,             /* if_statement  */
  YYSYMBOL_dangling_if = 162,              /* dangling_if  */
  YYSYMBOL_if_header = 163,                /* if_header  */
  YYSYMBOL_compound_statement = 164,       /* compound_statement  */
  YYSYMBOL_compound_header = 165,          /* compound_header  */
  YYSYMBOL_compound_tail = 166,            /* compound_tail  */
  YYSYMBOL_block_item_list = 167,          /* block_item_list  */
  YYSYMBOL_block_item = 168,               /* block_item  */
  YYSYMBOL_expression_statement = 169,     /* expression_statement  */
  YYSYMBOL_expression_statement2 = 170,    /* expression_statement2  */
  YYSYMBOL_iteration_statement = 171,      /* iteration_statement  */
  YYSYMBOL_dangling_iteration = 172,       /* dangling_iteration  */
  YYSYMBOL_boolean_scalar_expression = 173, /* boolean_scalar_expression  */
  YYSYMBOL_for_expression_opt = 174,       /* for_expression_opt  */
  YYSYMBOL_for_expression = 175,           /* for_expression  */
  YYSYMBOL_boolean_expression_opt = 176,   /* boolean_expression_opt  */
  YYSYMBOL_return_statement = 177,         /* return_statement  */
  YYSYMBOL_member_identifier = 178,        /* member_identifier  */
  YYSYMBOL_scope_identifier = 179,         /* scope_identifier  */
  YYSYMBOL_semantics_identifier = 180,     /* semantics_identifier  */
  YYSYMBOL_type_identifier = 181,          /* type_identifier  */
  YYSYMBOL_variable_identifier = 182,      /* variable_identifier  */
  YYSYMBOL_identifier = 183,               /* identifier  */
  YYSYMBOL_constant = 184                  /* constant  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  59
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1883

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  95
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  90
/* YYNRULES -- Number of rules.  */
#define YYNRULES  226
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  349

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   325


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    86,     2,     2,     2,    90,    91,     2,
      81,    80,    88,    84,    72,    85,    83,    89,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    75,    71,
      76,    73,    77,    94,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    78,     2,    79,    92,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    82,    93,    74,    87,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,     2
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   258,   258,   259,   266,   268,   271,   273,   275,   279,
     286,   288,   293,   295,   297,   299,   301,   303,   305,   310,
     312,   314,   316,   318,   320,   322,   326,   328,   332,   334,
     342,   344,   346,   348,   350,   352,   354,   356,   358,   360,
     362,   364,   366,   368,   370,   372,   374,   376,   378,   390,
     398,   400,   408,   410,   418,   420,   428,   430,   432,   441,
     443,   445,   449,   454,   456,   460,   461,   464,   468,   469,
     472,   491,   491,   496,   497,   504,   506,   510,   512,   516,
     518,   520,   522,   524,   528,   533,   534,   536,   553,   555,
     559,   561,   566,   567,   570,   576,   589,   591,   593,   595,
     603,   605,   617,   619,   623,   631,   632,   633,   635,   643,
     644,   646,   648,   650,   652,   657,   658,   661,   663,   667,
     669,   677,   678,   680,   682,   684,   686,   688,   696,   700,
     708,   709,   711,   713,   721,   722,   724,   732,   733,   735,
     743,   744,   746,   748,   750,   758,   759,   761,   769,   770,
     778,   779,   787,   788,   796,   797,   805,   806,   814,   815,
     819,   827,   838,   840,   844,   852,   853,   856,   857,   858,
     859,   860,   861,   862,   865,   866,   873,   875,   883,   885,
     893,   897,   899,   903,   911,   913,   917,   921,   929,   930,
     934,   935,   943,   944,   948,   950,   952,   954,   956,   958,
     960,   968,   970,   972,   976,   978,   983,   987,   989,   992,
     993,  1007,  1009,  1016,  1018,  1026,  1029,  1032,  1035,  1038,
    1041,  1043,  1053,  1055,  1057,  1059,  1061
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "AND_SY", "ASM_SY",
  "ASSIGNMINUS_SY", "ASSIGNMOD_SY", "ASSIGNPLUS_SY", "ASSIGNSLASH_SY",
  "ASSIGNSTAR_SY", "BOOLEAN_SY", "BREAK_SY", "CASE_SY", "CFLOATCONST_SY",
  "COLONCOLON_SY", "CONST_SY", "CONTINUE_SY", "DEFAULT_SY", "DISCARD_SY",
  "DO_SY", "EQ_SY", "ELSE_SY", "ERROR_SY", "EXTERN_SY", "FLOAT_SY",
  "FLOATCONST_SY", "FLOATHCONST_SY", "FLOATXCONST_SY", "FOR_SY", "GE_SY",
  "GG_SY", "GOTO_SY", "IDENT_SY", "IF_SY", "IN_SY", "INLINE_SY",
  "INOUT_SY", "INT_SY", "INTCONST_SY", "INTERNAL_SY", "LE_SY", "LL_SY",
  "MINUSMINUS_SY", "NE_SY", "OR_SY", "OUT_SY", "PACKED_SY", "PLUSPLUS_SY",
  "RETURN_SY", "STATIC_SY", "STRCONST_SY", "STRUCT_SY", "SWITCH_SY",
  "TEXOBJ_SY", "THIS_SY", "TYPEDEF_SY", "TYPEIDENT_SY", "UNIFORM_SY",
  "VARYING_SY", "VOID_SY", "WHILE_SY", "CHAR_SY", "DOUBLE_SY", "FIXED_SY",
  "HALF_SY", "INTERFACE_SY", "LONG_SY", "SHORT_SY", "UNSIGNED_SY",
  "RESERVED_SY", "FIRST_USER_TOKEN_SY", "';'", "','", "'='", "'}'", "':'",
  "'<'", "'>'", "'['", "']'", "')'", "'('", "'{'", "'.'", "'+'", "'-'",
  "'!'", "'~'", "'*'", "'/'", "'%'", "'&'", "'^'", "'|'", "'?'", "$accept",
  "compilation_unit", "external_declaration", "declaration",
  "abstract_declaration", "declaration_specifiers",
  "abstract_declaration_specifiers", "abstract_declaration_specifiers2",
  "init_declarator_list", "init_declarator", "type_specifier",
  "type_qualifier", "type_domain", "storage_class", "function_specifier",
  "in_out", "struct_or_connector_specifier", "struct_compound_header",
  "struct_or_connector_header", "struct_identifier",
  "untagged_struct_header", "struct_declaration_list",
  "struct_declaration", "annotation", "$@1", "annotation_decl_list",
  "declarator", "semantic_declarator", "basic_declarator",
  "function_decl_header", "abstract_declarator", "parameter_list",
  "parameter_declaration", "abstract_parameter_list",
  "non_empty_abstract_parameter_list", "initializer", "initializer_list",
  "variable", "basic_variable", "primary_expression", "postfix_expression",
  "actual_argument_list", "non_empty_argument_list", "expression_list",
  "unary_expression", "cast_expression", "multiplicative_expression",
  "additive_expression", "shift_expression", "relational_expression",
  "equality_expression", "AND_expression", "exclusive_OR_expression",
  "inclusive_OR_expression", "logical_AND_expression",
  "logical_OR_expression", "conditional_expression", "conditional_test",
  "expression", "function_definition", "function_definition_header",
  "statement", "balanced_statement", "dangling_statement",
  "discard_statement", "jump_statement", "if_statement", "dangling_if",
  "if_header", "compound_statement", "compound_header", "compound_tail",
  "block_item_list", "block_item", "expression_statement",
  "expression_statement2", "iteration_statement", "dangling_iteration",
  "boolean_scalar_expression", "for_expression_opt", "for_expression",
  "boolean_expression_opt", "return_statement", "member_identifier",
  "scope_identifier", "semantics_identifier", "type_identifier",
  "variable_identifier", "identifier", "constant", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-288)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-217)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    1697,  -288,  -288,  -288,   -44,  -288,  -288,  -288,  -288,  -288,
    -288,  -288,  -288,  1815,  -288,    -8,  -288,  1815,  -288,  -288,
    -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,    84,  1638,
    -288,  -288,     3,  -288,   160,  -288,  1815,  1815,  1815,  1815,
    1815,  -288,   -14,   -14,  -288,   296,  -288,  -288,  -288,  -288,
    -288,   -24,  -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,
    -288,  -288,    41,  -288,   -11,   -22,    50,  1383,  -288,  -288,
    -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,
    -288,  1697,  -288,  1697,    22,  -288,    24,   798,   594,  -288,
    -288,  -288,    23,    53,  -288,  1246,  1246,   862,    60,  -288,
    -288,   530,  1246,  1246,  1246,  1246,  -288,     3,    68,  -288,
    -288,  -288,   195,  -288,  -288,    58,    42,    62,   -19,    -1,
      29,    51,    59,   153,   -29,  -288,    69,  -288,  -288,  -288,
    -288,  -288,  -288,  -288,  -288,   594,  -288,   374,   452,  -288,
    -288,    91,  -288,  -288,  -288,   151,  -288,   152,  -288,    21,
    -288,    21,   926,  -288,  -288,  -288,    21,   -15,  -288,  -288,
      21,    38,   -52,  -288,    92,   101,  -288,  1501,  -288,  1569,
    -288,  -288,  -288,    68,     8,   103,   116,   990,  1310,  1310,
    -288,  -288,  -288,   107,  1310,   100,  -288,   102,  -288,  -288,
    -288,  -288,   112,  1310,  1310,  1310,  1310,  1310,  1310,  -288,
    -288,  1310,  1310,  1054,    21,  1310,  1310,  1310,  1310,  1310,
    1310,  1310,  1310,  1310,  1310,  1310,  1310,  1310,  1310,  1310,
    1310,  1310,  1310,  1310,  -288,   165,  -288,  -288,   374,  -288,
    -288,  -288,    21,  -288,  -288,  -288,   658,  -288,  -288,  -288,
    -288,   110,  -288,   119,   113,  1756,  -288,  -288,  1815,  -288,
    -288,  -288,  -288,   109,  -288,   122,   125,  -288,   127,  -288,
     128,  1310,  -288,   -31,  -288,  -288,  -288,  -288,  -288,  -288,
    -288,   131,   135,   126,  -288,  -288,  -288,  -288,  -288,  -288,
      58,    58,    42,    42,    62,    62,    62,    62,   -19,   -19,
      -1,    29,    51,    59,   153,   136,   594,  -288,  -288,  -288,
    -288,  -288,   -28,  1442,  -288,   926,   -13,  -288,  -288,  1310,
    1118,  1310,  -288,   594,  -288,  1310,  -288,  -288,  -288,  1310,
    1310,  -288,  -288,   728,  -288,  -288,  -288,  -288,   137,  -288,
     143,  -288,   154,  -288,  -288,  -288,  -288,  -288,  -288,  -288,
    -288,  -288,   156,  1182,  -288,   144,   594,  -288,  -288
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    48,    33,    49,     0,    53,    31,    56,    54,    58,
      30,    55,    57,     0,    52,    67,    34,     0,   218,    50,
      51,    32,    35,    40,    39,    38,    37,    36,    41,     0,
       2,     4,     0,    10,    12,    19,     0,     0,     0,     0,
       0,    46,    61,     0,     5,     0,    47,     8,    18,   220,
     221,    63,    66,    65,    11,    44,    42,    45,    43,     1,
       3,     6,     0,    26,    28,    75,    77,     0,    79,    25,
      20,    22,    21,    24,    23,    13,    15,    14,    17,    16,
     186,     0,    62,     0,     0,   223,     0,     0,     0,   224,
     225,   226,     0,     0,   222,     0,     0,     0,     0,   193,
     163,     0,     0,     0,     0,     0,   190,     0,    19,   105,
     102,   109,   121,   128,   130,   134,   137,   140,   145,   148,
     150,   152,   154,   156,   158,   161,     0,   195,   191,   165,
     166,   168,   172,   171,   174,     0,   167,     0,     0,   188,
     169,     0,   170,   175,   173,     0,   104,   219,   106,     0,
       7,     0,     0,   164,    71,    76,     0,     0,    84,    94,
       0,    85,     0,    88,     0,    93,    70,     0,    68,     0,
     178,   179,   176,     0,   121,     0,     0,     0,     0,     0,
     123,   122,   214,     0,     0,     0,    85,     0,   124,   125,
     126,   127,    28,     0,     0,     0,     0,     0,     0,   111,
     110,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   181,   165,   187,   185,     0,   162,
     189,   192,     0,    64,   217,    27,     0,    29,    96,    73,
      78,     0,    81,    90,     9,     0,    82,    83,     0,    59,
      69,    60,   177,     0,   209,     0,   207,   206,     0,   213,
       0,     0,   107,     0,   119,   196,   197,   198,   199,   200,
     194,     0,     0,   116,   117,   112,   215,   131,   132,   133,
     135,   136,   139,   138,   144,   143,   141,   142,   146,   147,
     149,   151,   153,   155,   157,     0,     0,   184,   103,   219,
      99,   100,     0,     0,    80,     0,     0,    89,    95,     0,
       0,     0,   183,     0,   129,     0,   108,   113,   114,     0,
       0,   180,   182,     0,    97,    72,    74,    91,     0,    87,
       0,   211,     0,   210,   201,   204,   120,   118,   159,    98,
     101,    86,     0,     0,   202,     0,     0,   203,   205
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -288,  -288,   200,     1,   -95,     2,    39,  -288,  -288,    82,
       0,   201,   204,   205,   207,   209,  -288,   191,  -288,  -288,
    -288,   163,   -82,  -288,  -288,  -288,   -18,  -288,  -288,  -288,
    -288,  -288,     4,  -288,  -288,  -225,  -288,  -288,    18,  -288,
     -41,  -288,  -288,  -288,   117,  -173,   -54,   -81,  -104,   -58,
      33,    34,    32,    35,    36,  -288,   -66,  -288,   -79,  -288,
    -288,   -72,  -132,  -287,  -288,  -288,  -288,  -288,  -288,  -288,
     118,    31,   120,  -126,  -288,  -172,  -288,  -288,  -177,   -80,
    -288,  -288,  -288,  -288,  -288,   105,   247,  -288,    28,  -288
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    29,    30,   106,   159,   107,    33,    34,    62,    63,
     173,    36,    37,    38,    39,    40,    41,    81,    42,    51,
      43,   167,   168,   155,   239,   303,   192,    65,    66,    67,
     244,   162,   163,   164,   165,   237,   302,   109,   110,   111,
     174,   272,   273,   263,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,    44,
      45,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   227,   138,   139,   140,   141,   142,   143,   258,   255,
     256,   332,   144,   275,   145,   233,    46,   146,   147,   148
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      35,    31,    32,   225,   112,   254,   185,   260,   175,   322,
     212,   301,   230,    35,    64,   222,   176,    35,   183,   216,
     245,   213,   187,   241,    49,   328,   335,    47,   246,    35,
      31,    32,   277,   278,   279,    49,    35,    35,    35,    35,
      35,   315,   217,    53,   323,   108,   324,   112,    18,   316,
     199,   149,    48,    49,   154,   200,    54,   214,   215,   348,
      68,    50,   152,   224,   242,  -160,   329,    35,    80,   160,
     -10,   153,    50,   238,    61,    75,    76,    77,    78,    79,
     327,    35,   166,    35,   166,   250,   202,   250,   314,   203,
      50,   204,   210,   170,   112,   171,   112,   112,   340,   257,
     187,   108,   230,   211,   177,   257,   161,   -10,   284,   285,
     286,   287,   150,   151,   264,   265,   266,   267,   268,   269,
     218,    55,   270,   271,   274,   156,   208,   209,   157,   282,
     283,   158,   330,   331,   178,    68,   112,   108,   108,   333,
     186,   184,   243,   219,   295,    56,   205,   206,   207,   193,
      57,    58,   220,   308,   280,   281,   221,   238,   288,   289,
      82,    82,   231,   223,   321,   232,  -216,    35,   166,    35,
     166,   254,   247,   248,   252,     3,   253,   234,   259,    68,
     261,   334,   262,     5,   234,   152,   296,   112,    68,   304,
     309,   306,   305,   310,     7,     8,     9,   311,   319,    11,
     194,   195,   196,   197,   198,    12,    69,   312,   313,    14,
     317,   320,   180,   181,   347,   318,   341,    19,    20,   188,
     189,   190,   191,   342,   346,   343,   238,   344,   108,    60,
     257,   257,   276,   235,    83,    70,   336,   199,    71,    72,
     337,    73,   200,    74,   238,    35,   169,   160,    35,   307,
     298,   290,   292,   291,   338,   112,   293,   228,   294,   297,
     299,   240,    52,   345,     0,     0,     0,     0,   201,     0,
     112,     0,   112,   202,     0,     0,   203,     0,   204,     0,
       0,     0,     0,     0,     0,     0,     0,   186,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     1,     0,     0,
       0,     0,   112,    35,   326,   112,     2,    84,     0,    85,
       0,     3,    86,     0,    87,    88,     0,     0,     4,     5,
       6,    89,    90,    91,    92,     0,     0,     0,    49,    93,
       7,     8,     9,    10,    94,    11,     0,     0,    95,     0,
       0,    12,    13,    96,    97,    14,     0,    15,     0,    16,
       0,    17,    18,    19,    20,    21,    98,    22,    23,    24,
      25,     0,    26,    27,    28,    50,     0,    99,     0,     0,
     100,     0,     0,     0,     0,     1,     0,   101,    80,     0,
     102,   103,   104,   105,     2,    84,     0,    85,     0,     3,
      86,     0,    87,    88,     0,     0,     4,     5,     6,    89,
      90,    91,    92,     0,     0,     0,    49,    93,     7,     8,
       9,    10,    94,    11,     0,     0,    95,     0,     0,    12,
      13,    96,    97,    14,     0,    15,     0,    16,     0,    17,
      18,    19,    20,    21,    98,    22,    23,    24,    25,     0,
      26,    27,    28,    50,     0,    99,     0,     0,   226,     0,
       0,     0,     0,     1,     0,   101,    80,     0,   102,   103,
     104,   105,     2,    84,     0,    85,     0,     3,    86,     0,
      87,    88,     0,     0,     4,     5,     6,    89,    90,    91,
      92,     0,     0,     0,    49,    93,     7,     8,     9,    10,
      94,    11,     0,     0,    95,     0,     0,    12,    13,    96,
      97,    14,     0,    15,     0,    16,     0,    17,    18,    19,
      20,    21,    98,    22,    23,    24,    25,     0,    26,    27,
      28,    50,     0,    99,     0,     0,   229,     0,     0,     0,
       0,     1,     0,   101,    80,     0,   102,   103,   104,   105,
       2,     0,     0,    85,     0,     3,     0,     0,     0,     0,
       0,     0,     0,     5,     6,    89,    90,    91,     0,     0,
       0,     0,    49,     0,     7,     8,     9,    10,    94,    11,
       0,     0,    95,     0,     0,    12,    13,    96,     0,    14,
       0,    15,     0,    16,     0,     0,    18,    19,    20,    21,
       0,    22,    23,    24,    25,     1,    26,    27,    28,    50,
       0,     0,     0,     0,     2,    84,     0,    85,     0,     0,
      86,   101,    87,    88,   102,   103,   104,   105,     6,    89,
      90,    91,    92,     0,     0,     0,    49,    93,     0,     0,
       0,    10,    94,     0,     0,     0,    95,     0,     0,     0,
       0,    96,    97,     0,     0,    15,     0,    16,     0,     0,
      18,     0,     0,    21,    98,    22,    23,    24,    25,     1,
      26,    27,    28,    50,     0,    99,     0,     0,     2,     0,
       0,    85,     0,     0,     0,   101,    80,     0,   102,   103,
     104,   105,     6,    89,    90,    91,     0,     0,     0,     0,
      49,     0,     0,     0,     0,    10,    94,     0,     0,     0,
      95,     0,     0,     0,     0,    96,     0,     0,     0,    15,
       0,    16,     0,     0,    18,     0,     0,    21,     0,    22,
      23,    24,    25,     0,    26,    27,    28,    50,     0,     1,
       0,     0,   300,     0,     0,     0,     0,     0,     2,   101,
     236,    85,   102,   103,   104,   105,     0,     0,     0,     0,
       0,     0,     6,    89,    90,    91,     0,     0,     0,     0,
      49,     0,     0,     0,     0,    10,    94,     0,     0,     0,
      95,     0,     0,     0,     0,    96,     0,     0,     0,    15,
       0,    16,     0,     0,    18,     0,     0,    21,     0,    22,
      23,    24,    25,     0,    26,    27,    28,    50,     0,     1,
       0,     0,   339,     0,     0,     0,     0,     0,     2,   101,
     236,    85,   102,   103,   104,   105,     0,     0,     0,     0,
       0,     0,     6,    89,    90,    91,     0,     0,     0,     0,
      49,     0,     0,     0,     0,    10,    94,     0,     0,     0,
      95,     0,     0,     0,     0,    96,     0,     0,     0,    15,
       0,    16,     0,     0,    18,     0,     0,    21,     0,    22,
      23,    24,    25,     1,    26,    27,    28,    50,     0,   172,
       0,     0,     2,     0,     0,    85,     0,     0,     0,   101,
       0,     0,   102,   103,   104,   105,     6,    89,    90,    91,
       0,     0,     0,     0,    49,     0,     0,     0,     0,    10,
      94,     0,     0,     0,    95,     0,     0,     0,     0,    96,
       0,     0,     0,    15,     0,    16,     0,     0,    18,     0,
       0,    21,     0,    22,    23,    24,    25,     1,    26,    27,
      28,    50,     0,   182,     0,     0,     2,     0,     0,    85,
       0,     0,     0,   101,     0,     0,   102,   103,   104,   105,
       6,    89,    90,    91,     0,     0,     0,     0,    49,     0,
       0,     0,     0,    10,    94,     0,     0,     0,    95,     0,
       0,     0,     0,    96,     0,     0,     0,    15,     0,    16,
       0,     0,    18,     0,     0,    21,     0,    22,    23,    24,
      25,     1,    26,    27,    28,    50,     0,     0,     0,     0,
       2,     0,     0,    85,     0,     0,     0,   101,   236,     0,
     102,   103,   104,   105,     6,    89,    90,    91,     0,     0,
       0,     0,    49,     0,     0,     0,     0,    10,    94,     0,
       0,     0,    95,     0,     0,     0,     0,    96,     0,     0,
       0,    15,     0,    16,     0,     0,    18,     0,     0,    21,
       0,    22,    23,    24,    25,     1,    26,    27,    28,    50,
       0,  -208,     0,     0,     2,     0,     0,    85,     0,     0,
       0,   101,     0,     0,   102,   103,   104,   105,     6,    89,
      90,    91,     0,     0,     0,     0,    49,     0,     0,     0,
       0,    10,    94,     0,     0,     0,    95,     0,     0,     0,
       0,    96,     0,     0,     0,    15,     0,    16,     0,     0,
      18,     0,     0,    21,     0,    22,    23,    24,    25,     1,
      26,    27,    28,    50,     0,     0,     0,     0,     2,     0,
       0,    85,     0,     0,  -115,   101,     0,     0,   102,   103,
     104,   105,     6,    89,    90,    91,     0,     0,     0,     0,
      49,     0,     0,     0,     0,    10,    94,     0,     0,     0,
      95,     0,     0,     0,     0,    96,     0,     0,     0,    15,
       0,    16,     0,     0,    18,     0,     0,    21,     0,    22,
      23,    24,    25,     1,    26,    27,    28,    50,     0,  -212,
       0,     0,     2,     0,     0,    85,     0,     0,     0,   101,
       0,     0,   102,   103,   104,   105,     6,    89,    90,    91,
       0,     0,     0,     0,    49,     0,     0,     0,     0,    10,
      94,     0,     0,     0,    95,     0,     0,     0,     0,    96,
       0,     0,     0,    15,     0,    16,     0,     0,    18,     0,
       0,    21,     0,    22,    23,    24,    25,     1,    26,    27,
      28,    50,     0,     0,     0,     0,     2,     0,     0,    85,
       0,     0,  -208,   101,     0,     0,   102,   103,   104,   105,
       6,    89,    90,    91,     0,     0,     0,     0,    49,     0,
       0,     0,     0,    10,    94,     0,     0,     0,    95,     0,
       0,     0,     0,    96,     0,     0,     0,    15,     0,    16,
       0,     0,    18,     0,     0,    21,     0,    22,    23,    24,
      25,     1,    26,    27,    28,    50,     0,     0,     0,     0,
       2,     0,     0,    85,     0,     0,     0,   179,     0,     0,
     102,   103,   104,   105,     6,    89,    90,    91,     0,     0,
       0,     0,    49,     0,     0,     0,     0,    10,    94,     0,
       0,     0,    95,     0,     0,     0,     0,    96,     0,     0,
       0,    15,     0,    16,     0,     0,    18,     0,     0,    21,
       0,    22,    23,    24,    25,     0,    26,    27,    28,    50,
       0,     0,     0,     0,     1,     0,     0,     0,     0,     0,
       0,   101,     0,     2,   102,   103,   104,   105,     3,     0,
       0,     0,     0,     0,     0,     0,     5,     6,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     7,     8,     9,
      10,     0,    11,     0,     0,     0,     0,     0,    12,    13,
       0,     0,    14,     0,    15,     0,    16,     0,    17,    18,
      19,    20,    21,     1,    22,    23,    24,    25,     0,    26,
      27,    28,     2,     0,     0,     0,     0,     3,     0,     0,
       0,     0,     0,   -92,     4,     5,     6,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     7,     8,     9,    10,
       0,    11,     0,     0,     0,     0,     0,    12,    13,     0,
       0,    14,     0,    15,     0,    16,     0,    17,    18,    19,
      20,    21,     1,    22,    23,    24,    25,     0,    26,    27,
      28,     2,     0,     0,     0,     0,     3,     0,     0,   325,
       0,     0,     0,     4,     5,     6,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     7,     8,     9,    10,     0,
      11,     0,     0,     0,     0,     0,    12,    13,     0,     0,
      14,     0,    15,     0,    16,     0,    17,    18,    19,    20,
      21,     0,    22,    23,    24,    25,     0,    26,    27,    28,
       1,     0,     0,     0,     0,   249,     0,     0,     0,     2,
       0,     0,     0,     0,     3,     0,     0,     0,     0,     0,
       0,     4,     5,     6,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     7,     8,     9,    10,     0,    11,     0,
       0,     0,     0,     0,    12,    13,     0,     0,    14,     0,
      15,     0,    16,     0,    17,    18,    19,    20,    21,     0,
      22,    23,    24,    25,     0,    26,    27,    28,    59,     1,
       0,     0,     0,   251,     0,     0,     0,     0,     2,     0,
       0,     0,     0,     3,     0,     0,     0,     0,     0,     0,
       4,     5,     6,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     7,     8,     9,    10,     0,    11,     0,     0,
       0,     0,     0,    12,    13,     0,     0,    14,     0,    15,
       0,    16,     0,    17,    18,    19,    20,    21,     1,    22,
      23,    24,    25,     0,    26,    27,    28,     2,     0,     0,
       0,     0,     3,     0,     0,     0,     0,     0,     0,     4,
       5,     6,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     7,     8,     9,    10,     0,    11,     0,     0,     0,
       0,     0,    12,    13,     0,     0,    14,     0,    15,     0,
      16,     0,    17,    18,    19,    20,    21,     1,    22,    23,
      24,    25,     0,    26,    27,    28,     2,     0,     0,     0,
       0,     3,     0,     0,     0,     0,     0,     0,     0,     5,
       6,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       7,     8,     9,    10,     0,    11,     0,     0,     0,     0,
       0,    12,    13,     0,     0,    14,     0,    15,     0,    16,
       0,    17,    18,    19,    20,    21,     1,    22,    23,    24,
      25,     0,    26,    27,    28,     2,     0,     0,     0,     0,
       3,     0,     0,     0,     0,     0,     0,     0,     5,     6,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     7,
       8,     9,    10,     0,    11,     0,     0,     0,     0,     0,
      12,    13,     0,     0,    14,     0,    15,     0,    16,     0,
       0,    18,    19,    20,    21,     0,    22,    23,    24,    25,
       0,    26,    27,    28
};

static const yytype_int16 yycheck[] =
{
       0,     0,     0,   135,    45,   177,   101,   184,    87,   296,
      29,   236,   138,    13,    32,    44,    88,    17,    97,    20,
      72,    40,   101,    38,    32,    38,   313,    71,    80,    29,
      29,    29,   205,   206,   207,    32,    36,    37,    38,    39,
      40,    72,    43,    15,    72,    45,    74,    88,    56,    80,
      42,    75,    13,    32,    76,    47,    17,    76,    77,   346,
      32,    69,    73,   135,    79,    94,    79,    67,    82,    67,
      32,    82,    69,   152,    71,    36,    37,    38,    39,    40,
     305,    81,    81,    83,    83,   167,    78,   169,   261,    81,
      69,    83,    30,    71,   135,    71,   137,   138,   323,   178,
     179,   101,   228,    41,    81,   184,    67,    69,   212,   213,
     214,   215,    71,    72,   193,   194,   195,   196,   197,   198,
      91,    37,   201,   202,   203,    75,    84,    85,    78,   210,
     211,    81,   309,   310,    81,   107,   177,   137,   138,   311,
     101,    81,   160,    92,   223,    61,    88,    89,    90,    81,
      66,    67,    93,   248,   208,   209,     3,   236,   216,   217,
      42,    43,    71,    94,   296,    14,    14,   167,   167,   169,
     169,   343,    80,    72,    71,    15,    60,   149,    71,   151,
      80,   313,    80,    23,   156,    73,    21,   228,   160,    79,
      81,    78,    73,    71,    34,    35,    36,    72,    72,    39,
       5,     6,     7,     8,     9,    45,    46,    80,    80,    49,
      79,    75,    95,    96,   346,    80,    79,    57,    58,   102,
     103,   104,   105,    80,    80,    71,   305,    71,   228,    29,
     309,   310,   204,   151,    43,    34,   315,    42,    34,    34,
     319,    34,    47,    34,   323,   245,    83,   245,   248,   245,
     232,   218,   220,   219,   320,   296,   221,   137,   222,   228,
     232,   156,    15,   343,    -1,    -1,    -1,    -1,    73,    -1,
     311,    -1,   313,    78,    -1,    -1,    81,    -1,    83,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   248,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,
      -1,    -1,   343,   303,   303,   346,    10,    11,    -1,    13,
      -1,    15,    16,    -1,    18,    19,    -1,    -1,    22,    23,
      24,    25,    26,    27,    28,    -1,    -1,    -1,    32,    33,
      34,    35,    36,    37,    38,    39,    -1,    -1,    42,    -1,
      -1,    45,    46,    47,    48,    49,    -1,    51,    -1,    53,
      -1,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    -1,    66,    67,    68,    69,    -1,    71,    -1,    -1,
      74,    -1,    -1,    -1,    -1,     1,    -1,    81,    82,    -1,
      84,    85,    86,    87,    10,    11,    -1,    13,    -1,    15,
      16,    -1,    18,    19,    -1,    -1,    22,    23,    24,    25,
      26,    27,    28,    -1,    -1,    -1,    32,    33,    34,    35,
      36,    37,    38,    39,    -1,    -1,    42,    -1,    -1,    45,
      46,    47,    48,    49,    -1,    51,    -1,    53,    -1,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    -1,
      66,    67,    68,    69,    -1,    71,    -1,    -1,    74,    -1,
      -1,    -1,    -1,     1,    -1,    81,    82,    -1,    84,    85,
      86,    87,    10,    11,    -1,    13,    -1,    15,    16,    -1,
      18,    19,    -1,    -1,    22,    23,    24,    25,    26,    27,
      28,    -1,    -1,    -1,    32,    33,    34,    35,    36,    37,
      38,    39,    -1,    -1,    42,    -1,    -1,    45,    46,    47,
      48,    49,    -1,    51,    -1,    53,    -1,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    -1,    66,    67,
      68,    69,    -1,    71,    -1,    -1,    74,    -1,    -1,    -1,
      -1,     1,    -1,    81,    82,    -1,    84,    85,    86,    87,
      10,    -1,    -1,    13,    -1,    15,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    23,    24,    25,    26,    27,    -1,    -1,
      -1,    -1,    32,    -1,    34,    35,    36,    37,    38,    39,
      -1,    -1,    42,    -1,    -1,    45,    46,    47,    -1,    49,
      -1,    51,    -1,    53,    -1,    -1,    56,    57,    58,    59,
      -1,    61,    62,    63,    64,     1,    66,    67,    68,    69,
      -1,    -1,    -1,    -1,    10,    11,    -1,    13,    -1,    -1,
      16,    81,    18,    19,    84,    85,    86,    87,    24,    25,
      26,    27,    28,    -1,    -1,    -1,    32,    33,    -1,    -1,
      -1,    37,    38,    -1,    -1,    -1,    42,    -1,    -1,    -1,
      -1,    47,    48,    -1,    -1,    51,    -1,    53,    -1,    -1,
      56,    -1,    -1,    59,    60,    61,    62,    63,    64,     1,
      66,    67,    68,    69,    -1,    71,    -1,    -1,    10,    -1,
      -1,    13,    -1,    -1,    -1,    81,    82,    -1,    84,    85,
      86,    87,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,
      42,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,
      -1,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,
      62,    63,    64,    -1,    66,    67,    68,    69,    -1,     1,
      -1,    -1,    74,    -1,    -1,    -1,    -1,    -1,    10,    81,
      82,    13,    84,    85,    86,    87,    -1,    -1,    -1,    -1,
      -1,    -1,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,
      42,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,
      -1,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,
      62,    63,    64,    -1,    66,    67,    68,    69,    -1,     1,
      -1,    -1,    74,    -1,    -1,    -1,    -1,    -1,    10,    81,
      82,    13,    84,    85,    86,    87,    -1,    -1,    -1,    -1,
      -1,    -1,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,
      42,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,
      -1,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,
      62,    63,    64,     1,    66,    67,    68,    69,    -1,    71,
      -1,    -1,    10,    -1,    -1,    13,    -1,    -1,    -1,    81,
      -1,    -1,    84,    85,    86,    87,    24,    25,    26,    27,
      -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,
      38,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    47,
      -1,    -1,    -1,    51,    -1,    53,    -1,    -1,    56,    -1,
      -1,    59,    -1,    61,    62,    63,    64,     1,    66,    67,
      68,    69,    -1,    71,    -1,    -1,    10,    -1,    -1,    13,
      -1,    -1,    -1,    81,    -1,    -1,    84,    85,    86,    87,
      24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,    -1,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,    53,
      -1,    -1,    56,    -1,    -1,    59,    -1,    61,    62,    63,
      64,     1,    66,    67,    68,    69,    -1,    -1,    -1,    -1,
      10,    -1,    -1,    13,    -1,    -1,    -1,    81,    82,    -1,
      84,    85,    86,    87,    24,    25,    26,    27,    -1,    -1,
      -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    38,    -1,
      -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    -1,    -1,
      -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,    59,
      -1,    61,    62,    63,    64,     1,    66,    67,    68,    69,
      -1,    71,    -1,    -1,    10,    -1,    -1,    13,    -1,    -1,
      -1,    81,    -1,    -1,    84,    85,    86,    87,    24,    25,
      26,    27,    -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    37,    38,    -1,    -1,    -1,    42,    -1,    -1,    -1,
      -1,    47,    -1,    -1,    -1,    51,    -1,    53,    -1,    -1,
      56,    -1,    -1,    59,    -1,    61,    62,    63,    64,     1,
      66,    67,    68,    69,    -1,    -1,    -1,    -1,    10,    -1,
      -1,    13,    -1,    -1,    80,    81,    -1,    -1,    84,    85,
      86,    87,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,
      42,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,
      -1,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,
      62,    63,    64,     1,    66,    67,    68,    69,    -1,    71,
      -1,    -1,    10,    -1,    -1,    13,    -1,    -1,    -1,    81,
      -1,    -1,    84,    85,    86,    87,    24,    25,    26,    27,
      -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,
      38,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    47,
      -1,    -1,    -1,    51,    -1,    53,    -1,    -1,    56,    -1,
      -1,    59,    -1,    61,    62,    63,    64,     1,    66,    67,
      68,    69,    -1,    -1,    -1,    -1,    10,    -1,    -1,    13,
      -1,    -1,    80,    81,    -1,    -1,    84,    85,    86,    87,
      24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,    -1,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,    53,
      -1,    -1,    56,    -1,    -1,    59,    -1,    61,    62,    63,
      64,     1,    66,    67,    68,    69,    -1,    -1,    -1,    -1,
      10,    -1,    -1,    13,    -1,    -1,    -1,    81,    -1,    -1,
      84,    85,    86,    87,    24,    25,    26,    27,    -1,    -1,
      -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    38,    -1,
      -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    -1,    -1,
      -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,    59,
      -1,    61,    62,    63,    64,    -1,    66,    67,    68,    69,
      -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,
      -1,    81,    -1,    10,    84,    85,    86,    87,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    23,    24,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,
      37,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,
      -1,    -1,    49,    -1,    51,    -1,    53,    -1,    55,    56,
      57,    58,    59,     1,    61,    62,    63,    64,    -1,    66,
      67,    68,    10,    -1,    -1,    -1,    -1,    15,    -1,    -1,
      -1,    -1,    -1,    80,    22,    23,    24,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,    37,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,    -1,
      -1,    49,    -1,    51,    -1,    53,    -1,    55,    56,    57,
      58,    59,     1,    61,    62,    63,    64,    -1,    66,    67,
      68,    10,    -1,    -1,    -1,    -1,    15,    -1,    -1,    77,
      -1,    -1,    -1,    22,    23,    24,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    34,    35,    36,    37,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    46,    -1,    -1,
      49,    -1,    51,    -1,    53,    -1,    55,    56,    57,    58,
      59,    -1,    61,    62,    63,    64,    -1,    66,    67,    68,
       1,    -1,    -1,    -1,    -1,    74,    -1,    -1,    -1,    10,
      -1,    -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,
      -1,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    34,    35,    36,    37,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    46,    -1,    -1,    49,    -1,
      51,    -1,    53,    -1,    55,    56,    57,    58,    59,    -1,
      61,    62,    63,    64,    -1,    66,    67,    68,     0,     1,
      -1,    -1,    -1,    74,    -1,    -1,    -1,    -1,    10,    -1,
      -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,
      22,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    34,    35,    36,    37,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    46,    -1,    -1,    49,    -1,    51,
      -1,    53,    -1,    55,    56,    57,    58,    59,     1,    61,
      62,    63,    64,    -1,    66,    67,    68,    10,    -1,    -1,
      -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,
      23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    35,    36,    37,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    46,    -1,    -1,    49,    -1,    51,    -1,
      53,    -1,    55,    56,    57,    58,    59,     1,    61,    62,
      63,    64,    -1,    66,    67,    68,    10,    -1,    -1,    -1,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      34,    35,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    46,    -1,    -1,    49,    -1,    51,    -1,    53,
      -1,    55,    56,    57,    58,    59,     1,    61,    62,    63,
      64,    -1,    66,    67,    68,    10,    -1,    -1,    -1,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    24,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,
      35,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    46,    -1,    -1,    49,    -1,    51,    -1,    53,    -1,
      -1,    56,    57,    58,    59,    -1,    61,    62,    63,    64,
      -1,    66,    67,    68
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    10,    15,    22,    23,    24,    34,    35,    36,
      37,    39,    45,    46,    49,    51,    53,    55,    56,    57,
      58,    59,    61,    62,    63,    64,    66,    67,    68,    96,
      97,    98,   100,   101,   102,   105,   106,   107,   108,   109,
     110,   111,   113,   115,   154,   155,   181,    71,   101,    32,
      69,   114,   181,   183,   101,    37,    61,    66,    67,     0,
      97,    71,   103,   104,   121,   122,   123,   124,   183,    46,
     106,   107,   108,   109,   110,   101,   101,   101,   101,   101,
      82,   112,   165,   112,    11,    13,    16,    18,    19,    25,
      26,    27,    28,    33,    38,    42,    47,    48,    60,    71,
      74,    81,    84,    85,    86,    87,    98,   100,   105,   132,
     133,   134,   135,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   156,   157,
     158,   159,   160,   161,   162,   163,   164,   165,   167,   168,
     169,   170,   171,   172,   177,   179,   182,   183,   184,    75,
      71,    72,    73,    82,    76,   118,    75,    78,    81,    99,
     100,   101,   126,   127,   128,   129,    98,   116,   117,   116,
      71,    71,    71,   105,   135,   153,   156,    81,    81,    81,
     139,   139,    71,   153,    81,    99,   101,   153,   139,   139,
     139,   139,   121,    81,     5,     6,     7,     8,     9,    42,
      47,    73,    78,    81,    83,    88,    89,    90,    84,    85,
      30,    41,    29,    40,    76,    77,    20,    43,    91,    92,
      93,     3,    44,    94,   156,   157,    74,   166,   167,    74,
     168,    71,    14,   180,   183,   104,    82,   130,   153,   119,
     180,    38,    79,   121,   125,    72,    80,    80,    72,    74,
     117,    74,    71,    60,   170,   174,   175,   153,   173,    71,
     173,    80,    80,   138,   153,   153,   153,   153,   153,   153,
     153,   153,   136,   137,   153,   178,   183,   140,   140,   140,
     141,   141,   142,   142,   143,   143,   143,   143,   144,   144,
     145,   146,   147,   148,   149,   153,    21,   166,   133,   183,
      74,   130,   131,   120,    79,    73,    78,   127,    99,    81,
      71,    72,    80,    80,   140,    72,    80,    79,    80,    72,
      75,   157,   158,    72,    74,    77,    98,   130,    38,    79,
     173,   173,   176,   170,   157,   158,   153,   153,   151,    74,
     130,    79,    80,    71,    71,   174,    80,   157,   158
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    95,    96,    96,    97,    97,    98,    98,    98,    99,
     100,   100,   101,   101,   101,   101,   101,   101,   101,   102,
     102,   102,   102,   102,   102,   102,   103,   103,   104,   104,
     105,   105,   105,   105,   105,   105,   105,   105,   105,   105,
     105,   105,   105,   105,   105,   105,   105,   105,   105,   106,
     107,   107,   108,   108,   109,   109,   110,   110,   110,   111,
     111,   111,   112,   113,   113,   114,   114,   115,   116,   116,
     117,   119,   118,   120,   120,   121,   121,   122,   122,   123,
     123,   123,   123,   123,   124,   125,   125,   125,   126,   126,
     127,   127,   128,   128,   129,   129,   130,   130,   130,   130,
     131,   131,   132,   132,   133,   134,   134,   134,   134,   135,
     135,   135,   135,   135,   135,   136,   136,   137,   137,   138,
     138,   139,   139,   139,   139,   139,   139,   139,   140,   140,
     141,   141,   141,   141,   142,   142,   142,   143,   143,   143,
     144,   144,   144,   144,   144,   145,   145,   145,   146,   146,
     147,   147,   148,   148,   149,   149,   150,   150,   151,   151,
     152,   153,   154,   154,   155,   156,   156,   157,   157,   157,
     157,   157,   157,   157,   158,   158,   159,   159,   160,   160,
     161,   162,   162,   163,   164,   164,   165,   166,   167,   167,
     168,   168,   169,   169,   170,   170,   170,   170,   170,   170,
     170,   171,   171,   171,   172,   172,   173,   174,   174,   175,
     175,   176,   176,   177,   177,   178,   179,   180,   181,   182,
     183,   183,   184,   184,   184,   184,   184
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     1,     1,     2,     3,     2,     2,
       1,     2,     1,     2,     2,     2,     2,     2,     2,     1,
       2,     2,     2,     2,     2,     2,     1,     3,     1,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     2,     2,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     4,
       4,     1,     1,     2,     4,     1,     1,     1,     1,     2,
       1,     0,     4,     0,     2,     1,     2,     1,     3,     1,
       4,     3,     3,     3,     2,     0,     4,     3,     1,     3,
       2,     4,     0,     1,     1,     3,     1,     3,     4,     2,
       1,     3,     1,     3,     1,     1,     1,     3,     4,     1,
       2,     2,     3,     4,     4,     0,     1,     1,     3,     1,
       3,     1,     2,     2,     2,     2,     2,     2,     1,     4,
       1,     3,     3,     3,     1,     3,     3,     1,     3,     3,
       1,     3,     3,     3,     3,     1,     3,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     5,
       1,     1,     3,     2,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     3,     2,     2,
       4,     2,     4,     4,     3,     2,     1,     1,     1,     2,
       1,     1,     2,     1,     3,     1,     3,     3,     3,     3,
       3,     5,     7,     9,     5,     9,     1,     1,     0,     1,
       3,     1,     0,     3,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 4: /* external_declaration: declaration  */
#line 267 "parser.y"
                              { (yyval.dummy) = GlobalInitStatements(CurrentScope, (yyvsp[0].sc_stmt)); }
#line 1893 "parser.c"
    break;

  case 6: /* declaration: declaration_specifiers ';'  */
#line 272 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 1899 "parser.c"
    break;

  case 7: /* declaration: declaration_specifiers init_declarator_list ';'  */
#line 274 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[-1].sc_stmt); }
#line 1905 "parser.c"
    break;

  case 8: /* declaration: ERROR_SY ';'  */
#line 276 "parser.y"
                              { RecordErrorPos(Cg->tokenLoc); (yyval.sc_stmt) = NULL; }
#line 1911 "parser.c"
    break;

  case 9: /* abstract_declaration: abstract_declaration_specifiers abstract_declarator  */
#line 280 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 1917 "parser.c"
    break;

  case 10: /* declaration_specifiers: abstract_declaration_specifiers  */
#line 287 "parser.y"
                              { (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 1923 "parser.c"
    break;

  case 11: /* declaration_specifiers: TYPEDEF_SY abstract_declaration_specifiers  */
#line 289 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, TYPE_MISC_TYPEDEF); (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 1929 "parser.c"
    break;

  case 12: /* abstract_declaration_specifiers: abstract_declaration_specifiers2  */
#line 294 "parser.y"
                              { (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 1935 "parser.c"
    break;

  case 13: /* abstract_declaration_specifiers: type_qualifier abstract_declaration_specifiers  */
#line 296 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1941 "parser.c"
    break;

  case 14: /* abstract_declaration_specifiers: storage_class abstract_declaration_specifiers  */
#line 298 "parser.y"
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1947 "parser.c"
    break;

  case 15: /* abstract_declaration_specifiers: type_domain abstract_declaration_specifiers  */
#line 300 "parser.y"
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1953 "parser.c"
    break;

  case 16: /* abstract_declaration_specifiers: in_out abstract_declaration_specifiers  */
#line 302 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1959 "parser.c"
    break;

  case 17: /* abstract_declaration_specifiers: function_specifier abstract_declaration_specifiers  */
#line 304 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1965 "parser.c"
    break;

  case 18: /* abstract_declaration_specifiers: PACKED_SY abstract_declaration_specifiers  */
#line 306 "parser.y"
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1971 "parser.c"
    break;

  case 19: /* abstract_declaration_specifiers2: type_specifier  */
#line 311 "parser.y"
                              { (yyval.sc_type) = *SetDType(&CurrentDeclTypeSpecs, (yyvsp[0].sc_ptype)); }
#line 1977 "parser.c"
    break;

  case 20: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 type_qualifier  */
#line 313 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1983 "parser.c"
    break;

  case 21: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 storage_class  */
#line 315 "parser.y"
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1989 "parser.c"
    break;

  case 22: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 type_domain  */
#line 317 "parser.y"
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1995 "parser.c"
    break;

  case 23: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 in_out  */
#line 319 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2001 "parser.c"
    break;

  case 24: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 function_specifier  */
#line 321 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2007 "parser.c"
    break;

  case 25: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 PACKED_SY  */
#line 323 "parser.y"
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2013 "parser.c"
    break;

  case 26: /* init_declarator_list: init_declarator  */
#line 327 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[0].sc_stmt); }
#line 2019 "parser.c"
    break;

  case 27: /* init_declarator_list: init_declarator_list ',' init_declarator  */
#line 329 "parser.y"
                              { (yyval.sc_stmt) = AddStmt((yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2025 "parser.c"
    break;

  case 28: /* init_declarator: declarator  */
#line 333 "parser.y"
                              { (yyval.sc_stmt) = Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_decl), NULL); }
#line 2031 "parser.c"
    break;

  case 29: /* init_declarator: declarator '=' initializer  */
#line 335 "parser.y"
                              { (yyval.sc_stmt) = Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[0].sc_expr)); }
#line 2037 "parser.c"
    break;

  case 30: /* type_specifier: INT_SY  */
#line 343 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, INT_SY); }
#line 2043 "parser.c"
    break;

  case 31: /* type_specifier: FLOAT_SY  */
#line 345 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, FLOAT_SY); }
#line 2049 "parser.c"
    break;

  case 32: /* type_specifier: VOID_SY  */
#line 347 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, VOID_SY); }
#line 2055 "parser.c"
    break;

  case 33: /* type_specifier: BOOLEAN_SY  */
#line 349 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, BOOLEAN_SY); }
#line 2061 "parser.c"
    break;

  case 34: /* type_specifier: TEXOBJ_SY  */
#line 351 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, TEXOBJ_SY); }
#line 2067 "parser.c"
    break;

  case 35: /* type_specifier: CHAR_SY  */
#line 353 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2073 "parser.c"
    break;

  case 36: /* type_specifier: SHORT_SY  */
#line 355 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2079 "parser.c"
    break;

  case 37: /* type_specifier: LONG_SY  */
#line 357 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2085 "parser.c"
    break;

  case 38: /* type_specifier: HALF_SY  */
#line 359 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2091 "parser.c"
    break;

  case 39: /* type_specifier: FIXED_SY  */
#line 361 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2097 "parser.c"
    break;

  case 40: /* type_specifier: DOUBLE_SY  */
#line 363 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2103 "parser.c"
    break;

  case 41: /* type_specifier: UNSIGNED_SY  */
#line 365 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2109 "parser.c"
    break;

  case 42: /* type_specifier: UNSIGNED_SY CHAR_SY  */
#line 367 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2115 "parser.c"
    break;

  case 43: /* type_specifier: UNSIGNED_SY SHORT_SY  */
#line 369 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2121 "parser.c"
    break;

  case 44: /* type_specifier: UNSIGNED_SY INT_SY  */
#line 371 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2127 "parser.c"
    break;

  case 45: /* type_specifier: UNSIGNED_SY LONG_SY  */
#line 373 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2133 "parser.c"
    break;

  case 46: /* type_specifier: struct_or_connector_specifier  */
#line 375 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2139 "parser.c"
    break;

  case 47: /* type_specifier: type_identifier  */
#line 377 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, (yyvsp[0].sc_ident)); }
#line 2145 "parser.c"
    break;

  case 48: /* type_specifier: error  */
#line 379 "parser.y"
                              {
                                SemanticParseError(Cg->tokenLoc, ERROR_S_TYPE_NAME_EXPECTED,
                                                   GetAtomString(atable, Cg->mostRecentToken /* yychar */));
                                (yyval.sc_ptype) = UndefinedType;
                              }
#line 2155 "parser.c"
    break;

  case 49: /* type_qualifier: CONST_SY  */
#line 391 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_CONST; }
#line 2161 "parser.c"
    break;

  case 50: /* type_domain: UNIFORM_SY  */
#line 399 "parser.y"
                              { (yyval.sc_int) = TYPE_DOMAIN_UNIFORM; }
#line 2167 "parser.c"
    break;

  case 51: /* type_domain: VARYING_SY  */
#line 401 "parser.y"
                              { (yyval.sc_int) = TYPE_DOMAIN_VARYING; }
#line 2173 "parser.c"
    break;

  case 52: /* storage_class: STATIC_SY  */
#line 409 "parser.y"
                              { (yyval.sc_int) = (int) SC_STATIC; }
#line 2179 "parser.c"
    break;

  case 53: /* storage_class: EXTERN_SY  */
#line 411 "parser.y"
                              { (yyval.sc_int) = (int) SC_EXTERN; }
#line 2185 "parser.c"
    break;

  case 54: /* function_specifier: INLINE_SY  */
#line 419 "parser.y"
                              { (yyval.sc_int) = TYPE_MISC_INLINE; }
#line 2191 "parser.c"
    break;

  case 55: /* function_specifier: INTERNAL_SY  */
#line 421 "parser.y"
                              { (yyval.sc_int) = TYPE_MISC_INTERNAL; }
#line 2197 "parser.c"
    break;

  case 56: /* in_out: IN_SY  */
#line 429 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_IN; }
#line 2203 "parser.c"
    break;

  case 57: /* in_out: OUT_SY  */
#line 431 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_OUT; }
#line 2209 "parser.c"
    break;

  case 58: /* in_out: INOUT_SY  */
#line 433 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_INOUT; }
#line 2215 "parser.c"
    break;

  case 59: /* struct_or_connector_specifier: struct_or_connector_header struct_compound_header struct_declaration_list '}'  */
#line 442 "parser.y"
                              { (yyval.sc_ptype) = SetStructMembers(Cg->tokenLoc, (yyvsp[-3].sc_ptype), PopScope()); }
#line 2221 "parser.c"
    break;

  case 60: /* struct_or_connector_specifier: untagged_struct_header struct_compound_header struct_declaration_list '}'  */
#line 444 "parser.y"
                              { (yyval.sc_ptype) = SetStructMembers(Cg->tokenLoc, (yyvsp[-3].sc_ptype), PopScope()); }
#line 2227 "parser.c"
    break;

  case 61: /* struct_or_connector_specifier: struct_or_connector_header  */
#line 446 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2233 "parser.c"
    break;

  case 62: /* struct_compound_header: compound_header  */
#line 450 "parser.y"
                              { CurrentScope->IsStructScope = 1; (yyval.dummy) = (yyvsp[0].dummy); }
#line 2239 "parser.c"
    break;

  case 63: /* struct_or_connector_header: STRUCT_SY struct_identifier  */
#line 455 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, 0, (yyvsp[0].sc_ident)); }
#line 2245 "parser.c"
    break;

  case 64: /* struct_or_connector_header: STRUCT_SY struct_identifier ':' semantics_identifier  */
#line 457 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_ident), (yyvsp[-2].sc_ident)); }
#line 2251 "parser.c"
    break;

  case 67: /* untagged_struct_header: STRUCT_SY  */
#line 465 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, 0, 0); }
#line 2257 "parser.c"
    break;

  case 70: /* struct_declaration: declaration  */
#line 473 "parser.y"
                            { (yyval.sc_stmt) = (yyvsp[0].sc_stmt); }
#line 2263 "parser.c"
    break;

  case 71: /* $@1: %empty  */
#line 491 "parser.y"
                              { PushScope(NewScope()); }
#line 2269 "parser.c"
    break;

  case 72: /* annotation: '<' $@1 annotation_decl_list '>'  */
#line 492 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[-1].sc_stmt); PopScope(); }
#line 2275 "parser.c"
    break;

  case 73: /* annotation_decl_list: %empty  */
#line 496 "parser.y"
                              { (yyval.sc_stmt) = 0; }
#line 2281 "parser.c"
    break;

  case 75: /* declarator: semantic_declarator  */
#line 505 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2287 "parser.c"
    break;

  case 76: /* declarator: semantic_declarator annotation  */
#line 507 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[-1].sc_decl); }
#line 2293 "parser.c"
    break;

  case 77: /* semantic_declarator: basic_declarator  */
#line 511 "parser.y"
                              { (yyval.sc_decl) = Declarator(Cg->tokenLoc, (yyvsp[0].sc_decl), 0); }
#line 2299 "parser.c"
    break;

  case 78: /* semantic_declarator: basic_declarator ':' semantics_identifier  */
#line 513 "parser.y"
                              { (yyval.sc_decl) = Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), (yyvsp[0].sc_ident)); }
#line 2305 "parser.c"
    break;

  case 79: /* basic_declarator: identifier  */
#line 517 "parser.y"
                              { (yyval.sc_decl) = NewDeclNode(Cg->tokenLoc, (yyvsp[0].sc_ident), &CurrentDeclTypeSpecs); }
#line 2311 "parser.c"
    break;

  case 80: /* basic_declarator: basic_declarator '[' INTCONST_SY ']'  */
#line 519 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-3].sc_decl), (int) (yyvsp[-1].sc_literal).value.i, 0); }
#line 2317 "parser.c"
    break;

  case 81: /* basic_declarator: basic_declarator '[' ']'  */
#line 521 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), 0 , 1); }
#line 2323 "parser.c"
    break;

  case 82: /* basic_declarator: function_decl_header parameter_list ')'  */
#line 523 "parser.y"
                              { (yyval.sc_decl) = SetFunTypeParams(CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_decl), (yyvsp[-1].sc_decl)); }
#line 2329 "parser.c"
    break;

  case 83: /* basic_declarator: function_decl_header abstract_parameter_list ')'  */
#line 525 "parser.y"
                              { (yyval.sc_decl) = SetFunTypeParams(CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_decl), NULL); }
#line 2335 "parser.c"
    break;

  case 84: /* function_decl_header: basic_declarator '('  */
#line 529 "parser.y"
                              { (yyval.sc_decl) = FunctionDeclHeader(&(yyvsp[-1].sc_decl)->loc, CurrentScope, (yyvsp[-1].sc_decl)); }
#line 2341 "parser.c"
    break;

  case 85: /* abstract_declarator: %empty  */
#line 533 "parser.y"
                              { (yyval.sc_decl) = NewDeclNode(Cg->tokenLoc, 0, &CurrentDeclTypeSpecs); }
#line 2347 "parser.c"
    break;

  case 86: /* abstract_declarator: abstract_declarator '[' INTCONST_SY ']'  */
#line 535 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-3].sc_decl), (int) (yyvsp[-1].sc_literal).value.i, 0); }
#line 2353 "parser.c"
    break;

  case 87: /* abstract_declarator: abstract_declarator '[' ']'  */
#line 537 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), 0 , 1); }
#line 2359 "parser.c"
    break;

  case 88: /* parameter_list: parameter_declaration  */
#line 554 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2365 "parser.c"
    break;

  case 89: /* parameter_list: parameter_list ',' parameter_declaration  */
#line 556 "parser.y"
                              { (yyval.sc_decl) = AddDecl((yyvsp[-2].sc_decl), (yyvsp[0].sc_decl)); }
#line 2371 "parser.c"
    break;

  case 90: /* parameter_declaration: declaration_specifiers declarator  */
#line 560 "parser.y"
                              { (yyval.sc_decl) = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_decl), NULL); }
#line 2377 "parser.c"
    break;

  case 91: /* parameter_declaration: declaration_specifiers declarator '=' initializer  */
#line 562 "parser.y"
                              { (yyval.sc_decl) = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[0].sc_expr)); }
#line 2383 "parser.c"
    break;

  case 92: /* abstract_parameter_list: %empty  */
#line 566 "parser.y"
                              { (yyval.sc_decl) = NULL; }
#line 2389 "parser.c"
    break;

  case 94: /* non_empty_abstract_parameter_list: abstract_declaration  */
#line 571 "parser.y"
                              {
                                if (IsVoid(&(yyvsp[0].sc_decl)->type.type))
                                    CurrentScope->HasVoidParameter = 1;
                                (yyval.sc_decl) = (yyvsp[0].sc_decl);
                              }
#line 2399 "parser.c"
    break;

  case 95: /* non_empty_abstract_parameter_list: non_empty_abstract_parameter_list ',' abstract_declaration  */
#line 577 "parser.y"
                              {
                                if (CurrentScope->HasVoidParameter || IsVoid(&(yyvsp[-2].sc_decl)->type.type)) {
                                    SemanticError(Cg->tokenLoc, ERROR___VOID_NOT_ONLY_PARAM);
                                }
                                (yyval.sc_decl) = AddDecl((yyvsp[-2].sc_decl), (yyvsp[0].sc_decl));
                              }
#line 2410 "parser.c"
    break;

  case 96: /* initializer: expression  */
#line 590 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[0].sc_expr)); }
#line 2416 "parser.c"
    break;

  case 97: /* initializer: '{' initializer_list '}'  */
#line 592 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[-1].sc_expr)); }
#line 2422 "parser.c"
    break;

  case 98: /* initializer: '{' initializer_list ',' '}'  */
#line 594 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[-2].sc_expr)); }
#line 2428 "parser.c"
    break;

  case 99: /* initializer: '{' '}'  */
#line 600 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, NULL); }
#line 2434 "parser.c"
    break;

  case 100: /* initializer_list: initializer  */
#line 604 "parser.y"
                              { (yyval.sc_expr) = InitializerList(Cg->tokenLoc, (yyvsp[0].sc_expr), NULL); }
#line 2440 "parser.c"
    break;

  case 101: /* initializer_list: initializer_list ',' initializer  */
#line 606 "parser.y"
                              { (yyval.sc_expr) = InitializerList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2446 "parser.c"
    break;

  case 102: /* variable: basic_variable  */
#line 618 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[0].sc_expr); }
#line 2452 "parser.c"
    break;

  case 103: /* variable: scope_identifier COLONCOLON_SY basic_variable  */
#line 620 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[0].sc_expr); }
#line 2458 "parser.c"
    break;

  case 104: /* basic_variable: variable_identifier  */
#line 624 "parser.y"
                              { (yyval.sc_expr) = BasicVariable(Cg->tokenLoc, (yyvsp[0].sc_ident)); }
#line 2464 "parser.c"
    break;

  case 107: /* primary_expression: '(' expression ')'  */
#line 634 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[-1].sc_expr); }
#line 2470 "parser.c"
    break;

  case 108: /* primary_expression: type_specifier '(' expression_list ')'  */
#line 636 "parser.y"
                              { (yyval.sc_expr) = NewVectorConstructor(Cg->tokenLoc, (yyvsp[-3].sc_ptype), (yyvsp[-1].sc_expr)); }
#line 2476 "parser.c"
    break;

  case 110: /* postfix_expression: postfix_expression PLUSPLUS_SY  */
#line 645 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(POSTINC_OP, (yyvsp[-1].sc_expr)); }
#line 2482 "parser.c"
    break;

  case 111: /* postfix_expression: postfix_expression MINUSMINUS_SY  */
#line 647 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(POSTDEC_OP, (yyvsp[-1].sc_expr)); }
#line 2488 "parser.c"
    break;

  case 112: /* postfix_expression: postfix_expression '.' member_identifier  */
#line 649 "parser.y"
                              { (yyval.sc_expr) = NewMemberSelectorOrSwizzleOrWriteMaskOperator(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_ident)); }
#line 2494 "parser.c"
    break;

  case 113: /* postfix_expression: postfix_expression '[' expression ']'  */
#line 651 "parser.y"
                              { (yyval.sc_expr) = NewIndexOperator(Cg->tokenLoc, (yyvsp[-3].sc_expr), (yyvsp[-1].sc_expr)); }
#line 2500 "parser.c"
    break;

  case 114: /* postfix_expression: postfix_expression '(' actual_argument_list ')'  */
#line 653 "parser.y"
                              { (yyval.sc_expr) = NewFunctionCallOperator(Cg->tokenLoc, (yyvsp[-3].sc_expr), (yyvsp[-1].sc_expr)); }
#line 2506 "parser.c"
    break;

  case 115: /* actual_argument_list: %empty  */
#line 657 "parser.y"
                                { (yyval.sc_expr) = NULL; }
#line 2512 "parser.c"
    break;

  case 117: /* non_empty_argument_list: expression  */
#line 662 "parser.y"
                              { (yyval.sc_expr) = ArgumentList(Cg->tokenLoc, NULL, (yyvsp[0].sc_expr)); }
#line 2518 "parser.c"
    break;

  case 118: /* non_empty_argument_list: non_empty_argument_list ',' expression  */
#line 664 "parser.y"
                              { (yyval.sc_expr) = ArgumentList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2524 "parser.c"
    break;

  case 119: /* expression_list: expression  */
#line 668 "parser.y"
                              { (yyval.sc_expr) = ExpressionList(Cg->tokenLoc, NULL, (yyvsp[0].sc_expr)); }
#line 2530 "parser.c"
    break;

  case 120: /* expression_list: expression_list ',' expression  */
#line 670 "parser.y"
                              { (yyval.sc_expr) = ExpressionList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2536 "parser.c"
    break;

  case 122: /* unary_expression: PLUSPLUS_SY unary_expression  */
#line 679 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(PREINC_OP, (yyvsp[0].sc_expr)); }
#line 2542 "parser.c"
    break;

  case 123: /* unary_expression: MINUSMINUS_SY unary_expression  */
#line 681 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(PREDEC_OP, (yyvsp[0].sc_expr)); }
#line 2548 "parser.c"
    break;

  case 124: /* unary_expression: '+' unary_expression  */
#line 683 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, POS_OP, '+', (yyvsp[0].sc_expr), 0); }
#line 2554 "parser.c"
    break;

  case 125: /* unary_expression: '-' unary_expression  */
#line 685 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, NEG_OP, '-', (yyvsp[0].sc_expr), 0); }
#line 2560 "parser.c"
    break;

  case 126: /* unary_expression: '!' unary_expression  */
#line 687 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, BNOT_OP, '!', (yyvsp[0].sc_expr), 0); }
#line 2566 "parser.c"
    break;

  case 127: /* unary_expression: '~' unary_expression  */
#line 689 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, NOT_OP, '~', (yyvsp[0].sc_expr), 1); }
#line 2572 "parser.c"
    break;

  case 129: /* cast_expression: '(' abstract_declaration ')' cast_expression  */
#line 701 "parser.y"
                              { (yyval.sc_expr) = NewCastOperator(Cg->tokenLoc, (yyvsp[0].sc_expr), GetTypePointer(&(yyvsp[-2].sc_decl)->loc, &(yyvsp[-2].sc_decl)->type)); }
#line 2578 "parser.c"
    break;

  case 131: /* multiplicative_expression: multiplicative_expression '*' cast_expression  */
#line 710 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, MUL_OP, '*', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2584 "parser.c"
    break;

  case 132: /* multiplicative_expression: multiplicative_expression '/' cast_expression  */
#line 712 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, DIV_OP, '/', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2590 "parser.c"
    break;

  case 133: /* multiplicative_expression: multiplicative_expression '%' cast_expression  */
#line 714 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, MOD_OP, '%', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2596 "parser.c"
    break;

  case 135: /* additive_expression: additive_expression '+' multiplicative_expression  */
#line 723 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, ADD_OP, '+', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2602 "parser.c"
    break;

  case 136: /* additive_expression: additive_expression '-' multiplicative_expression  */
#line 725 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SUB_OP, '-', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2608 "parser.c"
    break;

  case 138: /* shift_expression: shift_expression LL_SY additive_expression  */
#line 734 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SHL_OP, LL_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2614 "parser.c"
    break;

  case 139: /* shift_expression: shift_expression GG_SY additive_expression  */
#line 736 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SHR_OP, GG_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2620 "parser.c"
    break;

  case 141: /* relational_expression: relational_expression '<' shift_expression  */
#line 745 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, LT_OP, '<', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2626 "parser.c"
    break;

  case 142: /* relational_expression: relational_expression '>' shift_expression  */
#line 747 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, GT_OP, '>', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2632 "parser.c"
    break;

  case 143: /* relational_expression: relational_expression LE_SY shift_expression  */
#line 749 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, LE_OP, LE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2638 "parser.c"
    break;

  case 144: /* relational_expression: relational_expression GE_SY shift_expression  */
#line 751 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, GE_OP, GE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2644 "parser.c"
    break;

  case 146: /* equality_expression: equality_expression EQ_SY relational_expression  */
#line 760 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, EQ_OP, EQ_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2650 "parser.c"
    break;

  case 147: /* equality_expression: equality_expression NE_SY relational_expression  */
#line 762 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, NE_OP, NE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2656 "parser.c"
    break;

  case 149: /* AND_expression: AND_expression '&' equality_expression  */
#line 771 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, AND_OP, '&', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2662 "parser.c"
    break;

  case 151: /* exclusive_OR_expression: exclusive_OR_expression '^' AND_expression  */
#line 780 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, XOR_OP, '^', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2668 "parser.c"
    break;

  case 153: /* inclusive_OR_expression: inclusive_OR_expression '|' exclusive_OR_expression  */
#line 789 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, OR_OP, '|', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2674 "parser.c"
    break;

  case 155: /* logical_AND_expression: logical_AND_expression AND_SY inclusive_OR_expression  */
#line 798 "parser.y"
                              { (yyval.sc_expr) = NewBinaryBooleanOperator(Cg->tokenLoc, BAND_OP, AND_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2680 "parser.c"
    break;

  case 157: /* logical_OR_expression: logical_OR_expression OR_SY logical_AND_expression  */
#line 807 "parser.y"
                              { (yyval.sc_expr) = NewBinaryBooleanOperator(Cg->tokenLoc, BOR_OP, OR_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2686 "parser.c"
    break;

  case 159: /* conditional_expression: conditional_test '?' expression ':' conditional_expression  */
#line 816 "parser.y"
                              { (yyval.sc_expr) = NewConditionalOperator(Cg->tokenLoc, (yyvsp[-4].sc_expr), (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2692 "parser.c"
    break;

  case 160: /* conditional_test: logical_OR_expression  */
#line 820 "parser.y"
                              {  (yyval.sc_expr) = CheckBooleanExpr(Cg->tokenLoc, (yyvsp[0].sc_expr), 1); }
#line 2698 "parser.c"
    break;

  case 162: /* function_definition: function_definition_header block_item_list '}'  */
#line 839 "parser.y"
                              { DefineFunction(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_stmt)); PopScope(); }
#line 2704 "parser.c"
    break;

  case 163: /* function_definition: function_definition_header '}'  */
#line 841 "parser.y"
                              { DefineFunction(Cg->tokenLoc, CurrentScope, (yyvsp[-1].sc_decl), NULL); PopScope(); }
#line 2710 "parser.c"
    break;

  case 164: /* function_definition_header: declaration_specifiers declarator '{'  */
#line 845 "parser.y"
                              { (yyval.sc_decl) = Function_Definition_Header(Cg->tokenLoc, (yyvsp[-1].sc_decl)); }
#line 2716 "parser.c"
    break;

  case 176: /* discard_statement: DISCARD_SY ';'  */
#line 874 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewDiscardStmt(Cg->tokenLoc, NULL); }
#line 2722 "parser.c"
    break;

  case 177: /* discard_statement: DISCARD_SY expression ';'  */
#line 876 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewDiscardStmt(Cg->tokenLoc, CheckBooleanExpr(Cg->tokenLoc, (yyvsp[-1].sc_expr), 1)); }
#line 2728 "parser.c"
    break;

  case 178: /* jump_statement: BREAK_SY ';'  */
#line 884 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewSimpleStmt(Cg->tokenLoc, BREAK_STMT); }
#line 2734 "parser.c"
    break;

  case 179: /* jump_statement: CONTINUE_SY ';'  */
#line 886 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewSimpleStmt(Cg->tokenLoc, CONTINUE_STMT); }
#line 2740 "parser.c"
    break;

  case 180: /* if_statement: if_header balanced_statement ELSE_SY balanced_statement  */
#line 894 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-3].sc_stmt), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2746 "parser.c"
    break;

  case 181: /* dangling_if: if_header statement  */
#line 898 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-1].sc_stmt), (yyvsp[0].sc_stmt), NULL); }
#line 2752 "parser.c"
    break;

  case 182: /* dangling_if: if_header balanced_statement ELSE_SY dangling_statement  */
#line 900 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-3].sc_stmt), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2758 "parser.c"
    break;

  case 183: /* if_header: IF_SY '(' boolean_scalar_expression ')'  */
#line 904 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewIfStmt(Cg->tokenLoc, (yyvsp[-1].sc_expr), NULL, NULL); ; }
#line 2764 "parser.c"
    break;

  case 184: /* compound_statement: compound_header block_item_list compound_tail  */
#line 912 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewBlockStmt(Cg->tokenLoc, (yyvsp[-1].sc_stmt)); }
#line 2770 "parser.c"
    break;

  case 185: /* compound_statement: compound_header compound_tail  */
#line 914 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2776 "parser.c"
    break;

  case 186: /* compound_header: '{'  */
#line 918 "parser.y"
                              { PushScope(NewScope()); CurrentScope->funindex = NextFunctionIndex; }
#line 2782 "parser.c"
    break;

  case 187: /* compound_tail: '}'  */
#line 922 "parser.y"
                              {
                                if (Cg->options.DumpParseTree)
                                    PrintScopeDeclarations();
                                PopScope();
                              }
#line 2792 "parser.c"
    break;

  case 189: /* block_item_list: block_item_list block_item  */
#line 931 "parser.y"
                              { (yyval.sc_stmt) = AddStmt((yyvsp[-1].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2798 "parser.c"
    break;

  case 191: /* block_item: statement  */
#line 936 "parser.y"
                              { (yyval.sc_stmt) = CheckStmt((yyvsp[0].sc_stmt)); }
#line 2804 "parser.c"
    break;

  case 193: /* expression_statement: ';'  */
#line 945 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2810 "parser.c"
    break;

  case 194: /* expression_statement2: postfix_expression '=' expression  */
#line 949 "parser.y"
                              { (yyval.sc_stmt) = NewSimpleAssignmentStmt(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2816 "parser.c"
    break;

  case 195: /* expression_statement2: expression  */
#line 951 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewExprStmt(Cg->tokenLoc, (yyvsp[0].sc_expr)); }
#line 2822 "parser.c"
    break;

  case 196: /* expression_statement2: postfix_expression ASSIGNMINUS_SY expression  */
#line 953 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMINUS_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2828 "parser.c"
    break;

  case 197: /* expression_statement2: postfix_expression ASSIGNMOD_SY expression  */
#line 955 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMOD_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2834 "parser.c"
    break;

  case 198: /* expression_statement2: postfix_expression ASSIGNPLUS_SY expression  */
#line 957 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNPLUS_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2840 "parser.c"
    break;

  case 199: /* expression_statement2: postfix_expression ASSIGNSLASH_SY expression  */
#line 959 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSLASH_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2846 "parser.c"
    break;

  case 200: /* expression_statement2: postfix_expression ASSIGNSTAR_SY expression  */
#line 961 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSTAR_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2852 "parser.c"
    break;

  case 201: /* iteration_statement: WHILE_SY '(' boolean_scalar_expression ')' balanced_statement  */
#line 969 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, (yyvsp[-2].sc_expr), (yyvsp[0].sc_stmt)); }
#line 2858 "parser.c"
    break;

  case 202: /* iteration_statement: DO_SY statement WHILE_SY '(' boolean_scalar_expression ')' ';'  */
#line 971 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, DO_STMT, (yyvsp[-2].sc_expr), (yyvsp[-5].sc_stmt)); }
#line 2864 "parser.c"
    break;

  case 203: /* iteration_statement: FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' balanced_statement  */
#line 973 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewForStmt(Cg->tokenLoc, (yyvsp[-6].sc_stmt), (yyvsp[-4].sc_expr), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2870 "parser.c"
    break;

  case 204: /* dangling_iteration: WHILE_SY '(' boolean_scalar_expression ')' dangling_statement  */
#line 977 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, (yyvsp[-2].sc_expr), (yyvsp[0].sc_stmt)); }
#line 2876 "parser.c"
    break;

  case 205: /* dangling_iteration: FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' dangling_statement  */
#line 979 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewForStmt(Cg->tokenLoc, (yyvsp[-6].sc_stmt), (yyvsp[-4].sc_expr), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2882 "parser.c"
    break;

  case 206: /* boolean_scalar_expression: expression  */
#line 984 "parser.y"
                              {  (yyval.sc_expr) = CheckBooleanExpr(Cg->tokenLoc, (yyvsp[0].sc_expr), 0); }
#line 2888 "parser.c"
    break;

  case 208: /* for_expression_opt: %empty  */
#line 989 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2894 "parser.c"
    break;

  case 210: /* for_expression: for_expression ',' expression_statement2  */
#line 994 "parser.y"
                              {
                                stmt *lstmt = (yyvsp[-2].sc_stmt);
                                if (lstmt) {
                                    while (lstmt->exprst.next)
                                        lstmt = lstmt->exprst.next;
                                    lstmt->exprst.next = (yyvsp[0].sc_stmt);
                                    (yyval.sc_stmt) = (yyvsp[-2].sc_stmt);
                                } else {
                                    (yyval.sc_stmt) = (yyvsp[0].sc_stmt);
                                }
                              }
#line 2910 "parser.c"
    break;

  case 212: /* boolean_expression_opt: %empty  */
#line 1009 "parser.y"
                              { (yyval.sc_expr) = NULL; }
#line 2916 "parser.c"
    break;

  case 213: /* return_statement: RETURN_SY expression ';'  */
#line 1017 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, (yyvsp[-1].sc_expr)); }
#line 2922 "parser.c"
    break;

  case 214: /* return_statement: RETURN_SY ';'  */
#line 1019 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, NULL); }
#line 2928 "parser.c"
    break;

  case 220: /* identifier: IDENT_SY  */
#line 1042 "parser.y"
                              { (yyval.sc_ident) = (yyvsp[0].sc_ident); }
#line 2934 "parser.c"
    break;

  case 221: /* identifier: RESERVED_SY  */
#line 1044 "parser.y"
                              {
                                /* SemanticError, not SemanticParseError: the
                                 * latter is gated by AllowSemanticParseErrors */
                                SemanticError(Cg->tokenLoc, ERROR_S_RESERVED_WORD,
                                              GetAtomString(atable, (yyvsp[0].sc_token)));
                                (yyval.sc_ident) = (yyvsp[0].sc_token);
                              }
#line 2946 "parser.c"
    break;

  case 222: /* constant: INTCONST_SY  */
#line 1054 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(ICONST_OP, &(yyvsp[0].sc_literal)); }
#line 2952 "parser.c"
    break;

  case 223: /* constant: CFLOATCONST_SY  */
#line 1056 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 2958 "parser.c"
    break;

  case 224: /* constant: FLOATCONST_SY  */
#line 1058 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 2964 "parser.c"
    break;

  case 225: /* constant: FLOATHCONST_SY  */
#line 1060 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 2970 "parser.c"
    break;

  case 226: /* constant: FLOATXCONST_SY  */
#line 1062 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 2976 "parser.c"
    break;


#line 2980 "parser.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 1076 "parser.y"


