#!/usr/bin/env python3
"""Generate keyboard layout configs for the pi500kb project.

Usage:
  python3 config/gen_keymaps.py --all                  # all layouts
  python3 config/gen_keymaps.py --layout it            # Italian bare-key
  python3 config/gen_keymaps.py --layout it --altgr    # Italian via AltGr
  python3 config/gen_keymaps.py --layout de --altgr    # German via AltGr
"""

import argparse
import json
import sys

# Numpad HID keycodes for digits 0-9
NUMPAD = [0x62, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61]

BARE_MASK = 255       # all modifier bits must match (for bare-key triggers)
ALTGR_MASK = 0x40     # check only RightAlt bit
ALTGR_SHIFT = 0x42    # check RightAlt + Shift


def alt_steps(code):
    """Build Alt+numpad MacroStep list."""
    digits = [int(d) for d in str(code)]
    steps = [{"modifier": 4, "keys": [], "delay_us": 20000}]
    for d in digits:
        steps.append({"modifier": 4, "keys": [NUMPAD[d]], "delay_us": 15000})
        steps.append({"modifier": 4, "keys": [], "delay_us": 10000})
    steps.append({"modifier": 0, "keys": [], "delay_us": 0})
    return steps


def bare(keycode, code, shift=False, note=""):
    """Macro triggered by bare key press (no AltGr)."""
    t = 0x02 if shift else 0x00
    return {
        "comment": note,
        "trigger": {"modifiers": t, "keycode": keycode, "mod_mask": BARE_MASK},
        "steps": alt_steps(code),
    }


def altgr(keycode, code, shift=False, note=""):
    """Macro triggered by AltGr + key press."""
    t = ALTGR_SHIFT if shift else ALTGR_MASK
    m = ALTGR_SHIFT if shift else ALTGR_MASK
    return {
        "comment": note,
        "trigger": {"modifiers": t, "keycode": keycode, "mod_mask": m},
        "steps": alt_steps(code),
    }


def make(swaps, macros, lang, desc, host_os="windows"):
    return {
        "language": lang,
        "description": desc,
        "host_os": host_os,
        "keycode_map": {str(k): str(v) for k, v in swaps.items()},
        "macros": macros,
    }


def save(data, name):
    path = f"config/keymap_{name}.json"
    with open(path, "w") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")
    print(f"  {path} — {len(data['macros'])} macros")


# ============================================================
# Language definitions
# ============================================================

def lang_it():
    """Italian — bare-key injection at Italian physical positions."""
    return make(
        swaps={0x1C: 0x1D, 0x1D: 0x1C},  # Y↔Z
        macros=[
            bare(0x2F, 232, note="[ → è"),
            bare(0x2F, 233, shift=True, note="Shift+[ → é"),
            bare(0x33, 242, note="; → ò"),
            bare(0x33, 199, shift=True, note="Shift+; → Ç"),
            bare(0x34, 224, note="' → à"),
            bare(0x34, 176, shift=True, note="Shift+' → °"),
            bare(0x31, 249, note="\\ → ù"),
            bare(0x31, 167, shift=True, note="Shift+\\ → §"),
            bare(0x2D, 236, note="= → ì"),
            bare(0x2D, 94, shift=True, note="Shift+= → ^"),
            altgr(0x06, 231, note="AltGr+C → ç"),
            altgr(0x06, 199, shift=True, note="AltGr+Shift+C → Ç"),
            altgr(0x08, 234, note="AltGr+E → ê"),
            altgr(0x0A, 239, note="AltGr+I → ï"),
            altgr(0x11, 241, note="AltGr+N → ñ"),
            altgr(0x12, 244, note="AltGr+O → ô"),
            altgr(0x13, 235, note="AltGr+R → ë"),
            altgr(0x15, 251, note="AltGr+U → û"),
            altgr(0x1A, 252, note="AltGr+Y → ü"),
        ],
        lang="Italian",
        desc="Italian via bare-key: [=è ;=ò '=à \\=ù ==ì. Shift=é/Ç/°/§/^. Extended via AltGr: çêïñôëûü.",
    )


def lang_it_altgr():
    """Italian — AltGr+letter compose."""
    return make(
        swaps={},
        macros=[
            altgr(0x04, 224, note="AltGr+A → à"),
            altgr(0x04, 192, shift=True, note="AltGr+Shift+A → À"),
            altgr(0x08, 232, note="AltGr+E → è"),
            altgr(0x08, 200, shift=True, note="AltGr+Shift+E → È"),
            altgr(0x08, 233, shift=True, note="AltGr+Shift+E → é (conflict!)"),
            altgr(0x0A, 236, note="AltGr+I → ì"),
            altgr(0x12, 242, note="AltGr+O → ò"),
            altgr(0x15, 249, note="AltGr+U → ù"),
            altgr(0x06, 231, note="AltGr+C → ç"),
            altgr(0x11, 241, note="AltGr+N → ñ"),
        ],
        lang="Italian (AltGr)",
        desc="Italian via AltGr+letter: AltGr+A=à E=è I=ì O=ò U=ù C=ç N=ñ. Note: è/é conflict on E key.",
    )


def lang_de():
    """German QWERTZ — bare-key injection."""
    return make(
        swaps={0x1C: 0x1D, 0x1D: 0x1C},  # Y↔Z
        macros=[
            bare(0x34, 228, note="' → ä"),
            bare(0x34, 196, shift=True, note="Shift+' → Ä"),
            bare(0x33, 246, note="; → ö"),
            bare(0x33, 214, shift=True, note="Shift+; → Ö"),
            bare(0x2F, 252, note="[ → ü"),
            bare(0x2F, 220, shift=True, note="Shift+[ → Ü"),
            bare(0x2C, 223, note="- → ß"),
            bare(0x2C, 63, shift=True, note="Shift+- → ?"),
            altgr(0x08, 128, note="AltGr+E → €"),
        ],
        lang="German (QWERTZ)",
        desc="Deutsch via bare-key: Y↔Z swap. '=ä ;=ö [=ü -=ß. Shift=uppercase. AltGr+E=€.",
    )


def lang_de_altgr():
    """German — AltGr+letter compose."""
    return make(
        swaps={0x1C: 0x1D, 0x1D: 0x1C},  # Y↔Z
        macros=[
            altgr(0x04, 228, note="AltGr+A → ä"),
            altgr(0x04, 196, shift=True, note="AltGr+Shift+A → Ä"),
            altgr(0x12, 246, note="AltGr+O → ö"),
            altgr(0x12, 214, shift=True, note="AltGr+Shift+O → Ö"),
            altgr(0x15, 252, note="AltGr+U → ü"),
            altgr(0x15, 220, shift=True, note="AltGr+Shift+U → Ü"),
            altgr(0x16, 223, note="AltGr+S → ß"),
            altgr(0x08, 128, note="AltGr+E → €"),
        ],
        lang="German (AltGr)",
        desc="Deutsch via AltGr+letter: Y↔Z swap. AltGr+A=ä O=ö U=ü S=ß. Shift=uppercase. AltGr+E=€.",
    )


def lang_es():
    """Spanish — bare-key injection."""
    return make(
        swaps={},
        macros=[
            bare(0x33, 241, note="; → ñ"),
            bare(0x33, 209, shift=True, note="Shift+; → Ñ"),
            bare(0x34, 225, note="' → á"),
            bare(0x34, 193, shift=True, note="Shift+' → Á"),
            bare(0x2D, 237, note="= → í"),
            bare(0x2D, 205, shift=True, note="Shift+= → Í"),
            # é and ó via AltGr to avoid overwriting E and O
            altgr(0x08, 233, note="AltGr+E → é"),
            altgr(0x12, 243, note="AltGr+O → ó"),
            altgr(0x38, 191, note="AltGr+/ → ¿"),
            altgr(0x38, 161, shift=True, note="AltGr+Shift+/ → ¡"),
        ],
        lang="Spanish",
        desc="Español via bare-key: ;=ñ '=á ==í. é/ó via AltGr+E/O. ¿/¡ via AltGr+/.",
    )


def lang_es_altgr():
    """Spanish — AltGr+letter compose."""
    return make(
        swaps={},
        macros=[
            altgr(0x11, 241, note="AltGr+N → ñ"),
            altgr(0x11, 209, shift=True, note="AltGr+Shift+N → Ñ"),
            altgr(0x04, 225, note="AltGr+A → á"),
            altgr(0x08, 233, note="AltGr+E → é"),
            altgr(0x0C, 237, note="AltGr+I → í"),
            altgr(0x12, 243, note="AltGr+O → ó"),
            altgr(0x15, 250, note="AltGr+U → ú"),
            altgr(0x38, 191, note="AltGr+/ → ¿"),
        ],
        lang="Spanish (AltGr)",
        desc="Español via AltGr+letter: AltGr+N=ñ A=á E=é I=í O=ó U=ú ¿ via AltGr+/.",
    )


def lang_fr():
    """French AZERTY — bare-key injection."""
    return make(
        swaps={0x04: 0x14, 0x14: 0x04},  # A↔Q
        macros=[
            bare(0x1F, 233, note="2 → é"),
            bare(0x24, 232, note="7 → è"),
            bare(0x26, 231, note="9 → ç"),
            bare(0x27, 224, note="0 → à"),
            bare(0x2C, 249, note="- → ù"),
            altgr(0x08, 234, note="AltGr+E → ê"),
            altgr(0x08, 235, shift=True, note="AltGr+Shift+E → ë"),
            altgr(0x0C, 238, note="AltGr+I → î"),
            altgr(0x12, 244, note="AltGr+O → ô"),
            altgr(0x15, 251, note="AltGr+U → û"),
        ],
        lang="French (AZERTY)",
        desc="Français via bare-key: A↔Q swap. 2=é 7=è 9=ç 0=à -=ù. Extended via AltGr: êëîôû.",
    )


def lang_fr_altgr():
    """French — AltGr+letter compose."""
    return make(
        swaps={},
        macros=[
            altgr(0x08, 233, note="AltGr+E → é"),
            altgr(0x08, 232, shift=True, note="AltGr+Shift+E → è (conflict!)"),
            altgr(0x04, 224, note="AltGr+A → à"),
            altgr(0x06, 231, note="AltGr+C → ç"),
            altgr(0x15, 249, note="AltGr+U → ù"),
            altgr(0x08, 234, shift=True, note="AltGr+Shift+E → ê"),
            altgr(0x0C, 238, note="AltGr+I → î"),
            altgr(0x12, 244, note="AltGr+O → ô"),
        ],
        lang="French (AltGr)",
        desc="Français via AltGr+letter: AltGr+E=é A=à C=ç U=ù. Extended: î, ô, ê. Note: é/è conflict on E.",
    )


def lang_uk():
    """UK — raw forwarding (no remapping needed)."""
    return make(
        swaps={},
        macros=[],
        lang="UK",
        desc="UK keyboard — raw forwarding. No macros, no swaps. Host layout handles everything.",
    )


# ============================================================
# Registry
# ============================================================

LAYOUTS = {
    "it":      ("Italian bare-key",      lang_it),
    "it_altgr": ("Italian AltGr",        lang_it_altgr),
    "de":      ("German QWERTZ bare-key", lang_de),
    "de_altgr": ("German AltGr",          lang_de_altgr),
    "es":      ("Spanish bare-key",      lang_es),
    "es_altgr": ("Spanish AltGr",        lang_es_altgr),
    "fr":      ("French AZERTY bare-key", lang_fr),
    "fr_altgr": ("French AltGr",          lang_fr_altgr),
    "uk":      ("UK raw",                lang_uk),
}


def main():
    p = argparse.ArgumentParser(description="Generate pi500kb keymap configs")
    p.add_argument("--layout", help="Language code (it, de, es, fr, uk)")
    p.add_argument("--altgr", action="store_true", help="Use AltGr+letter approach")
    p.add_argument("--all", action="store_true", help="Generate all layouts")
    args = p.parse_args()

    if args.all:
        names = list(LAYOUTS.keys())
    elif args.layout:
        suffix = "_altgr" if args.altgr else ""
        names = [args.layout + suffix]
    else:
        p.print_help()
        sys.exit(1)

    for name in names:
        if name not in LAYOUTS:
            print(f"  Unknown layout: {name}")
            continue
        label, fn = LAYOUTS[name]
        data = fn()
        save(data, name)
        print(f"    → {label}")

    print("\nDone.")


if __name__ == "__main__":
    main()
