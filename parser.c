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
  YYSYMBOL_interface_specifier = 118,      /* interface_specifier  */
  YYSYMBOL_interface_compound_header = 119, /* interface_compound_header  */
  YYSYMBOL_interface_member_declaration_list = 120, /* interface_member_declaration_list  */
  YYSYMBOL_interface_member_declaration = 121, /* interface_member_declaration  */
  YYSYMBOL_annotation = 122,               /* annotation  */
  YYSYMBOL_123_1 = 123,                    /* $@1  */
  YYSYMBOL_annotation_decl_list = 124,     /* annotation_decl_list  */
  YYSYMBOL_declarator = 125,               /* declarator  */
  YYSYMBOL_semantic_declarator = 126,      /* semantic_declarator  */
  YYSYMBOL_basic_declarator = 127,         /* basic_declarator  */
  YYSYMBOL_function_decl_header = 128,     /* function_decl_header  */
  YYSYMBOL_abstract_declarator = 129,      /* abstract_declarator  */
  YYSYMBOL_parameter_list = 130,           /* parameter_list  */
  YYSYMBOL_parameter_declaration = 131,    /* parameter_declaration  */
  YYSYMBOL_abstract_parameter_list = 132,  /* abstract_parameter_list  */
  YYSYMBOL_non_empty_abstract_parameter_list = 133, /* non_empty_abstract_parameter_list  */
  YYSYMBOL_initializer = 134,              /* initializer  */
  YYSYMBOL_initializer_list = 135,         /* initializer_list  */
  YYSYMBOL_variable = 136,                 /* variable  */
  YYSYMBOL_basic_variable = 137,           /* basic_variable  */
  YYSYMBOL_primary_expression = 138,       /* primary_expression  */
  YYSYMBOL_postfix_expression = 139,       /* postfix_expression  */
  YYSYMBOL_actual_argument_list = 140,     /* actual_argument_list  */
  YYSYMBOL_non_empty_argument_list = 141,  /* non_empty_argument_list  */
  YYSYMBOL_expression_list = 142,          /* expression_list  */
  YYSYMBOL_unary_expression = 143,         /* unary_expression  */
  YYSYMBOL_cast_expression = 144,          /* cast_expression  */
  YYSYMBOL_multiplicative_expression = 145, /* multiplicative_expression  */
  YYSYMBOL_additive_expression = 146,      /* additive_expression  */
  YYSYMBOL_shift_expression = 147,         /* shift_expression  */
  YYSYMBOL_relational_expression = 148,    /* relational_expression  */
  YYSYMBOL_equality_expression = 149,      /* equality_expression  */
  YYSYMBOL_AND_expression = 150,           /* AND_expression  */
  YYSYMBOL_exclusive_OR_expression = 151,  /* exclusive_OR_expression  */
  YYSYMBOL_inclusive_OR_expression = 152,  /* inclusive_OR_expression  */
  YYSYMBOL_logical_AND_expression = 153,   /* logical_AND_expression  */
  YYSYMBOL_logical_OR_expression = 154,    /* logical_OR_expression  */
  YYSYMBOL_conditional_expression = 155,   /* conditional_expression  */
  YYSYMBOL_conditional_test = 156,         /* conditional_test  */
  YYSYMBOL_expression = 157,               /* expression  */
  YYSYMBOL_function_definition = 158,      /* function_definition  */
  YYSYMBOL_function_definition_header = 159, /* function_definition_header  */
  YYSYMBOL_statement = 160,                /* statement  */
  YYSYMBOL_balanced_statement = 161,       /* balanced_statement  */
  YYSYMBOL_dangling_statement = 162,       /* dangling_statement  */
  YYSYMBOL_discard_statement = 163,        /* discard_statement  */
  YYSYMBOL_jump_statement = 164,           /* jump_statement  */
  YYSYMBOL_if_statement = 165,             /* if_statement  */
  YYSYMBOL_dangling_if = 166,              /* dangling_if  */
  YYSYMBOL_if_header = 167,                /* if_header  */
  YYSYMBOL_compound_statement = 168,       /* compound_statement  */
  YYSYMBOL_compound_header = 169,          /* compound_header  */
  YYSYMBOL_compound_tail = 170,            /* compound_tail  */
  YYSYMBOL_block_item_list = 171,          /* block_item_list  */
  YYSYMBOL_block_item = 172,               /* block_item  */
  YYSYMBOL_expression_statement = 173,     /* expression_statement  */
  YYSYMBOL_expression_statement2 = 174,    /* expression_statement2  */
  YYSYMBOL_iteration_statement = 175,      /* iteration_statement  */
  YYSYMBOL_dangling_iteration = 176,       /* dangling_iteration  */
  YYSYMBOL_boolean_scalar_expression = 177, /* boolean_scalar_expression  */
  YYSYMBOL_for_expression_opt = 178,       /* for_expression_opt  */
  YYSYMBOL_for_expression = 179,           /* for_expression  */
  YYSYMBOL_boolean_expression_opt = 180,   /* boolean_expression_opt  */
  YYSYMBOL_return_statement = 181,         /* return_statement  */
  YYSYMBOL_member_identifier = 182,        /* member_identifier  */
  YYSYMBOL_scope_identifier = 183,         /* scope_identifier  */
  YYSYMBOL_semantics_identifier = 184,     /* semantics_identifier  */
  YYSYMBOL_type_identifier = 185,          /* type_identifier  */
  YYSYMBOL_variable_identifier = 186,      /* variable_identifier  */
  YYSYMBOL_identifier = 187,               /* identifier  */
  YYSYMBOL_constant = 188                  /* constant  */
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
#define YYFINAL  62
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2032

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  95
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  94
/* YYNRULES -- Number of rules.  */
#define YYNRULES  234
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  363

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
       0,   262,   262,   263,   270,   272,   275,   277,   279,   283,
     290,   292,   297,   299,   301,   303,   305,   307,   309,   314,
     316,   318,   320,   322,   324,   326,   330,   332,   336,   338,
     346,   348,   350,   352,   354,   356,   358,   360,   362,   364,
     366,   368,   370,   372,   374,   376,   378,   380,   382,   384,
     396,   404,   406,   414,   416,   424,   426,   434,   436,   438,
     447,   450,   453,   457,   462,   464,   466,   470,   471,   474,
     478,   479,   482,   484,   493,   502,   511,   512,   516,   538,
     538,   543,   544,   551,   553,   557,   559,   563,   565,   567,
     569,   571,   575,   580,   581,   583,   600,   602,   606,   608,
     613,   614,   617,   623,   636,   638,   640,   642,   650,   652,
     664,   666,   670,   678,   679,   680,   682,   690,   691,   693,
     695,   697,   699,   704,   705,   708,   710,   714,   716,   724,
     725,   727,   729,   731,   733,   735,   743,   747,   755,   756,
     758,   760,   768,   769,   771,   779,   780,   782,   790,   791,
     793,   795,   797,   805,   806,   808,   816,   817,   825,   826,
     834,   835,   843,   844,   852,   853,   861,   862,   866,   874,
     885,   888,   893,   901,   902,   905,   906,   907,   908,   909,
     910,   911,   914,   915,   922,   924,   932,   934,   942,   946,
     948,   952,   960,   962,   966,   970,   978,   979,   983,   984,
     992,   993,   997,   999,  1001,  1003,  1005,  1007,  1009,  1017,
    1019,  1021,  1025,  1027,  1032,  1036,  1038,  1041,  1042,  1056,
    1058,  1065,  1067,  1075,  1078,  1081,  1084,  1087,  1090,  1092,
    1102,  1104,  1106,  1108,  1110
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
  "struct_declaration", "interface_specifier", "interface_compound_header",
  "interface_member_declaration_list", "interface_member_declaration",
  "annotation", "$@1", "annotation_decl_list", "declarator",
  "semantic_declarator", "basic_declarator", "function_decl_header",
  "abstract_declarator", "parameter_list", "parameter_declaration",
  "abstract_parameter_list", "non_empty_abstract_parameter_list",
  "initializer", "initializer_list", "variable", "basic_variable",
  "primary_expression", "postfix_expression", "actual_argument_list",
  "non_empty_argument_list", "expression_list", "unary_expression",
  "cast_expression", "multiplicative_expression", "additive_expression",
  "shift_expression", "relational_expression", "equality_expression",
  "AND_expression", "exclusive_OR_expression", "inclusive_OR_expression",
  "logical_AND_expression", "logical_OR_expression",
  "conditional_expression", "conditional_test", "expression",
  "function_definition", "function_definition_header", "statement",
  "balanced_statement", "dangling_statement", "discard_statement",
  "jump_statement", "if_statement", "dangling_if", "if_header",
  "compound_statement", "compound_header", "compound_tail",
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

#define YYPACT_NINF (-284)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-225)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    1846,  -284,  -284,  -284,    -5,  -284,  -284,  -284,  -284,  -284,
    -284,  -284,  -284,  1964,  -284,    22,  -284,  1964,  -284,  -284,
    -284,  -284,  -284,  -284,  -284,  -284,    22,  -284,  -284,    52,
    1787,  -284,  -284,   -11,  -284,   172,  -284,  1964,  1964,  1964,
    1964,  1964,  -284,   -19,   -19,  -284,  -284,   308,  -284,  -284,
    -284,  -284,  -284,    24,  -284,  -284,  -284,   -19,  -284,  -284,
    -284,  -284,  -284,  -284,  -284,    37,  -284,   -40,    41,    74,
    1464,  -284,  -284,  -284,  -284,  -284,  -284,  -284,  -284,  -284,
    -284,  -284,  -284,  -284,  1846,  -284,  1846,    32,  -284,    34,
     838,   620,  -284,  -284,  -284,    64,    67,  -284,  1322,  1322,
     907,    69,  -284,  -284,   542,  1322,  1322,  1322,  1322,  -284,
     -11,    73,  -284,  -284,  -284,   154,  -284,  -284,    46,    43,
      27,    -4,    45,    65,    66,    76,   171,   -25,  -284,    88,
    -284,  -284,  -284,  -284,  -284,  -284,  -284,  -284,   620,  -284,
     386,   464,  -284,  -284,   115,  -284,  -284,  -284,   174,  -284,
     178,  -284,    22,  1905,  -284,  -284,     2,   976,  -284,  -284,
    -284,     2,    -3,  -284,  -284,     2,    13,   -16,  -284,   113,
     126,  -284,  1582,  -284,  -284,  1650,  -284,  -284,  -284,    73,
      33,   128,   142,  1046,  1391,  1391,  -284,  -284,  -284,   132,
    1391,   124,  -284,   125,  -284,  -284,  -284,  -284,   136,  1391,
    1391,  1391,  1391,  1391,  1391,  -284,  -284,  1391,  1391,  1115,
       2,  1391,  1391,  1391,  1391,  1391,  1391,  1391,  1391,  1391,
    1391,  1391,  1391,  1391,  1391,  1391,  1391,  1391,  1391,  1391,
    -284,   189,  -284,  -284,   386,  -284,  -284,  -284,     2,  -284,
    -284,  -284,     2,  1718,  -284,  -284,   698,  -284,  -284,  -284,
    -284,   134,  -284,   146,   148,  1905,  -284,  -284,  1964,  -284,
    -284,  -284,  -284,   139,  -284,   157,   161,  -284,   156,  -284,
     158,  1391,  -284,    20,  -284,  -284,  -284,  -284,  -284,  -284,
    -284,   162,   166,   170,  -284,  -284,  -284,  -284,  -284,  -284,
      46,    46,    43,    43,    27,    27,    27,    27,    -4,    -4,
      45,    65,    66,    76,   171,   173,   620,  -284,  -284,  -284,
     176,  -284,  -284,  -284,  -284,   -13,  1523,  -284,   976,    15,
    -284,  -284,  1391,  1184,  1391,  -284,   620,  -284,  1391,  -284,
    -284,  -284,  1391,  1391,  -284,  -284,  -284,   768,  -284,  -284,
    -284,  -284,   177,  -284,   169,  -284,   180,  -284,  -284,  -284,
    -284,  -284,  -284,  -284,  -284,  -284,   181,  1253,  -284,   182,
     620,  -284,  -284
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    49,    33,    50,     0,    54,    31,    57,    55,    59,
      30,    56,    58,     0,    53,    69,    34,     0,   226,    51,
      52,    32,    35,    40,    39,    38,     0,    37,    36,    41,
       0,     2,     4,     0,    10,    12,    19,     0,     0,     0,
       0,     0,    46,    62,     0,    47,     5,     0,    48,     8,
      18,   228,   229,    64,    68,    67,    11,     0,    44,    42,
      45,    43,     1,     3,     6,     0,    26,    28,    83,    85,
       0,    87,    25,    20,    22,    21,    24,    23,    13,    15,
      14,    17,    16,   194,     0,    63,     0,     0,   231,     0,
       0,     0,   232,   233,   234,     0,     0,   230,     0,     0,
       0,     0,   201,   171,     0,     0,     0,     0,     0,   198,
       0,    19,   113,   110,   117,   129,   136,   138,   142,   145,
     148,   153,   156,   158,   160,   162,   164,   166,   169,     0,
     203,   199,   173,   174,   176,   180,   179,   182,     0,   175,
       0,     0,   196,   177,     0,   178,   183,   181,     0,   112,
     227,   114,     0,     0,    75,     7,     0,     0,   172,    79,
      84,     0,     0,    92,   102,     0,    93,     0,    96,     0,
     101,    72,     0,    70,    73,     0,   186,   187,   184,     0,
     129,     0,     0,     0,     0,     0,   131,   130,   222,     0,
       0,     0,    93,     0,   132,   133,   134,   135,    28,     0,
       0,     0,     0,     0,     0,   119,   118,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     189,   173,   195,   193,     0,   170,   197,   200,     0,    65,
      66,   225,     0,     0,    76,    27,     0,    29,   104,    81,
      86,     0,    89,    98,     9,     0,    90,    91,     0,    60,
      71,    61,   185,     0,   217,     0,   215,   214,     0,   221,
       0,     0,   115,     0,   127,   204,   205,   206,   207,   208,
     202,     0,     0,   124,   125,   120,   223,   139,   140,   141,
     143,   144,   147,   146,   152,   151,   149,   150,   154,   155,
     157,   159,   161,   163,   165,     0,     0,   192,   111,   227,
       0,    74,    77,   107,   108,     0,     0,    88,     0,     0,
      97,   103,     0,     0,     0,   191,     0,   137,     0,   116,
     121,   122,     0,     0,   188,   190,    78,     0,   105,    80,
      82,    99,     0,    95,     0,   219,     0,   218,   209,   212,
     128,   126,   167,   106,   109,    94,     0,     0,   210,     0,
       0,   211,   213
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -284,  -284,   223,     1,  -101,   -43,    11,  -284,  -284,   101,
       0,   225,   228,   229,   230,   231,  -284,   224,  -284,   244,
    -284,   186,  -143,  -284,  -284,  -284,    31,  -284,  -284,  -284,
     -28,  -284,  -284,  -284,  -284,  -284,    21,  -284,  -284,  -235,
    -284,  -284,    39,  -284,   -45,  -284,  -284,  -284,   117,  -197,
     -44,   -38,   -54,   -39,    51,    53,    54,    55,    56,  -284,
     -50,  -284,   -78,    90,  -284,   -71,  -129,  -283,  -284,  -284,
    -284,  -284,  -284,  -284,    89,    57,   145,  -133,  -284,  -177,
    -284,  -284,  -180,   -70,  -284,  -284,  -284,  -284,  -284,   127,
      -8,  -284,    29,  -284
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    30,    31,   109,   164,    33,    34,    35,    65,    66,
     179,    37,    38,    39,    40,    41,    42,    84,    43,    53,
      44,   172,   173,    45,   153,   243,   244,   160,   249,   316,
     198,    68,    69,    70,   254,   167,   168,   169,   170,   247,
     315,   112,   113,   114,   180,   282,   283,   273,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   174,    47,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   233,   141,   142,   143,   144,
     145,   146,   268,   265,   266,   346,   147,   285,   148,   239,
      48,   149,   150,   151
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      36,    32,   115,   191,   110,    67,   264,    54,   236,   231,
     270,   314,   181,    36,   287,   288,   289,    36,    54,   228,
     182,    51,   189,   335,    50,   218,   193,   165,    56,   260,
      36,    32,   260,   157,    51,   251,   219,    36,    36,    36,
      36,    36,   158,   349,    55,   -10,   115,   111,    78,    79,
      80,    81,    82,   342,    51,    55,   255,   216,    52,   337,
      64,   338,    71,    83,   256,   222,    49,   230,   217,  -168,
      36,    52,   220,   221,   327,   205,   252,   362,    18,   248,
     206,   166,   -10,   341,    36,   171,    36,   171,   223,    58,
      46,    52,   328,   115,   343,   115,   115,   110,   110,   152,
     329,   236,   354,   176,   111,   177,   267,   193,   155,   156,
     242,   208,   267,    59,   209,   192,   210,   159,    60,    61,
      46,   274,   275,   276,   277,   278,   279,   214,   215,   280,
     281,   284,    85,    85,   211,   212,   213,   253,   115,    71,
     111,   111,   344,   345,   240,   183,   154,   347,   184,   161,
     190,   305,   162,    36,   199,   163,   224,   321,   225,   200,
     201,   202,   203,   204,   294,   295,   296,   297,   248,   226,
     290,   291,    36,   171,   227,    36,   171,   334,   292,   293,
     264,   241,   229,   298,   299,    71,   237,     3,   238,   115,
     241,   110,  -224,   257,    71,     5,   205,   348,   258,   262,
     242,   206,   263,   269,   271,   272,     7,     8,     9,   157,
     306,    11,   165,   317,   310,   186,   187,    12,    72,   318,
     322,    14,   194,   195,   196,   197,   319,   207,   323,    19,
      20,   361,   208,   324,   111,   209,   325,   210,   326,   286,
     248,   330,   332,    36,   267,   267,   331,   336,   333,   356,
     350,   357,   358,    63,   351,    36,   355,   245,    36,   248,
      73,   115,   360,    74,    75,    76,    77,   309,    86,   192,
      57,    71,   175,   110,   312,   300,   320,   308,   301,   115,
     302,   115,   303,   352,   304,   234,     0,   359,   250,     0,
       0,   307,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     1,
       0,     0,   115,     0,     0,   115,    36,   340,     2,    87,
       0,    88,     0,     3,    89,     0,    90,    91,     0,     0,
       4,     5,     6,    92,    93,    94,    95,     0,     0,     0,
      51,    96,     7,     8,     9,    10,    97,    11,     0,     0,
      98,     0,     0,    12,    13,    99,   100,    14,     0,    15,
       0,    16,     0,    17,    18,    19,    20,    21,   101,    22,
      23,    24,    25,    26,    27,    28,    29,    52,     0,   102,
       0,     0,   103,     0,     0,     0,     0,     1,     0,   104,
      83,     0,   105,   106,   107,   108,     2,    87,     0,    88,
       0,     3,    89,     0,    90,    91,     0,     0,     4,     5,
       6,    92,    93,    94,    95,     0,     0,     0,    51,    96,
       7,     8,     9,    10,    97,    11,     0,     0,    98,     0,
       0,    12,    13,    99,   100,    14,     0,    15,     0,    16,
       0,    17,    18,    19,    20,    21,   101,    22,    23,    24,
      25,    26,    27,    28,    29,    52,     0,   102,     0,     0,
     232,     0,     0,     0,     0,     1,     0,   104,    83,     0,
     105,   106,   107,   108,     2,    87,     0,    88,     0,     3,
      89,     0,    90,    91,     0,     0,     4,     5,     6,    92,
      93,    94,    95,     0,     0,     0,    51,    96,     7,     8,
       9,    10,    97,    11,     0,     0,    98,     0,     0,    12,
      13,    99,   100,    14,     0,    15,     0,    16,     0,    17,
      18,    19,    20,    21,   101,    22,    23,    24,    25,    26,
      27,    28,    29,    52,     0,   102,     0,     0,   235,     0,
       0,     0,     0,     1,     0,   104,    83,     0,   105,   106,
     107,   108,     2,     0,     0,    88,     0,     3,     0,     0,
       0,     0,     0,     0,     0,     5,     6,    92,    93,    94,
       0,     0,     0,     0,    51,     0,     7,     8,     9,    10,
      97,    11,     0,     0,    98,     0,     0,    12,    13,    99,
       0,    14,     0,    15,     0,    16,     0,     0,    18,    19,
      20,    21,     0,    22,    23,    24,    25,    26,    27,    28,
      29,    52,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     1,     0,   104,     0,     0,   105,   106,   107,   108,
       2,    87,     0,    88,     0,     0,    89,     0,    90,    91,
       0,     0,     0,     0,     6,    92,    93,    94,    95,     0,
       0,     0,    51,    96,     0,     0,     0,    10,    97,     0,
       0,     0,    98,     0,     0,     0,     0,    99,   100,     0,
       0,    15,     0,    16,     0,     0,    18,     0,     0,    21,
     101,    22,    23,    24,    25,    26,    27,    28,    29,    52,
       0,   102,     0,     0,     0,     0,     0,     0,     0,     1,
       0,   104,    83,     0,   105,   106,   107,   108,     2,     0,
       0,    88,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     6,    92,    93,    94,     0,     0,     0,     0,
      51,     0,     0,     0,     0,    10,    97,     0,     0,     0,
      98,     0,     0,     0,     0,    99,     0,     0,     0,    15,
       0,    16,     0,     0,    18,     0,     0,    21,     0,    22,
      23,    24,    25,    26,    27,    28,    29,    52,     0,     1,
       0,     0,   313,     0,     0,     0,     0,     0,     2,   104,
     246,    88,   105,   106,   107,   108,     0,     0,     0,     0,
       0,     0,     6,    92,    93,    94,     0,     0,     0,     0,
      51,     0,     0,     0,     0,    10,    97,     0,     0,     0,
      98,     0,     0,     0,     0,    99,     0,     0,     0,    15,
       0,    16,     0,     0,    18,     0,     0,    21,     0,    22,
      23,    24,    25,    26,    27,    28,    29,    52,     0,     1,
       0,     0,   353,     0,     0,     0,     0,     0,     2,   104,
     246,    88,   105,   106,   107,   108,     0,     0,     0,     0,
       0,     0,     6,    92,    93,    94,     0,     0,     0,     0,
      51,     0,     0,     0,     0,    10,    97,     0,     0,     0,
      98,     0,     0,     0,     0,    99,     0,     0,     0,    15,
       0,    16,     0,     0,    18,     0,     0,    21,     0,    22,
      23,    24,    25,    26,    27,    28,    29,    52,     1,   178,
       0,     0,     0,     0,     0,     0,     0,     2,     0,   104,
      88,     0,   105,   106,   107,   108,     0,     0,     0,     0,
       0,     6,    92,    93,    94,     0,     0,     0,     0,    51,
       0,     0,     0,     0,    10,    97,     0,     0,     0,    98,
       0,     0,     0,     0,    99,     0,     0,     0,    15,     0,
      16,     0,     0,    18,     0,     0,    21,     0,    22,    23,
      24,    25,    26,    27,    28,    29,    52,     1,   188,     0,
       0,     0,     0,     0,     0,     0,     2,     0,   104,    88,
       0,   105,   106,   107,   108,     0,     0,     0,     0,     0,
       6,    92,    93,    94,     0,     0,     0,     0,    51,     0,
       0,     0,     0,    10,    97,     0,     0,     0,    98,     0,
       0,     0,     0,    99,     0,     0,     0,    15,     0,    16,
       0,     0,    18,     0,     0,    21,     0,    22,    23,    24,
      25,    26,    27,    28,    29,    52,     0,     1,     0,     0,
       0,     0,     0,     0,     0,     0,     2,   104,   246,    88,
     105,   106,   107,   108,     0,     0,     0,     0,     0,     0,
       6,    92,    93,    94,     0,     0,     0,     0,    51,     0,
       0,     0,     0,    10,    97,     0,     0,     0,    98,     0,
       0,     0,     0,    99,     0,     0,     0,    15,     0,    16,
       0,     0,    18,     0,     0,    21,     0,    22,    23,    24,
      25,    26,    27,    28,    29,    52,     1,  -216,     0,     0,
       0,     0,     0,     0,     0,     2,     0,   104,    88,     0,
     105,   106,   107,   108,     0,     0,     0,     0,     0,     6,
      92,    93,    94,     0,     0,     0,     0,    51,     0,     0,
       0,     0,    10,    97,     0,     0,     0,    98,     0,     0,
       0,     0,    99,     0,     0,     0,    15,     0,    16,     0,
       0,    18,     0,     0,    21,     0,    22,    23,    24,    25,
      26,    27,    28,    29,    52,     1,     0,     0,     0,     0,
       0,     0,     0,     0,     2,  -123,   104,    88,     0,   105,
     106,   107,   108,     0,     0,     0,     0,     0,     6,    92,
      93,    94,     0,     0,     0,     0,    51,     0,     0,     0,
       0,    10,    97,     0,     0,     0,    98,     0,     0,     0,
       0,    99,     0,     0,     0,    15,     0,    16,     0,     0,
      18,     0,     0,    21,     0,    22,    23,    24,    25,    26,
      27,    28,    29,    52,     1,  -220,     0,     0,     0,     0,
       0,     0,     0,     2,     0,   104,    88,     0,   105,   106,
     107,   108,     0,     0,     0,     0,     0,     6,    92,    93,
      94,     0,     0,     0,     0,    51,     0,     0,     0,     0,
      10,    97,     0,     0,     0,    98,     0,     0,     0,     0,
      99,     0,     0,     0,    15,     0,    16,     0,     0,    18,
       0,     0,    21,     0,    22,    23,    24,    25,    26,    27,
      28,    29,    52,     1,     0,     0,     0,     0,     0,     0,
       0,     0,     2,  -216,   104,    88,     0,   105,   106,   107,
     108,     0,     0,     0,     0,     0,     6,    92,    93,    94,
       0,     0,     0,     0,    51,     0,     0,     0,     0,    10,
      97,     0,     0,     0,    98,     0,     0,     0,     0,    99,
       0,     0,     0,    15,     0,    16,     0,     0,    18,     0,
       0,    21,     0,    22,    23,    24,    25,    26,    27,    28,
      29,    52,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     2,     0,   185,    88,     0,   105,   106,   107,   108,
       0,     0,     0,     0,     0,     6,    92,    93,    94,     0,
       0,     0,     0,    51,     0,     0,     0,     0,    10,    97,
       0,     0,     0,    98,     0,     0,     0,     0,    99,     0,
       0,     0,    15,     0,    16,     0,     0,    18,     0,     0,
      21,     0,    22,    23,    24,    25,    26,    27,    28,    29,
      52,     0,     0,     0,     0,     1,     0,     0,     0,     0,
       0,     0,   104,     0,     2,   105,   106,   107,   108,     3,
       0,     0,     0,     0,     0,     0,     0,     5,     6,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     7,     8,
       9,    10,     0,    11,     0,     0,     0,     0,     0,    12,
      13,     0,     0,    14,     0,    15,     0,    16,     0,    17,
      18,    19,    20,    21,     1,    22,    23,    24,    25,    26,
      27,    28,    29,     2,     0,     0,     0,     0,     3,     0,
       0,     0,     0,     0,  -100,     4,     5,     6,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     7,     8,     9,
      10,     0,    11,     0,     0,     0,     0,     0,    12,    13,
       0,     0,    14,     0,    15,     0,    16,     0,    17,    18,
      19,    20,    21,     1,    22,    23,    24,    25,    26,    27,
      28,    29,     2,     0,     0,     0,     0,     3,     0,     0,
     339,     0,     0,     0,     4,     5,     6,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     7,     8,     9,    10,
       0,    11,     0,     0,     0,     0,     0,    12,    13,     0,
       0,    14,     0,    15,     0,    16,     0,    17,    18,    19,
      20,    21,     0,    22,    23,    24,    25,    26,    27,    28,
      29,     1,     0,     0,     0,     0,   259,     0,     0,     0,
       2,     0,     0,     0,     0,     3,     0,     0,     0,     0,
       0,     0,     4,     5,     6,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     7,     8,     9,    10,     0,    11,
       0,     0,     0,     0,     0,    12,    13,     0,     0,    14,
       0,    15,     0,    16,     0,    17,    18,    19,    20,    21,
       0,    22,    23,    24,    25,    26,    27,    28,    29,     1,
       0,     0,     0,     0,   261,     0,     0,     0,     2,     0,
       0,     0,     0,     3,     0,     0,     0,     0,     0,     0,
       0,     5,     6,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     7,     8,     9,    10,     0,    11,     0,     0,
       0,     0,     0,    12,    13,     0,     0,    14,     0,    15,
       0,    16,     0,    17,    18,    19,    20,    21,     0,    22,
      23,    24,    25,    26,    27,    28,    29,    62,     1,     0,
       0,     0,   311,     0,     0,     0,     0,     2,     0,     0,
       0,     0,     3,     0,     0,     0,     0,     0,     0,     4,
       5,     6,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     7,     8,     9,    10,     0,    11,     0,     0,     0,
       0,     0,    12,    13,     0,     0,    14,     0,    15,     0,
      16,     0,    17,    18,    19,    20,    21,     1,    22,    23,
      24,    25,    26,    27,    28,    29,     2,     0,     0,     0,
       0,     3,     0,     0,     0,     0,     0,     0,     4,     5,
       6,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       7,     8,     9,    10,     0,    11,     0,     0,     0,     0,
       0,    12,    13,     0,     0,    14,     0,    15,     0,    16,
       0,    17,    18,    19,    20,    21,     1,    22,    23,    24,
      25,    26,    27,    28,    29,     2,     0,     0,     0,     0,
       3,     0,     0,     0,     0,     0,     0,     0,     5,     6,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     7,
       8,     9,    10,     0,    11,     0,     0,     0,     0,     0,
      12,    13,     0,     0,    14,     0,    15,     0,    16,     0,
      17,    18,    19,    20,    21,     1,    22,    23,    24,    25,
      26,    27,    28,    29,     2,     0,     0,     0,     0,     3,
       0,     0,     0,     0,     0,     0,     0,     5,     6,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     7,     8,
       9,    10,     0,    11,     0,     0,     0,     0,     0,    12,
      13,     0,     0,    14,     0,    15,     0,    16,     0,     0,
      18,    19,    20,    21,     0,    22,    23,    24,    25,    26,
      27,    28,    29
};

static const yytype_int16 yycheck[] =
{
       0,     0,    47,   104,    47,    33,   183,    15,   141,   138,
     190,   246,    90,    13,   211,   212,   213,    17,    26,    44,
      91,    32,   100,   306,    13,    29,   104,    70,    17,   172,
      30,    30,   175,    73,    32,    38,    40,    37,    38,    39,
      40,    41,    82,   326,    15,    32,    91,    47,    37,    38,
      39,    40,    41,    38,    32,    26,    72,    30,    69,    72,
      71,    74,    33,    82,    80,    20,    71,   138,    41,    94,
      70,    69,    76,    77,   271,    42,    79,   360,    56,   157,
      47,    70,    69,   318,    84,    84,    86,    86,    43,    37,
       0,    69,    72,   138,    79,   140,   141,   140,   141,    75,
      80,   234,   337,    71,   104,    71,   184,   185,    71,    72,
     153,    78,   190,    61,    81,   104,    83,    76,    66,    67,
      30,   199,   200,   201,   202,   203,   204,    84,    85,   207,
     208,   209,    43,    44,    88,    89,    90,   165,   183,   110,
     140,   141,   322,   323,   152,    81,    57,   324,    81,    75,
      81,   229,    78,   153,    81,    81,    91,   258,    92,     5,
       6,     7,     8,     9,   218,   219,   220,   221,   246,    93,
     214,   215,   172,   172,     3,   175,   175,   306,   216,   217,
     357,   152,    94,   222,   223,   156,    71,    15,    14,   234,
     161,   234,    14,    80,   165,    23,    42,   326,    72,    71,
     243,    47,    60,    71,    80,    80,    34,    35,    36,    73,
      21,    39,   255,    79,   242,    98,    99,    45,    46,    73,
      81,    49,   105,   106,   107,   108,    78,    73,    71,    57,
      58,   360,    78,    72,   234,    81,    80,    83,    80,   210,
     318,    79,    72,   243,   322,   323,    80,    71,    75,    80,
     328,    71,    71,    30,   332,   255,    79,   156,   258,   337,
      35,   306,    80,    35,    35,    35,    35,   238,    44,   258,
      26,   242,    86,   316,   243,   224,   255,   238,   225,   324,
     226,   326,   227,   333,   228,   140,    -1,   357,   161,    -1,
      -1,   234,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,
      -1,    -1,   357,    -1,    -1,   360,   316,   316,    10,    11,
      -1,    13,    -1,    15,    16,    -1,    18,    19,    -1,    -1,
      22,    23,    24,    25,    26,    27,    28,    -1,    -1,    -1,
      32,    33,    34,    35,    36,    37,    38,    39,    -1,    -1,
      42,    -1,    -1,    45,    46,    47,    48,    49,    -1,    51,
      -1,    53,    -1,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    -1,    71,
      -1,    -1,    74,    -1,    -1,    -1,    -1,     1,    -1,    81,
      82,    -1,    84,    85,    86,    87,    10,    11,    -1,    13,
      -1,    15,    16,    -1,    18,    19,    -1,    -1,    22,    23,
      24,    25,    26,    27,    28,    -1,    -1,    -1,    32,    33,
      34,    35,    36,    37,    38,    39,    -1,    -1,    42,    -1,
      -1,    45,    46,    47,    48,    49,    -1,    51,    -1,    53,
      -1,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    -1,    71,    -1,    -1,
      74,    -1,    -1,    -1,    -1,     1,    -1,    81,    82,    -1,
      84,    85,    86,    87,    10,    11,    -1,    13,    -1,    15,
      16,    -1,    18,    19,    -1,    -1,    22,    23,    24,    25,
      26,    27,    28,    -1,    -1,    -1,    32,    33,    34,    35,
      36,    37,    38,    39,    -1,    -1,    42,    -1,    -1,    45,
      46,    47,    48,    49,    -1,    51,    -1,    53,    -1,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    -1,    71,    -1,    -1,    74,    -1,
      -1,    -1,    -1,     1,    -1,    81,    82,    -1,    84,    85,
      86,    87,    10,    -1,    -1,    13,    -1,    15,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    23,    24,    25,    26,    27,
      -1,    -1,    -1,    -1,    32,    -1,    34,    35,    36,    37,
      38,    39,    -1,    -1,    42,    -1,    -1,    45,    46,    47,
      -1,    49,    -1,    51,    -1,    53,    -1,    -1,    56,    57,
      58,    59,    -1,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     1,    -1,    81,    -1,    -1,    84,    85,    86,    87,
      10,    11,    -1,    13,    -1,    -1,    16,    -1,    18,    19,
      -1,    -1,    -1,    -1,    24,    25,    26,    27,    28,    -1,
      -1,    -1,    32,    33,    -1,    -1,    -1,    37,    38,    -1,
      -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    48,    -1,
      -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      -1,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,
      -1,    81,    82,    -1,    84,    85,    86,    87,    10,    -1,
      -1,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,
      42,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,
      -1,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    -1,     1,
      -1,    -1,    74,    -1,    -1,    -1,    -1,    -1,    10,    81,
      82,    13,    84,    85,    86,    87,    -1,    -1,    -1,    -1,
      -1,    -1,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,
      42,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,
      -1,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    -1,     1,
      -1,    -1,    74,    -1,    -1,    -1,    -1,    -1,    10,    81,
      82,    13,    84,    85,    86,    87,    -1,    -1,    -1,    -1,
      -1,    -1,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,
      42,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,
      -1,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,    69,     1,    71,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    10,    -1,    81,
      13,    -1,    84,    85,    86,    87,    -1,    -1,    -1,    -1,
      -1,    24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,
      -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,
      -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,
      53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,    62,
      63,    64,    65,    66,    67,    68,    69,     1,    71,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    10,    -1,    81,    13,
      -1,    84,    85,    86,    87,    -1,    -1,    -1,    -1,    -1,
      24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,    -1,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,    53,
      -1,    -1,    56,    -1,    -1,    59,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    -1,     1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    10,    81,    82,    13,
      84,    85,    86,    87,    -1,    -1,    -1,    -1,    -1,    -1,
      24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,    -1,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,    53,
      -1,    -1,    56,    -1,    -1,    59,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,    69,     1,    71,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    10,    -1,    81,    13,    -1,
      84,    85,    86,    87,    -1,    -1,    -1,    -1,    -1,    24,
      25,    26,    27,    -1,    -1,    -1,    -1,    32,    -1,    -1,
      -1,    -1,    37,    38,    -1,    -1,    -1,    42,    -1,    -1,
      -1,    -1,    47,    -1,    -1,    -1,    51,    -1,    53,    -1,
      -1,    56,    -1,    -1,    59,    -1,    61,    62,    63,    64,
      65,    66,    67,    68,    69,     1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    10,    80,    81,    13,    -1,    84,
      85,    86,    87,    -1,    -1,    -1,    -1,    -1,    24,    25,
      26,    27,    -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    37,    38,    -1,    -1,    -1,    42,    -1,    -1,    -1,
      -1,    47,    -1,    -1,    -1,    51,    -1,    53,    -1,    -1,
      56,    -1,    -1,    59,    -1,    61,    62,    63,    64,    65,
      66,    67,    68,    69,     1,    71,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    10,    -1,    81,    13,    -1,    84,    85,
      86,    87,    -1,    -1,    -1,    -1,    -1,    24,    25,    26,
      27,    -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,
      37,    38,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,
      47,    -1,    -1,    -1,    51,    -1,    53,    -1,    -1,    56,
      -1,    -1,    59,    -1,    61,    62,    63,    64,    65,    66,
      67,    68,    69,     1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    10,    80,    81,    13,    -1,    84,    85,    86,
      87,    -1,    -1,    -1,    -1,    -1,    24,    25,    26,    27,
      -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,
      38,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    47,
      -1,    -1,    -1,    51,    -1,    53,    -1,    -1,    56,    -1,
      -1,    59,    -1,    61,    62,    63,    64,    65,    66,    67,
      68,    69,     1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    10,    -1,    81,    13,    -1,    84,    85,    86,    87,
      -1,    -1,    -1,    -1,    -1,    24,    25,    26,    27,    -1,
      -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    38,
      -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,
      59,    -1,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,
      -1,    -1,    81,    -1,    10,    84,    85,    86,    87,    15,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    24,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    35,
      36,    37,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      46,    -1,    -1,    49,    -1,    51,    -1,    53,    -1,    55,
      56,    57,    58,    59,     1,    61,    62,    63,    64,    65,
      66,    67,    68,    10,    -1,    -1,    -1,    -1,    15,    -1,
      -1,    -1,    -1,    -1,    80,    22,    23,    24,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,
      37,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,
      -1,    -1,    49,    -1,    51,    -1,    53,    -1,    55,    56,
      57,    58,    59,     1,    61,    62,    63,    64,    65,    66,
      67,    68,    10,    -1,    -1,    -1,    -1,    15,    -1,    -1,
      77,    -1,    -1,    -1,    22,    23,    24,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,    37,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,    -1,
      -1,    49,    -1,    51,    -1,    53,    -1,    55,    56,    57,
      58,    59,    -1,    61,    62,    63,    64,    65,    66,    67,
      68,     1,    -1,    -1,    -1,    -1,    74,    -1,    -1,    -1,
      10,    -1,    -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,
      -1,    -1,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    34,    35,    36,    37,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    46,    -1,    -1,    49,
      -1,    51,    -1,    53,    -1,    55,    56,    57,    58,    59,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,     1,
      -1,    -1,    -1,    -1,    74,    -1,    -1,    -1,    10,    -1,
      -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    34,    35,    36,    37,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    46,    -1,    -1,    49,    -1,    51,
      -1,    53,    -1,    55,    56,    57,    58,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,     0,     1,    -1,
      -1,    -1,    74,    -1,    -1,    -1,    -1,    10,    -1,    -1,
      -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,
      23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    35,    36,    37,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    46,    -1,    -1,    49,    -1,    51,    -1,
      53,    -1,    55,    56,    57,    58,    59,     1,    61,    62,
      63,    64,    65,    66,    67,    68,    10,    -1,    -1,    -1,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      34,    35,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    46,    -1,    -1,    49,    -1,    51,    -1,    53,
      -1,    55,    56,    57,    58,    59,     1,    61,    62,    63,
      64,    65,    66,    67,    68,    10,    -1,    -1,    -1,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    24,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,
      35,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    46,    -1,    -1,    49,    -1,    51,    -1,    53,    -1,
      55,    56,    57,    58,    59,     1,    61,    62,    63,    64,
      65,    66,    67,    68,    10,    -1,    -1,    -1,    -1,    15,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    24,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    35,
      36,    37,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      46,    -1,    -1,    49,    -1,    51,    -1,    53,    -1,    -1,
      56,    57,    58,    59,    -1,    61,    62,    63,    64,    65,
      66,    67,    68
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    10,    15,    22,    23,    24,    34,    35,    36,
      37,    39,    45,    46,    49,    51,    53,    55,    56,    57,
      58,    59,    61,    62,    63,    64,    65,    66,    67,    68,
      96,    97,    98,   100,   101,   102,   105,   106,   107,   108,
     109,   110,   111,   113,   115,   118,   158,   159,   185,    71,
     101,    32,    69,   114,   185,   187,   101,   114,    37,    61,
      66,    67,     0,    97,    71,   103,   104,   125,   126,   127,
     128,   187,    46,   106,   107,   108,   109,   110,   101,   101,
     101,   101,   101,    82,   112,   169,   112,    11,    13,    16,
      18,    19,    25,    26,    27,    28,    33,    38,    42,    47,
      48,    60,    71,    74,    81,    84,    85,    86,    87,    98,
     100,   105,   136,   137,   138,   139,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   171,   172,   173,   174,   175,   176,   181,   183,   186,
     187,   188,    75,   119,   169,    71,    72,    73,    82,    76,
     122,    75,    78,    81,    99,   100,   101,   130,   131,   132,
     133,    98,   116,   117,   158,   116,    71,    71,    71,   105,
     139,   157,   160,    81,    81,    81,   143,   143,    71,   157,
      81,    99,   101,   157,   143,   143,   143,   143,   125,    81,
       5,     6,     7,     8,     9,    42,    47,    73,    78,    81,
      83,    88,    89,    90,    84,    85,    30,    41,    29,    40,
      76,    77,    20,    43,    91,    92,    93,     3,    44,    94,
     160,   161,    74,   170,   171,    74,   172,    71,    14,   184,
     185,   187,   100,   120,   121,   104,    82,   134,   157,   123,
     184,    38,    79,   125,   129,    72,    80,    80,    72,    74,
     117,    74,    71,    60,   174,   178,   179,   157,   177,    71,
     177,    80,    80,   142,   157,   157,   157,   157,   157,   157,
     157,   157,   140,   141,   157,   182,   187,   144,   144,   144,
     145,   145,   146,   146,   147,   147,   147,   147,   148,   148,
     149,   150,   151,   152,   153,   157,    21,   170,   137,   187,
     125,    74,   121,    74,   134,   135,   124,    79,    73,    78,
     131,    99,    81,    71,    72,    80,    80,   144,    72,    80,
      79,    80,    72,    75,   161,   162,    71,    72,    74,    77,
      98,   134,    38,    79,   177,   177,   180,   174,   161,   162,
     157,   157,   155,    74,   134,    79,    80,    71,    71,   178,
      80,   161,   162
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    95,    96,    96,    97,    97,    98,    98,    98,    99,
     100,   100,   101,   101,   101,   101,   101,   101,   101,   102,
     102,   102,   102,   102,   102,   102,   103,   103,   104,   104,
     105,   105,   105,   105,   105,   105,   105,   105,   105,   105,
     105,   105,   105,   105,   105,   105,   105,   105,   105,   105,
     106,   107,   107,   108,   108,   109,   109,   110,   110,   110,
     111,   111,   111,   112,   113,   113,   113,   114,   114,   115,
     116,   116,   117,   117,   118,   119,   120,   120,   121,   123,
     122,   124,   124,   125,   125,   126,   126,   127,   127,   127,
     127,   127,   128,   129,   129,   129,   130,   130,   131,   131,
     132,   132,   133,   133,   134,   134,   134,   134,   135,   135,
     136,   136,   137,   138,   138,   138,   138,   139,   139,   139,
     139,   139,   139,   140,   140,   141,   141,   142,   142,   143,
     143,   143,   143,   143,   143,   143,   144,   144,   145,   145,
     145,   145,   146,   146,   146,   147,   147,   147,   148,   148,
     148,   148,   148,   149,   149,   149,   150,   150,   151,   151,
     152,   152,   153,   153,   154,   154,   155,   155,   156,   157,
     158,   158,   159,   160,   160,   161,   161,   161,   161,   161,
     161,   161,   162,   162,   163,   163,   164,   164,   165,   166,
     166,   167,   168,   168,   169,   170,   171,   171,   172,   172,
     173,   173,   174,   174,   174,   174,   174,   174,   174,   175,
     175,   175,   176,   176,   177,   178,   178,   179,   179,   180,
     180,   181,   181,   182,   183,   184,   185,   186,   187,   187,
     188,   188,   188,   188,   188
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     1,     1,     2,     3,     2,     2,
       1,     2,     1,     2,     2,     2,     2,     2,     2,     1,
       2,     2,     2,     2,     2,     2,     1,     3,     1,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     2,     2,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       4,     4,     1,     1,     2,     4,     4,     1,     1,     1,
       1,     2,     1,     1,     5,     1,     1,     2,     3,     0,
       4,     0,     2,     1,     2,     1,     3,     1,     4,     3,
       3,     3,     2,     0,     4,     3,     1,     3,     2,     4,
       0,     1,     1,     3,     1,     3,     4,     2,     1,     3,
       1,     3,     1,     1,     1,     3,     4,     1,     2,     2,
       3,     4,     4,     0,     1,     1,     3,     1,     3,     1,
       2,     2,     2,     2,     2,     2,     1,     4,     1,     3,
       3,     3,     1,     3,     3,     1,     3,     3,     1,     3,
       3,     3,     3,     1,     3,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     5,     1,     1,
       3,     2,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     3,     2,     2,     4,     2,
       4,     4,     3,     2,     1,     1,     1,     2,     1,     1,
       2,     1,     3,     1,     3,     3,     3,     3,     3,     5,
       7,     9,     5,     9,     1,     1,     0,     1,     3,     1,
       0,     3,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1
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
#line 271 "parser.y"
                              { (yyval.dummy) = GlobalInitStatements(CurrentScope, (yyvsp[0].sc_stmt)); }
#line 1940 "parser.c"
    break;

  case 6: /* declaration: declaration_specifiers ';'  */
#line 276 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 1946 "parser.c"
    break;

  case 7: /* declaration: declaration_specifiers init_declarator_list ';'  */
#line 278 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[-1].sc_stmt); }
#line 1952 "parser.c"
    break;

  case 8: /* declaration: ERROR_SY ';'  */
#line 280 "parser.y"
                              { RecordErrorPos(Cg->tokenLoc); (yyval.sc_stmt) = NULL; }
#line 1958 "parser.c"
    break;

  case 9: /* abstract_declaration: abstract_declaration_specifiers abstract_declarator  */
#line 284 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 1964 "parser.c"
    break;

  case 10: /* declaration_specifiers: abstract_declaration_specifiers  */
#line 291 "parser.y"
                              { (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 1970 "parser.c"
    break;

  case 11: /* declaration_specifiers: TYPEDEF_SY abstract_declaration_specifiers  */
#line 293 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, TYPE_MISC_TYPEDEF); (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 1976 "parser.c"
    break;

  case 12: /* abstract_declaration_specifiers: abstract_declaration_specifiers2  */
#line 298 "parser.y"
                              { (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 1982 "parser.c"
    break;

  case 13: /* abstract_declaration_specifiers: type_qualifier abstract_declaration_specifiers  */
#line 300 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1988 "parser.c"
    break;

  case 14: /* abstract_declaration_specifiers: storage_class abstract_declaration_specifiers  */
#line 302 "parser.y"
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1994 "parser.c"
    break;

  case 15: /* abstract_declaration_specifiers: type_domain abstract_declaration_specifiers  */
#line 304 "parser.y"
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2000 "parser.c"
    break;

  case 16: /* abstract_declaration_specifiers: in_out abstract_declaration_specifiers  */
#line 306 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2006 "parser.c"
    break;

  case 17: /* abstract_declaration_specifiers: function_specifier abstract_declaration_specifiers  */
#line 308 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2012 "parser.c"
    break;

  case 18: /* abstract_declaration_specifiers: PACKED_SY abstract_declaration_specifiers  */
#line 310 "parser.y"
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2018 "parser.c"
    break;

  case 19: /* abstract_declaration_specifiers2: type_specifier  */
#line 315 "parser.y"
                              { (yyval.sc_type) = *SetDType(&CurrentDeclTypeSpecs, (yyvsp[0].sc_ptype)); }
#line 2024 "parser.c"
    break;

  case 20: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 type_qualifier  */
#line 317 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2030 "parser.c"
    break;

  case 21: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 storage_class  */
#line 319 "parser.y"
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2036 "parser.c"
    break;

  case 22: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 type_domain  */
#line 321 "parser.y"
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2042 "parser.c"
    break;

  case 23: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 in_out  */
#line 323 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2048 "parser.c"
    break;

  case 24: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 function_specifier  */
#line 325 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2054 "parser.c"
    break;

  case 25: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 PACKED_SY  */
#line 327 "parser.y"
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2060 "parser.c"
    break;

  case 26: /* init_declarator_list: init_declarator  */
#line 331 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[0].sc_stmt); }
#line 2066 "parser.c"
    break;

  case 27: /* init_declarator_list: init_declarator_list ',' init_declarator  */
#line 333 "parser.y"
                              { (yyval.sc_stmt) = AddStmt((yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2072 "parser.c"
    break;

  case 28: /* init_declarator: declarator  */
#line 337 "parser.y"
                              { (yyval.sc_stmt) = Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_decl), NULL); }
#line 2078 "parser.c"
    break;

  case 29: /* init_declarator: declarator '=' initializer  */
#line 339 "parser.y"
                              { (yyval.sc_stmt) = Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[0].sc_expr)); }
#line 2084 "parser.c"
    break;

  case 30: /* type_specifier: INT_SY  */
#line 347 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, INT_SY); }
#line 2090 "parser.c"
    break;

  case 31: /* type_specifier: FLOAT_SY  */
#line 349 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, FLOAT_SY); }
#line 2096 "parser.c"
    break;

  case 32: /* type_specifier: VOID_SY  */
#line 351 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, VOID_SY); }
#line 2102 "parser.c"
    break;

  case 33: /* type_specifier: BOOLEAN_SY  */
#line 353 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, BOOLEAN_SY); }
#line 2108 "parser.c"
    break;

  case 34: /* type_specifier: TEXOBJ_SY  */
#line 355 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, TEXOBJ_SY); }
#line 2114 "parser.c"
    break;

  case 35: /* type_specifier: CHAR_SY  */
#line 357 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2120 "parser.c"
    break;

  case 36: /* type_specifier: SHORT_SY  */
#line 359 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2126 "parser.c"
    break;

  case 37: /* type_specifier: LONG_SY  */
#line 361 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2132 "parser.c"
    break;

  case 38: /* type_specifier: HALF_SY  */
#line 363 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2138 "parser.c"
    break;

  case 39: /* type_specifier: FIXED_SY  */
#line 365 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2144 "parser.c"
    break;

  case 40: /* type_specifier: DOUBLE_SY  */
#line 367 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2150 "parser.c"
    break;

  case 41: /* type_specifier: UNSIGNED_SY  */
#line 369 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2156 "parser.c"
    break;

  case 42: /* type_specifier: UNSIGNED_SY CHAR_SY  */
#line 371 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2162 "parser.c"
    break;

  case 43: /* type_specifier: UNSIGNED_SY SHORT_SY  */
#line 373 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2168 "parser.c"
    break;

  case 44: /* type_specifier: UNSIGNED_SY INT_SY  */
#line 375 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2174 "parser.c"
    break;

  case 45: /* type_specifier: UNSIGNED_SY LONG_SY  */
#line 377 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2180 "parser.c"
    break;

  case 46: /* type_specifier: struct_or_connector_specifier  */
#line 379 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2186 "parser.c"
    break;

  case 47: /* type_specifier: interface_specifier  */
#line 381 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2192 "parser.c"
    break;

  case 48: /* type_specifier: type_identifier  */
#line 383 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, (yyvsp[0].sc_ident)); }
#line 2198 "parser.c"
    break;

  case 49: /* type_specifier: error  */
#line 385 "parser.y"
                              {
                                SemanticParseError(Cg->tokenLoc, ERROR_S_TYPE_NAME_EXPECTED,
                                                   GetAtomString(atable, Cg->mostRecentToken /* yychar */));
                                (yyval.sc_ptype) = UndefinedType;
                              }
#line 2208 "parser.c"
    break;

  case 50: /* type_qualifier: CONST_SY  */
#line 397 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_CONST; }
#line 2214 "parser.c"
    break;

  case 51: /* type_domain: UNIFORM_SY  */
#line 405 "parser.y"
                              { (yyval.sc_int) = TYPE_DOMAIN_UNIFORM; }
#line 2220 "parser.c"
    break;

  case 52: /* type_domain: VARYING_SY  */
#line 407 "parser.y"
                              { (yyval.sc_int) = TYPE_DOMAIN_VARYING; }
#line 2226 "parser.c"
    break;

  case 53: /* storage_class: STATIC_SY  */
#line 415 "parser.y"
                              { (yyval.sc_int) = (int) SC_STATIC; }
#line 2232 "parser.c"
    break;

  case 54: /* storage_class: EXTERN_SY  */
#line 417 "parser.y"
                              { (yyval.sc_int) = (int) SC_EXTERN; }
#line 2238 "parser.c"
    break;

  case 55: /* function_specifier: INLINE_SY  */
#line 425 "parser.y"
                              { (yyval.sc_int) = TYPE_MISC_INLINE; }
#line 2244 "parser.c"
    break;

  case 56: /* function_specifier: INTERNAL_SY  */
#line 427 "parser.y"
                              { (yyval.sc_int) = TYPE_MISC_INTERNAL; }
#line 2250 "parser.c"
    break;

  case 57: /* in_out: IN_SY  */
#line 435 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_IN; }
#line 2256 "parser.c"
    break;

  case 58: /* in_out: OUT_SY  */
#line 437 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_OUT; }
#line 2262 "parser.c"
    break;

  case 59: /* in_out: INOUT_SY  */
#line 439 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_INOUT; }
#line 2268 "parser.c"
    break;

  case 60: /* struct_or_connector_specifier: struct_or_connector_header struct_compound_header struct_declaration_list '}'  */
#line 448 "parser.y"
                              { (yyval.sc_ptype) = SetStructMembers(Cg->tokenLoc, (yyvsp[-3].sc_ptype), PopScope());
                                CheckInterfaceConformance(Cg->tokenLoc, (yyval.sc_ptype)); }
#line 2275 "parser.c"
    break;

  case 61: /* struct_or_connector_specifier: untagged_struct_header struct_compound_header struct_declaration_list '}'  */
#line 451 "parser.y"
                              { (yyval.sc_ptype) = SetStructMembers(Cg->tokenLoc, (yyvsp[-3].sc_ptype), PopScope());
                                CheckInterfaceConformance(Cg->tokenLoc, (yyval.sc_ptype)); }
#line 2282 "parser.c"
    break;

  case 62: /* struct_or_connector_specifier: struct_or_connector_header  */
#line 454 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2288 "parser.c"
    break;

  case 63: /* struct_compound_header: compound_header  */
#line 458 "parser.y"
                              { CurrentScope->IsStructScope = 1; (yyval.dummy) = (yyvsp[0].dummy); }
#line 2294 "parser.c"
    break;

  case 64: /* struct_or_connector_header: STRUCT_SY struct_identifier  */
#line 463 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, 0, (yyvsp[0].sc_ident)); }
#line 2300 "parser.c"
    break;

  case 65: /* struct_or_connector_header: STRUCT_SY struct_identifier ':' semantics_identifier  */
#line 465 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_ident), (yyvsp[-2].sc_ident)); }
#line 2306 "parser.c"
    break;

  case 66: /* struct_or_connector_header: STRUCT_SY struct_identifier ':' type_identifier  */
#line 467 "parser.y"
                              { (yyval.sc_ptype) = SetStructInterface(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_ident), (yyvsp[0].sc_ident)); }
#line 2312 "parser.c"
    break;

  case 69: /* untagged_struct_header: STRUCT_SY  */
#line 475 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, 0, 0); }
#line 2318 "parser.c"
    break;

  case 72: /* struct_declaration: declaration  */
#line 483 "parser.y"
                            { (yyval.sc_stmt) = (yyvsp[0].sc_stmt); }
#line 2324 "parser.c"
    break;

  case 73: /* struct_declaration: function_definition  */
#line 485 "parser.y"
                            { (yyval.sc_stmt) = NULL; }
#line 2330 "parser.c"
    break;

  case 74: /* interface_specifier: INTERFACE_SY struct_identifier interface_compound_header interface_member_declaration_list '}'  */
#line 496 "parser.y"
                              { (yyval.sc_ptype) = SetInterfaceMembers(Cg->tokenLoc,
                                                         InterfaceHeader(Cg->tokenLoc, CurrentScope, (yyvsp[-3].sc_ident)),
                                                         PopScope()); }
#line 2338 "parser.c"
    break;

  case 75: /* interface_compound_header: compound_header  */
#line 503 "parser.y"
                              { CurrentScope->IsStructScope = 1; (yyval.dummy) = (yyvsp[0].dummy); }
#line 2344 "parser.c"
    break;

  case 78: /* interface_member_declaration: declaration_specifiers declarator ';'  */
#line 520 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2350 "parser.c"
    break;

  case 79: /* $@1: %empty  */
#line 538 "parser.y"
                              { PushScope(NewScope()); }
#line 2356 "parser.c"
    break;

  case 80: /* annotation: '<' $@1 annotation_decl_list '>'  */
#line 539 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[-1].sc_stmt); PopScope(); }
#line 2362 "parser.c"
    break;

  case 81: /* annotation_decl_list: %empty  */
#line 543 "parser.y"
                              { (yyval.sc_stmt) = 0; }
#line 2368 "parser.c"
    break;

  case 83: /* declarator: semantic_declarator  */
#line 552 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2374 "parser.c"
    break;

  case 84: /* declarator: semantic_declarator annotation  */
#line 554 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[-1].sc_decl); }
#line 2380 "parser.c"
    break;

  case 85: /* semantic_declarator: basic_declarator  */
#line 558 "parser.y"
                              { (yyval.sc_decl) = Declarator(Cg->tokenLoc, (yyvsp[0].sc_decl), 0); }
#line 2386 "parser.c"
    break;

  case 86: /* semantic_declarator: basic_declarator ':' semantics_identifier  */
#line 560 "parser.y"
                              { (yyval.sc_decl) = Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), (yyvsp[0].sc_ident)); }
#line 2392 "parser.c"
    break;

  case 87: /* basic_declarator: identifier  */
#line 564 "parser.y"
                              { (yyval.sc_decl) = NewDeclNode(Cg->tokenLoc, (yyvsp[0].sc_ident), &CurrentDeclTypeSpecs); }
#line 2398 "parser.c"
    break;

  case 88: /* basic_declarator: basic_declarator '[' INTCONST_SY ']'  */
#line 566 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-3].sc_decl), (int) (yyvsp[-1].sc_literal).value.i, 0); }
#line 2404 "parser.c"
    break;

  case 89: /* basic_declarator: basic_declarator '[' ']'  */
#line 568 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), 0 , 1); }
#line 2410 "parser.c"
    break;

  case 90: /* basic_declarator: function_decl_header parameter_list ')'  */
#line 570 "parser.y"
                              { (yyval.sc_decl) = SetFunTypeParams(CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_decl), (yyvsp[-1].sc_decl)); }
#line 2416 "parser.c"
    break;

  case 91: /* basic_declarator: function_decl_header abstract_parameter_list ')'  */
#line 572 "parser.y"
                              { (yyval.sc_decl) = SetFunTypeParams(CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_decl), NULL); }
#line 2422 "parser.c"
    break;

  case 92: /* function_decl_header: basic_declarator '('  */
#line 576 "parser.y"
                              { (yyval.sc_decl) = FunctionDeclHeader(&(yyvsp[-1].sc_decl)->loc, CurrentScope, (yyvsp[-1].sc_decl)); }
#line 2428 "parser.c"
    break;

  case 93: /* abstract_declarator: %empty  */
#line 580 "parser.y"
                              { (yyval.sc_decl) = NewDeclNode(Cg->tokenLoc, 0, &CurrentDeclTypeSpecs); }
#line 2434 "parser.c"
    break;

  case 94: /* abstract_declarator: abstract_declarator '[' INTCONST_SY ']'  */
#line 582 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-3].sc_decl), (int) (yyvsp[-1].sc_literal).value.i, 0); }
#line 2440 "parser.c"
    break;

  case 95: /* abstract_declarator: abstract_declarator '[' ']'  */
#line 584 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), 0 , 1); }
#line 2446 "parser.c"
    break;

  case 96: /* parameter_list: parameter_declaration  */
#line 601 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2452 "parser.c"
    break;

  case 97: /* parameter_list: parameter_list ',' parameter_declaration  */
#line 603 "parser.y"
                              { (yyval.sc_decl) = AddDecl((yyvsp[-2].sc_decl), (yyvsp[0].sc_decl)); }
#line 2458 "parser.c"
    break;

  case 98: /* parameter_declaration: declaration_specifiers declarator  */
#line 607 "parser.y"
                              { (yyval.sc_decl) = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_decl), NULL); }
#line 2464 "parser.c"
    break;

  case 99: /* parameter_declaration: declaration_specifiers declarator '=' initializer  */
#line 609 "parser.y"
                              { (yyval.sc_decl) = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[0].sc_expr)); }
#line 2470 "parser.c"
    break;

  case 100: /* abstract_parameter_list: %empty  */
#line 613 "parser.y"
                              { (yyval.sc_decl) = NULL; }
#line 2476 "parser.c"
    break;

  case 102: /* non_empty_abstract_parameter_list: abstract_declaration  */
#line 618 "parser.y"
                              {
                                if (IsVoid(&(yyvsp[0].sc_decl)->type.type))
                                    CurrentScope->HasVoidParameter = 1;
                                (yyval.sc_decl) = (yyvsp[0].sc_decl);
                              }
#line 2486 "parser.c"
    break;

  case 103: /* non_empty_abstract_parameter_list: non_empty_abstract_parameter_list ',' abstract_declaration  */
#line 624 "parser.y"
                              {
                                if (CurrentScope->HasVoidParameter || IsVoid(&(yyvsp[-2].sc_decl)->type.type)) {
                                    SemanticError(Cg->tokenLoc, ERROR___VOID_NOT_ONLY_PARAM);
                                }
                                (yyval.sc_decl) = AddDecl((yyvsp[-2].sc_decl), (yyvsp[0].sc_decl));
                              }
#line 2497 "parser.c"
    break;

  case 104: /* initializer: expression  */
#line 637 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[0].sc_expr)); }
#line 2503 "parser.c"
    break;

  case 105: /* initializer: '{' initializer_list '}'  */
#line 639 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[-1].sc_expr)); }
#line 2509 "parser.c"
    break;

  case 106: /* initializer: '{' initializer_list ',' '}'  */
#line 641 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[-2].sc_expr)); }
#line 2515 "parser.c"
    break;

  case 107: /* initializer: '{' '}'  */
#line 647 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, NULL); }
#line 2521 "parser.c"
    break;

  case 108: /* initializer_list: initializer  */
#line 651 "parser.y"
                              { (yyval.sc_expr) = InitializerList(Cg->tokenLoc, (yyvsp[0].sc_expr), NULL); }
#line 2527 "parser.c"
    break;

  case 109: /* initializer_list: initializer_list ',' initializer  */
#line 653 "parser.y"
                              { (yyval.sc_expr) = InitializerList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2533 "parser.c"
    break;

  case 110: /* variable: basic_variable  */
#line 665 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[0].sc_expr); }
#line 2539 "parser.c"
    break;

  case 111: /* variable: scope_identifier COLONCOLON_SY basic_variable  */
#line 667 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[0].sc_expr); }
#line 2545 "parser.c"
    break;

  case 112: /* basic_variable: variable_identifier  */
#line 671 "parser.y"
                              { (yyval.sc_expr) = BasicVariable(Cg->tokenLoc, (yyvsp[0].sc_ident)); }
#line 2551 "parser.c"
    break;

  case 115: /* primary_expression: '(' expression ')'  */
#line 681 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[-1].sc_expr); }
#line 2557 "parser.c"
    break;

  case 116: /* primary_expression: type_specifier '(' expression_list ')'  */
#line 683 "parser.y"
                              { (yyval.sc_expr) = NewVectorConstructor(Cg->tokenLoc, (yyvsp[-3].sc_ptype), (yyvsp[-1].sc_expr)); }
#line 2563 "parser.c"
    break;

  case 118: /* postfix_expression: postfix_expression PLUSPLUS_SY  */
#line 692 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(POSTINC_OP, (yyvsp[-1].sc_expr)); }
#line 2569 "parser.c"
    break;

  case 119: /* postfix_expression: postfix_expression MINUSMINUS_SY  */
#line 694 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(POSTDEC_OP, (yyvsp[-1].sc_expr)); }
#line 2575 "parser.c"
    break;

  case 120: /* postfix_expression: postfix_expression '.' member_identifier  */
#line 696 "parser.y"
                              { (yyval.sc_expr) = NewMemberSelectorOrSwizzleOrWriteMaskOperator(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_ident)); }
#line 2581 "parser.c"
    break;

  case 121: /* postfix_expression: postfix_expression '[' expression ']'  */
#line 698 "parser.y"
                              { (yyval.sc_expr) = NewIndexOperator(Cg->tokenLoc, (yyvsp[-3].sc_expr), (yyvsp[-1].sc_expr)); }
#line 2587 "parser.c"
    break;

  case 122: /* postfix_expression: postfix_expression '(' actual_argument_list ')'  */
#line 700 "parser.y"
                              { (yyval.sc_expr) = NewFunctionCallOperator(Cg->tokenLoc, (yyvsp[-3].sc_expr), (yyvsp[-1].sc_expr)); }
#line 2593 "parser.c"
    break;

  case 123: /* actual_argument_list: %empty  */
#line 704 "parser.y"
                                { (yyval.sc_expr) = NULL; }
#line 2599 "parser.c"
    break;

  case 125: /* non_empty_argument_list: expression  */
#line 709 "parser.y"
                              { (yyval.sc_expr) = ArgumentList(Cg->tokenLoc, NULL, (yyvsp[0].sc_expr)); }
#line 2605 "parser.c"
    break;

  case 126: /* non_empty_argument_list: non_empty_argument_list ',' expression  */
#line 711 "parser.y"
                              { (yyval.sc_expr) = ArgumentList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2611 "parser.c"
    break;

  case 127: /* expression_list: expression  */
#line 715 "parser.y"
                              { (yyval.sc_expr) = ExpressionList(Cg->tokenLoc, NULL, (yyvsp[0].sc_expr)); }
#line 2617 "parser.c"
    break;

  case 128: /* expression_list: expression_list ',' expression  */
#line 717 "parser.y"
                              { (yyval.sc_expr) = ExpressionList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2623 "parser.c"
    break;

  case 130: /* unary_expression: PLUSPLUS_SY unary_expression  */
#line 726 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(PREINC_OP, (yyvsp[0].sc_expr)); }
#line 2629 "parser.c"
    break;

  case 131: /* unary_expression: MINUSMINUS_SY unary_expression  */
#line 728 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(PREDEC_OP, (yyvsp[0].sc_expr)); }
#line 2635 "parser.c"
    break;

  case 132: /* unary_expression: '+' unary_expression  */
#line 730 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, POS_OP, '+', (yyvsp[0].sc_expr), 0); }
#line 2641 "parser.c"
    break;

  case 133: /* unary_expression: '-' unary_expression  */
#line 732 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, NEG_OP, '-', (yyvsp[0].sc_expr), 0); }
#line 2647 "parser.c"
    break;

  case 134: /* unary_expression: '!' unary_expression  */
#line 734 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, BNOT_OP, '!', (yyvsp[0].sc_expr), 0); }
#line 2653 "parser.c"
    break;

  case 135: /* unary_expression: '~' unary_expression  */
#line 736 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, NOT_OP, '~', (yyvsp[0].sc_expr), 1); }
#line 2659 "parser.c"
    break;

  case 137: /* cast_expression: '(' abstract_declaration ')' cast_expression  */
#line 748 "parser.y"
                              { (yyval.sc_expr) = NewCastOperator(Cg->tokenLoc, (yyvsp[0].sc_expr), GetTypePointer(&(yyvsp[-2].sc_decl)->loc, &(yyvsp[-2].sc_decl)->type)); }
#line 2665 "parser.c"
    break;

  case 139: /* multiplicative_expression: multiplicative_expression '*' cast_expression  */
#line 757 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, MUL_OP, '*', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2671 "parser.c"
    break;

  case 140: /* multiplicative_expression: multiplicative_expression '/' cast_expression  */
#line 759 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, DIV_OP, '/', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2677 "parser.c"
    break;

  case 141: /* multiplicative_expression: multiplicative_expression '%' cast_expression  */
#line 761 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, MOD_OP, '%', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2683 "parser.c"
    break;

  case 143: /* additive_expression: additive_expression '+' multiplicative_expression  */
#line 770 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, ADD_OP, '+', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2689 "parser.c"
    break;

  case 144: /* additive_expression: additive_expression '-' multiplicative_expression  */
#line 772 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SUB_OP, '-', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2695 "parser.c"
    break;

  case 146: /* shift_expression: shift_expression LL_SY additive_expression  */
#line 781 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SHL_OP, LL_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2701 "parser.c"
    break;

  case 147: /* shift_expression: shift_expression GG_SY additive_expression  */
#line 783 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SHR_OP, GG_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2707 "parser.c"
    break;

  case 149: /* relational_expression: relational_expression '<' shift_expression  */
#line 792 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, LT_OP, '<', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2713 "parser.c"
    break;

  case 150: /* relational_expression: relational_expression '>' shift_expression  */
#line 794 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, GT_OP, '>', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2719 "parser.c"
    break;

  case 151: /* relational_expression: relational_expression LE_SY shift_expression  */
#line 796 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, LE_OP, LE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2725 "parser.c"
    break;

  case 152: /* relational_expression: relational_expression GE_SY shift_expression  */
#line 798 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, GE_OP, GE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2731 "parser.c"
    break;

  case 154: /* equality_expression: equality_expression EQ_SY relational_expression  */
#line 807 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, EQ_OP, EQ_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2737 "parser.c"
    break;

  case 155: /* equality_expression: equality_expression NE_SY relational_expression  */
#line 809 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, NE_OP, NE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2743 "parser.c"
    break;

  case 157: /* AND_expression: AND_expression '&' equality_expression  */
#line 818 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, AND_OP, '&', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2749 "parser.c"
    break;

  case 159: /* exclusive_OR_expression: exclusive_OR_expression '^' AND_expression  */
#line 827 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, XOR_OP, '^', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2755 "parser.c"
    break;

  case 161: /* inclusive_OR_expression: inclusive_OR_expression '|' exclusive_OR_expression  */
#line 836 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, OR_OP, '|', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2761 "parser.c"
    break;

  case 163: /* logical_AND_expression: logical_AND_expression AND_SY inclusive_OR_expression  */
#line 845 "parser.y"
                              { (yyval.sc_expr) = NewBinaryBooleanOperator(Cg->tokenLoc, BAND_OP, AND_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2767 "parser.c"
    break;

  case 165: /* logical_OR_expression: logical_OR_expression OR_SY logical_AND_expression  */
#line 854 "parser.y"
                              { (yyval.sc_expr) = NewBinaryBooleanOperator(Cg->tokenLoc, BOR_OP, OR_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2773 "parser.c"
    break;

  case 167: /* conditional_expression: conditional_test '?' expression ':' conditional_expression  */
#line 863 "parser.y"
                              { (yyval.sc_expr) = NewConditionalOperator(Cg->tokenLoc, (yyvsp[-4].sc_expr), (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2779 "parser.c"
    break;

  case 168: /* conditional_test: logical_OR_expression  */
#line 867 "parser.y"
                              {  (yyval.sc_expr) = CheckBooleanExpr(Cg->tokenLoc, (yyvsp[0].sc_expr), 1); }
#line 2785 "parser.c"
    break;

  case 170: /* function_definition: function_definition_header block_item_list '}'  */
#line 886 "parser.y"
                              { DefineFunction(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_stmt)); PopScope();
                                ResumeStructScopeAfterMethodBody(); }
#line 2792 "parser.c"
    break;

  case 171: /* function_definition: function_definition_header '}'  */
#line 889 "parser.y"
                              { DefineFunction(Cg->tokenLoc, CurrentScope, (yyvsp[-1].sc_decl), NULL); PopScope();
                                ResumeStructScopeAfterMethodBody(); }
#line 2799 "parser.c"
    break;

  case 172: /* function_definition_header: declaration_specifiers declarator '{'  */
#line 894 "parser.y"
                              { (yyval.sc_decl) = Function_Definition_Header(Cg->tokenLoc, (yyvsp[-1].sc_decl)); }
#line 2805 "parser.c"
    break;

  case 184: /* discard_statement: DISCARD_SY ';'  */
#line 923 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewDiscardStmt(Cg->tokenLoc, NULL); }
#line 2811 "parser.c"
    break;

  case 185: /* discard_statement: DISCARD_SY expression ';'  */
#line 925 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewDiscardStmt(Cg->tokenLoc, CheckBooleanExpr(Cg->tokenLoc, (yyvsp[-1].sc_expr), 1)); }
#line 2817 "parser.c"
    break;

  case 186: /* jump_statement: BREAK_SY ';'  */
#line 933 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewSimpleStmt(Cg->tokenLoc, BREAK_STMT); }
#line 2823 "parser.c"
    break;

  case 187: /* jump_statement: CONTINUE_SY ';'  */
#line 935 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewSimpleStmt(Cg->tokenLoc, CONTINUE_STMT); }
#line 2829 "parser.c"
    break;

  case 188: /* if_statement: if_header balanced_statement ELSE_SY balanced_statement  */
#line 943 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-3].sc_stmt), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2835 "parser.c"
    break;

  case 189: /* dangling_if: if_header statement  */
#line 947 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-1].sc_stmt), (yyvsp[0].sc_stmt), NULL); }
#line 2841 "parser.c"
    break;

  case 190: /* dangling_if: if_header balanced_statement ELSE_SY dangling_statement  */
#line 949 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-3].sc_stmt), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2847 "parser.c"
    break;

  case 191: /* if_header: IF_SY '(' boolean_scalar_expression ')'  */
#line 953 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewIfStmt(Cg->tokenLoc, (yyvsp[-1].sc_expr), NULL, NULL); ; }
#line 2853 "parser.c"
    break;

  case 192: /* compound_statement: compound_header block_item_list compound_tail  */
#line 961 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewBlockStmt(Cg->tokenLoc, (yyvsp[-1].sc_stmt)); }
#line 2859 "parser.c"
    break;

  case 193: /* compound_statement: compound_header compound_tail  */
#line 963 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2865 "parser.c"
    break;

  case 194: /* compound_header: '{'  */
#line 967 "parser.y"
                              { PushScope(NewScope()); CurrentScope->funindex = NextFunctionIndex; }
#line 2871 "parser.c"
    break;

  case 195: /* compound_tail: '}'  */
#line 971 "parser.y"
                              {
                                if (Cg->options.DumpParseTree)
                                    PrintScopeDeclarations();
                                PopScope();
                              }
#line 2881 "parser.c"
    break;

  case 197: /* block_item_list: block_item_list block_item  */
#line 980 "parser.y"
                              { (yyval.sc_stmt) = AddStmt((yyvsp[-1].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2887 "parser.c"
    break;

  case 199: /* block_item: statement  */
#line 985 "parser.y"
                              { (yyval.sc_stmt) = CheckStmt((yyvsp[0].sc_stmt)); }
#line 2893 "parser.c"
    break;

  case 201: /* expression_statement: ';'  */
#line 994 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2899 "parser.c"
    break;

  case 202: /* expression_statement2: postfix_expression '=' expression  */
#line 998 "parser.y"
                              { (yyval.sc_stmt) = NewSimpleAssignmentStmt(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2905 "parser.c"
    break;

  case 203: /* expression_statement2: expression  */
#line 1000 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewExprStmt(Cg->tokenLoc, (yyvsp[0].sc_expr)); }
#line 2911 "parser.c"
    break;

  case 204: /* expression_statement2: postfix_expression ASSIGNMINUS_SY expression  */
#line 1002 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMINUS_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2917 "parser.c"
    break;

  case 205: /* expression_statement2: postfix_expression ASSIGNMOD_SY expression  */
#line 1004 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMOD_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2923 "parser.c"
    break;

  case 206: /* expression_statement2: postfix_expression ASSIGNPLUS_SY expression  */
#line 1006 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNPLUS_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2929 "parser.c"
    break;

  case 207: /* expression_statement2: postfix_expression ASSIGNSLASH_SY expression  */
#line 1008 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSLASH_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2935 "parser.c"
    break;

  case 208: /* expression_statement2: postfix_expression ASSIGNSTAR_SY expression  */
#line 1010 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSTAR_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2941 "parser.c"
    break;

  case 209: /* iteration_statement: WHILE_SY '(' boolean_scalar_expression ')' balanced_statement  */
#line 1018 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, (yyvsp[-2].sc_expr), (yyvsp[0].sc_stmt)); }
#line 2947 "parser.c"
    break;

  case 210: /* iteration_statement: DO_SY statement WHILE_SY '(' boolean_scalar_expression ')' ';'  */
#line 1020 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, DO_STMT, (yyvsp[-2].sc_expr), (yyvsp[-5].sc_stmt)); }
#line 2953 "parser.c"
    break;

  case 211: /* iteration_statement: FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' balanced_statement  */
#line 1022 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewForStmt(Cg->tokenLoc, (yyvsp[-6].sc_stmt), (yyvsp[-4].sc_expr), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2959 "parser.c"
    break;

  case 212: /* dangling_iteration: WHILE_SY '(' boolean_scalar_expression ')' dangling_statement  */
#line 1026 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, (yyvsp[-2].sc_expr), (yyvsp[0].sc_stmt)); }
#line 2965 "parser.c"
    break;

  case 213: /* dangling_iteration: FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' dangling_statement  */
#line 1028 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewForStmt(Cg->tokenLoc, (yyvsp[-6].sc_stmt), (yyvsp[-4].sc_expr), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2971 "parser.c"
    break;

  case 214: /* boolean_scalar_expression: expression  */
#line 1033 "parser.y"
                              {  (yyval.sc_expr) = CheckBooleanExpr(Cg->tokenLoc, (yyvsp[0].sc_expr), 0); }
#line 2977 "parser.c"
    break;

  case 216: /* for_expression_opt: %empty  */
#line 1038 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2983 "parser.c"
    break;

  case 218: /* for_expression: for_expression ',' expression_statement2  */
#line 1043 "parser.y"
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
#line 2999 "parser.c"
    break;

  case 220: /* boolean_expression_opt: %empty  */
#line 1058 "parser.y"
                              { (yyval.sc_expr) = NULL; }
#line 3005 "parser.c"
    break;

  case 221: /* return_statement: RETURN_SY expression ';'  */
#line 1066 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, (yyvsp[-1].sc_expr)); }
#line 3011 "parser.c"
    break;

  case 222: /* return_statement: RETURN_SY ';'  */
#line 1068 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, NULL); }
#line 3017 "parser.c"
    break;

  case 228: /* identifier: IDENT_SY  */
#line 1091 "parser.y"
                              { (yyval.sc_ident) = (yyvsp[0].sc_ident); }
#line 3023 "parser.c"
    break;

  case 229: /* identifier: RESERVED_SY  */
#line 1093 "parser.y"
                              {
                                /* SemanticError, not SemanticParseError: the
                                 * latter is gated by AllowSemanticParseErrors */
                                SemanticError(Cg->tokenLoc, ERROR_S_RESERVED_WORD,
                                              GetAtomString(atable, (yyvsp[0].sc_token)));
                                (yyval.sc_ident) = (yyvsp[0].sc_token);
                              }
#line 3035 "parser.c"
    break;

  case 230: /* constant: INTCONST_SY  */
#line 1103 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(ICONST_OP, &(yyvsp[0].sc_literal)); }
#line 3041 "parser.c"
    break;

  case 231: /* constant: CFLOATCONST_SY  */
#line 1105 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3047 "parser.c"
    break;

  case 232: /* constant: FLOATCONST_SY  */
#line 1107 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3053 "parser.c"
    break;

  case 233: /* constant: FLOATHCONST_SY  */
#line 1109 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3059 "parser.c"
    break;

  case 234: /* constant: FLOATXCONST_SY  */
#line 1111 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3065 "parser.c"
    break;


#line 3069 "parser.c"

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

#line 1125 "parser.y"


