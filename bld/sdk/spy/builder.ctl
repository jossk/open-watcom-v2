# SPY Builder Control file
# ============================

set PROJDIR=<CWD>
set PROJNAME=wspy

[ INCLUDE <OWROOT>/build/master.ctl ]
[ LOG <LOGFNAME>.<LOGEXT> ]

[ INCLUDE <OWROOT>/build/defrule.ctl ]

[ BLOCK <1> rel ]
    cdsay <PROJDIR>

[ BLOCK <1> rel cprel ]
#======================
    <CCCMD> wini86/wspy.exe    <OWRELROOT>/binw/
    <CCCMD> wini86/wspyhk.dll  <OWRELROOT>/binw/
    <CCCMD> nt386/wspy.exe     <OWRELROOT>/binnt/
    <CCCMD> nt386/ntspyhk.dll  <OWRELROOT>/binnt/
    <CCCMD> ntaxp/wspy.exe     <OWRELROOT>/axpnt/
    <CCCMD> ntaxp/ntspyhk.dll  <OWRELROOT>/axpnt/

    <CCCMD> ntx64/wspy.exe     <OWRELROOT>/binnt64/
    <CCCMD> ntx64/ntspyhk.dll  <OWRELROOT>/binnt64/

[ BLOCK . . ]
#============
cdsay <PROJDIR>
