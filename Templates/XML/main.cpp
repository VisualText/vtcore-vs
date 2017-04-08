/*******************************************************************************
Copyright � 1999-2008 Text Analysis International, Inc.
All rights reserved.
********************************************************************************
*
* NAME: MAIN.CPP
* FILE: main.cpp
* CR:   01/29/99 AM.
* SUBJ: Sample driver program that uses the Analyzer and KB API functions.
*       Loading an analyzer invokes the user extensions library (user/user.dll).
* ENV:  APPS - a default environment variable for VisualText applications.
*
*******************************************************************************/
#ifndef LINUX
#include <windows.h>
#endif
#include <iostream>
#include <fstream>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <strstream>
#include <time.h>

#include <tchar.h>
#include <my_tchar.h>
#include <streamclasses.h>

#include "vtapi.h"		// VISUALTEXT API INCLUDES.			// 01/31/03 AM.
#include "u_out.h"                                          // 01/24/06 AM.

#include "Uanalyze.h"

bool cmdReadargs(int,_TCHAR*argv[],
		/*UP*/_TCHAR*&,_TCHAR*&,bool&,bool&,bool&);
void cmdHelpargs(_TCHAR*);

int _tmain(
   int argc,
   _TCHAR *argv[],
   _TCHAR *envp[]
   )
{
// Without this, can't find the error output on the console.
#ifndef LINUX
//cerr = cout;                                          // HACK. // 10/18/98 AM.
#endif

//////////////////////////////////
/// NAME OF CURRENT APP DIRECTORY
//////////////////////////////////
_TCHAR *appname;
appname = _T("App");      // TODO: REPLACE "App" WITH THE REAL DIRECTORY NAME.

/////////////////////////////////////////////////
// READ ARGUMENTS
/////////////////////////////////////////////////
// Need to get command line arguments and/or some kind of init file.
_TCHAR *input=0, *output=0;
bool develop = false;      // Development mode.
_TCHAR *sequence=0;
_TCHAR appdir[MAXSTR];       // Directory for the application.
appdir[0] = '\0';
bool compiled=false;       // Run compiled/interp analyzer.
bool silent=false;			// No log/debug output files.

/////////////////////////////////////////////////
// GET APP INFORMATION
/////////////////////////////////////////////////

// Get input and output filenames from command line.
if (!cmdReadargs(argc, argv,/*UP*/input,output,develop,compiled,silent))
   exit(1);

_t_cout << _T("\n[Current date: ") << today() << _T("]") << endl;

_TCHAR *path = 0;
if (!(path = _tgetenv(_T("APPS"))))
   {
   _t_cout << _T("[No APPS environment variable.]") << endl;
   return 1;
   }

_stprintf(appdir, _T("%s%c%s"), path,DIR_CH,appname);

_TCHAR *vtpath = 0;                                               // 08/28/02 AM.
if (!(vtpath = _tgetenv(_T("VISUALTEXT"))))                           // 08/28/02 AM.
   {
   _t_cout << _T("[No VISUALTEXT environment variable.]") << endl;
   return 1;
   }

_TCHAR rfbdir[MAXSTR];		    // Directory for VisualText data.   // 08/28/02 AM.
rfbdir[0] = '\0';                                               // 08/28/02 AM.
_stprintf(rfbdir, _T("%s%cdata%crfb%cspec"),                          // 08/28/02 AM.
                          vtpath,DIR_CH,DIR_CH,DIR_CH);         // 08/28/02 AM.

// A general temporary directory.                               // 08/28/02 AM.
_TCHAR *tmpdir = _T("c:\\temp");                                      // 08/28/02 AM.
_TCHAR logfile[MAXSTR];		 // General VT log file.             // 08/28/02 AM.
logfile[0] = '\0';                                              // 08/28/02 AM.
_stprintf(logfile, _T("%s%cvisualtext.log"),tmpdir,DIR_CH);           // 08/28/02 AM.

// Set up subdirectories with default names.
// Could read a configuration file here.
// Could set up to label output files based on input file name
// or some user-supplied prefix.  Or create unique file names.
_TCHAR specdir[MAXSTR];

_TCHAR infile[MAXSTR];
_TCHAR outfile[MAXSTR];
_TCHAR seqfile[MAXSTR];

_stprintf(specdir, _T("%s%sspec"),   appdir, DIR_STR);

if (!input)
   input = _T("input.txt");         // Default.
if (!output)
   output = _T("output.txt");       // Default.

// Get analyzer-sequence file name.
if (!sequence)
   sequence = _T("analyzer.seq");   // Default.

_stprintf(infile, _T("%s"), input);
_stprintf(outfile, _T("%s"), output);
_stprintf(seqfile, _T("%s%s%s"), specdir,DIR_STR, sequence);

if (!path_exists(infile))
   {
   _t_cerr << _T("[File/directory not found: ") << infile << _T("]") << endl;
   return 0;
   }

/////////////////////////////////////////////////
// INITIALIZE VISUALTEXT RUNTIME SYSTEM                        // 08/27/02 AM.
/////////////////////////////////////////////////
VTRun *vtrun = VTRun::makeVTRun(                               // 07/21/03 AM.
	logfile,                // Verbose/error log file.          // 08/28/02 AM.
	rfbdir,                 // VisualText dir for RFB spec.     // 08/28/02 AM.
	true                    // Build silently.                  // 08/28/02 AM.
	);

//ALIST *alist = (ALIST *)vtrun->alist_;                       // 07/21/03 AM.


/////////////////////////////////////////////////
// INITIALIZE ANALYZER RUNTIME ENGINE
/////////////////////////////////////////////////
// Create and initialize an NLP object to manage text analysis.
// NOTE: This init will dynamically load the user extensions dll at
// appdir\user\debug\user.dll
NLP *nlp = vtrun->makeNLP(                                     // 07/21/03 AM.
                     appdir,appname,develop,silent,compiled);  // 07/21/03 AM.

/////////////////////////////////////////////////
// SET UP THE KNOWLEDGE BASE
/////////////////////////////////////////////////

CG *cg = vtrun->makeCG(                                        // 07/21/03 AM.
         appdir,
         true,      // LOAD COMPILED KB IF POSSIBLE.
			nlp);      // Associated analyzer object.             // 07/21/03 AM.
if (!cg)                                                       // 07/21/03 AM.
   {
   _t_cerr << _T("[Couldn't make knowledge base.]") << endl;          // 07/21/03 AM.
   vtrun->deleteNLP(nlp);                                      // 07/21/03 AM.
   VTRun::deleteVTRun(vtrun);                                  // 07/21/03 AM.
   return -1;
   }

// Root of the KB hierarchy.
CONCEPT *root = VTRun::getKBroot(cg);                          // 12/19/03 AM.


/////////////////////////////////////////////////
// BUILD ANALYZER APPLICATION
/////////////////////////////////////////////////
// Create an analyzer dynamically using the sequence file and rules files
// under appdir\spec.

if (!nlp->make_analyzer(seqfile, appdir, develop,
	silent,              // Debug/log file output.              // 06/16/02 AM.
   0,
   false,               // false == Don't compile during load.
   compiled))                    // Compiled/interp analyzer.
   {
   _t_cerr << _T("[Couldn't build analyzer.]") << endl;
   vtrun->deleteNLP(nlp);                                      // 07/21/03 AM.
   VTRun::deleteVTRun(vtrun);                                  // 07/21/03 AM.
   return -1;
   }


/////////////////////////////
// TEST RULE GENERATION.
/////////////////////////////
#ifdef TEST_RUG_
CONCEPT *gram = cg->findConcept(root, _T("gram"));
if (!gram)
   gram = cg->makeConcept(root, _T("gram"));
RUG::rugInit(gram,cg);

// Execute rule generation.
RUG *rug = new RUG(gram, nlp, appdir, cg);
rug->rugGenerate(false,false);      // Generate ALL.
delete rug;
rug = 0;
#endif

// To have analyzer output to a stream, can rebind the cout
// stream.  // 05/05/02 AM.
_TCHAR ofstr[MAXSTR];
_stprintf(ofstr,_T("e:\\dummy.txt"));
_t_ofstream os(TCHAR2CA(ofstr), ios::out);												// 08/07/02 AM.

// Testing output to buffer.
_TCHAR obuf[MAXSTR];															// 05/11/02 AM.


/////////////////////////////////////////////////
// SET START OF BATCH
/////////////////////////////////////////////////
// Tell analyzer that first file of batch is being processed.
// Using the batchstart() NLP++ function, an analyzer can do initializations
// for the first file of a batch of files.

nlp->setFbatchstart(true);                                     // 05/16/08 AM.


/////////////////////////////////////////////////
// ANALYZE INPUT TEXT FILES
/////////////////////////////////////////////////

if (is_file(infile))      // Input is a single file.
   {
   // If input is from a buffer, specify its name and length.
   nlp->analyze(infile, outfile, appdir, develop,
      silent,        // Debug/log output files.                // 06/16/02 AM.
      0,            // Outdir.
      0,            // Input buffer.
      0,            // Length of input buffer, or 0.
      compiled,      // If running compiled analyzer.
		&os,				// Rebind cout output stream in analyzer  // 08/07/02 AM.
		obuf,																		// 05/11/02 AM.
		MAXSTR																	// 05/11/02 AM.
      );
   }
else      // Develop mode not on.
   {
   // Analyze file, directory, or tree of directories.
   analyze_path(nlp, infile, outfile, appdir,                 // 07/15/03 AM.
	               silent,0,compiled,&os,obuf,                 // 07/15/03 AM.
						true);
   }


// Can check that buffer was filled by analyzer calls such as
// cbuf() << "hello" << "\n";
//cerr << "obuf=";                                             // 05/11/02 AM.
//cerr << obuf << endl;                                        // 05/11/02 AM.


/////////////////////////////////////////////////
// CLEANUP VISUALTEXT RUNTIME
/////////////////////////////////////////////////

// This will close the user.dll for the application also.
vtrun->deleteNLP(nlp);                                         // 07/21/03 AM.
VTRun::deleteVTRun(vtrun);                                     // 07/21/03 AM.
object_counts();    // Report memory leaks to standard output.

return 0;
}


/********************************************
* FN:		CMDREADARGS
* CR:		11/30/98 AM.
* SUBJ:	Get command line arguments.
* RET:	True if ok read, else false if failed.
* NOTE:	06/16/02 AM. Renamed from dosReadargs, moved to user project.
********************************************/

bool cmdReadargs(
	int argc,
	_TCHAR *argv[],
	/*UP*/
	_TCHAR* &input,				// Input file from args.
	_TCHAR* &output,				// Output file from args.
	bool &develop,				// Development mode (output intermediate files).
	bool &compiled,	// true - compiled ana. false=interp(DEFAULT).
	bool &silent	// true == only output files specified by analyzer.
	)
{
_TCHAR *ptr;
_TCHAR **parg;
bool f_in  = false;
bool f_out = false;
bool flag  = false;
bool compiledck = false;	// If compiled/interp arg seen.

input = output = 0;
develop = false;				// Default is not development mode.
compiled = false;	// INTERP ANALYZER BY DEFAULT
silent = false;	// Produce debug files, etc. by default.		// 06/16/02 AM.

for (--argc, parg = &(argv[1]); argc > 0; --argc, ++parg)
	{
	// For each command line argument.
	//*gout << "command arg=" << *parg << endl;
	ptr = *parg;
	if (*ptr == '/' || *ptr == '-')	// DOS or UNIX style arg.
		{
		if (flag)
			{
			_t_cerr << _T("[Error in command line args for ") << argv[0]
						  << _T("]") << endl;
			return false;
			}
		++ptr;
		if (!strcmp_i(ptr, _T("in")))
			f_in = flag = true;					// Expecting input file.
		else if (!strcmp_i(ptr, _T("out")))
			f_out = flag = true;					// Expecting output file.
		else if (!strcmp_i(ptr, _T("dev")))		// 12/25/98 AM.
			{
			if (silent)
				{
				_t_cerr << _T("[Ignoring /dev flag.]") << endl;
				develop = false;
				}
			else
				develop = true;					// Development mode.
			}
		else if (!strcmp_i(ptr, _T("silent")))	                     // 06/16/02 AM.
			{
			if (develop)
				{
				_t_cerr << _T("[Ignoring /dev flag.]") << endl;
				develop = false;
				}
			silent = true;                                        // 06/16/02 AM.
			}
		else if (!strcmp_i(ptr, _T("interp")))	// Run interpreted analyzer.
			{
			if (compiledck)
				_t_cerr << _T("[Ignoring extra /compiled or /interp flag.]") << endl;
			else
				{
				compiledck = true;
				compiled = false;
				}
			}
		else if (!strcmp_i(ptr, _T("compiled")))	// Run compiled analyzer.
			{
			if (compiledck)
				_t_cerr << _T("[Ignoring extra /compiled or /interp flag.]") << endl;
			else
				{
				compiledck = true;
				compiled = true;
				}
			}
		}
	else if (flag)							// Expected an argument value.
		{
		if (f_in)
			{
			if (input)
				{
				_t_cerr << _T("[") << argv[0] << _T(": Input file specified twice.]")
						  << endl;
				cmdHelpargs(argv[0]);
				return false;
				}
			// Grab value as input file.
			input = ptr;
			f_in = flag = false;
			}
		else if (f_out)
			{
			if (output)
				{
				_t_cerr << _T("[") << argv[0]
						  << _T(": Output file specified twice.]") << endl;
				cmdHelpargs(argv[0]);
				return false;
				}
			// Grab value as output file.
			output = ptr;
			f_out = flag = false;
			}
		}
	else										// Got a "floating" value.
		{
		if (input && output)
			{
			_t_cerr << _T("[") << argv[0] << _T(": Extra arguments.]") << endl;
			cmdHelpargs(argv[0]);
			return false;
			}
		else if (input)
			output = ptr;
		else
			input = ptr;
		}
	}
return true;
}


/********************************************
* FN:		CMDHELPARGS
* CR:		11/30/98 AM.
* SUBJ:	Print command line argument help.
* RET:	True if ok read, else false if failed.
* NOTE:	06/16/02 AM. Renamed from dosHelpargs, moved to user project.
********************************************/

void cmdHelpargs(_TCHAR *name)
{
_t_cout << name
<< _T(" [/INTERP][/COMPILED][/IN infile] [/OUT outfile] [/DEV][/SILENT] [infile [outfile]]")
	  << endl
	  << _T("Note: /INTERP, the interpreted analyzer, is default.")
	  << endl;
}
