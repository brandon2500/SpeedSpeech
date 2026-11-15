#!/usr/bin/env python3
"""Sort the entries of core/src/ss_lexicon.c (strcmp order) and drop duplicates.

Usage: python3 tools/sort_lexicon.py [--check]
"""
import pathlib
import re
import sys

PATH = pathlib.Path(__file__).resolve().parent.parent / "core" / "src" / "ss_lexicon.c"
BEGIN, END = "/* BEGIN LEXICON */", "/* END LEXICON */"
ENTRY = re.compile(r'\s*\{"([^"]+)",\s*"([^"]+)"\},\s*$')


def main() -> int:
    src = PATH.read_text()
    head, rest = src.split(BEGIN, 1)
    body, tail = rest.split(END, 1)
    entries = {}
    for line in body.splitlines():
        if not line.strip():
            continue
        m = ENTRY.match(line)
        if not m:
            print(f"unparseable lexicon line: {line!r}", file=sys.stderr)
            return 1
        word, pron = m.groups()
        if word in entries and entries[word] != pron:
            print(f"conflicting entries for {word!r}", file=sys.stderr)
            return 1
        entries[word] = pron
    ordered = sorted(entries.items(), key=lambda kv: kv[0].encode())
    new_body = "\n" + "".join(f'    {{"{w}", "{p}"}},\n' for w, p in ordered) + "    "
    new_src = head + BEGIN + new_body + END + tail
    if "--check" in sys.argv:
        if new_src != src:
            print("ss_lexicon.c is not sorted; run tools/sort_lexicon.py", file=sys.stderr)
            return 1
        return 0
    PATH.write_text(new_src)
    print(f"{len(ordered)} entries")
    return 0


if __name__ == "__main__":
    sys.exit(main())
