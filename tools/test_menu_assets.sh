#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ISO="${1:-$ROOT/dist/melee.iso}"
PNG_OUT="${2:-$ROOT/build-x360/host-tests/menu_assets}"
OBJ="$ROOT/build-x360/host-tests"
mkdir -p "$OBJ/msvc" "$PNG_OUT"

VCVARS="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
WROOT="$(cygpath -w "$ROOT")"
WOBJ="$(cygpath -w "$OBJ")\msvc"
BAT="$OBJ/msvc/test_menu_assets.bat"

cat > "$BAT" <<BATCH
@echo off
call "$VCVARS" >nul 2>&1
if errorlevel 1 exit /b 1
set ROOT=$WROOT
set OBJ=$WOBJ
set BL=%ROOT%\upstream\melee-pc\src\sysdolphin\baselib
set INC=/I%ROOT%\src\xdk /I%ROOT%\src\common /I%ROOT%\upstream\melee-pc\src /I%ROOT%\upstream\melee-pc\src\sdk_include
set COMMON=/nologo /D_CRT_SECURE_NO_WARNINGS /MT /Od
set CC_UP=%COMMON% /TC /W3 %INC%
set CC_NEW=%COMMON% /W4 /WX %INC%

cl %CC_NEW% /TC /FI%ROOT%\src\xdk\memory_xdk_compat.h /c %BL%\memory.c /Fo%OBJ%\memory.obj || exit /b 1
cl %CC_NEW% /TP /EHsc /c %ROOT%\src\xdk\memory_xdk.cpp /Fo%OBJ%\memory_xdk.obj || exit /b 1
cl %CC_NEW% /TP /EHsc /c %ROOT%\src\xdk\hsd_class_xdk.cpp /Fo%OBJ%\hsd_class_xdk.obj || exit /b 1
for %%f in (hash class object objalloc id) do (
    cl %CC_UP% /FI%ROOT%\src\xdk\hsd_class_xdk_compat.h /c %BL%\%%f.c /Fo%OBJ%\%%f.obj || exit /b 1
)
cl %CC_UP% /DTARGET_PC /FI%ROOT%\src\xdk\hsd_class_xdk_compat.h /c %BL%\debug.c /Fo%OBJ%\debug.obj || exit /b 1
cl %CC_NEW% /TP /EHsc /c %ROOT%\src\xdk\hsdmath_xdk.cpp /Fo%OBJ%\hsdmath_xdk.obj || exit /b 1
for %%f in (mtx quatlib fobj random) do (
    cl %CC_UP% /FI%ROOT%\src\xdk\hsdmath_xdk_compat.h /c %BL%\%%f.c /Fo%OBJ%\%%f.obj || exit /b 1
)
cl %COMMON% /TP /EHsc /W3 %INC% /c %ROOT%\src\xdk\spline_xdk.cpp /Fo%OBJ%\spline.obj || exit /b 1
cl %CC_NEW% /TP /EHsc /c %ROOT%\src\xdk\hsdanim_xdk.cpp /Fo%OBJ%\hsdanim_xdk.obj || exit /b 1
for %%f in (list aobj dobj robj util bytecode) do (
    cl %CC_UP% /FI%ROOT%\src\xdk\hsdanim_xdk_compat.h /c %BL%\%%f.c /Fo%OBJ%\%%f.obj || exit /b 1
)
cl %CC_NEW% /TP /EHsc /c %ROOT%\src\xdk\hsdjobj_xdk.cpp /Fo%OBJ%\hsdjobj_xdk.obj || exit /b 1
for %%f in (jobj wobj) do (
    cl %CC_UP% /FI%ROOT%\src\xdk\hsdjobj_xdk_compat.h /c %BL%\%%f.c /Fo%OBJ%\%%f.obj || exit /b 1
)
cl %CC_NEW% /TC /c %ROOT%\src\common\gcm.c /Fo%OBJ%\gcm.obj || exit /b 1
cl %CC_NEW% /TC /c %ROOT%\src\xdk\hsd_texture_xdk.c /Fo%OBJ%\hsd_texture_xdk.obj || exit /b 1
cl %CC_NEW% /TC /I%BL% /FI%ROOT%\src\xdk\hsdanim_xdk_compat.h /c %ROOT%\tests\host\test_menu_assets.c /Fo%OBJ%\test_menu_assets.obj || exit /b 1

link /nologo /LARGEADDRESSAWARE:NO /DYNAMICBASE:NO /OUT:%OBJ%\test_menu_assets.exe %OBJ%\memory.obj %OBJ%\memory_xdk.obj %OBJ%\hsd_class_xdk.obj %OBJ%\hash.obj %OBJ%\debug.obj %OBJ%\class.obj %OBJ%\object.obj %OBJ%\objalloc.obj %OBJ%\id.obj %OBJ%\hsdmath_xdk.obj %OBJ%\mtx.obj %OBJ%\quatlib.obj %OBJ%\spline.obj %OBJ%\fobj.obj %OBJ%\random.obj %OBJ%\hsdanim_xdk.obj %OBJ%\list.obj %OBJ%\aobj.obj %OBJ%\dobj.obj %OBJ%\robj.obj %OBJ%\util.obj %OBJ%\bytecode.obj %OBJ%\hsdjobj_xdk.obj %OBJ%\jobj.obj %OBJ%\wobj.obj %OBJ%\gcm.obj %OBJ%\hsd_texture_xdk.obj %OBJ%\test_menu_assets.obj || exit /b 1
%OBJ%\test_menu_assets.exe "$(cygpath -w "$ISO")" "$(cygpath -w "$PNG_OUT")"
exit /b %errorlevel%
BATCH

cmd.exe //c "$(cygpath -w "$BAT")"
