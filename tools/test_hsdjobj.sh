#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OBJ="$ROOT/build-x360/host-tests"
BASELIB="$ROOT/upstream/melee-pc/src/sysdolphin/baselib"

mkdir -p "$OBJ"

if command -v cc >/dev/null 2>&1 && command -v c++ >/dev/null 2>&1; then
    OUT="$OBJ/test_hsdjobj"
    INC=(-I"$ROOT/src/xdk" -I"$ROOT/upstream/melee-pc/src"
         -I"$ROOT/upstream/melee-pc/src/sdk_include")

    cc -std=c11 -Wall -Wextra -Werror "${INC[@]}" \
        -include "$ROOT/src/xdk/memory_xdk_compat.h" \
        -c "$BASELIB/memory.c" -o "$OBJ/memory.o"
    c++ -std=c++17 -Wall -Wextra -Werror -I"$ROOT/src/xdk" \
        -c "$ROOT/src/xdk/memory_xdk.cpp" -o "$OBJ/memory_xdk.o"
    c++ -std=c++17 -Wall -Wextra -Werror -I"$ROOT/src/xdk" \
        -c "$ROOT/src/xdk/hsd_class_xdk.cpp" -o "$OBJ/hsd_class_xdk.o"

    for f in hash debug class object objalloc id; do
        extra=()
        if [ "$f" = "debug" ]; then
            extra+=(-DTARGET_PC)
        fi
        cc -std=c11 -Wall -Wextra -Werror "${extra[@]}" "${INC[@]}" \
            -include "$ROOT/src/xdk/hsd_class_xdk_compat.h" \
            -c "$BASELIB/$f.c" -o "$OBJ/$f.o"
    done

    c++ -std=c++17 -Wall -Wextra -Werror "${INC[@]}" \
        -c "$ROOT/src/xdk/hsdmath_xdk.cpp" -o "$OBJ/hsdmath_xdk.o"
    for f in mtx quatlib fobj random; do
        cc -std=c11 -Wall "${INC[@]}" \
            -include "$ROOT/src/xdk/hsdmath_xdk_compat.h" \
            -c "$BASELIB/$f.c" -o "$OBJ/$f.o"
    done
    c++ -std=c++17 -Wall "${INC[@]}" \
        -c "$ROOT/src/xdk/spline_xdk.cpp" -o "$OBJ/spline.o"

    c++ -std=c++17 -Wall -Wextra -Werror "${INC[@]}" \
        -c "$ROOT/src/xdk/hsdanim_xdk.cpp" -o "$OBJ/hsdanim_xdk.o"
    for f in list aobj dobj robj util bytecode; do
        cc -std=c11 -Wall "${INC[@]}" \
            -include "$ROOT/src/xdk/hsdanim_xdk_compat.h" \
            -c "$BASELIB/$f.c" -o "$OBJ/$f.o"
    done

    c++ -std=c++17 -Wall -Wextra -Werror "${INC[@]}" \
        -c "$ROOT/src/xdk/hsdjobj_xdk.cpp" -o "$OBJ/hsdjobj_xdk.o"
    for f in jobj wobj; do
        cc -std=c11 -Wall "${INC[@]}" \
            -include "$ROOT/src/xdk/hsdjobj_xdk_compat.h" \
            -c "$BASELIB/$f.c" -o "$OBJ/$f.o"
    done

    cc -std=c11 -Wall -Wextra -Werror -I"$BASELIB" "${INC[@]}" \
        -include "$ROOT/src/xdk/hsdjobj_xdk_compat.h" \
        -c "$ROOT/tests/host/test_hsdjobj.c" -o "$OBJ/test_hsdjobj.o"

    c++ "$OBJ/memory.o" "$OBJ/memory_xdk.o" "$OBJ/hsd_class_xdk.o" \
        "$OBJ/hash.o" "$OBJ/debug.o" "$OBJ/class.o" "$OBJ/object.o" \
        "$OBJ/objalloc.o" "$OBJ/id.o" "$OBJ/hsdmath_xdk.o" \
        "$OBJ/mtx.o" "$OBJ/quatlib.o" "$OBJ/spline.o" "$OBJ/fobj.o" \
        "$OBJ/random.o" "$OBJ/hsdanim_xdk.o" "$OBJ/list.o" "$OBJ/aobj.o" \
        "$OBJ/dobj.o" "$OBJ/robj.o" "$OBJ/util.o" "$OBJ/bytecode.o" \
        "$OBJ/hsdjobj_xdk.o" "$OBJ/jobj.o" "$OBJ/wobj.o" \
        "$OBJ/test_hsdjobj.o" -lm -o "$OUT"
    "$OUT"
    exit 0
fi

VCVARS="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
WROOT="$(cygpath -w "$ROOT")"
WOBJ="$(cygpath -w "$OBJ")\msvc"
mkdir -p "$OBJ/msvc"
BAT="$OBJ/msvc/test_hsdjobj.bat"

cat > "$BAT" <<BATCH
@echo off
call "$VCVARS" >nul 2>&1
if errorlevel 1 exit /b 1
set ROOT=$WROOT
set OBJ=$WOBJ
set BL=%ROOT%\upstream\melee-pc\src\sysdolphin\baselib
set INC=/I%ROOT%\src\xdk /I%ROOT%\upstream\melee-pc\src /I%ROOT%\upstream\melee-pc\src\sdk_include
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

cl %CC_NEW% /TC /I%BL% /FI%ROOT%\src\xdk\hsdjobj_xdk_compat.h /c %ROOT%\tests\host\test_hsdjobj.c /Fo%OBJ%\test_hsdjobj.obj || exit /b 1

link /nologo /OUT:%OBJ%\test_hsdjobj.exe %OBJ%\memory.obj %OBJ%\memory_xdk.obj %OBJ%\hsd_class_xdk.obj %OBJ%\hash.obj %OBJ%\debug.obj %OBJ%\class.obj %OBJ%\object.obj %OBJ%\objalloc.obj %OBJ%\id.obj %OBJ%\hsdmath_xdk.obj %OBJ%\mtx.obj %OBJ%\quatlib.obj %OBJ%\spline.obj %OBJ%\fobj.obj %OBJ%\random.obj %OBJ%\hsdanim_xdk.obj %OBJ%\list.obj %OBJ%\aobj.obj %OBJ%\dobj.obj %OBJ%\robj.obj %OBJ%\util.obj %OBJ%\bytecode.obj %OBJ%\hsdjobj_xdk.obj %OBJ%\jobj.obj %OBJ%\wobj.obj %OBJ%\test_hsdjobj.obj || exit /b 1
%OBJ%\test_hsdjobj.exe
exit /b %errorlevel%
BATCH

cmd.exe //c "$(cygpath -w "$BAT")"
