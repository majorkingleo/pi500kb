#!/usr/bin/env python3
"""Generate keymap_it.json — Italian keyboard layout config."""

import json

# Numpad HID keycodes for digits 0-9
NUMPAD = [0x62, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61]

# Italian accented chars: (trigger_keycode, alt_code, comment)
ITALIAN_CHARS = [
    (0x04, 224, "A + AltGr → à"),
    (0x04, 192, "AltGr+Shift+A → À"),
    (0x08, 232, "E + AltGr → è"),
    (0x08, 200, "AltGr+Shift+E → È"),
    (0x08, 233, "E + AltGr → é"),
    (0x08, 201, "AltGr+Shift+E → É"),
    (0x0A, 236, "I + AltGr → ì"),
    (0x0A, 204, "AltGr+Shift+I → Ì"),
    (0x12, 242, "O + AltGr → ò"),
    (0x12, 210, "AltGr+Shift+O → Ò"),
    (0x15, 249, "U + AltGr → ù"),
    (0x15, 217, "AltGr+Shift+U → Ù"),
]


def build_alt_numpad_steps(code):
    """Build MacroStep list for an Alt+numpad sequence."""
    digits = [int(d) for d in str(code)]
    steps = []
    # Press LeftAlt alone
    steps.append({"modifier": 4, "keys": [], "delay_us": 20000})
    # Type each digit
    for d in digits:
        steps.append({"modifier": 4, "keys": [NUMPAD[d]], "delay_us": 15000})
        steps.append({"modifier": 4, "keys": [], "delay_us": 10000})
    # Release LeftAlt
    steps.append({"modifier": 0, "keys": [], "delay_us": 0})
    return steps


def main():
    # keycode_map: physical US position → Italian layout position
    # Format: "decimal_from": "decimal_to" (string).
    # Only non-identity mappings are listed. Keys not listed = passthrough.
    #
    # HID 0x35 (53, US: `~, left of 1) → Italian: \ | (HID 0x31 = 49)
    # HID 0x37 (55, US: /?, right of .) → Italian: - _ (HID 0x36 = 54)
    keycode_map = {
        "53": "49",   # 0x35 → 0x31
        "55": "54",   # 0x37 → 0x36
    }

    config = {
        "language": "Italian",
        "description": "Italian QWERTY keyboard layout with AltGr+numpad macros for accented characters. "
                       "Works on any host OS with Alt+numpad support (Windows). "
                       "Trigger: RightAlt (AltGr, modifier bit 0x40) + letter key.",
        "host_os": "windows",
        "keycode_map": dict(sorted(keycode_map.items(), key=lambda x: int(x[0]))),
        "macros": []
    }

    for keycode, alt_code, comment in ITALIAN_CHARS:
        macro = {
            "comment": comment,
            "trigger": {
                "modifiers": 0x40,    # RightAlt / AltGr
                "keycode": keycode,
                "mod_mask": 0x40      # check only RightAlt bit
            },
            "steps": build_alt_numpad_steps(alt_code)
        }
        config["macros"].append(macro)

    with open("config/keymap_it.json", "w") as f:
        json.dump(config, f, indent=2)
        f.write("\n")

    print(f"Generated config/keymap_it.json with {len(config['macros'])} macros")


if __name__ == "__main__":
    main()
