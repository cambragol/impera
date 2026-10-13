"""Compare every original TLK greeting/farewell in both introduction states.

Assets remain local. Each reference directory is also consumed by the independent
Workshop host parity test; failures never count as skipped conversations.
"""
import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess


def run(reference, shared, game, output):
    cases = 0
    script = output / 'original-keys.tsv'
    # Wait/scroll prompts may consume a key before the topic editor sees it.
    for bank in ('TOWNE', 'DWELLING', 'CASTLE', 'KEEP'):
        resource = game / (bank + '.TLK')
        data = resource.read_bytes()
        count, = struct.unpack_from('<H', data)
        for index in range(count):
            identity, = struct.unpack_from('<H', data, 2 + index * 4)
            for introduced, topics in ((0, False), (1, False), (0, True), (1, True)):
                name = f'original-{bank}-{identity}-{introduced}-{int(topics)}'
                text = 'y\rn\rhelp\rhind\rbye\r' * 64
                if topics:
                    text = 'NaMe\rJoB\r' + text
                if bank == 'CASTLE' and identity == 8:
                    text = ('xy\rxn\rx' if topics else 'xy\rxown\rx') * 32
                if bank == 'CASTLE' and identity == 10:
                    text = (('xAvatar\r' if not introduced else 'x') +
                            ('y\ry\rimpe\rbye\r' if topics else 'n\rxx')) * 32
                script.write_text(''.join(f'0 {ord(ch)}\n' for ch in text))
                for executable, variant in ((reference, 'reference'), (shared, 'shared')):
                    root = output / variant
                    result = subprocess.run(
                        [str(executable), '--output', str(root), '--tlk', str(resource),
                         '--npc', str(identity), '--introduced', str(introduced),
                         '--script', str(script), '--allow-unused-keys', '1'],
                        capture_output=True, text=True, timeout=15)
                    if result.returncode:
                        raise RuntimeError(f'{name}/{variant}: {result.stdout}\n{result.stderr}')
                    shutil.move(root / 'custom', root / name)
                cases += 1
    print(f'PASS {cases} original dialogue introduction/topic/farewell paths')
    controls = set()
    for events in (output / 'reference').glob('*/events.jsonl'):
        for line in events.read_text().splitlines():
            event = json.loads(line)
            if event['event'] == 'opcode' and 128 <= event['value'] <= 159:
                controls.add(event['value'])
    if controls != set(range(128, 160)):
        raise RuntimeError(f'Missing dialogue control-byte coverage: {set(range(128, 160)) - controls}')
    print('PASS all 32 dialogue control bytes reached')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('reference', 'shared', 'game', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    run(args.reference, args.shared, args.game, args.output)
