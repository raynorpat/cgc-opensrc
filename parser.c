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


#line 124 "parser.c"

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
  YYSYMBOL_FIRST_USER_TOKEN_SY = 61,       /* FIRST_USER_TOKEN_SY  */
  YYSYMBOL_62_ = 62,                       /* ';'  */
  YYSYMBOL_63_ = 63,                       /* ','  */
  YYSYMBOL_64_ = 64,                       /* '='  */
  YYSYMBOL_65_ = 65,                       /* '}'  */
  YYSYMBOL_66_ = 66,                       /* ':'  */
  YYSYMBOL_67_ = 67,                       /* '<'  */
  YYSYMBOL_68_ = 68,                       /* '>'  */
  YYSYMBOL_69_ = 69,                       /* '['  */
  YYSYMBOL_70_ = 70,                       /* ']'  */
  YYSYMBOL_71_ = 71,                       /* ')'  */
  YYSYMBOL_72_ = 72,                       /* '('  */
  YYSYMBOL_73_ = 73,                       /* '{'  */
  YYSYMBOL_74_ = 74,                       /* '.'  */
  YYSYMBOL_75_ = 75,                       /* '+'  */
  YYSYMBOL_76_ = 76,                       /* '-'  */
  YYSYMBOL_77_ = 77,                       /* '!'  */
  YYSYMBOL_78_ = 78,                       /* '~'  */
  YYSYMBOL_79_ = 79,                       /* '*'  */
  YYSYMBOL_80_ = 80,                       /* '/'  */
  YYSYMBOL_81_ = 81,                       /* '%'  */
  YYSYMBOL_82_ = 82,                       /* '&'  */
  YYSYMBOL_83_ = 83,                       /* '^'  */
  YYSYMBOL_84_ = 84,                       /* '|'  */
  YYSYMBOL_85_ = 85,                       /* '?'  */
  YYSYMBOL_YYACCEPT = 86,                  /* $accept  */
  YYSYMBOL_compilation_unit = 87,          /* compilation_unit  */
  YYSYMBOL_external_declaration = 88,      /* external_declaration  */
  YYSYMBOL_declaration = 89,               /* declaration  */
  YYSYMBOL_abstract_declaration = 90,      /* abstract_declaration  */
  YYSYMBOL_declaration_specifiers = 91,    /* declaration_specifiers  */
  YYSYMBOL_abstract_declaration_specifiers = 92, /* abstract_declaration_specifiers  */
  YYSYMBOL_abstract_declaration_specifiers2 = 93, /* abstract_declaration_specifiers2  */
  YYSYMBOL_init_declarator_list = 94,      /* init_declarator_list  */
  YYSYMBOL_init_declarator = 95,           /* init_declarator  */
  YYSYMBOL_type_specifier = 96,            /* type_specifier  */
  YYSYMBOL_type_qualifier = 97,            /* type_qualifier  */
  YYSYMBOL_type_domain = 98,               /* type_domain  */
  YYSYMBOL_storage_class = 99,             /* storage_class  */
  YYSYMBOL_function_specifier = 100,       /* function_specifier  */
  YYSYMBOL_in_out = 101,                   /* in_out  */
  YYSYMBOL_struct_or_connector_specifier = 102, /* struct_or_connector_specifier  */
  YYSYMBOL_struct_compound_header = 103,   /* struct_compound_header  */
  YYSYMBOL_struct_or_connector_header = 104, /* struct_or_connector_header  */
  YYSYMBOL_struct_identifier = 105,        /* struct_identifier  */
  YYSYMBOL_untagged_struct_header = 106,   /* untagged_struct_header  */
  YYSYMBOL_struct_declaration_list = 107,  /* struct_declaration_list  */
  YYSYMBOL_struct_declaration = 108,       /* struct_declaration  */
  YYSYMBOL_annotation = 109,               /* annotation  */
  YYSYMBOL_110_1 = 110,                    /* $@1  */
  YYSYMBOL_annotation_decl_list = 111,     /* annotation_decl_list  */
  YYSYMBOL_declarator = 112,               /* declarator  */
  YYSYMBOL_semantic_declarator = 113,      /* semantic_declarator  */
  YYSYMBOL_basic_declarator = 114,         /* basic_declarator  */
  YYSYMBOL_function_decl_header = 115,     /* function_decl_header  */
  YYSYMBOL_abstract_declarator = 116,      /* abstract_declarator  */
  YYSYMBOL_parameter_list = 117,           /* parameter_list  */
  YYSYMBOL_parameter_declaration = 118,    /* parameter_declaration  */
  YYSYMBOL_abstract_parameter_list = 119,  /* abstract_parameter_list  */
  YYSYMBOL_non_empty_abstract_parameter_list = 120, /* non_empty_abstract_parameter_list  */
  YYSYMBOL_initializer = 121,              /* initializer  */
  YYSYMBOL_initializer_list = 122,         /* initializer_list  */
  YYSYMBOL_variable = 123,                 /* variable  */
  YYSYMBOL_basic_variable = 124,           /* basic_variable  */
  YYSYMBOL_primary_expression = 125,       /* primary_expression  */
  YYSYMBOL_postfix_expression = 126,       /* postfix_expression  */
  YYSYMBOL_actual_argument_list = 127,     /* actual_argument_list  */
  YYSYMBOL_non_empty_argument_list = 128,  /* non_empty_argument_list  */
  YYSYMBOL_expression_list = 129,          /* expression_list  */
  YYSYMBOL_unary_expression = 130,         /* unary_expression  */
  YYSYMBOL_cast_expression = 131,          /* cast_expression  */
  YYSYMBOL_multiplicative_expression = 132, /* multiplicative_expression  */
  YYSYMBOL_additive_expression = 133,      /* additive_expression  */
  YYSYMBOL_shift_expression = 134,         /* shift_expression  */
  YYSYMBOL_relational_expression = 135,    /* relational_expression  */
  YYSYMBOL_equality_expression = 136,      /* equality_expression  */
  YYSYMBOL_AND_expression = 137,           /* AND_expression  */
  YYSYMBOL_exclusive_OR_expression = 138,  /* exclusive_OR_expression  */
  YYSYMBOL_inclusive_OR_expression = 139,  /* inclusive_OR_expression  */
  YYSYMBOL_logical_AND_expression = 140,   /* logical_AND_expression  */
  YYSYMBOL_logical_OR_expression = 141,    /* logical_OR_expression  */
  YYSYMBOL_conditional_expression = 142,   /* conditional_expression  */
  YYSYMBOL_conditional_test = 143,         /* conditional_test  */
  YYSYMBOL_expression = 144,               /* expression  */
  YYSYMBOL_function_definition = 145,      /* function_definition  */
  YYSYMBOL_function_definition_header = 146, /* function_definition_header  */
  YYSYMBOL_statement = 147,                /* statement  */
  YYSYMBOL_balanced_statement = 148,       /* balanced_statement  */
  YYSYMBOL_dangling_statement = 149,       /* dangling_statement  */
  YYSYMBOL_discard_statement = 150,        /* discard_statement  */
  YYSYMBOL_jump_statement = 151,           /* jump_statement  */
  YYSYMBOL_if_statement = 152,             /* if_statement  */
  YYSYMBOL_dangling_if = 153,              /* dangling_if  */
  YYSYMBOL_if_header = 154,                /* if_header  */
  YYSYMBOL_compound_statement = 155,       /* compound_statement  */
  YYSYMBOL_compound_header = 156,          /* compound_header  */
  YYSYMBOL_compound_tail = 157,            /* compound_tail  */
  YYSYMBOL_block_item_list = 158,          /* block_item_list  */
  YYSYMBOL_block_item = 159,               /* block_item  */
  YYSYMBOL_expression_statement = 160,     /* expression_statement  */
  YYSYMBOL_expression_statement2 = 161,    /* expression_statement2  */
  YYSYMBOL_iteration_statement = 162,      /* iteration_statement  */
  YYSYMBOL_dangling_iteration = 163,       /* dangling_iteration  */
  YYSYMBOL_boolean_scalar_expression = 164, /* boolean_scalar_expression  */
  YYSYMBOL_for_expression_opt = 165,       /* for_expression_opt  */
  YYSYMBOL_for_expression = 166,           /* for_expression  */
  YYSYMBOL_boolean_expression_opt = 167,   /* boolean_expression_opt  */
  YYSYMBOL_return_statement = 168,         /* return_statement  */
  YYSYMBOL_member_identifier = 169,        /* member_identifier  */
  YYSYMBOL_scope_identifier = 170,         /* scope_identifier  */
  YYSYMBOL_semantics_identifier = 171,     /* semantics_identifier  */
  YYSYMBOL_type_identifier = 172,          /* type_identifier  */
  YYSYMBOL_variable_identifier = 173,      /* variable_identifier  */
  YYSYMBOL_identifier = 174,               /* identifier  */
  YYSYMBOL_constant = 175                  /* constant  */
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
#define YYFINAL  46
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1385

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  86
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  90
/* YYNRULES -- Number of rules.  */
#define YYNRULES  212
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  335

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   316


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
       2,     2,     2,    77,     2,     2,     2,    81,    82,     2,
      72,    71,    79,    75,    63,    76,    74,    80,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    66,    62,
      67,    64,    68,    85,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    69,     2,    70,    83,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    73,    84,    65,    78,     2,     2,     2,
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
      56,    57,    58,    59,    60,    61,     2
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   247,   247,   248,   255,   257,   260,   262,   264,   268,
     275,   277,   282,   284,   286,   288,   290,   292,   294,   299,
     301,   303,   305,   307,   309,   311,   315,   317,   321,   323,
     331,   333,   335,   337,   339,   341,   343,   345,   357,   365,
     373,   375,   383,   385,   393,   395,   397,   406,   408,   410,
     414,   419,   421,   425,   426,   429,   433,   434,   437,   456,
     456,   461,   462,   469,   471,   475,   477,   481,   483,   485,
     487,   489,   493,   498,   499,   501,   518,   520,   524,   526,
     531,   532,   535,   541,   554,   556,   558,   562,   564,   576,
     578,   582,   590,   591,   592,   594,   602,   603,   605,   607,
     609,   611,   616,   617,   620,   622,   626,   628,   636,   637,
     639,   641,   643,   645,   647,   655,   659,   667,   668,   670,
     672,   680,   681,   683,   691,   692,   694,   702,   703,   705,
     707,   709,   717,   718,   720,   728,   729,   737,   738,   746,
     747,   755,   756,   764,   765,   773,   774,   778,   786,   797,
     799,   803,   811,   812,   815,   816,   817,   818,   819,   820,
     821,   824,   825,   832,   834,   842,   844,   852,   856,   858,
     862,   870,   872,   876,   880,   888,   889,   893,   894,   902,
     903,   907,   909,   911,   913,   915,   917,   919,   927,   929,
     931,   935,   937,   942,   946,   948,   951,   952,   966,   968,
     975,   977,   985,   988,   991,   994,   997,  1000,  1003,  1005,
    1009,  1013,  1017
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
  "VARYING_SY", "VOID_SY", "WHILE_SY", "FIRST_USER_TOKEN_SY", "';'", "','",
  "'='", "'}'", "':'", "'<'", "'>'", "'['", "']'", "')'", "'('", "'{'",
  "'.'", "'+'", "'-'", "'!'", "'~'", "'*'", "'/'", "'%'", "'&'", "'^'",
  "'|'", "'?'", "$accept", "compilation_unit", "external_declaration",
  "declaration", "abstract_declaration", "declaration_specifiers",
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

#define YYPACT_NINF (-239)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-204)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    1227,  -239,  -239,  -239,   -25,  -239,  -239,  -239,  -239,  -239,
    -239,  -239,  -239,  1301,  -239,   -13,  -239,  1301,  -239,  -239,
    -239,  1176,  -239,  -239,   -16,  -239,  1328,  -239,  1301,  1301,
    1301,  1301,  1301,  -239,   -31,   -31,  -239,   286,  -239,  -239,
    -239,  -239,    -9,  -239,  -239,  -239,  -239,  -239,  -239,    28,
    -239,    -5,     0,    27,   151,  -239,  -239,  -239,  -239,  -239,
    -239,  -239,  -239,  -239,  -239,  -239,  -239,  -239,  1227,  -239,
    1227,     8,  -239,    21,   633,   493,  -239,  -239,  -239,    36,
      43,  -239,   927,   927,   675,    50,  -239,  -239,   549,   927,
     927,   927,   927,  -239,   -16,    65,  -239,  -239,  -239,   209,
    -239,  -239,    25,    58,    24,    11,    19,     7,    14,    16,
     127,   -30,  -239,    55,  -239,  -239,  -239,  -239,  -239,  -239,
    -239,  -239,   493,  -239,   355,   424,  -239,  -239,    89,  -239,
    -239,  -239,   140,  -239,   142,  -239,   130,  -239,   130,   717,
    -239,  -239,  -239,   130,   -20,  -239,  -239,   130,   131,    10,
    -239,    97,   106,  -239,  1065,  -239,  1116,  -239,  -239,  -239,
      65,    67,   109,   113,   759,   969,   969,  -239,  -239,  -239,
     117,   969,   110,  -239,   111,  -239,  -239,  -239,  -239,   119,
     969,   969,   969,   969,   969,   969,  -239,  -239,   969,   969,
     801,   130,   969,   969,   969,   969,   969,   969,   969,   969,
     969,   969,   969,   969,   969,   969,   969,   969,   969,   969,
     969,  -239,   156,  -239,  -239,   355,  -239,  -239,  -239,   130,
    -239,  -239,  -239,   717,  -239,  -239,  -239,  -239,   114,  -239,
     125,   122,  1264,  -239,  -239,  1301,  -239,  -239,  -239,  -239,
     120,  -239,   132,   135,  -239,   124,  -239,   128,   969,  -239,
      13,  -239,  -239,  -239,  -239,  -239,  -239,  -239,   123,   134,
     138,  -239,  -239,  -239,  -239,  -239,  -239,    25,    25,    58,
      58,    24,    24,    24,    24,    11,    11,    19,     7,    14,
      16,   127,   137,   493,  -239,  -239,  -239,  -239,    17,  1014,
    -239,   717,    -7,  -239,  -239,   969,   843,   969,  -239,   493,
    -239,   969,  -239,  -239,  -239,   969,   969,  -239,  -239,   591,
    -239,  -239,  -239,  -239,   139,  -239,   141,  -239,   149,  -239,
    -239,  -239,  -239,  -239,  -239,  -239,  -239,  -239,   159,   885,
    -239,   152,   493,  -239,  -239
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    37,    33,    38,     0,    41,    31,    44,    42,    46,
      30,    43,    45,     0,    40,    55,    34,     0,   205,    39,
      32,     0,     2,     4,     0,    10,    12,    19,     0,     0,
       0,     0,     0,    35,    49,     0,     5,     0,    36,     8,
      18,   207,    51,    54,    53,    11,     1,     3,     6,     0,
      26,    28,    63,    65,     0,    67,    25,    20,    22,    21,
      24,    23,    13,    15,    14,    17,    16,   173,     0,    50,
       0,     0,   209,     0,     0,     0,   210,   211,   212,     0,
       0,   208,     0,     0,     0,     0,   180,   150,     0,     0,
       0,     0,     0,   177,     0,    19,    92,    89,    96,   108,
     115,   117,   121,   124,   127,   132,   135,   137,   139,   141,
     143,   145,   148,     0,   182,   178,   152,   153,   155,   159,
     158,   161,     0,   154,     0,     0,   175,   156,     0,   157,
     162,   160,     0,    91,   206,    93,     0,     7,     0,     0,
     151,    59,    64,     0,     0,    72,    82,     0,    73,     0,
      76,     0,    81,    58,     0,    56,     0,   165,   166,   163,
       0,   108,     0,     0,     0,     0,     0,   110,   109,   201,
       0,     0,     0,    73,     0,   111,   112,   113,   114,    28,
       0,     0,     0,     0,     0,     0,    98,    97,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   168,   152,   174,   172,     0,   149,   176,   179,     0,
      52,   204,    27,     0,    29,    84,    61,    66,     0,    69,
      78,     9,     0,    70,    71,     0,    47,    57,    48,   164,
       0,   196,     0,   194,   193,     0,   200,     0,     0,    94,
       0,   106,   183,   184,   185,   186,   187,   181,     0,     0,
     103,   104,    99,   202,   118,   119,   120,   122,   123,   126,
     125,   131,   130,   128,   129,   133,   134,   136,   138,   140,
     142,   144,     0,     0,   171,    90,   206,    87,     0,     0,
      68,     0,     0,    77,    83,     0,     0,     0,   170,     0,
     116,     0,    95,   100,   101,     0,     0,   167,   169,     0,
      85,    60,    62,    79,     0,    75,     0,   198,     0,   197,
     188,   191,   107,   105,   146,    86,    88,    74,     0,     0,
     189,     0,     0,   190,   192
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -239,  -239,   203,     1,   -82,     2,    -2,  -239,  -239,    87,
       4,   200,   202,   204,   212,   214,  -239,   194,  -239,  -239,
    -239,   165,   -90,  -239,  -239,  -239,   -12,  -239,  -239,  -239,
    -239,  -239,    12,  -239,  -239,  -214,  -239,  -239,    23,  -239,
     -37,  -239,  -239,  -239,   175,  -145,   -53,   -50,   -89,   -59,
      38,    41,    42,    44,    45,  -239,   -56,  -239,   -64,  -239,
    -239,   -62,  -119,  -238,  -239,  -239,  -239,  -239,  -239,  -239,
     115,    40,   129,  -117,  -239,  -159,  -239,  -239,  -164,   -70,
    -239,  -239,  -239,  -239,  -239,   118,   248,  -239,    29,  -239
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    21,    22,    93,   146,    94,    25,    26,    49,    50,
     160,    28,    29,    30,    31,    32,    33,    68,    34,    42,
      35,   154,   155,   142,   226,   289,   179,    52,    53,    54,
     231,   149,   150,   151,   152,   224,   288,    96,    97,    98,
     161,   259,   260,   250,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,    36,
      37,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   214,   125,   126,   127,   128,   129,   130,   245,   242,
     243,   318,   131,   262,   132,   220,    38,   133,   134,   135
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      99,    23,    24,   212,    27,   241,   172,   247,   217,   287,
     162,    40,    51,   163,   209,    45,    41,    27,   228,    41,
     170,    27,    23,    24,   174,    27,    62,    63,    64,    65,
      66,   314,    27,    27,    27,    27,    27,    39,    99,   203,
     199,    95,    67,    18,    44,   308,    48,   264,   265,   266,
     229,   200,   148,    55,   197,  -147,   147,   136,    27,   139,
     211,   321,   204,   315,   237,   198,   237,   141,   140,   153,
     157,   153,    27,   232,    27,   225,   301,   313,   201,   202,
     309,   233,   310,   158,   302,    99,   173,    99,    99,   205,
     137,   138,    95,   143,   334,   326,   144,   206,   217,   145,
     207,   244,   174,   300,   192,   193,   194,   244,   164,   186,
     271,   272,   273,   274,   187,   165,   251,   252,   253,   254,
     255,   256,   171,    55,   257,   258,   261,    99,    95,    95,
     208,   316,   317,   195,   196,   230,   189,   180,   319,   190,
     210,   191,   267,   268,   275,   276,   282,   269,   270,    69,
      69,   218,     1,   294,   219,   153,  -203,   153,    27,   225,
      27,     2,    41,   -10,   307,   221,     3,    55,   234,   235,
     241,   239,   221,   240,     5,     6,    55,   283,    99,   246,
     320,   248,   249,   139,   290,     7,     8,     9,    10,   291,
      11,   292,   295,   303,   296,   298,    12,    13,   297,   299,
      14,   305,    15,   306,    16,   304,    17,    18,    19,   327,
      20,   329,   328,   333,   181,   182,   183,   184,   185,    95,
     263,   330,   -80,   332,    47,   222,    57,   225,    58,    70,
      59,   244,   244,   173,   147,   156,    27,   322,    60,    27,
      61,   323,   285,   277,   293,   225,    99,   278,   286,   279,
     324,   186,   280,   215,   281,   284,   187,   167,   168,   331,
      99,   227,    99,    43,   175,   176,   177,   178,     0,     0,
       0,     0,     0,   188,     0,     0,     0,     0,   189,     0,
       0,   190,     0,   191,     0,     0,     0,     1,     0,     0,
     312,     0,    99,    27,     0,    99,     2,    71,     0,    72,
       0,     3,    73,     0,    74,    75,     0,     0,     4,     5,
       6,    76,    77,    78,    79,     0,     0,     0,    41,    80,
       7,     8,     9,    10,    81,    11,     0,     0,    82,     0,
       0,    12,    13,    83,    84,    14,     0,    15,     0,    16,
       0,    17,    18,    19,     0,    20,    85,     0,    86,     0,
       0,    87,     0,     0,     0,     0,     1,     0,    88,    67,
       0,    89,    90,    91,    92,     2,    71,     0,    72,     0,
       3,    73,     0,    74,    75,     0,     0,     4,     5,     6,
      76,    77,    78,    79,     0,     0,     0,    41,    80,     7,
       8,     9,    10,    81,    11,     0,     0,    82,     0,     0,
      12,    13,    83,    84,    14,     0,    15,     0,    16,     0,
      17,    18,    19,     0,    20,    85,     0,    86,     0,     0,
     213,     0,     0,     0,     0,     1,     0,    88,    67,     0,
      89,    90,    91,    92,     2,    71,     0,    72,     0,     3,
      73,     0,    74,    75,     0,     0,     4,     5,     6,    76,
      77,    78,    79,     0,     0,     0,    41,    80,     7,     8,
       9,    10,    81,    11,     0,     0,    82,     0,     0,    12,
      13,    83,    84,    14,     0,    15,     0,    16,     0,    17,
      18,    19,     0,    20,    85,     0,    86,     0,     0,   216,
       0,     0,     0,     0,     1,     0,    88,    67,     0,    89,
      90,    91,    92,     2,    71,     0,    72,     0,     0,    73,
       0,    74,    75,     0,     0,     0,     0,     6,    76,    77,
      78,    79,     0,     0,     0,    41,    80,     0,     0,     0,
      10,    81,     0,     0,     0,    82,     0,     0,     0,     0,
      83,    84,     0,     0,    15,     0,    16,     0,     0,    18,
       1,     0,    20,    85,     0,    86,     0,     0,     0,     2,
       0,     0,    72,     0,     3,    88,    67,     0,    89,    90,
      91,    92,     5,     6,    76,    77,    78,     0,     0,     0,
       0,    41,     0,     7,     8,     9,    10,    81,    11,     0,
       0,    82,     1,     0,    12,    13,    83,     0,    14,     0,
      15,     2,    16,     0,    72,    18,    19,     0,    20,     0,
       0,     0,     0,     0,     0,     6,    76,    77,    78,     0,
       0,    88,     0,    41,    89,    90,    91,    92,    10,    81,
       0,     0,     0,    82,     1,     0,     0,     0,    83,     0,
       0,     0,    15,     2,    16,     0,    72,    18,     0,     0,
      20,     0,     0,     0,     0,     0,   325,     6,    76,    77,
      78,     0,     0,    88,   223,    41,    89,    90,    91,    92,
      10,    81,     0,     0,     0,    82,     1,     0,     0,     0,
      83,     0,     0,     0,    15,     2,    16,     0,    72,    18,
       0,     0,    20,     0,     0,   159,     0,     0,     0,     6,
      76,    77,    78,     0,     0,    88,     0,    41,    89,    90,
      91,    92,    10,    81,     0,     0,     0,    82,     1,     0,
       0,     0,    83,     0,     0,     0,    15,     2,    16,     0,
      72,    18,     0,     0,    20,     0,     0,   169,     0,     0,
       0,     6,    76,    77,    78,     0,     0,    88,     0,    41,
      89,    90,    91,    92,    10,    81,     0,     0,     0,    82,
       1,     0,     0,     0,    83,     0,     0,     0,    15,     2,
      16,     0,    72,    18,     0,     0,    20,     0,     0,     0,
       0,     0,     0,     6,    76,    77,    78,     0,     0,    88,
     223,    41,    89,    90,    91,    92,    10,    81,     0,     0,
       0,    82,     1,     0,     0,     0,    83,     0,     0,     0,
      15,     2,    16,     0,    72,    18,     0,     0,    20,     0,
       0,  -195,     0,     0,     0,     6,    76,    77,    78,     0,
       0,    88,     0,    41,    89,    90,    91,    92,    10,    81,
       0,     0,     0,    82,     1,     0,     0,     0,    83,     0,
       0,     0,    15,     2,    16,     0,    72,    18,     0,     0,
      20,     0,     0,     0,     0,     0,     0,     6,    76,    77,
      78,     0,  -102,    88,     0,    41,    89,    90,    91,    92,
      10,    81,     0,     0,     0,    82,     1,     0,     0,     0,
      83,     0,     0,     0,    15,     2,    16,     0,    72,    18,
       0,     0,    20,     0,     0,  -199,     0,     0,     0,     6,
      76,    77,    78,     0,     0,    88,     0,    41,    89,    90,
      91,    92,    10,    81,     0,     0,     0,    82,     1,     0,
       0,     0,    83,     0,     0,     0,    15,     2,    16,     0,
      72,    18,     0,     0,    20,     0,     0,     0,     0,     0,
       0,     6,    76,    77,    78,     0,  -195,    88,     0,    41,
      89,    90,    91,    92,    10,    81,     0,     0,     0,    82,
       1,     0,     0,     0,    83,     0,     0,     0,    15,     2,
      16,     0,    72,    18,     0,     0,    20,     0,     0,     0,
       0,     0,     0,     6,    76,    77,    78,     0,     0,   166,
       0,    41,    89,    90,    91,    92,    10,    81,     0,     0,
       0,    82,     0,     0,     0,     1,    83,     0,     0,     0,
      15,     0,    16,     0,     2,    18,     0,     0,    20,     3,
       0,     0,     0,     0,     0,     0,     4,     5,     6,     0,
       0,    88,     0,     0,    89,    90,    91,    92,     7,     8,
       9,    10,     0,    11,     0,     0,     0,     0,     0,    12,
      13,     0,     0,    14,     0,    15,     1,    16,     0,    17,
      18,    19,     0,    20,     0,     2,     0,     0,     0,     0,
       3,     0,   311,     0,     0,     0,     0,     4,     5,     6,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     7,
       8,     9,    10,     0,    11,     0,     0,     0,     0,     0,
      12,    13,     0,     0,    14,     0,    15,     1,    16,     0,
      17,    18,    19,     0,    20,     0,     2,     0,     0,     0,
     236,     3,     0,     0,     0,     0,     0,     0,     4,     5,
       6,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       7,     8,     9,    10,     0,    11,     0,     0,     0,     0,
       0,    12,    13,     0,     0,    14,     0,    15,     0,    16,
       0,    17,    18,    19,     0,    20,    46,     1,     0,     0,
       0,   238,     0,     0,     0,     0,     2,     0,     0,     0,
       0,     3,     0,     0,     0,     0,     0,     0,     4,     5,
       6,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       7,     8,     9,    10,     0,    11,     0,     0,     0,     0,
       0,    12,    13,     0,     0,    14,     0,    15,     1,    16,
       0,    17,    18,    19,     0,    20,     0,     2,     0,     0,
       0,     0,     3,     0,     0,     0,     0,     0,     0,     4,
       5,     6,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     7,     8,     9,    10,     1,    11,     0,     0,     0,
       0,     0,    12,    13,     2,     0,    14,     0,    15,     3,
      16,     0,    17,    18,    19,     0,    20,     5,     6,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     7,     8,
       9,    10,     1,    11,     0,     0,     0,     0,     0,    12,
      13,     2,     0,    14,     0,    15,     3,    16,     0,    17,
      18,    19,     0,    20,     5,     6,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     7,     8,     9,    10,     0,
      11,     0,     0,     3,     0,     0,    12,    13,     0,     0,
      14,     5,    15,     0,    16,     0,     0,    18,    19,     0,
      20,     0,     7,     8,     9,     0,     0,    11,     0,     0,
       0,     0,     0,    12,    56,     0,     0,    14,     0,     0,
       0,     0,     0,     0,     0,    19
};

static const yytype_int16 yycheck[] =
{
      37,     0,     0,   122,     0,   164,    88,   171,   125,   223,
      74,    13,    24,    75,    44,    17,    32,    13,    38,    32,
      84,    17,    21,    21,    88,    21,    28,    29,    30,    31,
      32,    38,    28,    29,    30,    31,    32,    62,    75,    20,
      29,    37,    73,    56,    15,   283,    62,   192,   193,   194,
      70,    40,    54,    24,    30,    85,    54,    66,    54,    64,
     122,   299,    43,    70,   154,    41,   156,    67,    73,    68,
      62,    70,    68,    63,    70,   139,    63,   291,    67,    68,
      63,    71,    65,    62,    71,   122,    88,   124,   125,    82,
      62,    63,    88,    66,   332,   309,    69,    83,   215,    72,
      84,   165,   166,   248,    79,    80,    81,   171,    72,    42,
     199,   200,   201,   202,    47,    72,   180,   181,   182,   183,
     184,   185,    72,    94,   188,   189,   190,   164,   124,   125,
       3,   295,   296,    75,    76,   147,    69,    72,   297,    72,
      85,    74,   195,   196,   203,   204,   210,   197,   198,    34,
      35,    62,     1,   235,    14,   154,    14,   156,   154,   223,
     156,    10,    32,    32,   283,   136,    15,   138,    71,    63,
     329,    62,   143,    60,    23,    24,   147,    21,   215,    62,
     299,    71,    71,    64,    70,    34,    35,    36,    37,    64,
      39,    69,    72,    70,    62,    71,    45,    46,    63,    71,
      49,    63,    51,    66,    53,    71,    55,    56,    57,    70,
      59,    62,    71,   332,     5,     6,     7,     8,     9,   215,
     191,    62,    71,    71,    21,   138,    26,   291,    26,    35,
      26,   295,   296,   235,   232,    70,   232,   301,    26,   235,
      26,   305,   219,   205,   232,   309,   283,   206,   219,   207,
     306,    42,   208,   124,   209,   215,    47,    82,    83,   329,
     297,   143,   299,    15,    89,    90,    91,    92,    -1,    -1,
      -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    69,    -1,
      -1,    72,    -1,    74,    -1,    -1,    -1,     1,    -1,    -1,
     289,    -1,   329,   289,    -1,   332,    10,    11,    -1,    13,
      -1,    15,    16,    -1,    18,    19,    -1,    -1,    22,    23,
      24,    25,    26,    27,    28,    -1,    -1,    -1,    32,    33,
      34,    35,    36,    37,    38,    39,    -1,    -1,    42,    -1,
      -1,    45,    46,    47,    48,    49,    -1,    51,    -1,    53,
      -1,    55,    56,    57,    -1,    59,    60,    -1,    62,    -1,
      -1,    65,    -1,    -1,    -1,    -1,     1,    -1,    72,    73,
      -1,    75,    76,    77,    78,    10,    11,    -1,    13,    -1,
      15,    16,    -1,    18,    19,    -1,    -1,    22,    23,    24,
      25,    26,    27,    28,    -1,    -1,    -1,    32,    33,    34,
      35,    36,    37,    38,    39,    -1,    -1,    42,    -1,    -1,
      45,    46,    47,    48,    49,    -1,    51,    -1,    53,    -1,
      55,    56,    57,    -1,    59,    60,    -1,    62,    -1,    -1,
      65,    -1,    -1,    -1,    -1,     1,    -1,    72,    73,    -1,
      75,    76,    77,    78,    10,    11,    -1,    13,    -1,    15,
      16,    -1,    18,    19,    -1,    -1,    22,    23,    24,    25,
      26,    27,    28,    -1,    -1,    -1,    32,    33,    34,    35,
      36,    37,    38,    39,    -1,    -1,    42,    -1,    -1,    45,
      46,    47,    48,    49,    -1,    51,    -1,    53,    -1,    55,
      56,    57,    -1,    59,    60,    -1,    62,    -1,    -1,    65,
      -1,    -1,    -1,    -1,     1,    -1,    72,    73,    -1,    75,
      76,    77,    78,    10,    11,    -1,    13,    -1,    -1,    16,
      -1,    18,    19,    -1,    -1,    -1,    -1,    24,    25,    26,
      27,    28,    -1,    -1,    -1,    32,    33,    -1,    -1,    -1,
      37,    38,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,
      47,    48,    -1,    -1,    51,    -1,    53,    -1,    -1,    56,
       1,    -1,    59,    60,    -1,    62,    -1,    -1,    -1,    10,
      -1,    -1,    13,    -1,    15,    72,    73,    -1,    75,    76,
      77,    78,    23,    24,    25,    26,    27,    -1,    -1,    -1,
      -1,    32,    -1,    34,    35,    36,    37,    38,    39,    -1,
      -1,    42,     1,    -1,    45,    46,    47,    -1,    49,    -1,
      51,    10,    53,    -1,    13,    56,    57,    -1,    59,    -1,
      -1,    -1,    -1,    -1,    -1,    24,    25,    26,    27,    -1,
      -1,    72,    -1,    32,    75,    76,    77,    78,    37,    38,
      -1,    -1,    -1,    42,     1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    51,    10,    53,    -1,    13,    56,    -1,    -1,
      59,    -1,    -1,    -1,    -1,    -1,    65,    24,    25,    26,
      27,    -1,    -1,    72,    73,    32,    75,    76,    77,    78,
      37,    38,    -1,    -1,    -1,    42,     1,    -1,    -1,    -1,
      47,    -1,    -1,    -1,    51,    10,    53,    -1,    13,    56,
      -1,    -1,    59,    -1,    -1,    62,    -1,    -1,    -1,    24,
      25,    26,    27,    -1,    -1,    72,    -1,    32,    75,    76,
      77,    78,    37,    38,    -1,    -1,    -1,    42,     1,    -1,
      -1,    -1,    47,    -1,    -1,    -1,    51,    10,    53,    -1,
      13,    56,    -1,    -1,    59,    -1,    -1,    62,    -1,    -1,
      -1,    24,    25,    26,    27,    -1,    -1,    72,    -1,    32,
      75,    76,    77,    78,    37,    38,    -1,    -1,    -1,    42,
       1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    10,
      53,    -1,    13,    56,    -1,    -1,    59,    -1,    -1,    -1,
      -1,    -1,    -1,    24,    25,    26,    27,    -1,    -1,    72,
      73,    32,    75,    76,    77,    78,    37,    38,    -1,    -1,
      -1,    42,     1,    -1,    -1,    -1,    47,    -1,    -1,    -1,
      51,    10,    53,    -1,    13,    56,    -1,    -1,    59,    -1,
      -1,    62,    -1,    -1,    -1,    24,    25,    26,    27,    -1,
      -1,    72,    -1,    32,    75,    76,    77,    78,    37,    38,
      -1,    -1,    -1,    42,     1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    51,    10,    53,    -1,    13,    56,    -1,    -1,
      59,    -1,    -1,    -1,    -1,    -1,    -1,    24,    25,    26,
      27,    -1,    71,    72,    -1,    32,    75,    76,    77,    78,
      37,    38,    -1,    -1,    -1,    42,     1,    -1,    -1,    -1,
      47,    -1,    -1,    -1,    51,    10,    53,    -1,    13,    56,
      -1,    -1,    59,    -1,    -1,    62,    -1,    -1,    -1,    24,
      25,    26,    27,    -1,    -1,    72,    -1,    32,    75,    76,
      77,    78,    37,    38,    -1,    -1,    -1,    42,     1,    -1,
      -1,    -1,    47,    -1,    -1,    -1,    51,    10,    53,    -1,
      13,    56,    -1,    -1,    59,    -1,    -1,    -1,    -1,    -1,
      -1,    24,    25,    26,    27,    -1,    71,    72,    -1,    32,
      75,    76,    77,    78,    37,    38,    -1,    -1,    -1,    42,
       1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    10,
      53,    -1,    13,    56,    -1,    -1,    59,    -1,    -1,    -1,
      -1,    -1,    -1,    24,    25,    26,    27,    -1,    -1,    72,
      -1,    32,    75,    76,    77,    78,    37,    38,    -1,    -1,
      -1,    42,    -1,    -1,    -1,     1,    47,    -1,    -1,    -1,
      51,    -1,    53,    -1,    10,    56,    -1,    -1,    59,    15,
      -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    24,    -1,
      -1,    72,    -1,    -1,    75,    76,    77,    78,    34,    35,
      36,    37,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      46,    -1,    -1,    49,    -1,    51,     1,    53,    -1,    55,
      56,    57,    -1,    59,    -1,    10,    -1,    -1,    -1,    -1,
      15,    -1,    68,    -1,    -1,    -1,    -1,    22,    23,    24,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,
      35,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    46,    -1,    -1,    49,    -1,    51,     1,    53,    -1,
      55,    56,    57,    -1,    59,    -1,    10,    -1,    -1,    -1,
      65,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      34,    35,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    46,    -1,    -1,    49,    -1,    51,    -1,    53,
      -1,    55,    56,    57,    -1,    59,     0,     1,    -1,    -1,
      -1,    65,    -1,    -1,    -1,    -1,    10,    -1,    -1,    -1,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      34,    35,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    46,    -1,    -1,    49,    -1,    51,     1,    53,
      -1,    55,    56,    57,    -1,    59,    -1,    10,    -1,    -1,
      -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,
      23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    35,    36,    37,     1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    46,    10,    -1,    49,    -1,    51,    15,
      53,    -1,    55,    56,    57,    -1,    59,    23,    24,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    35,
      36,    37,     1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      46,    10,    -1,    49,    -1,    51,    15,    53,    -1,    55,
      56,    57,    -1,    59,    23,    24,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    34,    35,    36,    37,    -1,
      39,    -1,    -1,    15,    -1,    -1,    45,    46,    -1,    -1,
      49,    23,    51,    -1,    53,    -1,    -1,    56,    57,    -1,
      59,    -1,    34,    35,    36,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    46,    -1,    -1,    49,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    57
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    10,    15,    22,    23,    24,    34,    35,    36,
      37,    39,    45,    46,    49,    51,    53,    55,    56,    57,
      59,    87,    88,    89,    91,    92,    93,    96,    97,    98,
      99,   100,   101,   102,   104,   106,   145,   146,   172,    62,
      92,    32,   105,   172,   174,    92,     0,    88,    62,    94,
      95,   112,   113,   114,   115,   174,    46,    97,    98,    99,
     100,   101,    92,    92,    92,    92,    92,    73,   103,   156,
     103,    11,    13,    16,    18,    19,    25,    26,    27,    28,
      33,    38,    42,    47,    48,    60,    62,    65,    72,    75,
      76,    77,    78,    89,    91,    96,   123,   124,   125,   126,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   147,   148,   149,   150,   151,
     152,   153,   154,   155,   156,   158,   159,   160,   161,   162,
     163,   168,   170,   173,   174,   175,    66,    62,    63,    64,
      73,    67,   109,    66,    69,    72,    90,    91,    92,   117,
     118,   119,   120,    89,   107,   108,   107,    62,    62,    62,
      96,   126,   144,   147,    72,    72,    72,   130,   130,    62,
     144,    72,    90,    92,   144,   130,   130,   130,   130,   112,
      72,     5,     6,     7,     8,     9,    42,    47,    64,    69,
      72,    74,    79,    80,    81,    75,    76,    30,    41,    29,
      40,    67,    68,    20,    43,    82,    83,    84,     3,    44,
      85,   147,   148,    65,   157,   158,    65,   159,    62,    14,
     171,   174,    95,    73,   121,   144,   110,   171,    38,    70,
     112,   116,    63,    71,    71,    63,    65,   108,    65,    62,
      60,   161,   165,   166,   144,   164,    62,   164,    71,    71,
     129,   144,   144,   144,   144,   144,   144,   144,   144,   127,
     128,   144,   169,   174,   131,   131,   131,   132,   132,   133,
     133,   134,   134,   134,   134,   135,   135,   136,   137,   138,
     139,   140,   144,    21,   157,   124,   174,   121,   122,   111,
      70,    64,    69,   118,    90,    72,    62,    63,    71,    71,
     131,    63,    71,    70,    71,    63,    66,   148,   149,    63,
      65,    68,    89,   121,    38,    70,   164,   164,   167,   161,
     148,   149,   144,   144,   142,    65,   121,    70,    71,    62,
      62,   165,    71,   148,   149
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    86,    87,    87,    88,    88,    89,    89,    89,    90,
      91,    91,    92,    92,    92,    92,    92,    92,    92,    93,
      93,    93,    93,    93,    93,    93,    94,    94,    95,    95,
      96,    96,    96,    96,    96,    96,    96,    96,    97,    98,
      99,    99,   100,   100,   101,   101,   101,   102,   102,   102,
     103,   104,   104,   105,   105,   106,   107,   107,   108,   110,
     109,   111,   111,   112,   112,   113,   113,   114,   114,   114,
     114,   114,   115,   116,   116,   116,   117,   117,   118,   118,
     119,   119,   120,   120,   121,   121,   121,   122,   122,   123,
     123,   124,   125,   125,   125,   125,   126,   126,   126,   126,
     126,   126,   127,   127,   128,   128,   129,   129,   130,   130,
     130,   130,   130,   130,   130,   131,   131,   132,   132,   132,
     132,   133,   133,   133,   134,   134,   134,   135,   135,   135,
     135,   135,   136,   136,   136,   137,   137,   138,   138,   139,
     139,   140,   140,   141,   141,   142,   142,   143,   144,   145,
     145,   146,   147,   147,   148,   148,   148,   148,   148,   148,
     148,   149,   149,   150,   150,   151,   151,   152,   153,   153,
     154,   155,   155,   156,   157,   158,   158,   159,   159,   160,
     160,   161,   161,   161,   161,   161,   161,   161,   162,   162,
     162,   163,   163,   164,   165,   165,   166,   166,   167,   167,
     168,   168,   169,   170,   171,   172,   173,   174,   175,   175,
     175,   175,   175
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     1,     1,     2,     3,     2,     2,
       1,     2,     1,     2,     2,     2,     2,     2,     2,     1,
       2,     2,     2,     2,     2,     2,     1,     3,     1,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     4,     4,     1,
       1,     2,     4,     1,     1,     1,     1,     2,     1,     0,
       4,     0,     2,     1,     2,     1,     3,     1,     4,     3,
       3,     3,     2,     0,     4,     3,     1,     3,     2,     4,
       0,     1,     1,     3,     1,     3,     4,     1,     3,     1,
       3,     1,     1,     1,     3,     4,     1,     2,     2,     3,
       4,     4,     0,     1,     1,     3,     1,     3,     1,     2,
       2,     2,     2,     2,     2,     1,     4,     1,     3,     3,
       3,     1,     3,     3,     1,     3,     3,     1,     3,     3,
       3,     3,     1,     3,     3,     1,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     1,     5,     1,     1,     3,
       2,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     3,     2,     2,     4,     2,     4,
       4,     3,     2,     1,     1,     1,     2,     1,     1,     2,
       1,     3,     1,     3,     3,     3,     3,     3,     5,     7,
       9,     5,     9,     1,     1,     0,     1,     3,     1,     0,
       3,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1
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
#line 256 "parser.y"
                              { (yyval.dummy) = GlobalInitStatements(CurrentScope, (yyvsp[0].sc_stmt)); }
#line 1774 "parser.c"
    break;

  case 6: /* declaration: declaration_specifiers ';'  */
#line 261 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 1780 "parser.c"
    break;

  case 7: /* declaration: declaration_specifiers init_declarator_list ';'  */
#line 263 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[-1].sc_stmt); }
#line 1786 "parser.c"
    break;

  case 8: /* declaration: ERROR_SY ';'  */
#line 265 "parser.y"
                              { RecordErrorPos(Cg->tokenLoc); (yyval.sc_stmt) = NULL; }
#line 1792 "parser.c"
    break;

  case 9: /* abstract_declaration: abstract_declaration_specifiers abstract_declarator  */
#line 269 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 1798 "parser.c"
    break;

  case 10: /* declaration_specifiers: abstract_declaration_specifiers  */
#line 276 "parser.y"
                              { (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 1804 "parser.c"
    break;

  case 11: /* declaration_specifiers: TYPEDEF_SY abstract_declaration_specifiers  */
#line 278 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, TYPE_MISC_TYPEDEF); (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 1810 "parser.c"
    break;

  case 12: /* abstract_declaration_specifiers: abstract_declaration_specifiers2  */
#line 283 "parser.y"
                              { (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 1816 "parser.c"
    break;

  case 13: /* abstract_declaration_specifiers: type_qualifier abstract_declaration_specifiers  */
#line 285 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1822 "parser.c"
    break;

  case 14: /* abstract_declaration_specifiers: storage_class abstract_declaration_specifiers  */
#line 287 "parser.y"
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1828 "parser.c"
    break;

  case 15: /* abstract_declaration_specifiers: type_domain abstract_declaration_specifiers  */
#line 289 "parser.y"
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1834 "parser.c"
    break;

  case 16: /* abstract_declaration_specifiers: in_out abstract_declaration_specifiers  */
#line 291 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1840 "parser.c"
    break;

  case 17: /* abstract_declaration_specifiers: function_specifier abstract_declaration_specifiers  */
#line 293 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1846 "parser.c"
    break;

  case 18: /* abstract_declaration_specifiers: PACKED_SY abstract_declaration_specifiers  */
#line 295 "parser.y"
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1852 "parser.c"
    break;

  case 19: /* abstract_declaration_specifiers2: type_specifier  */
#line 300 "parser.y"
                              { (yyval.sc_type) = *SetDType(&CurrentDeclTypeSpecs, (yyvsp[0].sc_ptype)); }
#line 1858 "parser.c"
    break;

  case 20: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 type_qualifier  */
#line 302 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1864 "parser.c"
    break;

  case 21: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 storage_class  */
#line 304 "parser.y"
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1870 "parser.c"
    break;

  case 22: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 type_domain  */
#line 306 "parser.y"
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1876 "parser.c"
    break;

  case 23: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 in_out  */
#line 308 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1882 "parser.c"
    break;

  case 24: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 function_specifier  */
#line 310 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1888 "parser.c"
    break;

  case 25: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 PACKED_SY  */
#line 312 "parser.y"
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 1894 "parser.c"
    break;

  case 26: /* init_declarator_list: init_declarator  */
#line 316 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[0].sc_stmt); }
#line 1900 "parser.c"
    break;

  case 27: /* init_declarator_list: init_declarator_list ',' init_declarator  */
#line 318 "parser.y"
                              { (yyval.sc_stmt) = AddStmt((yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 1906 "parser.c"
    break;

  case 28: /* init_declarator: declarator  */
#line 322 "parser.y"
                              { (yyval.sc_stmt) = Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_decl), NULL); }
#line 1912 "parser.c"
    break;

  case 29: /* init_declarator: declarator '=' initializer  */
#line 324 "parser.y"
                              { (yyval.sc_stmt) = Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[0].sc_expr)); }
#line 1918 "parser.c"
    break;

  case 30: /* type_specifier: INT_SY  */
#line 332 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, INT_SY); }
#line 1924 "parser.c"
    break;

  case 31: /* type_specifier: FLOAT_SY  */
#line 334 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, FLOAT_SY); }
#line 1930 "parser.c"
    break;

  case 32: /* type_specifier: VOID_SY  */
#line 336 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, VOID_SY); }
#line 1936 "parser.c"
    break;

  case 33: /* type_specifier: BOOLEAN_SY  */
#line 338 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, BOOLEAN_SY); }
#line 1942 "parser.c"
    break;

  case 34: /* type_specifier: TEXOBJ_SY  */
#line 340 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, TEXOBJ_SY); }
#line 1948 "parser.c"
    break;

  case 35: /* type_specifier: struct_or_connector_specifier  */
#line 342 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 1954 "parser.c"
    break;

  case 36: /* type_specifier: type_identifier  */
#line 344 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, (yyvsp[0].sc_ident)); }
#line 1960 "parser.c"
    break;

  case 37: /* type_specifier: error  */
#line 346 "parser.y"
                              {
                                SemanticParseError(Cg->tokenLoc, ERROR_S_TYPE_NAME_EXPECTED,
                                                   GetAtomString(atable, Cg->mostRecentToken /* yychar */));
                                (yyval.sc_ptype) = UndefinedType;
                              }
#line 1970 "parser.c"
    break;

  case 38: /* type_qualifier: CONST_SY  */
#line 358 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_CONST; }
#line 1976 "parser.c"
    break;

  case 39: /* type_domain: UNIFORM_SY  */
#line 366 "parser.y"
                              { (yyval.sc_int) = TYPE_DOMAIN_UNIFORM; }
#line 1982 "parser.c"
    break;

  case 40: /* storage_class: STATIC_SY  */
#line 374 "parser.y"
                              { (yyval.sc_int) = (int) SC_STATIC; }
#line 1988 "parser.c"
    break;

  case 41: /* storage_class: EXTERN_SY  */
#line 376 "parser.y"
                              { (yyval.sc_int) = (int) SC_EXTERN; }
#line 1994 "parser.c"
    break;

  case 42: /* function_specifier: INLINE_SY  */
#line 384 "parser.y"
                              { (yyval.sc_int) = TYPE_MISC_INLINE; }
#line 2000 "parser.c"
    break;

  case 43: /* function_specifier: INTERNAL_SY  */
#line 386 "parser.y"
                              { (yyval.sc_int) = TYPE_MISC_INTERNAL; }
#line 2006 "parser.c"
    break;

  case 44: /* in_out: IN_SY  */
#line 394 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_IN; }
#line 2012 "parser.c"
    break;

  case 45: /* in_out: OUT_SY  */
#line 396 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_OUT; }
#line 2018 "parser.c"
    break;

  case 46: /* in_out: INOUT_SY  */
#line 398 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_INOUT; }
#line 2024 "parser.c"
    break;

  case 47: /* struct_or_connector_specifier: struct_or_connector_header struct_compound_header struct_declaration_list '}'  */
#line 407 "parser.y"
                              { (yyval.sc_ptype) = SetStructMembers(Cg->tokenLoc, (yyvsp[-3].sc_ptype), PopScope()); }
#line 2030 "parser.c"
    break;

  case 48: /* struct_or_connector_specifier: untagged_struct_header struct_compound_header struct_declaration_list '}'  */
#line 409 "parser.y"
                              { (yyval.sc_ptype) = SetStructMembers(Cg->tokenLoc, (yyvsp[-3].sc_ptype), PopScope()); }
#line 2036 "parser.c"
    break;

  case 49: /* struct_or_connector_specifier: struct_or_connector_header  */
#line 411 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2042 "parser.c"
    break;

  case 50: /* struct_compound_header: compound_header  */
#line 415 "parser.y"
                              { CurrentScope->IsStructScope = 1; (yyval.dummy) = (yyvsp[0].dummy); }
#line 2048 "parser.c"
    break;

  case 51: /* struct_or_connector_header: STRUCT_SY struct_identifier  */
#line 420 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, 0, (yyvsp[0].sc_ident)); }
#line 2054 "parser.c"
    break;

  case 52: /* struct_or_connector_header: STRUCT_SY struct_identifier ':' semantics_identifier  */
#line 422 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_ident), (yyvsp[-2].sc_ident)); }
#line 2060 "parser.c"
    break;

  case 55: /* untagged_struct_header: STRUCT_SY  */
#line 430 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, 0, 0); }
#line 2066 "parser.c"
    break;

  case 58: /* struct_declaration: declaration  */
#line 438 "parser.y"
                            { (yyval.sc_stmt) = (yyvsp[0].sc_stmt); }
#line 2072 "parser.c"
    break;

  case 59: /* $@1: %empty  */
#line 456 "parser.y"
                              { PushScope(NewScope()); }
#line 2078 "parser.c"
    break;

  case 60: /* annotation: '<' $@1 annotation_decl_list '>'  */
#line 457 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[-1].sc_stmt); PopScope(); }
#line 2084 "parser.c"
    break;

  case 61: /* annotation_decl_list: %empty  */
#line 461 "parser.y"
                              { (yyval.sc_stmt) = 0; }
#line 2090 "parser.c"
    break;

  case 63: /* declarator: semantic_declarator  */
#line 470 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2096 "parser.c"
    break;

  case 64: /* declarator: semantic_declarator annotation  */
#line 472 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[-1].sc_decl); }
#line 2102 "parser.c"
    break;

  case 65: /* semantic_declarator: basic_declarator  */
#line 476 "parser.y"
                              { (yyval.sc_decl) = Declarator(Cg->tokenLoc, (yyvsp[0].sc_decl), 0); }
#line 2108 "parser.c"
    break;

  case 66: /* semantic_declarator: basic_declarator ':' semantics_identifier  */
#line 478 "parser.y"
                              { (yyval.sc_decl) = Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), (yyvsp[0].sc_ident)); }
#line 2114 "parser.c"
    break;

  case 67: /* basic_declarator: identifier  */
#line 482 "parser.y"
                              { (yyval.sc_decl) = NewDeclNode(Cg->tokenLoc, (yyvsp[0].sc_ident), &CurrentDeclTypeSpecs); }
#line 2120 "parser.c"
    break;

  case 68: /* basic_declarator: basic_declarator '[' INTCONST_SY ']'  */
#line 484 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-3].sc_decl), (yyvsp[-1].sc_int), 0); }
#line 2126 "parser.c"
    break;

  case 69: /* basic_declarator: basic_declarator '[' ']'  */
#line 486 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), 0 , 1); }
#line 2132 "parser.c"
    break;

  case 70: /* basic_declarator: function_decl_header parameter_list ')'  */
#line 488 "parser.y"
                              { (yyval.sc_decl) = SetFunTypeParams(CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_decl), (yyvsp[-1].sc_decl)); }
#line 2138 "parser.c"
    break;

  case 71: /* basic_declarator: function_decl_header abstract_parameter_list ')'  */
#line 490 "parser.y"
                              { (yyval.sc_decl) = SetFunTypeParams(CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_decl), NULL); }
#line 2144 "parser.c"
    break;

  case 72: /* function_decl_header: basic_declarator '('  */
#line 494 "parser.y"
                              { (yyval.sc_decl) = FunctionDeclHeader(&(yyvsp[-1].sc_decl)->loc, CurrentScope, (yyvsp[-1].sc_decl)); }
#line 2150 "parser.c"
    break;

  case 73: /* abstract_declarator: %empty  */
#line 498 "parser.y"
                              { (yyval.sc_decl) = NewDeclNode(Cg->tokenLoc, 0, &CurrentDeclTypeSpecs); }
#line 2156 "parser.c"
    break;

  case 74: /* abstract_declarator: abstract_declarator '[' INTCONST_SY ']'  */
#line 500 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-3].sc_decl), (yyvsp[-1].sc_int), 0); }
#line 2162 "parser.c"
    break;

  case 75: /* abstract_declarator: abstract_declarator '[' ']'  */
#line 502 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), 0 , 1); }
#line 2168 "parser.c"
    break;

  case 76: /* parameter_list: parameter_declaration  */
#line 519 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2174 "parser.c"
    break;

  case 77: /* parameter_list: parameter_list ',' parameter_declaration  */
#line 521 "parser.y"
                              { (yyval.sc_decl) = AddDecl((yyvsp[-2].sc_decl), (yyvsp[0].sc_decl)); }
#line 2180 "parser.c"
    break;

  case 78: /* parameter_declaration: declaration_specifiers declarator  */
#line 525 "parser.y"
                              { (yyval.sc_decl) = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_decl), NULL); }
#line 2186 "parser.c"
    break;

  case 79: /* parameter_declaration: declaration_specifiers declarator '=' initializer  */
#line 527 "parser.y"
                              { (yyval.sc_decl) = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[0].sc_expr)); }
#line 2192 "parser.c"
    break;

  case 80: /* abstract_parameter_list: %empty  */
#line 531 "parser.y"
                              { (yyval.sc_decl) = NULL; }
#line 2198 "parser.c"
    break;

  case 82: /* non_empty_abstract_parameter_list: abstract_declaration  */
#line 536 "parser.y"
                              {
                                if (IsVoid(&(yyvsp[0].sc_decl)->type.type))
                                    CurrentScope->HasVoidParameter = 1;
                                (yyval.sc_decl) = (yyvsp[0].sc_decl);
                              }
#line 2208 "parser.c"
    break;

  case 83: /* non_empty_abstract_parameter_list: non_empty_abstract_parameter_list ',' abstract_declaration  */
#line 542 "parser.y"
                              {
                                if (CurrentScope->HasVoidParameter || IsVoid(&(yyvsp[-2].sc_decl)->type.type)) {
                                    SemanticError(Cg->tokenLoc, ERROR___VOID_NOT_ONLY_PARAM);
                                }
                                (yyval.sc_decl) = AddDecl((yyvsp[-2].sc_decl), (yyvsp[0].sc_decl));
                              }
#line 2219 "parser.c"
    break;

  case 84: /* initializer: expression  */
#line 555 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[0].sc_expr)); }
#line 2225 "parser.c"
    break;

  case 85: /* initializer: '{' initializer_list '}'  */
#line 557 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[-1].sc_expr)); }
#line 2231 "parser.c"
    break;

  case 86: /* initializer: '{' initializer_list ',' '}'  */
#line 559 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[-2].sc_expr)); }
#line 2237 "parser.c"
    break;

  case 87: /* initializer_list: initializer  */
#line 563 "parser.y"
                              { (yyval.sc_expr) = InitializerList(Cg->tokenLoc, (yyvsp[0].sc_expr), NULL); }
#line 2243 "parser.c"
    break;

  case 88: /* initializer_list: initializer_list ',' initializer  */
#line 565 "parser.y"
                              { (yyval.sc_expr) = InitializerList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2249 "parser.c"
    break;

  case 89: /* variable: basic_variable  */
#line 577 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[0].sc_expr); }
#line 2255 "parser.c"
    break;

  case 90: /* variable: scope_identifier COLONCOLON_SY basic_variable  */
#line 579 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[0].sc_expr); }
#line 2261 "parser.c"
    break;

  case 91: /* basic_variable: variable_identifier  */
#line 583 "parser.y"
                              { (yyval.sc_expr) = BasicVariable(Cg->tokenLoc, (yyvsp[0].sc_ident)); }
#line 2267 "parser.c"
    break;

  case 94: /* primary_expression: '(' expression ')'  */
#line 593 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[-1].sc_expr); }
#line 2273 "parser.c"
    break;

  case 95: /* primary_expression: type_specifier '(' expression_list ')'  */
#line 595 "parser.y"
                              { (yyval.sc_expr) = NewVectorConstructor(Cg->tokenLoc, (yyvsp[-3].sc_ptype), (yyvsp[-1].sc_expr)); }
#line 2279 "parser.c"
    break;

  case 97: /* postfix_expression: postfix_expression PLUSPLUS_SY  */
#line 604 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(POSTINC_OP, (yyvsp[-1].sc_expr)); }
#line 2285 "parser.c"
    break;

  case 98: /* postfix_expression: postfix_expression MINUSMINUS_SY  */
#line 606 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(POSTDEC_OP, (yyvsp[-1].sc_expr)); }
#line 2291 "parser.c"
    break;

  case 99: /* postfix_expression: postfix_expression '.' member_identifier  */
#line 608 "parser.y"
                              { (yyval.sc_expr) = NewMemberSelectorOrSwizzleOrWriteMaskOperator(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_ident)); }
#line 2297 "parser.c"
    break;

  case 100: /* postfix_expression: postfix_expression '[' expression ']'  */
#line 610 "parser.y"
                              { (yyval.sc_expr) = NewIndexOperator(Cg->tokenLoc, (yyvsp[-3].sc_expr), (yyvsp[-1].sc_expr)); }
#line 2303 "parser.c"
    break;

  case 101: /* postfix_expression: postfix_expression '(' actual_argument_list ')'  */
#line 612 "parser.y"
                              { (yyval.sc_expr) = NewFunctionCallOperator(Cg->tokenLoc, (yyvsp[-3].sc_expr), (yyvsp[-1].sc_expr)); }
#line 2309 "parser.c"
    break;

  case 102: /* actual_argument_list: %empty  */
#line 616 "parser.y"
                                { (yyval.sc_expr) = NULL; }
#line 2315 "parser.c"
    break;

  case 104: /* non_empty_argument_list: expression  */
#line 621 "parser.y"
                              { (yyval.sc_expr) = ArgumentList(Cg->tokenLoc, NULL, (yyvsp[0].sc_expr)); }
#line 2321 "parser.c"
    break;

  case 105: /* non_empty_argument_list: non_empty_argument_list ',' expression  */
#line 623 "parser.y"
                              { (yyval.sc_expr) = ArgumentList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2327 "parser.c"
    break;

  case 106: /* expression_list: expression  */
#line 627 "parser.y"
                              { (yyval.sc_expr) = ExpressionList(Cg->tokenLoc, NULL, (yyvsp[0].sc_expr)); }
#line 2333 "parser.c"
    break;

  case 107: /* expression_list: expression_list ',' expression  */
#line 629 "parser.y"
                              { (yyval.sc_expr) = ExpressionList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2339 "parser.c"
    break;

  case 109: /* unary_expression: PLUSPLUS_SY unary_expression  */
#line 638 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(PREINC_OP, (yyvsp[0].sc_expr)); }
#line 2345 "parser.c"
    break;

  case 110: /* unary_expression: MINUSMINUS_SY unary_expression  */
#line 640 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(PREDEC_OP, (yyvsp[0].sc_expr)); }
#line 2351 "parser.c"
    break;

  case 111: /* unary_expression: '+' unary_expression  */
#line 642 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, POS_OP, '+', (yyvsp[0].sc_expr), 0); }
#line 2357 "parser.c"
    break;

  case 112: /* unary_expression: '-' unary_expression  */
#line 644 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, NEG_OP, '-', (yyvsp[0].sc_expr), 0); }
#line 2363 "parser.c"
    break;

  case 113: /* unary_expression: '!' unary_expression  */
#line 646 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, BNOT_OP, '!', (yyvsp[0].sc_expr), 0); }
#line 2369 "parser.c"
    break;

  case 114: /* unary_expression: '~' unary_expression  */
#line 648 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, NOT_OP, '~', (yyvsp[0].sc_expr), 1); }
#line 2375 "parser.c"
    break;

  case 116: /* cast_expression: '(' abstract_declaration ')' cast_expression  */
#line 660 "parser.y"
                              { (yyval.sc_expr) = NewCastOperator(Cg->tokenLoc, (yyvsp[0].sc_expr), GetTypePointer(&(yyvsp[-2].sc_decl)->loc, &(yyvsp[-2].sc_decl)->type)); }
#line 2381 "parser.c"
    break;

  case 118: /* multiplicative_expression: multiplicative_expression '*' cast_expression  */
#line 669 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, MUL_OP, '*', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2387 "parser.c"
    break;

  case 119: /* multiplicative_expression: multiplicative_expression '/' cast_expression  */
#line 671 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, DIV_OP, '/', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2393 "parser.c"
    break;

  case 120: /* multiplicative_expression: multiplicative_expression '%' cast_expression  */
#line 673 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, MOD_OP, '%', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2399 "parser.c"
    break;

  case 122: /* additive_expression: additive_expression '+' multiplicative_expression  */
#line 682 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, ADD_OP, '+', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2405 "parser.c"
    break;

  case 123: /* additive_expression: additive_expression '-' multiplicative_expression  */
#line 684 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SUB_OP, '-', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2411 "parser.c"
    break;

  case 125: /* shift_expression: shift_expression LL_SY additive_expression  */
#line 693 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SHL_OP, LL_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2417 "parser.c"
    break;

  case 126: /* shift_expression: shift_expression GG_SY additive_expression  */
#line 695 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SHR_OP, GG_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2423 "parser.c"
    break;

  case 128: /* relational_expression: relational_expression '<' shift_expression  */
#line 704 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, LT_OP, '<', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2429 "parser.c"
    break;

  case 129: /* relational_expression: relational_expression '>' shift_expression  */
#line 706 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, GT_OP, '>', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2435 "parser.c"
    break;

  case 130: /* relational_expression: relational_expression LE_SY shift_expression  */
#line 708 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, LE_OP, LE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2441 "parser.c"
    break;

  case 131: /* relational_expression: relational_expression GE_SY shift_expression  */
#line 710 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, GE_OP, GE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2447 "parser.c"
    break;

  case 133: /* equality_expression: equality_expression EQ_SY relational_expression  */
#line 719 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, EQ_OP, EQ_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2453 "parser.c"
    break;

  case 134: /* equality_expression: equality_expression NE_SY relational_expression  */
#line 721 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, NE_OP, NE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2459 "parser.c"
    break;

  case 136: /* AND_expression: AND_expression '&' equality_expression  */
#line 730 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, AND_OP, '&', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2465 "parser.c"
    break;

  case 138: /* exclusive_OR_expression: exclusive_OR_expression '^' AND_expression  */
#line 739 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, XOR_OP, '^', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2471 "parser.c"
    break;

  case 140: /* inclusive_OR_expression: inclusive_OR_expression '|' exclusive_OR_expression  */
#line 748 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, OR_OP, '|', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2477 "parser.c"
    break;

  case 142: /* logical_AND_expression: logical_AND_expression AND_SY inclusive_OR_expression  */
#line 757 "parser.y"
                              { (yyval.sc_expr) = NewBinaryBooleanOperator(Cg->tokenLoc, BAND_OP, AND_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2483 "parser.c"
    break;

  case 144: /* logical_OR_expression: logical_OR_expression OR_SY logical_AND_expression  */
#line 766 "parser.y"
                              { (yyval.sc_expr) = NewBinaryBooleanOperator(Cg->tokenLoc, BOR_OP, OR_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2489 "parser.c"
    break;

  case 146: /* conditional_expression: conditional_test '?' expression ':' conditional_expression  */
#line 775 "parser.y"
                              { (yyval.sc_expr) = NewConditionalOperator(Cg->tokenLoc, (yyvsp[-4].sc_expr), (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2495 "parser.c"
    break;

  case 147: /* conditional_test: logical_OR_expression  */
#line 779 "parser.y"
                              {  (yyval.sc_expr) = CheckBooleanExpr(Cg->tokenLoc, (yyvsp[0].sc_expr), 1); }
#line 2501 "parser.c"
    break;

  case 149: /* function_definition: function_definition_header block_item_list '}'  */
#line 798 "parser.y"
                              { DefineFunction(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_stmt)); PopScope(); }
#line 2507 "parser.c"
    break;

  case 150: /* function_definition: function_definition_header '}'  */
#line 800 "parser.y"
                              { DefineFunction(Cg->tokenLoc, CurrentScope, (yyvsp[-1].sc_decl), NULL); PopScope(); }
#line 2513 "parser.c"
    break;

  case 151: /* function_definition_header: declaration_specifiers declarator '{'  */
#line 804 "parser.y"
                              { (yyval.sc_decl) = Function_Definition_Header(Cg->tokenLoc, (yyvsp[-1].sc_decl)); }
#line 2519 "parser.c"
    break;

  case 163: /* discard_statement: DISCARD_SY ';'  */
#line 833 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewDiscardStmt(Cg->tokenLoc, NULL); }
#line 2525 "parser.c"
    break;

  case 164: /* discard_statement: DISCARD_SY expression ';'  */
#line 835 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewDiscardStmt(Cg->tokenLoc, CheckBooleanExpr(Cg->tokenLoc, (yyvsp[-1].sc_expr), 1)); }
#line 2531 "parser.c"
    break;

  case 165: /* jump_statement: BREAK_SY ';'  */
#line 843 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewSimpleStmt(Cg->tokenLoc, BREAK_STMT); }
#line 2537 "parser.c"
    break;

  case 166: /* jump_statement: CONTINUE_SY ';'  */
#line 845 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewSimpleStmt(Cg->tokenLoc, CONTINUE_STMT); }
#line 2543 "parser.c"
    break;

  case 167: /* if_statement: if_header balanced_statement ELSE_SY balanced_statement  */
#line 853 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-3].sc_stmt), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2549 "parser.c"
    break;

  case 168: /* dangling_if: if_header statement  */
#line 857 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-1].sc_stmt), (yyvsp[0].sc_stmt), NULL); }
#line 2555 "parser.c"
    break;

  case 169: /* dangling_if: if_header balanced_statement ELSE_SY dangling_statement  */
#line 859 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-3].sc_stmt), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2561 "parser.c"
    break;

  case 170: /* if_header: IF_SY '(' boolean_scalar_expression ')'  */
#line 863 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewIfStmt(Cg->tokenLoc, (yyvsp[-1].sc_expr), NULL, NULL); ; }
#line 2567 "parser.c"
    break;

  case 171: /* compound_statement: compound_header block_item_list compound_tail  */
#line 871 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewBlockStmt(Cg->tokenLoc, (yyvsp[-1].sc_stmt)); }
#line 2573 "parser.c"
    break;

  case 172: /* compound_statement: compound_header compound_tail  */
#line 873 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2579 "parser.c"
    break;

  case 173: /* compound_header: '{'  */
#line 877 "parser.y"
                              { PushScope(NewScope()); CurrentScope->funindex = NextFunctionIndex; }
#line 2585 "parser.c"
    break;

  case 174: /* compound_tail: '}'  */
#line 881 "parser.y"
                              {
                                if (Cg->options.DumpParseTree)
                                    PrintScopeDeclarations();
                                PopScope();
                              }
#line 2595 "parser.c"
    break;

  case 176: /* block_item_list: block_item_list block_item  */
#line 890 "parser.y"
                              { (yyval.sc_stmt) = AddStmt((yyvsp[-1].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2601 "parser.c"
    break;

  case 178: /* block_item: statement  */
#line 895 "parser.y"
                              { (yyval.sc_stmt) = CheckStmt((yyvsp[0].sc_stmt)); }
#line 2607 "parser.c"
    break;

  case 180: /* expression_statement: ';'  */
#line 904 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2613 "parser.c"
    break;

  case 181: /* expression_statement2: postfix_expression '=' expression  */
#line 908 "parser.y"
                              { (yyval.sc_stmt) = NewSimpleAssignmentStmt(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2619 "parser.c"
    break;

  case 182: /* expression_statement2: expression  */
#line 910 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewExprStmt(Cg->tokenLoc, (yyvsp[0].sc_expr)); }
#line 2625 "parser.c"
    break;

  case 183: /* expression_statement2: postfix_expression ASSIGNMINUS_SY expression  */
#line 912 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMINUS_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2631 "parser.c"
    break;

  case 184: /* expression_statement2: postfix_expression ASSIGNMOD_SY expression  */
#line 914 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMOD_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2637 "parser.c"
    break;

  case 185: /* expression_statement2: postfix_expression ASSIGNPLUS_SY expression  */
#line 916 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNPLUS_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2643 "parser.c"
    break;

  case 186: /* expression_statement2: postfix_expression ASSIGNSLASH_SY expression  */
#line 918 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSLASH_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2649 "parser.c"
    break;

  case 187: /* expression_statement2: postfix_expression ASSIGNSTAR_SY expression  */
#line 920 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSTAR_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2655 "parser.c"
    break;

  case 188: /* iteration_statement: WHILE_SY '(' boolean_scalar_expression ')' balanced_statement  */
#line 928 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, (yyvsp[-2].sc_expr), (yyvsp[0].sc_stmt)); }
#line 2661 "parser.c"
    break;

  case 189: /* iteration_statement: DO_SY statement WHILE_SY '(' boolean_scalar_expression ')' ';'  */
#line 930 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, DO_STMT, (yyvsp[-2].sc_expr), (yyvsp[-5].sc_stmt)); }
#line 2667 "parser.c"
    break;

  case 190: /* iteration_statement: FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' balanced_statement  */
#line 932 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewForStmt(Cg->tokenLoc, (yyvsp[-6].sc_stmt), (yyvsp[-4].sc_expr), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2673 "parser.c"
    break;

  case 191: /* dangling_iteration: WHILE_SY '(' boolean_scalar_expression ')' dangling_statement  */
#line 936 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, (yyvsp[-2].sc_expr), (yyvsp[0].sc_stmt)); }
#line 2679 "parser.c"
    break;

  case 192: /* dangling_iteration: FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' dangling_statement  */
#line 938 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewForStmt(Cg->tokenLoc, (yyvsp[-6].sc_stmt), (yyvsp[-4].sc_expr), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2685 "parser.c"
    break;

  case 193: /* boolean_scalar_expression: expression  */
#line 943 "parser.y"
                              {  (yyval.sc_expr) = CheckBooleanExpr(Cg->tokenLoc, (yyvsp[0].sc_expr), 0); }
#line 2691 "parser.c"
    break;

  case 195: /* for_expression_opt: %empty  */
#line 948 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2697 "parser.c"
    break;

  case 197: /* for_expression: for_expression ',' expression_statement2  */
#line 953 "parser.y"
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
#line 2713 "parser.c"
    break;

  case 199: /* boolean_expression_opt: %empty  */
#line 968 "parser.y"
                              { (yyval.sc_expr) = NULL; }
#line 2719 "parser.c"
    break;

  case 200: /* return_statement: RETURN_SY expression ';'  */
#line 976 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, (yyvsp[-1].sc_expr)); }
#line 2725 "parser.c"
    break;

  case 201: /* return_statement: RETURN_SY ';'  */
#line 978 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, NULL); }
#line 2731 "parser.c"
    break;

  case 208: /* constant: INTCONST_SY  */
#line 1004 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewIConstNode(ICONST_OP, (yyvsp[0].sc_int), TYPE_BASE_CINT); }
#line 2737 "parser.c"
    break;

  case 209: /* constant: CFLOATCONST_SY  */
#line 1006 "parser.y"
                              { int base = Cg->theHAL->GetFloatSuffixBase(Cg->tokenLoc, ' ');
                                (yyval.sc_expr) = (expr *) NewFConstNode(FCONST_OP, (yyvsp[0].sc_fval), base);
                              }
#line 2745 "parser.c"
    break;

  case 210: /* constant: FLOATCONST_SY  */
#line 1010 "parser.y"
                              { int base = Cg->theHAL->GetFloatSuffixBase(Cg->tokenLoc, 'f');
                                (yyval.sc_expr) = (expr *) NewFConstNode(FCONST_OP, (yyvsp[0].sc_fval), base);
                              }
#line 2753 "parser.c"
    break;

  case 211: /* constant: FLOATHCONST_SY  */
#line 1014 "parser.y"
                              { int base = Cg->theHAL->GetFloatSuffixBase(Cg->tokenLoc, 'h');
                                (yyval.sc_expr) = (expr *) NewFConstNode(FCONST_OP, (yyvsp[0].sc_fval), base);
                              }
#line 2761 "parser.c"
    break;

  case 212: /* constant: FLOATXCONST_SY  */
#line 1018 "parser.y"
                              {int base = Cg->theHAL->GetFloatSuffixBase(Cg->tokenLoc, 'x');
                                (yyval.sc_expr) = (expr *) NewFConstNode(FCONST_OP, (yyvsp[0].sc_fval), base);
                              }
#line 2769 "parser.c"
    break;


#line 2773 "parser.c"

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

#line 1034 "parser.y"


