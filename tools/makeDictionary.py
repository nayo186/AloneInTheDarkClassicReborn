"""Builds docs/DICCIONARIO.md, the dictionary of methods, from the headers.

Each public function is declared in a .h with a comment right above it (or at
the end of its line) saying what it is for; this script collects them by layer
and file. Change a function -> change its comment -> run it again:

    python tools/makeDictionary.py           writes docs/DICCIONARIO.md
    python tools/makeDictionary.py --missing lists the functions without a comment

The engine's own headers of third-party code (fmopl.h) and the generated
native scripts are left out.
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
SRC = os.path.join(REPO, 'TatouSource')

LAYERS = [
    ('util', 'Herramientas sueltas', ['FitdLib/util']),
    ('platform', 'El sistema', ['FitdLib/platform']),
    ('core', 'El juego de 1992', ['FitdLib/core']),
    ('scripts', 'La lógica del juego', ['FitdLib/scripts', 'FitdLib/scripts/native']),
    ('remaster', 'Extras propios', ['FitdLib/remaster/config', 'FitdLib/remaster/graphics', 'FitdLib/remaster/ui']),
    ('Shared', 'Compartido por el juego y el launcher', ['Shared', 'Shared/ui']),
    ('Launcher', 'Launcher.exe', ['Launcher']),
]
SKIP_FILES = {'fmopl.h', 'nativeLifeHelpers.h'}  # third-party / one helper per script instruction

# A function declaration on one line (possibly continued until ';')
DECL = re.compile(r'^\s*(?:extern\s+(?:"C"\s+)?)?(?:static\s+)?(?:inline\s+)?'
                  r'(?:const\s+)?[A-Za-z_][\w:<>,\s\*&]*?[\s\*&]'
                  r'(?P<name>[A-Za-z_]\w*)\s*\((?P<args>[^;{}]*)\)\s*(?:const\s*)?(?:;|\{)')
SKIP_NAMES = {'if', 'while', 'for', 'switch', 'return', 'sizeof', 'operator'}
POINTER = re.compile(r'^\s*extern\s+[\w:<>\*&\s]+\(\s*\*\s*(?P<name>\w+)\s*\)\s*\((?P<args>[^;]*)\)\s*;')


def comment_above(lines, i):
    """The // comment block right above line i (skipping nothing else)."""
    out = []
    j = i - 1
    while j >= 0:
        t = lines[j].strip()
        if t.startswith('//'):
            text = t.lstrip('/').strip()
            if text and not set(text) <= set('-=*'):
                out.insert(0, text)
            j -= 1
        else:
            break
    return ' '.join(out)


def scan_header(path):
    lines = open(path, encoding='utf-8', errors='replace').read().split('\n')
    found = []
    depth_class = 0
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith(('//', '#', '*', '/*')):
            continue
        m = POINTER.match(line) or DECL.match(line)
        if not m:
            continue
        name = m.group('name')
        if name in SKIP_NAMES or name.startswith('operator') or stripped.startswith(('typedef', 'return ')):
            continue
        # an object built with values (const ImVec2 corner(pos.x + size.x, ...)), not a function
        args_text = m.group('args').replace('...', '')
        if '.' in args_text or re.match(r'\s*[-\d]', args_text):
            continue
        # skip calls and members inside function bodies (indented lines of inline code)
        if DECL.match(line) and not POINTER.match(line):
            indent = len(line) - len(line.lstrip())
            if indent > 4:
                continue
        trailing = ''
        if '//' in line:
            trailing = line.split('//', 1)[1].strip()
        desc = comment_above(lines, i) or trailing
        args = ' '.join(m.group('args').split())
        found.append((name, args, desc, i + 1))
    return found


def collect():
    result = []
    for key, title, folders in LAYERS:
        files = []
        for folder in folders:
            d = os.path.join(SRC, folder)
            for n in sorted(os.listdir(d)):
                if n.endswith('.h') and n not in SKIP_FILES:
                    funcs = scan_header(os.path.join(d, n))
                    if funcs:
                        files.append((folder + '/' + n, funcs))
        result.append((key, title, files))
    return result


def write(result):
    out = ['# Diccionario de métodos', '',
           'Cada función pública del proyecto con para qué sirve, por capa y archivo. '
           'Las capas y los archivos se explican en `docs/MAPA_CODIGO.md`.', '',
           '**Generado** por `tools/makeDictionary.py` a partir de los comentarios de las cabeceras (`.h`): '
           'no se edita a mano. Para cambiar una descripción, se cambia el comentario de la función y se vuelve a generar.', '']
    total = 0
    for key, title, files in result:
        out.append('## %s: %s' % (key, title))
        out.append('')
        for path, funcs in files:
            out.append('### `%s`' % path.replace('FitdLib/', ''))
            out.append('')
            out.append('| Función | Para qué sirve |')
            out.append('| --- | --- |')
            for name, args, desc, line in funcs:
                out.append('| `%s` | %s |' % (name, desc.replace('|', '\\|') if desc else '—'))
                total += 1
            out.append('')
    with open(os.path.join(REPO, 'docs', 'DICCIONARIO.md'), 'w', encoding='utf-8', newline='\r\n') as f:
        f.write('\n'.join(out))
    print('docs/DICCIONARIO.md: %d functions' % total)


if __name__ == '__main__':
    data = collect()
    if '--missing' in sys.argv:
        n = 0
        for key, title, files in data:
            for path, funcs in files:
                for name, args, desc, line in funcs:
                    if not desc:
                        print('%s:%d  %s' % (path, line, name))
                        n += 1
        print(n, 'without a comment')
    else:
        write(data)
