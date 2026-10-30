"""Summary of a tatou.log (or tatou.prev.log): errors, drawing-order
contradictions and, with debug.logDrawOrder, what was drawn in each camera.

    python tools/readLog.py [log]                        summary (default: aitd1-data/tatou.log)
    python tools/readLog.py [log] --draw FLOOR CAMERA    last drawing order seen in that camera
    python tools/readLog.py [log] --order                every contradiction, with its boxes

A player's log is in UserData/AITD1 (or JACK) next to the programs.
"""
import argparse
import collections
import os
import re

DEFAULT_LOG = os.path.join(os.path.dirname(__file__), '..', '..', 'aitd1-data', 'tatou.log')


def blocks(lines, header, item):
    """Each header line with the item lines right after it."""
    current = None
    for line in lines:
        if line.startswith(header):
            current = [line]
            yield current
        elif current is not None and line.startswith(item):
            current.append(line)
        else:
            current = None


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('log', nargs='?', default=DEFAULT_LOG)
    parser.add_argument('--draw', nargs=2, type=int, metavar=('FLOOR', 'CAMERA'))
    parser.add_argument('--order', action='store_true')
    args = parser.parse_args()

    with open(args.log, encoding='utf-8', errors='replace') as file:
        lines = [line.rstrip('\r\n') for line in file]

    if args.draw:
        floor, camera = args.draw
        wanted = '[DRAW] frame: floor %d, camera %d,' % (floor, camera)
        found = [b for b in blocks(lines, '[DRAW] frame', '[DRAW]  ') if b[0].startswith(wanted)]
        if not found:
            print('No [DRAW] frame for floor %d camera %d (is debug.logDrawOrder = 1?)' % (floor, camera))
        for line in (found[-1] if found else []):
            print(line)
        return

    contradictions = list(blocks(lines, '[ORDER] contradiction', '[ORDER]  '))
    if args.order:
        for block in contradictions:
            print('\n'.join(block))
        return

    print('Log:', os.path.abspath(args.log))
    print(lines[0] if lines else '(empty)')

    problems = [l for l in lines if re.search(r'\b(CRITICAL|ERROR|FATAL)\b', l)]
    print('\nErrors: %d' % len(problems))
    for line in problems[:20]:
        print('  ' + line)
    warnings = [l for l in lines if re.search(r'\bWARNING\b', l)]
    print('Warnings: %d' % len(warnings))
    for line, count in collections.Counter(warnings).most_common(10):
        print('  %4d x %s' % (count, line))

    places = collections.Counter(re.sub(r' at .*', '', b[0]) for b in contradictions)
    print('\nDrawing-order contradictions: %d (details: --order)' % len(contradictions))
    for place, count in places.most_common():
        print('  %4d x %s' % (count, place))

    frames = collections.Counter(re.sub(r', \d+ objects.*', '', b[0])
                                 for b in blocks(lines, '[DRAW] frame', '[DRAW]  '))
    if frames:
        print('\nCameras with drawing order ([DRAW], seconds seen; details: --draw FLOOR CAMERA):')
        for frame, count in sorted(frames.items()):
            print('  %4d  %s' % (count, frame.replace('[DRAW] frame: ', '')))


if __name__ == '__main__':
    main()
