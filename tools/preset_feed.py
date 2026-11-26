"""Build an authenticated static preset feed from a private R48-style catalog.
Run on the publishing workstation, never in the customer plugin. Private keys
and private catalog data must stay outside the public web root and Git.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ID = re.compile(r'[a-z0-9][a-z0-9_-]{0,79}\Z')
SHA = re.compile(r'[0-9a-f]{64}\Z')


def identifier(s):
    if not isinstance(s, str) or not ID.fullmatch(s):
        raise ValueError('invalid identifier')
    return s


def label(s, limit):
    if not isinstance(s, str) or not s.strip() or len(s.encode()) > limit or any(ord(c) < 32 or ord(c) == 127 for c in s):
        raise ValueError('invalid label')
    return s


def checked_object(path, sha, limit):
    if not isinstance(sha, str) or not SHA.fullmatch(sha):
        raise ValueError('invalid object hash')
    if not path.is_file() or path.is_symlink() or not 0 < path.stat().st_size <= limit:
        raise ValueError('invalid object size')
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != sha:
        raise ValueError('object checksum mismatch')
    return data


def export(catalog, output, key, product, channel, generation, abi, validate_package, validate_cover):
    """Build into a new directory. Upload objects first, then signed catalog.
    Validators are mandatory and supplied by the product's native release tooling.
    """
    catalog, output, key = map(Path, (catalog, output, key))
    identifier(product)
    if channel not in ('release', 'trial') or type(generation) is not int or not 0 < generation <= 9007199254740991 or type(abi) is not int or not 0 < abi <= 1000000:
        raise ValueError('invalid feed identity')
    if output.exists() or output.resolve() in key.resolve().parents or catalog.resolve() in output.resolve().parents:
        raise ValueError('use a new output directory outside private catalog')
    c = json.loads((catalog / 'catalog.json').read_text(encoding='utf-8'))
    if c.get('schema') != 1:
        raise ValueError('unsupported catalog')
    rows = [f'SKELLFEED1\t{product}\t{channel}\t{generation}']
    objects = {}
    for pid, e in sorted(c['presets'].items()):
        identifier(pid)
        rev = next((r for r in e['revisions'] if r['sha256'] == e.get('latest')), None)
        if channel == 'release' and (rev is None or rev['status'] != 'approved'):
            # A newly staged trial must not remove the last approved revision.
            rev = next((r for r in reversed(e['revisions']) if r['status'] == 'approved'), None)
        if rev is None:
            continue
        if rev['status'] not in ('trial', 'approved'):
            raise ValueError('unknown revision status')
        aid = identifier(e['artist_id']); artist = c['artists'][aid]
        name = label(artist['name'], 240); title = label(e['name'], 288)
        category = e['category']
        if category not in ('main', 'adlib'):
            raise ValueError('invalid category')
        sha = rev['sha256']
        if not SHA.fullmatch(sha):
            raise ValueError('invalid revision hash')
        p = catalog / 'revisions' / pid / (sha + '.skellpreset')
        data = checked_object(p, sha, 4*1024*1024)
        if not validate_package(p):
            raise ValueError('invalid native package')
        objects[sha + '.skellpreset'] = data
        cover = artist.get('cover_sha', '-'); cover_size = 0
        if cover != '-':
            if not SHA.fullmatch(cover):
                raise ValueError('invalid cover hash')
            image = checked_object(catalog / 'artwork' / (cover + '.png'), cover, 8*1024*1024)
            if not validate_cover(image):
                raise ValueError('invalid cover')
            objects[cover + '.png'] = image; cover_size = len(image)
        rows.append('\t'.join(map(str, (pid, aid, name, title, category, abi, sha, len(data), cover, cover_size))))
    raw = ('\n'.join(rows) + '\n').encode()
    if len(rows) == 1 or len(rows) > 1001 or len(raw) > 1024*1024 or sum(map(len, objects.values())) > 128*1024*1024:
        raise ValueError('empty or oversized feed')
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.skell-feed-', dir=output.parent) as tmp:
        root = Path(tmp) / 'snapshot'; root.mkdir(); (root / 'objects').mkdir()
        for filename, data in objects.items():
            (root / 'objects' / filename).write_bytes(data)
        (root / 'manifest.tsv').write_bytes(raw)
        # Standard RSA SHA-256 PKCS#1 v1.5 signature. Pin the corresponding
        # public key in the native verifier; never trust a key from the feed.
        subprocess.run(['openssl', 'rsa', '-in', str(key), '-check', '-noout'], check=True, stdout=subprocess.DEVNULL)
        subprocess.run(['openssl', 'dgst', '-sha256', '-sign', str(key), '-out', str(root / 'manifest.sig'), str(root / 'manifest.tsv')], check=True)
        signature = root / 'manifest.sig'
        if not 0 < signature.stat().st_size <= 4096:
            raise ValueError('invalid signature size')
        os.rename(root, output)
    return len(rows)-1


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('catalog', type=Path); p.add_argument('output', type=Path)
    p.add_argument('--key', type=Path, required=True); p.add_argument('--channel', choices=['release','trial'], required=True)
    p.add_argument('--generation', type=int, required=True); p.add_argument('--abi', type=int, required=True)
    p.add_argument('--product', default='skell-preset')
    a = p.parse_args()
    # Existing Sound Match schema-v2 validator. Approval remains a separate
    # explicit native release decision, not a listening vote.
    from skell_sound_match.wav_only.preset_sync import package, valid_png
    print(export(a.catalog, a.output, a.key, a.product, a.channel, a.generation, a.abi, package, valid_png))

if __name__ == '__main__':
    main()
