"""Experimental native donor preparation from extracted BT4 B14 data only.

This does not run or read the BT3 creator. Output is private game data. The
Donor identities come from the BT4 roster, without substituting legacy names.
Runtime parity remains a
separate check before this importer can replace the development source cache.
"""
import argparse
import json
from pathlib import Path
import struct

from prepare_warrior_sources import LIMIT, fingerprint, validate_model


def unpack(data, terminal=True):
    if not 12 <= len(data) <= LIMIT:
        raise ValueError('Invalid BT4 source pack size')
    count = struct.unpack_from('<I', data)[0]
    table = (count + (2 if terminal else 1)) * 4
    if not 0 < count <= 4096 or table > len(data):
        raise ValueError('Invalid BT4 source pack table')
    raw = struct.unpack_from(f'<{count + (1 if terminal else 0)}I', data, 4)
    offsets = [n & ~3 for n in raw]
    if not terminal:
        # B14's external lip tables provide 100 starts; the next word is not
        # consistently an end offset. Retain the final stream through EOF.
        offsets.append(len(data))
    if offsets != sorted(offsets) or offsets[0] < table or offsets[-1] > len(data):
        raise ValueError('Invalid BT4 source pack offsets')
    flags = [n & 3 for n in (raw[:-1] if terminal else raw)]
    return [data[a:b] for a, b in zip(offsets, offsets[1:])], flags


def pack(entries, flags=None):
    flags = flags or [0] * len(entries)
    if len(flags) != len(entries) or not 0 < len(entries) <= 4096:
        raise ValueError('Invalid prepared model table')
    offset = ((len(entries) + 2) * 4 + 63) & ~63
    if offset + sum((len(entry) + 63) & ~63 for entry in entries) > LIMIT:
        raise ValueError('Prepared model exceeds resource limit')
    if any(flag not in range(4) for flag in flags):
        raise ValueError('Invalid prepared model entry flags')
    result = bytearray(offset)
    struct.pack_into('<I', result, 0, len(entries))
    for i, entry in enumerate(entries):
        struct.pack_into('<I', result, 4 + i * 4, offset | flags[i])
        result.extend(entry)
        result.extend(bytes((-len(entry)) % 64))
        offset = len(result)
    struct.pack_into('<I', result, 4 + len(entries) * 4, offset)
    if len(result) > LIMIT:
        raise ValueError('Prepared model exceeds resource limit')
    return bytes(result)


def read_resource(root, number):
    if number >= 0x20000:
        volume, index = 3, number - 0x20000
    elif number >= 0xD48:
        volume, index = 2, number - 0xD48
    elif number > 0:
        volume, index = 1, number - 1
    else:
        volume, index = 0, number
    parent = (root / f'pzs4us{volume}').resolve(strict=True)
    path = parent / f'{index:05d}.bin'
    if path.is_symlink() or path.resolve(strict=True).parent != parent:
        raise ValueError('Invalid BT4 source path')
    size = path.stat().st_size
    if not 0 < size <= LIMIT:
        raise ValueError('Invalid BT4 source resource size')
    return path.read_bytes()


def embed(model, metadata, text, english_lips, japanese_lips):
    entries, flags = unpack_model(model)
    if len(entries) not in (52, 252) or len(metadata) != 3 or len(metadata[1]) != 0x30D0:
        raise ValueError('Unsupported BT4 fighter metadata layout')
    # B14 mixes external-metadata models with some legacy embedded containers
    # (for example a damaged costume). Ordinary BT4 binding replaces their
    # legacy metadata with the external BT4 records too. Keep all 52 original
    # model sections and reconstruct the same BT4 metadata below.
    entries, flags = entries[:52], flags[:52]
    if not text.startswith(b'\xff\xfe'):
        raise ValueError('Invalid BT4 technique text')
    ai, ai_flags = unpack(metadata[2])
    if len(ai) != 3 or len(english_lips[0]) != 100 or len(japanese_lips[0]) != 100:
        raise ValueError('Unsupported BT4 AI or lip table')
    entries += [b''] * 200
    flags += [0] * 200
    entries[0] = metadata[0]
    # The runtime's existing BT4 binding offsets, split into the embedded
    # representation consumed by native assembly. Preserve complete records.
    offsets = [(17, 0, 0x100), (18, 0x100, 0x2240),
               (19, 0x2240, 0x2500), (20, 0x2500, 0x25C0),
               (21, 0x25C0, 0x29D0), (23, 0x29D0, 0x2C50),
               (24, 0x2C50, 0x2D50), (26, 0x2D50, 0x2F50),
               (27, 0x2F50, 0x30D0)]
    for index, start, end in offsets:
        entries[index] = metadata[1][start:end]
        flags[index] = 0
    # B14 provides one script in the disc's language. The BT4 marker tells the
    # runtime to select this entry directly, as it does for external BT4 text.
    # Duplicating it five times would waste model-buffer space.
    entries[44], flags[44] = text, 0
    for i in range(45, 49):
        entries[i], flags[i] = b'', 0
    entries[49:52], flags[49:52] = ai, ai_flags
    entries[52:152], flags[52:152] = english_lips
    entries[152:252], flags[152:252] = japanese_lips
    entries.append(b'BT4WARRI' + struct.pack('<fI', 1.0, 0))
    flags.append(0)
    result = pack(entries, flags)
    validate_model(result)
    return result


def unpack_model(data):
    """Read B14's stripped models without following stale legacy tail offsets.

    A few 52-entry containers retain offsets for a removed legacy tail. Only
    a wholly zero physical suffix may stand in for those absent sections.
    Geometry, facial draws, textures and secondary chains must be complete.
    Generic resource decoding remains strict.
    """
    try:
        return unpack(data)
    except ValueError:
        if not 216 <= len(data) <= LIMIT or struct.unpack_from('<I', data)[0] != 52:
            raise
        raw = struct.unpack_from('<53I', data, 4)
        offsets = [n & ~3 for n in raw]
        if offsets != sorted(offsets) or offsets[0] < 216 or offsets[-1] > LIMIT:
            raise ValueError('Invalid stripped BT4 model offsets')
        missing = next((i for i in range(52) if offsets[i + 1] > len(data)), None)
        if missing is None or missing <= 22 or offsets[missing] > len(data) or any(data[offsets[missing]:]):
            raise ValueError('BT4 model has an incomplete nonempty section')
        entries = [data[a:b] if i < missing else b''
                   for i, (a, b) in enumerate(zip(offsets, offsets[1:]))]
        return entries, [n & 3 for n in raw[:-1]]


def costume_families(table, fighters):
    if len(table) < 250 * 60:
        raise ValueError('Incomplete BT4 costume table')
    first, families = 5650, {}
    for i in range(250):
        count = struct.unpack_from('<H', table, i * 60 + 40)[0]
        if count > 16:
            raise ValueError('Invalid BT4 costume count')
        selectable = table[i * 60 + 10]
        # Hidden entries outside the requested donor set can advertise aliases
        # beyond their physical family. Their physical counts still determine
        # the following model offsets; they do not supply selectable models here.
        if i in fighters and not 1 <= selectable <= count:
            raise ValueError('Invalid BT4 selectable costume count')
        families[i] = first, count, selectable
        first += count * 2
    return families


def playable_fighters(table, roster):
    """Exclude menu sentinels and blank unused identities, not real fighters."""
    families = costume_families(table, set())
    return [i for i, (_, count, selectable) in families.items()
            if roster.get(i) and 1 <= selectable <= count]


def prepare(source, output, fighters=None, progress=None):
    source = source.resolve(strict=True)
    profile = json.loads((source / 'native-disc.json').read_text())
    # Confirm the disc profile before interpreting fixed resource offsets.
    if profile.get('game') != 'bt4' or profile.get('revision') != 'b14-rev2':
        raise ValueError('Choose extracted BT4 B14 REV2 game data')
    if output.exists():
        raise ValueError('Choose a new private source-cache directory')
    roster = {}
    for line in (source / 'native-roster.tsv').read_text(encoding='utf-8').splitlines():
        fields = line.split('\t')
        if len(fields) != 3:
            raise ValueError('Invalid BT4 roster catalogue')
        index = int(fields[0])
        if index in roster:
            raise ValueError('Duplicate BT4 roster identity')
        roster[index] = ' '.join(v for v in fields[1:] if v)
    table = unpack(read_resource(source, 4))[0][0]
    fighters = playable_fighters(table, roster) if fighters is None else sorted(set(fighters))
    if not fighters or any(not 0 <= i < 250 for i in fighters):
        raise ValueError('Choose BT4 donor identities from 0 through 249')
    families = costume_families(table, fighters)
    (output / 'models').mkdir(parents=True)
    (output / 'files').mkdir()
    index_rows = ['# BT4 native warrior sources v1\n']
    assets = {}
    for fighter in fighters:
        base, count, selectable = families[fighter]
        if not 1 <= count <= 16 or fighter not in roster:
            raise ValueError('Unsupported experimental donor costume family')
        metadata = unpack(read_resource(source, 4150 + fighter))[0]
        text = read_resource(source, 4400 + fighter)
        lips = [unpack(read_resource(source, n + fighter), terminal=False)
                for n in (4650, 4900)]
        prepared = []
        for damage in range(2):
            for costume in range(4):
                value = b''
                if costume < selectable:
                    raw = read_resource(source, base + costume + damage * count)
                    if struct.unpack_from('<I', raw)[0]:
                        value = embed(raw, metadata, text, *lips)
                # Empty slots retain a valid pack sentinel so the consumer's
                # existing first-intact fallback handles missing variants.
                if not value:
                    value = pack([b''])
                assets[1423 + fighter * 10 + costume + damage * 4] = value
                prepared.append(value)
        validate_model(prepared[0])
        (output / 'models' / f'{fighter}.pak').write_bytes(prepared[0])
        index_rows.append(f'{fighter}\t{roster[fighter]}\t{len(prepared[0])}\t{fingerprint(prepared[0]):016x}\n')
        for synthetic, actual in [(1431 + fighter * 10, 5150 + fighter),
                                  (1432 + fighter * 10, 5400 + fighter),
                                  (760 + fighter, 0x71C + fighter),
                                  (7 + fighter * 2, 0xD48 + fighter * 2),
                                  (8 + fighter * 2, 0xD49 + fighter * 2)]:
            assets[synthetic] = read_resource(source, actual)
        if progress:
            progress(fighter, roster[fighter])
    rows = ['# BT4 native warrior generation assets v1\n']
    for number, value in sorted(assets.items()):
        (output / 'files' / f'{number:05d}.bin').write_bytes(value)
        rows.append(f'{number}\t{len(value)}\t{fingerprint(value):016x}\n')
    # Publish eligibility markers last: partial preparation is never usable.
    (output / 'assets.tsv').write_text(''.join(rows), encoding='utf-8')
    (output / 'index.tsv').write_text(''.join(index_rows), encoding='utf-8')
    provenance = dict(profile='bt4-b14-rev2', fighters=fighters,
                      resources=len(assets), bt3_used=False, helper_used=False,
                      costume_limit=4, experimental=True)
    (output / 'provenance.json').write_text(json.dumps(provenance, indent=2))
    return provenance


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-data', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument('--fighter', type=int, action='append')
    selection.add_argument('--all', action='store_true', help='Prepare all named playable BT4 donor identities')
    args = parser.parse_args()
    try:
        print(prepare(args.source_data, args.output, args.fighter,
                      lambda i, name: print(f'Prepared {i}: {name}', flush=True)))
    except (OSError, ValueError, KeyError, struct.error) as error:
        parser.exit(1, str(error) + '\n')
