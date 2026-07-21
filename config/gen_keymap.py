#!/usr/bin/env python3
"""Generate keymap_it.json — Italian keyboard via bare-key Alt+numpad injection."""

import json

# Numpad HID keycodes for digits 0-9
NUMPAD = [0x62, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61]

# ============================================================
# Layer 1: Italian key positions → Alt+numpad injection
# Triggered by bare key press (no AltGr needed).
# mod_mask=0xFF means ALL modifier bits must match exactly:
#   trigger_mod=0x00 → no modifiers held
#   trigger_mod=0x02 → only Shift held
# ============================================================

# Bare-key triggers: (physical_keycode, alt_code, need_shift, label)
ITALIAN_KEYS = [
    # [ key (0x2F, UK position right of P): è / é
    (0x2F, 233, True,  "Shift+[ → é"),
    (0x2F, 232, False, "[ → è"),
    # ; key (0x33, UK position right of L): ò / Ç
    (0x33, 199, True,  "Shift+; → Ç"),
    (0x33, 242, False, "; → ò"),
    # ' key (0x34, UK position right of ;): à / °
    (0x34, 176, True,  "Shift+' → °"),
    (0x34, 224, False, "' → à"),
    # \ key (0x31, UK position left of Z): ù / §
    (0x31, 167, True,  "Shift+\\ → §"),
    (0x31, 249, False, "\\ → ù"),
    # = key (0x2D, UK position right of 0): ì / ^
    (0x2D, 94,  True,  "Shift+= → ^"),
    (0x2D, 236, False, "= → ì"),
]

# for bare-key triggers: mod_mask = 0xFF (all bits must match)
# trigger_mod: 0x00 = no modifiers, 0x02 = Shift held
BARE_MASK = 0xFF

# ============================================================
# Layer 2: Extended chars via AltGr trigger
# Characters not on standard Italian layout.
# ============================================================

ALTGR_CHARS = [
    (0x06, 199, True,  "AltGr+Shift+C → Ç"),
    (0x06, 231, False, "AltGr+C → ç"),
    (0x08, 202, True,  "AltGr+Shift+E → Ê"),
    (0x08, 234, False, "AltGr+E → ê"),
    (0x0A, 207, True,  "AltGr+Shift+I → Ï"),
    (0x0A, 239, False, "AltGr+I → ï"),
    (0x11, 209, True,  "AltGr+Shift+N → Ñ"),
    (0x11, 241, False, "AltGr+N → ñ"),
    (0x12, 212, True,  "AltGr+Shift+O → Ô"),
    (0x12, 244, False, "AltGr+O → ô"),
    (0x13, 203, True,  "AltGr+Shift+R → Ë"),
    (0x13, 235, False, "AltGr+R → ë"),
    (0x15, 219, True,  "AltGr+Shift+U → Û"),
    (0x15, 251, False, "AltGr+U → û"),
    (0x1A, 220, True,  "AltGr+Shift+Y → Ü"),
    (0x1A, 252, False, "AltGr+Y → ü"),
]


def build_alt_numpad_steps(code):
    """Build MacroStep list for an Alt+numpad sequence."""
    digits = [int(d) for d in str(code)]
    steps = []
    steps.append({"modifier": 4, "keys": [], "delay_us": 20000})
    for d in digits:
        steps.append({"modifier": 4, "keys": [NUMPAD[d]], "delay_us": 15000})
        steps.append({"modifier": 4, "keys": [], "delay_us": 10000})
    steps.append({"modifier": 0, "keys": [], "delay_us": 0})
    return steps


def make_bare_macro(keycode, alt_code, need_shift, comment):
    """Macro triggered by bare key press (no AltGr)."""
    trig_mod = 0x02 if need_shift else 0x00
    return {
        "comment": comment,
        "trigger": {
            "modifiers": trig_mod,
            "keycode": keycode,
            "mod_mask": BARE_MASK
        },
        "steps": build_alt_numpad_steps(alt_code)
    }


def make_altgr_macro(keycode, alt_code, need_shift, comment):
    """Macro triggered by AltGr + key press."""
    trig_mod = 0x42 if need_shift else 0x40
    mask = 0x42 if need_shift else 0x40
    return {
        "comment": comment,
        "trigger": {
            "modifiers": trig_mod,
            "keycode": keycode,
            "mod_mask": mask
        },
        "steps": build_alt_numpad_steps(alt_code)
    }


def main():
    # keycode_map: simple swaps
    keycode_map = {}
    keycode_map["28"] = "29"   # Y → Z
    keycode_map["29"] = "28"   # Z → Y

    config = {
        "language": "Italian",
        "description": "Italian keyboard via bare-key Alt+numpad injection. "
                       "Press [ for è, ; for ò, ' for à, \\ for ù, = for ì. "
                       "Shift variants produce é, Ç, °, §, ^. "
                       "Extended chars via AltGr: ç, ê, ï, ñ, ô, ë, û, ü. "
                       "Works on Windows without host layout change.",
        "host_os": "windows",
        "keycode_map": keycode_map,
        "macros": []
    }

    # Layer 1: bare-key Italian position macros
    for keycode, alt_code, need_shift, comment in ITALIAN_KEYS:
        config["macros"].append(
            make_bare_macro(keycode, alt_code, need_shift, comment))

    # Layer 2: AltGr extended macros
    for keycode, alt_code, need_shift, comment in ALTGR_CHARS:
        config["macros"].append(
            make_altgr_macro(keycode, alt_code, need_shift, comment))

    with open("config/keymap_it.json", "w") as f:
        json.dump(config, f, indent=2, ensure_ascii=False)
        f.write("\n")

    print(f"Generated config/keymap_it.json with {len(config['macros'])} macros")


if __name__ == "__main__":
    main()
