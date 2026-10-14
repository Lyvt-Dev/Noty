#!/usr/bin/env bash
set -euo pipefail
root="$RUNNER_TEMP/Noty update test"
mkdir -p "$root/profile"
printf 'keep-user-data\n' > "$root/profile/marker"
if [[ "$RUNNER_OS" == "macOS" ]]; then
  asset=Noty-macos-arm64.dmg
else
  asset=Noty-linux-x64.deb
fi
curl --fail --silent --show-error 'https://api.github.com/repos/Lyvt-Dev/Noty/releases/tags/v0.5.0' > "$root/release.json"
python3 - "$root/release.json" "$asset" "$root/asset.txt" <<'PY'
import json,sys
r=json.load(open(sys.argv[1]));a=next(a for a in r['assets'] if a['name']==sys.argv[2])
assert a['browser_download_url']=='https://github.com/Lyvt-Dev/Noty/releases/download/v0.5.0/'+sys.argv[2]
assert a['digest'].startswith('sha256:')
open(sys.argv[3],'w').write(a['browser_download_url']+'\n'+a['digest'][7:]+'\n')
PY
url=$(sed -n '1p' "$root/asset.txt")
digest=$(sed -n '2p' "$root/asset.txt")
curl --fail --location --silent --show-error "$url" -o "$root/legacy"
python3 - "$root/legacy" "$digest" <<'PY'
import hashlib,sys
assert hashlib.sha256(open(sys.argv[1],'rb').read()).hexdigest()==sys.argv[2], 'Legacy checksum mismatch'
PY
cp "build/$asset" "$root/current-package"
cp build/NotyUpdate "$root/NotyUpdate"
chmod +x "$root/NotyUpdate"
digest=$(python3 - "$root/current-package" <<'PY'
import hashlib,sys
print(hashlib.sha256(open(sys.argv[1],'rb').read()).hexdigest())
PY
)
# Parent PID has exited, exactly as it will have after the user's app closes.
sleep 0.01 & parent=$!
wait "$parent"
if [[ "$RUNNER_OS" == "macOS" ]]; then
  mkdir -p "$root/old-mount"
  hdiutil attach "$root/legacy" -nobrowse -readonly -mountpoint "$root/old-mount"
  ditto "$root/old-mount/Noty.app" "$root/Noty.app"
  hdiutil detach "$root/old-mount"
  installed="$root/Noty.app/Contents/MacOS/Noty"
  "$root/NotyUpdate" "$parent" "$root/current-package" "$digest" "$installed" close
  cmp "$installed" build/Noty.app/Contents/MacOS/Noty
  # Exercise the installed Cocoa application, including its fullscreen control.
  "$installed" --smoke-test
else
  sudo dpkg -i "$root/legacy"
  sudo "$root/NotyUpdate" "$parent" "$root/current-package" "$digest" /usr/bin/Noty close
  cmp /usr/bin/Noty build/Noty
  sudo chmod a+r /usr/bin/Noty
  # Portable test buffers live in a disposable copy, not in /usr/bin.
  mkdir -p "$root/smoke"
  cp /usr/bin/Noty "$root/smoke/Noty"
  xvfb-run -a "$root/smoke/Noty" --smoke-test
fi
[[ "$(cat "$root/profile/marker")" == keep-user-data ]]
[[ -f "build/$asset" ]]
cat "$root/update-result.txt"
printf 'PASS: 0.5 -> current native helper update; %s installation verified\n' "$RUNNER_OS"
