#!/usr/bin/env python3
from pathlib import Path
import re
import sys
import zipfile

root = Path(__file__).resolve().parents[1]
author = sys.argv[1] if len(sys.argv) > 1 else ''
if not re.fullmatch(r'[A-Za-z][A-Za-z0-9_-]*', author):
    raise SystemExit('Use: make dist AUTOR=NomeCompleto (sem espacos ou acentos).')
destination = root / 'entrega' / f'{author}.zip'
destination.parent.mkdir(exist_ok=True)
files = [root / name for name in ['Makefile', 'make.sh', 'README.md', 'configuracoes.xml', '.gitignore']]
for directory in ['src', 'third_party', 'tests', 'docs']:
    files += [p for p in (root / directory).rglob('*') if p.is_file() and '__pycache__' not in p.parts]
with zipfile.ZipFile(destination, 'w', zipfile.ZIP_DEFLATED) as archive:
    for path in sorted(files):
        if path.suffix in {'.o', '.so', '.a', '.exe', '.pyc'}:
            raise SystemExit(f'Artefato compilado inesperado: {path}')
        archive.write(path, str(path.relative_to(root)))
print(destination)
