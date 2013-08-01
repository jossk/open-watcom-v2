/****************************************************************************
*
*                            Open Watcom Project
*
*    Copyright (c) 2013 Open Watcom Contributors. All Rights Reserved.
*
* =========================================================================
*
* Description:  C preprocessor environment processing.
*
****************************************************************************/


#include "global.h"
#include "preproc.h"

char *PP_GetEnv( const char *name )
{
    return( RcGetEnv( name ) );
}