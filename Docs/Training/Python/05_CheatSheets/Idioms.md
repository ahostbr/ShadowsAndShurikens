# Idioms Buddy must copy (don’t invent)

## Paths
from pathlib import Path
p = Path(arg).expanduser().resolve()

## Atomic JSON write
from io_atomic import write_json_atomic
write_json_atomic(out_path, obj)

## Subprocess
from subprocess_run import run_or_raise
run_or_raise(["python", "--version"])

## Enumerate files with excludes
from walk_glob import iter_files
for f in iter_files(root, pattern="*.cpp"): ...
