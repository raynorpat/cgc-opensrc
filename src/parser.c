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
  YYSYMBOL_ATTRIBARRAY_SY = 78,            /* ATTRIBARRAY_SY  */
  YYSYMBOL_FIRST_USER_TOKEN_SY = 79,       /* FIRST_USER_TOKEN_SY  */
  YYSYMBOL_80_ = 80,                       /* ';'  */
  YYSYMBOL_81_ = 81,                       /* ','  */
  YYSYMBOL_82_ = 82,                       /* '='  */
  YYSYMBOL_83_ = 83,                       /* '<'  */
  YYSYMBOL_84_ = 84,                       /* '>'  */
  YYSYMBOL_85_ = 85,                       /* '}'  */
  YYSYMBOL_86_ = 86,                       /* ':'  */
  YYSYMBOL_87_ = 87,                       /* '['  */
  YYSYMBOL_88_ = 88,                       /* ']'  */
  YYSYMBOL_89_ = 89,                       /* ')'  */
  YYSYMBOL_90_ = 90,                       /* '('  */
  YYSYMBOL_91_ = 91,                       /* '{'  */
  YYSYMBOL_92_ = 92,                       /* '.'  */
  YYSYMBOL_93_ = 93,                       /* '+'  */
  YYSYMBOL_94_ = 94,                       /* '-'  */
  YYSYMBOL_95_ = 95,                       /* '!'  */
  YYSYMBOL_96_ = 96,                       /* '~'  */
  YYSYMBOL_97_ = 97,                       /* '*'  */
  YYSYMBOL_98_ = 98,                       /* '/'  */
  YYSYMBOL_99_ = 99,                       /* '%'  */
  YYSYMBOL_100_ = 100,                     /* '&'  */
  YYSYMBOL_101_ = 101,                     /* '^'  */
  YYSYMBOL_102_ = 102,                     /* '|'  */
  YYSYMBOL_103_ = 103,                     /* '?'  */
  YYSYMBOL_YYACCEPT = 104,                 /* $accept  */
  YYSYMBOL_compilation_unit = 105,         /* compilation_unit  */
  YYSYMBOL_external_declaration = 106,     /* external_declaration  */
  YYSYMBOL_profile_specifier = 107,        /* profile_specifier  */
  YYSYMBOL_declaration = 108,              /* declaration  */
  YYSYMBOL_abstract_declaration = 109,     /* abstract_declaration  */
  YYSYMBOL_declaration_specifiers = 110,   /* declaration_specifiers  */
  YYSYMBOL_abstract_declaration_specifiers = 111, /* abstract_declaration_specifiers  */
  YYSYMBOL_abstract_declaration_specifiers2 = 112, /* abstract_declaration_specifiers2  */
  YYSYMBOL_init_declarator_list = 113,     /* init_declarator_list  */
  YYSYMBOL_init_declarator = 114,          /* init_declarator  */
  YYSYMBOL_type_specifier = 115,           /* type_specifier  */
  YYSYMBOL_type_qualifier = 116,           /* type_qualifier  */
  YYSYMBOL_type_domain = 117,              /* type_domain  */
  YYSYMBOL_storage_class = 118,            /* storage_class  */
  YYSYMBOL_function_specifier = 119,       /* function_specifier  */
  YYSYMBOL_geometry_modifier = 120,        /* geometry_modifier  */
  YYSYMBOL_in_out = 121,                   /* in_out  */
  YYSYMBOL_struct_or_connector_specifier = 122, /* struct_or_connector_specifier  */
  YYSYMBOL_struct_compound_header = 123,   /* struct_compound_header  */
  YYSYMBOL_struct_or_connector_header = 124, /* struct_or_connector_header  */
  YYSYMBOL_struct_identifier = 125,        /* struct_identifier  */
  YYSYMBOL_untagged_struct_header = 126,   /* untagged_struct_header  */
  YYSYMBOL_struct_declaration_list = 127,  /* struct_declaration_list  */
  YYSYMBOL_struct_declaration = 128,       /* struct_declaration  */
  YYSYMBOL_interface_specifier = 129,      /* interface_specifier  */
  YYSYMBOL_interface_compound_header = 130, /* interface_compound_header  */
  YYSYMBOL_interface_member_declaration_list = 131, /* interface_member_declaration_list  */
  YYSYMBOL_interface_member_declaration = 132, /* interface_member_declaration  */
  YYSYMBOL_annotation = 133,               /* annotation  */
  YYSYMBOL_134_1 = 134,                    /* $@1  */
  YYSYMBOL_annotation_decl_list = 135,     /* annotation_decl_list  */
  YYSYMBOL_declarator = 136,               /* declarator  */
  YYSYMBOL_semantic_declarator = 137,      /* semantic_declarator  */
  YYSYMBOL_basic_declarator = 138,         /* basic_declarator  */
  YYSYMBOL_function_decl_header = 139,     /* function_decl_header  */
  YYSYMBOL_abstract_declarator = 140,      /* abstract_declarator  */
  YYSYMBOL_parameter_list = 141,           /* parameter_list  */
  YYSYMBOL_parameter_declaration = 142,    /* parameter_declaration  */
  YYSYMBOL_abstract_parameter_list = 143,  /* abstract_parameter_list  */
  YYSYMBOL_non_empty_abstract_parameter_list = 144, /* non_empty_abstract_parameter_list  */
  YYSYMBOL_initializer = 145,              /* initializer  */
  YYSYMBOL_initializer_list = 146,         /* initializer_list  */
  YYSYMBOL_variable = 147,                 /* variable  */
  YYSYMBOL_basic_variable = 148,           /* basic_variable  */
  YYSYMBOL_primary_expression = 149,       /* primary_expression  */
  YYSYMBOL_postfix_expression = 150,       /* postfix_expression  */
  YYSYMBOL_actual_argument_list = 151,     /* actual_argument_list  */
  YYSYMBOL_non_empty_argument_list = 152,  /* non_empty_argument_list  */
  YYSYMBOL_actual_argument = 153,          /* actual_argument  */
  YYSYMBOL_expression_list = 154,          /* expression_list  */
  YYSYMBOL_unary_expression = 155,         /* unary_expression  */
  YYSYMBOL_cast_expression = 156,          /* cast_expression  */
  YYSYMBOL_multiplicative_expression = 157, /* multiplicative_expression  */
  YYSYMBOL_additive_expression = 158,      /* additive_expression  */
  YYSYMBOL_shift_expression = 159,         /* shift_expression  */
  YYSYMBOL_relational_expression = 160,    /* relational_expression  */
  YYSYMBOL_equality_expression = 161,      /* equality_expression  */
  YYSYMBOL_AND_expression = 162,           /* AND_expression  */
  YYSYMBOL_exclusive_OR_expression = 163,  /* exclusive_OR_expression  */
  YYSYMBOL_inclusive_OR_expression = 164,  /* inclusive_OR_expression  */
  YYSYMBOL_logical_AND_expression = 165,   /* logical_AND_expression  */
  YYSYMBOL_logical_OR_expression = 166,    /* logical_OR_expression  */
  YYSYMBOL_conditional_expression = 167,   /* conditional_expression  */
  YYSYMBOL_conditional_test = 168,         /* conditional_test  */
  YYSYMBOL_expression = 169,               /* expression  */
  YYSYMBOL_function_definition = 170,      /* function_definition  */
  YYSYMBOL_function_definition_header = 171, /* function_definition_header  */
  YYSYMBOL_statement = 172,                /* statement  */
  YYSYMBOL_balanced_statement = 173,       /* balanced_statement  */
  YYSYMBOL_dangling_statement = 174,       /* dangling_statement  */
  YYSYMBOL_discard_statement = 175,        /* discard_statement  */
  YYSYMBOL_jump_statement = 176,           /* jump_statement  */
  YYSYMBOL_if_statement = 177,             /* if_statement  */
  YYSYMBOL_dangling_if = 178,              /* dangling_if  */
  YYSYMBOL_if_header = 179,                /* if_header  */
  YYSYMBOL_compound_statement = 180,       /* compound_statement  */
  YYSYMBOL_compound_header = 181,          /* compound_header  */
  YYSYMBOL_compound_tail = 182,            /* compound_tail  */
  YYSYMBOL_block_item_list = 183,          /* block_item_list  */
  YYSYMBOL_block_item = 184,               /* block_item  */
  YYSYMBOL_expression_statement = 185,     /* expression_statement  */
  YYSYMBOL_expression_statement2 = 186,    /* expression_statement2  */
  YYSYMBOL_iteration_statement = 187,      /* iteration_statement  */
  YYSYMBOL_dangling_iteration = 188,       /* dangling_iteration  */
  YYSYMBOL_boolean_scalar_expression = 189, /* boolean_scalar_expression  */
  YYSYMBOL_for_expression_opt = 190,       /* for_expression_opt  */
  YYSYMBOL_for_expression = 191,           /* for_expression  */
  YYSYMBOL_boolean_expression_opt = 192,   /* boolean_expression_opt  */
  YYSYMBOL_return_statement = 193,         /* return_statement  */
  YYSYMBOL_member_identifier = 194,        /* member_identifier  */
  YYSYMBOL_scope_identifier = 195,         /* scope_identifier  */
  YYSYMBOL_semantics_identifier = 196,     /* semantics_identifier  */
  YYSYMBOL_type_identifier = 197,          /* type_identifier  */
  YYSYMBOL_variable_identifier = 198,      /* variable_identifier  */
  YYSYMBOL_identifier = 199,               /* identifier  */
  YYSYMBOL_constant = 200                  /* constant  */
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
#define YYFINAL  75
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2408

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  104
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  97
/* YYNRULES -- Number of rules.  */
#define YYNRULES  249
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  384

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   336


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
       2,     2,     2,    95,     2,     2,     2,    99,   100,     2,
      90,    89,    97,    93,    81,    94,    92,    98,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    86,    80,
      83,    82,    84,   103,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    87,     2,    88,   101,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    91,   102,    85,    96,     2,     2,     2,
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
      75,    76,    77,     2,    78,    79,     2
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   279,   279,   280,   287,   289,   291,   293,   303,   307,
     309,   311,   317,   324,   326,   331,   333,   335,   337,   339,
     341,   343,   348,   353,   355,   357,   359,   361,   363,   365,
     369,   371,   375,   377,   385,   387,   389,   391,   393,   395,
     397,   399,   401,   403,   405,   407,   409,   411,   413,   415,
     417,   419,   421,   423,   425,   438,   446,   448,   456,   458,
     466,   468,   482,   483,   484,   485,   486,   487,   488,   489,
     496,   498,   500,   509,   512,   515,   519,   524,   526,   528,
     532,   533,   536,   540,   541,   544,   546,   555,   564,   573,
     574,   578,   600,   600,   605,   606,   613,   615,   619,   621,
     625,   627,   629,   631,   633,   637,   642,   643,   645,   662,
     664,   668,   670,   675,   676,   679,   685,   698,   700,   702,
     704,   712,   714,   726,   728,   732,   740,   741,   742,   744,
     752,   753,   755,   757,   759,   761,   766,   767,   770,   772,
     782,   784,   788,   790,   798,   799,   801,   803,   805,   807,
     809,   817,   821,   829,   830,   832,   834,   842,   843,   845,
     853,   854,   856,   864,   865,   867,   869,   871,   879,   880,
     882,   890,   891,   899,   900,   908,   909,   917,   918,   926,
     927,   935,   936,   940,   948,   959,   962,   967,   975,   976,
     979,   980,   981,   982,   983,   984,   985,   988,   989,   996,
     998,  1006,  1008,  1016,  1020,  1022,  1026,  1034,  1036,  1040,
    1044,  1052,  1053,  1057,  1058,  1066,  1067,  1071,  1073,  1075,
    1077,  1079,  1081,  1083,  1091,  1093,  1095,  1099,  1101,  1106,
    1110,  1112,  1115,  1116,  1130,  1132,  1139,  1141,  1149,  1152,
    1155,  1158,  1161,  1164,  1166,  1176,  1178,  1180,  1182,  1184
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
  "ATTRIBARRAY_SY", "FIRST_USER_TOKEN_SY", "';'", "','", "'='", "'<'",
  "'>'", "'}'", "':'", "'['", "']'", "')'", "'('", "'{'", "'.'", "'+'",
  "'-'", "'!'", "'~'", "'*'", "'/'", "'%'", "'&'", "'^'", "'|'", "'?'",
  "$accept", "compilation_unit", "external_declaration",
  "profile_specifier", "declaration", "abstract_declaration",
  "declaration_specifiers", "abstract_declaration_specifiers",
  "abstract_declaration_specifiers2", "init_declarator_list",
  "init_declarator", "type_specifier", "type_qualifier", "type_domain",
  "storage_class", "function_specifier", "geometry_modifier", "in_out",
  "struct_or_connector_specifier", "struct_compound_header",
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
  "non_empty_argument_list", "actual_argument", "expression_list",
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

#define YYPACT_NINF (-317)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-240)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    2037,  -317,  -317,  -317,   -33,  -317,  -317,  -317,  -317,  -317,
    -317,  -317,  -317,  -317,  2271,  -317,    68,  -317,  2271,  -317,
    -317,  -317,  -317,  -317,  -317,  -317,  -317,    68,  -317,  -317,
      52,  -317,  -317,  -317,  -317,  -317,  -317,  -317,  -317,  -317,
     -34,  1959,  -317,  2115,  -317,    -6,  -317,   199,  -317,  2271,
    2271,  2271,  2271,  2271,  2271,  -317,   -32,   -32,  -317,  -317,
     343,  -317,  -317,  -317,  -317,     4,  -317,  -317,  -317,   -32,
    -317,  -317,  -317,  -317,  2330,  -317,  -317,  -317,  -317,  -317,
     -45,  -317,   -51,    25,    60,  1568,  -317,  -317,  -317,  -317,
    -317,  -317,  -317,  -317,  -317,  -317,  -317,  -317,  -317,  -317,
    2115,  -317,  2115,    43,  -317,    47,   910,   691,  -317,  -317,
    -317,    -9,    45,  -317,  1421,  1421,   983,    48,  -317,  -317,
     604,  1421,  1421,  1421,  1421,  -317,    -6,    51,  -317,  -317,
    -317,    87,  -317,  -317,    67,    55,     2,    -1,    14,    59,
      54,    65,   158,   -27,  -317,    73,  -317,  -317,  -317,  -317,
    -317,  -317,  -317,  -317,   691,  -317,   430,   517,  -317,  -317,
     111,  -317,  -317,  -317,   178,  -317,   182,  -317,    68,  2193,
    -317,   121,  -317,   -11,  1056,  -317,  -317,  -317,   -11,   -28,
    -317,  -317,   -11,    46,    -2,  -317,   120,   129,  -317,  1646,
    -317,  -317,  1724,  -317,  -317,  -317,    51,    38,   132,   155,
    1129,  1494,  1494,  -317,  -317,  -317,   136,  1494,   130,  -317,
     131,  -317,  -317,  -317,  -317,   139,  1494,  1494,  1494,  1494,
    1494,  1494,  -317,  -317,  1494,  1494,  1202,   -11,  1494,  1494,
    1494,  1494,  1494,  1494,  1494,  1494,  1494,  1494,  1494,  1494,
    1494,  1494,  1494,  1494,  1494,  1494,  1494,  -317,   197,  -317,
    -317,   430,  -317,  -317,  -317,   -11,  -317,  -317,  -317,   -11,
    1802,  -317,  -317,  -317,   764,  -317,  -317,  -317,  -317,   135,
    -317,   142,   138,  2193,  -317,  -317,  2271,  -317,  -317,  -317,
    -317,   140,  -317,   149,   151,  -317,   147,  -317,   150,  1494,
    -317,    31,  -317,  -317,  -317,  -317,  -317,  -317,  -317,   152,
     153,   156,  -317,   157,  -317,  -317,  -317,  -317,  -317,    67,
      67,    55,    55,     2,     2,     2,     2,    -1,    -1,    14,
      59,    54,    65,   158,   161,   691,  -317,  -317,  -317,   169,
    -317,  -317,  -317,  -317,   -19,  1880,  -317,  1056,   -24,  -317,
    -317,  1494,  1275,  1494,  -317,   691,  -317,  1494,  -317,  -317,
    -317,  1494,   -11,  1494,  -317,  -317,  -317,   837,  -317,  -317,
    -317,  -317,   162,  -317,   164,  -317,   171,  -317,  -317,  -317,
    -317,  -317,  -317,  -317,  -317,  -317,  -317,   174,  1348,  -317,
     175,   691,  -317,  -317
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    54,    37,    55,     0,    59,    35,   243,    70,    60,
      72,    34,    61,    71,     0,    58,    82,    38,     0,   241,
      56,    57,    36,    39,    44,    43,    42,     0,    41,    40,
      45,   244,    62,    63,    64,    65,    66,    67,    68,    69,
       0,     0,     2,     0,     4,     0,    13,    15,    23,     0,
       0,     0,     0,     0,     0,    50,    75,     0,    51,     5,
       0,    53,     8,    11,    22,    77,    81,    80,    14,     0,
      48,    46,    49,    47,     0,     1,     3,     7,     6,     9,
       0,    30,    32,    96,    98,     0,   100,    29,    24,    26,
      25,    28,    27,    16,    18,    17,    20,    21,    19,   209,
       0,    76,     0,     0,   246,     0,     0,     0,   247,   248,
     249,     0,     0,   245,     0,     0,     0,     0,   216,   186,
       0,     0,     0,     0,     0,   213,     0,    23,   126,   123,
     130,   144,   151,   153,   157,   160,   163,   168,   171,   173,
     175,   177,   179,   181,   184,     0,   218,   214,   188,   189,
     191,   195,   194,   197,     0,   190,     0,     0,   211,   192,
       0,   193,   198,   196,     0,   125,   242,   127,     0,     0,
      88,     0,    10,     0,     0,   187,    92,    97,     0,     0,
     105,   115,     0,   106,     0,   109,     0,   114,    85,     0,
      83,    86,     0,   201,   202,   199,     0,   144,     0,     0,
       0,     0,     0,   146,   145,   237,     0,     0,     0,   106,
       0,   147,   148,   149,   150,    32,     0,     0,     0,     0,
       0,     0,   132,   131,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   204,   188,   210,
     208,     0,   185,   212,   215,     0,    78,    79,   240,     0,
       0,    89,    52,    31,     0,    33,   117,    94,    99,     0,
     102,   111,    12,     0,   103,   104,     0,    73,    84,    74,
     200,     0,   232,     0,   230,   229,     0,   236,     0,     0,
     128,     0,   142,   219,   220,   221,   222,   223,   217,     0,
       0,   137,   138,   140,   133,   238,   154,   155,   156,   158,
     159,   162,   161,   167,   166,   164,   165,   169,   170,   172,
     174,   176,   178,   180,     0,     0,   207,   124,   242,     0,
      87,    90,   120,   121,     0,     0,   101,     0,     0,   110,
     116,     0,     0,     0,   206,     0,   152,     0,   129,   134,
     135,     0,     0,     0,   203,   205,    91,     0,   118,    93,
      95,   112,     0,   108,     0,   234,     0,   233,   224,   227,
     143,   139,   141,   182,   119,   122,   107,     0,     0,   225,
       0,     0,   226,   228
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -317,  -317,   200,  -317,     5,  -114,   -47,    19,  -317,  -317,
      85,     1,   213,   218,   219,   220,  -317,   221,  -317,   216,
    -317,   248,  -317,   176,   -56,  -317,  -317,  -317,    16,  -317,
    -317,  -317,   -42,  -317,  -317,  -317,  -317,  -317,     6,  -317,
    -317,  -246,  -317,  -317,    26,  -317,   -40,  -317,  -317,   -71,
    -317,   148,  -205,   -60,   -48,   -84,   -52,    41,    42,    40,
      49,    44,  -317,   -67,  -317,   -18,    56,  -317,   -77,  -150,
    -316,  -317,  -317,  -317,  -317,  -317,  -317,    75,    36,   134,
    -145,  -317,  -198,  -317,  -317,  -199,   -87,  -317,  -317,  -317,
    -317,  -317,  -171,    -5,  -317,     0,  -317
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    41,    42,    43,   125,   181,    45,    46,    47,    80,
      81,   196,    49,    50,    51,    52,    53,    54,    55,   100,
      56,    65,    57,   189,   190,    58,   169,   260,   261,   177,
     267,   335,   215,    83,    84,    85,   272,   184,   185,   186,
     187,   265,   334,   128,   129,   130,   197,   300,   301,   302,
     291,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   191,    60,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   250,   157,
     158,   159,   160,   161,   162,   286,   283,   284,   366,   163,
     304,   164,   256,    61,   165,   166,   167
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      62,    48,   282,    82,   248,    44,   208,   268,   288,   355,
     269,    66,   253,   126,   362,    48,    67,   245,   333,    48,
     131,     7,    66,   306,   307,   308,     7,    67,   235,   369,
     199,   174,   233,    64,   239,   172,   173,    68,   182,   236,
     175,    62,    48,   234,    48,    86,    44,    63,    77,    74,
      48,    48,    48,    48,    48,    48,    59,   240,    31,    99,
     270,   127,   357,    31,   363,   383,   358,   131,    93,    94,
      95,    96,    97,    98,    79,   171,  -183,   247,   -13,   273,
     222,   200,   237,   238,   346,   223,    48,   274,   198,    70,
     168,   361,   217,   218,   219,   220,   221,    59,   206,    78,
       7,    48,   210,    48,   183,   188,   253,   188,   176,   126,
     126,   375,   347,    71,   131,   -13,   131,   131,    72,    73,
     348,   127,   259,   193,    19,   225,    86,   194,   226,   222,
     227,   101,   101,   278,   223,   201,   278,    31,   207,   209,
     271,   216,   364,   365,   170,   367,   178,   179,   231,   232,
     180,   313,   314,   315,   316,   242,   266,   127,   127,   241,
     131,   244,   340,   257,   228,   229,   230,   243,   258,   224,
      48,   309,   310,    86,   225,   354,   246,   226,   258,   227,
     282,   372,    86,   285,   210,   311,   312,   317,   318,   285,
      48,   254,   255,    48,   188,   368,  -239,   188,   292,   293,
     294,   295,   296,   297,   126,   262,   298,   299,   303,   275,
     276,   131,   280,   259,     3,   281,   287,   329,   325,   289,
     290,   174,     5,   336,   337,   338,   182,   305,   324,   342,
     341,   382,   343,     8,     9,    10,   344,   351,    12,   345,
     349,    76,   350,   352,    13,    87,   266,   353,    15,   356,
     376,   378,   127,   377,   379,   328,    20,    21,   263,    86,
      88,    48,   203,   204,   381,    89,    90,    91,    92,   211,
     212,   213,   214,   102,    48,    69,   331,    48,   192,   339,
     371,   327,   319,   321,   320,   131,   373,   326,   126,   323,
     251,   380,     0,   322,     0,   209,     0,     0,     0,     0,
       0,     0,     0,   131,     0,   131,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   266,
       0,     0,     0,   285,   285,     0,     0,     0,     0,   370,
       0,     0,     0,   303,     0,     0,    48,     0,   131,   266,
     360,   131,     0,     0,     1,     0,     0,     0,     0,     0,
       0,     0,   258,     2,   103,     0,   104,     0,     3,   105,
       0,   106,   107,     0,     0,     4,     5,     6,   108,   109,
     110,   111,     0,     0,     0,     7,   112,     8,     9,    10,
      11,   113,    12,     0,     0,   114,     0,     0,    13,    14,
     115,   116,    15,     0,    16,     0,    17,     0,    18,    19,
      20,    21,    22,   117,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,     0,   118,     0,     0,     0,     0,   119,     0,
       0,     1,     0,   120,    99,     0,   121,   122,   123,   124,
       2,   103,     0,   104,     0,     3,   105,     0,   106,   107,
       0,     0,     4,     5,     6,   108,   109,   110,   111,     0,
       0,     0,     7,   112,     8,     9,    10,    11,   113,    12,
       0,     0,   114,     0,     0,    13,    14,   115,   116,    15,
       0,    16,     0,    17,     0,    18,    19,    20,    21,    22,
     117,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,     0,
     118,     0,     0,     0,     0,   249,     0,     0,     1,     0,
     120,    99,     0,   121,   122,   123,   124,     2,   103,     0,
     104,     0,     3,   105,     0,   106,   107,     0,     0,     4,
       5,     6,   108,   109,   110,   111,     0,     0,     0,     7,
     112,     8,     9,    10,    11,   113,    12,     0,     0,   114,
       0,     0,    13,    14,   115,   116,    15,     0,    16,     0,
      17,     0,    18,    19,    20,    21,    22,   117,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,     0,   118,     0,     0,
       0,     0,   252,     0,     0,     1,     0,   120,    99,     0,
     121,   122,   123,   124,     2,     0,     0,   104,     0,     3,
       0,     0,     0,     0,     0,     0,     0,     5,     6,   108,
     109,   110,     0,     0,     0,     0,     7,     0,     8,     9,
      10,    11,   113,    12,     0,     0,   114,     0,     0,    13,
      14,   115,     0,    15,     0,    16,     0,    17,     0,     0,
      19,    20,    21,    22,     0,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     1,     0,   120,     0,     0,   121,   122,   123,
     124,     2,   103,     0,   104,     0,     0,   105,     0,   106,
     107,     0,     0,     0,     0,     6,   108,   109,   110,   111,
       0,     0,     0,     7,   112,     0,     0,     0,    11,   113,
       0,     0,     0,   114,     0,     0,     0,     0,   115,   116,
       0,     0,    16,     0,    17,     0,     0,    19,     0,     0,
      22,   117,    23,    24,    25,    26,    27,    28,    29,    30,
      31,     0,     0,     0,     0,     1,     0,     0,     0,    40,
       0,   118,     0,     0,     2,     0,     0,   104,     0,     0,
       0,   120,    99,     0,   121,   122,   123,   124,     6,   108,
     109,   110,     0,     0,     0,     0,     7,     0,     0,     0,
       0,    11,   113,     0,     0,     0,   114,     0,     0,     0,
       0,   115,     0,     0,     0,    16,     0,    17,     0,     0,
      19,     0,     0,    22,     0,    23,    24,    25,    26,    27,
      28,    29,    30,    31,     0,     0,     0,     0,     1,     0,
       0,     0,    40,     0,     0,     0,     0,     2,     0,   332,
     104,     0,     0,     0,   120,   264,     0,   121,   122,   123,
     124,     6,   108,   109,   110,     0,     0,     0,     0,     7,
       0,     0,     0,     0,    11,   113,     0,     0,     0,   114,
       0,     0,     0,     0,   115,     0,     0,     0,    16,     0,
      17,     0,     0,    19,     0,     0,    22,     0,    23,    24,
      25,    26,    27,    28,    29,    30,    31,     0,     0,     0,
       0,     1,     0,     0,     0,    40,     0,     0,     0,     0,
       2,     0,   374,   104,     0,     0,     0,   120,   264,     0,
     121,   122,   123,   124,     6,   108,   109,   110,     0,     0,
       0,     0,     7,     0,     0,     0,     0,    11,   113,     0,
       0,     0,   114,     0,     0,     0,     0,   115,     0,     0,
       0,    16,     0,    17,     0,     0,    19,     0,     0,    22,
       0,    23,    24,    25,    26,    27,    28,    29,    30,    31,
       0,     0,     0,     0,     1,     0,     0,     0,    40,     0,
     195,     0,     0,     2,     0,     0,   104,     0,     0,     0,
     120,     0,     0,   121,   122,   123,   124,     6,   108,   109,
     110,     0,     0,     0,     0,     7,     0,     0,     0,     0,
      11,   113,     0,     0,     0,   114,     0,     0,     0,     0,
     115,     0,     0,     0,    16,     0,    17,     0,     0,    19,
       0,     0,    22,     0,    23,    24,    25,    26,    27,    28,
      29,    30,    31,     0,     0,     0,     0,     1,     0,     0,
       0,    40,     0,   205,     0,     0,     2,     0,     0,   104,
       0,     0,     0,   120,     0,     0,   121,   122,   123,   124,
       6,   108,   109,   110,     0,     0,     0,     0,     7,     0,
       0,     0,     0,    11,   113,     0,     0,     0,   114,     0,
       0,     0,     0,   115,     0,     0,     0,    16,     0,    17,
       0,     0,    19,     0,     0,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,     0,     0,     0,     0,
       1,     0,     0,     0,    40,     0,     0,     0,     0,     2,
       0,     0,   104,     0,     0,     0,   120,   264,     0,   121,
     122,   123,   124,     6,   108,   109,   110,     0,     0,     0,
       0,     7,     0,     0,     0,     0,    11,   113,     0,     0,
       0,   114,     0,     0,     0,     0,   115,     0,     0,     0,
      16,     0,    17,     0,     0,    19,     0,     0,    22,     0,
      23,    24,    25,    26,    27,    28,    29,    30,    31,     0,
       0,     0,     0,     1,     0,     0,     0,    40,     0,  -231,
       0,     0,     2,     0,     0,   104,     0,     0,     0,   120,
       0,     0,   121,   122,   123,   124,     6,   108,   109,   110,
       0,     0,     0,     0,     7,     0,     0,     0,     0,    11,
     113,     0,     0,     0,   114,     0,     0,     0,     0,   115,
       0,     0,     0,    16,     0,    17,     0,     0,    19,     0,
       0,    22,     0,    23,    24,    25,    26,    27,    28,    29,
      30,    31,     0,     0,     0,     0,     1,     0,     0,     0,
      40,     0,     0,     0,     0,     2,     0,     0,   104,     0,
       0,  -136,   120,     0,     0,   121,   122,   123,   124,     6,
     108,   109,   110,     0,     0,     0,     0,     7,     0,     0,
       0,     0,    11,   113,     0,     0,     0,   114,     0,     0,
       0,     0,   115,     0,     0,     0,    16,     0,    17,     0,
       0,    19,     0,     0,    22,     0,    23,    24,    25,    26,
      27,    28,    29,    30,    31,     0,     0,     0,     0,     1,
       0,     0,     0,    40,     0,  -235,     0,     0,     2,     0,
       0,   104,     0,     0,     0,   120,     0,     0,   121,   122,
     123,   124,     6,   108,   109,   110,     0,     0,     0,     0,
       7,     0,     0,     0,     0,    11,   113,     0,     0,     0,
     114,     0,     0,     0,     0,   115,     0,     0,     0,    16,
       0,    17,     0,     0,    19,     0,     0,    22,     0,    23,
      24,    25,    26,    27,    28,    29,    30,    31,     0,     0,
       0,     0,     1,     0,     0,     0,    40,     0,     0,     0,
       0,     2,     0,     0,   104,     0,     0,  -231,   120,     0,
       0,   121,   122,   123,   124,     6,   108,   109,   110,     0,
       0,     0,     0,     7,     0,     0,     0,     0,    11,   113,
       0,     0,     0,   114,     0,     0,     0,     0,   115,     0,
       0,     0,    16,     0,    17,     0,     0,    19,     0,     0,
      22,     0,    23,    24,    25,    26,    27,    28,    29,    30,
      31,     0,     0,     0,     0,     1,     0,     0,     0,    40,
       0,     0,     0,     0,     2,     0,     0,   104,     0,     0,
       0,   202,     0,     0,   121,   122,   123,   124,     6,   108,
     109,   110,     0,     0,     0,     0,     7,     0,     0,     0,
       0,    11,   113,     0,     0,     0,   114,     0,     0,     0,
       0,   115,     0,     0,     0,    16,     0,    17,     0,     0,
      19,     0,     0,    22,     0,    23,    24,    25,    26,    27,
      28,    29,    30,    31,     0,     0,     0,     0,     0,     1,
       0,     0,    40,     0,     0,     0,     0,     0,     2,     0,
       0,     0,     0,     3,   120,     0,     0,   121,   122,   123,
     124,     5,     6,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     8,     9,    10,    11,     0,    12,     0,     0,
       0,     0,     0,    13,    14,     0,     0,    15,     0,    16,
       0,    17,     0,    18,    19,    20,    21,    22,     0,    23,
      24,    25,    26,    27,    28,    29,    30,     0,    32,    33,
      34,    35,    36,    37,    38,    39,    40,     1,     0,     0,
       0,     0,     0,     0,     0,     0,     2,  -113,     0,     0,
       0,     3,     0,     0,     0,     0,     0,     0,     4,     5,
       6,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       8,     9,    10,    11,     0,    12,     0,     0,     0,     0,
       0,    13,    14,     0,     0,    15,     0,    16,     0,    17,
       0,    18,    19,    20,    21,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,     0,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     1,     0,     0,     0,     0,
       0,   277,     0,     0,     2,     0,     0,     0,     0,     3,
       0,     0,     0,     0,     0,     0,     4,     5,     6,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     8,     9,
      10,    11,     0,    12,     0,     0,     0,     0,     0,    13,
      14,     0,     0,    15,     0,    16,     0,    17,     0,    18,
      19,    20,    21,    22,     0,    23,    24,    25,    26,    27,
      28,    29,    30,     0,    32,    33,    34,    35,    36,    37,
      38,    39,    40,     1,     0,     0,     0,     0,     0,   279,
       0,     0,     2,     0,     0,     0,     0,     3,     0,     0,
       0,     0,     0,     0,     0,     5,     6,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
       0,    12,     0,     0,     0,     0,     0,    13,    14,     0,
       0,    15,     0,    16,     0,    17,     0,    18,    19,    20,
      21,    22,     0,    23,    24,    25,    26,    27,    28,    29,
      30,     0,    32,    33,    34,    35,    36,    37,    38,    39,
      40,     1,     0,     0,     0,     0,     0,   330,     0,     0,
       2,     0,     0,     0,     0,     3,     0,     0,     0,     0,
       0,     0,     4,     5,     6,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     8,     9,    10,    11,     0,    12,
       0,     0,     0,     0,     0,    13,    14,     0,     0,    15,
       0,    16,     0,    17,     0,    18,    19,    20,    21,    22,
       0,    23,    24,    25,    26,    27,    28,    29,    30,     0,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    75,
       1,     0,     0,     0,   359,     0,     0,     0,     0,     2,
       0,     0,     0,     0,     3,     0,     0,     0,     0,     0,
       0,     4,     5,     6,     0,     0,     0,     0,     0,     0,
       0,     7,     0,     8,     9,    10,    11,     0,    12,     0,
       0,     0,     0,     0,    13,    14,     0,     0,    15,     0,
      16,     0,    17,     0,    18,    19,    20,    21,    22,     0,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,     1,     0,
       0,     0,     0,     0,     0,     0,     0,     2,     0,     0,
       0,     0,     3,     0,     0,     0,     0,     0,     0,     4,
       5,     6,     0,     0,     0,     0,     0,     0,     0,     7,
       0,     8,     9,    10,    11,     0,    12,     0,     0,     0,
       0,     0,    13,    14,     0,     0,    15,     0,    16,     0,
      17,     0,    18,    19,    20,    21,    22,     0,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,     1,     0,     0,     0,
       0,     0,     0,     0,     0,     2,     0,     0,     0,     0,
       3,     0,     0,     0,     0,     0,     0,     4,     5,     6,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     8,
       9,    10,    11,     0,    12,     0,     0,     0,     0,     0,
      13,    14,     0,     0,    15,     0,    16,     0,    17,     0,
      18,    19,    20,    21,    22,     0,    23,    24,    25,    26,
      27,    28,    29,    30,     0,    32,    33,    34,    35,    36,
      37,    38,    39,    40,     1,     0,     0,     0,     0,     0,
       0,     0,     0,     2,     0,     0,     0,     0,     3,     0,
       0,     0,     0,     0,     0,     0,     5,     6,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,     0,    12,     0,     0,     0,     0,     0,    13,    14,
       0,     0,    15,     0,    16,     0,    17,     0,    18,    19,
      20,    21,    22,     0,    23,    24,    25,    26,    27,    28,
      29,    30,     0,    32,    33,    34,    35,    36,    37,    38,
      39,    40,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     2,     0,     0,     0,     0,     3,     0,     0,     0,
       0,     0,     0,     0,     5,     6,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,     0,
      12,     0,     0,     0,     0,     0,    13,    14,     0,     0,
      15,     0,    16,     0,    17,     0,     0,    19,    20,    21,
      22,     1,    23,    24,    25,    26,    27,    28,    29,    30,
       2,    32,    33,    34,    35,    36,    37,    38,    39,    40,
       0,     0,     0,     0,     6,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    11,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    16,     0,    17,     0,     0,    19,     0,     0,    22,
       0,    23,    24,    25,    26,    27,    28,    29,    30,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    40
};

static const yytype_int16 yycheck[] =
{
       0,     0,   200,    45,   154,     0,   120,   178,   207,   325,
      38,    16,   157,    60,    38,    14,    16,    44,   264,    18,
      60,    32,    27,   228,   229,   230,    32,    27,    29,   345,
     107,    82,    30,    14,    20,    80,    81,    18,    85,    40,
      91,    41,    41,    41,    43,    45,    41,    80,    43,    83,
      49,    50,    51,    52,    53,    54,     0,    43,    69,    91,
      88,    60,    81,    69,    88,   381,    85,   107,    49,    50,
      51,    52,    53,    54,    80,    74,   103,   154,    32,    81,
      42,    90,    83,    84,   289,    47,    85,    89,   106,    37,
      86,   337,     5,     6,     7,     8,     9,    41,   116,    43,
      32,   100,   120,   102,    85,   100,   251,   102,    83,   156,
     157,   357,    81,    61,   154,    69,   156,   157,    66,    67,
      89,   120,   169,    80,    56,    87,   126,    80,    90,    42,
      92,    56,    57,   189,    47,    90,   192,    69,    90,   120,
     182,    90,   341,   342,    69,   343,    86,    87,    93,    94,
      90,   235,   236,   237,   238,   101,   174,   156,   157,   100,
     200,     3,   276,   168,    97,    98,    99,   102,   168,    82,
     169,   231,   232,   173,    87,   325,   103,    90,   178,    92,
     378,   352,   182,   201,   202,   233,   234,   239,   240,   207,
     189,    80,    14,   192,   189,   345,    14,   192,   216,   217,
     218,   219,   220,   221,   251,    84,   224,   225,   226,    89,
      81,   251,    80,   260,    15,    60,    80,   259,    21,    89,
      89,    82,    23,    88,    82,    87,   273,   227,   246,    80,
      90,   381,    81,    34,    35,    36,    89,    81,    39,    89,
      88,    41,    89,    86,    45,    46,   264,    86,    49,    80,
      88,    80,   251,    89,    80,   255,    57,    58,   173,   259,
      47,   260,   114,   115,    89,    47,    47,    47,    47,   121,
     122,   123,   124,    57,   273,    27,   260,   276,   102,   273,
     351,   255,   241,   243,   242,   325,   353,   251,   335,   245,
     156,   378,    -1,   244,    -1,   276,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   343,    -1,   345,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   337,
      -1,    -1,    -1,   341,   342,    -1,    -1,    -1,    -1,   347,
      -1,    -1,    -1,   351,    -1,    -1,   335,    -1,   378,   357,
     335,   381,    -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   352,    10,    11,    -1,    13,    -1,    15,    16,
      -1,    18,    19,    -1,    -1,    22,    23,    24,    25,    26,
      27,    28,    -1,    -1,    -1,    32,    33,    34,    35,    36,
      37,    38,    39,    -1,    -1,    42,    -1,    -1,    45,    46,
      47,    48,    49,    -1,    51,    -1,    53,    -1,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    -1,    80,    -1,    -1,    -1,    -1,    85,    -1,
      -1,     1,    -1,    90,    91,    -1,    93,    94,    95,    96,
      10,    11,    -1,    13,    -1,    15,    16,    -1,    18,    19,
      -1,    -1,    22,    23,    24,    25,    26,    27,    28,    -1,
      -1,    -1,    32,    33,    34,    35,    36,    37,    38,    39,
      -1,    -1,    42,    -1,    -1,    45,    46,    47,    48,    49,
      -1,    51,    -1,    53,    -1,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    -1,
      80,    -1,    -1,    -1,    -1,    85,    -1,    -1,     1,    -1,
      90,    91,    -1,    93,    94,    95,    96,    10,    11,    -1,
      13,    -1,    15,    16,    -1,    18,    19,    -1,    -1,    22,
      23,    24,    25,    26,    27,    28,    -1,    -1,    -1,    32,
      33,    34,    35,    36,    37,    38,    39,    -1,    -1,    42,
      -1,    -1,    45,    46,    47,    48,    49,    -1,    51,    -1,
      53,    -1,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    -1,    80,    -1,    -1,
      -1,    -1,    85,    -1,    -1,     1,    -1,    90,    91,    -1,
      93,    94,    95,    96,    10,    -1,    -1,    13,    -1,    15,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    24,    25,
      26,    27,    -1,    -1,    -1,    -1,    32,    -1,    34,    35,
      36,    37,    38,    39,    -1,    -1,    42,    -1,    -1,    45,
      46,    47,    -1,    49,    -1,    51,    -1,    53,    -1,    -1,
      56,    57,    58,    59,    -1,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     1,    -1,    90,    -1,    -1,    93,    94,    95,
      96,    10,    11,    -1,    13,    -1,    -1,    16,    -1,    18,
      19,    -1,    -1,    -1,    -1,    24,    25,    26,    27,    28,
      -1,    -1,    -1,    32,    33,    -1,    -1,    -1,    37,    38,
      -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    48,
      -1,    -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    78,
      -1,    80,    -1,    -1,    10,    -1,    -1,    13,    -1,    -1,
      -1,    90,    91,    -1,    93,    94,    95,    96,    24,    25,
      26,    27,    -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    37,    38,    -1,    -1,    -1,    42,    -1,    -1,    -1,
      -1,    47,    -1,    -1,    -1,    51,    -1,    53,    -1,    -1,
      56,    -1,    -1,    59,    -1,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,    78,    -1,    -1,    -1,    -1,    10,    -1,    85,
      13,    -1,    -1,    -1,    90,    91,    -1,    93,    94,    95,
      96,    24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,
      -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,
      -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,
      53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    -1,    -1,    -1,
      -1,     1,    -1,    -1,    -1,    78,    -1,    -1,    -1,    -1,
      10,    -1,    85,    13,    -1,    -1,    -1,    90,    91,    -1,
      93,    94,    95,    96,    24,    25,    26,    27,    -1,    -1,
      -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    38,    -1,
      -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    -1,    -1,
      -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,    59,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    78,    -1,
      80,    -1,    -1,    10,    -1,    -1,    13,    -1,    -1,    -1,
      90,    -1,    -1,    93,    94,    95,    96,    24,    25,    26,
      27,    -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,
      37,    38,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,
      47,    -1,    -1,    -1,    51,    -1,    53,    -1,    -1,    56,
      -1,    -1,    59,    -1,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    -1,    -1,    -1,    -1,     1,    -1,    -1,
      -1,    78,    -1,    80,    -1,    -1,    10,    -1,    -1,    13,
      -1,    -1,    -1,    90,    -1,    -1,    93,    94,    95,    96,
      24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,    -1,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,    53,
      -1,    -1,    56,    -1,    -1,    59,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    78,    -1,    -1,    -1,    -1,    10,
      -1,    -1,    13,    -1,    -1,    -1,    90,    91,    -1,    93,
      94,    95,    96,    24,    25,    26,    27,    -1,    -1,    -1,
      -1,    32,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,
      -1,    42,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,
      51,    -1,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    -1,
      -1,    -1,    -1,     1,    -1,    -1,    -1,    78,    -1,    80,
      -1,    -1,    10,    -1,    -1,    13,    -1,    -1,    -1,    90,
      -1,    -1,    93,    94,    95,    96,    24,    25,    26,    27,
      -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,
      38,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    47,
      -1,    -1,    -1,    51,    -1,    53,    -1,    -1,    56,    -1,
      -1,    59,    -1,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,
      78,    -1,    -1,    -1,    -1,    10,    -1,    -1,    13,    -1,
      -1,    89,    90,    -1,    -1,    93,    94,    95,    96,    24,
      25,    26,    27,    -1,    -1,    -1,    -1,    32,    -1,    -1,
      -1,    -1,    37,    38,    -1,    -1,    -1,    42,    -1,    -1,
      -1,    -1,    47,    -1,    -1,    -1,    51,    -1,    53,    -1,
      -1,    56,    -1,    -1,    59,    -1,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    -1,    -1,    -1,    -1,     1,
      -1,    -1,    -1,    78,    -1,    80,    -1,    -1,    10,    -1,
      -1,    13,    -1,    -1,    -1,    90,    -1,    -1,    93,    94,
      95,    96,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,
      42,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,
      -1,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    -1,    -1,
      -1,    -1,     1,    -1,    -1,    -1,    78,    -1,    -1,    -1,
      -1,    10,    -1,    -1,    13,    -1,    -1,    89,    90,    -1,
      -1,    93,    94,    95,    96,    24,    25,    26,    27,    -1,
      -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    38,
      -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,
      59,    -1,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    78,
      -1,    -1,    -1,    -1,    10,    -1,    -1,    13,    -1,    -1,
      -1,    90,    -1,    -1,    93,    94,    95,    96,    24,    25,
      26,    27,    -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    37,    38,    -1,    -1,    -1,    42,    -1,    -1,    -1,
      -1,    47,    -1,    -1,    -1,    51,    -1,    53,    -1,    -1,
      56,    -1,    -1,    59,    -1,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    -1,    -1,    -1,    -1,    -1,     1,
      -1,    -1,    78,    -1,    -1,    -1,    -1,    -1,    10,    -1,
      -1,    -1,    -1,    15,    90,    -1,    -1,    93,    94,    95,
      96,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    34,    35,    36,    37,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    46,    -1,    -1,    49,    -1,    51,
      -1,    53,    -1,    55,    56,    57,    58,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,    -1,    70,    71,
      72,    73,    74,    75,    76,    77,    78,     1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    10,    89,    -1,    -1,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      34,    35,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    46,    -1,    -1,    49,    -1,    51,    -1,    53,
      -1,    55,    56,    57,    58,    59,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,    -1,    70,    71,    72,    73,
      74,    75,    76,    77,    78,     1,    -1,    -1,    -1,    -1,
      -1,    85,    -1,    -1,    10,    -1,    -1,    -1,    -1,    15,
      -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    24,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    35,
      36,    37,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      46,    -1,    -1,    49,    -1,    51,    -1,    53,    -1,    55,
      56,    57,    58,    59,    -1,    61,    62,    63,    64,    65,
      66,    67,    68,    -1,    70,    71,    72,    73,    74,    75,
      76,    77,    78,     1,    -1,    -1,    -1,    -1,    -1,    85,
      -1,    -1,    10,    -1,    -1,    -1,    -1,    15,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    23,    24,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,    37,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,    -1,
      -1,    49,    -1,    51,    -1,    53,    -1,    55,    56,    57,
      58,    59,    -1,    61,    62,    63,    64,    65,    66,    67,
      68,    -1,    70,    71,    72,    73,    74,    75,    76,    77,
      78,     1,    -1,    -1,    -1,    -1,    -1,    85,    -1,    -1,
      10,    -1,    -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,
      -1,    -1,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    34,    35,    36,    37,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    46,    -1,    -1,    49,
      -1,    51,    -1,    53,    -1,    55,    56,    57,    58,    59,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,    -1,
      70,    71,    72,    73,    74,    75,    76,    77,    78,     0,
       1,    -1,    -1,    -1,    84,    -1,    -1,    -1,    -1,    10,
      -1,    -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,
      -1,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    32,    -1,    34,    35,    36,    37,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    46,    -1,    -1,    49,    -1,
      51,    -1,    53,    -1,    55,    56,    57,    58,    59,    -1,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    10,    -1,    -1,
      -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,
      23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      -1,    34,    35,    36,    37,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    46,    -1,    -1,    49,    -1,    51,    -1,
      53,    -1,    55,    56,    57,    58,    59,    -1,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,     1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    10,    -1,    -1,    -1,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    24,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,
      35,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    46,    -1,    -1,    49,    -1,    51,    -1,    53,    -1,
      55,    56,    57,    58,    59,    -1,    61,    62,    63,    64,
      65,    66,    67,    68,    -1,    70,    71,    72,    73,    74,
      75,    76,    77,    78,     1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    10,    -1,    -1,    -1,    -1,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    23,    24,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,
      37,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,
      -1,    -1,    49,    -1,    51,    -1,    53,    -1,    55,    56,
      57,    58,    59,    -1,    61,    62,    63,    64,    65,    66,
      67,    68,    -1,    70,    71,    72,    73,    74,    75,    76,
      77,    78,     1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    10,    -1,    -1,    -1,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    23,    24,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    34,    35,    36,    37,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    46,    -1,    -1,
      49,    -1,    51,    -1,    53,    -1,    -1,    56,    57,    58,
      59,     1,    61,    62,    63,    64,    65,    66,    67,    68,
      10,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      -1,    -1,    -1,    -1,    24,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,    59,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    78
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    10,    15,    22,    23,    24,    32,    34,    35,
      36,    37,    39,    45,    46,    49,    51,    53,    55,    56,
      57,    58,    59,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,   105,   106,   107,   108,   110,   111,   112,   115,   116,
     117,   118,   119,   120,   121,   122,   124,   126,   129,   170,
     171,   197,   199,    80,   111,   125,   197,   199,   111,   125,
      37,    61,    66,    67,    83,     0,   106,   108,   170,    80,
     113,   114,   136,   137,   138,   139,   199,    46,   116,   117,
     118,   119,   121,   111,   111,   111,   111,   111,   111,    91,
     123,   181,   123,    11,    13,    16,    18,    19,    25,    26,
      27,    28,    33,    38,    42,    47,    48,    60,    80,    85,
      90,    93,    94,    95,    96,   108,   110,   115,   147,   148,
     149,   150,   155,   156,   157,   158,   159,   160,   161,   162,
     163,   164,   165,   166,   167,   168,   169,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   183,   184,   185,
     186,   187,   188,   193,   195,   198,   199,   200,    86,   130,
     181,   115,    80,    81,    82,    91,    83,   133,    86,    87,
      90,   109,   110,   111,   141,   142,   143,   144,   108,   127,
     128,   170,   127,    80,    80,    80,   115,   150,   169,   172,
      90,    90,    90,   155,   155,    80,   169,    90,   109,   111,
     169,   155,   155,   155,   155,   136,    90,     5,     6,     7,
       8,     9,    42,    47,    82,    87,    90,    92,    97,    98,
      99,    93,    94,    30,    41,    29,    40,    83,    84,    20,
      43,   100,   101,   102,     3,    44,   103,   172,   173,    85,
     182,   183,    85,   184,    80,    14,   196,   197,   199,   110,
     131,   132,    84,   114,    91,   145,   169,   134,   196,    38,
      88,   136,   140,    81,    89,    89,    81,    85,   128,    85,
      80,    60,   186,   190,   191,   169,   189,    80,   189,    89,
      89,   154,   169,   169,   169,   169,   169,   169,   169,   169,
     151,   152,   153,   169,   194,   199,   156,   156,   156,   157,
     157,   158,   158,   159,   159,   159,   159,   160,   160,   161,
     162,   163,   164,   165,   169,    21,   182,   148,   199,   136,
      85,   132,    85,   145,   146,   135,    88,    82,    87,   142,
     109,    90,    80,    81,    89,    89,   156,    81,    89,    88,
      89,    81,    86,    86,   173,   174,    80,    81,    85,    84,
     108,   145,    38,    88,   189,   189,   192,   186,   173,   174,
     169,   153,   196,   167,    85,   145,    88,    89,    80,    80,
     190,    89,   173,   174
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,   104,   105,   105,   106,   106,   106,   106,   107,   108,
     108,   108,   109,   110,   110,   111,   111,   111,   111,   111,
     111,   111,   111,   112,   112,   112,   112,   112,   112,   112,
     113,   113,   114,   114,   115,   115,   115,   115,   115,   115,
     115,   115,   115,   115,   115,   115,   115,   115,   115,   115,
     115,   115,   115,   115,   115,   116,   117,   117,   118,   118,
     119,   119,   120,   120,   120,   120,   120,   120,   120,   120,
     121,   121,   121,   122,   122,   122,   123,   124,   124,   124,
     125,   125,   126,   127,   127,   128,   128,   129,   130,   131,
     131,   132,   134,   133,   135,   135,   136,   136,   137,   137,
     138,   138,   138,   138,   138,   139,   140,   140,   140,   141,
     141,   142,   142,   143,   143,   144,   144,   145,   145,   145,
     145,   146,   146,   147,   147,   148,   149,   149,   149,   149,
     150,   150,   150,   150,   150,   150,   151,   151,   152,   152,
     153,   153,   154,   154,   155,   155,   155,   155,   155,   155,
     155,   156,   156,   157,   157,   157,   157,   158,   158,   158,
     159,   159,   159,   160,   160,   160,   160,   160,   161,   161,
     161,   162,   162,   163,   163,   164,   164,   165,   165,   166,
     166,   167,   167,   168,   169,   170,   170,   171,   172,   172,
     173,   173,   173,   173,   173,   173,   173,   174,   174,   175,
     175,   176,   176,   177,   178,   178,   179,   180,   180,   181,
     182,   183,   183,   184,   184,   185,   185,   186,   186,   186,
     186,   186,   186,   186,   187,   187,   187,   188,   188,   189,
     190,   190,   191,   191,   192,   192,   193,   193,   194,   195,
     196,   197,   198,   199,   199,   200,   200,   200,   200,   200
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     1,     1,     2,     2,     1,     2,
       3,     2,     2,     1,     2,     1,     2,     2,     2,     2,
       2,     2,     2,     1,     2,     2,     2,     2,     2,     2,
       1,     3,     1,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     2,     2,
       1,     1,     4,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     4,     4,     1,     1,     2,     4,     4,
       1,     1,     1,     1,     2,     1,     1,     5,     1,     1,
       2,     3,     0,     4,     0,     2,     1,     2,     1,     3,
       1,     4,     3,     3,     3,     2,     0,     4,     3,     1,
       3,     2,     4,     0,     1,     1,     3,     1,     3,     4,
       2,     1,     3,     1,     3,     1,     1,     1,     3,     4,
       1,     2,     2,     3,     4,     4,     0,     1,     1,     3,
       1,     3,     1,     3,     1,     2,     2,     2,     2,     2,
       2,     1,     4,     1,     3,     3,     3,     1,     3,     3,
       1,     3,     3,     1,     3,     3,     3,     3,     1,     3,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     5,     1,     1,     3,     2,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       3,     2,     2,     4,     2,     4,     4,     3,     2,     1,
       1,     1,     2,     1,     1,     2,     1,     3,     1,     3,
       3,     3,     3,     3,     5,     7,     9,     5,     9,     1,
       1,     0,     1,     3,     1,     0,     3,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1
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
#line 288 "parser.y"
                              { (yyval.dummy) = GlobalInitStatements(CurrentScope, (yyvsp[0].sc_stmt)); }
#line 2039 "parser.c"
    break;

  case 5: /* external_declaration: function_definition  */
#line 290 "parser.y"
                              { (yyval.dummy) = 0; }
#line 2045 "parser.c"
    break;

  case 6: /* external_declaration: profile_specifier function_definition  */
#line 292 "parser.y"
                              { (yyval.dummy) = 0; }
#line 2051 "parser.c"
    break;

  case 7: /* external_declaration: profile_specifier declaration  */
#line 294 "parser.y"
                              { (yyval.dummy) = GlobalInitStatements(CurrentScope, (yyvsp[0].sc_stmt)); ClearPendingProfileSpecifier(); }
#line 2057 "parser.c"
    break;

  case 8: /* profile_specifier: identifier  */
#line 304 "parser.y"
                              { (yyval.sc_ident) = (yyvsp[0].sc_ident); SetPendingProfileSpecifier(Cg->tokenLoc, (yyvsp[0].sc_ident)); }
#line 2063 "parser.c"
    break;

  case 9: /* declaration: declaration_specifiers ';'  */
#line 308 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2069 "parser.c"
    break;

  case 10: /* declaration: declaration_specifiers init_declarator_list ';'  */
#line 310 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[-1].sc_stmt); }
#line 2075 "parser.c"
    break;

  case 11: /* declaration: ERROR_SY ';'  */
#line 312 "parser.y"
                              { RecordErrorPos(Cg->tokenLoc);
                                ClearPendingGeometryModifiers();
                                (yyval.sc_stmt) = NULL; }
#line 2083 "parser.c"
    break;

  case 12: /* abstract_declaration: abstract_declaration_specifiers abstract_declarator  */
#line 318 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2089 "parser.c"
    break;

  case 13: /* declaration_specifiers: abstract_declaration_specifiers  */
#line 325 "parser.y"
                              { (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 2095 "parser.c"
    break;

  case 14: /* declaration_specifiers: TYPEDEF_SY abstract_declaration_specifiers  */
#line 327 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, TYPE_MISC_TYPEDEF); (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 2101 "parser.c"
    break;

  case 15: /* abstract_declaration_specifiers: abstract_declaration_specifiers2  */
#line 332 "parser.y"
                              { (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 2107 "parser.c"
    break;

  case 16: /* abstract_declaration_specifiers: type_qualifier abstract_declaration_specifiers  */
#line 334 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2113 "parser.c"
    break;

  case 17: /* abstract_declaration_specifiers: storage_class abstract_declaration_specifiers  */
#line 336 "parser.y"
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2119 "parser.c"
    break;

  case 18: /* abstract_declaration_specifiers: type_domain abstract_declaration_specifiers  */
#line 338 "parser.y"
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2125 "parser.c"
    break;

  case 19: /* abstract_declaration_specifiers: in_out abstract_declaration_specifiers  */
#line 340 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2131 "parser.c"
    break;

  case 20: /* abstract_declaration_specifiers: function_specifier abstract_declaration_specifiers  */
#line 342 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2137 "parser.c"
    break;

  case 21: /* abstract_declaration_specifiers: geometry_modifier abstract_declaration_specifiers  */
#line 347 "parser.y"
                              { (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2143 "parser.c"
    break;

  case 22: /* abstract_declaration_specifiers: PACKED_SY abstract_declaration_specifiers  */
#line 349 "parser.y"
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2149 "parser.c"
    break;

  case 23: /* abstract_declaration_specifiers2: type_specifier  */
#line 354 "parser.y"
                              { (yyval.sc_type) = *SetDType(&CurrentDeclTypeSpecs, (yyvsp[0].sc_ptype)); }
#line 2155 "parser.c"
    break;

  case 24: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 type_qualifier  */
#line 356 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2161 "parser.c"
    break;

  case 25: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 storage_class  */
#line 358 "parser.y"
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2167 "parser.c"
    break;

  case 26: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 type_domain  */
#line 360 "parser.y"
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2173 "parser.c"
    break;

  case 27: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 in_out  */
#line 362 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2179 "parser.c"
    break;

  case 28: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 function_specifier  */
#line 364 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2185 "parser.c"
    break;

  case 29: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 PACKED_SY  */
#line 366 "parser.y"
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2191 "parser.c"
    break;

  case 30: /* init_declarator_list: init_declarator  */
#line 370 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[0].sc_stmt); }
#line 2197 "parser.c"
    break;

  case 31: /* init_declarator_list: init_declarator_list ',' init_declarator  */
#line 372 "parser.y"
                              { (yyval.sc_stmt) = AddStmt((yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2203 "parser.c"
    break;

  case 32: /* init_declarator: declarator  */
#line 376 "parser.y"
                              { (yyval.sc_stmt) = Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_decl), NULL); }
#line 2209 "parser.c"
    break;

  case 33: /* init_declarator: declarator '=' initializer  */
#line 378 "parser.y"
                              { (yyval.sc_stmt) = Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[0].sc_expr)); }
#line 2215 "parser.c"
    break;

  case 34: /* type_specifier: INT_SY  */
#line 386 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, INT_SY); }
#line 2221 "parser.c"
    break;

  case 35: /* type_specifier: FLOAT_SY  */
#line 388 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, FLOAT_SY); }
#line 2227 "parser.c"
    break;

  case 36: /* type_specifier: VOID_SY  */
#line 390 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, VOID_SY); }
#line 2233 "parser.c"
    break;

  case 37: /* type_specifier: BOOLEAN_SY  */
#line 392 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, BOOLEAN_SY); }
#line 2239 "parser.c"
    break;

  case 38: /* type_specifier: TEXOBJ_SY  */
#line 394 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, TEXOBJ_SY); }
#line 2245 "parser.c"
    break;

  case 39: /* type_specifier: CHAR_SY  */
#line 396 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2251 "parser.c"
    break;

  case 40: /* type_specifier: SHORT_SY  */
#line 398 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2257 "parser.c"
    break;

  case 41: /* type_specifier: LONG_SY  */
#line 400 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2263 "parser.c"
    break;

  case 42: /* type_specifier: HALF_SY  */
#line 402 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2269 "parser.c"
    break;

  case 43: /* type_specifier: FIXED_SY  */
#line 404 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2275 "parser.c"
    break;

  case 44: /* type_specifier: DOUBLE_SY  */
#line 406 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2281 "parser.c"
    break;

  case 45: /* type_specifier: UNSIGNED_SY  */
#line 408 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2287 "parser.c"
    break;

  case 46: /* type_specifier: UNSIGNED_SY CHAR_SY  */
#line 410 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2293 "parser.c"
    break;

  case 47: /* type_specifier: UNSIGNED_SY SHORT_SY  */
#line 412 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2299 "parser.c"
    break;

  case 48: /* type_specifier: UNSIGNED_SY INT_SY  */
#line 414 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2305 "parser.c"
    break;

  case 49: /* type_specifier: UNSIGNED_SY LONG_SY  */
#line 416 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2311 "parser.c"
    break;

  case 50: /* type_specifier: struct_or_connector_specifier  */
#line 418 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2317 "parser.c"
    break;

  case 51: /* type_specifier: interface_specifier  */
#line 420 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2323 "parser.c"
    break;

  case 52: /* type_specifier: ATTRIBARRAY_SY '<' type_specifier '>'  */
#line 422 "parser.y"
                              { (yyval.sc_ptype) = SetAttribArrayType(Cg->tokenLoc, (yyvsp[-1].sc_ptype)); }
#line 2329 "parser.c"
    break;

  case 53: /* type_specifier: type_identifier  */
#line 424 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, (yyvsp[0].sc_ident)); }
#line 2335 "parser.c"
    break;

  case 54: /* type_specifier: error  */
#line 426 "parser.y"
                              {
                                ClearPendingGeometryModifiers();
                                SemanticParseError(Cg->tokenLoc, ERROR_S_TYPE_NAME_EXPECTED,
                                                   GetAtomString(atable, Cg->mostRecentToken /* yychar */));
                                (yyval.sc_ptype) = UndefinedType;
                              }
#line 2346 "parser.c"
    break;

  case 55: /* type_qualifier: CONST_SY  */
#line 439 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_CONST; }
#line 2352 "parser.c"
    break;

  case 56: /* type_domain: UNIFORM_SY  */
#line 447 "parser.y"
                              { (yyval.sc_int) = TYPE_DOMAIN_UNIFORM; }
#line 2358 "parser.c"
    break;

  case 57: /* type_domain: VARYING_SY  */
#line 449 "parser.y"
                              { (yyval.sc_int) = TYPE_DOMAIN_VARYING; }
#line 2364 "parser.c"
    break;

  case 58: /* storage_class: STATIC_SY  */
#line 457 "parser.y"
                              { (yyval.sc_int) = (int) SC_STATIC; }
#line 2370 "parser.c"
    break;

  case 59: /* storage_class: EXTERN_SY  */
#line 459 "parser.y"
                              { (yyval.sc_int) = (int) SC_EXTERN; }
#line 2376 "parser.c"
    break;

  case 60: /* function_specifier: INLINE_SY  */
#line 467 "parser.y"
                              { (yyval.sc_int) = TYPE_MISC_INLINE; }
#line 2382 "parser.c"
    break;

  case 61: /* function_specifier: INTERNAL_SY  */
#line 469 "parser.y"
                              { (yyval.sc_int) = TYPE_MISC_INTERNAL; }
#line 2388 "parser.c"
    break;

  case 62: /* geometry_modifier: POINT_SY  */
#line 482 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_POINT); }
#line 2394 "parser.c"
    break;

  case 63: /* geometry_modifier: LINE_SY  */
#line 483 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_LINE); }
#line 2400 "parser.c"
    break;

  case 64: /* geometry_modifier: LINE_ADJ_SY  */
#line 484 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_LINE_ADJACENCY); }
#line 2406 "parser.c"
    break;

  case 65: /* geometry_modifier: TRIANGLE_SY  */
#line 485 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_TRIANGLE); }
#line 2412 "parser.c"
    break;

  case 66: /* geometry_modifier: TRIANGLE_ADJ_SY  */
#line 486 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY); }
#line 2418 "parser.c"
    break;

  case 67: /* geometry_modifier: POINT_OUT_SY  */
#line 487 "parser.y"
                          { (yyval.sc_int) = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_POINTS); }
#line 2424 "parser.c"
    break;

  case 68: /* geometry_modifier: LINE_OUT_SY  */
#line 488 "parser.y"
                          { (yyval.sc_int) = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_LINE_STRIP); }
#line 2430 "parser.c"
    break;

  case 69: /* geometry_modifier: TRIANGLE_OUT_SY  */
#line 489 "parser.y"
                          { (yyval.sc_int) = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP); }
#line 2436 "parser.c"
    break;

  case 70: /* in_out: IN_SY  */
#line 497 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_IN; }
#line 2442 "parser.c"
    break;

  case 71: /* in_out: OUT_SY  */
#line 499 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_OUT; }
#line 2448 "parser.c"
    break;

  case 72: /* in_out: INOUT_SY  */
#line 501 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_INOUT; }
#line 2454 "parser.c"
    break;

  case 73: /* struct_or_connector_specifier: struct_or_connector_header struct_compound_header struct_declaration_list '}'  */
#line 510 "parser.y"
                              { (yyval.sc_ptype) = SetStructMembers(Cg->tokenLoc, (yyvsp[-3].sc_ptype), PopScope());
                                CheckInterfaceConformance(Cg->tokenLoc, (yyval.sc_ptype)); }
#line 2461 "parser.c"
    break;

  case 74: /* struct_or_connector_specifier: untagged_struct_header struct_compound_header struct_declaration_list '}'  */
#line 513 "parser.y"
                              { (yyval.sc_ptype) = SetStructMembers(Cg->tokenLoc, (yyvsp[-3].sc_ptype), PopScope());
                                CheckInterfaceConformance(Cg->tokenLoc, (yyval.sc_ptype)); }
#line 2468 "parser.c"
    break;

  case 75: /* struct_or_connector_specifier: struct_or_connector_header  */
#line 516 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2474 "parser.c"
    break;

  case 76: /* struct_compound_header: compound_header  */
#line 520 "parser.y"
                              { CurrentScope->IsStructScope = 1; (yyval.dummy) = (yyvsp[0].dummy); }
#line 2480 "parser.c"
    break;

  case 77: /* struct_or_connector_header: STRUCT_SY struct_identifier  */
#line 525 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, 0, (yyvsp[0].sc_ident)); }
#line 2486 "parser.c"
    break;

  case 78: /* struct_or_connector_header: STRUCT_SY struct_identifier ':' semantics_identifier  */
#line 527 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_ident), (yyvsp[-2].sc_ident)); }
#line 2492 "parser.c"
    break;

  case 79: /* struct_or_connector_header: STRUCT_SY struct_identifier ':' type_identifier  */
#line 529 "parser.y"
                              { (yyval.sc_ptype) = SetStructInterface(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_ident), (yyvsp[0].sc_ident)); }
#line 2498 "parser.c"
    break;

  case 82: /* untagged_struct_header: STRUCT_SY  */
#line 537 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, 0, 0); }
#line 2504 "parser.c"
    break;

  case 85: /* struct_declaration: declaration  */
#line 545 "parser.y"
                            { (yyval.sc_stmt) = (yyvsp[0].sc_stmt); }
#line 2510 "parser.c"
    break;

  case 86: /* struct_declaration: function_definition  */
#line 547 "parser.y"
                            { (yyval.sc_stmt) = NULL; }
#line 2516 "parser.c"
    break;

  case 87: /* interface_specifier: INTERFACE_SY struct_identifier interface_compound_header interface_member_declaration_list '}'  */
#line 558 "parser.y"
                              { (yyval.sc_ptype) = SetInterfaceMembers(Cg->tokenLoc,
                                                         InterfaceHeader(Cg->tokenLoc, CurrentScope, (yyvsp[-3].sc_ident)),
                                                         PopScope()); }
#line 2524 "parser.c"
    break;

  case 88: /* interface_compound_header: compound_header  */
#line 565 "parser.y"
                              { CurrentScope->IsStructScope = 1; (yyval.dummy) = (yyvsp[0].dummy); }
#line 2530 "parser.c"
    break;

  case 91: /* interface_member_declaration: declaration_specifiers declarator ';'  */
#line 582 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2536 "parser.c"
    break;

  case 92: /* $@1: %empty  */
#line 600 "parser.y"
                              { PushScope(NewScope()); }
#line 2542 "parser.c"
    break;

  case 93: /* annotation: '<' $@1 annotation_decl_list '>'  */
#line 601 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[-1].sc_stmt); PopScope(); }
#line 2548 "parser.c"
    break;

  case 94: /* annotation_decl_list: %empty  */
#line 605 "parser.y"
                              { (yyval.sc_stmt) = 0; }
#line 2554 "parser.c"
    break;

  case 96: /* declarator: semantic_declarator  */
#line 614 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2560 "parser.c"
    break;

  case 97: /* declarator: semantic_declarator annotation  */
#line 616 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[-1].sc_decl); }
#line 2566 "parser.c"
    break;

  case 98: /* semantic_declarator: basic_declarator  */
#line 620 "parser.y"
                              { (yyval.sc_decl) = Declarator(Cg->tokenLoc, (yyvsp[0].sc_decl), 0); }
#line 2572 "parser.c"
    break;

  case 99: /* semantic_declarator: basic_declarator ':' semantics_identifier  */
#line 622 "parser.y"
                              { (yyval.sc_decl) = Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), (yyvsp[0].sc_ident)); }
#line 2578 "parser.c"
    break;

  case 100: /* basic_declarator: identifier  */
#line 626 "parser.y"
                              { (yyval.sc_decl) = NewDeclNode(Cg->tokenLoc, (yyvsp[0].sc_ident), &CurrentDeclTypeSpecs); }
#line 2584 "parser.c"
    break;

  case 101: /* basic_declarator: basic_declarator '[' INTCONST_SY ']'  */
#line 628 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-3].sc_decl), (int) (yyvsp[-1].sc_literal).value.i, 0); }
#line 2590 "parser.c"
    break;

  case 102: /* basic_declarator: basic_declarator '[' ']'  */
#line 630 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), 0 , 1); }
#line 2596 "parser.c"
    break;

  case 103: /* basic_declarator: function_decl_header parameter_list ')'  */
#line 632 "parser.y"
                              { (yyval.sc_decl) = SetFunTypeParams(CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_decl), (yyvsp[-1].sc_decl)); }
#line 2602 "parser.c"
    break;

  case 104: /* basic_declarator: function_decl_header abstract_parameter_list ')'  */
#line 634 "parser.y"
                              { (yyval.sc_decl) = SetFunTypeParams(CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_decl), NULL); }
#line 2608 "parser.c"
    break;

  case 105: /* function_decl_header: basic_declarator '('  */
#line 638 "parser.y"
                              { (yyval.sc_decl) = FunctionDeclHeader(&(yyvsp[-1].sc_decl)->loc, CurrentScope, (yyvsp[-1].sc_decl)); }
#line 2614 "parser.c"
    break;

  case 106: /* abstract_declarator: %empty  */
#line 642 "parser.y"
                              { (yyval.sc_decl) = NewDeclNode(Cg->tokenLoc, 0, &CurrentDeclTypeSpecs); }
#line 2620 "parser.c"
    break;

  case 107: /* abstract_declarator: abstract_declarator '[' INTCONST_SY ']'  */
#line 644 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-3].sc_decl), (int) (yyvsp[-1].sc_literal).value.i, 0); }
#line 2626 "parser.c"
    break;

  case 108: /* abstract_declarator: abstract_declarator '[' ']'  */
#line 646 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), 0 , 1); }
#line 2632 "parser.c"
    break;

  case 109: /* parameter_list: parameter_declaration  */
#line 663 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2638 "parser.c"
    break;

  case 110: /* parameter_list: parameter_list ',' parameter_declaration  */
#line 665 "parser.y"
                              { (yyval.sc_decl) = AddDecl((yyvsp[-2].sc_decl), (yyvsp[0].sc_decl)); }
#line 2644 "parser.c"
    break;

  case 111: /* parameter_declaration: declaration_specifiers declarator  */
#line 669 "parser.y"
                              { (yyval.sc_decl) = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_decl), NULL); }
#line 2650 "parser.c"
    break;

  case 112: /* parameter_declaration: declaration_specifiers declarator '=' initializer  */
#line 671 "parser.y"
                              { (yyval.sc_decl) = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[0].sc_expr)); }
#line 2656 "parser.c"
    break;

  case 113: /* abstract_parameter_list: %empty  */
#line 675 "parser.y"
                              { (yyval.sc_decl) = NULL; }
#line 2662 "parser.c"
    break;

  case 115: /* non_empty_abstract_parameter_list: abstract_declaration  */
#line 680 "parser.y"
                              {
                                if (IsVoid(&(yyvsp[0].sc_decl)->type.type))
                                    CurrentScope->HasVoidParameter = 1;
                                (yyval.sc_decl) = (yyvsp[0].sc_decl);
                              }
#line 2672 "parser.c"
    break;

  case 116: /* non_empty_abstract_parameter_list: non_empty_abstract_parameter_list ',' abstract_declaration  */
#line 686 "parser.y"
                              {
                                if (CurrentScope->HasVoidParameter || IsVoid(&(yyvsp[-2].sc_decl)->type.type)) {
                                    SemanticError(Cg->tokenLoc, ERROR___VOID_NOT_ONLY_PARAM);
                                }
                                (yyval.sc_decl) = AddDecl((yyvsp[-2].sc_decl), (yyvsp[0].sc_decl));
                              }
#line 2683 "parser.c"
    break;

  case 117: /* initializer: expression  */
#line 699 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[0].sc_expr)); }
#line 2689 "parser.c"
    break;

  case 118: /* initializer: '{' initializer_list '}'  */
#line 701 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[-1].sc_expr)); }
#line 2695 "parser.c"
    break;

  case 119: /* initializer: '{' initializer_list ',' '}'  */
#line 703 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[-2].sc_expr)); }
#line 2701 "parser.c"
    break;

  case 120: /* initializer: '{' '}'  */
#line 709 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, NULL); }
#line 2707 "parser.c"
    break;

  case 121: /* initializer_list: initializer  */
#line 713 "parser.y"
                              { (yyval.sc_expr) = InitializerList(Cg->tokenLoc, (yyvsp[0].sc_expr), NULL); }
#line 2713 "parser.c"
    break;

  case 122: /* initializer_list: initializer_list ',' initializer  */
#line 715 "parser.y"
                              { (yyval.sc_expr) = InitializerList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2719 "parser.c"
    break;

  case 123: /* variable: basic_variable  */
#line 727 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[0].sc_expr); }
#line 2725 "parser.c"
    break;

  case 124: /* variable: scope_identifier COLONCOLON_SY basic_variable  */
#line 729 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[0].sc_expr); }
#line 2731 "parser.c"
    break;

  case 125: /* basic_variable: variable_identifier  */
#line 733 "parser.y"
                              { (yyval.sc_expr) = BasicVariable(Cg->tokenLoc, (yyvsp[0].sc_ident)); }
#line 2737 "parser.c"
    break;

  case 128: /* primary_expression: '(' expression ')'  */
#line 743 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[-1].sc_expr); }
#line 2743 "parser.c"
    break;

  case 129: /* primary_expression: type_specifier '(' expression_list ')'  */
#line 745 "parser.y"
                              { (yyval.sc_expr) = NewVectorConstructor(Cg->tokenLoc, (yyvsp[-3].sc_ptype), (yyvsp[-1].sc_expr)); }
#line 2749 "parser.c"
    break;

  case 131: /* postfix_expression: postfix_expression PLUSPLUS_SY  */
#line 754 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(POSTINC_OP, (yyvsp[-1].sc_expr)); }
#line 2755 "parser.c"
    break;

  case 132: /* postfix_expression: postfix_expression MINUSMINUS_SY  */
#line 756 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(POSTDEC_OP, (yyvsp[-1].sc_expr)); }
#line 2761 "parser.c"
    break;

  case 133: /* postfix_expression: postfix_expression '.' member_identifier  */
#line 758 "parser.y"
                              { (yyval.sc_expr) = NewMemberSelectorOrSwizzleOrWriteMaskOperator(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_ident)); }
#line 2767 "parser.c"
    break;

  case 134: /* postfix_expression: postfix_expression '[' expression ']'  */
#line 760 "parser.y"
                              { (yyval.sc_expr) = NewIndexOperator(Cg->tokenLoc, (yyvsp[-3].sc_expr), (yyvsp[-1].sc_expr)); }
#line 2773 "parser.c"
    break;

  case 135: /* postfix_expression: postfix_expression '(' actual_argument_list ')'  */
#line 762 "parser.y"
                              { (yyval.sc_expr) = NewFunctionCallOperator(Cg->tokenLoc, (yyvsp[-3].sc_expr), (yyvsp[-1].sc_expr)); }
#line 2779 "parser.c"
    break;

  case 136: /* actual_argument_list: %empty  */
#line 766 "parser.y"
                                { (yyval.sc_expr) = NULL; }
#line 2785 "parser.c"
    break;

  case 138: /* non_empty_argument_list: actual_argument  */
#line 771 "parser.y"
                              { (yyval.sc_expr) = ArgumentList(Cg->tokenLoc, NULL, (yyvsp[0].sc_expr)); }
#line 2791 "parser.c"
    break;

  case 139: /* non_empty_argument_list: non_empty_argument_list ',' actual_argument  */
#line 773 "parser.y"
                              { (yyval.sc_expr) = ArgumentList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2797 "parser.c"
    break;

  case 140: /* actual_argument: expression  */
#line 783 "parser.y"
                              { (yyval.sc_expr) = NewGeometryArgument(Cg->tokenLoc, (yyvsp[0].sc_expr), 0); }
#line 2803 "parser.c"
    break;

  case 141: /* actual_argument: expression ':' semantics_identifier  */
#line 785 "parser.y"
                              { (yyval.sc_expr) = NewGeometryArgument(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_ident)); }
#line 2809 "parser.c"
    break;

  case 142: /* expression_list: expression  */
#line 789 "parser.y"
                              { (yyval.sc_expr) = ExpressionList(Cg->tokenLoc, NULL, (yyvsp[0].sc_expr)); }
#line 2815 "parser.c"
    break;

  case 143: /* expression_list: expression_list ',' expression  */
#line 791 "parser.y"
                              { (yyval.sc_expr) = ExpressionList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2821 "parser.c"
    break;

  case 145: /* unary_expression: PLUSPLUS_SY unary_expression  */
#line 800 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(PREINC_OP, (yyvsp[0].sc_expr)); }
#line 2827 "parser.c"
    break;

  case 146: /* unary_expression: MINUSMINUS_SY unary_expression  */
#line 802 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(PREDEC_OP, (yyvsp[0].sc_expr)); }
#line 2833 "parser.c"
    break;

  case 147: /* unary_expression: '+' unary_expression  */
#line 804 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, POS_OP, '+', (yyvsp[0].sc_expr), 0); }
#line 2839 "parser.c"
    break;

  case 148: /* unary_expression: '-' unary_expression  */
#line 806 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, NEG_OP, '-', (yyvsp[0].sc_expr), 0); }
#line 2845 "parser.c"
    break;

  case 149: /* unary_expression: '!' unary_expression  */
#line 808 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, BNOT_OP, '!', (yyvsp[0].sc_expr), 0); }
#line 2851 "parser.c"
    break;

  case 150: /* unary_expression: '~' unary_expression  */
#line 810 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, NOT_OP, '~', (yyvsp[0].sc_expr), 1); }
#line 2857 "parser.c"
    break;

  case 152: /* cast_expression: '(' abstract_declaration ')' cast_expression  */
#line 822 "parser.y"
                              { (yyval.sc_expr) = NewCastOperator(Cg->tokenLoc, (yyvsp[0].sc_expr), GetTypePointer(&(yyvsp[-2].sc_decl)->loc, &(yyvsp[-2].sc_decl)->type)); }
#line 2863 "parser.c"
    break;

  case 154: /* multiplicative_expression: multiplicative_expression '*' cast_expression  */
#line 831 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, MUL_OP, '*', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2869 "parser.c"
    break;

  case 155: /* multiplicative_expression: multiplicative_expression '/' cast_expression  */
#line 833 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, DIV_OP, '/', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2875 "parser.c"
    break;

  case 156: /* multiplicative_expression: multiplicative_expression '%' cast_expression  */
#line 835 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, MOD_OP, '%', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2881 "parser.c"
    break;

  case 158: /* additive_expression: additive_expression '+' multiplicative_expression  */
#line 844 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, ADD_OP, '+', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2887 "parser.c"
    break;

  case 159: /* additive_expression: additive_expression '-' multiplicative_expression  */
#line 846 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SUB_OP, '-', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2893 "parser.c"
    break;

  case 161: /* shift_expression: shift_expression LL_SY additive_expression  */
#line 855 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SHL_OP, LL_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2899 "parser.c"
    break;

  case 162: /* shift_expression: shift_expression GG_SY additive_expression  */
#line 857 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SHR_OP, GG_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2905 "parser.c"
    break;

  case 164: /* relational_expression: relational_expression '<' shift_expression  */
#line 866 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, LT_OP, '<', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2911 "parser.c"
    break;

  case 165: /* relational_expression: relational_expression '>' shift_expression  */
#line 868 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, GT_OP, '>', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2917 "parser.c"
    break;

  case 166: /* relational_expression: relational_expression LE_SY shift_expression  */
#line 870 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, LE_OP, LE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2923 "parser.c"
    break;

  case 167: /* relational_expression: relational_expression GE_SY shift_expression  */
#line 872 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, GE_OP, GE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2929 "parser.c"
    break;

  case 169: /* equality_expression: equality_expression EQ_SY relational_expression  */
#line 881 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, EQ_OP, EQ_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2935 "parser.c"
    break;

  case 170: /* equality_expression: equality_expression NE_SY relational_expression  */
#line 883 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, NE_OP, NE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2941 "parser.c"
    break;

  case 172: /* AND_expression: AND_expression '&' equality_expression  */
#line 892 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, AND_OP, '&', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2947 "parser.c"
    break;

  case 174: /* exclusive_OR_expression: exclusive_OR_expression '^' AND_expression  */
#line 901 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, XOR_OP, '^', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2953 "parser.c"
    break;

  case 176: /* inclusive_OR_expression: inclusive_OR_expression '|' exclusive_OR_expression  */
#line 910 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, OR_OP, '|', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2959 "parser.c"
    break;

  case 178: /* logical_AND_expression: logical_AND_expression AND_SY inclusive_OR_expression  */
#line 919 "parser.y"
                              { (yyval.sc_expr) = NewBinaryBooleanOperator(Cg->tokenLoc, BAND_OP, AND_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2965 "parser.c"
    break;

  case 180: /* logical_OR_expression: logical_OR_expression OR_SY logical_AND_expression  */
#line 928 "parser.y"
                              { (yyval.sc_expr) = NewBinaryBooleanOperator(Cg->tokenLoc, BOR_OP, OR_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2971 "parser.c"
    break;

  case 182: /* conditional_expression: conditional_test '?' expression ':' conditional_expression  */
#line 937 "parser.y"
                              { (yyval.sc_expr) = NewConditionalOperator(Cg->tokenLoc, (yyvsp[-4].sc_expr), (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2977 "parser.c"
    break;

  case 183: /* conditional_test: logical_OR_expression  */
#line 941 "parser.y"
                              {  (yyval.sc_expr) = CheckBooleanExpr(Cg->tokenLoc, (yyvsp[0].sc_expr), 1); }
#line 2983 "parser.c"
    break;

  case 185: /* function_definition: function_definition_header block_item_list '}'  */
#line 960 "parser.y"
                              { DefineFunction(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_stmt)); PopScope();
                                ResumeStructScopeAfterMethodBody(); }
#line 2990 "parser.c"
    break;

  case 186: /* function_definition: function_definition_header '}'  */
#line 963 "parser.y"
                              { DefineFunction(Cg->tokenLoc, CurrentScope, (yyvsp[-1].sc_decl), NULL); PopScope();
                                ResumeStructScopeAfterMethodBody(); }
#line 2997 "parser.c"
    break;

  case 187: /* function_definition_header: declaration_specifiers declarator '{'  */
#line 968 "parser.y"
                              { (yyval.sc_decl) = Function_Definition_Header(Cg->tokenLoc, (yyvsp[-1].sc_decl)); }
#line 3003 "parser.c"
    break;

  case 199: /* discard_statement: DISCARD_SY ';'  */
#line 997 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewDiscardStmt(Cg->tokenLoc, NULL); }
#line 3009 "parser.c"
    break;

  case 200: /* discard_statement: DISCARD_SY expression ';'  */
#line 999 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewDiscardStmt(Cg->tokenLoc, CheckBooleanExpr(Cg->tokenLoc, (yyvsp[-1].sc_expr), 1)); }
#line 3015 "parser.c"
    break;

  case 201: /* jump_statement: BREAK_SY ';'  */
#line 1007 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewSimpleStmt(Cg->tokenLoc, BREAK_STMT); }
#line 3021 "parser.c"
    break;

  case 202: /* jump_statement: CONTINUE_SY ';'  */
#line 1009 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewSimpleStmt(Cg->tokenLoc, CONTINUE_STMT); }
#line 3027 "parser.c"
    break;

  case 203: /* if_statement: if_header balanced_statement ELSE_SY balanced_statement  */
#line 1017 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-3].sc_stmt), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3033 "parser.c"
    break;

  case 204: /* dangling_if: if_header statement  */
#line 1021 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-1].sc_stmt), (yyvsp[0].sc_stmt), NULL); }
#line 3039 "parser.c"
    break;

  case 205: /* dangling_if: if_header balanced_statement ELSE_SY dangling_statement  */
#line 1023 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-3].sc_stmt), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3045 "parser.c"
    break;

  case 206: /* if_header: IF_SY '(' boolean_scalar_expression ')'  */
#line 1027 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewIfStmt(Cg->tokenLoc, (yyvsp[-1].sc_expr), NULL, NULL); ; }
#line 3051 "parser.c"
    break;

  case 207: /* compound_statement: compound_header block_item_list compound_tail  */
#line 1035 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewBlockStmt(Cg->tokenLoc, (yyvsp[-1].sc_stmt)); }
#line 3057 "parser.c"
    break;

  case 208: /* compound_statement: compound_header compound_tail  */
#line 1037 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 3063 "parser.c"
    break;

  case 209: /* compound_header: '{'  */
#line 1041 "parser.y"
                              { PushScope(NewScope()); CurrentScope->funindex = NextFunctionIndex; }
#line 3069 "parser.c"
    break;

  case 210: /* compound_tail: '}'  */
#line 1045 "parser.y"
                              {
                                if (Cg->options.DumpParseTree)
                                    PrintScopeDeclarations();
                                PopScope();
                              }
#line 3079 "parser.c"
    break;

  case 212: /* block_item_list: block_item_list block_item  */
#line 1054 "parser.y"
                              { (yyval.sc_stmt) = AddStmt((yyvsp[-1].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3085 "parser.c"
    break;

  case 214: /* block_item: statement  */
#line 1059 "parser.y"
                              { (yyval.sc_stmt) = CheckStmt((yyvsp[0].sc_stmt)); }
#line 3091 "parser.c"
    break;

  case 216: /* expression_statement: ';'  */
#line 1068 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 3097 "parser.c"
    break;

  case 217: /* expression_statement2: postfix_expression '=' expression  */
#line 1072 "parser.y"
                              { (yyval.sc_stmt) = NewSimpleAssignmentStmt(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 3103 "parser.c"
    break;

  case 218: /* expression_statement2: expression  */
#line 1074 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewExprStmt(Cg->tokenLoc, (yyvsp[0].sc_expr)); }
#line 3109 "parser.c"
    break;

  case 219: /* expression_statement2: postfix_expression ASSIGNMINUS_SY expression  */
#line 1076 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMINUS_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3115 "parser.c"
    break;

  case 220: /* expression_statement2: postfix_expression ASSIGNMOD_SY expression  */
#line 1078 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMOD_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3121 "parser.c"
    break;

  case 221: /* expression_statement2: postfix_expression ASSIGNPLUS_SY expression  */
#line 1080 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNPLUS_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3127 "parser.c"
    break;

  case 222: /* expression_statement2: postfix_expression ASSIGNSLASH_SY expression  */
#line 1082 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSLASH_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3133 "parser.c"
    break;

  case 223: /* expression_statement2: postfix_expression ASSIGNSTAR_SY expression  */
#line 1084 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSTAR_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3139 "parser.c"
    break;

  case 224: /* iteration_statement: WHILE_SY '(' boolean_scalar_expression ')' balanced_statement  */
#line 1092 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, (yyvsp[-2].sc_expr), (yyvsp[0].sc_stmt)); }
#line 3145 "parser.c"
    break;

  case 225: /* iteration_statement: DO_SY statement WHILE_SY '(' boolean_scalar_expression ')' ';'  */
#line 1094 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, DO_STMT, (yyvsp[-2].sc_expr), (yyvsp[-5].sc_stmt)); }
#line 3151 "parser.c"
    break;

  case 226: /* iteration_statement: FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' balanced_statement  */
#line 1096 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewForStmt(Cg->tokenLoc, (yyvsp[-6].sc_stmt), (yyvsp[-4].sc_expr), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3157 "parser.c"
    break;

  case 227: /* dangling_iteration: WHILE_SY '(' boolean_scalar_expression ')' dangling_statement  */
#line 1100 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, (yyvsp[-2].sc_expr), (yyvsp[0].sc_stmt)); }
#line 3163 "parser.c"
    break;

  case 228: /* dangling_iteration: FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' dangling_statement  */
#line 1102 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewForStmt(Cg->tokenLoc, (yyvsp[-6].sc_stmt), (yyvsp[-4].sc_expr), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3169 "parser.c"
    break;

  case 229: /* boolean_scalar_expression: expression  */
#line 1107 "parser.y"
                              {  (yyval.sc_expr) = CheckBooleanExpr(Cg->tokenLoc, (yyvsp[0].sc_expr), 0); }
#line 3175 "parser.c"
    break;

  case 231: /* for_expression_opt: %empty  */
#line 1112 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 3181 "parser.c"
    break;

  case 233: /* for_expression: for_expression ',' expression_statement2  */
#line 1117 "parser.y"
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
#line 3197 "parser.c"
    break;

  case 235: /* boolean_expression_opt: %empty  */
#line 1132 "parser.y"
                              { (yyval.sc_expr) = NULL; }
#line 3203 "parser.c"
    break;

  case 236: /* return_statement: RETURN_SY expression ';'  */
#line 1140 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, (yyvsp[-1].sc_expr)); }
#line 3209 "parser.c"
    break;

  case 237: /* return_statement: RETURN_SY ';'  */
#line 1142 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, NULL); }
#line 3215 "parser.c"
    break;

  case 243: /* identifier: IDENT_SY  */
#line 1165 "parser.y"
                              { (yyval.sc_ident) = (yyvsp[0].sc_ident); }
#line 3221 "parser.c"
    break;

  case 244: /* identifier: RESERVED_SY  */
#line 1167 "parser.y"
                              {
                                /* SemanticError, not SemanticParseError: the
                                 * latter is gated by AllowSemanticParseErrors */
                                SemanticError(Cg->tokenLoc, ERROR_S_RESERVED_WORD,
                                              GetAtomString(atable, (yyvsp[0].sc_token)));
                                (yyval.sc_ident) = (yyvsp[0].sc_token);
                              }
#line 3233 "parser.c"
    break;

  case 245: /* constant: INTCONST_SY  */
#line 1177 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(ICONST_OP, &(yyvsp[0].sc_literal)); }
#line 3239 "parser.c"
    break;

  case 246: /* constant: CFLOATCONST_SY  */
#line 1179 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3245 "parser.c"
    break;

  case 247: /* constant: FLOATCONST_SY  */
#line 1181 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3251 "parser.c"
    break;

  case 248: /* constant: FLOATHCONST_SY  */
#line 1183 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3257 "parser.c"
    break;

  case 249: /* constant: FLOATXCONST_SY  */
#line 1185 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3263 "parser.c"
    break;


#line 3267 "parser.c"

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

#line 1199 "parser.y"


