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


#include "cgstd.h"
#include "coderep.h"
#include "cgdefs.h"
#include "zoiks.h"
#include "freelist.h"
#include "cfloat.h"
#include "makeins.h"
#include "data.h"
#include "types.h"
#include "addrfold.h"
#include "display.h"
#include "utils.h"
#include "makeaddr.h"
#include "namelist.h"
#include "feprotos.h"

static    pointer       *AddrNameFrl;

extern  type_class_def  TypeClass(type_def*);
extern  name            *SAllocUserTemp(pointer,type_class_def,type_length);
extern  void            AddIns(instruction*);
extern  name            *BGNewTemp(type_def*);
extern  void            AllocALocal(name*);
extern  void            BGDone(an);
extern  cg_type         NamePtrType( name *op );
extern  name            *AllocRegName( hw_reg_set );


static  void    CopyAddr( an src, an dst )
/****************************************/
{
    an  link;

    link = dst->link;
    Copy( src, dst, sizeof( address_name ) );
    dst->link = link;
}


extern  an      NewAddrName( void )
/*********************************/
{
    an  addr;

    addr = AllocFrl( &AddrNameFrl, sizeof( address_name ) );
    addr->link = AddrList;
    AddrList = addr;
    addr->tipe = NULL;
    addr->u.n.index = NULL;
    addr->u.n.offset = 0;
    addr->u.n.name = NULL;
    addr->format = NF_ADDR;
    addr->flags = 0;
    addr->u.n.base = NULL;
    addr->u.n.alignment = 0;
    return( addr );
}


extern  an      NewBoolNode( void )
/*********************************/
{
    an  addr;

    addr = AllocFrl( &AddrNameFrl, sizeof( address_name ) );
    addr->tipe = NULL;
    addr->format = NF_BOOL;
    addr->flags = 0;
    return( addr );
}


extern  an      MakeTypeTempAddr( name *op, type_def *tipe )
/**********************************************************/
{
    an          addr;

    addr = NewAddrName();
    addr->tipe = tipe;
    addr->u.n.name = op;
    addr->class = CL_ADDR_TEMP;
    return( addr );
}

extern  an      MakeTempAddr( name *op )
/**************************************/
{
    return( MakeTypeTempAddr( op, TypeAddress( TY_NEAR_POINTER ) ) );
}

extern  void    InitMakeAddr( void )
/**********************************/
{
    InitFrl( &AddrNameFrl );
    AddrList = NULL;
}


extern  name    *GenIns( an addr )
/********************************/
{
    return( GetValue( addr, NULL ) );
}


extern  void    AddrFree( an node )
/*********************************/
{
    an  *owner;

    if( node->format != NF_BOOL ) {
        owner = &AddrList;
        for( ;; ) {
            if( *owner == node ) {
                *owner = node->link;
                break;
            }
            owner = &(*owner)->link;
        }
        if( node->format == NF_INS ) {
            FreeIns( node->u.i.ins );
        }
    }
    FrlFreeSize( &AddrNameFrl, (pointer *)node, sizeof( address_name ) );
}


extern  void    InsToAddr( an addr )
/**********************************/
{
    an          new;
    instruction *ins;

    if( addr->format == NF_INS ) {
        ins = addr->u.i.ins;
        ins->result = BGNewTemp( addr->tipe );
        new = AddrName( ins->result, addr->tipe );
        new->flags = addr->flags;
        new->u.n.base = addr->u.i.base;
        new->u.n.alignment = addr->u.i.alignment;
        CopyAddr( new, addr );
        AddrFree( new );
    }
}


extern  void    NamesCrossBlocks( void )
/**************************************/
{
    an          addr;
    an          new;
    an          next;
    name        *temp;

    /* Careful. The list shifts under our feet.*/
    for( addr = AddrList; addr != NULL; addr = next ) {
        next = addr->link;
        if( addr->flags & FL_ADDR_OK_ACROSS_BLOCKS ) {
            addr->flags |= FL_ADDR_CROSSED_BLOCKS;
        } else if( addr->format == NF_INS ) {
            InsToAddr( addr );
            addr->u.n.name->v.usage |= USE_IN_ANOTHER_BLOCK;
        } else if( addr->format != NF_CONS
             && addr->format != NF_BOOL
             && ( addr->format != NF_ADDR
                || ( addr->class != CL_ADDR_GLOBAL
                  && addr->class != CL_ADDR_TEMP ) ) ) {
            temp = GenIns( addr );
            if( temp->n.class == N_TEMP ) {
                temp->v.usage |= USE_IN_ANOTHER_BLOCK;
            } else if( temp->n.class == N_INDEXED ) {
                if( temp->i.index->n.class == N_TEMP ) {
                    temp->i.index->v.usage
                        |= USE_IN_ANOTHER_BLOCK;
                }
            }
            new = AddrName( temp, addr->tipe );
            CopyAddr( new, addr );
            AddrFree( new );
        }
    }
}


extern  bool    AddrFrlFree( void )
/*********************************/
{
    return( FrlFreeAll( &AddrNameFrl, sizeof( address_name ) ) );
}


static  name    *Display( cg_sym_handle symbol, int level )
/******************************************************/
{
    proc_def    *old_currproc;
    name        *old_names;
    name        *op;

    old_currproc = CurrProc;
    old_names = Names[N_TEMP];
    while( level != CurrProc->lex_level ) {
        CurrProc = CurrProc->next_proc;
    }
    Names[N_TEMP] = CurrProc->names[N_TEMP];
    for( op = Names[N_TEMP]; op->v.symbol != symbol; ) {
        op = op->n.next_name;
    }
    op->v.usage |= ( NEEDS_MEMORY | USE_MEMORY | USE_IN_ANOTHER_BLOCK);
    AllocALocal( op );
    CurrProc = old_currproc;
    Names[N_TEMP] = old_names;
    return( MakeDisplay( op, level ) );
}


extern  an      MakeGets( an dst, an src, type_def *tipe )
/********************************************************/
{
    name                *dst_name;
    name                *src_name;
    instruction         *ins;
    type_class_def      class;
    name                *temp;

    InsToAddr( dst );
    dst_name = Points( dst, tipe );
    ins = src->u.i.ins;
    if( src->format == NF_INS && CurrBlock->ins.hd.prev == ins ) {
        ins->result = dst_name;
        src->format = NF_ADDR;  /* so instruction doesn't get freed! */
    } else {
        src_name = GetValue( src, dst_name );
        if( src_name != dst_name ||
         (( src_name->n.class == N_MEMORY ) && ( src_name->v.usage & VAR_VOLATILE )) ) {
            class = TypeClass( tipe );
            src_name = GenIns( src );
            if( dst_name->n.class == N_INDEXED &&
             !( dst_name->i.index_flags & X_VOLATILE ) ) {
                /* don't give him back an indexed name - it extends the life of*/
                /* a pointer*/
                temp = SAllocTemp( dst_name->n.name_class, dst_name->n.size );
                AddIns( MakeMove( src_name, dst_name, class ) );
                AddIns( MakeMove( dst_name, temp, class ) );
                dst_name = temp;
            } else {
                AddIns( MakeMove( src_name, dst_name, class ) );
            }
        }
    }
    BGDone( src );
    BGDone( dst );
    return( AddrName( dst_name, tipe ) );
}


extern  an      MakeConst( float_handle cf, type_def *tipe )
/**********************************************************/
{
    return( AddrName( AllocConst( cf ), tipe ) );
}


extern  an      MakePoints( an name, type_def *tipe )
/***************************************************/
{
    an  new;

    new = AddrName( Points( name, tipe ), tipe );
    BGDone( name );
    return( new );
}

extern  an      RegName( hw_reg_set reg, type_def *tipe )
/*******************************************************/
{
    an  addr;

    addr = NewAddrName();
    addr->format = NF_NAME;
    addr->tipe = tipe;
    addr->u.n.name = AllocRegName( reg );
    return( addr );
}

extern  an      InsName( instruction *ins, type_def *tipe )
/*********************************************************/
{
    an  addr;

    addr = NewAddrName();
    addr->u.i.ins = ins;
    addr->tipe = tipe;
    addr->format = NF_INS;
    return( addr );
}

extern  name    *LoadTemp( name *temp, type_class_def class )
/***********************************************************/
{
    instruction *ins;

    ins = MakeMove( temp, AllocTemp( class ), class );
    temp = ins->result;
    AddIns( ins );
    return( temp );
}


static  name    *Temporary( name *temp, type_def *tipe )
/******************************************************/
{
    if( temp->n.class != N_TEMP ) {
        temp = LoadTemp( temp, TypeClass( tipe ) /*temp->n.name_class*/ );
    }
    return( temp );
}


extern  an      AddrEval( an addr )
/*********************************/
{
    an  new;

    new = AddrName( Temporary( GenIns( addr ), addr->tipe ), addr->tipe );
    AddrFree( addr );
    return( new );
}


extern  void    MoveAddress(  an src,  an  dest )
/***********************************************/
{
    dest->format = src->format;
    dest->class = src->class;
    dest->u.n.name  = src->u.n.name;
    dest->u.n.index = src->u.n.index;
    dest->u.n.offset= src->u.n.offset;
}


extern  void    Convert( an addr, type_class_def class )
/******************************************************/
{
    instruction *ins;

    if( addr->u.n.offset != 0 ) {
        ins = MakeBinary( OP_ADD, addr->u.n.name,
                                AllocIntConst( addr->u.n.offset ),
                                AllocTemp( addr->u.n.name->n.name_class ),
                                TypeClass( addr->tipe ) );
        addr->u.n.name = ins->result;
        AddIns( ins );
    }
    ins = MakeUnary( OP_CONVERT, addr->u.n.name, AllocTemp( class ), class );
    addr->u.n.name = ins->result;
    addr->u.n.offset = 0;
    AddIns( ins );
}


extern  bool    PointLess( an l_addr, an r_addr )
/***********************************************/
{
    if( l_addr->class != CL_POINTER && r_addr->class != CL_POINTER )
        return( FALSE );
    if( l_addr->u.n.offset != 0 || r_addr->u.n.offset != 0 ) return( FALSE );
    return( TRUE );
}


extern  an      AddrToIns( an addr )
/**********************************/
{
    instruction *ins;
    an          new;

    if( addr->format != NF_INS ) {
        ins = MakeMove( GenIns( addr ), NULL, TypeClass( addr->tipe ) );
        new = InsName( ins, addr->tipe );
        new->flags = addr->flags;
        new->u.n.alignment = addr->u.n.alignment;
        BGDone( addr );
        AddIns( ins );
    } else {
        new = addr;
    }
    return( new );
}


extern  an      AddrDuplicate( an node )
/**************************************/
{
    an          new;
    name        *op;

    InsToAddr( node );
    op = GenIns( node );
    new = AddrName( op, node->tipe );
    CopyAddr( new, node );
    return( new );
}

extern  an      AddrCopy( an node )
/*********************************/
{
    an  new;

    InsToAddr( node );
    new = NewAddrName();
    CopyAddr( node, new );
    return( new );
}


extern  an      AddrSave( an node )
/*********************************/
{
    InsToAddr( node );
    return( node );
}


extern  void    AddrDemote( an node )
/***********************************/
{
    node->flags |= FL_ADDR_DEMOTED;
    if( node->format == NF_INS ) {
        node->u.i.ins->ins_flags |= INS_DEMOTED;
    }
}


extern  name    *MaybeTemp( name *op, type_class_def kind )
/*********************************************************/
{
    if( op == NULL ) {
        op = AllocTemp( kind );
    }
    return( op );
}


extern  void    CheckPointer( an addr )
/*************************************/
{
    InsToAddr( addr );
    if( addr->format == NF_NAME && ( addr->tipe->attr & TYPE_POINTER ) ) {
        addr->u.n.index = Temporary( addr->u.n.name, addr->tipe );
        addr->u.n.name = NULL;
        addr->u.n.offset = 0;
        addr->class = CL_POINTER;
        addr->format = NF_ADDR;
    }
}


extern  void    FixCodePtr( an addr )
/***********************************/
{
    if( addr->format == NF_INS ) {
        addr->u.i.ins->ins_flags |= INS_CODE_POINTER;
    }
}


extern  bool    NeedPtrConvert( an addr, type_def * tipe )
/********************************************************/
{
    if( addr->format != NF_ADDR ) return( TRUE );
    if( addr->class == CL_ADDR_GLOBAL || addr->class == CL_ADDR_TEMP ) {
        if( tipe->refno == TY_NEAR_POINTER ) return( FALSE );
        if( tipe->refno == TY_LONG_POINTER ) return( FALSE );
        if( tipe->refno == TY_HUGE_POINTER ) return( FALSE );
    }
    return( TRUE );
}


extern  name    *LoadAddress( name *op, name *suggest, type_def *type_ptr )
/*************************************************************************/
{
    name                *new;
    type_class_def      class;

    if( op->n.class == N_INDEXED && !HasTrueBase( op ) ) {
        if( op->i.constant != 0 ) {
            class = op->i.index->n.name_class;
            new = MaybeTemp( suggest, class );
            AddIns( MakeBinary( OP_ADD, op->i.index,
                         AllocS32Const( op->i.constant ),
                         new, class ) );
        } else {
            new = op->i.index;
        }
    } else {
        if( suggest != NULL ) {
            class = suggest->n.name_class;
        } else {
            if( type_ptr->length == WORD_SIZE ) {
                class = WD;
            } else {
                class = CP;
            }
        }
        new = MaybeTemp( suggest, class );
        AddIns( MakeUnary( OP_LA, op, new, class ) );
    }
    return( new );
}


extern  an      MakeAddrName( cg_class class, cg_sym_handle sym, type_def *tipe )
/****************************************************************************/
{
    an          addr;
    fe_attr     attr;
    int         level;
    name        *op;

    addr = NewAddrName();
    if( class != CG_FE ) {
        op = (name *)SAllocMemory( sym, 0, class, TypeClass( tipe ), tipe->length );
        addr->u.n.name = op;
        addr->class = CL_ADDR_GLOBAL;
    } else {
        attr = FEAttr( sym );
        level = FELexLevel( sym );
        if( attr & FE_STATIC ) {
            op = (name *)SAllocMemory( sym, 0, class, TypeClass(tipe), tipe->length );
            if( ( attr & FE_MEMORY ) != EMPTY ) {
                op->v.usage |= NEEDS_MEMORY | USE_MEMORY;
            }
            addr->u.n.name = op;
            addr->class = CL_ADDR_GLOBAL;
            if( attr & FE_VOLATILE ) {
                op->v.usage |= VAR_VOLATILE | NEEDS_MEMORY | USE_MEMORY;
            }
            if( attr & FE_UNALIGNED ) {
                op->v.usage |= VAR_UNALIGNED;
            }
            if( attr & FE_CONSTANT ) {
                op->v.usage |= VAR_CONSTANT;
            }
        } else if( level != CurrProc->lex_level ) {
            op = Display( sym, level );
            addr->u.n.name = op;
            addr->format = NF_NAME;
        } else {
            op = SAllocUserTemp( sym, TypeClass( tipe ), tipe->length );
            if( attr & FE_MEMORY ) {
                op->v.usage |= NEEDS_MEMORY | USE_MEMORY;
            }
            if( attr & FE_ADDR_TAKEN ) {
                op->v.usage |= USE_ADDRESS;
            }
            addr->u.n.name = op;
            addr->class = CL_ADDR_TEMP;
            op->v.usage |= USE_IN_ANOTHER_BLOCK;
            if( attr & FE_VOLATILE ) {
                op->v.usage |= VAR_VOLATILE | NEEDS_MEMORY | USE_MEMORY;
            }
        }
    }
    addr->tipe = TypeAddress( NamePtrType( op ) );
    return( addr );
}
