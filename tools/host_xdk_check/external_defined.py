#!/usr/bin/env python3
"""Approximate the functions the rest of the XEX defines (the upstream files
tools/build_xex.ps1 compiles whole, plus the native src/xdk sources outside the
match build). Used to tell real new link failures from names that are simply
resolved outside the match objects."""
import os, re, sys
root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
base = os.path.join(root, 'upstream/melee-pc/src/sysdolphin/baselib')
files = [os.path.join(base, n) for n in (
    'aobj.c bytecode.c class.c cobj.c debug.c devcom.c displayfunc.c dobj.c fobj.c fog.c gobj.c '
    'gobjgxlink.c gobjinit.c gobjobject.c gobjplink.c gobjproc.c gobjuserdata.c hash.c id.c jobj.c '
    'list.c lobj.c mtx.c objalloc.c object.c quatlib.c random.c robj.c spline.c synth.c util.c wobj.c '
    'controller.c memory.c archive.c').split()]
files += [os.path.join(root, 'upstream/melee-pc/src/melee', n) for n in (
    'lb/lb_00CE.c', 'lb/lbtime.c', 'gm/gm_1A36.c', 'mn/mnmain.c', 'mn/mn_22EC.c')]
skip = {'fighter_glue_xdk.c', 'fighter_unported_xdk.c'}
for n in sorted(os.listdir(os.path.join(root, 'src/xdk'))):
    if n.endswith(('.c', '.cpp')) and n not in skip:
        files.append(os.path.join(root, 'src/xdk', n))
pat = re.compile(r'^(?!static\b)[A-Za-z_][\w \*]*?[\s\*](\w+)\s*\([^;]*$')
# CRT names the XEX links through /MT.
names = set(['lb_8000B1CC', 'OSPanic', 'tan', 'atoi', 'sscanf', 'HSD_GObj_804D7814', 'malloc', 'free', 'calloc', 'realloc', 'memcpy', 'memset',
             'memmove', 'sprintf', 'strlen', 'strcmp', 'sqrt', 'sin', 'cos', 'atan2', 'fabs', 'pow'])
for f in files:
    for line in open(f, encoding='latin-1'):
        m = pat.match(line)
        if m and m.group(1) not in ('if', 'while', 'for', 'switch', 'return'):
            names.add(m.group(1))
print('\n'.join(sorted(names)))
