"""Release metadata must remain readable by the 0.4 ANSI-conversion updater.

This changes display notes only, never tags, installers or checksums.
"""
import json
import sys
import unicodedata
from pathlib import Path
def ascii_text(value):
    for source,destination in {'Ä':'Ae','Ö':'Oe','Ü':'Ue','ä':'ae','ö':'oe','ü':'ue','ß':'ss','—':'-','–':'-','…':'...'}.items():
        value=value.replace(source,destination)
    return unicodedata.normalize('NFKD',value).encode('ascii','ignore').decode('ascii')
if len(sys.argv)>1:
    Path(sys.argv[2]).write_text(ascii_text(Path(sys.argv[1]).read_text(encoding='utf-8')),encoding='ascii')
else:
    release=json.load(sys.stdin)
    print(json.dumps({'name':ascii_text(release.get('name') or release['tag_name']),'body':ascii_text(release.get('body') or '')}))
