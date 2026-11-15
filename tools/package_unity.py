#!/usr/bin/env python3
"""Assemble the Unity package: native plugins + .meta files.

Unity needs a .meta file next to every asset of a package installed from git
(those folders are read-only), and native plugins need per-platform import
settings. This script writes both. Existing GUIDs are kept.

  python3 tools/package_unity.py                    # metas only
  python3 tools/package_unity.py --host build       # + the plugin built for this machine
  python3 tools/package_unity.py --binaries DIR     # + plugins from CI (see layout below)

--binaries layout (what .github/workflows/ci.yml produces):
  DIR/Windows/x86_64/speedspeech.dll
  DIR/Linux/x86_64/libspeedspeech.so
  DIR/macOS/speedspeech.dylib                 (universal arm64 + x86_64)
  DIR/Android/arm64-v8a/libspeedspeech.so     (also armeabi-v7a, x86_64)
iOS and WebGL compile Plugins/Source/speedspeech.c directly (IL2CPP).
"""
import hashlib
import pathlib
import platform
import shutil
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
PKG = ROOT / "bindings" / "unity" / "com.speedspeech.unity"
PLUGINS = PKG / "Plugins"


def guid_for(path: pathlib.Path) -> str:
    return hashlib.md5(("speedspeech:" + path.relative_to(PKG).as_posix()).encode()).hexdigest()


def existing_guid(meta: pathlib.Path):
    if meta.exists():
        for line in meta.read_text().splitlines():
            if line.startswith("guid:"):
                return line.split(":", 1)[1].strip()
    return None


def platform_entry(first: str, enabled: bool, settings: dict) -> str:
    out = [f"  - first:\n      {first}\n    second:\n      enabled: {1 if enabled else 0}\n      settings:"]
    if settings:
        out += [f"        {k}: {v}" for k, v in settings.items()]
    else:
        out[0] += " {}"
    return "\n".join(out)


def plugin_importer(entries) -> str:
    body = "\n".join(entries)
    return ("PluginImporter:\n  externalObjects: {}\n  serializedVersion: 2\n  iconMap: {}\n"
            "  executionOrder: {}\n  defineConstraints: []\n  isPreloaded: 0\n  isOverridable: 1\n"
            "  isExplicitlyReferenced: 0\n  validateReferences: 1\n  platformData:\n"
            f"{body}\n  userData: \n  assetBundleName: \n  assetBundleVariant: \n")


def native_meta(rel: str) -> str:
    """Import settings for a native plugin, keyed on its location."""
    any_off = platform_entry("Any: ", False, {})
    editor = lambda os_, cpu: platform_entry("Editor: Editor", True, {"CPU": cpu, "DefaultValueInitialized": "true", "OS": os_})
    standalone = lambda target, on, cpu: platform_entry(f"Standalone: {target}", on, {"CPU": cpu if on else "None"})
    if rel.startswith("Windows/"):
        return plugin_importer([any_off, editor("Windows", "x86_64"), standalone("Win64", True, "x86_64"),
                                standalone("Linux64", False, ""), standalone("OSXUniversal", False, "")])
    if rel.startswith("Linux/"):
        return plugin_importer([any_off, editor("Linux", "x86_64"), standalone("Linux64", True, "x86_64"),
                                standalone("Win64", False, ""), standalone("OSXUniversal", False, "")])
    if rel.startswith("macOS/"):
        return plugin_importer([any_off, editor("OSX", "AnyCPU"), standalone("OSXUniversal", True, "AnyCPU"),
                                standalone("Win64", False, ""), standalone("Linux64", False, "")])
    if rel.startswith("Android/"):
        abi = rel.split("/")[1]
        cpu = {"arm64-v8a": "ARM64", "armeabi-v7a": "ARMv7", "x86_64": "X86_64"}.get(abi, "ARM64")
        return plugin_importer([any_off, platform_entry("Android: Android", True, {"CPU": cpu}),
                                platform_entry("Editor: Editor", False, {})])
    if rel.startswith("Source/"):
        return plugin_importer([any_off,
                                platform_entry("iPhone: iOS", True, {"AddToEmbeddedBinaries": "false", "CPU": "AnyCPU"}),
                                platform_entry("tvOS: tvOS", True, {"CPU": "AnyCPU"}),
                                platform_entry("WebGL: WebGL", True, {}),
                                platform_entry("Editor: Editor", False, {})])
    raise ValueError(rel)


def meta_for(path: pathlib.Path) -> str:
    guid = existing_guid(path.with_name(path.name + ".meta")) or guid_for(path)
    head = f"fileFormatVersion: 2\nguid: {guid}\n"
    if path.is_dir():
        return head + "folderAsset: yes\nDefaultImporter:\n  externalObjects: {}\n  userData: \n  assetBundleName: \n  assetBundleVariant: \n"
    suffix = path.suffix.lower()
    if suffix == ".cs":
        return head + ("MonoImporter:\n  externalObjects: {}\n  serializedVersion: 2\n  defaultReferences: []\n"
                       "  executionOrder: 0\n  icon: {instanceID: 0}\n  userData: \n  assetBundleName: \n  assetBundleVariant: \n")
    if suffix == ".asmdef":
        return head + "AssemblyDefinitionImporter:\n  externalObjects: {}\n  userData: \n  assetBundleName: \n  assetBundleVariant: \n"
    if path.name == "package.json":
        return head + "PackageManifestImporter:\n  externalObjects: {}\n  userData: \n  assetBundleName: \n  assetBundleVariant: \n"
    if PLUGINS in path.parents:
        return head + native_meta(path.relative_to(PLUGINS).as_posix())
    return head + "TextScriptImporter:\n  externalObjects: {}\n  userData: \n  assetBundleName: \n  assetBundleVariant: \n"


def write_metas() -> int:
    count = 0
    for path in sorted(PKG.rglob("*")):
        rel = path.relative_to(PKG)
        if path.suffix == ".meta" or any(p.endswith("~") or p.startswith(".") for p in rel.parts):
            continue
        meta = path.with_name(path.name + ".meta")
        content = meta_for(path)
        if not meta.exists() or meta.read_text() != content:
            meta.write_text(content)
            count += 1
    # drop metas whose asset is gone
    for meta in PKG.rglob("*.meta"):
        if not meta.with_name(meta.name[:-5]).exists():
            meta.unlink()
    return count


def copy_host(build_dir: pathlib.Path) -> None:
    system = platform.system()
    if system == "Windows":
        candidates = [build_dir / "Release" / "speedspeech.dll", build_dir / "speedspeech.dll"]
        dest = PLUGINS / "Windows" / "x86_64" / "speedspeech.dll"
    elif system == "Darwin":
        candidates = [build_dir / "speedspeech.dylib"]
        dest = PLUGINS / "macOS" / "speedspeech.dylib"
    else:
        candidates = [build_dir / "libspeedspeech.so"]
        dest = PLUGINS / "Linux" / "x86_64" / "libspeedspeech.so"
    src = next((c for c in candidates if c.exists()), None)
    if src is None:
        sys.exit(f"no native library found in {build_dir}; build with CMake first")
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dest)
    print(f"copied {src} -> {dest.relative_to(ROOT)}")


def copy_tree(binaries: pathlib.Path) -> None:
    for f in binaries.rglob("*"):
        if f.is_file() and f.suffix in (".dll", ".so", ".dylib"):
            dest = PLUGINS / f.relative_to(binaries)
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(f, dest)
            print(f"copied {dest.relative_to(ROOT)}")


def main() -> int:
    if "--host" in sys.argv:
        copy_host(pathlib.Path(sys.argv[sys.argv.index("--host") + 1]).resolve())
    if "--binaries" in sys.argv:
        copy_tree(pathlib.Path(sys.argv[sys.argv.index("--binaries") + 1]).resolve())
    n = write_metas()
    print(f"{n} meta files written")
    return 0


if __name__ == "__main__":
    sys.exit(main())
