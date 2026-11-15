#!/usr/bin/env python3
"""Build the single-file amalgamation of the SpeedSpeech core.

Writes speedspeech.h + speedspeech.c (one translation unit, valid C99 and
C++) into every engine binding that compiles the core from source:

  bindings/unity/com.speedspeech.unity/Plugins/Source/   (iOS, WebGL via IL2CPP)
  bindings/unreal/SpeedSpeech/Source/ThirdParty/SpeedSpeechCore/

Usage:
  python3 tools/amalgamate.py           # regenerate
  python3 tools/amalgamate.py --check   # fail if the committed copies are stale
  python3 tools/amalgamate.py --out DIR # write to DIR only
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
CORE = ROOT / "core"
SOURCES = ["ss_phonemes.c", "ss_lexicon.c", "ss_rules.c", "ss_text.c",
           "ss_prosody.c", "ss_klatt.c", "ss_engine.c"]
TARGETS = [
    ROOT / "bindings/unity/com.speedspeech.unity/Plugins/Source",
    ROOT / "bindings/unreal/SpeedSpeech/Source/ThirdParty/SpeedSpeechCore",
]
LOCAL_INCLUDE = re.compile(r'^\s*#\s*include\s+"(speedspeech\.h|ss_internal\.h)"\s*$')
BANNER = ("/* GENERATED FILE - do not edit. Produced by tools/amalgamate.py from core/.\n"
          " * SpeedSpeech (MIT License) - https://github.com/brandon2500/SpeedSpeech */\n")


def strip_local_includes(text: str) -> str:
    return "\n".join(l for l in text.splitlines() if not LOCAL_INCLUDE.match(l)) + "\n"


def build() -> dict:
    header = (CORE / "include" / "speedspeech.h").read_text()
    parts = [BANNER, '#include "speedspeech.h"\n',
             "\n/* ---- ss_internal.h ---- */\n",
             strip_local_includes((CORE / "src" / "ss_internal.h").read_text())]
    for name in SOURCES:
        parts.append(f"\n/* ---- {name} ---- */\n")
        parts.append(strip_local_includes((CORE / "src" / name).read_text()))
    return {"speedspeech.h": BANNER + header, "speedspeech.c": "".join(parts)}


def main() -> int:
    files = build()
    targets = TARGETS
    if "--out" in sys.argv:
        targets = [pathlib.Path(sys.argv[sys.argv.index("--out") + 1])]
    stale = []
    for t in targets:
        for name, content in files.items():
            path = t / name
            if "--check" in sys.argv:
                if not path.exists() or path.read_text() != content:
                    stale.append(str(path.relative_to(ROOT)))
            else:
                t.mkdir(parents=True, exist_ok=True)
                path.write_text(content)
    if stale:
        print("stale amalgamation, run tools/amalgamate.py:\n  " + "\n  ".join(stale), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
