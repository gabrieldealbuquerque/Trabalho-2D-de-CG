#!/usr/bin/env python3
"""Audita chamadas GL; allowlist conferida no registro oficial Khronos GL_VERSION_1_0."""
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
allowed = set("""glBegin glClear glClearColor glColor3f glDisable glEnd glGetError
glGetString glGetBooleanv glGetIntegerv glLineWidth glLoadIdentity glMatrixMode
glOrtho glPixelStorei glPopMatrix glPushMatrix glReadBuffer glReadPixels glRotated
glScaled glTranslated glVertex2d glVertex2i glViewport""".split())
if len(sys.argv) == 2:
    feature = ET.parse(sys.argv[1]).getroot().find("feature[@name='GL_VERSION_1_0']")
    allowed = {node.attrib['name'] for node in feature.findall('./require/command')}
used = set()
for path in list((root / 'src').glob('*.[ch]pp')) + list((root / 'tests').glob('*.cpp')):
    used.update(re.findall(r'\b(gl[A-Z]\w*)\s*\(', path.read_text()))
invalid = used - allowed
if invalid:
    raise SystemExit('Chamadas fora da lista OpenGL 1.0: ' + ', '.join(sorted(invalid)))
print(f'OK: {len(used)} chamadas OpenGL pertencem a API 1.0; sem shaders, VBOs ou extensoes.')
