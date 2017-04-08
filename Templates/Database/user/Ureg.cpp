/*******************************************************************************
Copyright (c) 1998,1999 by Text Analysis International, Inc.
All rights reserved.  No part of this document may be copied, used, or
modified without written permission from Text Analysis International, Inc.
********************************************************************************
*
* NAME:	UREG.CPP
* FILE:	c:\dev\nlplite\Ureg.cpp
* CR:		12/04/98 AM.
* SUBJ:	File for registering user-defined objects with NLP Lite.
* NOTE:	OBSOLETE.
*
*******************************************************************************/

// Include your user-defined objects here.
#include <windows.h>
#include <stdlib.h>
#include <iostream>											// Upgrade.	// 02/08/01 AM.
using namespace std;											// Upgrade.	// 02/08/01 AM.
#ifdef UNICODE
#include <sstream>
#else
#include <strstream>
#endif

#include <tchar.h>
#include <my_tchar.h>
#include <streamClasses.h>

//#include "Ualgo.h"			// Sample user-defined algorithm.
#include "Upre.h"
#include "user.h"																// 02/13/01 AM.
//#include "Ucode.h"

/********************************************
* FN:		UREG
* CR:		12/04/98 AM.
* SUBJ:	Register user-defined objects with NLP Lite.
********************************************/

void Ureg()
{
}