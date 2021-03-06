/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#ifndef _MEM_INCLUDED
#define _MEM_INCLUDED

#define MAX_X       16
#define MIN_BLOCK   8
#define MAX_HEAPS   10

typedef struct overlay {
    unsigned char       active  : 1;
    unsigned char       exp     : 5;
    struct overlay __near *next, __near *prev;
} overlay;

#define NIL     (overlay __near *)1
#define NON_NIL (overlay __near *)0

#define OVERHEAD sizeof( overlay )

typedef struct heap {
    unsigned        segment;
    unsigned        maxx;
    overlay __near  *free[MAX_X + 2];
} heap;

void    *MemAlloc( unsigned );
void    MemFree( void * );

#endif
