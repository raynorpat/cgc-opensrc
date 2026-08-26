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
  YYSYMBOL_POINT_SY = 70,                  /* POINT_SY  */
  YYSYMBOL_LINE_SY = 71,                   /* LINE_SY  */
  YYSYMBOL_LINE_ADJ_SY = 72,               /* LINE_ADJ_SY  */
  YYSYMBOL_TRIANGLE_SY = 73,               /* TRIANGLE_SY  */
  YYSYMBOL_TRIANGLE_ADJ_SY = 74,           /* TRIANGLE_ADJ_SY  */
  YYSYMBOL_POINT_OUT_SY = 75,              /* POINT_OUT_SY  */
  YYSYMBOL_LINE_OUT_SY = 76,               /* LINE_OUT_SY  */
  YYSYMBOL_TRIANGLE_OUT_SY = 77,           /* TRIANGLE_OUT_SY  */
  YYSYMBOL_FIRST_USER_TOKEN_SY = 78,       /* FIRST_USER_TOKEN_SY  */
  YYSYMBOL_79_ = 79,                       /* ';'  */
  YYSYMBOL_80_ = 80,                       /* ','  */
  YYSYMBOL_81_ = 81,                       /* '='  */
  YYSYMBOL_82_ = 82,                       /* '}'  */
  YYSYMBOL_83_ = 83,                       /* ':'  */
  YYSYMBOL_84_ = 84,                       /* '<'  */
  YYSYMBOL_85_ = 85,                       /* '>'  */
  YYSYMBOL_86_ = 86,                       /* '['  */
  YYSYMBOL_87_ = 87,                       /* ']'  */
  YYSYMBOL_88_ = 88,                       /* ')'  */
  YYSYMBOL_89_ = 89,                       /* '('  */
  YYSYMBOL_90_ = 90,                       /* '{'  */
  YYSYMBOL_91_ = 91,                       /* '.'  */
  YYSYMBOL_92_ = 92,                       /* '+'  */
  YYSYMBOL_93_ = 93,                       /* '-'  */
  YYSYMBOL_94_ = 94,                       /* '!'  */
  YYSYMBOL_95_ = 95,                       /* '~'  */
  YYSYMBOL_96_ = 96,                       /* '*'  */
  YYSYMBOL_97_ = 97,                       /* '/'  */
  YYSYMBOL_98_ = 98,                       /* '%'  */
  YYSYMBOL_99_ = 99,                       /* '&'  */
  YYSYMBOL_100_ = 100,                     /* '^'  */
  YYSYMBOL_101_ = 101,                     /* '|'  */
  YYSYMBOL_102_ = 102,                     /* '?'  */
  YYSYMBOL_YYACCEPT = 103,                 /* $accept  */
  YYSYMBOL_compilation_unit = 104,         /* compilation_unit  */
  YYSYMBOL_external_declaration = 105,     /* external_declaration  */
  YYSYMBOL_profile_specifier = 106,        /* profile_specifier  */
  YYSYMBOL_declaration = 107,              /* declaration  */
  YYSYMBOL_abstract_declaration = 108,     /* abstract_declaration  */
  YYSYMBOL_declaration_specifiers = 109,   /* declaration_specifiers  */
  YYSYMBOL_abstract_declaration_specifiers = 110, /* abstract_declaration_specifiers  */
  YYSYMBOL_abstract_declaration_specifiers2 = 111, /* abstract_declaration_specifiers2  */
  YYSYMBOL_init_declarator_list = 112,     /* init_declarator_list  */
  YYSYMBOL_init_declarator = 113,          /* init_declarator  */
  YYSYMBOL_type_specifier = 114,           /* type_specifier  */
  YYSYMBOL_type_qualifier = 115,           /* type_qualifier  */
  YYSYMBOL_type_domain = 116,              /* type_domain  */
  YYSYMBOL_storage_class = 117,            /* storage_class  */
  YYSYMBOL_function_specifier = 118,       /* function_specifier  */
  YYSYMBOL_geometry_modifier = 119,        /* geometry_modifier  */
  YYSYMBOL_in_out = 120,                   /* in_out  */
  YYSYMBOL_struct_or_connector_specifier = 121, /* struct_or_connector_specifier  */
  YYSYMBOL_struct_compound_header = 122,   /* struct_compound_header  */
  YYSYMBOL_struct_or_connector_header = 123, /* struct_or_connector_header  */
  YYSYMBOL_struct_identifier = 124,        /* struct_identifier  */
  YYSYMBOL_untagged_struct_header = 125,   /* untagged_struct_header  */
  YYSYMBOL_struct_declaration_list = 126,  /* struct_declaration_list  */
  YYSYMBOL_struct_declaration = 127,       /* struct_declaration  */
  YYSYMBOL_interface_specifier = 128,      /* interface_specifier  */
  YYSYMBOL_interface_compound_header = 129, /* interface_compound_header  */
  YYSYMBOL_interface_member_declaration_list = 130, /* interface_member_declaration_list  */
  YYSYMBOL_interface_member_declaration = 131, /* interface_member_declaration  */
  YYSYMBOL_annotation = 132,               /* annotation  */
  YYSYMBOL_133_1 = 133,                    /* $@1  */
  YYSYMBOL_annotation_decl_list = 134,     /* annotation_decl_list  */
  YYSYMBOL_declarator = 135,               /* declarator  */
  YYSYMBOL_semantic_declarator = 136,      /* semantic_declarator  */
  YYSYMBOL_basic_declarator = 137,         /* basic_declarator  */
  YYSYMBOL_function_decl_header = 138,     /* function_decl_header  */
  YYSYMBOL_abstract_declarator = 139,      /* abstract_declarator  */
  YYSYMBOL_parameter_list = 140,           /* parameter_list  */
  YYSYMBOL_parameter_declaration = 141,    /* parameter_declaration  */
  YYSYMBOL_abstract_parameter_list = 142,  /* abstract_parameter_list  */
  YYSYMBOL_non_empty_abstract_parameter_list = 143, /* non_empty_abstract_parameter_list  */
  YYSYMBOL_initializer = 144,              /* initializer  */
  YYSYMBOL_initializer_list = 145,         /* initializer_list  */
  YYSYMBOL_variable = 146,                 /* variable  */
  YYSYMBOL_basic_variable = 147,           /* basic_variable  */
  YYSYMBOL_primary_expression = 148,       /* primary_expression  */
  YYSYMBOL_postfix_expression = 149,       /* postfix_expression  */
  YYSYMBOL_actual_argument_list = 150,     /* actual_argument_list  */
  YYSYMBOL_non_empty_argument_list = 151,  /* non_empty_argument_list  */
  YYSYMBOL_expression_list = 152,          /* expression_list  */
  YYSYMBOL_unary_expression = 153,         /* unary_expression  */
  YYSYMBOL_cast_expression = 154,          /* cast_expression  */
  YYSYMBOL_multiplicative_expression = 155, /* multiplicative_expression  */
  YYSYMBOL_additive_expression = 156,      /* additive_expression  */
  YYSYMBOL_shift_expression = 157,         /* shift_expression  */
  YYSYMBOL_relational_expression = 158,    /* relational_expression  */
  YYSYMBOL_equality_expression = 159,      /* equality_expression  */
  YYSYMBOL_AND_expression = 160,           /* AND_expression  */
  YYSYMBOL_exclusive_OR_expression = 161,  /* exclusive_OR_expression  */
  YYSYMBOL_inclusive_OR_expression = 162,  /* inclusive_OR_expression  */
  YYSYMBOL_logical_AND_expression = 163,   /* logical_AND_expression  */
  YYSYMBOL_logical_OR_expression = 164,    /* logical_OR_expression  */
  YYSYMBOL_conditional_expression = 165,   /* conditional_expression  */
  YYSYMBOL_conditional_test = 166,         /* conditional_test  */
  YYSYMBOL_expression = 167,               /* expression  */
  YYSYMBOL_function_definition = 168,      /* function_definition  */
  YYSYMBOL_function_definition_header = 169, /* function_definition_header  */
  YYSYMBOL_statement = 170,                /* statement  */
  YYSYMBOL_balanced_statement = 171,       /* balanced_statement  */
  YYSYMBOL_dangling_statement = 172,       /* dangling_statement  */
  YYSYMBOL_discard_statement = 173,        /* discard_statement  */
  YYSYMBOL_jump_statement = 174,           /* jump_statement  */
  YYSYMBOL_if_statement = 175,             /* if_statement  */
  YYSYMBOL_dangling_if = 176,              /* dangling_if  */
  YYSYMBOL_if_header = 177,                /* if_header  */
  YYSYMBOL_compound_statement = 178,       /* compound_statement  */
  YYSYMBOL_compound_header = 179,          /* compound_header  */
  YYSYMBOL_compound_tail = 180,            /* compound_tail  */
  YYSYMBOL_block_item_list = 181,          /* block_item_list  */
  YYSYMBOL_block_item = 182,               /* block_item  */
  YYSYMBOL_expression_statement = 183,     /* expression_statement  */
  YYSYMBOL_expression_statement2 = 184,    /* expression_statement2  */
  YYSYMBOL_iteration_statement = 185,      /* iteration_statement  */
  YYSYMBOL_dangling_iteration = 186,       /* dangling_iteration  */
  YYSYMBOL_boolean_scalar_expression = 187, /* boolean_scalar_expression  */
  YYSYMBOL_for_expression_opt = 188,       /* for_expression_opt  */
  YYSYMBOL_for_expression = 189,           /* for_expression  */
  YYSYMBOL_boolean_expression_opt = 190,   /* boolean_expression_opt  */
  YYSYMBOL_return_statement = 191,         /* return_statement  */
  YYSYMBOL_member_identifier = 192,        /* member_identifier  */
  YYSYMBOL_scope_identifier = 193,         /* scope_identifier  */
  YYSYMBOL_semantics_identifier = 194,     /* semantics_identifier  */
  YYSYMBOL_type_identifier = 195,          /* type_identifier  */
  YYSYMBOL_variable_identifier = 196,      /* variable_identifier  */
  YYSYMBOL_identifier = 197,               /* identifier  */
  YYSYMBOL_constant = 198                  /* constant  */
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
#define YYFINAL  73
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2250

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  103
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  96
/* YYNRULES -- Number of rules.  */
#define YYNRULES  246
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  377

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   334


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
       2,     2,     2,    94,     2,     2,     2,    98,    99,     2,
      89,    88,    96,    92,    80,    93,    91,    97,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    83,    79,
      84,    81,    85,   102,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    86,     2,    87,   100,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    90,   101,    82,    95,     2,     2,     2,
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
      66,    67,    68,    69,     2,    70,    71,    72,    73,    74,
      75,    76,    77,    78,     2
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   277,   277,   278,   285,   287,   289,   291,   301,   305,
     307,   309,   315,   322,   324,   329,   331,   333,   335,   337,
     339,   341,   346,   351,   353,   355,   357,   359,   361,   363,
     367,   369,   373,   375,   383,   385,   387,   389,   391,   393,
     395,   397,   399,   401,   403,   405,   407,   409,   411,   413,
     415,   417,   419,   421,   434,   442,   444,   452,   454,   462,
     464,   478,   479,   480,   481,   482,   483,   484,   485,   492,
     494,   496,   505,   508,   511,   515,   520,   522,   524,   528,
     529,   532,   536,   537,   540,   542,   551,   560,   569,   570,
     574,   596,   596,   601,   602,   609,   611,   615,   617,   621,
     623,   625,   627,   629,   633,   638,   639,   641,   658,   660,
     664,   666,   671,   672,   675,   681,   694,   696,   698,   700,
     708,   710,   722,   724,   728,   736,   737,   738,   740,   748,
     749,   751,   753,   755,   757,   762,   763,   766,   768,   772,
     774,   782,   783,   785,   787,   789,   791,   793,   801,   805,
     813,   814,   816,   818,   826,   827,   829,   837,   838,   840,
     848,   849,   851,   853,   855,   863,   864,   866,   874,   875,
     883,   884,   892,   893,   901,   902,   910,   911,   919,   920,
     924,   932,   943,   946,   951,   959,   960,   963,   964,   965,
     966,   967,   968,   969,   972,   973,   980,   982,   990,   992,
    1000,  1004,  1006,  1010,  1018,  1020,  1024,  1028,  1036,  1037,
    1041,  1042,  1050,  1051,  1055,  1057,  1059,  1061,  1063,  1065,
    1067,  1075,  1077,  1079,  1083,  1085,  1090,  1094,  1096,  1099,
    1100,  1114,  1116,  1123,  1125,  1133,  1136,  1139,  1142,  1145,
    1148,  1150,  1160,  1162,  1164,  1166,  1168
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
  "RESERVED_SY", "POINT_SY", "LINE_SY", "LINE_ADJ_SY", "TRIANGLE_SY",
  "TRIANGLE_ADJ_SY", "POINT_OUT_SY", "LINE_OUT_SY", "TRIANGLE_OUT_SY",
  "FIRST_USER_TOKEN_SY", "';'", "','", "'='", "'}'", "':'", "'<'", "'>'",
  "'['", "']'", "')'", "'('", "'{'", "'.'", "'+'", "'-'", "'!'", "'~'",
  "'*'", "'/'", "'%'", "'&'", "'^'", "'|'", "'?'", "$accept",
  "compilation_unit", "external_declaration", "profile_specifier",
  "declaration", "abstract_declaration", "declaration_specifiers",
  "abstract_declaration_specifiers", "abstract_declaration_specifiers2",
  "init_declarator_list", "init_declarator", "type_specifier",
  "type_qualifier", "type_domain", "storage_class", "function_specifier",
  "geometry_modifier", "in_out", "struct_or_connector_specifier",
  "struct_compound_header", "struct_or_connector_header",
  "struct_identifier", "untagged_struct_header", "struct_declaration_list",
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

#define YYPACT_NINF (-308)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-237)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    1960,  -308,  -308,  -308,   -55,  -308,  -308,  -308,  -308,  -308,
    -308,  -308,  -308,  -308,  2173,  -308,    56,  -308,  2173,  -308,
    -308,  -308,  -308,  -308,  -308,  -308,  -308,    56,  -308,  -308,
      55,  -308,  -308,  -308,  -308,  -308,  -308,  -308,  -308,  -308,
    1883,  -308,  2037,  -308,    -1,  -308,   197,  -308,  2173,  2173,
    2173,  2173,  2173,  2173,  -308,   -44,   -44,  -308,  -308,   322,
    -308,  -308,  -308,  -308,     7,  -308,  -308,  -308,   -44,  -308,
    -308,  -308,  -308,  -308,  -308,  -308,  -308,  -308,   -22,  -308,
      27,    36,    63,  1533,  -308,  -308,  -308,  -308,  -308,  -308,
    -308,  -308,  -308,  -308,  -308,  -308,  -308,  -308,  2037,  -308,
    2037,    34,  -308,    49,   884,   666,  -308,  -308,  -308,    68,
      70,  -308,  1388,  1388,   956,    79,  -308,  -308,   580,  1388,
    1388,  1388,  1388,  -308,    -1,    80,  -308,  -308,  -308,   221,
    -308,  -308,    13,    31,    -7,   -13,    -5,    71,    72,    73,
     170,   -32,  -308,    86,  -308,  -308,  -308,  -308,  -308,  -308,
    -308,  -308,   666,  -308,   408,   494,  -308,  -308,   103,  -308,
    -308,  -308,   178,  -308,   179,  -308,    56,  2105,  -308,  -308,
       5,  1028,  -308,  -308,  -308,     5,    -6,  -308,  -308,     5,
      22,   -41,  -308,   106,   116,  -308,  1669,  -308,  -308,  1737,
    -308,  -308,  -308,    80,    40,   118,   141,  1100,  1460,  1460,
    -308,  -308,  -308,   123,  1460,   115,  -308,   119,  -308,  -308,
    -308,  -308,   125,  1460,  1460,  1460,  1460,  1460,  1460,  -308,
    -308,  1460,  1460,  1172,     5,  1460,  1460,  1460,  1460,  1460,
    1460,  1460,  1460,  1460,  1460,  1460,  1460,  1460,  1460,  1460,
    1460,  1460,  1460,  1460,  -308,   189,  -308,  -308,   408,  -308,
    -308,  -308,     5,  -308,  -308,  -308,     5,  1805,  -308,  -308,
     738,  -308,  -308,  -308,  -308,   124,  -308,   132,   129,  2105,
    -308,  -308,  2173,  -308,  -308,  -308,  -308,   127,  -308,   138,
     139,  -308,   130,  -308,   135,  1460,  -308,     9,  -308,  -308,
    -308,  -308,  -308,  -308,  -308,   137,   146,   155,  -308,  -308,
    -308,  -308,  -308,  -308,    13,    13,    31,    31,    -7,    -7,
      -7,    -7,   -13,   -13,    -5,    71,    72,    73,   170,   154,
     666,  -308,  -308,  -308,   159,  -308,  -308,  -308,  -308,    52,
    1601,  -308,  1028,    -2,  -308,  -308,  1460,  1244,  1460,  -308,
     666,  -308,  1460,  -308,  -308,  -308,  1460,  1460,  -308,  -308,
    -308,   811,  -308,  -308,  -308,  -308,   152,  -308,   153,  -308,
     161,  -308,  -308,  -308,  -308,  -308,  -308,  -308,  -308,  -308,
     165,  1316,  -308,   157,   666,  -308,  -308
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    53,    37,    54,     0,    58,    35,   240,    69,    59,
      71,    34,    60,    70,     0,    57,    81,    38,     0,   238,
      55,    56,    36,    39,    44,    43,    42,     0,    41,    40,
      45,   241,    61,    62,    63,    64,    65,    66,    67,    68,
       0,     2,     0,     4,     0,    13,    15,    23,     0,     0,
       0,     0,     0,     0,    50,    74,     0,    51,     5,     0,
      52,     8,    11,    22,    76,    80,    79,    14,     0,    48,
      46,    49,    47,     1,     3,     7,     6,     9,     0,    30,
      32,    95,    97,     0,    99,    29,    24,    26,    25,    28,
      27,    16,    18,    17,    20,    21,    19,   206,     0,    75,
       0,     0,   243,     0,     0,     0,   244,   245,   246,     0,
       0,   242,     0,     0,     0,     0,   213,   183,     0,     0,
       0,     0,     0,   210,     0,    23,   125,   122,   129,   141,
     148,   150,   154,   157,   160,   165,   168,   170,   172,   174,
     176,   178,   181,     0,   215,   211,   185,   186,   188,   192,
     191,   194,     0,   187,     0,     0,   208,   189,     0,   190,
     195,   193,     0,   124,   239,   126,     0,     0,    87,    10,
       0,     0,   184,    91,    96,     0,     0,   104,   114,     0,
     105,     0,   108,     0,   113,    84,     0,    82,    85,     0,
     198,   199,   196,     0,   141,     0,     0,     0,     0,     0,
     143,   142,   234,     0,     0,     0,   105,     0,   144,   145,
     146,   147,    32,     0,     0,     0,     0,     0,     0,   131,
     130,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   201,   185,   207,   205,     0,   182,
     209,   212,     0,    77,    78,   237,     0,     0,    88,    31,
       0,    33,   116,    93,    98,     0,   101,   110,    12,     0,
     102,   103,     0,    72,    83,    73,   197,     0,   229,     0,
     227,   226,     0,   233,     0,     0,   127,     0,   139,   216,
     217,   218,   219,   220,   214,     0,     0,   136,   137,   132,
     235,   151,   152,   153,   155,   156,   159,   158,   164,   163,
     161,   162,   166,   167,   169,   171,   173,   175,   177,     0,
       0,   204,   123,   239,     0,    86,    89,   119,   120,     0,
       0,   100,     0,     0,   109,   115,     0,     0,     0,   203,
       0,   149,     0,   128,   133,   134,     0,     0,   200,   202,
      90,     0,   117,    92,    94,   111,     0,   107,     0,   231,
       0,   230,   221,   224,   140,   138,   179,   118,   121,   106,
       0,     0,   222,     0,     0,   223,   225
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -308,  -308,   207,  -308,     1,  -114,   -48,    12,  -308,  -308,
      81,     0,   203,   204,   206,   210,  -308,   212,  -308,   205,
    -308,   232,  -308,   160,  -164,  -308,  -308,  -308,     8,  -308,
    -308,  -308,   -42,  -308,  -308,  -308,  -308,  -308,    -3,  -308,
    -308,  -255,  -308,  -308,    10,  -308,   -50,  -308,  -308,  -308,
      23,  -206,  -135,   -92,   -70,   -86,    26,    28,    33,    30,
      32,  -308,   -72,  -308,   -38,    44,  -308,   -77,  -149,  -307,
    -308,  -308,  -308,  -308,  -308,  -308,    59,    35,   122,  -145,
    -308,  -190,  -308,  -308,  -196,   -94,  -308,  -308,  -308,  -308,
    -308,   104,   -10,  -308,    29,  -308
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    40,    41,    42,   123,   178,    44,    45,    46,    78,
      79,   193,    48,    49,    50,    51,    52,    53,    54,    98,
      55,    64,    56,   186,   187,    57,   167,   257,   258,   174,
     263,   330,   212,    81,    82,    83,   268,   181,   182,   183,
     184,   261,   329,   126,   127,   128,   194,   296,   297,   287,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   188,    59,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   247,   155,   156,
     157,   158,   159,   160,   282,   279,   280,   360,   161,   299,
     162,   253,    60,   163,   164,   165
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      47,    43,    80,   245,   205,   328,    65,   278,   284,   129,
     250,   124,   242,   349,    47,   236,   232,    65,    47,   301,
     302,   303,   274,   230,    62,   274,    63,   233,   196,    61,
      67,     7,   265,   363,   231,   179,   356,     7,   237,   269,
      47,    43,    47,    75,    58,    66,    97,   270,    47,    47,
      47,    47,    47,    47,   -13,   129,    66,   169,   170,   125,
      91,    92,    93,    94,    95,    96,   195,   376,    31,    61,
    -180,   234,   235,    84,    31,   244,   203,   355,    77,   341,
     207,   266,   219,    47,    58,   357,    76,   220,     7,   342,
     166,   -13,    69,   304,   305,   180,   368,   343,    47,   185,
      47,   185,   129,   250,   129,   129,   124,   124,   171,   225,
     226,   227,    19,   190,    99,    99,    70,   172,   125,   256,
     173,    71,    72,   228,   229,    31,   222,   168,   191,   223,
     206,   224,   351,   262,   352,   200,   201,   267,   306,   307,
     358,   359,   208,   209,   210,   211,   175,   129,   361,   176,
     312,   313,   177,    84,   125,   125,   254,   197,   335,   198,
     281,   207,   308,   309,   310,   311,   281,    47,   204,   213,
     238,   348,   239,   241,   240,   288,   289,   290,   291,   292,
     293,   278,   251,   294,   295,   298,    47,   185,   243,    47,
     185,   362,   252,  -236,   271,   255,   272,   276,   129,    84,
     124,   277,   283,   285,   255,   319,   171,   286,    84,   256,
     320,   331,     3,   332,   324,   333,   336,   337,   339,   338,
       5,   179,   262,   340,   344,   375,   214,   215,   216,   217,
     218,     8,     9,    10,   345,   346,    12,   347,   350,   369,
     371,   370,    13,    85,   372,   374,    15,    74,   125,    86,
      87,   259,    88,   300,    20,    21,    89,    47,    90,    68,
     189,   100,   322,   219,   314,   326,   334,   315,   220,    47,
     129,   317,    47,   316,   318,   366,   248,   373,     0,   264,
       0,   323,   124,   321,   206,    84,     0,     0,   129,     0,
     129,     0,     0,     0,   262,     0,     0,     0,   281,   281,
       0,     0,   221,     0,   364,     0,     0,   222,   365,     0,
     223,     0,   224,   262,     0,     0,     0,     0,     0,     0,
       0,   129,     0,     1,   129,     0,     0,     0,     0,     0,
      47,   354,     2,   101,     0,   102,     0,     3,   103,     0,
     104,   105,     0,     0,     4,     5,     6,   106,   107,   108,
     109,     0,     0,     0,     7,   110,     8,     9,    10,    11,
     111,    12,     0,     0,   112,     0,     0,    13,    14,   113,
     114,    15,     0,    16,     0,    17,     0,    18,    19,    20,
      21,    22,   115,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
       0,   116,     0,     0,   117,     0,     0,     0,     0,     1,
       0,   118,    97,     0,   119,   120,   121,   122,     2,   101,
       0,   102,     0,     3,   103,     0,   104,   105,     0,     0,
       4,     5,     6,   106,   107,   108,   109,     0,     0,     0,
       7,   110,     8,     9,    10,    11,   111,    12,     0,     0,
     112,     0,     0,    13,    14,   113,   114,    15,     0,    16,
       0,    17,     0,    18,    19,    20,    21,    22,   115,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,     0,   116,     0,     0,
     246,     0,     0,     0,     0,     1,     0,   118,    97,     0,
     119,   120,   121,   122,     2,   101,     0,   102,     0,     3,
     103,     0,   104,   105,     0,     0,     4,     5,     6,   106,
     107,   108,   109,     0,     0,     0,     7,   110,     8,     9,
      10,    11,   111,    12,     0,     0,   112,     0,     0,    13,
      14,   113,   114,    15,     0,    16,     0,    17,     0,    18,
      19,    20,    21,    22,   115,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,     0,   116,     0,     0,   249,     0,     0,     0,
       0,     1,     0,   118,    97,     0,   119,   120,   121,   122,
       2,     0,     0,   102,     0,     3,     0,     0,     0,     0,
       0,     0,     0,     5,     6,   106,   107,   108,     0,     0,
       0,     0,     7,     0,     8,     9,    10,    11,   111,    12,
       0,     0,   112,     0,     0,    13,    14,   113,     0,    15,
       0,    16,     0,    17,     0,     0,    19,    20,    21,    22,
       0,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     1,     0,   118,
       0,     0,   119,   120,   121,   122,     2,   101,     0,   102,
       0,     0,   103,     0,   104,   105,     0,     0,     0,     0,
       6,   106,   107,   108,   109,     0,     0,     0,     7,   110,
       0,     0,     0,    11,   111,     0,     0,     0,   112,     0,
       0,     0,     0,   113,   114,     0,     0,    16,     0,    17,
       0,     0,    19,     0,     0,    22,   115,    23,    24,    25,
      26,    27,    28,    29,    30,    31,     0,     0,     0,     1,
       0,     0,     0,     0,     0,   116,     0,     0,     2,     0,
       0,   102,     0,     0,     0,   118,    97,     0,   119,   120,
     121,   122,     6,   106,   107,   108,     0,     0,     0,     0,
       7,     0,     0,     0,     0,    11,   111,     0,     0,     0,
     112,     0,     0,     0,     0,   113,     0,     0,     0,    16,
       0,    17,     0,     0,    19,     0,     0,    22,     0,    23,
      24,    25,    26,    27,    28,    29,    30,    31,     0,     0,
       0,     0,     1,     0,     0,     0,     0,     0,     0,     0,
     327,     2,     0,     0,   102,     0,     0,   118,   260,     0,
     119,   120,   121,   122,     0,     6,   106,   107,   108,     0,
       0,     0,     0,     7,     0,     0,     0,     0,    11,   111,
       0,     0,     0,   112,     0,     0,     0,     0,   113,     0,
       0,     0,    16,     0,    17,     0,     0,    19,     0,     0,
      22,     0,    23,    24,    25,    26,    27,    28,    29,    30,
      31,     0,     0,     0,     0,     1,     0,     0,     0,     0,
       0,     0,     0,   367,     2,     0,     0,   102,     0,     0,
     118,   260,     0,   119,   120,   121,   122,     0,     6,   106,
     107,   108,     0,     0,     0,     0,     7,     0,     0,     0,
       0,    11,   111,     0,     0,     0,   112,     0,     0,     0,
       0,   113,     0,     0,     0,    16,     0,    17,     0,     0,
      19,     0,     0,    22,     0,    23,    24,    25,    26,    27,
      28,    29,    30,    31,     0,     0,     0,     1,     0,     0,
       0,     0,     0,   192,     0,     0,     2,     0,     0,   102,
       0,     0,     0,   118,     0,     0,   119,   120,   121,   122,
       6,   106,   107,   108,     0,     0,     0,     0,     7,     0,
       0,     0,     0,    11,   111,     0,     0,     0,   112,     0,
       0,     0,     0,   113,     0,     0,     0,    16,     0,    17,
       0,     0,    19,     0,     0,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,     0,     0,     0,     1,
       0,     0,     0,     0,     0,   202,     0,     0,     2,     0,
       0,   102,     0,     0,     0,   118,     0,     0,   119,   120,
     121,   122,     6,   106,   107,   108,     0,     0,     0,     0,
       7,     0,     0,     0,     0,    11,   111,     0,     0,     0,
     112,     0,     0,     0,     0,   113,     0,     0,     0,    16,
       0,    17,     0,     0,    19,     0,     0,    22,     0,    23,
      24,    25,    26,    27,    28,    29,    30,    31,     0,     0,
       0,     1,     0,     0,     0,     0,     0,     0,     0,     0,
       2,     0,     0,   102,     0,     0,     0,   118,   260,     0,
     119,   120,   121,   122,     6,   106,   107,   108,     0,     0,
       0,     0,     7,     0,     0,     0,     0,    11,   111,     0,
       0,     0,   112,     0,     0,     0,     0,   113,     0,     0,
       0,    16,     0,    17,     0,     0,    19,     0,     0,    22,
       0,    23,    24,    25,    26,    27,    28,    29,    30,    31,
       0,     0,     0,     1,     0,     0,     0,     0,     0,  -228,
       0,     0,     2,     0,     0,   102,     0,     0,     0,   118,
       0,     0,   119,   120,   121,   122,     6,   106,   107,   108,
       0,     0,     0,     0,     7,     0,     0,     0,     0,    11,
     111,     0,     0,     0,   112,     0,     0,     0,     0,   113,
       0,     0,     0,    16,     0,    17,     0,     0,    19,     0,
       0,    22,     0,    23,    24,    25,    26,    27,    28,    29,
      30,    31,     0,     0,     0,     1,     0,     0,     0,     0,
       0,     0,     0,     0,     2,     0,     0,   102,     0,     0,
    -135,   118,     0,     0,   119,   120,   121,   122,     6,   106,
     107,   108,     0,     0,     0,     0,     7,     0,     0,     0,
       0,    11,   111,     0,     0,     0,   112,     0,     0,     0,
       0,   113,     0,     0,     0,    16,     0,    17,     0,     0,
      19,     0,     0,    22,     0,    23,    24,    25,    26,    27,
      28,    29,    30,    31,     0,     0,     0,     1,     0,     0,
       0,     0,     0,  -232,     0,     0,     2,     0,     0,   102,
       0,     0,     0,   118,     0,     0,   119,   120,   121,   122,
       6,   106,   107,   108,     0,     0,     0,     0,     7,     0,
       0,     0,     0,    11,   111,     0,     0,     0,   112,     0,
       0,     0,     0,   113,     0,     0,     0,    16,     0,    17,
       0,     0,    19,     0,     0,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,     0,     0,     0,     1,
       0,     0,     0,     0,     0,     0,     0,     0,     2,     0,
       0,   102,     0,     0,  -228,   118,     0,     0,   119,   120,
     121,   122,     6,   106,   107,   108,     0,     0,     0,     0,
       7,     0,     0,     0,     0,    11,   111,     0,     0,     0,
     112,     0,     0,     0,     0,   113,     0,     0,     0,    16,
       0,    17,     0,     0,    19,     0,     0,    22,     0,    23,
      24,    25,    26,    27,    28,    29,    30,    31,     0,     0,
       0,     1,     0,     0,     0,     0,     0,     0,     0,     0,
       2,     0,     0,   102,     0,     0,     0,   199,     0,     0,
     119,   120,   121,   122,     6,   106,   107,   108,     0,     0,
       0,     0,     7,     0,     0,     0,     0,    11,   111,     0,
       0,     0,   112,     0,     0,     0,     0,   113,     0,     0,
       0,    16,     0,    17,     0,     0,    19,     0,     0,    22,
       0,    23,    24,    25,    26,    27,    28,    29,    30,    31,
       0,     0,     0,     0,     1,     0,     0,     0,     0,     0,
       0,     0,     0,     2,     0,     0,     0,     0,     3,   118,
       0,     0,   119,   120,   121,   122,     5,     6,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,     0,    12,     0,     0,     0,     0,     0,    13,    14,
       0,     0,    15,     0,    16,     0,    17,     0,    18,    19,
      20,    21,    22,     0,    23,    24,    25,    26,    27,    28,
      29,    30,     1,    32,    33,    34,    35,    36,    37,    38,
      39,     2,     0,     0,     0,     0,     3,     0,     0,     0,
       0,  -112,     0,     4,     5,     6,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,     0,
      12,     0,     0,     0,     0,     0,    13,    14,     0,     0,
      15,     0,    16,     0,    17,     0,    18,    19,    20,    21,
      22,     0,    23,    24,    25,    26,    27,    28,    29,    30,
       1,    32,    33,    34,    35,    36,    37,    38,    39,     2,
       0,     0,     0,     0,     3,     0,   353,     0,     0,     0,
       0,     4,     5,     6,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     8,     9,    10,    11,     0,    12,     0,
       0,     0,     0,     0,    13,    14,     0,     0,    15,     0,
      16,     0,    17,     0,    18,    19,    20,    21,    22,     0,
      23,    24,    25,    26,    27,    28,    29,    30,     1,    32,
      33,    34,    35,    36,    37,    38,    39,     2,     0,     0,
       0,   273,     3,     0,     0,     0,     0,     0,     0,     4,
       5,     6,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,     0,    12,     0,     0,     0,
       0,     0,    13,    14,     0,     0,    15,     0,    16,     0,
      17,     0,    18,    19,    20,    21,    22,     0,    23,    24,
      25,    26,    27,    28,    29,    30,     1,    32,    33,    34,
      35,    36,    37,    38,    39,     2,     0,     0,     0,   275,
       3,     0,     0,     0,     0,     0,     0,     0,     5,     6,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,     0,    12,     0,     0,     0,     0,     0,
      13,    14,     0,     0,    15,     0,    16,     0,    17,     0,
      18,    19,    20,    21,    22,     0,    23,    24,    25,    26,
      27,    28,    29,    30,     0,    32,    33,    34,    35,    36,
      37,    38,    39,    73,     1,     0,     0,   325,     0,     0,
       0,     0,     0,     2,     0,     0,     0,     0,     3,     0,
       0,     0,     0,     0,     0,     4,     5,     6,     0,     0,
       0,     0,     0,     0,     0,     7,     0,     8,     9,    10,
      11,     0,    12,     0,     0,     0,     0,     0,    13,    14,
       0,     0,    15,     0,    16,     0,    17,     0,    18,    19,
      20,    21,    22,     0,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,     1,     0,     0,     0,     0,     0,     0,     0,     0,
       2,     0,     0,     0,     0,     3,     0,     0,     0,     0,
       0,     0,     4,     5,     6,     0,     0,     0,     0,     0,
       0,     0,     7,     0,     8,     9,    10,    11,     0,    12,
       0,     0,     0,     0,     0,    13,    14,     0,     0,    15,
       0,    16,     0,    17,     0,    18,    19,    20,    21,    22,
       0,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,     1,     0,
       0,     0,     0,     0,     0,     0,     0,     2,     0,     0,
       0,     0,     3,     0,     0,     0,     0,     0,     0,     4,
       5,     6,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,     0,    12,     0,     0,     0,
       0,     0,    13,    14,     0,     0,    15,     0,    16,     0,
      17,     0,    18,    19,    20,    21,    22,     0,    23,    24,
      25,    26,    27,    28,    29,    30,     1,    32,    33,    34,
      35,    36,    37,    38,    39,     2,     0,     0,     0,     0,
       3,     0,     0,     0,     0,     0,     0,     0,     5,     6,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,     0,    12,     0,     0,     0,     0,     0,
      13,    14,     0,     0,    15,     0,    16,     0,    17,     0,
      18,    19,    20,    21,    22,     0,    23,    24,    25,    26,
      27,    28,    29,    30,     1,    32,    33,    34,    35,    36,
      37,    38,    39,     2,     0,     0,     0,     0,     3,     0,
       0,     0,     0,     0,     0,     0,     5,     6,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,     0,    12,     0,     0,     0,     0,     0,    13,    14,
       0,     0,    15,     0,    16,     0,    17,     0,     0,    19,
      20,    21,    22,     0,    23,    24,    25,    26,    27,    28,
      29,    30,     0,    32,    33,    34,    35,    36,    37,    38,
      39
};

static const yytype_int16 yycheck[] =
{
       0,     0,    44,   152,   118,   260,    16,   197,   204,    59,
     155,    59,    44,   320,    14,    20,    29,    27,    18,   225,
     226,   227,   186,    30,    79,   189,    14,    40,   105,     0,
      18,    32,    38,   340,    41,    83,    38,    32,    43,    80,
      40,    40,    42,    42,     0,    16,    90,    88,    48,    49,
      50,    51,    52,    53,    32,   105,    27,    79,    80,    59,
      48,    49,    50,    51,    52,    53,   104,   374,    69,    40,
     102,    84,    85,    44,    69,   152,   114,   332,    79,   285,
     118,    87,    42,    83,    40,    87,    42,    47,    32,    80,
      83,    69,    37,   228,   229,    83,   351,    88,    98,    98,
     100,   100,   152,   248,   154,   155,   154,   155,    81,    96,
      97,    98,    56,    79,    55,    56,    61,    90,   118,   167,
      84,    66,    67,    92,    93,    69,    86,    68,    79,    89,
     118,    91,    80,   171,    82,   112,   113,   179,   230,   231,
     336,   337,   119,   120,   121,   122,    83,   197,   338,    86,
     236,   237,    89,   124,   154,   155,   166,    89,   272,    89,
     198,   199,   232,   233,   234,   235,   204,   167,    89,    89,
      99,   320,   100,     3,   101,   213,   214,   215,   216,   217,
     218,   371,    79,   221,   222,   223,   186,   186,   102,   189,
     189,   340,    14,    14,    88,   166,    80,    79,   248,   170,
     248,    60,    79,    88,   175,   243,    81,    88,   179,   257,
      21,    87,    15,    81,   256,    86,    89,    79,    88,    80,
      23,   269,   260,    88,    87,   374,     5,     6,     7,     8,
       9,    34,    35,    36,    88,    80,    39,    83,    79,    87,
      79,    88,    45,    46,    79,    88,    49,    40,   248,    46,
      46,   170,    46,   224,    57,    58,    46,   257,    46,    27,
     100,    56,   252,    42,   238,   257,   269,   239,    47,   269,
     320,   241,   272,   240,   242,   347,   154,   371,    -1,   175,
      -1,   252,   330,   248,   272,   256,    -1,    -1,   338,    -1,
     340,    -1,    -1,    -1,   332,    -1,    -1,    -1,   336,   337,
      -1,    -1,    81,    -1,   342,    -1,    -1,    86,   346,    -1,
      89,    -1,    91,   351,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   371,    -1,     1,   374,    -1,    -1,    -1,    -1,    -1,
     330,   330,    10,    11,    -1,    13,    -1,    15,    16,    -1,
      18,    19,    -1,    -1,    22,    23,    24,    25,    26,    27,
      28,    -1,    -1,    -1,    32,    33,    34,    35,    36,    37,
      38,    39,    -1,    -1,    42,    -1,    -1,    45,    46,    47,
      48,    49,    -1,    51,    -1,    53,    -1,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      -1,    79,    -1,    -1,    82,    -1,    -1,    -1,    -1,     1,
      -1,    89,    90,    -1,    92,    93,    94,    95,    10,    11,
      -1,    13,    -1,    15,    16,    -1,    18,    19,    -1,    -1,
      22,    23,    24,    25,    26,    27,    28,    -1,    -1,    -1,
      32,    33,    34,    35,    36,    37,    38,    39,    -1,    -1,
      42,    -1,    -1,    45,    46,    47,    48,    49,    -1,    51,
      -1,    53,    -1,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    -1,    79,    -1,    -1,
      82,    -1,    -1,    -1,    -1,     1,    -1,    89,    90,    -1,
      92,    93,    94,    95,    10,    11,    -1,    13,    -1,    15,
      16,    -1,    18,    19,    -1,    -1,    22,    23,    24,    25,
      26,    27,    28,    -1,    -1,    -1,    32,    33,    34,    35,
      36,    37,    38,    39,    -1,    -1,    42,    -1,    -1,    45,
      46,    47,    48,    49,    -1,    51,    -1,    53,    -1,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    -1,    79,    -1,    -1,    82,    -1,    -1,    -1,
      -1,     1,    -1,    89,    90,    -1,    92,    93,    94,    95,
      10,    -1,    -1,    13,    -1,    15,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    23,    24,    25,    26,    27,    -1,    -1,
      -1,    -1,    32,    -1,    34,    35,    36,    37,    38,    39,
      -1,    -1,    42,    -1,    -1,    45,    46,    47,    -1,    49,
      -1,    51,    -1,    53,    -1,    -1,    56,    57,    58,    59,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,    89,
      -1,    -1,    92,    93,    94,    95,    10,    11,    -1,    13,
      -1,    -1,    16,    -1,    18,    19,    -1,    -1,    -1,    -1,
      24,    25,    26,    27,    28,    -1,    -1,    -1,    32,    33,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,    -1,
      -1,    -1,    -1,    47,    48,    -1,    -1,    51,    -1,    53,
      -1,    -1,    56,    -1,    -1,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    -1,    -1,    -1,     1,
      -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,    10,    -1,
      -1,    13,    -1,    -1,    -1,    89,    90,    -1,    92,    93,
      94,    95,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,
      42,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,
      -1,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    -1,    -1,
      -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      82,    10,    -1,    -1,    13,    -1,    -1,    89,    90,    -1,
      92,    93,    94,    95,    -1,    24,    25,    26,    27,    -1,
      -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    38,
      -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,
      59,    -1,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    82,    10,    -1,    -1,    13,    -1,    -1,
      89,    90,    -1,    92,    93,    94,    95,    -1,    24,    25,
      26,    27,    -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    37,    38,    -1,    -1,    -1,    42,    -1,    -1,    -1,
      -1,    47,    -1,    -1,    -1,    51,    -1,    53,    -1,    -1,
      56,    -1,    -1,    59,    -1,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    -1,    -1,    -1,     1,    -1,    -1,
      -1,    -1,    -1,    79,    -1,    -1,    10,    -1,    -1,    13,
      -1,    -1,    -1,    89,    -1,    -1,    92,    93,    94,    95,
      24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,    -1,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,    53,
      -1,    -1,    56,    -1,    -1,    59,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    -1,    -1,    -1,     1,
      -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,    10,    -1,
      -1,    13,    -1,    -1,    -1,    89,    -1,    -1,    92,    93,
      94,    95,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,
      42,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,
      -1,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    -1,    -1,
      -1,     1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      10,    -1,    -1,    13,    -1,    -1,    -1,    89,    90,    -1,
      92,    93,    94,    95,    24,    25,    26,    27,    -1,    -1,
      -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    38,    -1,
      -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    -1,    -1,
      -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,    59,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,    79,
      -1,    -1,    10,    -1,    -1,    13,    -1,    -1,    -1,    89,
      -1,    -1,    92,    93,    94,    95,    24,    25,    26,    27,
      -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,
      38,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    47,
      -1,    -1,    -1,    51,    -1,    53,    -1,    -1,    56,    -1,
      -1,    59,    -1,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    10,    -1,    -1,    13,    -1,    -1,
      88,    89,    -1,    -1,    92,    93,    94,    95,    24,    25,
      26,    27,    -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    37,    38,    -1,    -1,    -1,    42,    -1,    -1,    -1,
      -1,    47,    -1,    -1,    -1,    51,    -1,    53,    -1,    -1,
      56,    -1,    -1,    59,    -1,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    -1,    -1,    -1,     1,    -1,    -1,
      -1,    -1,    -1,    79,    -1,    -1,    10,    -1,    -1,    13,
      -1,    -1,    -1,    89,    -1,    -1,    92,    93,    94,    95,
      24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,    -1,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,    53,
      -1,    -1,    56,    -1,    -1,    59,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    -1,    -1,    -1,     1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    10,    -1,
      -1,    13,    -1,    -1,    88,    89,    -1,    -1,    92,    93,
      94,    95,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,
      42,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,
      -1,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    -1,    -1,
      -1,     1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      10,    -1,    -1,    13,    -1,    -1,    -1,    89,    -1,    -1,
      92,    93,    94,    95,    24,    25,    26,    27,    -1,    -1,
      -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    38,    -1,
      -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    -1,    -1,
      -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,    59,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    10,    -1,    -1,    -1,    -1,    15,    89,
      -1,    -1,    92,    93,    94,    95,    23,    24,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,
      37,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,
      -1,    -1,    49,    -1,    51,    -1,    53,    -1,    55,    56,
      57,    58,    59,    -1,    61,    62,    63,    64,    65,    66,
      67,    68,     1,    70,    71,    72,    73,    74,    75,    76,
      77,    10,    -1,    -1,    -1,    -1,    15,    -1,    -1,    -1,
      -1,    88,    -1,    22,    23,    24,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    34,    35,    36,    37,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    46,    -1,    -1,
      49,    -1,    51,    -1,    53,    -1,    55,    56,    57,    58,
      59,    -1,    61,    62,    63,    64,    65,    66,    67,    68,
       1,    70,    71,    72,    73,    74,    75,    76,    77,    10,
      -1,    -1,    -1,    -1,    15,    -1,    85,    -1,    -1,    -1,
      -1,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    34,    35,    36,    37,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    46,    -1,    -1,    49,    -1,
      51,    -1,    53,    -1,    55,    56,    57,    58,    59,    -1,
      61,    62,    63,    64,    65,    66,    67,    68,     1,    70,
      71,    72,    73,    74,    75,    76,    77,    10,    -1,    -1,
      -1,    82,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,
      23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    35,    36,    37,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    46,    -1,    -1,    49,    -1,    51,    -1,
      53,    -1,    55,    56,    57,    58,    59,    -1,    61,    62,
      63,    64,    65,    66,    67,    68,     1,    70,    71,    72,
      73,    74,    75,    76,    77,    10,    -1,    -1,    -1,    82,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    24,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,
      35,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    46,    -1,    -1,    49,    -1,    51,    -1,    53,    -1,
      55,    56,    57,    58,    59,    -1,    61,    62,    63,    64,
      65,    66,    67,    68,    -1,    70,    71,    72,    73,    74,
      75,    76,    77,     0,     1,    -1,    -1,    82,    -1,    -1,
      -1,    -1,    -1,    10,    -1,    -1,    -1,    -1,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    22,    23,    24,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    32,    -1,    34,    35,    36,
      37,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,
      -1,    -1,    49,    -1,    51,    -1,    53,    -1,    55,    56,
      57,    58,    59,    -1,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,     1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      10,    -1,    -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,
      -1,    -1,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    32,    -1,    34,    35,    36,    37,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    46,    -1,    -1,    49,
      -1,    51,    -1,    53,    -1,    55,    56,    57,    58,    59,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    10,    -1,    -1,
      -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,
      23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    35,    36,    37,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    46,    -1,    -1,    49,    -1,    51,    -1,
      53,    -1,    55,    56,    57,    58,    59,    -1,    61,    62,
      63,    64,    65,    66,    67,    68,     1,    70,    71,    72,
      73,    74,    75,    76,    77,    10,    -1,    -1,    -1,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    24,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,
      35,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    46,    -1,    -1,    49,    -1,    51,    -1,    53,    -1,
      55,    56,    57,    58,    59,    -1,    61,    62,    63,    64,
      65,    66,    67,    68,     1,    70,    71,    72,    73,    74,
      75,    76,    77,    10,    -1,    -1,    -1,    -1,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    23,    24,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,
      37,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,
      -1,    -1,    49,    -1,    51,    -1,    53,    -1,    -1,    56,
      57,    58,    59,    -1,    61,    62,    63,    64,    65,    66,
      67,    68,    -1,    70,    71,    72,    73,    74,    75,    76,
      77
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    10,    15,    22,    23,    24,    32,    34,    35,
      36,    37,    39,    45,    46,    49,    51,    53,    55,    56,
      57,    58,    59,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
     104,   105,   106,   107,   109,   110,   111,   114,   115,   116,
     117,   118,   119,   120,   121,   123,   125,   128,   168,   169,
     195,   197,    79,   110,   124,   195,   197,   110,   124,    37,
      61,    66,    67,     0,   105,   107,   168,    79,   112,   113,
     135,   136,   137,   138,   197,    46,   115,   116,   117,   118,
     120,   110,   110,   110,   110,   110,   110,    90,   122,   179,
     122,    11,    13,    16,    18,    19,    25,    26,    27,    28,
      33,    38,    42,    47,    48,    60,    79,    82,    89,    92,
      93,    94,    95,   107,   109,   114,   146,   147,   148,   149,
     153,   154,   155,   156,   157,   158,   159,   160,   161,   162,
     163,   164,   165,   166,   167,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   181,   182,   183,   184,   185,
     186,   191,   193,   196,   197,   198,    83,   129,   179,    79,
      80,    81,    90,    84,   132,    83,    86,    89,   108,   109,
     110,   140,   141,   142,   143,   107,   126,   127,   168,   126,
      79,    79,    79,   114,   149,   167,   170,    89,    89,    89,
     153,   153,    79,   167,    89,   108,   110,   167,   153,   153,
     153,   153,   135,    89,     5,     6,     7,     8,     9,    42,
      47,    81,    86,    89,    91,    96,    97,    98,    92,    93,
      30,    41,    29,    40,    84,    85,    20,    43,    99,   100,
     101,     3,    44,   102,   170,   171,    82,   180,   181,    82,
     182,    79,    14,   194,   195,   197,   109,   130,   131,   113,
      90,   144,   167,   133,   194,    38,    87,   135,   139,    80,
      88,    88,    80,    82,   127,    82,    79,    60,   184,   188,
     189,   167,   187,    79,   187,    88,    88,   152,   167,   167,
     167,   167,   167,   167,   167,   167,   150,   151,   167,   192,
     197,   154,   154,   154,   155,   155,   156,   156,   157,   157,
     157,   157,   158,   158,   159,   160,   161,   162,   163,   167,
      21,   180,   147,   197,   135,    82,   131,    82,   144,   145,
     134,    87,    81,    86,   141,   108,    89,    79,    80,    88,
      88,   154,    80,    88,    87,    88,    80,    83,   171,   172,
      79,    80,    82,    85,   107,   144,    38,    87,   187,   187,
     190,   184,   171,   172,   167,   167,   165,    82,   144,    87,
      88,    79,    79,   188,    88,   171,   172
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,   103,   104,   104,   105,   105,   105,   105,   106,   107,
     107,   107,   108,   109,   109,   110,   110,   110,   110,   110,
     110,   110,   110,   111,   111,   111,   111,   111,   111,   111,
     112,   112,   113,   113,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   115,   116,   116,   117,   117,   118,
     118,   119,   119,   119,   119,   119,   119,   119,   119,   120,
     120,   120,   121,   121,   121,   122,   123,   123,   123,   124,
     124,   125,   126,   126,   127,   127,   128,   129,   130,   130,
     131,   133,   132,   134,   134,   135,   135,   136,   136,   137,
     137,   137,   137,   137,   138,   139,   139,   139,   140,   140,
     141,   141,   142,   142,   143,   143,   144,   144,   144,   144,
     145,   145,   146,   146,   147,   148,   148,   148,   148,   149,
     149,   149,   149,   149,   149,   150,   150,   151,   151,   152,
     152,   153,   153,   153,   153,   153,   153,   153,   154,   154,
     155,   155,   155,   155,   156,   156,   156,   157,   157,   157,
     158,   158,   158,   158,   158,   159,   159,   159,   160,   160,
     161,   161,   162,   162,   163,   163,   164,   164,   165,   165,
     166,   167,   168,   168,   169,   170,   170,   171,   171,   171,
     171,   171,   171,   171,   172,   172,   173,   173,   174,   174,
     175,   176,   176,   177,   178,   178,   179,   180,   181,   181,
     182,   182,   183,   183,   184,   184,   184,   184,   184,   184,
     184,   185,   185,   185,   186,   186,   187,   188,   188,   189,
     189,   190,   190,   191,   191,   192,   193,   194,   195,   196,
     197,   197,   198,   198,   198,   198,   198
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     1,     1,     2,     2,     1,     2,
       3,     2,     2,     1,     2,     1,     2,     2,     2,     2,
       2,     2,     2,     1,     2,     2,     2,     2,     2,     2,
       1,     3,     1,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     2,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     4,     4,     1,     1,     2,     4,     4,     1,
       1,     1,     1,     2,     1,     1,     5,     1,     1,     2,
       3,     0,     4,     0,     2,     1,     2,     1,     3,     1,
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
#line 286 "parser.y"
                              { (yyval.dummy) = GlobalInitStatements(CurrentScope, (yyvsp[0].sc_stmt)); }
#line 2003 "parser.c"
    break;

  case 5: /* external_declaration: function_definition  */
#line 288 "parser.y"
                              { (yyval.dummy) = 0; }
#line 2009 "parser.c"
    break;

  case 6: /* external_declaration: profile_specifier function_definition  */
#line 290 "parser.y"
                              { (yyval.dummy) = 0; }
#line 2015 "parser.c"
    break;

  case 7: /* external_declaration: profile_specifier declaration  */
#line 292 "parser.y"
                              { (yyval.dummy) = GlobalInitStatements(CurrentScope, (yyvsp[0].sc_stmt)); ClearPendingProfileSpecifier(); }
#line 2021 "parser.c"
    break;

  case 8: /* profile_specifier: identifier  */
#line 302 "parser.y"
                              { (yyval.sc_ident) = (yyvsp[0].sc_ident); SetPendingProfileSpecifier(Cg->tokenLoc, (yyvsp[0].sc_ident)); }
#line 2027 "parser.c"
    break;

  case 9: /* declaration: declaration_specifiers ';'  */
#line 306 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2033 "parser.c"
    break;

  case 10: /* declaration: declaration_specifiers init_declarator_list ';'  */
#line 308 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[-1].sc_stmt); }
#line 2039 "parser.c"
    break;

  case 11: /* declaration: ERROR_SY ';'  */
#line 310 "parser.y"
                              { RecordErrorPos(Cg->tokenLoc);
                                ClearPendingGeometryModifiers();
                                (yyval.sc_stmt) = NULL; }
#line 2047 "parser.c"
    break;

  case 12: /* abstract_declaration: abstract_declaration_specifiers abstract_declarator  */
#line 316 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2053 "parser.c"
    break;

  case 13: /* declaration_specifiers: abstract_declaration_specifiers  */
#line 323 "parser.y"
                              { (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 2059 "parser.c"
    break;

  case 14: /* declaration_specifiers: TYPEDEF_SY abstract_declaration_specifiers  */
#line 325 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, TYPE_MISC_TYPEDEF); (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 2065 "parser.c"
    break;

  case 15: /* abstract_declaration_specifiers: abstract_declaration_specifiers2  */
#line 330 "parser.y"
                              { (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 2071 "parser.c"
    break;

  case 16: /* abstract_declaration_specifiers: type_qualifier abstract_declaration_specifiers  */
#line 332 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2077 "parser.c"
    break;

  case 17: /* abstract_declaration_specifiers: storage_class abstract_declaration_specifiers  */
#line 334 "parser.y"
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2083 "parser.c"
    break;

  case 18: /* abstract_declaration_specifiers: type_domain abstract_declaration_specifiers  */
#line 336 "parser.y"
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2089 "parser.c"
    break;

  case 19: /* abstract_declaration_specifiers: in_out abstract_declaration_specifiers  */
#line 338 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2095 "parser.c"
    break;

  case 20: /* abstract_declaration_specifiers: function_specifier abstract_declaration_specifiers  */
#line 340 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2101 "parser.c"
    break;

  case 21: /* abstract_declaration_specifiers: geometry_modifier abstract_declaration_specifiers  */
#line 345 "parser.y"
                              { (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2107 "parser.c"
    break;

  case 22: /* abstract_declaration_specifiers: PACKED_SY abstract_declaration_specifiers  */
#line 347 "parser.y"
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2113 "parser.c"
    break;

  case 23: /* abstract_declaration_specifiers2: type_specifier  */
#line 352 "parser.y"
                              { (yyval.sc_type) = *SetDType(&CurrentDeclTypeSpecs, (yyvsp[0].sc_ptype)); }
#line 2119 "parser.c"
    break;

  case 24: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 type_qualifier  */
#line 354 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2125 "parser.c"
    break;

  case 25: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 storage_class  */
#line 356 "parser.y"
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2131 "parser.c"
    break;

  case 26: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 type_domain  */
#line 358 "parser.y"
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2137 "parser.c"
    break;

  case 27: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 in_out  */
#line 360 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2143 "parser.c"
    break;

  case 28: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 function_specifier  */
#line 362 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2149 "parser.c"
    break;

  case 29: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 PACKED_SY  */
#line 364 "parser.y"
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2155 "parser.c"
    break;

  case 30: /* init_declarator_list: init_declarator  */
#line 368 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[0].sc_stmt); }
#line 2161 "parser.c"
    break;

  case 31: /* init_declarator_list: init_declarator_list ',' init_declarator  */
#line 370 "parser.y"
                              { (yyval.sc_stmt) = AddStmt((yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2167 "parser.c"
    break;

  case 32: /* init_declarator: declarator  */
#line 374 "parser.y"
                              { (yyval.sc_stmt) = Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_decl), NULL); }
#line 2173 "parser.c"
    break;

  case 33: /* init_declarator: declarator '=' initializer  */
#line 376 "parser.y"
                              { (yyval.sc_stmt) = Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[0].sc_expr)); }
#line 2179 "parser.c"
    break;

  case 34: /* type_specifier: INT_SY  */
#line 384 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, INT_SY); }
#line 2185 "parser.c"
    break;

  case 35: /* type_specifier: FLOAT_SY  */
#line 386 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, FLOAT_SY); }
#line 2191 "parser.c"
    break;

  case 36: /* type_specifier: VOID_SY  */
#line 388 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, VOID_SY); }
#line 2197 "parser.c"
    break;

  case 37: /* type_specifier: BOOLEAN_SY  */
#line 390 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, BOOLEAN_SY); }
#line 2203 "parser.c"
    break;

  case 38: /* type_specifier: TEXOBJ_SY  */
#line 392 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, TEXOBJ_SY); }
#line 2209 "parser.c"
    break;

  case 39: /* type_specifier: CHAR_SY  */
#line 394 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2215 "parser.c"
    break;

  case 40: /* type_specifier: SHORT_SY  */
#line 396 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2221 "parser.c"
    break;

  case 41: /* type_specifier: LONG_SY  */
#line 398 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2227 "parser.c"
    break;

  case 42: /* type_specifier: HALF_SY  */
#line 400 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2233 "parser.c"
    break;

  case 43: /* type_specifier: FIXED_SY  */
#line 402 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2239 "parser.c"
    break;

  case 44: /* type_specifier: DOUBLE_SY  */
#line 404 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2245 "parser.c"
    break;

  case 45: /* type_specifier: UNSIGNED_SY  */
#line 406 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2251 "parser.c"
    break;

  case 46: /* type_specifier: UNSIGNED_SY CHAR_SY  */
#line 408 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2257 "parser.c"
    break;

  case 47: /* type_specifier: UNSIGNED_SY SHORT_SY  */
#line 410 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2263 "parser.c"
    break;

  case 48: /* type_specifier: UNSIGNED_SY INT_SY  */
#line 412 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2269 "parser.c"
    break;

  case 49: /* type_specifier: UNSIGNED_SY LONG_SY  */
#line 414 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2275 "parser.c"
    break;

  case 50: /* type_specifier: struct_or_connector_specifier  */
#line 416 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2281 "parser.c"
    break;

  case 51: /* type_specifier: interface_specifier  */
#line 418 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2287 "parser.c"
    break;

  case 52: /* type_specifier: type_identifier  */
#line 420 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, (yyvsp[0].sc_ident)); }
#line 2293 "parser.c"
    break;

  case 53: /* type_specifier: error  */
#line 422 "parser.y"
                              {
                                ClearPendingGeometryModifiers();
                                SemanticParseError(Cg->tokenLoc, ERROR_S_TYPE_NAME_EXPECTED,
                                                   GetAtomString(atable, Cg->mostRecentToken /* yychar */));
                                (yyval.sc_ptype) = UndefinedType;
                              }
#line 2304 "parser.c"
    break;

  case 54: /* type_qualifier: CONST_SY  */
#line 435 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_CONST; }
#line 2310 "parser.c"
    break;

  case 55: /* type_domain: UNIFORM_SY  */
#line 443 "parser.y"
                              { (yyval.sc_int) = TYPE_DOMAIN_UNIFORM; }
#line 2316 "parser.c"
    break;

  case 56: /* type_domain: VARYING_SY  */
#line 445 "parser.y"
                              { (yyval.sc_int) = TYPE_DOMAIN_VARYING; }
#line 2322 "parser.c"
    break;

  case 57: /* storage_class: STATIC_SY  */
#line 453 "parser.y"
                              { (yyval.sc_int) = (int) SC_STATIC; }
#line 2328 "parser.c"
    break;

  case 58: /* storage_class: EXTERN_SY  */
#line 455 "parser.y"
                              { (yyval.sc_int) = (int) SC_EXTERN; }
#line 2334 "parser.c"
    break;

  case 59: /* function_specifier: INLINE_SY  */
#line 463 "parser.y"
                              { (yyval.sc_int) = TYPE_MISC_INLINE; }
#line 2340 "parser.c"
    break;

  case 60: /* function_specifier: INTERNAL_SY  */
#line 465 "parser.y"
                              { (yyval.sc_int) = TYPE_MISC_INTERNAL; }
#line 2346 "parser.c"
    break;

  case 61: /* geometry_modifier: POINT_SY  */
#line 478 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_POINT); }
#line 2352 "parser.c"
    break;

  case 62: /* geometry_modifier: LINE_SY  */
#line 479 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_LINE); }
#line 2358 "parser.c"
    break;

  case 63: /* geometry_modifier: LINE_ADJ_SY  */
#line 480 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_LINE_ADJACENCY); }
#line 2364 "parser.c"
    break;

  case 64: /* geometry_modifier: TRIANGLE_SY  */
#line 481 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_TRIANGLE); }
#line 2370 "parser.c"
    break;

  case 65: /* geometry_modifier: TRIANGLE_ADJ_SY  */
#line 482 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY); }
#line 2376 "parser.c"
    break;

  case 66: /* geometry_modifier: POINT_OUT_SY  */
#line 483 "parser.y"
                          { (yyval.sc_int) = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_POINTS); }
#line 2382 "parser.c"
    break;

  case 67: /* geometry_modifier: LINE_OUT_SY  */
#line 484 "parser.y"
                          { (yyval.sc_int) = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_LINE_STRIP); }
#line 2388 "parser.c"
    break;

  case 68: /* geometry_modifier: TRIANGLE_OUT_SY  */
#line 485 "parser.y"
                          { (yyval.sc_int) = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP); }
#line 2394 "parser.c"
    break;

  case 69: /* in_out: IN_SY  */
#line 493 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_IN; }
#line 2400 "parser.c"
    break;

  case 70: /* in_out: OUT_SY  */
#line 495 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_OUT; }
#line 2406 "parser.c"
    break;

  case 71: /* in_out: INOUT_SY  */
#line 497 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_INOUT; }
#line 2412 "parser.c"
    break;

  case 72: /* struct_or_connector_specifier: struct_or_connector_header struct_compound_header struct_declaration_list '}'  */
#line 506 "parser.y"
                              { (yyval.sc_ptype) = SetStructMembers(Cg->tokenLoc, (yyvsp[-3].sc_ptype), PopScope());
                                CheckInterfaceConformance(Cg->tokenLoc, (yyval.sc_ptype)); }
#line 2419 "parser.c"
    break;

  case 73: /* struct_or_connector_specifier: untagged_struct_header struct_compound_header struct_declaration_list '}'  */
#line 509 "parser.y"
                              { (yyval.sc_ptype) = SetStructMembers(Cg->tokenLoc, (yyvsp[-3].sc_ptype), PopScope());
                                CheckInterfaceConformance(Cg->tokenLoc, (yyval.sc_ptype)); }
#line 2426 "parser.c"
    break;

  case 74: /* struct_or_connector_specifier: struct_or_connector_header  */
#line 512 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2432 "parser.c"
    break;

  case 75: /* struct_compound_header: compound_header  */
#line 516 "parser.y"
                              { CurrentScope->IsStructScope = 1; (yyval.dummy) = (yyvsp[0].dummy); }
#line 2438 "parser.c"
    break;

  case 76: /* struct_or_connector_header: STRUCT_SY struct_identifier  */
#line 521 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, 0, (yyvsp[0].sc_ident)); }
#line 2444 "parser.c"
    break;

  case 77: /* struct_or_connector_header: STRUCT_SY struct_identifier ':' semantics_identifier  */
#line 523 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_ident), (yyvsp[-2].sc_ident)); }
#line 2450 "parser.c"
    break;

  case 78: /* struct_or_connector_header: STRUCT_SY struct_identifier ':' type_identifier  */
#line 525 "parser.y"
                              { (yyval.sc_ptype) = SetStructInterface(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_ident), (yyvsp[0].sc_ident)); }
#line 2456 "parser.c"
    break;

  case 81: /* untagged_struct_header: STRUCT_SY  */
#line 533 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, 0, 0); }
#line 2462 "parser.c"
    break;

  case 84: /* struct_declaration: declaration  */
#line 541 "parser.y"
                            { (yyval.sc_stmt) = (yyvsp[0].sc_stmt); }
#line 2468 "parser.c"
    break;

  case 85: /* struct_declaration: function_definition  */
#line 543 "parser.y"
                            { (yyval.sc_stmt) = NULL; }
#line 2474 "parser.c"
    break;

  case 86: /* interface_specifier: INTERFACE_SY struct_identifier interface_compound_header interface_member_declaration_list '}'  */
#line 554 "parser.y"
                              { (yyval.sc_ptype) = SetInterfaceMembers(Cg->tokenLoc,
                                                         InterfaceHeader(Cg->tokenLoc, CurrentScope, (yyvsp[-3].sc_ident)),
                                                         PopScope()); }
#line 2482 "parser.c"
    break;

  case 87: /* interface_compound_header: compound_header  */
#line 561 "parser.y"
                              { CurrentScope->IsStructScope = 1; (yyval.dummy) = (yyvsp[0].dummy); }
#line 2488 "parser.c"
    break;

  case 90: /* interface_member_declaration: declaration_specifiers declarator ';'  */
#line 578 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2494 "parser.c"
    break;

  case 91: /* $@1: %empty  */
#line 596 "parser.y"
                              { PushScope(NewScope()); }
#line 2500 "parser.c"
    break;

  case 92: /* annotation: '<' $@1 annotation_decl_list '>'  */
#line 597 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[-1].sc_stmt); PopScope(); }
#line 2506 "parser.c"
    break;

  case 93: /* annotation_decl_list: %empty  */
#line 601 "parser.y"
                              { (yyval.sc_stmt) = 0; }
#line 2512 "parser.c"
    break;

  case 95: /* declarator: semantic_declarator  */
#line 610 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2518 "parser.c"
    break;

  case 96: /* declarator: semantic_declarator annotation  */
#line 612 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[-1].sc_decl); }
#line 2524 "parser.c"
    break;

  case 97: /* semantic_declarator: basic_declarator  */
#line 616 "parser.y"
                              { (yyval.sc_decl) = Declarator(Cg->tokenLoc, (yyvsp[0].sc_decl), 0); }
#line 2530 "parser.c"
    break;

  case 98: /* semantic_declarator: basic_declarator ':' semantics_identifier  */
#line 618 "parser.y"
                              { (yyval.sc_decl) = Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), (yyvsp[0].sc_ident)); }
#line 2536 "parser.c"
    break;

  case 99: /* basic_declarator: identifier  */
#line 622 "parser.y"
                              { (yyval.sc_decl) = NewDeclNode(Cg->tokenLoc, (yyvsp[0].sc_ident), &CurrentDeclTypeSpecs); }
#line 2542 "parser.c"
    break;

  case 100: /* basic_declarator: basic_declarator '[' INTCONST_SY ']'  */
#line 624 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-3].sc_decl), (int) (yyvsp[-1].sc_literal).value.i, 0); }
#line 2548 "parser.c"
    break;

  case 101: /* basic_declarator: basic_declarator '[' ']'  */
#line 626 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), 0 , 1); }
#line 2554 "parser.c"
    break;

  case 102: /* basic_declarator: function_decl_header parameter_list ')'  */
#line 628 "parser.y"
                              { (yyval.sc_decl) = SetFunTypeParams(CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_decl), (yyvsp[-1].sc_decl)); }
#line 2560 "parser.c"
    break;

  case 103: /* basic_declarator: function_decl_header abstract_parameter_list ')'  */
#line 630 "parser.y"
                              { (yyval.sc_decl) = SetFunTypeParams(CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_decl), NULL); }
#line 2566 "parser.c"
    break;

  case 104: /* function_decl_header: basic_declarator '('  */
#line 634 "parser.y"
                              { (yyval.sc_decl) = FunctionDeclHeader(&(yyvsp[-1].sc_decl)->loc, CurrentScope, (yyvsp[-1].sc_decl)); }
#line 2572 "parser.c"
    break;

  case 105: /* abstract_declarator: %empty  */
#line 638 "parser.y"
                              { (yyval.sc_decl) = NewDeclNode(Cg->tokenLoc, 0, &CurrentDeclTypeSpecs); }
#line 2578 "parser.c"
    break;

  case 106: /* abstract_declarator: abstract_declarator '[' INTCONST_SY ']'  */
#line 640 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-3].sc_decl), (int) (yyvsp[-1].sc_literal).value.i, 0); }
#line 2584 "parser.c"
    break;

  case 107: /* abstract_declarator: abstract_declarator '[' ']'  */
#line 642 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), 0 , 1); }
#line 2590 "parser.c"
    break;

  case 108: /* parameter_list: parameter_declaration  */
#line 659 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2596 "parser.c"
    break;

  case 109: /* parameter_list: parameter_list ',' parameter_declaration  */
#line 661 "parser.y"
                              { (yyval.sc_decl) = AddDecl((yyvsp[-2].sc_decl), (yyvsp[0].sc_decl)); }
#line 2602 "parser.c"
    break;

  case 110: /* parameter_declaration: declaration_specifiers declarator  */
#line 665 "parser.y"
                              { (yyval.sc_decl) = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_decl), NULL); }
#line 2608 "parser.c"
    break;

  case 111: /* parameter_declaration: declaration_specifiers declarator '=' initializer  */
#line 667 "parser.y"
                              { (yyval.sc_decl) = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[0].sc_expr)); }
#line 2614 "parser.c"
    break;

  case 112: /* abstract_parameter_list: %empty  */
#line 671 "parser.y"
                              { (yyval.sc_decl) = NULL; }
#line 2620 "parser.c"
    break;

  case 114: /* non_empty_abstract_parameter_list: abstract_declaration  */
#line 676 "parser.y"
                              {
                                if (IsVoid(&(yyvsp[0].sc_decl)->type.type))
                                    CurrentScope->HasVoidParameter = 1;
                                (yyval.sc_decl) = (yyvsp[0].sc_decl);
                              }
#line 2630 "parser.c"
    break;

  case 115: /* non_empty_abstract_parameter_list: non_empty_abstract_parameter_list ',' abstract_declaration  */
#line 682 "parser.y"
                              {
                                if (CurrentScope->HasVoidParameter || IsVoid(&(yyvsp[-2].sc_decl)->type.type)) {
                                    SemanticError(Cg->tokenLoc, ERROR___VOID_NOT_ONLY_PARAM);
                                }
                                (yyval.sc_decl) = AddDecl((yyvsp[-2].sc_decl), (yyvsp[0].sc_decl));
                              }
#line 2641 "parser.c"
    break;

  case 116: /* initializer: expression  */
#line 695 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[0].sc_expr)); }
#line 2647 "parser.c"
    break;

  case 117: /* initializer: '{' initializer_list '}'  */
#line 697 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[-1].sc_expr)); }
#line 2653 "parser.c"
    break;

  case 118: /* initializer: '{' initializer_list ',' '}'  */
#line 699 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[-2].sc_expr)); }
#line 2659 "parser.c"
    break;

  case 119: /* initializer: '{' '}'  */
#line 705 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, NULL); }
#line 2665 "parser.c"
    break;

  case 120: /* initializer_list: initializer  */
#line 709 "parser.y"
                              { (yyval.sc_expr) = InitializerList(Cg->tokenLoc, (yyvsp[0].sc_expr), NULL); }
#line 2671 "parser.c"
    break;

  case 121: /* initializer_list: initializer_list ',' initializer  */
#line 711 "parser.y"
                              { (yyval.sc_expr) = InitializerList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2677 "parser.c"
    break;

  case 122: /* variable: basic_variable  */
#line 723 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[0].sc_expr); }
#line 2683 "parser.c"
    break;

  case 123: /* variable: scope_identifier COLONCOLON_SY basic_variable  */
#line 725 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[0].sc_expr); }
#line 2689 "parser.c"
    break;

  case 124: /* basic_variable: variable_identifier  */
#line 729 "parser.y"
                              { (yyval.sc_expr) = BasicVariable(Cg->tokenLoc, (yyvsp[0].sc_ident)); }
#line 2695 "parser.c"
    break;

  case 127: /* primary_expression: '(' expression ')'  */
#line 739 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[-1].sc_expr); }
#line 2701 "parser.c"
    break;

  case 128: /* primary_expression: type_specifier '(' expression_list ')'  */
#line 741 "parser.y"
                              { (yyval.sc_expr) = NewVectorConstructor(Cg->tokenLoc, (yyvsp[-3].sc_ptype), (yyvsp[-1].sc_expr)); }
#line 2707 "parser.c"
    break;

  case 130: /* postfix_expression: postfix_expression PLUSPLUS_SY  */
#line 750 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(POSTINC_OP, (yyvsp[-1].sc_expr)); }
#line 2713 "parser.c"
    break;

  case 131: /* postfix_expression: postfix_expression MINUSMINUS_SY  */
#line 752 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(POSTDEC_OP, (yyvsp[-1].sc_expr)); }
#line 2719 "parser.c"
    break;

  case 132: /* postfix_expression: postfix_expression '.' member_identifier  */
#line 754 "parser.y"
                              { (yyval.sc_expr) = NewMemberSelectorOrSwizzleOrWriteMaskOperator(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_ident)); }
#line 2725 "parser.c"
    break;

  case 133: /* postfix_expression: postfix_expression '[' expression ']'  */
#line 756 "parser.y"
                              { (yyval.sc_expr) = NewIndexOperator(Cg->tokenLoc, (yyvsp[-3].sc_expr), (yyvsp[-1].sc_expr)); }
#line 2731 "parser.c"
    break;

  case 134: /* postfix_expression: postfix_expression '(' actual_argument_list ')'  */
#line 758 "parser.y"
                              { (yyval.sc_expr) = NewFunctionCallOperator(Cg->tokenLoc, (yyvsp[-3].sc_expr), (yyvsp[-1].sc_expr)); }
#line 2737 "parser.c"
    break;

  case 135: /* actual_argument_list: %empty  */
#line 762 "parser.y"
                                { (yyval.sc_expr) = NULL; }
#line 2743 "parser.c"
    break;

  case 137: /* non_empty_argument_list: expression  */
#line 767 "parser.y"
                              { (yyval.sc_expr) = ArgumentList(Cg->tokenLoc, NULL, (yyvsp[0].sc_expr)); }
#line 2749 "parser.c"
    break;

  case 138: /* non_empty_argument_list: non_empty_argument_list ',' expression  */
#line 769 "parser.y"
                              { (yyval.sc_expr) = ArgumentList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2755 "parser.c"
    break;

  case 139: /* expression_list: expression  */
#line 773 "parser.y"
                              { (yyval.sc_expr) = ExpressionList(Cg->tokenLoc, NULL, (yyvsp[0].sc_expr)); }
#line 2761 "parser.c"
    break;

  case 140: /* expression_list: expression_list ',' expression  */
#line 775 "parser.y"
                              { (yyval.sc_expr) = ExpressionList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2767 "parser.c"
    break;

  case 142: /* unary_expression: PLUSPLUS_SY unary_expression  */
#line 784 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(PREINC_OP, (yyvsp[0].sc_expr)); }
#line 2773 "parser.c"
    break;

  case 143: /* unary_expression: MINUSMINUS_SY unary_expression  */
#line 786 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(PREDEC_OP, (yyvsp[0].sc_expr)); }
#line 2779 "parser.c"
    break;

  case 144: /* unary_expression: '+' unary_expression  */
#line 788 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, POS_OP, '+', (yyvsp[0].sc_expr), 0); }
#line 2785 "parser.c"
    break;

  case 145: /* unary_expression: '-' unary_expression  */
#line 790 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, NEG_OP, '-', (yyvsp[0].sc_expr), 0); }
#line 2791 "parser.c"
    break;

  case 146: /* unary_expression: '!' unary_expression  */
#line 792 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, BNOT_OP, '!', (yyvsp[0].sc_expr), 0); }
#line 2797 "parser.c"
    break;

  case 147: /* unary_expression: '~' unary_expression  */
#line 794 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, NOT_OP, '~', (yyvsp[0].sc_expr), 1); }
#line 2803 "parser.c"
    break;

  case 149: /* cast_expression: '(' abstract_declaration ')' cast_expression  */
#line 806 "parser.y"
                              { (yyval.sc_expr) = NewCastOperator(Cg->tokenLoc, (yyvsp[0].sc_expr), GetTypePointer(&(yyvsp[-2].sc_decl)->loc, &(yyvsp[-2].sc_decl)->type)); }
#line 2809 "parser.c"
    break;

  case 151: /* multiplicative_expression: multiplicative_expression '*' cast_expression  */
#line 815 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, MUL_OP, '*', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2815 "parser.c"
    break;

  case 152: /* multiplicative_expression: multiplicative_expression '/' cast_expression  */
#line 817 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, DIV_OP, '/', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2821 "parser.c"
    break;

  case 153: /* multiplicative_expression: multiplicative_expression '%' cast_expression  */
#line 819 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, MOD_OP, '%', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2827 "parser.c"
    break;

  case 155: /* additive_expression: additive_expression '+' multiplicative_expression  */
#line 828 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, ADD_OP, '+', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2833 "parser.c"
    break;

  case 156: /* additive_expression: additive_expression '-' multiplicative_expression  */
#line 830 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SUB_OP, '-', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2839 "parser.c"
    break;

  case 158: /* shift_expression: shift_expression LL_SY additive_expression  */
#line 839 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SHL_OP, LL_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2845 "parser.c"
    break;

  case 159: /* shift_expression: shift_expression GG_SY additive_expression  */
#line 841 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SHR_OP, GG_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2851 "parser.c"
    break;

  case 161: /* relational_expression: relational_expression '<' shift_expression  */
#line 850 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, LT_OP, '<', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2857 "parser.c"
    break;

  case 162: /* relational_expression: relational_expression '>' shift_expression  */
#line 852 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, GT_OP, '>', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2863 "parser.c"
    break;

  case 163: /* relational_expression: relational_expression LE_SY shift_expression  */
#line 854 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, LE_OP, LE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2869 "parser.c"
    break;

  case 164: /* relational_expression: relational_expression GE_SY shift_expression  */
#line 856 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, GE_OP, GE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2875 "parser.c"
    break;

  case 166: /* equality_expression: equality_expression EQ_SY relational_expression  */
#line 865 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, EQ_OP, EQ_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2881 "parser.c"
    break;

  case 167: /* equality_expression: equality_expression NE_SY relational_expression  */
#line 867 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, NE_OP, NE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2887 "parser.c"
    break;

  case 169: /* AND_expression: AND_expression '&' equality_expression  */
#line 876 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, AND_OP, '&', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2893 "parser.c"
    break;

  case 171: /* exclusive_OR_expression: exclusive_OR_expression '^' AND_expression  */
#line 885 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, XOR_OP, '^', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2899 "parser.c"
    break;

  case 173: /* inclusive_OR_expression: inclusive_OR_expression '|' exclusive_OR_expression  */
#line 894 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, OR_OP, '|', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2905 "parser.c"
    break;

  case 175: /* logical_AND_expression: logical_AND_expression AND_SY inclusive_OR_expression  */
#line 903 "parser.y"
                              { (yyval.sc_expr) = NewBinaryBooleanOperator(Cg->tokenLoc, BAND_OP, AND_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2911 "parser.c"
    break;

  case 177: /* logical_OR_expression: logical_OR_expression OR_SY logical_AND_expression  */
#line 912 "parser.y"
                              { (yyval.sc_expr) = NewBinaryBooleanOperator(Cg->tokenLoc, BOR_OP, OR_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2917 "parser.c"
    break;

  case 179: /* conditional_expression: conditional_test '?' expression ':' conditional_expression  */
#line 921 "parser.y"
                              { (yyval.sc_expr) = NewConditionalOperator(Cg->tokenLoc, (yyvsp[-4].sc_expr), (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2923 "parser.c"
    break;

  case 180: /* conditional_test: logical_OR_expression  */
#line 925 "parser.y"
                              {  (yyval.sc_expr) = CheckBooleanExpr(Cg->tokenLoc, (yyvsp[0].sc_expr), 1); }
#line 2929 "parser.c"
    break;

  case 182: /* function_definition: function_definition_header block_item_list '}'  */
#line 944 "parser.y"
                              { DefineFunction(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_stmt)); PopScope();
                                ResumeStructScopeAfterMethodBody(); }
#line 2936 "parser.c"
    break;

  case 183: /* function_definition: function_definition_header '}'  */
#line 947 "parser.y"
                              { DefineFunction(Cg->tokenLoc, CurrentScope, (yyvsp[-1].sc_decl), NULL); PopScope();
                                ResumeStructScopeAfterMethodBody(); }
#line 2943 "parser.c"
    break;

  case 184: /* function_definition_header: declaration_specifiers declarator '{'  */
#line 952 "parser.y"
                              { (yyval.sc_decl) = Function_Definition_Header(Cg->tokenLoc, (yyvsp[-1].sc_decl)); }
#line 2949 "parser.c"
    break;

  case 196: /* discard_statement: DISCARD_SY ';'  */
#line 981 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewDiscardStmt(Cg->tokenLoc, NULL); }
#line 2955 "parser.c"
    break;

  case 197: /* discard_statement: DISCARD_SY expression ';'  */
#line 983 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewDiscardStmt(Cg->tokenLoc, CheckBooleanExpr(Cg->tokenLoc, (yyvsp[-1].sc_expr), 1)); }
#line 2961 "parser.c"
    break;

  case 198: /* jump_statement: BREAK_SY ';'  */
#line 991 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewSimpleStmt(Cg->tokenLoc, BREAK_STMT); }
#line 2967 "parser.c"
    break;

  case 199: /* jump_statement: CONTINUE_SY ';'  */
#line 993 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewSimpleStmt(Cg->tokenLoc, CONTINUE_STMT); }
#line 2973 "parser.c"
    break;

  case 200: /* if_statement: if_header balanced_statement ELSE_SY balanced_statement  */
#line 1001 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-3].sc_stmt), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2979 "parser.c"
    break;

  case 201: /* dangling_if: if_header statement  */
#line 1005 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-1].sc_stmt), (yyvsp[0].sc_stmt), NULL); }
#line 2985 "parser.c"
    break;

  case 202: /* dangling_if: if_header balanced_statement ELSE_SY dangling_statement  */
#line 1007 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-3].sc_stmt), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2991 "parser.c"
    break;

  case 203: /* if_header: IF_SY '(' boolean_scalar_expression ')'  */
#line 1011 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewIfStmt(Cg->tokenLoc, (yyvsp[-1].sc_expr), NULL, NULL); ; }
#line 2997 "parser.c"
    break;

  case 204: /* compound_statement: compound_header block_item_list compound_tail  */
#line 1019 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewBlockStmt(Cg->tokenLoc, (yyvsp[-1].sc_stmt)); }
#line 3003 "parser.c"
    break;

  case 205: /* compound_statement: compound_header compound_tail  */
#line 1021 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 3009 "parser.c"
    break;

  case 206: /* compound_header: '{'  */
#line 1025 "parser.y"
                              { PushScope(NewScope()); CurrentScope->funindex = NextFunctionIndex; }
#line 3015 "parser.c"
    break;

  case 207: /* compound_tail: '}'  */
#line 1029 "parser.y"
                              {
                                if (Cg->options.DumpParseTree)
                                    PrintScopeDeclarations();
                                PopScope();
                              }
#line 3025 "parser.c"
    break;

  case 209: /* block_item_list: block_item_list block_item  */
#line 1038 "parser.y"
                              { (yyval.sc_stmt) = AddStmt((yyvsp[-1].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3031 "parser.c"
    break;

  case 211: /* block_item: statement  */
#line 1043 "parser.y"
                              { (yyval.sc_stmt) = CheckStmt((yyvsp[0].sc_stmt)); }
#line 3037 "parser.c"
    break;

  case 213: /* expression_statement: ';'  */
#line 1052 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 3043 "parser.c"
    break;

  case 214: /* expression_statement2: postfix_expression '=' expression  */
#line 1056 "parser.y"
                              { (yyval.sc_stmt) = NewSimpleAssignmentStmt(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 3049 "parser.c"
    break;

  case 215: /* expression_statement2: expression  */
#line 1058 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewExprStmt(Cg->tokenLoc, (yyvsp[0].sc_expr)); }
#line 3055 "parser.c"
    break;

  case 216: /* expression_statement2: postfix_expression ASSIGNMINUS_SY expression  */
#line 1060 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMINUS_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3061 "parser.c"
    break;

  case 217: /* expression_statement2: postfix_expression ASSIGNMOD_SY expression  */
#line 1062 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMOD_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3067 "parser.c"
    break;

  case 218: /* expression_statement2: postfix_expression ASSIGNPLUS_SY expression  */
#line 1064 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNPLUS_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3073 "parser.c"
    break;

  case 219: /* expression_statement2: postfix_expression ASSIGNSLASH_SY expression  */
#line 1066 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSLASH_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3079 "parser.c"
    break;

  case 220: /* expression_statement2: postfix_expression ASSIGNSTAR_SY expression  */
#line 1068 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSTAR_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3085 "parser.c"
    break;

  case 221: /* iteration_statement: WHILE_SY '(' boolean_scalar_expression ')' balanced_statement  */
#line 1076 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, (yyvsp[-2].sc_expr), (yyvsp[0].sc_stmt)); }
#line 3091 "parser.c"
    break;

  case 222: /* iteration_statement: DO_SY statement WHILE_SY '(' boolean_scalar_expression ')' ';'  */
#line 1078 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, DO_STMT, (yyvsp[-2].sc_expr), (yyvsp[-5].sc_stmt)); }
#line 3097 "parser.c"
    break;

  case 223: /* iteration_statement: FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' balanced_statement  */
#line 1080 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewForStmt(Cg->tokenLoc, (yyvsp[-6].sc_stmt), (yyvsp[-4].sc_expr), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3103 "parser.c"
    break;

  case 224: /* dangling_iteration: WHILE_SY '(' boolean_scalar_expression ')' dangling_statement  */
#line 1084 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, (yyvsp[-2].sc_expr), (yyvsp[0].sc_stmt)); }
#line 3109 "parser.c"
    break;

  case 225: /* dangling_iteration: FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' dangling_statement  */
#line 1086 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewForStmt(Cg->tokenLoc, (yyvsp[-6].sc_stmt), (yyvsp[-4].sc_expr), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3115 "parser.c"
    break;

  case 226: /* boolean_scalar_expression: expression  */
#line 1091 "parser.y"
                              {  (yyval.sc_expr) = CheckBooleanExpr(Cg->tokenLoc, (yyvsp[0].sc_expr), 0); }
#line 3121 "parser.c"
    break;

  case 228: /* for_expression_opt: %empty  */
#line 1096 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 3127 "parser.c"
    break;

  case 230: /* for_expression: for_expression ',' expression_statement2  */
#line 1101 "parser.y"
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
#line 3143 "parser.c"
    break;

  case 232: /* boolean_expression_opt: %empty  */
#line 1116 "parser.y"
                              { (yyval.sc_expr) = NULL; }
#line 3149 "parser.c"
    break;

  case 233: /* return_statement: RETURN_SY expression ';'  */
#line 1124 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, (yyvsp[-1].sc_expr)); }
#line 3155 "parser.c"
    break;

  case 234: /* return_statement: RETURN_SY ';'  */
#line 1126 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, NULL); }
#line 3161 "parser.c"
    break;

  case 240: /* identifier: IDENT_SY  */
#line 1149 "parser.y"
                              { (yyval.sc_ident) = (yyvsp[0].sc_ident); }
#line 3167 "parser.c"
    break;

  case 241: /* identifier: RESERVED_SY  */
#line 1151 "parser.y"
                              {
                                /* SemanticError, not SemanticParseError: the
                                 * latter is gated by AllowSemanticParseErrors */
                                SemanticError(Cg->tokenLoc, ERROR_S_RESERVED_WORD,
                                              GetAtomString(atable, (yyvsp[0].sc_token)));
                                (yyval.sc_ident) = (yyvsp[0].sc_token);
                              }
#line 3179 "parser.c"
    break;

  case 242: /* constant: INTCONST_SY  */
#line 1161 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(ICONST_OP, &(yyvsp[0].sc_literal)); }
#line 3185 "parser.c"
    break;

  case 243: /* constant: CFLOATCONST_SY  */
#line 1163 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3191 "parser.c"
    break;

  case 244: /* constant: FLOATCONST_SY  */
#line 1165 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3197 "parser.c"
    break;

  case 245: /* constant: FLOATHCONST_SY  */
#line 1167 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3203 "parser.c"
    break;

  case 246: /* constant: FLOATXCONST_SY  */
#line 1169 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3209 "parser.c"
    break;


#line 3213 "parser.c"

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

#line 1183 "parser.y"


