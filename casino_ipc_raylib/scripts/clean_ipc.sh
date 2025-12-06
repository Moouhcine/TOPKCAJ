#!/usr/bin/env bash
set -euo pipefail

# Uses libc shm_unlink/mq_unlink via Python+ctypes to avoid extra deps.
python3 - <<'PY'
import ctypes
import sys
libc = ctypes.CDLL('libc.so.6')
for name, func in {'shm_unlink': libc.shm_unlink, 'mq_unlink': libc.mq_unlink}.items():
    for target in (b'/casino_ipc_shared', b'/casino_ipc_mq'):
        res = func(target)
        if res != 0:
            # errno set; ignore if not present
            continue
        else:
            sys.stdout.write(f"[{name}] cleaned {target.decode()}\n")
PY
