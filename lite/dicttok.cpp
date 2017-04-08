/*******************************************************************************
Copyright (c) 2011 by Text Analysis International, Inc.
All rights reserved.
********************************************************************************
*
* NAME:	DICTTOK.CPP
* FILE:	lite\tok.cpp
* CR:		07/29/11 AM.
* SUBJ:	Tokenization.
* NOTE:	(1) Copy the TOK.cpp pass.
*			(2) Clean it up to look more like the CMLTOK pass.
*			(3) Save that off as something like SIMPLETOK, to serve as a template.
*			(4) Include the "Unicode issues" in this one.
*			Because CMLTOK is nicely structured for building complex tokenizers and
*			parse trees. (eg, with pages, lines, blobs of tokens, and so on.)
*
*******************************************************************************/

#include "StdAfx.h"
#include "machine.h"	// 10/25/06 AM.
#include "u_out.h"		// 01/19/06 AM.
#include "consh/libconsh.h"
#include "consh/cg.h"
#include "htab.h"
#include "kb.h"
#include "lite/global.h"
#include "inline.h"		// 05/19/99 AM.
#include "lite/lite.h"				// 07/07/03 AM.
#include "dlist.h"					// 07/07/03 AM.
#include "node.h"				// 07/07/03 AM.
#include "tree.h"				// 07/07/03 AM.
#include "lite/iarg.h"		// 05/14/03 AM.
#include "str.h"				// 02/26/01 AM.
#include "io.h"
#include "node.h"	// 07/07/03 AM.
#include "parse.h"
#include "ana.h"
#include "Eana.h"				// 02/26/01 AM.
#include "nlp.h"				// 06/25/03 AM.
#include "ivar.h"

#ifdef UNICODE
#include "utypes.h"	// 03/03/05 AM.
#include "uchar.h"
#endif

#include "dicttok.h"

// For pretty printing the algorithm name.
static _TCHAR algo_name[] = _T("dicttok");

/********************************************
* FN:		Special Functions for DICTTok class.
* CR:		10/09/98 AM.
********************************************/

DICTTok::DICTTok()			// Constructor
	: Algo(algo_name /*, 0 */)
{
bad_ = false;					// 01/15/99 AM.
zapwhite_ = false;			// 08/16/11 AM.
//parse = 0;
lines_ = tabs_ = 0;	// 08/16/11 AM.
tottabs_ = 0;
totlines_ = 1;	// Every text has minimum 1 line.
totlowers_ = totcaps_ = totuppers_ = totnums_ = 0;
}

DICTTok::DICTTok(const DICTTok &orig)			// Copy constructor	// 12/03/98 AM.
{
name = orig.name;
//parse = orig.parse;			// 12/04/98 AM.
debug_ = orig.debug_;
}

#ifdef OLD_
DICTTok::DICTTok(			// Constructor that does it all.
	Parse *p
	)
	: Algo(p)
{
//parse = p;

// PERFORM TOKENIZATION.
//Tokenize();
}
#endif

/********************************************
* FN:		Access functions
********************************************/

//Parse		*DICTTok::getParse() { return parse; }
bool		DICTTok::getBad()		{return bad_;}				// 01/15/99 AM.
bool		DICTTok::getZapwhite()		{return zapwhite_;}		// 08/16/11 AM.


/********************************************
* FN:		Modify functions
********************************************/

//void DICTTok::setParse(Parse *x) { parse = x; }
void		DICTTok::setBad(bool x)	{bad_		= x;}			// 01/15/99 AM.
void		DICTTok::setZapwhite(bool x)	{zapwhite_		= x;}			// 08/16/11 AM. AM.



/********************************************
* FN:		DUP
* CR:		12/03/98 AM.
* SUBJ:	Duplicate the given Algo object.
* NOTE:	Don't know a better way to have a base pointer duplicate the
*			object that it points to!
********************************************/

Algo &DICTTok::dup()
{
DICTTok *ptr;
ptr = new DICTTok(*this);					// Copy constructor.
//ptr = new Pat();
//ptr->setName(this->name);
//ptr->setParse(this->parse);
return (Algo &) *ptr;
}


/********************************************
* FN:		SETUP
* CR:		12/04/98 AM.
* SUBJ:	Set up Algo as an analyzer pass.
* ARG:	s_data = This is an argument from the analyzer sequence file,
*		   if any, for the current pass.
********************************************/

void DICTTok::setup(_TCHAR *s_data)
{
// No arguments to this pass in sequence file.
}


/********************************************
* FN:		EXECUTE
* CR:		10/12/98 AM.
* SUBJ:	Perform the tokenization.
* ASS:	text is in place, and tree should be empty.
* NOTE:	Will make this a virtual function in Pass class.
********************************************/

bool DICTTok::Execute(Parse *parse, Seqn *seqn)
{
// Initialize the tokenizer.
initTok(parse);

// Ignore the pass data.
Tokenize(parse);

return finTok();	// Final cleanups for pass.
}


/********************************************
* FN:		TOKENIZE
* CR:		10/09/98 AM.
* SUBJ:	Perform the tokenization.
* RET:	ok - true if ok, false if failure.
* ASS:	text is in place, and tree should be empty.
********************************************/

bool DICTTok::Tokenize(Parse *parse)
{

//////////////////////////////////////////////////////////
// Traverse buffer, creating a node for every token found.
//////////////////////////////////////////////////////////

_TCHAR *buf = text_;			// For traversing text.
long start = 0;			// Count offset of current char.
Node<Pn> *last = 0;			// Last token node.	// FIX. ZERO INIT.	// 08/28/11 AM.

prevwh_ = false;	// No whitespace before first token.	// 08/16/11 AM.

// Bookkeep line numbers for debug.										// 05/17/01 AM.
long line = 1;

// Get first token and attach to tree.
FirstToken(tree_, htab_, /*DU*/ &buf, start, last,
					line															// 05/17/01 AM.
					);

// Continue getting tokens.
while (*buf)
	{
	if (!NextToken(tree_, htab_, /*DU*/ &buf, start, last,			// 01/26/02 AM.
					line															// 05/17/01 AM.
					))
		return false;															// 01/26/02 AM.
	}

return true;																	// 01/26/02 AM.
}


/********************************************
* FN:		INITTOK
* CR:		07/31/11 AM.
* SUBJ:	Initialize tokenizer.
********************************************/

bool DICTTok::initTok(Parse *parse)
{
if (!parse)
	return false;

if (parse->Verbose())
	*gout << _T("[DICTTok:]") << endl;

// Need to get the current KB.
cg_ = parse->getAna()->getCG();
//CONCEPT *root = cg_->findRoot();


text_ = parse->text;

if (!parse->text)
	{
	_t_strstream gerrStr;
	gerrStr << _T("[DICTTok: Given no text.]") << ends;
	return errOut(&gerrStr,false);
	}

tree_ = (Tree<Pn> *)parse->getTree();

if (tree_)
	{
	_t_strstream gerrStr;
	gerrStr << _T("[DICTTok: Parse tree exists. Ignoring tokenization pass.]") << ends;
	errOut(&gerrStr,false);
	return true;
	}

//////////////////////

// Reset.  No bad chars seen yet.
bad_ = false;
// zapwhite_	// Comes from caller of the tokenizer, leave it be.
lines_ = tabs_ = 0;
tottabs_ = 0;
totlines_ = 1;	// Every text has minimum 1 line.
totlowers_ = totcaps_ = totuppers_ = totnums_ = 0;

// Track line numbers in input text, for debug messages, etc.
lineno_ = 1;

htab_ = parse->htab_;
parse_ = parse;

// Initialize parse tree levels.
root_ = 
// page_ = line_ = word_ = 
tok_ = 0;
//firsttok_ = 0;

fmpos_ = 0;

fmptr_ = parse->text;

// The allocated input buffer length.
// Note that we'll be using less of it after removing information text.
long len  = parse->length;

// CREATE PARSE TREE ROOT.
_TCHAR *str = _T("_ROOT");
Sym *sym = htab_->hsym(str);
tree_ = Pn::makeTree(0, len-1, PNNODE, fmptr_, str, sym); // Create parse tree.
parse->setTree(tree_);								// Update global data.

if (!tree_)
	{
	_t_strstream gerrStr;
	gerrStr << _T("[DICTTok: Could not create parse tree.]") << ends;
	errOut(&gerrStr,false);
	return false;
	}

root_ = tree_->getRoot();	// FETCH PARSE TREE ROOT.

if (!root_)
	{
	_t_strstream gerrStr;
	gerrStr << _T("[DICTTok: No parse tree root.]") << ends;
	errOut(&gerrStr,false);
	return false;
	}

// NEED TO MAKE THE ROOT NODE UNSEALED!
Pn *pn = root_->getData();
pn->setUnsealed(true);

return true;
}


/********************************************
* FN:		FINTOK
* CR:		07/31/11 AM.
* SUBJ:	Final processing of tokenizer pass.
********************************************/

bool DICTTok::finTok()
{
if (!parse_ || !tree_ || !root_)
	return false;

// Place totals onto parse tree root.
replaceNum(root_,_T("TOT LINES"),totlines_);
replaceNum(root_,_T("TOT TABS"),tottabs_);
replaceNum(root_,_T("TOT LOWERS"),totlowers_);
replaceNum(root_,_T("TOT CAPS"),totcaps_);
replaceNum(root_,_T("TOT UPPERS"),totuppers_);
replaceNum(root_,_T("TOT NUMS"),totnums_);

// MOVED HERE FROM TOKENIZE fn.	// 08/18/11 AM.
//if (parse_->Verbose())											// FIX	// 02/01/00 AM.
if (parse_->getEana()->getFlogfiles())						// FIX	// 02/01/00 AM.
	{
	//*gout << "[Tokenize: Dumping parse tree.]" << endl;
	tree_->Traverse(root_, *gout);
	}

return true;
}

/********************************************
* FN:		FIRSTTOKEN
* CR:		10/09/98 AM.
* SUBJ:	Find and set up first token.
********************************************/

void DICTTok::FirstToken(
	Tree<Pn> *tree,
	Htab *htab,
	/*DU*/
	_TCHAR* *buf,
	long &start,
	Node<Pn>* &last,
	long &line					// Bookkeep line number.				// 05/17/01 AM.
	)
{
long end;
_TCHAR *ptr;	// Point to end of token.
enum Pntype typ;
bool lineflag = false;														// 05/17/01 AM.

// Get first token information.
nextTok(*buf, start, /*UP*/ end, ptr, typ,
	lineflag		// Flag newline seen										// 05/17/01 AM.
	);

/* Attach first node. */
Sym *sym;
_TCHAR *str;
_TCHAR *lcstr;

// If skipping whitespace!	// 08/16/11 AM.
if (zapwhite_ && typ == PNWHITE)	// 08/16/11 AM.
	{
	prevwh_ = true;	// 08/16/11 AM.
	}
else
	{
sym = internTok(*buf, end-start+1, htab,/*UP*/lcstr);
str = sym->getStr();
last = Pn::makeTnode(start, end, typ, *buf, str, sym,				// 10/09/99 AM.
							line);												// 05/17/01 AM.

// Lookup, add attrs, reduce, attach to tree.	// 07/31/11 AM
handleTok(last,0,typ,str,lcstr);	// 07/31/11 AM.
//tree->firstNode(*last);	// Attach first node to tree.

	}	// END else (not zapwhite or not whitespace)

if (lineflag)		// First token was a newline!						// 05/17/01 AM.
	++line;																		// 05/17/01 AM.

/* UP */
start = end + 1;	// Continue tokenizing from next char.
*buf   = ptr + 1;	// Continue tokenizing from next char.
}


/********************************************
* FN:		NEXTTOKEN
* CR:		10/09/98 AM.
* SUBJ:	Find and set up next token in parse tree.
* RET:	ok - true if ok, false if failed.
********************************************/

bool DICTTok::NextToken(
	Tree<Pn> *tree,
	Htab *htab,
	/*DU*/
	_TCHAR* *buf,
	long &start,
	Node<Pn>* &last,
	long &line					// Bookkeep line number.				// 05/17/01 AM.
	)
{
long end;
_TCHAR *ptr;	// Point to end of token.
enum Pntype typ;

// Get next token information.
bool lineflag = false;														// 05/17/01 AM.
nextTok(*buf, start, /*UP*/ end, ptr, typ,
			lineflag);															// 05/17/01 AM.

/* Attach next node to list. */
Sym *sym;
_TCHAR *str;
_TCHAR *lcstr;

// If skipping whitespace!	// 08/16/11 AM.
if (zapwhite_ && typ == PNWHITE)	// 08/16/11 AM.
	{
	prevwh_ = true;	// 08/16/11 AM.
	}
else
	{
sym = internTok(*buf, end-start+1, htab,/*UP*/lcstr);
str = sym->getStr();
Node<Pn> *node;
//node = Pn::makeNode(start, end, typ, *buf, str, sym);			// 10/09/99 AM.
node = Pn::makeTnode(start, end, typ, *buf, str, sym,				// 10/09/99 AM.
							line);												// 05/17/01 AM.

// CHECK NODE OVERFLOW.														// 01/24/02 AM.
if (!node)																		// 01/24/02 AM.
	{
	_t_strstream gerrStr;						// 01/24/02 AM.
	gerrStr << _T("[Node overflow at ") << start << _T(" chars, ")		// 01/24/02 AM.
		<< last->getCount() << _T(" nodes.]") << ends;					// 01/24/02 AM.
	return errOut(&gerrStr,false,0,0);												// 01/26/02 AM.
	}

// Lookup, add attrs, reduce, attach to tree.	// 07/31/11 AM
handleTok(node,last,typ,str,lcstr);	// 07/31/11 AM.

last = node;		// node is last node of list now.

	}	// END else (not zapwhite or not whitespace)

if (lineflag)																	// 05/17/01 AM.
	++line;																		// 05/17/01 AM.

/* UP */
start = end + 1;	// Continue tokenizing from next char.
*buf  = ptr + 1;	// Continue tokenizing from next char.
return true;																	// 01/26/02 AM.
}


/********************************************
* FN:		NEXTTOK
* CR:		10/09/98 AM.
* SUBJ:	Get the data for next token.
* OPT:	Could make a big switch statement.
* UNI:	Not worrying about Unicode.
********************************************/

void DICTTok::nextTok(
	_TCHAR *buf,		// Start char of token.
	long start,		// Start offset of token.
	/*UP*/
	long &end,		// End offset of token.
	_TCHAR* &ptr,		// End char of token.
	enum Pntype &typ,	// Type of token.
	bool &lineflag		// Flag new line number.						// 05/17/01 AM.
	)
{
end = start;
ptr = buf;
lineflag = false;	// RESET end of line tracker.	// 08/16/11 AM.

//if (*ptr < 0)		// Non-ASCII char.	// 07/28/99 AM.	// 09/22/99 AM.
//	{
	// Fall through.
//	}

if (alphabetic(*ptr))						// 09/22/99 AM.
	{
#ifdef UNICODE
	 short u_gcb = u_getIntPropertyValue((UChar32)*ptr, UCHAR_GRAPHEME_CLUSTER_BREAK);	// 01/30/06 AM.
	 short new_gcb = 0;

	short u_wb = u_getIntPropertyValue((UChar32)*ptr, UCHAR_WORD_BREAK);			// 01/30/06 AM.
	short new_wb = 0;																				// 01/30/06 AM.

	UErrorCode errorCode;
	short u_script = uscript_getScript(*ptr,&errorCode);								// 01/30/06 AM.
	short new_script = 0;																		// 01/30/06 AM.
#endif

	typ = PNALPHA;
	// 09/22/99 AM. Negative char values now handled in alphabetic() fn.
	++ptr;		// 11/05/99 AM.
	++end;		// 11/05/99 AM.

#ifdef UNICODE
//	while (alphabetic(*ptr) && ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z')))
//   while (alphabetic(*ptr) && ! u_hasBinaryProperty((_TUCHAR)*ptr, UCHAR_WORD_BREAK))
	while (alphabetic_extend(*ptr,u_gcb,u_wb,u_script,/*UP*/new_gcb,new_wb,new_script))
		{
		u_gcb = new_gcb;
		u_wb = new_wb;
		u_script = new_script;
		new_gcb = new_wb = new_script = 0;

		++ptr;
		++end;
		}
#else
	while (alphabetic(*ptr)					// 09/22/99 AM.
			// && *ptr > 0						// 07/28/99 AM.	// 09/22/99 AM.
			 )
		{
		++ptr;
		++end;
		}
#endif

	--end;		// Back up to final alpha char.
	--ptr;
	return;		// 07/28/99 AM.
	}
else if (*ptr < 0)	// Other extended ANSI chars.	// 09/22/99 AM.
	{
	// Fall through.			// 09/22/99 AM.
	}
else if (_istdigit(*ptr))
	{
	typ = PNNUM;
	++ptr;		// 11/05/99 AM.
	++end;		// 11/05/99 AM.
	while (_istdigit(*ptr)
			 && *ptr > 0						// 07/28/99 AM.
			)
		{
		++ptr;
		++end;
		}
	--end;		// Back up to final digit char.
	--ptr;
	return;		// 07/28/99 AM.
	}
else if (_istspace(*ptr))
	{
	// Assuming newline always present at end of line.				// 05/17/01 AM.
	if (*ptr == '\n')															// 05/17/01 AM.
		{
		lineflag = true;	// Flag new line number.					// 05/17/01 AM.
		++lines_;	// 08/16/11 AM.
		++totlines_;
		tabs_ = 0;	// Newlines kill tabbing!	// 08/16/11 AM.
		}
	else if (*ptr == '\t')	// 08/16/11 AM.
		{
		++tabs_;	// 08/16/11 AM.
		++tottabs_;
		}
	typ = PNWHITE;
	return;		// 07/28/99 AM.
	}
else if (_istpunct((_TUCHAR)*ptr))
	{
	typ = PNPUNCT;
	return;		// 07/28/99 AM.
	}

// NON ASCII CHAR.			// 07/28/99 AM.

if (!bad_)						// 01/15/99 AM.
	{
	bad_ = true;					// 01/15/99 AM.
//	_t_strstream gerrStr;
//	gerrStr << _T("[Non-ASCII chars in file.]") << ends;
//	errOut(&gerrStr,false);
	}
// Hack. 12/05/98 AM.
//typ = PNPUNCT;
//*ptr = '~';				// 07/28/99 AM. I didn't like @.
typ = PNCTRL;																	// 07/19/00 AM.
}


/********************************************
* FN:		INTERNTOK
* CR:		10/28/98 AM.
* SUBJ:	Internalize the token.
* NOTE:	Need to lookup/place one copy of the token in a hash table.
*			(If alphabetic).  This is a placeholder for string and hash
*			table implementation in the future.
********************************************/

Sym *DICTTok::internTok(
	_TCHAR *str,				// Ptr to string in a buffer.
	long len,				// Length of string within buffer.
	Htab *htab,				// Hashed symbol table.		// 11/19/98 AM.
	/*UP*/ _TCHAR* &lcstr
	)
{
lcstr = 0;
if (empty(str) || len <= 0)
	{
	_t_strstream gerrStr;
	gerrStr << _T("[internTok: Given bad string or length.]") << ends;
	errOut(&gerrStr,false);
	return 0;
	}

// If token too long, truncate.	// FIX.	// 08/06/06 AM.
if (len >= MAXSTR)					// FIX.	// 08/06/06 AM.
	{
	_t_strstream gerrStr;
	gerrStr << _T("[Intern Token: Too long -- truncating.]") << ends;
	errOut(&gerrStr,false);
	len = MAXSTR - 1;	// Recover.	// FIX.	// 08/06/06 AM.
	}

// 01/26/99 AM. Building lowercase variant of sym here also.
Sym *sym;
if (!(sym = htab->hsym_kb(str, len,/*UP*/lcstr)))			// 01/26/99 AM.
	{
	_t_strstream gerrStr;
	gerrStr << _T("[Intern Token: Failed.]") << ends;
	errOut(&gerrStr,false);
	return 0;
	}

return sym;								// 11/19/98 AM.
}


/********************************************
* FN:		HANDLETOK
* CR:		07/31/11 AM.
* SUBJ:	Lookup, reduce, attach token to tree.
* NOTE:	Looking up alphabetics in KB DICTIONARY.
********************************************/

bool DICTTok::handleTok(
	Node<Pn> *node,
	Node<Pn> *last,
	enum Pntype typ,
	_TCHAR *str,
	_TCHAR *lcstr	// Lowercase str
	)
{
if (!node)
	return false;

// If ctrl, get code onto node.
// If alphabetic, lookup in dictionary.
// If unique part of speech, can reduce here as desired.
// Put attributes on.
CONCEPT *con = 0;

switch (typ)
	{
	case PNCTRL:
		{
		// Put this as a variable on the node!
		int ansi = (unsigned char) *str;
		replaceNum(node,_T("CTRL"),ansi);
		if (!prevwh_)
			replaceNum(node,_T("NOSP"),1);
		if (lines_)
			replaceNum(node,_T("NL"),lines_);
		if (tabs_)
			replaceNum(node,_T("TABS"),tabs_);
		prevwh_ = false;
		lines_ = tabs_ = 0;
		}
		break;
	case PNALPHA:
		if (!prevwh_)
			replaceNum(node,_T("NOSP"),1);
		if (lines_)
			replaceNum(node,_T("NL"),lines_);
		if (tabs_)
			replaceNum(node,_T("TABS"),tabs_);
		prevwh_ = false;
		lines_ = tabs_ = 0;
		if (lcstr && *lcstr)
		  {
		  int pos_num = 0;	// Count parts of speech.
		  con = cg_->findWordConcept(lcstr);	// dictfindword.
		  _TCHAR *attr = _T("pos");
			VAL *vals = cg_->findVals(con, attr);
			_TCHAR *val;
			while (vals)
				{
				val = popsval(vals);
				// Some kb editing.
				if (!_tcscmp(_T("adjective"),val))
					val = _T("adj");
				else if (!_tcscmp(_T("adverb"),val))
					val = _T("adv");
				else if (!_tcscmp(_T("pronoun"),val))
					val = _T("pro");
				else if (!_tcscmp(_T("conjunction"),val))
					val = _T("conj");
				// TODO: Look for COLON as first char, copy attr and VALUE.
				replaceNum(node,val,1);
				++pos_num;
				vals = cg_->nextVal(vals);
				}
			if (pos_num) replaceNum(node,_T("pos num"),pos_num);
			// MORE ATTRS ONTO ALPHA NODE.
			if (!_tcscmp(str,lcstr))
				{
				replaceNum(node,_T("lower"),1);	// LOWERCASE.
				++totlowers_;
				}
			else
				{
				++totcaps_;
				replaceNum(node,_T("cap"),1);
				// CHECK UPPERCASE HERE.
				_TCHAR ucstr[MAXSTR];
				str_to_upper(str, ucstr);
				if (!_tcscmp(str,ucstr))
					{
					++totuppers_;
					replaceNum(node,_T("upper"),1);
					}
				}

//		  _TCHAR *val = KB::strVal(con,attr,cg_,htab_);
		  }
		break;
	case PNNUM:	// Placed here for easy reference.
		++totnums_;
		// Fall through to PNPUNCT for now.
	case PNPUNCT:
		if (!prevwh_)
			replaceNum(node,_T("NOSP"),1);
		if (lines_)
			replaceNum(node,_T("NL"),lines_);
		if (tabs_)
			replaceNum(node,_T("TABS"),tabs_);
		prevwh_ = false;
		lines_ = tabs_ = 0;
		break;
	case PNWHITE:
		prevwh_ = true;
		break;
	case PNNULL:
	default:
		break;
	}

if (last)
	tree_->insertRight(*node,*last);
else if (root_)	// Sanity check.
	tree_->insertDown(*node,*root_);
return true;
}



/********************************************
* FN:		REPLACENUM
* CR:		08/21/08 AM.
* SUBJ:	Add/replace numeric var in node.
* RET:	true if ok.
* NOTE:	Shouldn't need to worry about interning name.
********************************************/

inline bool DICTTok::replaceNum(
	Node<Pn> *node,
	_TCHAR *name,	// variable name.
	long val
	)
{
if (!node)
	return false;

Pn *pn = node->getData();
return Ivar::nodeReplaceval(pn, name, val);
}

/********************************************
* FN:		REPLACESTR
* CR:		08/30/08 AM.
* SUBJ:	Add/replace string var in node.
* RET:	true if ok.
* NOTE:	Shouldn't need to worry about interning name.
********************************************/

inline bool DICTTok::replaceStr(
	Node<Pn> *node,
	_TCHAR *name,	// variable name.
	_TCHAR *val
	)
{
if (!node)
	return false;

Pn *pn = node->getData();
return Ivar::nodeReplaceval(pn, name, val);
}


/********************************************
* FN:		POPSVAL
* CR:		08/01/11 AM.
* SUBJ:	Pop string attr value from list. KB convenience.
* RET:	Interned string value.
********************************************/

inline _TCHAR *DICTTok::popsval(
	VAL *val
	)
{

_TCHAR buf[MAXSTR];
cg_->popSval(val, /*UP*/ buf);
if (!buf[0])		
	return 0;

_TCHAR *str;
parse_->internStr(buf, /*UP*/ str);
return str;
}