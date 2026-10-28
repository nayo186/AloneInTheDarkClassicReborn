"""Checks the layer order (docs/MAPA_CODIGO.md, section 1): lists every
#include that goes up a layer.

    python tools/checkLayers.py

The only accepted crossing is platform -> core (platform uses the game's
types, common.h); anything else printed is a mistake to fix."""
import os, re, sys
from collections import defaultdict

ROOT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'TatouSource') + '/'
LAYERS = {  # folder -> layer
    'FitdLib/util': 'util', 'FitdLib/platform': 'platform', 'FitdLib/core': 'core',
    'FitdLib/scripts': 'scripts', 'FitdLib/scripts/native': 'scripts',
    'FitdLib/remaster/graphics': 'remaster', 'FitdLib/remaster/ui': 'remaster',
    'FitdLib/remaster/config': 'remaster', 'Shared': 'shared', 'Shared/ui': 'shared',
    'Launcher': 'launcher', 'Fitd': 'app',
}
# allowed dependencies (a layer may include itself and these)
ALLOWED = {
    'util': {'util'},
    'platform': {'platform', 'util', 'shared'},
    'core': {'core', 'scripts', 'platform', 'util'},
    'scripts': {'scripts', 'core', 'platform', 'util'},
    'remaster': {'remaster', 'core', 'scripts', 'platform', 'util', 'shared'},
    'shared': {'shared'},
    'launcher': {'launcher', 'shared'},
    'app': {'app', 'core', 'platform', 'util', 'remaster', 'scripts', 'shared'},
}
where = {}
files = []
for folder, layer in LAYERS.items():
    d = ROOT + folder
    for n in os.listdir(d):
        if n.endswith(('.cpp', '.h')):
            where.setdefault(n, layer)
            files.append((folder + '/' + n, layer))
bad = defaultdict(list)
for path, layer in files:
    s = open(ROOT + path, encoding='utf-8', errors='replace').read()
    for inc in re.findall(r'^\s*#\s*include\s+"([^"]+)"', s, re.M):
        name = os.path.basename(inc)
        target = where.get(name)
        if target and target not in ALLOWED[layer]:
            bad[(layer, target)].append('%s -> %s' % (path, name))
for k in sorted(bad):
    print('== %s includes %s (%d)' % (k[0], k[1], len(bad[k])))
    for line in bad[k]:
        print('   ' + line)
