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
  YYSYMBOL_profile_specifier = 98,         /* profile_specifier  */
  YYSYMBOL_declaration = 99,               /* declaration  */
  YYSYMBOL_abstract_declaration = 100,     /* abstract_declaration  */
  YYSYMBOL_declaration_specifiers = 101,   /* declaration_specifiers  */
  YYSYMBOL_abstract_declaration_specifiers = 102, /* abstract_declaration_specifiers  */
  YYSYMBOL_abstract_declaration_specifiers2 = 103, /* abstract_declaration_specifiers2  */
  YYSYMBOL_init_declarator_list = 104,     /* init_declarator_list  */
  YYSYMBOL_init_declarator = 105,          /* init_declarator  */
  YYSYMBOL_type_specifier = 106,           /* type_specifier  */
  YYSYMBOL_type_qualifier = 107,           /* type_qualifier  */
  YYSYMBOL_type_domain = 108,              /* type_domain  */
  YYSYMBOL_storage_class = 109,            /* storage_class  */
  YYSYMBOL_function_specifier = 110,       /* function_specifier  */
  YYSYMBOL_in_out = 111,                   /* in_out  */
  YYSYMBOL_struct_or_connector_specifier = 112, /* struct_or_connector_specifier  */
  YYSYMBOL_struct_compound_header = 113,   /* struct_compound_header  */
  YYSYMBOL_struct_or_connector_header = 114, /* struct_or_connector_header  */
  YYSYMBOL_struct_identifier = 115,        /* struct_identifier  */
  YYSYMBOL_untagged_struct_header = 116,   /* untagged_struct_header  */
  YYSYMBOL_struct_declaration_list = 117,  /* struct_declaration_list  */
  YYSYMBOL_struct_declaration = 118,       /* struct_declaration  */
  YYSYMBOL_interface_specifier = 119,      /* interface_specifier  */
  YYSYMBOL_interface_compound_header = 120, /* interface_compound_header  */
  YYSYMBOL_interface_member_declaration_list = 121, /* interface_member_declaration_list  */
  YYSYMBOL_interface_member_declaration = 122, /* interface_member_declaration  */
  YYSYMBOL_annotation = 123,               /* annotation  */
  YYSYMBOL_124_1 = 124,                    /* $@1  */
  YYSYMBOL_annotation_decl_list = 125,     /* annotation_decl_list  */
  YYSYMBOL_declarator = 126,               /* declarator  */
  YYSYMBOL_semantic_declarator = 127,      /* semantic_declarator  */
  YYSYMBOL_basic_declarator = 128,         /* basic_declarator  */
  YYSYMBOL_function_decl_header = 129,     /* function_decl_header  */
  YYSYMBOL_abstract_declarator = 130,      /* abstract_declarator  */
  YYSYMBOL_parameter_list = 131,           /* parameter_list  */
  YYSYMBOL_parameter_declaration = 132,    /* parameter_declaration  */
  YYSYMBOL_abstract_parameter_list = 133,  /* abstract_parameter_list  */
  YYSYMBOL_non_empty_abstract_parameter_list = 134, /* non_empty_abstract_parameter_list  */
  YYSYMBOL_initializer = 135,              /* initializer  */
  YYSYMBOL_initializer_list = 136,         /* initializer_list  */
  YYSYMBOL_variable = 137,                 /* variable  */
  YYSYMBOL_basic_variable = 138,           /* basic_variable  */
  YYSYMBOL_primary_expression = 139,       /* primary_expression  */
  YYSYMBOL_postfix_expression = 140,       /* postfix_expression  */
  YYSYMBOL_actual_argument_list = 141,     /* actual_argument_list  */
  YYSYMBOL_non_empty_argument_list = 142,  /* non_empty_argument_list  */
  YYSYMBOL_expression_list = 143,          /* expression_list  */
  YYSYMBOL_unary_expression = 144,         /* unary_expression  */
  YYSYMBOL_cast_expression = 145,          /* cast_expression  */
  YYSYMBOL_multiplicative_expression = 146, /* multiplicative_expression  */
  YYSYMBOL_additive_expression = 147,      /* additive_expression  */
  YYSYMBOL_shift_expression = 148,         /* shift_expression  */
  YYSYMBOL_relational_expression = 149,    /* relational_expression  */
  YYSYMBOL_equality_expression = 150,      /* equality_expression  */
  YYSYMBOL_AND_expression = 151,           /* AND_expression  */
  YYSYMBOL_exclusive_OR_expression = 152,  /* exclusive_OR_expression  */
  YYSYMBOL_inclusive_OR_expression = 153,  /* inclusive_OR_expression  */
  YYSYMBOL_logical_AND_expression = 154,   /* logical_AND_expression  */
  YYSYMBOL_logical_OR_expression = 155,    /* logical_OR_expression  */
  YYSYMBOL_conditional_expression = 156,   /* conditional_expression  */
  YYSYMBOL_conditional_test = 157,         /* conditional_test  */
  YYSYMBOL_expression = 158,               /* expression  */
  YYSYMBOL_function_definition = 159,      /* function_definition  */
  YYSYMBOL_function_definition_header = 160, /* function_definition_header  */
  YYSYMBOL_statement = 161,                /* statement  */
  YYSYMBOL_balanced_statement = 162,       /* balanced_statement  */
  YYSYMBOL_dangling_statement = 163,       /* dangling_statement  */
  YYSYMBOL_discard_statement = 164,        /* discard_statement  */
  YYSYMBOL_jump_statement = 165,           /* jump_statement  */
  YYSYMBOL_if_statement = 166,             /* if_statement  */
  YYSYMBOL_dangling_if = 167,              /* dangling_if  */
  YYSYMBOL_if_header = 168,                /* if_header  */
  YYSYMBOL_compound_statement = 169,       /* compound_statement  */
  YYSYMBOL_compound_header = 170,          /* compound_header  */
  YYSYMBOL_compound_tail = 171,            /* compound_tail  */
  YYSYMBOL_block_item_list = 172,          /* block_item_list  */
  YYSYMBOL_block_item = 173,               /* block_item  */
  YYSYMBOL_expression_statement = 174,     /* expression_statement  */
  YYSYMBOL_expression_statement2 = 175,    /* expression_statement2  */
  YYSYMBOL_iteration_statement = 176,      /* iteration_statement  */
  YYSYMBOL_dangling_iteration = 177,       /* dangling_iteration  */
  YYSYMBOL_boolean_scalar_expression = 178, /* boolean_scalar_expression  */
  YYSYMBOL_for_expression_opt = 179,       /* for_expression_opt  */
  YYSYMBOL_for_expression = 180,           /* for_expression  */
  YYSYMBOL_boolean_expression_opt = 181,   /* boolean_expression_opt  */
  YYSYMBOL_return_statement = 182,         /* return_statement  */
  YYSYMBOL_member_identifier = 183,        /* member_identifier  */
  YYSYMBOL_scope_identifier = 184,         /* scope_identifier  */
  YYSYMBOL_semantics_identifier = 185,     /* semantics_identifier  */
  YYSYMBOL_type_identifier = 186,          /* type_identifier  */
  YYSYMBOL_variable_identifier = 187,      /* variable_identifier  */
  YYSYMBOL_identifier = 188,               /* identifier  */
  YYSYMBOL_constant = 189                  /* constant  */
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
#define YYFINAL  64
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2159

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  95
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  95
/* YYNRULES -- Number of rules.  */
#define YYNRULES  237
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  367

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
       0,   263,   263,   264,   271,   273,   275,   277,   287,   291,
     293,   295,   299,   306,   308,   313,   315,   317,   319,   321,
     323,   325,   330,   332,   334,   336,   338,   340,   342,   346,
     348,   352,   354,   362,   364,   366,   368,   370,   372,   374,
     376,   378,   380,   382,   384,   386,   388,   390,   392,   394,
     396,   398,   400,   412,   420,   422,   430,   432,   440,   442,
     450,   452,   454,   463,   466,   469,   473,   478,   480,   482,
     486,   487,   490,   494,   495,   498,   500,   509,   518,   527,
     528,   532,   554,   554,   559,   560,   567,   569,   573,   575,
     579,   581,   583,   585,   587,   591,   596,   597,   599,   616,
     618,   622,   624,   629,   630,   633,   639,   652,   654,   656,
     658,   666,   668,   680,   682,   686,   694,   695,   696,   698,
     706,   707,   709,   711,   713,   715,   720,   721,   724,   726,
     730,   732,   740,   741,   743,   745,   747,   749,   751,   759,
     763,   771,   772,   774,   776,   784,   785,   787,   795,   796,
     798,   806,   807,   809,   811,   813,   821,   822,   824,   832,
     833,   841,   842,   850,   851,   859,   860,   868,   869,   877,
     878,   882,   890,   901,   904,   909,   917,   918,   921,   922,
     923,   924,   925,   926,   927,   930,   931,   938,   940,   948,
     950,   958,   962,   964,   968,   976,   978,   982,   986,   994,
     995,   999,  1000,  1008,  1009,  1013,  1015,  1017,  1019,  1021,
    1023,  1025,  1033,  1035,  1037,  1041,  1043,  1048,  1052,  1054,
    1057,  1058,  1072,  1074,  1081,  1083,  1091,  1094,  1097,  1100,
    1103,  1106,  1108,  1118,  1120,  1122,  1124,  1126
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
  "compilation_unit", "external_declaration", "profile_specifier",
  "declaration", "abstract_declaration", "declaration_specifiers",
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

#define YYPACT_NINF (-292)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-228)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    1904,  -292,  -292,  -292,   -24,  -292,  -292,  -292,  -292,  -292,
    -292,  -292,  -292,  -292,  2091,  -292,   -10,  -292,  2091,  -292,
    -292,  -292,  -292,  -292,  -292,  -292,  -292,   -10,  -292,  -292,
      57,  -292,  1835,  -292,  1973,  -292,    -1,  -292,    70,  -292,
    2091,  2091,  2091,  2091,  2091,  -292,   -46,   -46,  -292,  -292,
     356,  -292,  -292,  -292,  -292,   -12,  -292,  -292,  -292,   -46,
    -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,   -14,
    -292,     5,   -25,    32,  1512,  -292,  -292,  -292,  -292,  -292,
    -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  1973,  -292,
    1973,     6,  -292,    10,   886,   668,  -292,  -292,  -292,    15,
      22,  -292,  1370,  1370,   955,    52,  -292,  -292,   590,  1370,
    1370,  1370,  1370,  -292,    -1,    55,  -292,  -292,  -292,   161,
    -292,  -292,    85,    68,    -3,   -16,    29,    56,    51,    66,
     158,   -32,  -292,    77,  -292,  -292,  -292,  -292,  -292,  -292,
    -292,  -292,   668,  -292,   434,   512,  -292,  -292,   115,  -292,
    -292,  -292,   174,  -292,   180,  -292,   -10,  2032,  -292,  -292,
      -4,  1024,  -292,  -292,  -292,    -4,   -15,  -292,  -292,    -4,
      13,     4,  -292,   118,   123,  -292,  1630,  -292,  -292,  1698,
    -292,  -292,  -292,    55,   104,   128,   140,  1094,  1439,  1439,
    -292,  -292,  -292,   130,  1439,   124,  -292,   127,  -292,  -292,
    -292,  -292,   136,  1439,  1439,  1439,  1439,  1439,  1439,  -292,
    -292,  1439,  1439,  1163,    -4,  1439,  1439,  1439,  1439,  1439,
    1439,  1439,  1439,  1439,  1439,  1439,  1439,  1439,  1439,  1439,
    1439,  1439,  1439,  1439,  -292,   189,  -292,  -292,   434,  -292,
    -292,  -292,    -4,  -292,  -292,  -292,    -4,  1766,  -292,  -292,
     746,  -292,  -292,  -292,  -292,   134,  -292,   142,   138,  2032,
    -292,  -292,  2091,  -292,  -292,  -292,  -292,   137,  -292,   146,
     147,  -292,   151,  -292,   152,  1439,  -292,    54,  -292,  -292,
    -292,  -292,  -292,  -292,  -292,   148,   153,   163,  -292,  -292,
    -292,  -292,  -292,  -292,    85,    85,    68,    68,    -3,    -3,
      -3,    -3,   -16,   -16,    29,    56,    51,    66,   158,   162,
     668,  -292,  -292,  -292,   169,  -292,  -292,  -292,  -292,    65,
    1571,  -292,  1024,   -13,  -292,  -292,  1439,  1232,  1439,  -292,
     668,  -292,  1439,  -292,  -292,  -292,  1439,  1439,  -292,  -292,
    -292,   816,  -292,  -292,  -292,  -292,   164,  -292,   165,  -292,
     170,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,
     175,  1301,  -292,   168,   668,  -292,  -292
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    52,    36,    53,     0,    57,    34,   231,    60,    58,
      62,    33,    59,    61,     0,    56,    72,    37,     0,   229,
      54,    55,    35,    38,    43,    42,    41,     0,    40,    39,
      44,   232,     0,     2,     0,     4,     0,    13,    15,    22,
       0,     0,     0,     0,     0,    49,    65,     0,    50,     5,
       0,    51,     8,    11,    21,    67,    71,    70,    14,     0,
      47,    45,    48,    46,     1,     3,     7,     6,     9,     0,
      29,    31,    86,    88,     0,    90,    28,    23,    25,    24,
      27,    26,    16,    18,    17,    20,    19,   197,     0,    66,
       0,     0,   234,     0,     0,     0,   235,   236,   237,     0,
       0,   233,     0,     0,     0,     0,   204,   174,     0,     0,
       0,     0,     0,   201,     0,    22,   116,   113,   120,   132,
     139,   141,   145,   148,   151,   156,   159,   161,   163,   165,
     167,   169,   172,     0,   206,   202,   176,   177,   179,   183,
     182,   185,     0,   178,     0,     0,   199,   180,     0,   181,
     186,   184,     0,   115,   230,   117,     0,     0,    78,    10,
       0,     0,   175,    82,    87,     0,     0,    95,   105,     0,
      96,     0,    99,     0,   104,    75,     0,    73,    76,     0,
     189,   190,   187,     0,   132,     0,     0,     0,     0,     0,
     134,   133,   225,     0,     0,     0,    96,     0,   135,   136,
     137,   138,    31,     0,     0,     0,     0,     0,     0,   122,
     121,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   192,   176,   198,   196,     0,   173,
     200,   203,     0,    68,    69,   228,     0,     0,    79,    30,
       0,    32,   107,    84,    89,     0,    92,   101,    12,     0,
      93,    94,     0,    63,    74,    64,   188,     0,   220,     0,
     218,   217,     0,   224,     0,     0,   118,     0,   130,   207,
     208,   209,   210,   211,   205,     0,     0,   127,   128,   123,
     226,   142,   143,   144,   146,   147,   150,   149,   155,   154,
     152,   153,   157,   158,   160,   162,   164,   166,   168,     0,
       0,   195,   114,   230,     0,    77,    80,   110,   111,     0,
       0,    91,     0,     0,   100,   106,     0,     0,     0,   194,
       0,   140,     0,   119,   124,   125,     0,     0,   191,   193,
      81,     0,   108,    83,    85,   102,     0,    98,     0,   222,
       0,   221,   212,   215,   131,   129,   170,   109,   112,    97,
       0,     0,   213,     0,     0,   214,   216
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -292,  -292,   217,  -292,     1,  -102,   -45,    12,  -292,  -292,
      91,     0,   214,   215,   216,   218,   219,  -292,   208,  -292,
     231,  -292,   171,   -62,  -292,  -292,  -292,    18,  -292,  -292,
    -292,   -34,  -292,  -292,  -292,  -292,  -292,     7,  -292,  -292,
    -239,  -292,  -292,    26,  -292,   -47,  -292,  -292,  -292,    53,
    -200,   -35,   -31,   -93,   -30,    36,    31,    39,    40,    38,
    -292,   -65,  -292,    17,    37,  -292,   -75,  -138,  -291,  -292,
    -292,  -292,  -292,  -292,  -292,    33,    35,   132,  -137,  -292,
    -180,  -292,  -292,  -185,   -84,  -292,  -292,  -292,  -292,  -292,
     114,    -6,  -292,   122,  -292
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    32,    33,    34,   113,   168,    36,    37,    38,    69,
      70,   183,    40,    41,    42,    43,    44,    45,    88,    46,
      55,    47,   176,   177,    48,   157,   247,   248,   164,   253,
     320,   202,    72,    73,    74,   258,   171,   172,   173,   174,
     251,   319,   116,   117,   118,   184,   286,   287,   277,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   178,    50,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   237,   145,   146,   147,
     148,   149,   150,   272,   269,   270,   350,   151,   289,   152,
     243,    51,   153,   154,   155
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      39,    35,    71,   119,   235,   114,   195,   268,   240,   274,
      56,   318,   232,   222,    39,   291,   292,   293,    39,   339,
     186,    56,     7,   255,   223,   346,    54,   220,     7,   169,
      58,     7,    39,    35,    39,    66,    87,    49,   221,   353,
      39,    39,    39,    39,    39,   -13,    19,    53,   119,   226,
     115,   163,    82,    83,    84,    85,    86,   159,   160,    31,
     224,   225,  -171,   156,   256,    31,   347,   234,    31,    49,
      68,    67,   227,   366,    39,   331,   259,   180,   161,    89,
      89,   181,   -13,   345,   260,     3,   170,   162,    39,   175,
      39,   175,   158,     5,    60,   119,   187,   119,   119,   114,
     114,   240,   358,   188,     8,     9,    10,   165,   115,    12,
     166,   185,   246,   167,   264,    13,    76,   264,    61,    15,
     196,   193,    52,    62,    63,   197,   332,    20,    21,   298,
     299,   300,   301,   194,   333,   257,   203,   341,    57,   342,
     119,   348,   349,   229,   115,   115,   209,   228,   351,    57,
     244,   210,   218,   219,    52,   190,   191,    39,    75,   230,
     325,   231,   198,   199,   200,   201,   204,   205,   206,   207,
     208,   233,   338,   215,   216,   217,    39,   175,   252,    39,
     175,   268,   212,   294,   295,   213,   241,   214,   242,   296,
     297,   119,   352,   114,  -227,   262,   302,   303,   261,   266,
     267,   273,   246,   209,   275,   271,   197,   276,   210,   161,
     310,   271,   314,   321,   169,   322,   323,   327,   326,   328,
     278,   279,   280,   281,   282,   283,   365,   334,   284,   285,
     288,   329,   330,   335,   211,   336,    75,   337,   115,   212,
     340,   361,   213,   359,   214,   360,   362,    39,   364,    65,
     309,   249,    77,    78,    79,    90,    80,    81,    59,    39,
     305,   179,    39,   119,   304,   316,   324,   252,   312,   306,
     308,   307,   356,   311,   196,   114,   238,   363,   245,   254,
       0,   119,    75,   119,     0,     0,     0,   245,     0,     0,
       0,    75,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   119,     0,     0,   119,     0,     0,
      39,   344,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   290,     0,     0,   252,
       0,     0,     0,   271,   271,     0,     0,     0,     0,   354,
       0,     0,     0,   355,     0,     0,     0,     1,   252,     0,
       0,     0,     0,     0,   313,     0,     2,    91,    75,    92,
       0,     3,    93,     0,    94,    95,     0,     0,     4,     5,
       6,    96,    97,    98,    99,     0,     0,     0,     7,   100,
       8,     9,    10,    11,   101,    12,     0,     0,   102,     0,
       0,    13,    14,   103,   104,    15,     0,    16,     0,    17,
       0,    18,    19,    20,    21,    22,   105,    23,    24,    25,
      26,    27,    28,    29,    30,    31,     0,   106,     0,     0,
     107,     0,     0,     0,     0,     1,     0,   108,    87,     0,
     109,   110,   111,   112,     2,    91,     0,    92,     0,     3,
      93,     0,    94,    95,     0,     0,     4,     5,     6,    96,
      97,    98,    99,     0,     0,     0,     7,   100,     8,     9,
      10,    11,   101,    12,     0,     0,   102,     0,     0,    13,
      14,   103,   104,    15,     0,    16,     0,    17,     0,    18,
      19,    20,    21,    22,   105,    23,    24,    25,    26,    27,
      28,    29,    30,    31,     0,   106,     0,     0,   236,     0,
       0,     0,     0,     1,     0,   108,    87,     0,   109,   110,
     111,   112,     2,    91,     0,    92,     0,     3,    93,     0,
      94,    95,     0,     0,     4,     5,     6,    96,    97,    98,
      99,     0,     0,     0,     7,   100,     8,     9,    10,    11,
     101,    12,     0,     0,   102,     0,     0,    13,    14,   103,
     104,    15,     0,    16,     0,    17,     0,    18,    19,    20,
      21,    22,   105,    23,    24,    25,    26,    27,    28,    29,
      30,    31,     0,   106,     0,     0,   239,     0,     0,     0,
       0,     1,     0,   108,    87,     0,   109,   110,   111,   112,
       2,     0,     0,    92,     0,     3,     0,     0,     0,     0,
       0,     0,     0,     5,     6,    96,    97,    98,     0,     0,
       0,     0,     7,     0,     8,     9,    10,    11,   101,    12,
       0,     0,   102,     0,     0,    13,    14,   103,     0,    15,
       0,    16,     0,    17,     0,     0,    19,    20,    21,    22,
       0,    23,    24,    25,    26,    27,    28,    29,    30,    31,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     1,
       0,   108,     0,     0,   109,   110,   111,   112,     2,    91,
       0,    92,     0,     0,    93,     0,    94,    95,     0,     0,
       0,     0,     6,    96,    97,    98,    99,     0,     0,     0,
       7,   100,     0,     0,     0,    11,   101,     0,     0,     0,
     102,     0,     0,     0,     0,   103,   104,     0,     0,    16,
       0,    17,     0,     0,    19,     0,     0,    22,   105,    23,
      24,    25,    26,    27,    28,    29,    30,    31,     0,   106,
       0,     0,     0,     0,     0,     0,     0,     1,     0,   108,
      87,     0,   109,   110,   111,   112,     2,     0,     0,    92,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       6,    96,    97,    98,     0,     0,     0,     0,     7,     0,
       0,     0,     0,    11,   101,     0,     0,     0,   102,     0,
       0,     0,     0,   103,     0,     0,     0,    16,     0,    17,
       0,     0,    19,     0,     0,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,     0,     1,     0,     0,
     317,     0,     0,     0,     0,     0,     2,   108,   250,    92,
     109,   110,   111,   112,     0,     0,     0,     0,     0,     0,
       6,    96,    97,    98,     0,     0,     0,     0,     7,     0,
       0,     0,     0,    11,   101,     0,     0,     0,   102,     0,
       0,     0,     0,   103,     0,     0,     0,    16,     0,    17,
       0,     0,    19,     0,     0,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,     0,     1,     0,     0,
     357,     0,     0,     0,     0,     0,     2,   108,   250,    92,
     109,   110,   111,   112,     0,     0,     0,     0,     0,     0,
       6,    96,    97,    98,     0,     0,     0,     0,     7,     0,
       0,     0,     0,    11,   101,     0,     0,     0,   102,     0,
       0,     0,     0,   103,     0,     0,     0,    16,     0,    17,
       0,     0,    19,     0,     0,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,     1,   182,     0,     0,
       0,     0,     0,     0,     0,     2,     0,   108,    92,     0,
     109,   110,   111,   112,     0,     0,     0,     0,     0,     6,
      96,    97,    98,     0,     0,     0,     0,     7,     0,     0,
       0,     0,    11,   101,     0,     0,     0,   102,     0,     0,
       0,     0,   103,     0,     0,     0,    16,     0,    17,     0,
       0,    19,     0,     0,    22,     0,    23,    24,    25,    26,
      27,    28,    29,    30,    31,     1,   192,     0,     0,     0,
       0,     0,     0,     0,     2,     0,   108,    92,     0,   109,
     110,   111,   112,     0,     0,     0,     0,     0,     6,    96,
      97,    98,     0,     0,     0,     0,     7,     0,     0,     0,
       0,    11,   101,     0,     0,     0,   102,     0,     0,     0,
       0,   103,     0,     0,     0,    16,     0,    17,     0,     0,
      19,     0,     0,    22,     0,    23,    24,    25,    26,    27,
      28,    29,    30,    31,     0,     1,     0,     0,     0,     0,
       0,     0,     0,     0,     2,   108,   250,    92,   109,   110,
     111,   112,     0,     0,     0,     0,     0,     0,     6,    96,
      97,    98,     0,     0,     0,     0,     7,     0,     0,     0,
       0,    11,   101,     0,     0,     0,   102,     0,     0,     0,
       0,   103,     0,     0,     0,    16,     0,    17,     0,     0,
      19,     0,     0,    22,     0,    23,    24,    25,    26,    27,
      28,    29,    30,    31,     1,  -219,     0,     0,     0,     0,
       0,     0,     0,     2,     0,   108,    92,     0,   109,   110,
     111,   112,     0,     0,     0,     0,     0,     6,    96,    97,
      98,     0,     0,     0,     0,     7,     0,     0,     0,     0,
      11,   101,     0,     0,     0,   102,     0,     0,     0,     0,
     103,     0,     0,     0,    16,     0,    17,     0,     0,    19,
       0,     0,    22,     0,    23,    24,    25,    26,    27,    28,
      29,    30,    31,     1,     0,     0,     0,     0,     0,     0,
       0,     0,     2,  -126,   108,    92,     0,   109,   110,   111,
     112,     0,     0,     0,     0,     0,     6,    96,    97,    98,
       0,     0,     0,     0,     7,     0,     0,     0,     0,    11,
     101,     0,     0,     0,   102,     0,     0,     0,     0,   103,
       0,     0,     0,    16,     0,    17,     0,     0,    19,     0,
       0,    22,     0,    23,    24,    25,    26,    27,    28,    29,
      30,    31,     1,  -223,     0,     0,     0,     0,     0,     0,
       0,     2,     0,   108,    92,     0,   109,   110,   111,   112,
       0,     0,     0,     0,     0,     6,    96,    97,    98,     0,
       0,     0,     0,     7,     0,     0,     0,     0,    11,   101,
       0,     0,     0,   102,     0,     0,     0,     0,   103,     0,
       0,     0,    16,     0,    17,     0,     0,    19,     0,     0,
      22,     0,    23,    24,    25,    26,    27,    28,    29,    30,
      31,     1,     0,     0,     0,     0,     0,     0,     0,     0,
       2,  -219,   108,    92,     0,   109,   110,   111,   112,     0,
       0,     0,     0,     0,     6,    96,    97,    98,     0,     0,
       0,     0,     7,     0,     0,     0,     0,    11,   101,     0,
       0,     0,   102,     0,     0,     0,     0,   103,     0,     0,
       0,    16,     0,    17,     0,     0,    19,     0,     0,    22,
       0,    23,    24,    25,    26,    27,    28,    29,    30,    31,
       1,     0,     0,     0,     0,     0,     0,     0,     0,     2,
       0,   189,    92,     0,   109,   110,   111,   112,     0,     0,
       0,     0,     0,     6,    96,    97,    98,     0,     0,     0,
       0,     7,     0,     0,     0,     0,    11,   101,     0,     0,
       0,   102,     0,     0,     0,     0,   103,     0,     0,     0,
      16,     0,    17,     0,     0,    19,     0,     0,    22,     0,
      23,    24,    25,    26,    27,    28,    29,    30,    31,     0,
       0,     0,     0,     1,     0,     0,     0,     0,     0,     0,
     108,     0,     2,   109,   110,   111,   112,     3,     0,     0,
       0,     0,     0,     0,     0,     5,     6,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
       0,    12,     0,     0,     0,     0,     0,    13,    14,     0,
       0,    15,     0,    16,     0,    17,     0,    18,    19,    20,
      21,    22,     1,    23,    24,    25,    26,    27,    28,    29,
      30,     2,     0,     0,     0,     0,     3,     0,     0,     0,
       0,     0,  -103,     4,     5,     6,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,     0,
      12,     0,     0,     0,     0,     0,    13,    14,     0,     0,
      15,     0,    16,     0,    17,     0,    18,    19,    20,    21,
      22,     1,    23,    24,    25,    26,    27,    28,    29,    30,
       2,     0,     0,     0,     0,     3,     0,     0,   343,     0,
       0,     0,     4,     5,     6,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     8,     9,    10,    11,     0,    12,
       0,     0,     0,     0,     0,    13,    14,     0,     0,    15,
       0,    16,     0,    17,     0,    18,    19,    20,    21,    22,
       0,    23,    24,    25,    26,    27,    28,    29,    30,     1,
       0,     0,     0,     0,   263,     0,     0,     0,     2,     0,
       0,     0,     0,     3,     0,     0,     0,     0,     0,     0,
       4,     5,     6,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     8,     9,    10,    11,     0,    12,     0,     0,
       0,     0,     0,    13,    14,     0,     0,    15,     0,    16,
       0,    17,     0,    18,    19,    20,    21,    22,     0,    23,
      24,    25,    26,    27,    28,    29,    30,     1,     0,     0,
       0,     0,   265,     0,     0,     0,     2,     0,     0,     0,
       0,     3,     0,     0,     0,     0,     0,     0,     0,     5,
       6,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       8,     9,    10,    11,     0,    12,     0,     0,     0,     0,
       0,    13,    14,     0,     0,    15,     0,    16,     0,    17,
       0,    18,    19,    20,    21,    22,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    64,     1,     0,     0,     0,
     315,     0,     0,     0,     0,     2,     0,     0,     0,     0,
       3,     0,     0,     0,     0,     0,     0,     4,     5,     6,
       0,     0,     0,     0,     0,     0,     0,     7,     0,     8,
       9,    10,    11,     0,    12,     0,     0,     0,     0,     0,
      13,    14,     0,     0,    15,     0,    16,     0,    17,     0,
      18,    19,    20,    21,    22,     0,    23,    24,    25,    26,
      27,    28,    29,    30,    31,     1,     0,     0,     0,     0,
       0,     0,     0,     0,     2,     0,     0,     0,     0,     3,
       0,     0,     0,     0,     0,     0,     4,     5,     6,     0,
       0,     0,     0,     0,     0,     0,     7,     0,     8,     9,
      10,    11,     0,    12,     0,     0,     0,     0,     0,    13,
      14,     0,     0,    15,     0,    16,     0,    17,     0,    18,
      19,    20,    21,    22,     0,    23,    24,    25,    26,    27,
      28,    29,    30,    31,     1,     0,     0,     0,     0,     0,
       0,     0,     0,     2,     0,     0,     0,     0,     3,     0,
       0,     0,     0,     0,     0,     4,     5,     6,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
      11,     0,    12,     0,     0,     0,     0,     0,    13,    14,
       0,     0,    15,     0,    16,     0,    17,     0,    18,    19,
      20,    21,    22,     1,    23,    24,    25,    26,    27,    28,
      29,    30,     2,     0,     0,     0,     0,     3,     0,     0,
       0,     0,     0,     0,     0,     5,     6,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
       0,    12,     0,     0,     0,     0,     0,    13,    14,     0,
       0,    15,     0,    16,     0,    17,     0,    18,    19,    20,
      21,    22,     1,    23,    24,    25,    26,    27,    28,    29,
      30,     2,     0,     0,     0,     0,     3,     0,     0,     0,
       0,     0,     0,     0,     5,     6,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,     0,
      12,     0,     0,     0,     0,     0,    13,    14,     0,     0,
      15,     0,    16,     0,    17,     0,     0,    19,    20,    21,
      22,     0,    23,    24,    25,    26,    27,    28,    29,    30
};

static const yytype_int16 yycheck[] =
{
       0,     0,    36,    50,   142,    50,   108,   187,   145,   194,
      16,   250,    44,    29,    14,   215,   216,   217,    18,   310,
      95,    27,    32,    38,    40,    38,    14,    30,    32,    74,
      18,    32,    32,    32,    34,    34,    82,     0,    41,   330,
      40,    41,    42,    43,    44,    32,    56,    71,    95,    20,
      50,    76,    40,    41,    42,    43,    44,    71,    72,    69,
      76,    77,    94,    75,    79,    69,    79,   142,    69,    32,
      71,    34,    43,   364,    74,   275,    72,    71,    73,    46,
      47,    71,    69,   322,    80,    15,    74,    82,    88,    88,
      90,    90,    59,    23,    37,   142,    81,   144,   145,   144,
     145,   238,   341,    81,    34,    35,    36,    75,   108,    39,
      78,    94,   157,    81,   176,    45,    46,   179,    61,    49,
     108,   104,     0,    66,    67,   108,    72,    57,    58,   222,
     223,   224,   225,    81,    80,   169,    81,    72,    16,    74,
     187,   326,   327,    92,   144,   145,    42,    91,   328,    27,
     156,    47,    84,    85,    32,   102,   103,   157,    36,    93,
     262,     3,   109,   110,   111,   112,     5,     6,     7,     8,
       9,    94,   310,    88,    89,    90,   176,   176,   161,   179,
     179,   361,    78,   218,   219,    81,    71,    83,    14,   220,
     221,   238,   330,   238,    14,    72,   226,   227,    80,    71,
      60,    71,   247,    42,    80,   188,   189,    80,    47,    73,
      21,   194,   246,    79,   259,    73,    78,    71,    81,    72,
     203,   204,   205,   206,   207,   208,   364,    79,   211,   212,
     213,    80,    80,    80,    73,    72,   114,    75,   238,    78,
      71,    71,    81,    79,    83,    80,    71,   247,    80,    32,
     233,   160,    38,    38,    38,    47,    38,    38,    27,   259,
     229,    90,   262,   310,   228,   247,   259,   250,   242,   230,
     232,   231,   337,   238,   262,   320,   144,   361,   156,   165,
      -1,   328,   160,   330,    -1,    -1,    -1,   165,    -1,    -1,
      -1,   169,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   361,    -1,    -1,   364,    -1,    -1,
     320,   320,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   214,    -1,    -1,   322,
      -1,    -1,    -1,   326,   327,    -1,    -1,    -1,    -1,   332,
      -1,    -1,    -1,   336,    -1,    -1,    -1,     1,   341,    -1,
      -1,    -1,    -1,    -1,   242,    -1,    10,    11,   246,    13,
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
      86,    87,    10,    11,    -1,    13,    -1,    15,    16,    -1,
      18,    19,    -1,    -1,    22,    23,    24,    25,    26,    27,
      28,    -1,    -1,    -1,    32,    33,    34,    35,    36,    37,
      38,    39,    -1,    -1,    42,    -1,    -1,    45,    46,    47,
      48,    49,    -1,    51,    -1,    53,    -1,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    -1,    71,    -1,    -1,    74,    -1,    -1,    -1,
      -1,     1,    -1,    81,    82,    -1,    84,    85,    86,    87,
      10,    -1,    -1,    13,    -1,    15,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    23,    24,    25,    26,    27,    -1,    -1,
      -1,    -1,    32,    -1,    34,    35,    36,    37,    38,    39,
      -1,    -1,    42,    -1,    -1,    45,    46,    47,    -1,    49,
      -1,    51,    -1,    53,    -1,    -1,    56,    57,    58,    59,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,
      -1,    81,    -1,    -1,    84,    85,    86,    87,    10,    11,
      -1,    13,    -1,    -1,    16,    -1,    18,    19,    -1,    -1,
      -1,    -1,    24,    25,    26,    27,    28,    -1,    -1,    -1,
      32,    33,    -1,    -1,    -1,    37,    38,    -1,    -1,    -1,
      42,    -1,    -1,    -1,    -1,    47,    48,    -1,    -1,    51,
      -1,    53,    -1,    -1,    56,    -1,    -1,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    -1,    71,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,    81,
      82,    -1,    84,    85,    86,    87,    10,    -1,    -1,    13,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,    -1,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,    53,
      -1,    -1,    56,    -1,    -1,    59,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    -1,     1,    -1,    -1,
      74,    -1,    -1,    -1,    -1,    -1,    10,    81,    82,    13,
      84,    85,    86,    87,    -1,    -1,    -1,    -1,    -1,    -1,
      24,    25,    26,    27,    -1,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    37,    38,    -1,    -1,    -1,    42,    -1,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,    53,
      -1,    -1,    56,    -1,    -1,    59,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    -1,     1,    -1,    -1,
      74,    -1,    -1,    -1,    -1,    -1,    10,    81,    82,    13,
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
      65,    66,    67,    68,    69,     1,    71,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    10,    -1,    81,    13,    -1,    84,
      85,    86,    87,    -1,    -1,    -1,    -1,    -1,    24,    25,
      26,    27,    -1,    -1,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    37,    38,    -1,    -1,    -1,    42,    -1,    -1,    -1,
      -1,    47,    -1,    -1,    -1,    51,    -1,    53,    -1,    -1,
      56,    -1,    -1,    59,    -1,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    -1,     1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    10,    81,    82,    13,    84,    85,
      86,    87,    -1,    -1,    -1,    -1,    -1,    -1,    24,    25,
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
      68,    69,     1,    71,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    10,    -1,    81,    13,    -1,    84,    85,    86,    87,
      -1,    -1,    -1,    -1,    -1,    24,    25,    26,    27,    -1,
      -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    38,
      -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,
      59,    -1,    61,    62,    63,    64,    65,    66,    67,    68,
      69,     1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      10,    80,    81,    13,    -1,    84,    85,    86,    87,    -1,
      -1,    -1,    -1,    -1,    24,    25,    26,    27,    -1,    -1,
      -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    38,    -1,
      -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    -1,    -1,
      -1,    51,    -1,    53,    -1,    -1,    56,    -1,    -1,    59,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,    69,
       1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    10,
      -1,    81,    13,    -1,    84,    85,    86,    87,    -1,    -1,
      -1,    -1,    -1,    24,    25,    26,    27,    -1,    -1,    -1,
      -1,    32,    -1,    -1,    -1,    -1,    37,    38,    -1,    -1,
      -1,    42,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,
      51,    -1,    53,    -1,    -1,    56,    -1,    -1,    59,    -1,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    -1,
      -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,    -1,
      81,    -1,    10,    84,    85,    86,    87,    15,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    23,    24,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,    37,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,    -1,
      -1,    49,    -1,    51,    -1,    53,    -1,    55,    56,    57,
      58,    59,     1,    61,    62,    63,    64,    65,    66,    67,
      68,    10,    -1,    -1,    -1,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    80,    22,    23,    24,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    34,    35,    36,    37,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    46,    -1,    -1,
      49,    -1,    51,    -1,    53,    -1,    55,    56,    57,    58,
      59,     1,    61,    62,    63,    64,    65,    66,    67,    68,
      10,    -1,    -1,    -1,    -1,    15,    -1,    -1,    77,    -1,
      -1,    -1,    22,    23,    24,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    34,    35,    36,    37,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    46,    -1,    -1,    49,
      -1,    51,    -1,    53,    -1,    55,    56,    57,    58,    59,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,     1,
      -1,    -1,    -1,    -1,    74,    -1,    -1,    -1,    10,    -1,
      -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,
      22,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    34,    35,    36,    37,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    46,    -1,    -1,    49,    -1,    51,
      -1,    53,    -1,    55,    56,    57,    58,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,     1,    -1,    -1,
      -1,    -1,    74,    -1,    -1,    -1,    10,    -1,    -1,    -1,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      34,    35,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    46,    -1,    -1,    49,    -1,    51,    -1,    53,
      -1,    55,    56,    57,    58,    59,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,     0,     1,    -1,    -1,    -1,
      74,    -1,    -1,    -1,    -1,    10,    -1,    -1,    -1,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    24,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,    -1,    34,
      35,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    46,    -1,    -1,    49,    -1,    51,    -1,    53,    -1,
      55,    56,    57,    58,    59,    -1,    61,    62,    63,    64,
      65,    66,    67,    68,    69,     1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    10,    -1,    -1,    -1,    -1,    15,
      -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    24,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    32,    -1,    34,    35,
      36,    37,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      46,    -1,    -1,    49,    -1,    51,    -1,    53,    -1,    55,
      56,    57,    58,    59,    -1,    61,    62,    63,    64,    65,
      66,    67,    68,    69,     1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    10,    -1,    -1,    -1,    -1,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    22,    23,    24,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,
      37,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,
      -1,    -1,    49,    -1,    51,    -1,    53,    -1,    55,    56,
      57,    58,    59,     1,    61,    62,    63,    64,    65,    66,
      67,    68,    10,    -1,    -1,    -1,    -1,    15,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    23,    24,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,    37,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,    -1,
      -1,    49,    -1,    51,    -1,    53,    -1,    55,    56,    57,
      58,    59,     1,    61,    62,    63,    64,    65,    66,    67,
      68,    10,    -1,    -1,    -1,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    23,    24,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    34,    35,    36,    37,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    46,    -1,    -1,
      49,    -1,    51,    -1,    53,    -1,    -1,    56,    57,    58,
      59,    -1,    61,    62,    63,    64,    65,    66,    67,    68
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    10,    15,    22,    23,    24,    32,    34,    35,
      36,    37,    39,    45,    46,    49,    51,    53,    55,    56,
      57,    58,    59,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    96,    97,    98,    99,   101,   102,   103,   106,
     107,   108,   109,   110,   111,   112,   114,   116,   119,   159,
     160,   186,   188,    71,   102,   115,   186,   188,   102,   115,
      37,    61,    66,    67,     0,    97,    99,   159,    71,   104,
     105,   126,   127,   128,   129,   188,    46,   107,   108,   109,
     110,   111,   102,   102,   102,   102,   102,    82,   113,   170,
     113,    11,    13,    16,    18,    19,    25,    26,    27,    28,
      33,    38,    42,    47,    48,    60,    71,    74,    81,    84,
      85,    86,    87,    99,   101,   106,   137,   138,   139,   140,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   157,   158,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   172,   173,   174,   175,   176,
     177,   182,   184,   187,   188,   189,    75,   120,   170,    71,
      72,    73,    82,    76,   123,    75,    78,    81,   100,   101,
     102,   131,   132,   133,   134,    99,   117,   118,   159,   117,
      71,    71,    71,   106,   140,   158,   161,    81,    81,    81,
     144,   144,    71,   158,    81,   100,   102,   158,   144,   144,
     144,   144,   126,    81,     5,     6,     7,     8,     9,    42,
      47,    73,    78,    81,    83,    88,    89,    90,    84,    85,
      30,    41,    29,    40,    76,    77,    20,    43,    91,    92,
      93,     3,    44,    94,   161,   162,    74,   171,   172,    74,
     173,    71,    14,   185,   186,   188,   101,   121,   122,   105,
      82,   135,   158,   124,   185,    38,    79,   126,   130,    72,
      80,    80,    72,    74,   118,    74,    71,    60,   175,   179,
     180,   158,   178,    71,   178,    80,    80,   143,   158,   158,
     158,   158,   158,   158,   158,   158,   141,   142,   158,   183,
     188,   145,   145,   145,   146,   146,   147,   147,   148,   148,
     148,   148,   149,   149,   150,   151,   152,   153,   154,   158,
      21,   171,   138,   188,   126,    74,   122,    74,   135,   136,
     125,    79,    73,    78,   132,   100,    81,    71,    72,    80,
      80,   145,    72,    80,    79,    80,    72,    75,   162,   163,
      71,    72,    74,    77,    99,   135,    38,    79,   178,   178,
     181,   175,   162,   163,   158,   158,   156,    74,   135,    79,
      80,    71,    71,   179,    80,   162,   163
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    95,    96,    96,    97,    97,    97,    97,    98,    99,
      99,    99,   100,   101,   101,   102,   102,   102,   102,   102,
     102,   102,   103,   103,   103,   103,   103,   103,   103,   104,
     104,   105,   105,   106,   106,   106,   106,   106,   106,   106,
     106,   106,   106,   106,   106,   106,   106,   106,   106,   106,
     106,   106,   106,   107,   108,   108,   109,   109,   110,   110,
     111,   111,   111,   112,   112,   112,   113,   114,   114,   114,
     115,   115,   116,   117,   117,   118,   118,   119,   120,   121,
     121,   122,   124,   123,   125,   125,   126,   126,   127,   127,
     128,   128,   128,   128,   128,   129,   130,   130,   130,   131,
     131,   132,   132,   133,   133,   134,   134,   135,   135,   135,
     135,   136,   136,   137,   137,   138,   139,   139,   139,   139,
     140,   140,   140,   140,   140,   140,   141,   141,   142,   142,
     143,   143,   144,   144,   144,   144,   144,   144,   144,   145,
     145,   146,   146,   146,   146,   147,   147,   147,   148,   148,
     148,   149,   149,   149,   149,   149,   150,   150,   150,   151,
     151,   152,   152,   153,   153,   154,   154,   155,   155,   156,
     156,   157,   158,   159,   159,   160,   161,   161,   162,   162,
     162,   162,   162,   162,   162,   163,   163,   164,   164,   165,
     165,   166,   167,   167,   168,   169,   169,   170,   171,   172,
     172,   173,   173,   174,   174,   175,   175,   175,   175,   175,
     175,   175,   176,   176,   176,   177,   177,   178,   179,   179,
     180,   180,   181,   181,   182,   182,   183,   184,   185,   186,
     187,   188,   188,   189,   189,   189,   189,   189
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     1,     1,     2,     2,     1,     2,
       3,     2,     2,     1,     2,     1,     2,     2,     2,     2,
       2,     2,     1,     2,     2,     2,     2,     2,     2,     1,
       3,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     2,     2,     2,     1,
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
#line 272 "parser.y"
                              { (yyval.dummy) = GlobalInitStatements(CurrentScope, (yyvsp[0].sc_stmt)); }
#line 1965 "parser.c"
    break;

  case 5: /* external_declaration: function_definition  */
#line 274 "parser.y"
                              { (yyval.dummy) = 0; }
#line 1971 "parser.c"
    break;

  case 6: /* external_declaration: profile_specifier function_definition  */
#line 276 "parser.y"
                              { (yyval.dummy) = 0; }
#line 1977 "parser.c"
    break;

  case 7: /* external_declaration: profile_specifier declaration  */
#line 278 "parser.y"
                              { (yyval.dummy) = GlobalInitStatements(CurrentScope, (yyvsp[0].sc_stmt)); ClearPendingProfileSpecifier(); }
#line 1983 "parser.c"
    break;

  case 8: /* profile_specifier: identifier  */
#line 288 "parser.y"
                              { (yyval.sc_ident) = (yyvsp[0].sc_ident); SetPendingProfileSpecifier(Cg->tokenLoc, (yyvsp[0].sc_ident)); }
#line 1989 "parser.c"
    break;

  case 9: /* declaration: declaration_specifiers ';'  */
#line 292 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 1995 "parser.c"
    break;

  case 10: /* declaration: declaration_specifiers init_declarator_list ';'  */
#line 294 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[-1].sc_stmt); }
#line 2001 "parser.c"
    break;

  case 11: /* declaration: ERROR_SY ';'  */
#line 296 "parser.y"
                              { RecordErrorPos(Cg->tokenLoc); (yyval.sc_stmt) = NULL; }
#line 2007 "parser.c"
    break;

  case 12: /* abstract_declaration: abstract_declaration_specifiers abstract_declarator  */
#line 300 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2013 "parser.c"
    break;

  case 13: /* declaration_specifiers: abstract_declaration_specifiers  */
#line 307 "parser.y"
                              { (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 2019 "parser.c"
    break;

  case 14: /* declaration_specifiers: TYPEDEF_SY abstract_declaration_specifiers  */
#line 309 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, TYPE_MISC_TYPEDEF); (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 2025 "parser.c"
    break;

  case 15: /* abstract_declaration_specifiers: abstract_declaration_specifiers2  */
#line 314 "parser.y"
                              { (yyval.sc_type) = (yyvsp[0].sc_type); }
#line 2031 "parser.c"
    break;

  case 16: /* abstract_declaration_specifiers: type_qualifier abstract_declaration_specifiers  */
#line 316 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2037 "parser.c"
    break;

  case 17: /* abstract_declaration_specifiers: storage_class abstract_declaration_specifiers  */
#line 318 "parser.y"
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2043 "parser.c"
    break;

  case 18: /* abstract_declaration_specifiers: type_domain abstract_declaration_specifiers  */
#line 320 "parser.y"
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2049 "parser.c"
    break;

  case 19: /* abstract_declaration_specifiers: in_out abstract_declaration_specifiers  */
#line 322 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2055 "parser.c"
    break;

  case 20: /* abstract_declaration_specifiers: function_specifier abstract_declaration_specifiers  */
#line 324 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[-1].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2061 "parser.c"
    break;

  case 21: /* abstract_declaration_specifiers: PACKED_SY abstract_declaration_specifiers  */
#line 326 "parser.y"
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2067 "parser.c"
    break;

  case 22: /* abstract_declaration_specifiers2: type_specifier  */
#line 331 "parser.y"
                              { (yyval.sc_type) = *SetDType(&CurrentDeclTypeSpecs, (yyvsp[0].sc_ptype)); }
#line 2073 "parser.c"
    break;

  case 23: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 type_qualifier  */
#line 333 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2079 "parser.c"
    break;

  case 24: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 storage_class  */
#line 335 "parser.y"
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2085 "parser.c"
    break;

  case 25: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 type_domain  */
#line 337 "parser.y"
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2091 "parser.c"
    break;

  case 26: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 in_out  */
#line 339 "parser.y"
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2097 "parser.c"
    break;

  case 27: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 function_specifier  */
#line 341 "parser.y"
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, (yyvsp[0].sc_int)); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2103 "parser.c"
    break;

  case 28: /* abstract_declaration_specifiers2: abstract_declaration_specifiers2 PACKED_SY  */
#line 343 "parser.y"
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); (yyval.sc_type) = CurrentDeclTypeSpecs; }
#line 2109 "parser.c"
    break;

  case 29: /* init_declarator_list: init_declarator  */
#line 347 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[0].sc_stmt); }
#line 2115 "parser.c"
    break;

  case 30: /* init_declarator_list: init_declarator_list ',' init_declarator  */
#line 349 "parser.y"
                              { (yyval.sc_stmt) = AddStmt((yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2121 "parser.c"
    break;

  case 31: /* init_declarator: declarator  */
#line 353 "parser.y"
                              { (yyval.sc_stmt) = Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_decl), NULL); }
#line 2127 "parser.c"
    break;

  case 32: /* init_declarator: declarator '=' initializer  */
#line 355 "parser.y"
                              { (yyval.sc_stmt) = Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[0].sc_expr)); }
#line 2133 "parser.c"
    break;

  case 33: /* type_specifier: INT_SY  */
#line 363 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, INT_SY); }
#line 2139 "parser.c"
    break;

  case 34: /* type_specifier: FLOAT_SY  */
#line 365 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, FLOAT_SY); }
#line 2145 "parser.c"
    break;

  case 35: /* type_specifier: VOID_SY  */
#line 367 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, VOID_SY); }
#line 2151 "parser.c"
    break;

  case 36: /* type_specifier: BOOLEAN_SY  */
#line 369 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, BOOLEAN_SY); }
#line 2157 "parser.c"
    break;

  case 37: /* type_specifier: TEXOBJ_SY  */
#line 371 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, TEXOBJ_SY); }
#line 2163 "parser.c"
    break;

  case 38: /* type_specifier: CHAR_SY  */
#line 373 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2169 "parser.c"
    break;

  case 39: /* type_specifier: SHORT_SY  */
#line 375 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2175 "parser.c"
    break;

  case 40: /* type_specifier: LONG_SY  */
#line 377 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2181 "parser.c"
    break;

  case 41: /* type_specifier: HALF_SY  */
#line 379 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2187 "parser.c"
    break;

  case 42: /* type_specifier: FIXED_SY  */
#line 381 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2193 "parser.c"
    break;

  case 43: /* type_specifier: DOUBLE_SY  */
#line 383 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2199 "parser.c"
    break;

  case 44: /* type_specifier: UNSIGNED_SY  */
#line 385 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 0); }
#line 2205 "parser.c"
    break;

  case 45: /* type_specifier: UNSIGNED_SY CHAR_SY  */
#line 387 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2211 "parser.c"
    break;

  case 46: /* type_specifier: UNSIGNED_SY SHORT_SY  */
#line 389 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2217 "parser.c"
    break;

  case 47: /* type_specifier: UNSIGNED_SY INT_SY  */
#line 391 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2223 "parser.c"
    break;

  case 48: /* type_specifier: UNSIGNED_SY LONG_SY  */
#line 393 "parser.y"
                              { (yyval.sc_ptype) = ResolveScalarTypeSpecifier(Cg->tokenLoc, (yyvsp[0].sc_token), 1); }
#line 2229 "parser.c"
    break;

  case 49: /* type_specifier: struct_or_connector_specifier  */
#line 395 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2235 "parser.c"
    break;

  case 50: /* type_specifier: interface_specifier  */
#line 397 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2241 "parser.c"
    break;

  case 51: /* type_specifier: type_identifier  */
#line 399 "parser.y"
                              { (yyval.sc_ptype) = LookUpTypeSymbol(NULL, (yyvsp[0].sc_ident)); }
#line 2247 "parser.c"
    break;

  case 52: /* type_specifier: error  */
#line 401 "parser.y"
                              {
                                SemanticParseError(Cg->tokenLoc, ERROR_S_TYPE_NAME_EXPECTED,
                                                   GetAtomString(atable, Cg->mostRecentToken /* yychar */));
                                (yyval.sc_ptype) = UndefinedType;
                              }
#line 2257 "parser.c"
    break;

  case 53: /* type_qualifier: CONST_SY  */
#line 413 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_CONST; }
#line 2263 "parser.c"
    break;

  case 54: /* type_domain: UNIFORM_SY  */
#line 421 "parser.y"
                              { (yyval.sc_int) = TYPE_DOMAIN_UNIFORM; }
#line 2269 "parser.c"
    break;

  case 55: /* type_domain: VARYING_SY  */
#line 423 "parser.y"
                              { (yyval.sc_int) = TYPE_DOMAIN_VARYING; }
#line 2275 "parser.c"
    break;

  case 56: /* storage_class: STATIC_SY  */
#line 431 "parser.y"
                              { (yyval.sc_int) = (int) SC_STATIC; }
#line 2281 "parser.c"
    break;

  case 57: /* storage_class: EXTERN_SY  */
#line 433 "parser.y"
                              { (yyval.sc_int) = (int) SC_EXTERN; }
#line 2287 "parser.c"
    break;

  case 58: /* function_specifier: INLINE_SY  */
#line 441 "parser.y"
                              { (yyval.sc_int) = TYPE_MISC_INLINE; }
#line 2293 "parser.c"
    break;

  case 59: /* function_specifier: INTERNAL_SY  */
#line 443 "parser.y"
                              { (yyval.sc_int) = TYPE_MISC_INTERNAL; }
#line 2299 "parser.c"
    break;

  case 60: /* in_out: IN_SY  */
#line 451 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_IN; }
#line 2305 "parser.c"
    break;

  case 61: /* in_out: OUT_SY  */
#line 453 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_OUT; }
#line 2311 "parser.c"
    break;

  case 62: /* in_out: INOUT_SY  */
#line 455 "parser.y"
                              { (yyval.sc_int) = TYPE_QUALIFIER_INOUT; }
#line 2317 "parser.c"
    break;

  case 63: /* struct_or_connector_specifier: struct_or_connector_header struct_compound_header struct_declaration_list '}'  */
#line 464 "parser.y"
                              { (yyval.sc_ptype) = SetStructMembers(Cg->tokenLoc, (yyvsp[-3].sc_ptype), PopScope());
                                CheckInterfaceConformance(Cg->tokenLoc, (yyval.sc_ptype)); }
#line 2324 "parser.c"
    break;

  case 64: /* struct_or_connector_specifier: untagged_struct_header struct_compound_header struct_declaration_list '}'  */
#line 467 "parser.y"
                              { (yyval.sc_ptype) = SetStructMembers(Cg->tokenLoc, (yyvsp[-3].sc_ptype), PopScope());
                                CheckInterfaceConformance(Cg->tokenLoc, (yyval.sc_ptype)); }
#line 2331 "parser.c"
    break;

  case 65: /* struct_or_connector_specifier: struct_or_connector_header  */
#line 470 "parser.y"
                              { (yyval.sc_ptype) = (yyvsp[0].sc_ptype); }
#line 2337 "parser.c"
    break;

  case 66: /* struct_compound_header: compound_header  */
#line 474 "parser.y"
                              { CurrentScope->IsStructScope = 1; (yyval.dummy) = (yyvsp[0].dummy); }
#line 2343 "parser.c"
    break;

  case 67: /* struct_or_connector_header: STRUCT_SY struct_identifier  */
#line 479 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, 0, (yyvsp[0].sc_ident)); }
#line 2349 "parser.c"
    break;

  case 68: /* struct_or_connector_header: STRUCT_SY struct_identifier ':' semantics_identifier  */
#line 481 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_ident), (yyvsp[-2].sc_ident)); }
#line 2355 "parser.c"
    break;

  case 69: /* struct_or_connector_header: STRUCT_SY struct_identifier ':' type_identifier  */
#line 483 "parser.y"
                              { (yyval.sc_ptype) = SetStructInterface(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_ident), (yyvsp[0].sc_ident)); }
#line 2361 "parser.c"
    break;

  case 72: /* untagged_struct_header: STRUCT_SY  */
#line 491 "parser.y"
                              { (yyval.sc_ptype) = StructHeader(Cg->tokenLoc, CurrentScope, 0, 0); }
#line 2367 "parser.c"
    break;

  case 75: /* struct_declaration: declaration  */
#line 499 "parser.y"
                            { (yyval.sc_stmt) = (yyvsp[0].sc_stmt); }
#line 2373 "parser.c"
    break;

  case 76: /* struct_declaration: function_definition  */
#line 501 "parser.y"
                            { (yyval.sc_stmt) = NULL; }
#line 2379 "parser.c"
    break;

  case 77: /* interface_specifier: INTERFACE_SY struct_identifier interface_compound_header interface_member_declaration_list '}'  */
#line 512 "parser.y"
                              { (yyval.sc_ptype) = SetInterfaceMembers(Cg->tokenLoc,
                                                         InterfaceHeader(Cg->tokenLoc, CurrentScope, (yyvsp[-3].sc_ident)),
                                                         PopScope()); }
#line 2387 "parser.c"
    break;

  case 78: /* interface_compound_header: compound_header  */
#line 519 "parser.y"
                              { CurrentScope->IsStructScope = 1; (yyval.dummy) = (yyvsp[0].dummy); }
#line 2393 "parser.c"
    break;

  case 81: /* interface_member_declaration: declaration_specifiers declarator ';'  */
#line 536 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2399 "parser.c"
    break;

  case 82: /* $@1: %empty  */
#line 554 "parser.y"
                              { PushScope(NewScope()); }
#line 2405 "parser.c"
    break;

  case 83: /* annotation: '<' $@1 annotation_decl_list '>'  */
#line 555 "parser.y"
                              { (yyval.sc_stmt) = (yyvsp[-1].sc_stmt); PopScope(); }
#line 2411 "parser.c"
    break;

  case 84: /* annotation_decl_list: %empty  */
#line 559 "parser.y"
                              { (yyval.sc_stmt) = 0; }
#line 2417 "parser.c"
    break;

  case 86: /* declarator: semantic_declarator  */
#line 568 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2423 "parser.c"
    break;

  case 87: /* declarator: semantic_declarator annotation  */
#line 570 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[-1].sc_decl); }
#line 2429 "parser.c"
    break;

  case 88: /* semantic_declarator: basic_declarator  */
#line 574 "parser.y"
                              { (yyval.sc_decl) = Declarator(Cg->tokenLoc, (yyvsp[0].sc_decl), 0); }
#line 2435 "parser.c"
    break;

  case 89: /* semantic_declarator: basic_declarator ':' semantics_identifier  */
#line 576 "parser.y"
                              { (yyval.sc_decl) = Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), (yyvsp[0].sc_ident)); }
#line 2441 "parser.c"
    break;

  case 90: /* basic_declarator: identifier  */
#line 580 "parser.y"
                              { (yyval.sc_decl) = NewDeclNode(Cg->tokenLoc, (yyvsp[0].sc_ident), &CurrentDeclTypeSpecs); }
#line 2447 "parser.c"
    break;

  case 91: /* basic_declarator: basic_declarator '[' INTCONST_SY ']'  */
#line 582 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-3].sc_decl), (int) (yyvsp[-1].sc_literal).value.i, 0); }
#line 2453 "parser.c"
    break;

  case 92: /* basic_declarator: basic_declarator '[' ']'  */
#line 584 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), 0 , 1); }
#line 2459 "parser.c"
    break;

  case 93: /* basic_declarator: function_decl_header parameter_list ')'  */
#line 586 "parser.y"
                              { (yyval.sc_decl) = SetFunTypeParams(CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_decl), (yyvsp[-1].sc_decl)); }
#line 2465 "parser.c"
    break;

  case 94: /* basic_declarator: function_decl_header abstract_parameter_list ')'  */
#line 588 "parser.y"
                              { (yyval.sc_decl) = SetFunTypeParams(CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_decl), NULL); }
#line 2471 "parser.c"
    break;

  case 95: /* function_decl_header: basic_declarator '('  */
#line 592 "parser.y"
                              { (yyval.sc_decl) = FunctionDeclHeader(&(yyvsp[-1].sc_decl)->loc, CurrentScope, (yyvsp[-1].sc_decl)); }
#line 2477 "parser.c"
    break;

  case 96: /* abstract_declarator: %empty  */
#line 596 "parser.y"
                              { (yyval.sc_decl) = NewDeclNode(Cg->tokenLoc, 0, &CurrentDeclTypeSpecs); }
#line 2483 "parser.c"
    break;

  case 97: /* abstract_declarator: abstract_declarator '[' INTCONST_SY ']'  */
#line 598 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-3].sc_decl), (int) (yyvsp[-1].sc_literal).value.i, 0); }
#line 2489 "parser.c"
    break;

  case 98: /* abstract_declarator: abstract_declarator '[' ']'  */
#line 600 "parser.y"
                              { (yyval.sc_decl) = Array_Declarator(Cg->tokenLoc, (yyvsp[-2].sc_decl), 0 , 1); }
#line 2495 "parser.c"
    break;

  case 99: /* parameter_list: parameter_declaration  */
#line 617 "parser.y"
                              { (yyval.sc_decl) = (yyvsp[0].sc_decl); }
#line 2501 "parser.c"
    break;

  case 100: /* parameter_list: parameter_list ',' parameter_declaration  */
#line 619 "parser.y"
                              { (yyval.sc_decl) = AddDecl((yyvsp[-2].sc_decl), (yyvsp[0].sc_decl)); }
#line 2507 "parser.c"
    break;

  case 101: /* parameter_declaration: declaration_specifiers declarator  */
#line 623 "parser.y"
                              { (yyval.sc_decl) = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[0].sc_decl), NULL); }
#line 2513 "parser.c"
    break;

  case 102: /* parameter_declaration: declaration_specifiers declarator '=' initializer  */
#line 625 "parser.y"
                              { (yyval.sc_decl) = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[0].sc_expr)); }
#line 2519 "parser.c"
    break;

  case 103: /* abstract_parameter_list: %empty  */
#line 629 "parser.y"
                              { (yyval.sc_decl) = NULL; }
#line 2525 "parser.c"
    break;

  case 105: /* non_empty_abstract_parameter_list: abstract_declaration  */
#line 634 "parser.y"
                              {
                                if (IsVoid(&(yyvsp[0].sc_decl)->type.type))
                                    CurrentScope->HasVoidParameter = 1;
                                (yyval.sc_decl) = (yyvsp[0].sc_decl);
                              }
#line 2535 "parser.c"
    break;

  case 106: /* non_empty_abstract_parameter_list: non_empty_abstract_parameter_list ',' abstract_declaration  */
#line 640 "parser.y"
                              {
                                if (CurrentScope->HasVoidParameter || IsVoid(&(yyvsp[-2].sc_decl)->type.type)) {
                                    SemanticError(Cg->tokenLoc, ERROR___VOID_NOT_ONLY_PARAM);
                                }
                                (yyval.sc_decl) = AddDecl((yyvsp[-2].sc_decl), (yyvsp[0].sc_decl));
                              }
#line 2546 "parser.c"
    break;

  case 107: /* initializer: expression  */
#line 653 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[0].sc_expr)); }
#line 2552 "parser.c"
    break;

  case 108: /* initializer: '{' initializer_list '}'  */
#line 655 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[-1].sc_expr)); }
#line 2558 "parser.c"
    break;

  case 109: /* initializer: '{' initializer_list ',' '}'  */
#line 657 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, (yyvsp[-2].sc_expr)); }
#line 2564 "parser.c"
    break;

  case 110: /* initializer: '{' '}'  */
#line 663 "parser.y"
                              { (yyval.sc_expr) = Initializer(Cg->tokenLoc, NULL); }
#line 2570 "parser.c"
    break;

  case 111: /* initializer_list: initializer  */
#line 667 "parser.y"
                              { (yyval.sc_expr) = InitializerList(Cg->tokenLoc, (yyvsp[0].sc_expr), NULL); }
#line 2576 "parser.c"
    break;

  case 112: /* initializer_list: initializer_list ',' initializer  */
#line 669 "parser.y"
                              { (yyval.sc_expr) = InitializerList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2582 "parser.c"
    break;

  case 113: /* variable: basic_variable  */
#line 681 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[0].sc_expr); }
#line 2588 "parser.c"
    break;

  case 114: /* variable: scope_identifier COLONCOLON_SY basic_variable  */
#line 683 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[0].sc_expr); }
#line 2594 "parser.c"
    break;

  case 115: /* basic_variable: variable_identifier  */
#line 687 "parser.y"
                              { (yyval.sc_expr) = BasicVariable(Cg->tokenLoc, (yyvsp[0].sc_ident)); }
#line 2600 "parser.c"
    break;

  case 118: /* primary_expression: '(' expression ')'  */
#line 697 "parser.y"
                              { (yyval.sc_expr) = (yyvsp[-1].sc_expr); }
#line 2606 "parser.c"
    break;

  case 119: /* primary_expression: type_specifier '(' expression_list ')'  */
#line 699 "parser.y"
                              { (yyval.sc_expr) = NewVectorConstructor(Cg->tokenLoc, (yyvsp[-3].sc_ptype), (yyvsp[-1].sc_expr)); }
#line 2612 "parser.c"
    break;

  case 121: /* postfix_expression: postfix_expression PLUSPLUS_SY  */
#line 708 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(POSTINC_OP, (yyvsp[-1].sc_expr)); }
#line 2618 "parser.c"
    break;

  case 122: /* postfix_expression: postfix_expression MINUSMINUS_SY  */
#line 710 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(POSTDEC_OP, (yyvsp[-1].sc_expr)); }
#line 2624 "parser.c"
    break;

  case 123: /* postfix_expression: postfix_expression '.' member_identifier  */
#line 712 "parser.y"
                              { (yyval.sc_expr) = NewMemberSelectorOrSwizzleOrWriteMaskOperator(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_ident)); }
#line 2630 "parser.c"
    break;

  case 124: /* postfix_expression: postfix_expression '[' expression ']'  */
#line 714 "parser.y"
                              { (yyval.sc_expr) = NewIndexOperator(Cg->tokenLoc, (yyvsp[-3].sc_expr), (yyvsp[-1].sc_expr)); }
#line 2636 "parser.c"
    break;

  case 125: /* postfix_expression: postfix_expression '(' actual_argument_list ')'  */
#line 716 "parser.y"
                              { (yyval.sc_expr) = NewFunctionCallOperator(Cg->tokenLoc, (yyvsp[-3].sc_expr), (yyvsp[-1].sc_expr)); }
#line 2642 "parser.c"
    break;

  case 126: /* actual_argument_list: %empty  */
#line 720 "parser.y"
                                { (yyval.sc_expr) = NULL; }
#line 2648 "parser.c"
    break;

  case 128: /* non_empty_argument_list: expression  */
#line 725 "parser.y"
                              { (yyval.sc_expr) = ArgumentList(Cg->tokenLoc, NULL, (yyvsp[0].sc_expr)); }
#line 2654 "parser.c"
    break;

  case 129: /* non_empty_argument_list: non_empty_argument_list ',' expression  */
#line 727 "parser.y"
                              { (yyval.sc_expr) = ArgumentList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2660 "parser.c"
    break;

  case 130: /* expression_list: expression  */
#line 731 "parser.y"
                              { (yyval.sc_expr) = ExpressionList(Cg->tokenLoc, NULL, (yyvsp[0].sc_expr)); }
#line 2666 "parser.c"
    break;

  case 131: /* expression_list: expression_list ',' expression  */
#line 733 "parser.y"
                              { (yyval.sc_expr) = ExpressionList(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2672 "parser.c"
    break;

  case 133: /* unary_expression: PLUSPLUS_SY unary_expression  */
#line 742 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(PREINC_OP, (yyvsp[0].sc_expr)); }
#line 2678 "parser.c"
    break;

  case 134: /* unary_expression: MINUSMINUS_SY unary_expression  */
#line 744 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewUnopNode(PREDEC_OP, (yyvsp[0].sc_expr)); }
#line 2684 "parser.c"
    break;

  case 135: /* unary_expression: '+' unary_expression  */
#line 746 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, POS_OP, '+', (yyvsp[0].sc_expr), 0); }
#line 2690 "parser.c"
    break;

  case 136: /* unary_expression: '-' unary_expression  */
#line 748 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, NEG_OP, '-', (yyvsp[0].sc_expr), 0); }
#line 2696 "parser.c"
    break;

  case 137: /* unary_expression: '!' unary_expression  */
#line 750 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, BNOT_OP, '!', (yyvsp[0].sc_expr), 0); }
#line 2702 "parser.c"
    break;

  case 138: /* unary_expression: '~' unary_expression  */
#line 752 "parser.y"
                              { (yyval.sc_expr) = NewUnaryOperator(Cg->tokenLoc, NOT_OP, '~', (yyvsp[0].sc_expr), 1); }
#line 2708 "parser.c"
    break;

  case 140: /* cast_expression: '(' abstract_declaration ')' cast_expression  */
#line 764 "parser.y"
                              { (yyval.sc_expr) = NewCastOperator(Cg->tokenLoc, (yyvsp[0].sc_expr), GetTypePointer(&(yyvsp[-2].sc_decl)->loc, &(yyvsp[-2].sc_decl)->type)); }
#line 2714 "parser.c"
    break;

  case 142: /* multiplicative_expression: multiplicative_expression '*' cast_expression  */
#line 773 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, MUL_OP, '*', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2720 "parser.c"
    break;

  case 143: /* multiplicative_expression: multiplicative_expression '/' cast_expression  */
#line 775 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, DIV_OP, '/', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2726 "parser.c"
    break;

  case 144: /* multiplicative_expression: multiplicative_expression '%' cast_expression  */
#line 777 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, MOD_OP, '%', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2732 "parser.c"
    break;

  case 146: /* additive_expression: additive_expression '+' multiplicative_expression  */
#line 786 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, ADD_OP, '+', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2738 "parser.c"
    break;

  case 147: /* additive_expression: additive_expression '-' multiplicative_expression  */
#line 788 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SUB_OP, '-', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2744 "parser.c"
    break;

  case 149: /* shift_expression: shift_expression LL_SY additive_expression  */
#line 797 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SHL_OP, LL_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2750 "parser.c"
    break;

  case 150: /* shift_expression: shift_expression GG_SY additive_expression  */
#line 799 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, SHR_OP, GG_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2756 "parser.c"
    break;

  case 152: /* relational_expression: relational_expression '<' shift_expression  */
#line 808 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, LT_OP, '<', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2762 "parser.c"
    break;

  case 153: /* relational_expression: relational_expression '>' shift_expression  */
#line 810 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, GT_OP, '>', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2768 "parser.c"
    break;

  case 154: /* relational_expression: relational_expression LE_SY shift_expression  */
#line 812 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, LE_OP, LE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2774 "parser.c"
    break;

  case 155: /* relational_expression: relational_expression GE_SY shift_expression  */
#line 814 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, GE_OP, GE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2780 "parser.c"
    break;

  case 157: /* equality_expression: equality_expression EQ_SY relational_expression  */
#line 823 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, EQ_OP, EQ_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2786 "parser.c"
    break;

  case 158: /* equality_expression: equality_expression NE_SY relational_expression  */
#line 825 "parser.y"
                              { (yyval.sc_expr) = NewBinaryComparisonOperator(Cg->tokenLoc, NE_OP, NE_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2792 "parser.c"
    break;

  case 160: /* AND_expression: AND_expression '&' equality_expression  */
#line 834 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, AND_OP, '&', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2798 "parser.c"
    break;

  case 162: /* exclusive_OR_expression: exclusive_OR_expression '^' AND_expression  */
#line 843 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, XOR_OP, '^', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2804 "parser.c"
    break;

  case 164: /* inclusive_OR_expression: inclusive_OR_expression '|' exclusive_OR_expression  */
#line 852 "parser.y"
                              { (yyval.sc_expr) = NewBinaryOperator(Cg->tokenLoc, OR_OP, '|', (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 1); }
#line 2810 "parser.c"
    break;

  case 166: /* logical_AND_expression: logical_AND_expression AND_SY inclusive_OR_expression  */
#line 861 "parser.y"
                              { (yyval.sc_expr) = NewBinaryBooleanOperator(Cg->tokenLoc, BAND_OP, AND_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2816 "parser.c"
    break;

  case 168: /* logical_OR_expression: logical_OR_expression OR_SY logical_AND_expression  */
#line 870 "parser.y"
                              { (yyval.sc_expr) = NewBinaryBooleanOperator(Cg->tokenLoc, BOR_OP, OR_SY, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2822 "parser.c"
    break;

  case 170: /* conditional_expression: conditional_test '?' expression ':' conditional_expression  */
#line 879 "parser.y"
                              { (yyval.sc_expr) = NewConditionalOperator(Cg->tokenLoc, (yyvsp[-4].sc_expr), (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2828 "parser.c"
    break;

  case 171: /* conditional_test: logical_OR_expression  */
#line 883 "parser.y"
                              {  (yyval.sc_expr) = CheckBooleanExpr(Cg->tokenLoc, (yyvsp[0].sc_expr), 1); }
#line 2834 "parser.c"
    break;

  case 173: /* function_definition: function_definition_header block_item_list '}'  */
#line 902 "parser.y"
                              { DefineFunction(Cg->tokenLoc, CurrentScope, (yyvsp[-2].sc_decl), (yyvsp[-1].sc_stmt)); PopScope();
                                ResumeStructScopeAfterMethodBody(); }
#line 2841 "parser.c"
    break;

  case 174: /* function_definition: function_definition_header '}'  */
#line 905 "parser.y"
                              { DefineFunction(Cg->tokenLoc, CurrentScope, (yyvsp[-1].sc_decl), NULL); PopScope();
                                ResumeStructScopeAfterMethodBody(); }
#line 2848 "parser.c"
    break;

  case 175: /* function_definition_header: declaration_specifiers declarator '{'  */
#line 910 "parser.y"
                              { (yyval.sc_decl) = Function_Definition_Header(Cg->tokenLoc, (yyvsp[-1].sc_decl)); }
#line 2854 "parser.c"
    break;

  case 187: /* discard_statement: DISCARD_SY ';'  */
#line 939 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewDiscardStmt(Cg->tokenLoc, NULL); }
#line 2860 "parser.c"
    break;

  case 188: /* discard_statement: DISCARD_SY expression ';'  */
#line 941 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewDiscardStmt(Cg->tokenLoc, CheckBooleanExpr(Cg->tokenLoc, (yyvsp[-1].sc_expr), 1)); }
#line 2866 "parser.c"
    break;

  case 189: /* jump_statement: BREAK_SY ';'  */
#line 949 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewSimpleStmt(Cg->tokenLoc, BREAK_STMT); }
#line 2872 "parser.c"
    break;

  case 190: /* jump_statement: CONTINUE_SY ';'  */
#line 951 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewSimpleStmt(Cg->tokenLoc, CONTINUE_STMT); }
#line 2878 "parser.c"
    break;

  case 191: /* if_statement: if_header balanced_statement ELSE_SY balanced_statement  */
#line 959 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-3].sc_stmt), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2884 "parser.c"
    break;

  case 192: /* dangling_if: if_header statement  */
#line 963 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-1].sc_stmt), (yyvsp[0].sc_stmt), NULL); }
#line 2890 "parser.c"
    break;

  case 193: /* dangling_if: if_header balanced_statement ELSE_SY dangling_statement  */
#line 965 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) SetThenElseStmts(Cg->tokenLoc, (yyvsp[-3].sc_stmt), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2896 "parser.c"
    break;

  case 194: /* if_header: IF_SY '(' boolean_scalar_expression ')'  */
#line 969 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewIfStmt(Cg->tokenLoc, (yyvsp[-1].sc_expr), NULL, NULL); ; }
#line 2902 "parser.c"
    break;

  case 195: /* compound_statement: compound_header block_item_list compound_tail  */
#line 977 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewBlockStmt(Cg->tokenLoc, (yyvsp[-1].sc_stmt)); }
#line 2908 "parser.c"
    break;

  case 196: /* compound_statement: compound_header compound_tail  */
#line 979 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2914 "parser.c"
    break;

  case 197: /* compound_header: '{'  */
#line 983 "parser.y"
                              { PushScope(NewScope()); CurrentScope->funindex = NextFunctionIndex; }
#line 2920 "parser.c"
    break;

  case 198: /* compound_tail: '}'  */
#line 987 "parser.y"
                              {
                                if (Cg->options.DumpParseTree)
                                    PrintScopeDeclarations();
                                PopScope();
                              }
#line 2930 "parser.c"
    break;

  case 200: /* block_item_list: block_item_list block_item  */
#line 996 "parser.y"
                              { (yyval.sc_stmt) = AddStmt((yyvsp[-1].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 2936 "parser.c"
    break;

  case 202: /* block_item: statement  */
#line 1001 "parser.y"
                              { (yyval.sc_stmt) = CheckStmt((yyvsp[0].sc_stmt)); }
#line 2942 "parser.c"
    break;

  case 204: /* expression_statement: ';'  */
#line 1010 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 2948 "parser.c"
    break;

  case 205: /* expression_statement2: postfix_expression '=' expression  */
#line 1014 "parser.y"
                              { (yyval.sc_stmt) = NewSimpleAssignmentStmt(Cg->tokenLoc, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr), 0); }
#line 2954 "parser.c"
    break;

  case 206: /* expression_statement2: expression  */
#line 1016 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewExprStmt(Cg->tokenLoc, (yyvsp[0].sc_expr)); }
#line 2960 "parser.c"
    break;

  case 207: /* expression_statement2: postfix_expression ASSIGNMINUS_SY expression  */
#line 1018 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMINUS_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2966 "parser.c"
    break;

  case 208: /* expression_statement2: postfix_expression ASSIGNMOD_SY expression  */
#line 1020 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMOD_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2972 "parser.c"
    break;

  case 209: /* expression_statement2: postfix_expression ASSIGNPLUS_SY expression  */
#line 1022 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNPLUS_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2978 "parser.c"
    break;

  case 210: /* expression_statement2: postfix_expression ASSIGNSLASH_SY expression  */
#line 1024 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSLASH_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2984 "parser.c"
    break;

  case 211: /* expression_statement2: postfix_expression ASSIGNSTAR_SY expression  */
#line 1026 "parser.y"
                              { (yyval.sc_stmt) = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSTAR_OP, (yyvsp[-2].sc_expr), (yyvsp[0].sc_expr)); }
#line 2990 "parser.c"
    break;

  case 212: /* iteration_statement: WHILE_SY '(' boolean_scalar_expression ')' balanced_statement  */
#line 1034 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, (yyvsp[-2].sc_expr), (yyvsp[0].sc_stmt)); }
#line 2996 "parser.c"
    break;

  case 213: /* iteration_statement: DO_SY statement WHILE_SY '(' boolean_scalar_expression ')' ';'  */
#line 1036 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, DO_STMT, (yyvsp[-2].sc_expr), (yyvsp[-5].sc_stmt)); }
#line 3002 "parser.c"
    break;

  case 214: /* iteration_statement: FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' balanced_statement  */
#line 1038 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewForStmt(Cg->tokenLoc, (yyvsp[-6].sc_stmt), (yyvsp[-4].sc_expr), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3008 "parser.c"
    break;

  case 215: /* dangling_iteration: WHILE_SY '(' boolean_scalar_expression ')' dangling_statement  */
#line 1042 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, (yyvsp[-2].sc_expr), (yyvsp[0].sc_stmt)); }
#line 3014 "parser.c"
    break;

  case 216: /* dangling_iteration: FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' dangling_statement  */
#line 1044 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewForStmt(Cg->tokenLoc, (yyvsp[-6].sc_stmt), (yyvsp[-4].sc_expr), (yyvsp[-2].sc_stmt), (yyvsp[0].sc_stmt)); }
#line 3020 "parser.c"
    break;

  case 217: /* boolean_scalar_expression: expression  */
#line 1049 "parser.y"
                              {  (yyval.sc_expr) = CheckBooleanExpr(Cg->tokenLoc, (yyvsp[0].sc_expr), 0); }
#line 3026 "parser.c"
    break;

  case 219: /* for_expression_opt: %empty  */
#line 1054 "parser.y"
                              { (yyval.sc_stmt) = NULL; }
#line 3032 "parser.c"
    break;

  case 221: /* for_expression: for_expression ',' expression_statement2  */
#line 1059 "parser.y"
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
#line 3048 "parser.c"
    break;

  case 223: /* boolean_expression_opt: %empty  */
#line 1074 "parser.y"
                              { (yyval.sc_expr) = NULL; }
#line 3054 "parser.c"
    break;

  case 224: /* return_statement: RETURN_SY expression ';'  */
#line 1082 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, (yyvsp[-1].sc_expr)); }
#line 3060 "parser.c"
    break;

  case 225: /* return_statement: RETURN_SY ';'  */
#line 1084 "parser.y"
                              { (yyval.sc_stmt) = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, NULL); }
#line 3066 "parser.c"
    break;

  case 231: /* identifier: IDENT_SY  */
#line 1107 "parser.y"
                              { (yyval.sc_ident) = (yyvsp[0].sc_ident); }
#line 3072 "parser.c"
    break;

  case 232: /* identifier: RESERVED_SY  */
#line 1109 "parser.y"
                              {
                                /* SemanticError, not SemanticParseError: the
                                 * latter is gated by AllowSemanticParseErrors */
                                SemanticError(Cg->tokenLoc, ERROR_S_RESERVED_WORD,
                                              GetAtomString(atable, (yyvsp[0].sc_token)));
                                (yyval.sc_ident) = (yyvsp[0].sc_token);
                              }
#line 3084 "parser.c"
    break;

  case 233: /* constant: INTCONST_SY  */
#line 1119 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(ICONST_OP, &(yyvsp[0].sc_literal)); }
#line 3090 "parser.c"
    break;

  case 234: /* constant: CFLOATCONST_SY  */
#line 1121 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3096 "parser.c"
    break;

  case 235: /* constant: FLOATCONST_SY  */
#line 1123 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3102 "parser.c"
    break;

  case 236: /* constant: FLOATHCONST_SY  */
#line 1125 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3108 "parser.c"
    break;

  case 237: /* constant: FLOATXCONST_SY  */
#line 1127 "parser.y"
                              { (yyval.sc_expr) = (expr *) NewNumericConstNode(FCONST_OP, &(yyvsp[0].sc_literal)); }
#line 3114 "parser.c"
    break;


#line 3118 "parser.c"

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

#line 1141 "parser.y"


