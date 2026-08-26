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
  YYSYMBOL_expression_list = 153,          /* expression_list  */
  YYSYMBOL_unary_expression = 154,         /* unary_expression  */
  YYSYMBOL_cast_expression = 155,          /* cast_expression  */
  YYSYMBOL_multiplicative_expression = 156, /* multiplicative_expression  */
  YYSYMBOL_additive_expression = 157,      /* additive_expression  */
  YYSYMBOL_shift_expression = 158,         /* shift_expression  */
  YYSYMBOL_relational_expression = 159,    /* relational_expression  */
  YYSYMBOL_equality_expression = 160,      /* equality_expression  */
  YYSYMBOL_AND_expression = 161,           /* AND_expression  */
  YYSYMBOL_exclusive_OR_expression = 162,  /* exclusive_OR_expression  */
  YYSYMBOL_inclusive_OR_expression = 163,  /* inclusive_OR_expression  */
  YYSYMBOL_logical_AND_expression = 164,   /* logical_AND_expression  */
  YYSYMBOL_logical_OR_expression = 165,    /* logical_OR_expression  */
  YYSYMBOL_conditional_expression = 166,   /* conditional_expression  */
  YYSYMBOL_conditional_test = 167,         /* conditional_test  */
  YYSYMBOL_expression = 168,               /* expression  */
  YYSYMBOL_function_definition = 169,      /* function_definition  */
  YYSYMBOL_function_definition_header = 170, /* function_definition_header  */
  YYSYMBOL_statement = 171,                /* statement  */
  YYSYMBOL_balanced_statement = 172,       /* balanced_statement  */
  YYSYMBOL_dangling_statement = 173,       /* dangling_statement  */
  YYSYMBOL_discard_statement = 174,        /* discard_statement  */
  YYSYMBOL_jump_statement = 175,           /* jump_statement  */
  YYSYMBOL_if_statement = 176,             /* if_statement  */
  YYSYMBOL_dangling_if = 177,              /* dangling_if  */
  YYSYMBOL_if_header = 178,                /* if_header  */
  YYSYMBOL_compound_statement = 179,       /* compound_statement  */
  YYSYMBOL_compound_header = 180,          /* compound_header  */
  YYSYMBOL_compound_tail = 181,            /* compound_tail  */
  YYSYMBOL_block_item_list = 182,          /* block_item_list  */
  YYSYMBOL_block_item = 183,               /* block_item  */
  YYSYMBOL_expression_statement = 184,     /* expression_statement  */
  YYSYMBOL_expression_statement2 = 185,    /* expression_statement2  */
  YYSYMBOL_iteration_statement = 186,      /* iteration_statement  */
  YYSYMBOL_dangling_iteration = 187,       /* dangling_iteration  */
  YYSYMBOL_boolean_scalar_expression = 188, /* boolean_scalar_expression  */
  YYSYMBOL_for_expression_opt = 189,       /* for_expression_opt  */
  YYSYMBOL_for_expression = 190,           /* for_expression  */
  YYSYMBOL_boolean_expression_opt = 191,   /* boolean_expression_opt  */
  YYSYMBOL_return_statement = 192,         /* return_statement  */
  YYSYMBOL_member_identifier = 193,        /* member_identifier  */
  YYSYMBOL_scope_identifier = 194,         /* scope_identifier  */
  YYSYMBOL_semantics_identifier = 195,     /* semantics_identifier  */
  YYSYMBOL_type_identifier = 196,          /* type_identifier  */
  YYSYMBOL_variable_identifier = 197,      /* variable_identifier  */
  YYSYMBOL_identifier = 198,               /* identifier  */
  YYSYMBOL_constant = 199                  /* constant  */
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
#define YYLAST   2361

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  104
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  96
/* YYNRULES -- Number of rules.  */
#define YYNRULES  247
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  381

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
       0,   278,   278,   279,   286,   288,   290,   292,   302,   306,
     308,   310,   316,   323,   325,   330,   332,   334,   336,   338,
     340,   342,   347,   352,   354,   356,   358,   360,   362,   364,
     368,   370,   374,   376,   384,   386,   388,   390,   392,   394,
     396,   398,   400,   402,   404,   406,   408,   410,   412,   414,
     416,   418,   420,   422,   424,   437,   445,   447,   455,   457,
     465,   467,   481,   482,   483,   484,   485,   486,   487,   488,
     495,   497,   499,   508,   511,   514,   518,   523,   525,   527,
     531,   532,   535,   539,   540,   543,   545,   554,   563,   572,
     573,   577,   599,   599,   604,   605,   612,   614,   618,   620,
     624,   626,   628,   630,   632,   636,   641,   642,   644,   661,
     663,   667,   669,   674,   675,   678,   684,   697,   699,   701,
     703,   711,   713,   725,   727,   731,   739,   740,   741,   743,
     751,   752,   754,   756,   758,   760,   765,   766,   769,   771,
     775,   777,   785,   786,   788,   790,   792,   794,   796,   804,
     808,   816,   817,   819,   821,   829,   830,   832,   840,   841,
     843,   851,   852,   854,   856,   858,   866,   867,   869,   877,
     878,   886,   887,   895,   896,   904,   905,   913,   914,   922,
     923,   927,   935,   946,   949,   954,   962,   963,   966,   967,
     968,   969,   970,   971,   972,   975,   976,   983,   985,   993,
     995,  1003,  1007,  1009,  1013,  1021,  1023,  1027,  1031,  1039,
    1040,  1044,  1045,  1053,  1054,  1058,  1060,  1062,  1064,  1066,
    1068,  1070,  1078,  1080,  1082,  1086,  1088,  1093,  1097,  1099,
    1102,  1103,  1117,  1119,  1126,  1128,  1136,  1139,  1142,  1145,
    1148,  1151,  1153,  1163,  1165,  1167,  1169,  1171
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

#define YYPACT_NINF (-312)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-238)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    2049,  -312,  -312,  -312,   -55,  -312,  -312,  -312,  -312,  -312,
    -312,  -312,  -312,  -312,  2283,  -312,    43,  -312,  2283,  -312,
    -312,  -312,  -312,  -312,  -312,  -312,  -312,    43,  -312,  -312,
      52,  -312,  -312,  -312,  -312,  -312,  -312,  -312,  -312,  -312,
     -52,  1971,  -312,  2127,  -312,    13,  -312,   303,  -312,  2283,
    2283,  2283,  2283,  2283,  2283,  -312,   -53,   -53,  -312,  -312,
     355,  -312,  -312,  -312,  -312,   -31,  -312,  -312,  -312,   -53,
    -312,  -312,  -312,  -312,   245,  -312,  -312,  -312,  -312,  -312,
      36,  -312,    33,   -25,    48,  1580,  -312,  -312,  -312,  -312,
    -312,  -312,  -312,  -312,  -312,  -312,  -312,  -312,  -312,  -312,
    2127,  -312,  2127,    16,  -312,    42,   922,   703,  -312,  -312,
    -312,     4,    24,  -312,  1433,  1433,   995,    35,  -312,  -312,
     616,  1433,  1433,  1433,  1433,  -312,    13,    40,  -312,  -312,
    -312,   157,  -312,  -312,    45,    34,    -7,    -5,    67,    53,
      69,    57,   133,   -35,  -312,    68,  -312,  -312,  -312,  -312,
    -312,  -312,  -312,  -312,   703,  -312,   442,   529,  -312,  -312,
      95,  -312,  -312,  -312,   162,  -312,   163,  -312,    43,  2205,
    -312,    99,  -312,    14,  1068,  -312,  -312,  -312,    14,   -32,
    -312,  -312,    14,    15,    -1,  -312,    96,   103,  -312,  1658,
    -312,  -312,  1736,  -312,  -312,  -312,    40,    39,   106,   127,
    1141,  1506,  1506,  -312,  -312,  -312,   108,  1506,   107,  -312,
     109,  -312,  -312,  -312,  -312,   113,  1506,  1506,  1506,  1506,
    1506,  1506,  -312,  -312,  1506,  1506,  1214,    14,  1506,  1506,
    1506,  1506,  1506,  1506,  1506,  1506,  1506,  1506,  1506,  1506,
    1506,  1506,  1506,  1506,  1506,  1506,  1506,  -312,   176,  -312,
    -312,   442,  -312,  -312,  -312,    14,  -312,  -312,  -312,    14,
    1814,  -312,  -312,  -312,   776,  -312,  -312,  -312,  -312,   114,
    -312,   124,   120,  2205,  -312,  -312,  2283,  -312,  -312,  -312,
    -312,   118,  -312,   129,   130,  -312,   126,  -312,   128,  1506,
    -312,     9,  -312,  -312,  -312,  -312,  -312,  -312,  -312,   125,
     131,   140,  -312,  -312,  -312,  -312,  -312,  -312,    45,    45,
      34,    34,    -7,    -7,    -7,    -7,    -5,    -5,    67,    53,
      69,    57,   133,   136,   703,  -312,  -312,  -312,   143,  -312,
    -312,  -312,  -312,    10,  1892,  -312,  1068,   -17,  -312,  -312,
    1506,  1287,  1506,  -312,   703,  -312,  1506,  -312,  -312,  -312,
    1506,  1506,  -312,  -312,  -312,   849,  -312,  -312,  -312,  -312,
     138,  -312,   141,  -312,   147,  -312,  -312,  -312,  -312,  -312,
    -312,  -312,  -312,  -312,   149,  1360,  -312,   142,   703,  -312,
    -312
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    54,    37,    55,     0,    59,    35,   241,    70,    60,
      72,    34,    61,    71,     0,    58,    82,    38,     0,   239,
      56,    57,    36,    39,    44,    43,    42,     0,    41,    40,
      45,   242,    62,    63,    64,    65,    66,    67,    68,    69,
       0,     0,     2,     0,     4,     0,    13,    15,    23,     0,
       0,     0,     0,     0,     0,    50,    75,     0,    51,     5,
       0,    53,     8,    11,    22,    77,    81,    80,    14,     0,
      48,    46,    49,    47,     0,     1,     3,     7,     6,     9,
       0,    30,    32,    96,    98,     0,   100,    29,    24,    26,
      25,    28,    27,    16,    18,    17,    20,    21,    19,   207,
       0,    76,     0,     0,   244,     0,     0,     0,   245,   246,
     247,     0,     0,   243,     0,     0,     0,     0,   214,   184,
       0,     0,     0,     0,     0,   211,     0,    23,   126,   123,
     130,   142,   149,   151,   155,   158,   161,   166,   169,   171,
     173,   175,   177,   179,   182,     0,   216,   212,   186,   187,
     189,   193,   192,   195,     0,   188,     0,     0,   209,   190,
       0,   191,   196,   194,     0,   125,   240,   127,     0,     0,
      88,     0,    10,     0,     0,   185,    92,    97,     0,     0,
     105,   115,     0,   106,     0,   109,     0,   114,    85,     0,
      83,    86,     0,   199,   200,   197,     0,   142,     0,     0,
       0,     0,     0,   144,   143,   235,     0,     0,     0,   106,
       0,   145,   146,   147,   148,    32,     0,     0,     0,     0,
       0,     0,   132,   131,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   202,   186,   208,
     206,     0,   183,   210,   213,     0,    78,    79,   238,     0,
       0,    89,    52,    31,     0,    33,   117,    94,    99,     0,
     102,   111,    12,     0,   103,   104,     0,    73,    84,    74,
     198,     0,   230,     0,   228,   227,     0,   234,     0,     0,
     128,     0,   140,   217,   218,   219,   220,   221,   215,     0,
       0,   137,   138,   133,   236,   152,   153,   154,   156,   157,
     160,   159,   165,   164,   162,   163,   167,   168,   170,   172,
     174,   176,   178,     0,     0,   205,   124,   240,     0,    87,
      90,   120,   121,     0,     0,   101,     0,     0,   110,   116,
       0,     0,     0,   204,     0,   150,     0,   129,   134,   135,
       0,     0,   201,   203,    91,     0,   118,    93,    95,   112,
       0,   108,     0,   232,     0,   231,   222,   225,   141,   139,
     180,   119,   122,   107,     0,     0,   223,     0,     0,   224,
     226
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -312,  -312,   191,  -312,     1,  -115,   -48,    12,  -312,  -312,
      72,     0,   193,   201,   203,   205,  -312,   206,  -312,   197,
    -312,   229,  -312,   155,  -153,  -312,  -312,  -312,    -2,  -312,
    -312,  -312,   -43,  -312,  -312,  -312,  -312,  -312,   -12,  -312,
    -312,  -244,  -312,  -312,     7,  -312,   -50,  -312,  -312,  -312,
      58,  -213,   -80,   -79,   -89,   -72,    23,    25,    22,    26,
      21,  -312,   -83,  -312,    17,    29,  -312,   -85,  -150,  -311,
    -312,  -312,  -312,  -312,  -312,  -312,   -29,    20,   116,  -146,
    -312,  -197,  -312,  -312,  -200,  -100,  -312,  -312,  -312,  -312,
    -312,   100,    -8,  -312,    32,  -312
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    41,    42,    43,   125,   181,    45,    46,    47,    80,
      81,   196,    49,    50,    51,    52,    53,    54,    55,   100,
      56,    65,    57,   189,   190,    58,   169,   260,   261,   177,
     267,   334,   215,    83,    84,    85,   272,   184,   185,   186,
     187,   265,   333,   128,   129,   130,   197,   300,   301,   291,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   191,    60,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,   250,   157,   158,
     159,   160,   161,   162,   286,   283,   284,   364,   163,   303,
     164,   256,    61,   165,   166,   167
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      48,    44,    82,   282,   248,   208,   269,   288,    66,   245,
     131,   253,   126,   353,    48,   305,   306,   307,    48,    66,
     332,   360,   199,   233,   235,    63,    64,   101,   101,    59,
      68,    74,    62,   367,   234,   236,   278,   182,    99,   278,
     170,    48,    44,    48,    77,     7,     7,   -13,    67,    48,
      48,    48,    48,    48,    48,   168,   270,   131,   176,    67,
     127,    93,    94,    95,    96,    97,    98,   380,  -181,   247,
      59,   361,    78,    62,   171,     7,   345,    86,   237,   238,
     273,   222,    31,    31,   -13,    48,   223,   239,   274,    70,
     346,   355,   359,    79,   200,   356,   193,   183,   347,    19,
      48,   188,    48,   188,   131,   253,   131,   131,   126,   126,
     240,   372,    31,    71,   201,   174,   172,   173,    72,    73,
     127,   259,   194,   198,   175,   207,   225,   231,   232,   226,
     216,   227,   209,   206,   178,   179,   244,   210,   180,   271,
     362,   363,   228,   229,   230,   365,   312,   313,   314,   315,
     131,   308,   309,   241,   310,   311,   127,   127,    86,   243,
     257,   339,   217,   218,   219,   220,   221,   316,   317,    48,
     242,   246,   203,   204,   352,   254,   255,  -237,   282,   211,
     212,   213,   214,   262,   276,   275,   280,   281,   287,    48,
     188,   266,    48,   188,   366,   174,   289,   324,   290,   222,
     258,   131,   335,   126,   223,    86,   336,   337,   340,   341,
     258,   342,   259,   348,    86,   343,   328,   344,   285,   210,
     349,   350,   351,   354,   285,   182,   373,   375,   379,   376,
     374,   378,    76,   292,   293,   294,   295,   296,   297,   224,
      88,   298,   299,   302,   225,   263,     1,   226,    89,   227,
      90,   127,    91,    92,   102,     2,    69,   192,   330,   304,
      48,   338,   326,   323,   318,   320,   322,   319,   370,     6,
     321,   325,   251,    48,   131,   377,    48,     0,   268,     0,
       0,   266,    11,     0,     0,     0,   126,   327,   209,     0,
       0,    86,   131,     0,   131,     0,    16,     0,    17,     0,
       0,    19,     0,     0,    22,     0,    23,    24,    25,    26,
      27,    28,    29,    30,     0,     0,     0,     0,     3,     0,
       0,     0,     0,    40,     0,   131,     5,     0,   131,     0,
       0,     0,     0,     0,    48,   358,     0,     8,     9,    10,
       0,     0,    12,     0,     0,     0,     0,     0,    13,    87,
       0,     0,    15,   266,     0,     0,     1,   285,   285,     0,
      20,    21,     0,   368,     0,     2,   103,   369,   104,     0,
       3,   105,   266,   106,   107,     0,     0,     4,     5,     6,
     108,   109,   110,   111,     0,     0,     0,     7,   112,     8,
       9,    10,    11,   113,    12,     0,     0,   114,     0,     0,
      13,    14,   115,   116,    15,     0,    16,     0,    17,     0,
      18,    19,    20,    21,    22,   117,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,     0,   118,     0,     0,     0,     0,
     119,     0,     0,     1,     0,   120,    99,     0,   121,   122,
     123,   124,     2,   103,     0,   104,     0,     3,   105,     0,
     106,   107,     0,     0,     4,     5,     6,   108,   109,   110,
     111,     0,     0,     0,     7,   112,     8,     9,    10,    11,
     113,    12,     0,     0,   114,     0,     0,    13,    14,   115,
     116,    15,     0,    16,     0,    17,     0,    18,    19,    20,
      21,    22,   117,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,     0,   118,     0,     0,     0,     0,   249,     0,     0,
       1,     0,   120,    99,     0,   121,   122,   123,   124,     2,
     103,     0,   104,     0,     3,   105,     0,   106,   107,     0,
       0,     4,     5,     6,   108,   109,   110,   111,     0,     0,
       0,     7,   112,     8,     9,    10,    11,   113,    12,     0,
       0,   114,     0,     0,    13,    14,   115,   116,    15,     0,
      16,     0,    17,     0,    18,    19,    20,    21,    22,   117,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,     0,   118,
       0,     0,     0,     0,   252,     0,     0,     1,     0,   120,
      99,     0,   121,   122,   123,   124,     2,     0,     0,   104,
       0,     3,     0,     0,     0,     0,     0,     0,     0,     5,
       6,   108,   109,   110,     0,     0,     0,     0,     7,     0,
       8,     9,    10,    11,   113,    12,     0,     0,   114,     0,
       0,    13,    14,   115,     0,    15,     0,    16,     0,    17,
       0,     0,    19,    20,    21,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     1,     0,   120,     0,     0,   121,
     122,   123,   124,     2,   103,     0,   104,     0,     0,   105,
       0,   106,   107,     0,     0,     0,     0,     6,   108,   109,
     110,   111,     0,     0,     0,     7,   112,     0,     0,     0,
      11,   113,     0,     0,     0,   114,     0,     0,     0,     0,
     115,   116,     0,     0,    16,     0,    17,     0,     0,    19,
       0,     0,    22,   117,    23,    24,    25,    26,    27,    28,
      29,    30,    31,     0,     0,     0,     0,     1,     0,     0,
       0,    40,     0,   118,     0,     0,     2,     0,     0,   104,
       0,     0,     0,   120,    99,     0,   121,   122,   123,   124,
       6,   108,   109,   110,     0,     0,     0,     0,     7,     0,
       0,     0,     0,    11,   113,     0,     0,     0,   114,     0,
       0,     0,     0,   115,     0,     0,     0,    16,     0,    17,
       0,     0,    19,     0,     0,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,     0,     0,     0,     0,
       1,     0,     0,     0,    40,     0,     0,     0,     0,     2,
       0,   331,   104,     0,     0,     0,   120,   264,     0,   121,
     122,   123,   124,     6,   108,   109,   110,     0,     0,     0,
       0,     7,     0,     0,     0,     0,    11,   113,     0,     0,
       0,   114,     0,     0,     0,     0,   115,     0,     0,     0,
      16,     0,    17,     0,     0,    19,     0,     0,    22,     0,
      23,    24,    25,    26,    27,    28,    29,    30,    31,     0,
       0,     0,     0,     1,     0,     0,     0,    40,     0,     0,
       0,     0,     2,     0,   371,   104,     0,     0,     0,   120,
     264,     0,   121,   122,   123,   124,     6,   108,   109,   110,
       0,     0,     0,     0,     7,     0,     0,     0,     0,    11,
     113,     0,     0,     0,   114,     0,     0,     0,     0,   115,
       0,     0,     0,    16,     0,    17,     0,     0,    19,     0,
       0,    22,     0,    23,    24,    25,    26,    27,    28,    29,
      30,    31,     0,     0,     0,     0,     1,     0,     0,     0,
      40,     0,   195,     0,     0,     2,     0,     0,   104,     0,
       0,     0,   120,     0,     0,   121,   122,   123,   124,     6,
     108,   109,   110,     0,     0,     0,     0,     7,     0,     0,
       0,     0,    11,   113,     0,     0,     0,   114,     0,     0,
       0,     0,   115,     0,     0,     0,    16,     0,    17,     0,
       0,    19,     0,     0,    22,     0,    23,    24,    25,    26,
      27,    28,    29,    30,    31,     0,     0,     0,     0,     1,
       0,     0,     0,    40,     0,   205,     0,     0,     2,     0,
       0,   104,     0,     0,     0,   120,     0,     0,   121,   122,
     123,   124,     6,   108,   109,   110,     0,     0,     0,     0,
       7,     0,     0,     0,     0,    11,   113,     0,     0,     0,
     114,     0,     0,     0,     0,   115,     0,     0,     0,    16,
       0,    17,     0,     0,    19,     0,     0,    22,     0,    23,
      24,    25,    26,    27,    28,    29,    30,    31,     0,     0,
       0,     0,     1,     0,     0,     0,    40,     0,     0,     0,
       0,     2,     0,     0,   104,     0,     0,     0,   120,   264,
       0,   121,   122,   123,   124,     6,   108,   109,   110,     0,
       0,     0,     0,     7,     0,     0,     0,     0,    11,   113,
       0,     0,     0,   114,     0,     0,     0,     0,   115,     0,
       0,     0,    16,     0,    17,     0,     0,    19,     0,     0,
      22,     0,    23,    24,    25,    26,    27,    28,    29,    30,
      31,     0,     0,     0,     0,     1,     0,     0,     0,    40,
       0,  -229,     0,     0,     2,     0,     0,   104,     0,     0,
       0,   120,     0,     0,   121,   122,   123,   124,     6,   108,
     109,   110,     0,     0,     0,     0,     7,     0,     0,     0,
       0,    11,   113,     0,     0,     0,   114,     0,     0,     0,
       0,   115,     0,     0,     0,    16,     0,    17,     0,     0,
      19,     0,     0,    22,     0,    23,    24,    25,    26,    27,
      28,    29,    30,    31,     0,     0,     0,     0,     1,     0,
       0,     0,    40,     0,     0,     0,     0,     2,     0,     0,
     104,     0,     0,  -136,   120,     0,     0,   121,   122,   123,
     124,     6,   108,   109,   110,     0,     0,     0,     0,     7,
       0,     0,     0,     0,    11,   113,     0,     0,     0,   114,
       0,     0,     0,     0,   115,     0,     0,     0,    16,     0,
      17,     0,     0,    19,     0,     0,    22,     0,    23,    24,
      25,    26,    27,    28,    29,    30,    31,     0,     0,     0,
       0,     1,     0,     0,     0,    40,     0,  -233,     0,     0,
       2,     0,     0,   104,     0,     0,     0,   120,     0,     0,
     121,   122,   123,   124,     6,   108,   109,   110,     0,     0,
       0,     0,     7,     0,     0,     0,     0,    11,   113,     0,
       0,     0,   114,     0,     0,     0,     0,   115,     0,     0,
       0,    16,     0,    17,     0,     0,    19,     0,     0,    22,
       0,    23,    24,    25,    26,    27,    28,    29,    30,    31,
       0,     0,     0,     0,     1,     0,     0,     0,    40,     0,
       0,     0,     0,     2,     0,     0,   104,     0,     0,  -229,
     120,     0,     0,   121,   122,   123,   124,     6,   108,   109,
     110,     0,     0,     0,     0,     7,     0,     0,     0,     0,
      11,   113,     0,     0,     0,   114,     0,     0,     0,     0,
     115,     0,     0,     0,    16,     0,    17,     0,     0,    19,
       0,     0,    22,     0,    23,    24,    25,    26,    27,    28,
      29,    30,    31,     0,     0,     0,     0,     1,     0,     0,
       0,    40,     0,     0,     0,     0,     2,     0,     0,   104,
       0,     0,     0,   202,     0,     0,   121,   122,   123,   124,
       6,   108,   109,   110,     0,     0,     0,     0,     7,     0,
       0,     0,     0,    11,   113,     0,     0,     0,   114,     0,
       0,     0,     0,   115,     0,     0,     0,    16,     0,    17,
       0,     0,    19,     0,     0,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,     0,     0,     0,     0,
       0,     1,     0,     0,    40,     0,     0,     0,     0,     0,
       2,     0,     0,     0,     0,     3,   120,     0,     0,   121,
     122,   123,   124,     5,     6,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     8,     9,    10,    11,     0,    12,
       0,     0,     0,     0,     0,    13,    14,     0,     0,    15,
       0,    16,     0,    17,     0,    18,    19,    20,    21,    22,
       0,    23,    24,    25,    26,    27,    28,    29,    30,     0,
      32,    33,    34,    35,    36,    37,    38,    39,    40,     1,
       0,     0,     0,     0,     0,     0,     0,     0,     2,  -113,
       0,     0,     0,     3,     0,     0,     0,     0,     0,     0,
       4,     5,     6,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     8,     9,    10,    11,     0,    12,     0,     0,
       0,     0,     0,    13,    14,     0,     0,    15,     0,    16,
       0,    17,     0,    18,    19,    20,    21,    22,     0,    23,
      24,    25,    26,    27,    28,    29,    30,     0,    32,    33,
      34,    35,    36,    37,    38,    39,    40,     1,     0,     0,
       0,     0,     0,   277,     0,     0,     2,     0,     0,     0,
       0,     3,     0,     0,     0,     0,     0,     0,     4,     5,
       6,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       8,     9,    10,    11,     0,    12,     0,     0,     0,     0,
       0,    13,    14,     0,     0,    15,     0,    16,     0,    17,
       0,    18,    19,    20,    21,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,     0,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     1,     0,     0,     0,     0,
       0,   279,     0,     0,     2,     0,     0,     0,     0,     3,
       0,     0,     0,     0,     0,     0,     0,     5,     6,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     8,     9,
      10,    11,     0,    12,     0,     0,     0,     0,     0,    13,
      14,     0,     0,    15,     0,    16,     0,    17,     0,    18,
      19,    20,    21,    22,     0,    23,    24,    25,    26,    27,
      28,    29,    30,     0,    32,    33,    34,    35,    36,    37,
      38,    39,    40,     1,     0,     0,     0,     0,     0,   329,
       0,     0,     2,     0,     0,     0,     0,     3,     0,     0,
       0,     0,     0,     0,     4,     5,     6,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
       0,    12,     0,     0,     0,     0,     0,    13,    14,     0,
       0,    15,     0,    16,     0,    17,     0,    18,    19,    20,
      21,    22,     0,    23,    24,    25,    26,    27,    28,    29,
      30,     0,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    75,     1,     0,     0,     0,   357,     0,     0,     0,
       0,     2,     0,     0,     0,     0,     3,     0,     0,     0,
       0,     0,     0,     4,     5,     6,     0,     0,     0,     0,
       0,     0,     0,     7,     0,     8,     9,    10,    11,     0,
      12,     0,     0,     0,     0,     0,    13,    14,     0,     0,
      15,     0,    16,     0,    17,     0,    18,    19,    20,    21,
      22,     0,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
       1,     0,     0,     0,     0,     0,     0,     0,     0,     2,
       0,     0,     0,     0,     3,     0,     0,     0,     0,     0,
       0,     4,     5,     6,     0,     0,     0,     0,     0,     0,
       0,     7,     0,     8,     9,    10,    11,     0,    12,     0,
       0,     0,     0,     0,    13,    14,     0,     0,    15,     0,
      16,     0,    17,     0,    18,    19,    20,    21,    22,     0,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,     1,     0,
       0,     0,     0,     0,     0,     0,     0,     2,     0,     0,
       0,     0,     3,     0,     0,     0,     0,     0,     0,     4,
       5,     6,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,    11,     0,    12,     0,     0,     0,
       0,     0,    13,    14,     0,     0,    15,     0,    16,     0,
      17,     0,    18,    19,    20,    21,    22,     0,    23,    24,
      25,    26,    27,    28,    29,    30,     0,    32,    33,    34,
      35,    36,    37,    38,    39,    40,     1,     0,     0,     0,
       0,     0,     0,     0,     0,     2,     0,     0,     0,     0,
       3,     0,     0,     0,     0,     0,     0,     0,     5,     6,
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
       0,     0,    15,     0,    16,     0,    17,     0,     0,    19,
      20,    21,    22,     0,    23,    24,    25,    26,    27,    28,
      29,    30,     0,    32,    33,    34,    35,    36,    37,    38,
      39,    40
};

static const yytype_int16 yycheck[] =
{
       0,     0,    45,   200,   154,   120,    38,   207,    16,    44,
      60,   157,    60,   324,    14,   228,   229,   230,    18,    27,
     264,    38,   107,    30,    29,    80,    14,    56,    57,     0,
      18,    83,     0,   344,    41,    40,   189,    85,    91,   192,
      69,    41,    41,    43,    43,    32,    32,    32,    16,    49,
      50,    51,    52,    53,    54,    86,    88,   107,    83,    27,
      60,    49,    50,    51,    52,    53,    54,   378,   103,   154,
      41,    88,    43,    41,    74,    32,   289,    45,    83,    84,
      81,    42,    69,    69,    69,    85,    47,    20,    89,    37,
      81,    81,   336,    80,    90,    85,    80,    85,    89,    56,
     100,   100,   102,   102,   154,   251,   156,   157,   156,   157,
      43,   355,    69,    61,    90,    82,    80,    81,    66,    67,
     120,   169,    80,   106,    91,    90,    87,    93,    94,    90,
      90,    92,   120,   116,    86,    87,     3,   120,    90,   182,
     340,   341,    97,    98,    99,   342,   235,   236,   237,   238,
     200,   231,   232,   100,   233,   234,   156,   157,   126,   102,
     168,   276,     5,     6,     7,     8,     9,   239,   240,   169,
     101,   103,   114,   115,   324,    80,    14,    14,   375,   121,
     122,   123,   124,    84,    81,    89,    80,    60,    80,   189,
     189,   174,   192,   192,   344,    82,    89,    21,    89,    42,
     168,   251,    88,   251,    47,   173,    82,    87,    90,    80,
     178,    81,   260,    88,   182,    89,   259,    89,   201,   202,
      89,    81,    86,    80,   207,   273,    88,    80,   378,    80,
      89,    89,    41,   216,   217,   218,   219,   220,   221,    82,
      47,   224,   225,   226,    87,   173,     1,    90,    47,    92,
      47,   251,    47,    47,    57,    10,    27,   102,   260,   227,
     260,   273,   255,   246,   241,   243,   245,   242,   351,    24,
     244,   251,   156,   273,   324,   375,   276,    -1,   178,    -1,
      -1,   264,    37,    -1,    -1,    -1,   334,   255,   276,    -1,
      -1,   259,   342,    -1,   344,    -1,    51,    -1,    53,    -1,
      -1,    56,    -1,    -1,    59,    -1,    61,    62,    63,    64,
      65,    66,    67,    68,    -1,    -1,    -1,    -1,    15,    -1,
      -1,    -1,    -1,    78,    -1,   375,    23,    -1,   378,    -1,
      -1,    -1,    -1,    -1,   334,   334,    -1,    34,    35,    36,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,
      -1,    -1,    49,   336,    -1,    -1,     1,   340,   341,    -1,
      57,    58,    -1,   346,    -1,    10,    11,   350,    13,    -1,
      15,    16,   355,    18,    19,    -1,    -1,    22,    23,    24,
      25,    26,    27,    28,    -1,    -1,    -1,    32,    33,    34,
      35,    36,    37,    38,    39,    -1,    -1,    42,    -1,    -1,
      45,    46,    47,    48,    49,    -1,    51,    -1,    53,    -1,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    -1,    80,    -1,    -1,    -1,    -1,
      85,    -1,    -1,     1,    -1,    90,    91,    -1,    93,    94,
      95,    96,    10,    11,    -1,    13,    -1,    15,    16,    -1,
      18,    19,    -1,    -1,    22,    23,    24,    25,    26,    27,
      28,    -1,    -1,    -1,    32,    33,    34,    35,    36,    37,
      38,    39,    -1,    -1,    42,    -1,    -1,    45,    46,    47,
      48,    49,    -1,    51,    -1,    53,    -1,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    -1,    80,    -1,    -1,    -1,    -1,    85,    -1,    -1,
       1,    -1,    90,    91,    -1,    93,    94,    95,    96,    10,
      11,    -1,    13,    -1,    15,    16,    -1,    18,    19,    -1,
      -1,    22,    23,    24,    25,    26,    27,    28,    -1,    -1,
      -1,    32,    33,    34,    35,    36,    37,    38,    39,    -1,
      -1,    42,    -1,    -1,    45,    46,    47,    48,    49,    -1,
      51,    -1,    53,    -1,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    -1,    80,
      -1,    -1,    -1,    -1,    85,    -1,    -1,     1,    -1,    90,
      91,    -1,    93,    94,    95,    96,    10,    -1,    -1,    13,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,
      24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,    -1,
      34,    35,    36,    37,    38,    39,    -1,    -1,    42,    -1,
      -1,    45,    46,    47,    -1,    49,    -1,    51,    -1,    53,
      -1,    -1,    56,    57,    58,    59,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     1,    -1,    90,    -1,    -1,    93,
      94,    95,    96,    10,    11,    -1,    13,    -1,    -1,    16,
      -1,    18,    19,    -1,    -1,    -1,    -1,    24,    25,    26,
      27,    28,    -1,    -1,    -1,    32,    33,    -1,    -1,    -1,
      37,    38,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,
      47,    48,    -1,    -1,    51,    -1,    53,    -1,    -1,    56,
      -1,    -1,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    -1,    -1,    -1,    -1,     1,    -1,    -1,
      -1,    78,    -1,    80,    -1,    -1,    10,    -1,    -1,    13,
      -1,    -1,    -1,    90,    91,    -1,    93,    94,    95,    96,
      24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,    -1,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,    53,
      -1,    -1,    56,    -1,    -1,    59,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    78,    -1,    -1,    -1,    -1,    10,
      -1,    85,    13,    -1,    -1,    -1,    90,    91,    -1,    93,
      94,    95,    96,    24,    25,    26,    27,    -1,    -1,    -1,
      -1,    32,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,
      -1,    42,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,
      51,    -1,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    -1,
      -1,    -1,    -1,     1,    -1,    -1,    -1,    78,    -1,    -1,
      -1,    -1,    10,    -1,    85,    13,    -1,    -1,    -1,    90,
      91,    -1,    93,    94,    95,    96,    24,    25,    26,    27,
      -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,
      38,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    47,
      -1,    -1,    -1,    51,    -1,    53,    -1,    -1,    56,    -1,
      -1,    59,    -1,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,
      78,    -1,    80,    -1,    -1,    10,    -1,    -1,    13,    -1,
      -1,    -1,    90,    -1,    -1,    93,    94,    95,    96,    24,
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
      -1,    10,    -1,    -1,    13,    -1,    -1,    -1,    90,    91,
      -1,    93,    94,    95,    96,    24,    25,    26,    27,    -1,
      -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    38,
      -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,
      59,    -1,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    78,
      -1,    80,    -1,    -1,    10,    -1,    -1,    13,    -1,    -1,
      -1,    90,    -1,    -1,    93,    94,    95,    96,    24,    25,
      26,    27,    -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    37,    38,    -1,    -1,    -1,    42,    -1,    -1,    -1,
      -1,    47,    -1,    -1,    -1,    51,    -1,    53,    -1,    -1,
      56,    -1,    -1,    59,    -1,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,    78,    -1,    -1,    -1,    -1,    10,    -1,    -1,
      13,    -1,    -1,    89,    90,    -1,    -1,    93,    94,    95,
      96,    24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,
      -1,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,
      -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,
      53,    -1,    -1,    56,    -1,    -1,    59,    -1,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    -1,    -1,    -1,
      -1,     1,    -1,    -1,    -1,    78,    -1,    80,    -1,    -1,
      10,    -1,    -1,    13,    -1,    -1,    -1,    90,    -1,    -1,
      93,    94,    95,    96,    24,    25,    26,    27,    -1,    -1,
      -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    38,    -1,
      -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    -1,    -1,
      -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,    59,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    78,    -1,
      -1,    -1,    -1,    10,    -1,    -1,    13,    -1,    -1,    89,
      90,    -1,    -1,    93,    94,    95,    96,    24,    25,    26,
      27,    -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,
      37,    38,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,
      47,    -1,    -1,    -1,    51,    -1,    53,    -1,    -1,    56,
      -1,    -1,    59,    -1,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    -1,    -1,    -1,    -1,     1,    -1,    -1,
      -1,    78,    -1,    -1,    -1,    -1,    10,    -1,    -1,    13,
      -1,    -1,    -1,    90,    -1,    -1,    93,    94,    95,    96,
      24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,    -1,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,    53,
      -1,    -1,    56,    -1,    -1,    59,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    -1,    -1,    -1,    -1,
      -1,     1,    -1,    -1,    78,    -1,    -1,    -1,    -1,    -1,
      10,    -1,    -1,    -1,    -1,    15,    90,    -1,    -1,    93,
      94,    95,    96,    23,    24,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    34,    35,    36,    37,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    46,    -1,    -1,    49,
      -1,    51,    -1,    53,    -1,    55,    56,    57,    58,    59,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,    -1,
      70,    71,    72,    73,    74,    75,    76,    77,    78,     1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    10,    89,
      -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,
      22,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    34,    35,    36,    37,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    46,    -1,    -1,    49,    -1,    51,
      -1,    53,    -1,    55,    56,    57,    58,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,    -1,    70,    71,
      72,    73,    74,    75,    76,    77,    78,     1,    -1,    -1,
      -1,    -1,    -1,    85,    -1,    -1,    10,    -1,    -1,    -1,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      34,    35,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    46,    -1,    -1,    49,    -1,    51,    -1,    53,
      -1,    55,    56,    57,    58,    59,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,    -1,    70,    71,    72,    73,
      74,    75,    76,    77,    78,     1,    -1,    -1,    -1,    -1,
      -1,    85,    -1,    -1,    10,    -1,    -1,    -1,    -1,    15,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    24,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    35,
      36,    37,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      46,    -1,    -1,    49,    -1,    51,    -1,    53,    -1,    55,
      56,    57,    58,    59,    -1,    61,    62,    63,    64,    65,
      66,    67,    68,    -1,    70,    71,    72,    73,    74,    75,
      76,    77,    78,     1,    -1,    -1,    -1,    -1,    -1,    85,
      -1,    -1,    10,    -1,    -1,    -1,    -1,    15,    -1,    -1,
      -1,    -1,    -1,    -1,    22,    23,    24,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,    37,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,    -1,
      -1,    49,    -1,    51,    -1,    53,    -1,    55,    56,    57,
      58,    59,    -1,    61,    62,    63,    64,    65,    66,    67,
      68,    -1,    70,    71,    72,    73,    74,    75,    76,    77,
      78,     0,     1,    -1,    -1,    -1,    84,    -1,    -1,    -1,
      -1,    10,    -1,    -1,    -1,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    -1,    22,    23,    24,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    32,    -1,    34,    35,    36,    37,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    46,    -1,    -1,
      49,    -1,    51,    -1,    53,    -1,    55,    56,    57,    58,
      59,    -1,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
       1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    10,
      -1,    -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,
      -1,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    32,    -1,    34,    35,    36,    37,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    46,    -1,    -1,    49,    -1,
      51,    -1,    53,    -1,    55,    56,    57,    58,    59,    -1,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    10,    -1,    -1,
      -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,
      23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    35,    36,    37,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    46,    -1,    -1,    49,    -1,    51,    -1,
      53,    -1,    55,    56,    57,    58,    59,    -1,    61,    62,
      63,    64,    65,    66,    67,    68,    -1,    70,    71,    72,
      73,    74,    75,    76,    77,    78,     1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    10,    -1,    -1,    -1,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    24,
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
      -1,    -1,    49,    -1,    51,    -1,    53,    -1,    -1,    56,
      57,    58,    59,    -1,    61,    62,    63,    64,    65,    66,
      67,    68,    -1,    70,    71,    72,    73,    74,    75,    76,
      77,    78
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
     117,   118,   119,   120,   121,   122,   124,   126,   129,   169,
     170,   196,   198,    80,   111,   125,   196,   198,   111,   125,
      37,    61,    66,    67,    83,     0,   106,   108,   169,    80,
     113,   114,   136,   137,   138,   139,   198,    46,   116,   117,
     118,   119,   121,   111,   111,   111,   111,   111,   111,    91,
     123,   180,   123,    11,    13,    16,    18,    19,    25,    26,
      27,    28,    33,    38,    42,    47,    48,    60,    80,    85,
      90,    93,    94,    95,    96,   108,   110,   115,   147,   148,
     149,   150,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   182,   183,   184,
     185,   186,   187,   192,   194,   197,   198,   199,    86,   130,
     180,   115,    80,    81,    82,    91,    83,   133,    86,    87,
      90,   109,   110,   111,   141,   142,   143,   144,   108,   127,
     128,   169,   127,    80,    80,    80,   115,   150,   168,   171,
      90,    90,    90,   154,   154,    80,   168,    90,   109,   111,
     168,   154,   154,   154,   154,   136,    90,     5,     6,     7,
       8,     9,    42,    47,    82,    87,    90,    92,    97,    98,
      99,    93,    94,    30,    41,    29,    40,    83,    84,    20,
      43,   100,   101,   102,     3,    44,   103,   171,   172,    85,
     181,   182,    85,   183,    80,    14,   195,   196,   198,   110,
     131,   132,    84,   114,    91,   145,   168,   134,   195,    38,
      88,   136,   140,    81,    89,    89,    81,    85,   128,    85,
      80,    60,   185,   189,   190,   168,   188,    80,   188,    89,
      89,   153,   168,   168,   168,   168,   168,   168,   168,   168,
     151,   152,   168,   193,   198,   155,   155,   155,   156,   156,
     157,   157,   158,   158,   158,   158,   159,   159,   160,   161,
     162,   163,   164,   168,    21,   181,   148,   198,   136,    85,
     132,    85,   145,   146,   135,    88,    82,    87,   142,   109,
      90,    80,    81,    89,    89,   155,    81,    89,    88,    89,
      81,    86,   172,   173,    80,    81,    85,    84,   108,   145,
      38,    88,   188,   188,   191,   185,   172,   173,   168,   168,
     166,    85,   145,    88,    89,    80,    80,   189,    89,   172,
     173
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
     153,   153,   154,   154,   154,   154,   154,   154,   154,   155,
     155,   156,   156,   156,   156,   157,   157,   157,   158,   158,
     158,   159,   159,   159,   159,   159,   160,   160,   160,   161,
     161,   162,   162,   163,   163,   164,   164,   165,   165,   166,
     166,   167,   168,   169,   169,   170,   171,   171,   172,   172,
     172,   172,   172,   172,   172,   173,   173,   174,   174,   175,
     175,   176,   177,   177,   178,   179,   179,   180,   181,   182,
     182,   183,   183,   184,   184,   185,   185,   185,   185,   185,
     185,   185,   186,   186,   186,   187,   187,   188,   189,   189,
     190,   190,   191,   191,   192,   192,   193,   194,   195,   196,
     197,   198,   198,   199,   199,   199,   199,   199
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
       1,     3,     1,     2,     2,     2,     2,     2,     2,     1,
       4,     1,     3,     3,     3,     1,     3,     3,     1,     3,
       3,     1,     3,     3,     3,     3,     1,     3,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       5,     1,     1,     3,     2,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     3,     2,
       2,     4,     2,     4,     4,     3,     2,     1,     1,     1,
       2,     1,     1,     2,     1,     3,     1,     3,     3,     3,
       3,     3,     5,     7,     9,     5,     9,     1,     1,     0,
       1,     3,     1,     0,     3,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1
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
#line 287 "parser.y"
                              { (yyval.dummy) = GlobalInitStatements(CurrentScope, (yyvsp[0].sc_stmt)); }
#line 2030 "parser.c"
    break;

  case 5: /* external_declaration: function_definition  */
#line 289 "parser.y"
                              { (yyval.dummy) = 0; }
#line 2036 "parser.c"
    break;

  case 6: /* external_declaration: profile_specifier function_definition  */
#line 291 "parser.y"
                              { (yyval.dummy) = 0; }
#line 2042 "parser.c"
    break;

  case 7: /* external_declaration: profile_specifier declaration  */
#line 293 "parser.y"
                              { (yyval.dummy) = GlobalInitStatements(CurrentScope, (yyvsp[0].sc_stmt)); ClearPendingProfileSpecifier(); }
#line 2048 "parser.c"
    break;

  case 8: /* profile_specifier: identifier  */
#line 303 "parser.y"
                              { (yyval.sc_ident) = (yyvsp[0].sc_ident); SetPendingProfileSpecifier(Cg->tokenLoc, (yyvsp[0].sc_ident)); }
#line 2054 "parser.c"
    break;

  case 9: /* declaration: declaration_specifiers ';'  */
#line 307 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2060 "parser.c"
    break;

  case 10: /* declaration: declaration_specifiers init_declarator_list ';'  */
#line 309 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[-1].sc_stmt); }
#line 2066 "parser.c"
    break;

  case 11: /* declaration: ERROR_SY ';'  */
#line 311 "parser.y"
                              { RecordErrorPos(Cg->tokenLoc);
                                ClearPendingGeometryModifiers();
                                (yyval.sc_stmt) = NULL; }
#line 2074 "parser.c"
    break;

  case 12: /* abstract_declaration: abstract_declaration_specifiers abstract_declarator  */
#line 317 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2080 "parser.c"
    break;

  case 13: /* declaration_specifiers: abstract_declaration_specifiers  */
#line 324 "parser.y"
                              { (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 2086 "parser.c"
    break;

  case 14: /* declaration_specifiers: TYPEDEF_SY abstract_declaration_specifiers  */
#line 326 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, TYPE_MISC_TYPEDEF); (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 2092 "parser.c"
    break;

  case 15: /* abstract_declaration_specifiers: abstract_declaration_specifiers2  */
#line 331 "parser.y"
                              { (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 2098 "parser.c"
    break;

  case 16: /* abstract_declaration_specifiers: type_qualifier abstract_declaration_specifiers  */
#line 333 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2104 "parser.c"
    break;

  case 17: /* abstract_declaration_specifiers: storage_class abstract_declaration_specifiers  */
#line 335 "parser.y"
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2110 "parser.c"
    break;

  case 18: /* abstract_declaration_specifiers: type_domain abstract_declaration_specifiers  */
#line 337 "parser.y"
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2116 "parser.c"
    break;

  case 19: /* abstract_declaration_specifiers: in_out abstract_declaration_specifiers  */
#line 339 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2122 "parser.c"
    break;

  case 20: /* abstract_declaration_specifiers: function_specifier abstract_declaration_specifiers  */
#line 341 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2128 "parser.c"
    break;

  case 21: /* abstract_declaration_specifiers: geometry_modifier abstract_declaration_specifiers  */
#line 346 "parser.y"
                              { (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2134 "parser.c"
    break;

  case 22: /* abstract_declaration_specifiers: PACKED_SY abstract_declaration_specifiers  */
#line 348 "parser.y"
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2140 "parser.c"
    break;

  case 23: /* abstract_declaration_specifiers2: type_specifier  */
#line 353 "parser.y"
                              { (yyval.sc_type) = *SetDType(&CurrentDeclTypeSpecs, (yyvsp[0].sc_ptype)); }
#line 2146 "parser.c"
    break;

  case 24: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 type_qualifier  */
#line 355 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2152 "parser.c"
    break;

  case 25: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 storage_class  */
#line 357 "parser.y"
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2158 "parser.c"
    break;

  case 26: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 type_domain  */
#line 359 "parser.y"
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2164 "parser.c"
    break;

  case 27: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 in_out  */
#line 361 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2170 "parser.c"
    break;

  case 28: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 function_specifier  */
#line 363 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2176 "parser.c"
    break;

  case 29: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 PACKED_SY  */
#line 365 "parser.y"
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2182 "parser.c"
    break;

  case 30: /* init_declarator_list: init_declarator  */
#line 369 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[0].sc_stmt); }
#line 2188 "parser.c"
    break;

  case 31: /* init_declarator_list: init_declarator_list ',' init_declarator  */
#line 371 "parser.y"
                              { (yyval.sc_stmt) = AddStmt((yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2194 "parser.c"
    break;

  case 32: /* init_declarator: declarator  */
#line 375 "parser.y"
                              { (yyval.sc_stmt) = Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_decl), NULL); }
#line 2200 "parser.c"
    break;

  case 33: /* init_declarator: declarator '=' initializer  */
#line 377 "parser.y"
                              { (yyval.sc_stmt) = Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[0].sc_expr)); }
#line 2206 "parser.c"
    break;

  case 34: /* type_specifier: INT_SY  */
#line 385 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, INT_SY); }
#line 2212 "parser.c"
    break;

  case 35: /* type_specifier: FLOAT_SY  */
#line 387 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, FLOAT_SY); }
#line 2218 "parser.c"
    break;

  case 36: /* type_specifier: VOID_SY  */
#line 389 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, VOID_SY); }
#line 2224 "parser.c"
    break;

  case 37: /* type_specifier: BOOLEAN_SY  */
#line 391 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, BOOLEAN_SY); }
#line 2230 "parser.c"
    break;

  case 38: /* type_specifier: TEXOBJ_SY  */
#line 393 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, TEXOBJ_SY); }
#line 2236 "parser.c"
    break;

  case 39: /* type_specifier: CHAR_SY  */
#line 395 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2242 "parser.c"
    break;

  case 40: /* type_specifier: SHORT_SY  */
#line 397 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2248 "parser.c"
    break;

  case 41: /* type_specifier: LONG_SY  */
#line 399 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2254 "parser.c"
    break;

  case 42: /* type_specifier: HALF_SY  */
#line 401 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2260 "parser.c"
    break;

  case 43: /* type_specifier: FIXED_SY  */
#line 403 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2266 "parser.c"
    break;

  case 44: /* type_specifier: DOUBLE_SY  */
#line 405 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2272 "parser.c"
    break;

  case 45: /* type_specifier: UNSIGNED_SY  */
#line 407 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2278 "parser.c"
    break;

  case 46: /* type_specifier: UNSIGNED_SY CHAR_SY  */
#line 409 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2284 "parser.c"
    break;

  case 47: /* type_specifier: UNSIGNED_SY SHORT_SY  */
#line 411 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2290 "parser.c"
    break;

  case 48: /* type_specifier: UNSIGNED_SY INT_SY  */
#line 413 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2296 "parser.c"
    break;

  case 49: /* type_specifier: UNSIGNED_SY LONG_SY  */
#line 415 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2302 "parser.c"
    break;

  case 50: /* type_specifier: struct_or_connector_specifier  */
#line 417 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2308 "parser.c"
    break;

  case 51: /* type_specifier: interface_specifier  */
#line 419 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2314 "parser.c"
    break;

  case 52: /* type_specifier: ATTRIBARRAY_SY '<' type_specifier '>'  */
#line 421 "parser.y"
                              { (yyval.sc_ptype) = SetAttribArrayType(Cg->tokenLoc, (yyvsp[-1].sc_ptype)); }
#line 2320 "parser.c"
    break;

  case 53: /* type_specifier: type_identifier  */
#line 423 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, (yyvsp[0].sc_ident)); }
#line 2326 "parser.c"
    break;

  case 54: /* type_specifier: error  */
#line 425 "parser.y"
                              {
                                ClearPendingGeometryModifiers();
                                SemanticParseError(Cg->tokenLoc, ERROR_S_TYPE_NAME_EXPECTED,
                                                   GetAtomString(atable, Cg->mostRecentToken /* yychar */));
                                (yyval.sc_ptype) = UndefinedType;
                              }
#line 2337 "parser.c"
    break;

  case 55: /* type_qualifier: CONST_SY  */
#line 438 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_CONST; }
#line 2343 "parser.c"
    break;

  case 56: /* type_domain: UNIFORM_SY  */
#line 446 "parser.y"
                              { (yyval.sc_int) = TYPE_DOMAIN_UNIFORM; }
#line 2349 "parser.c"
    break;

  case 57: /* type_domain: VARYING_SY  */
#line 448 "parser.y"
                              { (yyval.sc_int) = TYPE_DOMAIN_VARYING; }
#line 2355 "parser.c"
    break;

  case 58: /* storage_class: STATIC_SY  */
#line 456 "parser.y"
                              { (yyval.sc_int) = (int) SC_STATIC; }
#line 2361 "parser.c"
    break;

  case 59: /* storage_class: EXTERN_SY  */
#line 458 "parser.y"
                              { (yyval.sc_int) = (int) SC_EXTERN; }
#line 2367 "parser.c"
    break;

  case 60: /* function_specifier: INLINE_SY  */
#line 466 "parser.y"
                              { (yyval.sc_int) = TYPE_MISC_INLINE; }
#line 2373 "parser.c"
    break;

  case 61: /* function_specifier: INTERNAL_SY  */
#line 468 "parser.y"
                              { (yyval.sc_int) = TYPE_MISC_INTERNAL; }
#line 2379 "parser.c"
    break;

  case 62: /* geometry_modifier: POINT_SY  */
#line 481 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_POINT); }
#line 2385 "parser.c"
    break;

  case 63: /* geometry_modifier: LINE_SY  */
#line 482 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_LINE); }
#line 2391 "parser.c"
    break;

  case 64: /* geometry_modifier: LINE_ADJ_SY  */
#line 483 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_LINE_ADJACENCY); }
#line 2397 "parser.c"
    break;

  case 65: /* geometry_modifier: TRIANGLE_SY  */
#line 484 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_TRIANGLE); }
#line 2403 "parser.c"
    break;

  case 66: /* geometry_modifier: TRIANGLE_ADJ_SY  */
#line 485 "parser.y"
                          { (yyval.sc_int) = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY); }
#line 2409 "parser.c"
    break;

  case 67: /* geometry_modifier: POINT_OUT_SY  */
#line 486 "parser.y"
                          { (yyval.sc_int) = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_POINTS); }
#line 2415 "parser.c"
    break;

  case 68: /* geometry_modifier: LINE_OUT_SY  */
#line 487 "parser.y"
                          { (yyval.sc_int) = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_LINE_STRIP); }
#line 2421 "parser.c"
    break;

  case 69: /* geometry_modifier: TRIANGLE_OUT_SY  */
#line 488 "parser.y"
                          { (yyval.sc_int) = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP); }
#line 2427 "parser.c"
    break;

  case 70: /* in_out: IN_SY  */
#line 496 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_IN; }
#line 2433 "parser.c"
    break;

  case 71: /* in_out: OUT_SY  */
#line 498 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_OUT; }
#line 2439 "parser.c"
    break;

  case 72: /* in_out: INOUT_SY  */
#line 500 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_INOUT; }
#line 2445 "parser.c"
    break;

  case 73: /* struct_or_connector_specifier: struct_or_connector_header struct_compound_header struct_declaration_list '}'  */
#line 509 "parser.y"
                              { (yyval.sc_ptype) = SetStructMembers(Cg->tokenLoc, (yyvsp[-3].sc_ptype), PopScope());
                                CheckInterfaceConformance(Cg->tokenLoc, (yyval.sc_ptype)); }
#line 2452 "parser.c"
    break;

  case 74: /* struct_or_connector_specifier: untagged_struct_header struct_compound_header struct_declaration_list '}'  */
#line 512 "parser.y"
                              { (yyval.sc_ptype) = SetStructMembers(Cg->tokenLoc, (yyvsp[-3].sc_ptype), PopScope());
                                CheckInterfaceConformance(Cg->tokenLoc, (yyval.sc_ptype)); }
#line 2459 "parser.c"
    break;

  case 75: /* struct_or_connector_specifier: struct_or_connector_header  */
#line 515 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2465 "parser.c"
    break;

  case 76: /* struct_compound_header: compound_header  */
#line 519 "parser.y"
                              { CurrentScope->IsStructScope = 1; (yyval.dummy) = (yyvsp[0].dummy); }
#line 2471 "parser.c"
    break;

  case 77: /* struct_or_connector_header: STRUCT_SY struct_identifier  */
#line 524 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, 0, (yyvsp[0].sc_ident)); }
#line 2477 "parser.c"
    break;

  case 78: /* struct_or_connector_header: STRUCT_SY struct_identifier ':' semantics_identifier  */
#line 526 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_ident), (yyvsp[-2].sc_ident)); }
#line 2483 "parser.c"
    break;

  case 79: /* struct_or_connector_header: STRUCT_SY struct_identifier ':' type_identifier  */
#line 528 "parser.y"
                              { (yyval.sc_ptype) = SetStructInterface(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_ident), (yyvsp[0].sc_ident)); }
#line 2489 "parser.c"
    break;

  case 82: /* untagged_struct_header: STRUCT_SY  */
#line 536 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, 0, 0); }
#line 2495 "parser.c"
    break;

  case 85: /* struct_declaration: declaration  */
#line 544 "parser.y"
                            { (yyval.sc_stmt) = (yyvsp[0].sc_stmt); }
#line 2501 "parser.c"
    break;

  case 86: /* struct_declaration: function_definition  */
#line 546 "parser.y"
                            { (yyval.sc_stmt) = NULL; }
#line 2507 "parser.c"
    break;

  case 87: /* interface_specifier: INTERFACE_SY struct_identifier interface_compound_header interface_member_declaration_list '}'  */
#line 557 "parser.y"
                              { (yyval.sc_ptype) = SetInterfaceMembers(Cg->tokenLoc,
                                                         InterfaceHeader(Cg->tokenLoc, CurrentScope, (yyvsp[-3].sc_ident)),
                                                         PopScope()); }
#line 2515 "parser.c"
    break;

  case 88: /* interface_compound_header: compound_header  */
#line 564 "parser.y"
                              { CurrentScope->IsStructScope = 1; (yyval.dummy) = (yyvsp[0].dummy); }
#line 2521 "parser.c"
    break;

  case 91: /* interface_member_declaration: declaration_specifiers declarator ';'  */
#line 581 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2527 "parser.c"
    break;

  case 92: /* $@1: %empty  */
#line 599 "parser.y"
                              { PushScope(NewScope()); }
#line 2533 "parser.c"
    break;

  case 93: /* annotation: '<' $@1 annotation_decl_list '>'  */
#line 600 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[-1].sc_stmt); PopScope(); }
#line 2539 "parser.c"
    break;

  case 94: /* annotation_decl_list: %empty  */
#line 604 "parser.y"
                              { (yyval.sc_stmt) = 0; }
#line 2545 "parser.c"
    break;

  case 96: /* declarator: semantic_declarator  */
#line 613 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2551 "parser.c"
    break;

  case 97: /* declarator: semantic_declarator annotation  */
#line 615 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[-1].sc_decl); }
#line 2557 "parser.c"
    break;

  case 98: /* semantic_declarator: basic_declarator  */
#line 619 "parser.y"
                              { (yyval.sc_decl) = Declarator(Cg->tokenLoc, (yyvsp[0].sc_decl), 0); }
#line 2563 "parser.c"
    break;

  case 99: /* semantic_declarator: basic_declarator ':' semantics_identifier  */
#line 621 "parser.y"
                              { (yyval.sc_decl) = Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), (yyvsp[0].sc_ident)); }
#line 2569 "parser.c"
    break;

  case 100: /* basic_declarator: identifier  */
#line 625 "parser.y"
                              { (yyval.sc_decl) = NewDeclNode(Cg->tokenLoc, (yyvsp[0].sc_ident), &CurrentDeclTypeSpecs); }
#line 2575 "parser.c"
    break;

  case 101: /* basic_declarator: basic_declarator '[' INTCONST_SY ']'  */
#line 627 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-3].sc_decl), (int) (yyvsp[-1].sc_literal).value.i, 0); }
#line 2581 "parser.c"
    break;

  case 102: /* basic_declarator: basic_declarator '[' ']'  */
#line 629 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), 0 , 1); }
#line 2587 "parser.c"
    break;

  case 103: /* basic_declarator: function_decl_header parameter_list ')'  */
#line 631 "parser.y"
                              { (yyval.sc_decl) = SetFunTypeParams(CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_decl), (yyvsp[-1].sc_decl)); }
#line 2593 "parser.c"
    break;

  case 104: /* basic_declarator: function_decl_header abstract_parameter_list ')'  */
#line 633 "parser.y"
                              { (yyval.sc_decl) = SetFunTypeParams(CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_decl), NULL); }
#line 2599 "parser.c"
    break;

  case 105: /* function_decl_header: basic_declarator '('  */
#line 637 "parser.y"
                              { (yyval.sc_decl) = FunctionDeclHeader(&(yyvsp[-1].sc_decl)->loc, CurrentScope, (yyvsp[-1].sc_decl)); }
#line 2605 "parser.c"
    break;

  case 106: /* abstract_declarator: %empty  */
#line 641 "parser.y"
                              { (yyval.sc_decl) = NewDeclNode(Cg->tokenLoc, 0, &CurrentDeclTypeSpecs); }
#line 2611 "parser.c"
    break;

  case 107: /* abstract_declarator: abstract_declarator '[' INTCONST_SY ']'  */
#line 643 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-3].sc_decl), (int) (yyvsp[-1].sc_literal).value.i, 0); }
#line 2617 "parser.c"
    break;

  case 108: /* abstract_declarator: abstract_declarator '[' ']'  */
#line 645 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), 0 , 1); }
#line 2623 "parser.c"
    break;

  case 109: /* parameter_list: parameter_declaration  */
#line 662 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2629 "parser.c"
    break;

  case 110: /* parameter_list: parameter_list ',' parameter_declaration  */
#line 664 "parser.y"
                              { (yyval.sc_decl) = AddDecl((yyvsp[-2].sc_decl), (yyvsp[0].sc_decl)); }
#line 2635 "parser.c"
    break;

  case 111: /* parameter_declaration: declaration_specifiers declarator  */
#line 668 "parser.y"
                              { (yyval.sc_decl) = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_decl), NULL); }
#line 2641 "parser.c"
    break;

  case 112: /* parameter_declaration: declaration_specifiers declarator '=' initializer  */
#line 670 "parser.y"
                              { (yyval.sc_decl) = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[0].sc_expr)); }
#line 2647 "parser.c"
    break;

  case 113: /* abstract_parameter_list: %empty  */
#line 674 "parser.y"
                              { (yyval.sc_decl) = NULL; }
#line 2653 "parser.c"
    break;

  case 115: /* non_empty_abstract_parameter_list: abstract_declaration  */
#line 679 "parser.y"
                              {
                                if (IsVoid(&(yyvsp[0].sc_decl)->type.type))
                                    CurrentScope->HasVoidParameter = 1;
                                (yyval.sc_decl) = (yyvsp[0].sc_decl);
                              }
#line 2663 "parser.c"
    break;

  case 116: /* non_empty_abstract_parameter_list: non_empty_abstract_parameter_list ',' abstract_declaration  */
#line 685 "parser.y"
                              {
                                if (CurrentScope->HasVoidParameter || IsVoid(&(yyvsp[-2].sc_decl)->type.type)) {
                                    SemanticError(Cg->tokenLoc, ERROR___VOID_NOT_ONLY_PARAM);
                                }
                                (yyval.sc_decl) = AddDecl((yyvsp[-2].sc_decl), (yyvsp[0].sc_decl));
                              }
#line 2674 "parser.c"
    break;

  case 117: /* initializer: expression  */
#line 698 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[0].sc_expr)); }
#line 2680 "parser.c"
    break;

  case 118: /* initializer: '{' initializer_list '}'  */
#line 700 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[-1].sc_expr)); }
#line 2686 "parser.c"
    break;

  case 119: /* initializer: '{' initializer_list ',' '}'  */
#line 702 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[-2].sc_expr)); }
#line 2692 "parser.c"
    break;

  case 120: /* initializer: '{' '}'  */
#line 708 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, NULL); }
#line 2698 "parser.c"
    break;

  case 121: /* initializer_list: initializer  */
#line 712 "parser.y"
                              { (yyval.sc_expr) = InitializerList(Cg->tokenLoc, (yyvsp[0].sc_expr), NULL); }
#line 2704 "parser.c"
    break;

  case 122: /* initializer_list: initializer_list ',' initializer  */
#line 714 "parser.y"
                              { (yyval.sc_expr) = InitializerList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2710 "parser.c"
    break;

  case 123: /* variable: basic_variable  */
#line 726 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[0].sc_expr); }
#line 2716 "parser.c"
    break;

  case 124: /* variable: scope_identifier COLONCOLON_SY basic_variable  */
#line 728 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[0].sc_expr); }
#line 2722 "parser.c"
    break;

  case 125: /* basic_variable: variable_identifier  */
#line 732 "parser.y"
                              { (yyval.sc_expr) = BasicVariable(Cg->tokenLoc, (yyvsp[0].sc_ident)); }
#line 2728 "parser.c"
    break;

  case 128: /* primary_expression: '(' expression ')'  */
#line 742 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[-1].sc_expr); }
#line 2734 "parser.c"
    break;

  case 129: /* primary_expression: type_specifier '(' expression_list ')'  */
#line 744 "parser.y"
                              { (yyval.sc_expr) = NewVectorConstructor(Cg->tokenLoc, (yyvsp[-3].sc_ptype), (yyvsp[-1].sc_expr)); }
#line 2740 "parser.c"
    break;

  case 131: /* postfix_expression: postfix_expression PLUSPLUS_SY  */
#line 753 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(POSTINC_OP, (yyvsp[-1].sc_expr)); }
#line 2746 "parser.c"
    break;

  case 132: /* postfix_expression: postfix_expression MINUSMINUS_SY  */
#line 755 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(POSTDEC_OP, (yyvsp[-1].sc_expr)); }
#line 2752 "parser.c"
    break;

  case 133: /* postfix_expression: postfix_expression '.' member_identifier  */
#line 757 "parser.y"
                              { (yyval.sc_expr) = NewMemberSelectorOrSwizzleOrWriteMaskOperator(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_ident)); }
#line 2758 "parser.c"
    break;

  case 134: /* postfix_expression: postfix_expression '[' expression ']'  */
#line 759 "parser.y"
                              { (yyval.sc_expr) = NewIndexOperator(Cg->tokenLoc, (yyvsp[-3].sc_expr), (yyvsp[-1].sc_expr)); }
#line 2764 "parser.c"
    break;

  case 135: /* postfix_expression: postfix_expression '(' actual_argument_list ')'  */
#line 761 "parser.y"
                              { (yyval.sc_expr) = NewFunctionCallOperator(Cg->tokenLoc, (yyvsp[-3].sc_expr), (yyvsp[-1].sc_expr)); }
#line 2770 "parser.c"
    break;

  case 136: /* actual_argument_list: %empty  */
#line 765 "parser.y"
                                { (yyval.sc_expr) = NULL; }
#line 2776 "parser.c"
    break;

  case 138: /* non_empty_argument_list: expression  */
#line 770 "parser.y"
                              { (yyval.sc_expr) = ArgumentList(Cg->tokenLoc, NULL, (yyvsp[0].sc_expr)); }
#line 2782 "parser.c"
    break;

  case 139: /* non_empty_argument_list: non_empty_argument_list ',' expression  */
#line 772 "parser.y"
                              { (yyval.sc_expr) = ArgumentList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2788 "parser.c"
    break;

  case 140: /* expression_list: expression  */
#line 776 "parser.y"
                              { (yyval.sc_expr) = ExpressionList(Cg->tokenLoc, NULL, (yyvsp[0].sc_expr)); }
#line 2794 "parser.c"
    break;

  case 141: /* expression_list: expression_list ',' expression  */
#line 778 "parser.y"
                              { (yyval.sc_expr) = ExpressionList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2800 "parser.c"
    break;

  case 143: /* unary_expression: PLUSPLUS_SY unary_expression  */
#line 787 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(PREINC_OP, (yyvsp[0].sc_expr)); }
#line 2806 "parser.c"
    break;

  case 144: /* unary_expression: MINUSMINUS_SY unary_expression  */
#line 789 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(PREDEC_OP, (yyvsp[0].sc_expr)); }
#line 2812 "parser.c"
    break;

  case 145: /* unary_expression: '+' unary_expression  */
#line 791 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, POS_OP, '+', (yyvsp[0].sc_expr), 0); }
#line 2818 "parser.c"
    break;

  case 146: /* unary_expression: '-' unary_expression  */
#line 793 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, NEG_OP, '-', (yyvsp[0].sc_expr), 0); }
#line 2824 "parser.c"
    break;

  case 147: /* unary_expression: '!' unary_expression  */
#line 795 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, BNOT_OP, '!', (yyvsp[0].sc_expr), 0); }
#line 2830 "parser.c"
    break;

  case 148: /* unary_expression: '~' unary_expression  */
#line 797 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, NOT_OP, '~', (yyvsp[0].sc_expr), 1); }
#line 2836 "parser.c"
    break;

  case 150: /* cast_expression: '(' abstract_declaration ')' cast_expression  */
#line 809 "parser.y"
                              { (yyval.sc_expr) = NewCastOperator(Cg->tokenLoc, (yyvsp[0].sc_expr), GetTypePointer(&(yyvsp[-2].sc_decl)->loc, &(yyvsp[-2].sc_decl)->type)); }
#line 2842 "parser.c"
    break;

  case 152: /* multiplicative_expression: multiplicative_expression '*' cast_expression  */
#line 818 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, MUL_OP, '*', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2848 "parser.c"
    break;

  case 153: /* multiplicative_expression: multiplicative_expression '/' cast_expression  */
#line 820 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, DIV_OP, '/', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2854 "parser.c"
    break;

  case 154: /* multiplicative_expression: multiplicative_expression '%' cast_expression  */
#line 822 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, MOD_OP, '%', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2860 "parser.c"
    break;

  case 156: /* additive_expression: additive_expression '+' multiplicative_expression  */
#line 831 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, ADD_OP, '+', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2866 "parser.c"
    break;

  case 157: /* additive_expression: additive_expression '-' multiplicative_expression  */
#line 833 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SUB_OP, '-', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2872 "parser.c"
    break;

  case 159: /* shift_expression: shift_expression LL_SY additive_expression  */
#line 842 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SHL_OP, LL_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2878 "parser.c"
    break;

  case 160: /* shift_expression: shift_expression GG_SY additive_expression  */
#line 844 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SHR_OP, GG_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2884 "parser.c"
    break;

  case 162: /* relational_expression: relational_expression '<' shift_expression  */
#line 853 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, LT_OP, '<', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2890 "parser.c"
    break;

  case 163: /* relational_expression: relational_expression '>' shift_expression  */
#line 855 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, GT_OP, '>', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2896 "parser.c"
    break;

  case 164: /* relational_expression: relational_expression LE_SY shift_expression  */
#line 857 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, LE_OP, LE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2902 "parser.c"
    break;

  case 165: /* relational_expression: relational_expression GE_SY shift_expression  */
#line 859 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, GE_OP, GE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2908 "parser.c"
    break;

  case 167: /* equality_expression: equality_expression EQ_SY relational_expression  */
#line 868 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, EQ_OP, EQ_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2914 "parser.c"
    break;

  case 168: /* equality_expression: equality_expression NE_SY relational_expression  */
#line 870 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, NE_OP, NE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2920 "parser.c"
    break;

  case 170: /* AND_expression: AND_expression '&' equality_expression  */
#line 879 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, AND_OP, '&', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2926 "parser.c"
    break;

  case 172: /* exclusive_OR_expression: exclusive_OR_expression '^' AND_expression  */
#line 888 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, XOR_OP, '^', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2932 "parser.c"
    break;

  case 174: /* inclusive_OR_expression: inclusive_OR_expression '|' exclusive_OR_expression  */
#line 897 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, OR_OP, '|', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2938 "parser.c"
    break;

  case 176: /* logical_AND_expression: logical_AND_expression AND_SY inclusive_OR_expression  */
#line 906 "parser.y"
                              { (yyval.sc_expr) = NewBinaryBooleanOperator(Cg->tokenLoc, BAND_OP, AND_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2944 "parser.c"
    break;

  case 178: /* logical_OR_expression: logical_OR_expression OR_SY logical_AND_expression  */
#line 915 "parser.y"
                              { (yyval.sc_expr) = NewBinaryBooleanOperator(Cg->tokenLoc, BOR_OP, OR_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2950 "parser.c"
    break;

  case 180: /* conditional_expression: conditional_test '?' expression ':' conditional_expression  */
#line 924 "parser.y"
                              { (yyval.sc_expr) = NewConditionalOperator(Cg->tokenLoc, (yyvsp[-4].sc_expr), (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2956 "parser.c"
    break;

  case 181: /* conditional_test: logical_OR_expression  */
#line 928 "parser.y"
                              {  (yyval.sc_expr) = CheckBooleanExpr(Cg->tokenLoc, (yyvsp[0].sc_expr), 1); }
#line 2962 "parser.c"
    break;

  case 183: /* function_definition: function_definition_header block_item_list '}'  */
#line 947 "parser.y"
                              { DefineFunction(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_stmt)); PopScope();
                                ResumeStructScopeAfterMethodBody(); }
#line 2969 "parser.c"
    break;

  case 184: /* function_definition: function_definition_header '}'  */
#line 950 "parser.y"
                              { DefineFunction(Cg->tokenLoc, CurrentScope, (yyvsp[-1].sc_decl), NULL); PopScope();
                                ResumeStructScopeAfterMethodBody(); }
#line 2976 "parser.c"
    break;

  case 185: /* function_definition_header: declaration_specifiers declarator '{'  */
#line 955 "parser.y"
                              { (yyval.sc_decl) = Function_Definition_Header(Cg->tokenLoc, (yyvsp[-1].sc_decl)); }
#line 2982 "parser.c"
    break;

  case 197: /* discard_statement: DISCARD_SY ';'  */
#line 984 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewDiscardStmt(Cg->tokenLoc, NULL); }
#line 2988 "parser.c"
    break;

  case 198: /* discard_statement: DISCARD_SY expression ';'  */
#line 986 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewDiscardStmt(Cg->tokenLoc, CheckBooleanExpr(Cg->tokenLoc, (yyvsp[-1].sc_expr), 1)); }
#line 2994 "parser.c"
    break;

  case 199: /* jump_statement: BREAK_SY ';'  */
#line 994 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewSimpleStmt(Cg->tokenLoc, BREAK_STMT); }
#line 3000 "parser.c"
    break;

  case 200: /* jump_statement: CONTINUE_SY ';'  */
#line 996 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewSimpleStmt(Cg->tokenLoc, CONTINUE_STMT); }
#line 3006 "parser.c"
    break;

  case 201: /* if_statement: if_header balanced_statement ELSE_SY balanced_statement  */
#line 1004 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-3].sc_stmt), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3012 "parser.c"
    break;

  case 202: /* dangling_if: if_header statement  */
#line 1008 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-1].sc_stmt), (yyvsp[0].sc_stmt), NULL); }
#line 3018 "parser.c"
    break;

  case 203: /* dangling_if: if_header balanced_statement ELSE_SY dangling_statement  */
#line 1010 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-3].sc_stmt), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3024 "parser.c"
    break;

  case 204: /* if_header: IF_SY '(' boolean_scalar_expression ')'  */
#line 1014 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewIfStmt(Cg->tokenLoc, (yyvsp[-1].sc_expr), NULL, NULL); ; }
#line 3030 "parser.c"
    break;

  case 205: /* compound_statement: compound_header block_item_list compound_tail  */
#line 1022 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewBlockStmt(Cg->tokenLoc, (yyvsp[-1].sc_stmt)); }
#line 3036 "parser.c"
    break;

  case 206: /* compound_statement: compound_header compound_tail  */
#line 1024 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 3042 "parser.c"
    break;

  case 207: /* compound_header: '{'  */
#line 1028 "parser.y"
                              { PushScope(NewScope()); CurrentScope->funindex = NextFunctionIndex; }
#line 3048 "parser.c"
    break;

  case 208: /* compound_tail: '}'  */
#line 1032 "parser.y"
                              {
                                if (Cg->options.DumpParseTree)
                                    PrintScopeDeclarations();
                                PopScope();
                              }
#line 3058 "parser.c"
    break;

  case 210: /* block_item_list: block_item_list block_item  */
#line 1041 "parser.y"
                              { (yyval.sc_stmt) = AddStmt((yyvsp[-1].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3064 "parser.c"
    break;

  case 212: /* block_item: statement  */
#line 1046 "parser.y"
                              { (yyval.sc_stmt) = CheckStmt((yyvsp[0].sc_stmt)); }
#line 3070 "parser.c"
    break;

  case 214: /* expression_statement: ';'  */
#line 1055 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 3076 "parser.c"
    break;

  case 215: /* expression_statement2: postfix_expression '=' expression  */
#line 1059 "parser.y"
                              { (yyval.sc_stmt) = NewSimpleAssignmentStmt(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 3082 "parser.c"
    break;

  case 216: /* expression_statement2: expression  */
#line 1061 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewExprStmt(Cg->tokenLoc, (yyvsp[0].sc_expr)); }
#line 3088 "parser.c"
    break;

  case 217: /* expression_statement2: postfix_expression ASSIGNMINUS_SY expression  */
#line 1063 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMINUS_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3094 "parser.c"
    break;

  case 218: /* expression_statement2: postfix_expression ASSIGNMOD_SY expression  */
#line 1065 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMOD_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3100 "parser.c"
    break;

  case 219: /* expression_statement2: postfix_expression ASSIGNPLUS_SY expression  */
#line 1067 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNPLUS_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3106 "parser.c"
    break;

  case 220: /* expression_statement2: postfix_expression ASSIGNSLASH_SY expression  */
#line 1069 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSLASH_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3112 "parser.c"
    break;

  case 221: /* expression_statement2: postfix_expression ASSIGNSTAR_SY expression  */
#line 1071 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSTAR_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 3118 "parser.c"
    break;

  case 222: /* iteration_statement: WHILE_SY '(' boolean_scalar_expression ')' balanced_statement  */
#line 1079 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, (yyvsp[-2].sc_expr), (yyvsp[0].sc_stmt)); }
#line 3124 "parser.c"
    break;

  case 223: /* iteration_statement: DO_SY statement WHILE_SY '(' boolean_scalar_expression ')' ';'  */
#line 1081 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, DO_STMT, (yyvsp[-2].sc_expr), (yyvsp[-5].sc_stmt)); }
#line 3130 "parser.c"
    break;

  case 224: /* iteration_statement: FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' balanced_statement  */
#line 1083 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewForStmt(Cg->tokenLoc, (yyvsp[-6].sc_stmt), (yyvsp[-4].sc_expr), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3136 "parser.c"
    break;

  case 225: /* dangling_iteration: WHILE_SY '(' boolean_scalar_expression ')' dangling_statement  */
#line 1087 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, (yyvsp[-2].sc_expr), (yyvsp[0].sc_stmt)); }
#line 3142 "parser.c"
    break;

  case 226: /* dangling_iteration: FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' dangling_statement  */
#line 1089 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewForStmt(Cg->tokenLoc, (yyvsp[-6].sc_stmt), (yyvsp[-4].sc_expr), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3148 "parser.c"
    break;

  case 227: /* boolean_scalar_expression: expression  */
#line 1094 "parser.y"
                              {  (yyval.sc_expr) = CheckBooleanExpr(Cg->tokenLoc, (yyvsp[0].sc_expr), 0); }
#line 3154 "parser.c"
    break;

  case 229: /* for_expression_opt: %empty  */
#line 1099 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 3160 "parser.c"
    break;

  case 231: /* for_expression: for_expression ',' expression_statement2  */
#line 1104 "parser.y"
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
#line 3176 "parser.c"
    break;

  case 233: /* boolean_expression_opt: %empty  */
#line 1119 "parser.y"
                              { (yyval.sc_expr) = NULL; }
#line 3182 "parser.c"
    break;

  case 234: /* return_statement: RETURN_SY expression ';'  */
#line 1127 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, (yyvsp[-1].sc_expr)); }
#line 3188 "parser.c"
    break;

  case 235: /* return_statement: RETURN_SY ';'  */
#line 1129 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, NULL); }
#line 3194 "parser.c"
    break;

  case 241: /* identifier: IDENT_SY  */
#line 1152 "parser.y"
                              { (yyval.sc_ident) = (yyvsp[0].sc_ident); }
#line 3200 "parser.c"
    break;

  case 242: /* identifier: RESERVED_SY  */
#line 1154 "parser.y"
                              {
                                /* SemanticError, not SemanticParseError: the
                                 * latter is gated by AllowSemanticParseErrors */
                                SemanticError(Cg->tokenLoc, ERROR_S_RESERVED_WORD,
                                              GetAtomString(atable, (yyvsp[0].sc_token)));
                                (yyval.sc_ident) = (yyvsp[0].sc_token);
                              }
#line 3212 "parser.c"
    break;

  case 243: /* constant: INTCONST_SY  */
#line 1164 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(ICONST_OP, &(yyvsp[0].sc_literal)); }
#line 3218 "parser.c"
    break;

  case 244: /* constant: CFLOATCONST_SY  */
#line 1166 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3224 "parser.c"
    break;

  case 245: /* constant: FLOATCONST_SY  */
#line 1168 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3230 "parser.c"
    break;

  case 246: /* constant: FLOATHCONST_SY  */
#line 1170 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3236 "parser.c"
    break;

  case 247: /* constant: FLOATXCONST_SY  */
#line 1172 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3242 "parser.c"
    break;


#line 3246 "parser.c"

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

#line 1186 "parser.y"


