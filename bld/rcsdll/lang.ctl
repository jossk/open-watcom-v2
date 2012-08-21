# RCSDLL Builder Control file
# ===========================

set PROJDIR=<CWD>

[ INCLUDE <OWROOT>/build/master.ctl ]
[ LOG <LOGFNAME>.<LOGEXT> ]

cdsay .

[ BLOCK <1> build rel2 ]
#=======================
    pmake -d build <2> <3> <4> <5> <6> <7> <8> <9> -h

[ BLOCK <1> rel2 ]
#=================
    cdsay <PROJDIR>

[ BLOCK <1> rel2 cprel2 ]
#========================
  [ IFDEF (os_dos "") <2*> ]
    <CPCMD> wini86/rcsdll.dll  <RELROOT>/binw/rcsdll.dll
    <CPCMD> bat/*.bat          <RELROOT>/binw/

  [ IFDEF (os_os2 "") <2*> ]
    <CPCMD> os2386/rcsdll.dll  <RELROOT>/binp/dll/rcsdll.dll
    <CPCMD> cmd/*.cmd          <RELROOT>/binp/

  [ IFDEF (os_nt "") <2*> ]
    <CPCMD> nt386/rcsdll.dll   <RELROOT>/binnt/rcsdll.dll

  [ IFDEF (cpu_axp) <2*> ]
    <CPCMD> ntaxp/rcsdll.dll   <RELROOT>/axpnt/rcsdll.dll

[ BLOCK <1> clean ]
#==================
    pmake -d build <2> <3> <4> <5> <6> <7> <8> <9> -h clean

[ BLOCK . . ]
#============

cdsay <PROJDIR>
