/*
 * declare.c
 *
 * Ullrich von Bassewitz, 20.06.1998
 */



#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "anonname.h"
#include "codegen.h"
#include "datatype.h"
#include "error.h"
#include "expr.h"
#include "funcdesc.h"
#include "function.h"
#include "global.h"
#include "litpool.h"
#include "mem.h"
#include "pragma.h"
#include "scanner.h"
#include "symtab.h"
#include "declare.h"



/*****************************************************************************/
/*				   Forwards				     */
/*****************************************************************************/



static void ParseTypeSpec (DeclSpec* D, int Default);
/* Parse a type specificier */



/*****************************************************************************/
/*			      internal functions			     */
/*****************************************************************************/



static void optional_modifiers (void)
/* Eat optional "const" or "volatile" tokens */
{
    while (curtok == CONST || curtok == VOLATILE) {
	/* Skip it */
 	gettok ();
    }
}



static void optionalint (void)
/* Eat an optional "int" token */
{
    if (curtok == INT) {
	/* Skip it */
 	gettok ();
    }
}



static void optionalsigned (void)
/* Eat an optional "signed" token */
{
    if (curtok == SIGNED) {
	/* Skip it */
 	gettok ();
    }
}



static void InitDeclSpec (DeclSpec* D)
/* Initialize the DeclSpec struct for use */
{
    D->StorageClass   	= 0;
    D->Type[0]	    	= T_END;
    D->Flags 	    	= 0;
}



static void InitDeclaration (Declaration* D)
/* Initialize the Declaration struct for use */
{
    D->Ident[0]		= '\0';
    D->Type[0]		= T_END;
    D->T		= D->Type;
}



static void ParseStorageClass (DeclSpec* D, unsigned DefStorage)
/* Parse a storage class */
{
    /* Assume we're using an explicit storage class */
    D->Flags &= ~DS_DEF_STORAGE;

    /* Check the storage class given */
    switch (curtok) {

     	case EXTERN:
	    D->StorageClass = SC_EXTERN | SC_STATIC;
     	    gettok ();
    	    break;

    	case STATIC:
	    D->StorageClass = SC_STATIC;
    	    gettok ();
    	    break;

    	case REGISTER:
    	    D->StorageClass = SC_REGISTER | SC_STATIC;
    	    gettok ();
	    break;

    	case AUTO:
	    D->StorageClass = SC_AUTO;
    	    gettok ();
	    break;

	case TYPEDEF:
	    D->StorageClass = SC_TYPEDEF;
	    gettok ();
	    break;

    	default:
	    /* No storage class given, use default */
       	    D->Flags |= DS_DEF_STORAGE;
    	    D->StorageClass = DefStorage;
	    break;
    }
}



static void ParseEnumDecl (void)
/* Process an enum declaration . */
{
    int EnumVal;
    ident Ident;

    /* Accept forward definitions */
    if (curtok != LCURLY) {
   	return;
    }

    /* Skip the opening curly brace */
    gettok ();

    /* Read the enum tags */
    EnumVal = 0;
    while (curtok != RCURLY) {

	/* We expect an identifier */
   	if (curtok != IDENT) {
   	    Error (ERR_IDENT_EXPECTED);
   	    continue;
   	}

	/* Remember the identifier and skip it */
	strcpy (Ident, CurTok.Ident);
   	gettok ();

	/* Check for an assigned value */
   	if (curtok == ASGN) {
    	    struct expent lval;
   	    gettok ();
   	    constexpr (&lval);
   	    EnumVal = lval.e_const;
    	}

	/* Add an entry to the symbol table */
	AddEnumSym (Ident, EnumVal++);

	/* Check for end of definition */
    	if (curtok != COMMA)
    	    break;
    	gettok ();
    }
    ConsumeRCurly ();
}



static SymEntry* ParseStructDecl (const char* Name, type StructType)
/* Parse a struct/union declaration. */
{

    unsigned Size;
    unsigned Offs;
    SymTable* FieldTab;
    SymEntry* Entry;


    if (curtok != LCURLY) {
    	/* Just a forward declaration. Try to find a struct with the given
	 * name. If there is none, insert a forward declaration into the
	 * current lexical level.
	 */
	Entry = FindStructSym (Name);
	if (Entry == 0 || Entry->Flags != SC_STRUCT) {
	    Entry = AddStructSym (Name, 0, 0);
	}
    	return Entry;	  
    }

    /* Add a forward declaration for the struct in the current lexical level */
    Entry = AddStructSym (Name, 0, 0);

    /* Skip the curly brace */
    gettok ();

    /* Enter a new lexical level for the struct */
    EnterStructLevel ();

    /* Parse struct fields */
    Size = 0;
    while (curtok != RCURLY) {

	/* Get the type of the entry */
	DeclSpec Spec;
	InitDeclSpec (&Spec);
	ParseTypeSpec (&Spec, -1);

	/* Read fields with this type */
	while (1) {

	    /* Get type and name of the struct field */
	    Declaration Decl;
	    ParseDecl (&Spec, &Decl, 0);

	    /* Add a field entry to the table */
	    AddLocalSym (Decl.Ident, Decl.Type, SC_SFLD, (StructType == T_STRUCT)? Size : 0);

	    /* Calculate offset of next field/size of the union */
    	    Offs = SizeOf (Decl.Type);
	    if (StructType == T_STRUCT) {
	       	Size += Offs;
	    } else {
	       	if (Offs > Size) {
	       	    Size = Offs;
	       	}
	    }

	    if (curtok != COMMA)
	       	break;
	    gettok ();
	}
	ConsumeSemi ();
    }

    /* Skip the closing brace */
    gettok ();

    /* Remember the symbol table and leave the struct level */
    FieldTab = GetSymTab ();
    LeaveStructLevel ();

    /* Make a real entry from the forward decl and return it */
    return AddStructSym (Name, Size, FieldTab);
}



static void ParseTypeSpec (DeclSpec* D, int Default)
/* Parse a type specificier */
{
    ident Ident;
    SymEntry* Entry;
    type StructType;

    /* Assume have an explicit type */
    D->Flags &= ~DS_DEF_TYPE;

    /* Skip const or volatile modifiers if needed */
    optional_modifiers ();

    /* Look at the data type */
    switch (curtok) {

    	case VOID:
    	    gettok ();
	    D->Type[0] = T_VOID;
	    D->Type[1] = T_END;
    	    break;

    	case CHAR:
    	    gettok ();
	    D->Type[0] = GetDefaultChar();
	    D->Type[1] = T_END;
	    break;

    	case LONG:
    	    gettok ();
    	    if (curtok == UNSIGNED) {
    	     	gettok ();
    	     	optionalint ();
		D->Type[0] = T_ULONG;
		D->Type[1] = T_END;
    	    } else {
    	   	optionalsigned ();
    	     	optionalint ();
		D->Type[0] = T_LONG;
		D->Type[1] = T_END;
    	    }
	    break;

    	case SHORT:
    	    gettok ();
    	    if (curtok == UNSIGNED) {
    	   	gettok ();
    	   	optionalint ();
		D->Type[0] = T_USHORT;
		D->Type[1] = T_END;
    	    } else {
    		optionalsigned ();
    		optionalint ();
		D->Type[0] = T_SHORT;
		D->Type[1] = T_END;
    	    }
	    break;

    	case INT:
    	    gettok ();
	    D->Type[0] = T_INT;
	    D->Type[1] = T_END;
    	    break;

       case SIGNED:
    	    gettok ();
    	    switch (curtok) {

       		case CHAR:
    		    gettok ();
		    D->Type[0] = T_CHAR;
		    D->Type[1] = T_END;
    		    break;

    		case SHORT:
    		    gettok ();
    		    optionalint ();
		    D->Type[0] = T_SHORT;
		    D->Type[1] = T_END;
    		    break;

    		case LONG:
    		    gettok ();
    	 	    optionalint ();
		    D->Type[0] = T_LONG;
		    D->Type[1] = T_END;
    		    break;

    		case INT:
    		    gettok ();
    		    /* FALL THROUGH */

    		default:
		    D->Type[0] = T_INT;
		    D->Type[1] = T_END;
	  	    break;
    	    }
	    break;

    	case UNSIGNED:
    	    gettok ();
    	    switch (curtok) {

       	 	case CHAR:
    		    gettok ();
		    D->Type[0] = T_UCHAR;
		    D->Type[1] = T_END;
    		    break;

    	    	case SHORT:
    		    gettok ();
    		    optionalint ();
		    D->Type[0] = T_USHORT;
		    D->Type[1] = T_END;
    		    break;

    		case LONG:
    		    gettok ();
    		    optionalint ();
		    D->Type[0] = T_ULONG;
		    D->Type[1] = T_END;
    		    break;

    		case INT:
    		    gettok ();
    		    /* FALL THROUGH */

    		default:
		    D->Type[0] = T_UINT;
		    D->Type[1] = T_END;
		    break;
    	    }
	    break;

    	case STRUCT:
    	case UNION:
    	    StructType = (curtok == STRUCT)? T_STRUCT : T_UNION;
    	    gettok ();
    	    if (curtok == IDENT) {
	 	strcpy (Ident, CurTok.Ident);
    	 	gettok ();
    	    } else {
	 	AnonName (Ident, (StructType == T_STRUCT)? "struct" : "union");
    	    }
	    /* Declare the struct in the current scope */
    	    Entry = ParseStructDecl (Ident, StructType);
       	    /* Encode the struct entry into the type */
	    D->Type[0] = StructType;
	    EncodePtr (D->Type+1, Entry);
	    D->Type[DECODE_SIZE+1] = T_END;
	    break;

    	case ENUM:
    	    gettok ();
	    if (curtok != LCURLY) {
	 	/* Named enum */
    	        Consume (IDENT, ERR_IDENT_EXPECTED);
	    }
    	    ParseEnumDecl ();
	    D->Type[0] = T_INT;
	    D->Type[1] = T_END;
    	    break;

        case IDENT:
	    Entry = FindSym (CurTok.Ident);
	    if (Entry && IsTypeDef (Entry)) {
       	       	/* It's a typedef */
    	      	gettok ();
		TypeCpy (D->Type, Entry->Type);
    	      	break;
    	    }
    	    /* FALL THROUGH */

    	default:
    	    if (Default < 0) {
    		Error (ERR_TYPE_EXPECTED);
		D->Type[0] = T_INT;
		D->Type[1] = T_END;
    	    } else {
		D->Flags  |= DS_DEF_TYPE;
		D->Type[0] = (type) Default;
		D->Type[1] = T_END;
	    }
	    break;
    }
}



static FuncDesc* ParseFuncDecl (void)
/* Parse the argument list of a function. */
{
    unsigned UnnamedCount = 0;
    unsigned Offs;
    SymEntry* Sym;
    type* Type;

    /* Create a new function descriptor */
    FuncDesc* F = NewFuncDesc ();

    /* Enter a new lexical level */
    EnterFunctionLevel ();

    /* Check for an empty or void parameter list */
    if (curtok == RPAREN) {
	/* Parameter list is empty */
 	F->Flags |= (FD_EMPTY | FD_ELLIPSIS);
    } else if (curtok == VOID && nxttok == RPAREN) {
	/* Parameter list declared as void */
	gettok ();
	F->Flags |= FD_VOID_PARAM;
    }

    /* Parse params */
    while (curtok != RPAREN) {

	DeclSpec Spec;
	Declaration Decl;

      	/* Allow an ellipsis as last parameter */
	if (curtok == ELLIPSIS) {
	    gettok ();
	    F->Flags |= FD_ELLIPSIS;
	    break;
	}

	/* Read the declaration specifier */
	ParseDeclSpec (&Spec, SC_AUTO, T_INT);

       	/* We accept only auto and register as storage class specifiers, but
	 * we ignore all this and use auto.
	 */
	if ((Spec.StorageClass & SC_AUTO) == 0 &&
	    (Spec.StorageClass & SC_REGISTER) == 0) {
	    Error (ERR_ILLEGAL_STORAGE_CLASS);
	}
	Spec.StorageClass = SC_AUTO | SC_PARAM | SC_DEF;

	/* Allow parameters without a name, but remember if we had some to
      	 * eventually print an error message later.
	 */
	ParseDecl (&Spec, &Decl, DM_ACCEPT_IDENT);
       	if (Decl.Ident[0] == '\0') {

	    /* Unnamed symbol. Generate a name that is not user accessible,
	     * then handle the symbol normal.
	     */
	    AnonName (Decl.Ident, "param");
	    ++UnnamedCount;

	    /* Clear defined bit on nonames */
	    Spec.StorageClass &= ~SC_DEF;
	}

	/* If the parameter is an array, convert it to a pointer */
	Type = Decl.Type;
	if (IsArray (Type)) {
	    Type += DECODE_SIZE;
	    Type[0] = T_PTR;
	}

	/* Create a symbol table entry */
	AddLocalSym (Decl.Ident, Type, Spec.StorageClass, 0);

	/* Count arguments */
       	++F->ParamCount;
	F->ParamSize += SizeOf (Type);

	/* Check for more parameters */
	if (curtok == COMMA) {
	    gettok ();
	} else {
	    break;
	}
    }

    /* Skip right paren. We must explicitly check for one here, since some of
     * the breaks above bail out without checking.
     */
    ConsumeRParen ();

    /* Assign offsets. If the function has a variable parameter list,
     * there's one additional byte (the arg size).
     */
    Offs = (F->Flags & FD_ELLIPSIS)? 1 : 0;
    Sym = GetSymTab()->SymTail;
    while (Sym) {
     	Sym->V.Offs = Offs;
       	Offs += SizeOf (Sym->Type);
	Sym = Sym->PrevSym;
    }

    /* Check if this is a function definition */
    if (curtok == LCURLY) {
	/* Print an error if in strict ANSI mode and we have unnamed
	 * parameters.
	 */
	if (ANSI && UnnamedCount > 0) {
	    Error (ERR_MISSING_PARAM_NAME);
	}
    }

    /* Leave the lexical level remembering the symbol tables */
    RememberFunctionLevel (F);

    /* Return the function descriptor */
    return F;
}



static void Decl (Declaration* D, unsigned Mode)
/* Recursively process declarators. Build a type array in reverse order. */
{
    if (curtok == STAR) {
       	gettok ();
	/* Allow optional const or volatile modifiers */
	optional_modifiers ();
       	Decl (D, Mode);
       	*D->T++ = T_PTR;
       	return;
    } else if (curtok == LPAREN) {
       	gettok ();
       	Decl (D, Mode);
       	ConsumeRParen ();
    } else if (curtok == FASTCALL) {
	/* Remember the current type pointer */
	type* T = D->T;
	/* Skip the fastcall token */
      	gettok ();
	/* Parse the function */
	Decl (D, Mode);
	/* Set the fastcall flag */
	if (!IsFunc (T)) {
	    Error (ERR_ILLEGAL_MODIFIER);
	} else {
	    FuncDesc* F = DecodePtr (T+1);
       	    F->Flags |= FD_FASTCALL;
	}
	return;
    } else {
	/* Things depend on Mode now:
       	 *  - Mode == DM_NEED_IDENT means:
	 *   	we *must* have a type and a variable identifer.
	 *  - Mode == DM_NO_IDENT means:
	 *	we must have a type but no variable identifer
	 *     	(if there is one, it's not read).
	 *  - Mode == DM_ACCEPT_IDENT means:
	 *	we *may* have an identifier. If there is an identifier,
	 *	it is read, but it is no error, if there is none.
	 */
	if (Mode == DM_NO_IDENT) {
	    D->Ident[0] = '\0';
	} else if (curtok == IDENT) {
       	    strcpy (D->Ident, CurTok.Ident);
	    gettok ();
	} else {
    	    if (Mode == DM_NEED_IDENT) {
	       	Error (ERR_IDENT_EXPECTED);
	    }
	    D->Ident[0] = '\0';
	    return;
	}
    }

    while (curtok == LBRACK || curtok == LPAREN) {
       	if (curtok == LPAREN) {
       	    /* Function declaration */
	    FuncDesc* F;
       	    gettok ();
	    /* Parse the function declaration */
       	    F = ParseFuncDecl ();
	    *D->T++ = T_FUNC;
	    EncodePtr (D->T, F);
	    D->T += DECODE_SIZE;
       	} else {
	    /* Array declaration */
       	    unsigned long Size = 0;
       	    gettok ();
	    /* Read the size if it is given */
       	    if (curtok != RBRACK) {
    	     	struct expent lval;
       	       	constexpr (&lval);
       	       	Size = lval.e_const;
       	    }
       	    ConsumeRBrack ();
       	    *D->T++ = T_ARRAY;
       	    Encode (D->T, Size);
       	    D->T += DECODE_SIZE;
       	}
    }
}



/*****************************************************************************/
/*	       	      	       	     code	     			     */
/*****************************************************************************/



type* ParseType (type* Type)
/* Parse a complete type specification */
{
    DeclSpec Spec;
    Declaration Decl;

    /* Get a type without a default */
    InitDeclSpec (&Spec);
    ParseTypeSpec (&Spec, -1);

    /* Parse additional declarators */
    InitDeclaration (&Decl);
    ParseDecl (&Spec, &Decl, DM_NO_IDENT);

    /* Copy the type to the target buffer */
    TypeCpy (Type, Decl.Type);

    /* Return a pointer to the target buffer */
    return Type;
}



void ParseDecl (const DeclSpec* Spec, Declaration* D, unsigned Mode)
/* Parse a variable, type or function declaration */
{
    /* Initialize the Declaration struct */
    InitDeclaration (D);

    /* Get additional declarators and the identifier */
    Decl (D, Mode);

    /* Add the base type. */
    TypeCpy (D->T, Spec->Type);

    /* Check the size of the generated type */
    if (!IsFunc (D->Type) && SizeOf (D->Type) >= 0x10000) {
     	Error (ERR_ILLEGAL_SIZE);
    }
}



void ParseDeclSpec (DeclSpec* D, unsigned DefStorage, int DefType)
/* Parse a declaration specification */
{
    /* Initialize the DeclSpec struct */
    InitDeclSpec (D);

    /* First, get the storage class specifier for this declaration */
    ParseStorageClass (D, DefStorage);

    /* Parse the type specifiers */
    ParseTypeSpec (D, DefType);
}



static void ParseVoidInit (void)
/* Parse an initialization of a void variable (special cc65 extension) */
{
    struct expent lval;

    /* Allow an arbitrary list of values */
    ConsumeLCurly ();
    do {
	constexpr (&lval);
	switch (lval.e_tptr[0]) {

	    case T_CHAR:
	    case T_UCHAR:
		if ((lval.e_flags & E_MCTYPE) == E_TCONST) {
		    /* Make it byte sized */
		    lval.e_const &= 0xFF;
		}
		DefineData (&lval);
		break;

	    case T_SHORT:
	    case T_USHORT:
	    case T_INT:
	    case T_UINT:
	    case T_PTR:
	    case T_ARRAY:
		if ((lval.e_flags & E_MCTYPE) == E_TCONST) {
		    /* Make it word sized */
		    lval.e_const &= 0xFFFF;
		}
		DefineData (&lval);
		break;

	    case T_LONG:
	    case T_ULONG:
		DefineData (&lval);
		break;

	    default:
		Error (ERR_ILLEGAL_TYPE);
	    	break;

	}

	if (curtok != COMMA) {
	    break;
	}
	gettok ();

    } while (curtok != RCURLY);

    ConsumeRCurly ();
}



static void ParseStructInit (type* Type)
/* Parse initialization of a struct or union */
{
    SymEntry* Entry;
    SymTable* Tab;

    /* Consume the opening curly brace */
    ConsumeLCurly ();

    /* Get a pointer to the struct entry from the type */
    Entry = (SymEntry*) Decode (Type + 1);

    /* Check if this struct definition has a field table. If it doesn't, it
     * is an incomplete definition.
     */
    Tab = Entry->V.S.SymTab;
    if (Tab == 0) {
     	Error (ERR_INIT_INCOMPLETE_TYPE);
	/* Returning here will cause lots of errors, but recovery is difficult */
	return;
    }

    /* Get a pointer to the list of symbols */
    Entry = Tab->SymHead;
    while (curtok != RCURLY) {
 	if (Entry == NULL) {
 	    Error (ERR_TOO_MANY_INITIALIZERS);
 	    return;
 	}
 	ParseInit (Entry->Type);
 	Entry = Entry->NextSym;
 	if (curtok != COMMA)
 	    break;
 	gettok ();
    }

    /* Consume the closing curly brace */
    ConsumeRCurly ();

    /* If there are struct fields left, reserve additional storage */
    while (Entry) {
 	g_zerobytes (SizeOf (Entry->Type));
 	Entry = Entry->NextSym;
    }
}





void ParseInit (type *tptr)
/* Parse initialization of variables */
{
    int count;
    struct expent lval;
    type* t;
    const char* str;
    int sz;

    switch (*tptr) {

     	case T_CHAR:
     	case T_UCHAR:
     	    constexpr (&lval);
	    if ((lval.e_flags & E_MCTYPE) == E_TCONST) {
	    	/* Make it byte sized */
	    	lval.e_const &= 0xFF;
	    }
	    assignadjust (tptr, &lval);
	    DefineData (&lval);
     	    break;

    	case T_SHORT:
    	case T_USHORT:
     	case T_INT:
     	case T_UINT:
     	case T_PTR:
     	    constexpr (&lval);
	    if ((lval.e_flags & E_MCTYPE) == E_TCONST) {
	    	/* Make it word sized */
	    	lval.e_const &= 0xFFFF;
	    }
	    assignadjust (tptr, &lval);
	    DefineData (&lval);
     	    break;

    	case T_LONG:
    	case T_ULONG:
	    constexpr (&lval);
	    if ((lval.e_flags & E_MCTYPE) == E_TCONST) {
	    	/* Make it long sized */
	    	lval.e_const &= 0xFFFFFFFF;
	    }
	    assignadjust (tptr, &lval);
	    DefineData (&lval);
     	    break;

     	case T_ARRAY:
     	    sz = Decode (tptr + 1);
	    t = tptr + DECODE_SIZE + 1;
       	    if ((t [0] == T_CHAR || t [0] == T_UCHAR) && curtok == SCONST) {
     	     	str = GetLiteral (curval);
     	     	count = strlen (str) + 1;
	    	TranslateLiteralPool (curval);	/* Translate into target charset */
     	     	g_defbytes (str, count);
     	     	ResetLiteralOffs (curval);	/* Remove string from pool */
     	     	gettok ();
     	    } else {
     	     	ConsumeLCurly ();
     	     	count = 0;
     	     	while (curtok != RCURLY) {
     	     	    ParseInit (tptr + DECODE_SIZE + 1);
     	     	    ++count;
     	     	    if (curtok != COMMA)
     	     	 	break;
     	     	    gettok ();
     	     	}
     	     	ConsumeRCurly ();
     	    }
     	    if (sz == 0) {
     	     	Encode (tptr + 1, count);
     	    } else if (count < sz) {
     	     	g_zerobytes ((sz - count) * SizeOf (tptr + DECODE_SIZE + 1));
     	    } else if (count > sz) {
     	     	Error (ERR_TOO_MANY_INITIALIZERS);
     	    }
     	    break;

        case T_STRUCT:
        case T_UNION:
	    ParseStructInit (tptr);
     	    break;

	case T_VOID:
	    if (!ANSI) {
	    	/* Special cc65 extension in non ANSI mode */
	      	ParseVoidInit ();
		break;
	    }
	    /* FALLTHROUGH */

     	default:
	    Error (ERR_ILLEGAL_TYPE);
	    break;

    }
}



