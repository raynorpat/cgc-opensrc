%{
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

%}

/* Grammar semantic type: */

%union {
    int    sc_token;
    int    sc_int;
    float  sc_fval;
    CgNumericValue sc_literal;
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
}

/* Preserve the token numbers embedded in the checked-in stdlib stream. */
%token <sc_token> AND_SY 257
%token <sc_token> ASM_SY 258
%token <sc_token> ASSIGNMINUS_SY 259
%token <sc_token> ASSIGNMOD_SY 260
%token <sc_token> ASSIGNPLUS_SY 261
%token <sc_token> ASSIGNSLASH_SY 262
%token <sc_token> ASSIGNSTAR_SY 263
%token <sc_token> BOOLEAN_SY 264
%token <sc_token> BREAK_SY 265
%token <sc_token> CASE_SY 266
%token <sc_literal> CFLOATCONST_SY 267
%token <sc_token> COLONCOLON_SY 268
%token <sc_token> CONST_SY 269
%token <sc_token> CONTINUE_SY 270
%token <sc_token> DEFAULT_SY 271
%token <sc_token> DISCARD_SY 272
%token <sc_token> DO_SY 273
%token <sc_token> EQ_SY 274
%token <sc_token> ELSE_SY 275
%token <sc_token> ERROR_SY 276
%token <sc_token> EXTERN_SY 277
%token <sc_token> FLOAT_SY 278
%token <sc_literal> FLOATCONST_SY 279
%token <sc_literal> FLOATHCONST_SY 280
%token <sc_literal> FLOATXCONST_SY 281
%token <sc_token> FOR_SY 282
%token <sc_token> GE_SY 283
%token <sc_token> GG_SY 284
%token <sc_token> GOTO_SY 285
%token <sc_ident> IDENT_SY 286
%token <sc_token> IF_SY 287
%token <sc_token> IN_SY 288
%token <sc_token> INLINE_SY 289
%token <sc_token> INOUT_SY 290
%token <sc_token> INT_SY 291
%token <sc_literal> INTCONST_SY 292
%token <sc_token> INTERNAL_SY 293
%token <sc_token> LE_SY 294
%token <sc_token> LL_SY 295
%token <sc_token> MINUSMINUS_SY 296
%token <sc_token> NE_SY 297
%token <sc_token> OR_SY 298
%token <sc_token> OUT_SY 299
%token <sc_token> PACKED_SY 300
%token <sc_token> PLUSPLUS_SY 301
%token <sc_token> RETURN_SY 302
%token <sc_token> STATIC_SY 303
%token <sc_token> STRCONST_SY 304
%token <sc_token> STRUCT_SY 305
%token <sc_token> SWITCH_SY 306
%token <sc_token> TEXOBJ_SY 307
%token <sc_token> THIS_SY 308
%token <sc_token> TYPEDEF_SY 309
%token <sc_ident> TYPEIDENT_SY 310
%token <sc_token> UNIFORM_SY 311
%token <sc_token> VARYING_SY 312
%token <sc_token> VOID_SY 313
%token <sc_token> WHILE_SY 314

%token <sc_token> CHAR_SY 315
%token <sc_token> DOUBLE_SY 316
%token <sc_token> FIXED_SY 317
%token <sc_token> HALF_SY 318
%token <sc_token> INTERFACE_SY 319
%token <sc_token> LONG_SY 320
%token <sc_token> SHORT_SY 321
%token <sc_token> UNSIGNED_SY 322
%token <sc_token> RESERVED_SY 323

/* Geometry topology modifiers and the Cg 2.0 AttribArray reserved
 * word: appended after the last fixed token with explicit increasing
 * values; FIRST_USER_TOKEN_SY stays the final declaration and no prior
 * token was renumbered (stdlib.c embeds token values). */
%token <sc_token> POINT_SY 325
%token <sc_token> LINE_SY 326
%token <sc_token> LINE_ADJ_SY 327
%token <sc_token> TRIANGLE_SY 328
%token <sc_token> TRIANGLE_ADJ_SY 329
%token <sc_token> POINT_OUT_SY 330
%token <sc_token> LINE_OUT_SY 331
%token <sc_token> TRIANGLE_OUT_SY 332
%token <sc_token> ATTRIBARRAY_SY 334
%token <sc_token> FIRST_USER_TOKEN_SY 335  /* Must be last token declaration */

/*************<<<<<<<<<<<<<<<<<<<********************
%type <dummy> abstract_parameter_declaration
**************>>>>>>>>>>>>>>>>>***********************/
%type <dummy> compilation_unit
%type <dummy> compound_header
%type <dummy> compound_tail
%type <dummy> external_declaration
%type <dummy> function_definition
%type <dummy> interface_compound_header
%type <dummy> struct_compound_header

%type <sc_int> function_specifier
%type <sc_int> geometry_modifier
%type <sc_int> in_out
/***
%type <sc_int> integer_constant
***/
%type <sc_int> type_domain
%type <sc_int> type_qualifier
%type <sc_int> storage_class

%type <sc_ident> identifier
%type <sc_ident> member_identifier
%type <sc_ident> profile_specifier
%type <sc_ident> scope_identifier
%type <sc_ident> semantics_identifier
%type <sc_ident> struct_identifier
%type <sc_ident> type_identifier
%type <sc_ident> variable_identifier

%type <sc_decl> abstract_declaration
%type <sc_decl> abstract_declarator
%type <sc_decl> abstract_parameter_list
%type <sc_decl> declarator
%type <sc_decl> basic_declarator
%type <sc_decl> semantic_declarator
%type <sc_decl> function_decl_header
%type <sc_decl> function_definition_header
%type <sc_decl> non_empty_abstract_parameter_list
%type <sc_decl> parameter_declaration
%type <sc_decl> parameter_list

%type <sc_type> abstract_declaration_specifiers
%type <sc_type> abstract_declaration_specifiers2
%type <sc_type> declaration_specifiers
%type <sc_ptype> interface_specifier
%type <sc_ptype> struct_or_connector_header
%type <sc_ptype> struct_or_connector_specifier
/***
%type <sc_type> type_name
***/
%type <sc_ptype> type_specifier
%type <sc_ptype> untagged_struct_header

%type <sc_expr> actual_argument_list
%type <sc_expr> additive_expression
%type <sc_expr> AND_expression
%type <sc_expr> basic_variable
%type <sc_expr> boolean_expression_opt
%type <sc_expr> boolean_scalar_expression
%type <sc_expr> cast_expression
%type <sc_expr> conditional_expression
%type <sc_expr> constant
/***
%type <sc_expr> constant_expression
***/
%type <sc_expr> conditional_test
%type <sc_expr> equality_expression
%type <sc_expr> exclusive_OR_expression
%type <sc_expr> expression
%type <sc_expr> expression_list
%type <sc_expr> inclusive_OR_expression
%type <sc_expr> initializer
%type <sc_expr> initializer_list
%type <sc_expr> logical_AND_expression
%type <sc_expr> logical_OR_expression
%type <sc_expr> multiplicative_expression
%type <sc_expr> actual_argument
%type <sc_expr> non_empty_argument_list
%type <sc_expr> postfix_expression
%type <sc_expr> primary_expression
%type <sc_expr> relational_expression
%type <sc_expr> shift_expression
%type <sc_expr> unary_expression
%type <sc_expr> variable

%type <sc_stmt> annotation
%type <sc_stmt> annotation_decl_list
%type <sc_stmt> balanced_statement
%type <sc_stmt> block_item
%type <sc_stmt> block_item_list
%type <sc_stmt> compound_statement
%type <sc_stmt> dangling_if
%type <sc_stmt> dangling_iteration
%type <sc_stmt> dangling_statement
%type <sc_stmt> declaration
%type <sc_stmt> discard_statement
%type <sc_stmt> expression_statement
%type <sc_stmt> expression_statement2
%type <sc_stmt> for_expression
%type <sc_stmt> for_expression_opt
%type <sc_stmt> if_header
%type <sc_stmt> if_statement
%type <sc_stmt> init_declarator
%type <sc_stmt> init_declarator_list
%type <sc_stmt> interface_member_declaration
%type <sc_stmt> interface_member_declaration_list
%type <sc_stmt> iteration_statement
%type <sc_stmt> jump_statement
%type <sc_stmt> return_statement
%type <sc_stmt> statement
%type <sc_stmt> struct_declaration
%type <sc_stmt> struct_declaration_list

/* Operator precedence rules: */

/* Don't even THINK about it! */

%%

compilation_unit:         external_declaration
                        | compilation_unit external_declaration
;

/****************/
/* Declarations */
/****************/

external_declaration:     declaration
                              { $$ = GlobalInitStatements(CurrentScope, $1); }
                        | function_definition
                              { $$ = 0; }
                        | profile_specifier function_definition
                              { $$ = 0; }
                        | profile_specifier declaration
                              { $$ = GlobalInitStatements(CurrentScope, $2); ClearPendingProfileSpecifier(); }
;

/* A profile specifier names either an exact profile or a registered
 * wildcard before a function return type.  Names shadowed by a typedef
 * reach the parser as TYPEIDENT_SY and therefore keep their ordinary
 * type interpretation; only bare identifiers get here, and the action
 * rejects identifiers that no profile answers to. */

profile_specifier:        identifier
                              { $$ = $1; SetPendingProfileSpecifier(Cg->tokenLoc, $1); }
;

declaration:              declaration_specifiers ';'
                              { $$ = NULL; }
                        | declaration_specifiers init_declarator_list ';'
                              { $$ = $2; }
                        | ERROR_SY ';'
                              { RecordErrorPos(Cg->tokenLoc);
                                ClearPendingGeometryModifiers();
                                $$ = NULL; }
;

abstract_declaration:     abstract_declaration_specifiers abstract_declarator
                              { $$ = $2; }
/***
                        | abstract_declarator
***/
;

declaration_specifiers:   abstract_declaration_specifiers
                              { $$ = $1; }
                        | TYPEDEF_SY abstract_declaration_specifiers
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, TYPE_MISC_TYPEDEF); $$ = $2; }
;

abstract_declaration_specifiers:
                          abstract_declaration_specifiers2
                              { $$ = $1; }
                        | type_qualifier abstract_declaration_specifiers
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, $1); $$ = CurrentDeclTypeSpecs; }
                        | storage_class abstract_declaration_specifiers
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, $1); $$ = CurrentDeclTypeSpecs; }
                        | type_domain abstract_declaration_specifiers
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, $1); $$ = CurrentDeclTypeSpecs; }
                        | in_out abstract_declaration_specifiers
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, $1); $$ = CurrentDeclTypeSpecs; }
                        | function_specifier abstract_declaration_specifiers
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, $1); $$ = CurrentDeclTypeSpecs; }
                        | geometry_modifier abstract_declaration_specifiers
                              /* The modifier token already parked its record;
                               * SetDType initialised it into the scratch dtype
                               * when the base type specifier reduced. */
                              { $$ = CurrentDeclTypeSpecs; }
                        | PACKED_SY abstract_declaration_specifiers
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); $$ = CurrentDeclTypeSpecs; }
;

abstract_declaration_specifiers2:
                          type_specifier
                              { $$ = *SetDType(&CurrentDeclTypeSpecs, $1); }
                        | abstract_declaration_specifiers2 type_qualifier
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, $2); $$ = CurrentDeclTypeSpecs; }
                        | abstract_declaration_specifiers2 storage_class
                              { SetStorageClass(Cg->tokenLoc, &CurrentDeclTypeSpecs, $2); $$ = CurrentDeclTypeSpecs; }
                        | abstract_declaration_specifiers2 type_domain
                              { SetTypeDomain(Cg->tokenLoc, &CurrentDeclTypeSpecs, $2); $$ = CurrentDeclTypeSpecs; }
                        | abstract_declaration_specifiers2 in_out
                              { SetTypeQualifiers(Cg->tokenLoc, &CurrentDeclTypeSpecs, $2); $$ = CurrentDeclTypeSpecs; }
                        | abstract_declaration_specifiers2 function_specifier
                              { SetTypeMisc(Cg->tokenLoc, &CurrentDeclTypeSpecs, $2); $$ = CurrentDeclTypeSpecs; }
                        | abstract_declaration_specifiers2 PACKED_SY
                              { SetTypePacked(Cg->tokenLoc, &CurrentDeclTypeSpecs); $$ = CurrentDeclTypeSpecs; }
;

init_declarator_list:     init_declarator
                              { $$ = $1; }
                        | init_declarator_list ',' init_declarator
                              { $$ = AddStmt($1, $3); }
;

init_declarator:          declarator
                              { $$ = Init_Declarator(Cg->tokenLoc, CurrentScope, $1, NULL); }
                        | declarator '=' initializer
                              { $$ = Init_Declarator(Cg->tokenLoc, CurrentScope, $1, $3); }
;

/*******************/
/* Type Specifiers */
/*******************/

type_specifier:           INT_SY
                              { $$ = LookUpTypeSymbol(NULL, INT_SY); }
                        | FLOAT_SY
                              { $$ = LookUpTypeSymbol(NULL, FLOAT_SY); }
                        | VOID_SY
                              { $$ = LookUpTypeSymbol(NULL, VOID_SY); }
                        | BOOLEAN_SY
                              { $$ = LookUpTypeSymbol(NULL, BOOLEAN_SY); }
                        | TEXOBJ_SY
                              { $$ = LookUpTypeSymbol(NULL, TEXOBJ_SY); }
                        | CHAR_SY
                              { $$ = ResolveScalarTypeSpecifier(Cg->tokenLoc, $1, 0); }
                        | SHORT_SY
                              { $$ = ResolveScalarTypeSpecifier(Cg->tokenLoc, $1, 0); }
                        | LONG_SY
                              { $$ = ResolveScalarTypeSpecifier(Cg->tokenLoc, $1, 0); }
                        | HALF_SY
                              { $$ = ResolveScalarTypeSpecifier(Cg->tokenLoc, $1, 0); }
                        | FIXED_SY
                              { $$ = ResolveScalarTypeSpecifier(Cg->tokenLoc, $1, 0); }
                        | DOUBLE_SY
                              { $$ = ResolveScalarTypeSpecifier(Cg->tokenLoc, $1, 0); }
                        | UNSIGNED_SY
                              { $$ = ResolveScalarTypeSpecifier(Cg->tokenLoc, $1, 0); }
                        | UNSIGNED_SY CHAR_SY
                              { $$ = ResolveScalarTypeSpecifier(Cg->tokenLoc, $2, 1); }
                        | UNSIGNED_SY SHORT_SY
                              { $$ = ResolveScalarTypeSpecifier(Cg->tokenLoc, $2, 1); }
                        | UNSIGNED_SY INT_SY
                              { $$ = ResolveScalarTypeSpecifier(Cg->tokenLoc, $2, 1); }
                        | UNSIGNED_SY LONG_SY
                              { $$ = ResolveScalarTypeSpecifier(Cg->tokenLoc, $2, 1); }
                        | struct_or_connector_specifier
                              { $$ = $1; }
                        | interface_specifier
                              { $$ = $1; }
                        | ATTRIBARRAY_SY '<' type_specifier '>'
                              { $$ = SetAttribArrayType(Cg->tokenLoc, $3); }
                        | type_identifier
                              { $$ = LookUpTypeSymbol(NULL, $1); }
                        | error
                              {
                                ClearPendingGeometryModifiers();
                                SemanticParseError(Cg->tokenLoc, ERROR_S_TYPE_NAME_EXPECTED,
                                                   GetAtomString(atable, Cg->mostRecentToken /* yychar */));
                                $$ = UndefinedType;
                              }
;

/*******************/
/* Type Qualifiers */
/*******************/

type_qualifier:           CONST_SY
                              { $$ = TYPE_QUALIFIER_CONST; }
;

/****************/
/* Type Domains */
/****************/

type_domain:              UNIFORM_SY
                              { $$ = TYPE_DOMAIN_UNIFORM; }
                        | VARYING_SY
                              { $$ = TYPE_DOMAIN_VARYING; }
;

/*******************/
/* Storage Classes */
/*******************/

storage_class:            STATIC_SY
                              { $$ = (int) SC_STATIC; }
                        | EXTERN_SY
                              { $$ = (int) SC_EXTERN; }
;

/**********************/
/* Function Specifier */
/**********************/

function_specifier:       INLINE_SY
                              { $$ = TYPE_MISC_INLINE; }
                        | INTERNAL_SY
                              { $$ = TYPE_MISC_INTERNAL; }
;

/**********************/
/* Geometry Modifiers */
/**********************/

/* Each action parks one topology modifier with its location via the
 * parser-facing wrappers.  A zero result means a diagnostic was already
 * reported at this token; parsing recovers at the declaration boundary.
 * No backend selection or default-output derivation happens here. */

geometry_modifier:
          POINT_SY        { $$ = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_POINT); }
        | LINE_SY         { $$ = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_LINE); }
        | LINE_ADJ_SY     { $$ = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_LINE_ADJACENCY); }
        | TRIANGLE_SY     { $$ = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_TRIANGLE); }
        | TRIANGLE_ADJ_SY { $$ = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY); }
        | POINT_OUT_SY    { $$ = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_POINTS); }
        | LINE_OUT_SY     { $$ = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_LINE_STRIP); }
        | TRIANGLE_OUT_SY { $$ = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP); }
;

/**********/
/* In Out */
/**********/

in_out:                   IN_SY
                              { $$ = TYPE_QUALIFIER_IN; }
                        | OUT_SY
                              { $$ = TYPE_QUALIFIER_OUT; }
                        | INOUT_SY
                              { $$ = TYPE_QUALIFIER_INOUT; }
;

/****************/
/* Struct Types */
/****************/

struct_or_connector_specifier:
                          struct_or_connector_header struct_compound_header struct_declaration_list '}'
                              { $$ = SetStructMembers(Cg->tokenLoc, $1, PopScope());
                                CheckInterfaceConformance(Cg->tokenLoc, $$); }
                        | untagged_struct_header struct_compound_header struct_declaration_list '}'
                              { $$ = SetStructMembers(Cg->tokenLoc, $1, PopScope());
                                CheckInterfaceConformance(Cg->tokenLoc, $$); }
                        | struct_or_connector_header
                              { $$ = $1; }
;

struct_compound_header:   compound_header
                              { CurrentScope->IsStructScope = 1; $$ = $1; }
;

struct_or_connector_header:
                          STRUCT_SY struct_identifier
                              { $$ = StructHeader(Cg->tokenLoc, CurrentScope, 0, $2); }
                        | STRUCT_SY struct_identifier ':' semantics_identifier
                              { $$ = StructHeader(Cg->tokenLoc, CurrentScope, $4, $2); }
                        | STRUCT_SY struct_identifier ':' type_identifier
                              { $$ = SetStructInterface(Cg->tokenLoc, CurrentScope, $2, $4); }
;

struct_identifier:        identifier
                        | type_identifier
;

untagged_struct_header:   STRUCT_SY
                              { $$ = StructHeader(Cg->tokenLoc, CurrentScope, 0, 0); }
;

struct_declaration_list:  struct_declaration
                        | struct_declaration_list struct_declaration
;

struct_declaration:       declaration
                            { $$ = $1; }
                        | function_definition
                            { $$ = NULL; }
;

/******************/
/* Interface Types */
/******************/

interface_specifier:
                          INTERFACE_SY struct_identifier interface_compound_header
                          interface_member_declaration_list
                          '}'
                              { $$ = SetInterfaceMembers(Cg->tokenLoc,
                                                         InterfaceHeader(Cg->tokenLoc, CurrentScope, $2),
                                                         PopScope()); }
;

interface_compound_header:
                          compound_header
                              { CurrentScope->IsStructScope = 1; $$ = $1; }
;

/* Interface members are method prototypes only: a declaration ending in
 * ';'.  Bodies cannot be parsed here, and the completion action rejects
 * anything that is not a function. */

interface_member_declaration_list:
                          interface_member_declaration
                        | interface_member_declaration_list interface_member_declaration
;

interface_member_declaration:
                          declaration_specifiers declarator ';'
                              /* Data members are rejected when the body
                               * completes (SetInterfaceMembers), which
                               * reports at the offending declaration. */
                              { $$ = NULL; }
;

/**************/
/* Type Names */
/**************/

/*** Not used -- Use abstract_declaration" instead ***
type_name:                type_specifier
                        | type_qualifier type_name
                              { $$ = $2; }
;
***/

/***************/
/* Annotations */
/***************/

annotation:               '<' { PushScope(NewScope()); } annotation_decl_list '>'
                              { $$ = $3; PopScope(); }
;

annotation_decl_list:     /* empty */
                              { $$ = 0; }
                        | annotation_decl_list declaration
;

/***************/
/* Declarators */
/***************/

declarator:               semantic_declarator
                              { $$ = $1; }
                        | semantic_declarator annotation
                              { $$ = $1; }
;

semantic_declarator:      basic_declarator
                              { $$ = Declarator(Cg->tokenLoc, $1, 0); }
                        | basic_declarator ':' semantics_identifier
                              { $$ = Declarator(Cg->tokenLoc, $1, $3); }
;

basic_declarator:         identifier
                              { $$ = NewDeclNode(Cg->tokenLoc, $1, &CurrentDeclTypeSpecs); }
                        | basic_declarator '[' INTCONST_SY /* constant_expression */ ']'
                              { $$ = Array_Declarator(Cg->tokenLoc, $1, (int) $3.value.i, 0); }
                        | basic_declarator '[' ']'
                              { $$ = Array_Declarator(Cg->tokenLoc, $1, 0 , 1); }
                        | function_decl_header parameter_list ')'
                              { $$ = SetFunTypeParams(CurrentScope, $1, $2, $2); }
                        | function_decl_header abstract_parameter_list ')'
                              { $$ = SetFunTypeParams(CurrentScope, $1, $2, NULL); }
;

function_decl_header:     basic_declarator '('
                              { $$ = FunctionDeclHeader(&$1->loc, CurrentScope, $1); }
;

abstract_declarator:      /* empty */
                              { $$ = NewDeclNode(Cg->tokenLoc, 0, &CurrentDeclTypeSpecs); }
                        | abstract_declarator '[' INTCONST_SY /* constant_expression */  ']'
                              { $$ = Array_Declarator(Cg->tokenLoc, $1, (int) $3.value.i, 0); }
                        | abstract_declarator '[' ']'
                              { $$ = Array_Declarator(Cg->tokenLoc, $1, 0 , 1); }
/***
 *** This rule causes a major shift reduce conflict with:
 ***
 ***      primary_expression  :;=  type_specifier '(' expression_list ')'
 ***
 *** Cannot be easily factored.  Would force: "( expr -list )" to be merged with "( abstract-param-list )"
 ***
 *** Matches other shading languages' syntax.
 *** Will disallow abstract literal function parameter declarations should we ever defide to
 ***      support function parameters in the future.
 ***
                        | abstract_declarator '(' abstract_parameter_list ')'
***/
;

parameter_list:           parameter_declaration
                              { $$ = $1; }
                        | parameter_list ',' parameter_declaration
                              { $$ = AddDecl($1, $3); }
;

parameter_declaration:    declaration_specifiers declarator
                              { $$ = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, $2, NULL); }
                        | declaration_specifiers declarator '=' initializer
                              { $$ = Param_Init_Declarator(Cg->tokenLoc, CurrentScope, $2, $4); }
;

abstract_parameter_list:  /* empty */
                              { $$ = NULL; }
                        | non_empty_abstract_parameter_list
;

non_empty_abstract_parameter_list:  abstract_declaration
                              {
                                if (IsVoid(&$1->type.type))
                                    CurrentScope->HasVoidParameter = 1;
                                $$ = $1;
                              }
                        | non_empty_abstract_parameter_list ',' abstract_declaration
                              {
                                if (CurrentScope->HasVoidParameter || IsVoid(&$1->type.type)) {
                                    SemanticError(Cg->tokenLoc, ERROR___VOID_NOT_ONLY_PARAM);
                                }
                                $$ = AddDecl($1, $3);
                              }
;

/******************/
/* Initialization */
/******************/

initializer:              expression
                              { $$ = Initializer(Cg->tokenLoc, $1); }
                        | '{' initializer_list '}'
                              { $$ = Initializer(Cg->tokenLoc, $2); }
                        | '{' initializer_list ',' '}'
                              { $$ = Initializer(Cg->tokenLoc, $2); }
                        | '{' '}'
                              /* An empty list: element counting in the
                               * semantic actions decides whether the size
                               * can be inferred ("cannot infer array size").
                               */
                              { $$ = Initializer(Cg->tokenLoc, NULL); }
;

initializer_list:         initializer
                              { $$ = InitializerList(Cg->tokenLoc, $1, NULL); }
                        | initializer_list ',' initializer
                              { $$ = InitializerList(Cg->tokenLoc, $1, $3); }
;

/***************/
/* EXPRESSIONS */
/***************/

/************/
/* Variable */
/************/

variable:                 basic_variable
                              { $$ = $1; }
                        | scope_identifier COLONCOLON_SY basic_variable
                              { $$ = $3; }
;

basic_variable:           variable_identifier
                              { $$ = BasicVariable(Cg->tokenLoc, $1); }
;

/**********************/
/* Primary Expression */
/**********************/

primary_expression:       variable
                        | constant
                        | '(' expression ')'
                              { $$ = $2; }
                        | type_specifier '(' expression_list ')'
                              { $$ = NewVectorConstructor(Cg->tokenLoc, $1, $3); }
;

/*********************/
/* Postfix Operators */
/*********************/

postfix_expression:       primary_expression
                        | postfix_expression PLUSPLUS_SY
                              { $$ = (expr *) NewUnopNode(POSTINC_OP, $1); }
                        | postfix_expression MINUSMINUS_SY
                              { $$ = (expr *) NewUnopNode(POSTDEC_OP, $1); }
                        | postfix_expression '.' member_identifier
                              { $$ = NewMemberSelectorOrSwizzleOrWriteMaskOperator(Cg->tokenLoc, $1, $3); }
                        | postfix_expression '[' expression ']'
                              { $$ = NewIndexOperator(Cg->tokenLoc, $1, $3); }
                        | postfix_expression '(' actual_argument_list ')'
                              { $$ = NewFunctionCallOperator(Cg->tokenLoc, $1, $3); }
;

actual_argument_list:     /* empty */
                                { $$ = NULL; }
                        | non_empty_argument_list
;

non_empty_argument_list:  actual_argument
                              { $$ = ArgumentList(Cg->tokenLoc, NULL, $1); }
                        | non_empty_argument_list ',' actual_argument
                              { $$ = ArgumentList(Cg->tokenLoc, $1, $3); }
;

/* One call argument: a plain expression, or the geometry-operation
 * spelling with an inline binding semantic.  The wrapper keeps the
 * annotation until the selected function decides whether the syntax
 * is legal there; ordinary calls unwrap semantic-zero arguments
 * unchanged. */

actual_argument:          expression
                              { $$ = NewGeometryArgument(Cg->tokenLoc, $1, 0); }
                        | expression ':' semantics_identifier
                              { $$ = NewGeometryArgument(Cg->tokenLoc, $1, $3); }
;

expression_list:          expression
                              { $$ = ExpressionList(Cg->tokenLoc, NULL, $1); }
                        | expression_list ',' expression
                              { $$ = ExpressionList(Cg->tokenLoc, $1, $3); }
;

/*******************/
/* Unary Operators */
/*******************/

unary_expression:         postfix_expression
                        | PLUSPLUS_SY unary_expression
                              { $$ = (expr *) NewUnopNode(PREINC_OP, $2); }
                        | MINUSMINUS_SY unary_expression
                              { $$ = (expr *) NewUnopNode(PREDEC_OP, $2); }
                        | '+' unary_expression
                              { $$ = NewUnaryOperator(Cg->tokenLoc, POS_OP, '+', $2, 0); }
                        | '-' unary_expression
                              { $$ = NewUnaryOperator(Cg->tokenLoc, NEG_OP, '-', $2, 0); }
                        | '!' unary_expression
                              { $$ = NewUnaryOperator(Cg->tokenLoc, BNOT_OP, '!', $2, 0); }
                        | '~' unary_expression
                              { $$ = NewUnaryOperator(Cg->tokenLoc, NOT_OP, '~', $2, 1); }
;

/*****************/
/* Cast Operator */
/*****************/

cast_expression:          unary_expression
/* *** reduce/reduce conflict: (var-ident) (type-ident) ***
                        | '(' type_name ')' cast_expression
*/
                        | '(' abstract_declaration ')' cast_expression
                              { $$ = NewCastOperator(Cg->tokenLoc, $4, GetTypePointer(&$2->loc, &$2->type)); }
;

/****************************/
/* Multiplicative Operators */
/****************************/

multiplicative_expression: cast_expression
                        | multiplicative_expression '*' cast_expression
                              { $$ = NewBinaryOperator(Cg->tokenLoc, MUL_OP, '*', $1, $3, 0); }
                        | multiplicative_expression '/' cast_expression
                              { $$ = NewBinaryOperator(Cg->tokenLoc, DIV_OP, '/', $1, $3, 0); }
                        | multiplicative_expression '%' cast_expression
                              { $$ = NewBinaryOperator(Cg->tokenLoc, MOD_OP, '%', $1, $3, 1); }
;

/**********************/
/* Addative Operators */
/**********************/

additive_expression:      multiplicative_expression
                        | additive_expression '+' multiplicative_expression
                              { $$ = NewBinaryOperator(Cg->tokenLoc, ADD_OP, '+', $1, $3, 0); }
                        | additive_expression '-' multiplicative_expression
                              { $$ = NewBinaryOperator(Cg->tokenLoc, SUB_OP, '-', $1, $3, 0); }
;

/***************************/
/* Bitwise Shift Operators */
/***************************/

shift_expression:         additive_expression
                        | shift_expression LL_SY additive_expression
                              { $$ = NewBinaryOperator(Cg->tokenLoc, SHL_OP, LL_SY, $1, $3, 1); }
                        | shift_expression GG_SY additive_expression
                              { $$ = NewBinaryOperator(Cg->tokenLoc, SHR_OP, GG_SY, $1, $3, 1); }
;

/************************/
/* Relational Operators */
/************************/

relational_expression:    shift_expression
                        | relational_expression '<' shift_expression
                              { $$ = NewBinaryComparisonOperator(Cg->tokenLoc, LT_OP, '<', $1, $3); }
                        | relational_expression '>' shift_expression
                              { $$ = NewBinaryComparisonOperator(Cg->tokenLoc, GT_OP, '>', $1, $3); }
                        | relational_expression LE_SY shift_expression
                              { $$ = NewBinaryComparisonOperator(Cg->tokenLoc, LE_OP, LE_SY, $1, $3); }
                        | relational_expression GE_SY shift_expression
                              { $$ = NewBinaryComparisonOperator(Cg->tokenLoc, GE_OP, GE_SY, $1, $3); }
;

/**********************/
/* Equality Operators */
/**********************/

equality_expression:      relational_expression
                        | equality_expression EQ_SY relational_expression
                              { $$ = NewBinaryComparisonOperator(Cg->tokenLoc, EQ_OP, EQ_SY, $1, $3); }
                        | equality_expression NE_SY relational_expression
                              { $$ = NewBinaryComparisonOperator(Cg->tokenLoc, NE_OP, NE_SY, $1, $3); }
;

/************************/
/* Bitwise AND Operator */
/************************/

AND_expression:           equality_expression
                        | AND_expression '&' equality_expression
                              { $$ = NewBinaryOperator(Cg->tokenLoc, AND_OP, '&', $1, $3, 1); }
;

/*********************************/
/* Bitwise Exclusive OR Operator */
/*********************************/

exclusive_OR_expression: AND_expression
                        | exclusive_OR_expression '^' AND_expression
                              { $$ = NewBinaryOperator(Cg->tokenLoc, XOR_OP, '^', $1, $3, 1); }
;

/*********************************/
/* Bitwise Inclusive OR Operator */
/*********************************/

inclusive_OR_expression:  exclusive_OR_expression
                        | inclusive_OR_expression '|' exclusive_OR_expression
                              { $$ = NewBinaryOperator(Cg->tokenLoc, OR_OP, '|', $1, $3, 1); }
;

/************************/
/* Logical AND Operator */
/************************/

logical_AND_expression:   inclusive_OR_expression
                        | logical_AND_expression AND_SY inclusive_OR_expression
                              { $$ = NewBinaryBooleanOperator(Cg->tokenLoc, BAND_OP, AND_SY, $1, $3); }
;

/***********************/
/* Logical OR Operator */
/***********************/

logical_OR_expression:    logical_AND_expression
                        | logical_OR_expression OR_SY logical_AND_expression
                              { $$ = NewBinaryBooleanOperator(Cg->tokenLoc, BOR_OP, OR_SY, $1, $3); }
;

/************************/
/* Conditional Operator */
/************************/

conditional_expression:   logical_OR_expression
                        | conditional_test '?' expression ':' conditional_expression
                              { $$ = NewConditionalOperator(Cg->tokenLoc, $1, $3, $5); }
;

conditional_test:         logical_OR_expression
                              {  $$ = CheckBooleanExpr(Cg->tokenLoc, $1, 1); }
;

/***********************/
/* Assignment operator */
/***********************/

expression:               conditional_expression
/***
                        | basic_variable '=' expression
                              { $$ = (expr *) NewBinopNode(ASSIGN_OP, $1, $3); }
***/
;

/***********************/
/* Function Definition */
/***********************/

function_definition:      function_definition_header block_item_list '}'
                              { DefineFunction(Cg->tokenLoc, CurrentScope, $1, $2); PopScope();
                                ResumeStructScopeAfterMethodBody(); }
                        | function_definition_header '}'
                              { DefineFunction(Cg->tokenLoc, CurrentScope, $1, NULL); PopScope();
                                ResumeStructScopeAfterMethodBody(); }
;

function_definition_header: declaration_specifiers declarator '{'
                              { $$ = Function_Definition_Header(Cg->tokenLoc, $2); }
;

/*************/
/* Statement */
/*************/

statement:                balanced_statement
                        | dangling_statement
;

balanced_statement:       compound_statement
                        | discard_statement
                        | expression_statement
                        | iteration_statement
                        | if_statement
                        | jump_statement
                        | return_statement
;

dangling_statement:       dangling_if
                        | dangling_iteration
;

/*********************/
/* Default Statement */
/*********************/

discard_statement:        DISCARD_SY ';'
                              { $$ = (stmt *) NewDiscardStmt(Cg->tokenLoc, NULL); }
                        | DISCARD_SY expression ';'
                              { $$ = (stmt *) NewDiscardStmt(Cg->tokenLoc, CheckBooleanExpr(Cg->tokenLoc, $2, 1)); }
;

/******************/
/* Jump Statement */
/******************/

jump_statement:           BREAK_SY ';'
                              { $$ = (stmt *) NewSimpleStmt(Cg->tokenLoc, BREAK_STMT); }
                        | CONTINUE_SY ';'
                              { $$ = (stmt *) NewSimpleStmt(Cg->tokenLoc, CONTINUE_STMT); }
;

/****************/
/* If Statement */
/****************/

if_statement:             if_header balanced_statement ELSE_SY balanced_statement
                              { $$ = (stmt *) SetThenElseStmts(Cg->tokenLoc, $1, $2, $4); }
;

dangling_if:              if_header statement
                              { $$ = (stmt *) SetThenElseStmts(Cg->tokenLoc, $1, $2, NULL); }
                        | if_header balanced_statement ELSE_SY dangling_statement
                              { $$ = (stmt *) SetThenElseStmts(Cg->tokenLoc, $1, $2, $4); }
;

if_header:                IF_SY '(' boolean_scalar_expression ')'
                              { $$ = (stmt *) NewIfStmt(Cg->tokenLoc, $3, NULL, NULL); ; }
;

/**********************/
/* Compound Statement */
/**********************/

compound_statement:       compound_header block_item_list compound_tail
                              { $$ = (stmt *) NewBlockStmt(Cg->tokenLoc, $2); }
                        | compound_header compound_tail
                              { $$ = NULL; }
;

compound_header:          '{'
                              { PushScope(NewScope()); CurrentScope->funindex = NextFunctionIndex; }
;

compound_tail:            '}'
                              {
                                if (Cg->options.DumpParseTree)
                                    PrintScopeDeclarations();
                                PopScope();
                              }
;

block_item_list:          block_item
                        | block_item_list block_item
                              { $$ = AddStmt($1, $2); }
;

block_item:               declaration
                        | statement
                              { $$ = CheckStmt($1); }
;

/************************/
/* Expression Stetement */
/************************/

expression_statement:     expression_statement2 ';'
                        | ';'
                              { $$ = NULL; }
;

expression_statement2:    postfix_expression /* basic_variable */ '=' expression
                              { $$ = NewSimpleAssignmentStmt(Cg->tokenLoc, $1, $3, 0); }
                        | expression
                              { $$ = (stmt *) NewExprStmt(Cg->tokenLoc, $1); }
                        | postfix_expression ASSIGNMINUS_SY expression
                              { $$ = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMINUS_OP, $1, $3); }
                        | postfix_expression ASSIGNMOD_SY expression
                              { $$ = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNMOD_OP, $1, $3); }
                        | postfix_expression ASSIGNPLUS_SY expression
                              { $$ = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNPLUS_OP, $1, $3); }
                        | postfix_expression ASSIGNSLASH_SY expression
                              { $$ = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSLASH_OP, $1, $3); }
                        | postfix_expression ASSIGNSTAR_SY expression
                              { $$ = NewCompoundAssignmentStmt(Cg->tokenLoc, ASSIGNSTAR_OP, $1, $3); }
;

/***********************/
/* Iteration Statement */
/***********************/

iteration_statement:      WHILE_SY '(' boolean_scalar_expression ')' balanced_statement
                              { $$ = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, $3, $5); }
                        | DO_SY statement WHILE_SY '(' boolean_scalar_expression ')' ';'
                              { $$ = (stmt *) NewWhileStmt(Cg->tokenLoc, DO_STMT, $5, $2); }
                        | FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' balanced_statement
                              { $$ = (stmt *) NewForStmt(Cg->tokenLoc, $3, $5, $7, $9); }
;

dangling_iteration:       WHILE_SY '(' boolean_scalar_expression ')' dangling_statement
                              { $$ = (stmt *) NewWhileStmt(Cg->tokenLoc, WHILE_STMT, $3, $5); }
                        | FOR_SY '(' for_expression_opt ';' boolean_expression_opt ';' for_expression_opt ')' dangling_statement
                              { $$ = (stmt *) NewForStmt(Cg->tokenLoc, $3, $5, $7, $9); }
;

boolean_scalar_expression:
                          expression
                              {  $$ = CheckBooleanExpr(Cg->tokenLoc, $1, 0); }
;

for_expression_opt:       for_expression
                        | /* empty */
                              { $$ = NULL; }
;

for_expression:           expression_statement2
                        | for_expression ',' expression_statement2
                              {
                                stmt *lstmt = $1;
                                if (lstmt) {
                                    while (lstmt->exprst.next)
                                        lstmt = lstmt->exprst.next;
                                    lstmt->exprst.next = $3;
                                    $$ = $1;
                                } else {
                                    $$ = $3;
                                }
                              }
;

boolean_expression_opt: boolean_scalar_expression
                        | /* empty */
                              { $$ = NULL; }
;

/*******************/
/*Return Statement */
/*******************/

return_statement:         RETURN_SY expression ';'
                              { $$ = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, $2); }
                        | RETURN_SY ';'
                              { $$ = (stmt *) NewReturnStmt(Cg->tokenLoc, CurrentScope, NULL); }
;

/*********/
/* Misc. */
/*********/

member_identifier:        identifier
;

scope_identifier:         identifier
;

semantics_identifier:     identifier
;

type_identifier:          TYPEIDENT_SY
;

variable_identifier:      identifier
;

identifier:               IDENT_SY
                              { $$ = $1; }
                        | RESERVED_SY
                              {
                                /* SemanticError, not SemanticParseError: the
                                 * latter is gated by AllowSemanticParseErrors */
                                SemanticError(Cg->tokenLoc, ERROR_S_RESERVED_WORD,
                                              GetAtomString(atable, $1));
                                $$ = $1;
                              }
;

constant:                 INTCONST_SY /* Temporary! */
                              { $$ = (expr *) NewNumericConstNode(ICONST_OP, &$1); }
                        | CFLOATCONST_SY /* Temporary! */
                              { $$ = (expr *) NewNumericConstNode(FCONST_OP, &$1); }
                        | FLOATCONST_SY /* Temporary! */
                              { $$ = (expr *) NewNumericConstNode(FCONST_OP, &$1); }
                        | FLOATHCONST_SY /* Temporary! */
                              { $$ = (expr *) NewNumericConstNode(FCONST_OP, &$1); }
                        | FLOATXCONST_SY /* Temporary! */
                              { $$ = (expr *) NewNumericConstNode(FCONST_OP, &$1); }
;

/***
integer_constant:         INTCONST_SY
                              { $$ = $1; }
;
***/

/***
constant_expression:      expression
;
***/

%%

