# Pi 500 as a USB HID Keyboard

Turn your Raspberry Pi 500 into a USB keyboard for your PC. The Pi 500's built-in
keyboard is forwarded to the host computer via the USB-C port.

> **Tested on:** Raspberry Pi 500 (Rev 1.0) with kernel `6.18.34+rpt-rpi-2712`,
> Raspberry Pi OS (Debian Bookworm). UDC: `1000480000.usb`.

## How It Works

The Pi 500's internal keyboard is a USB HID device connected internally via the
RP1 southbridge. `pi400kb` reads raw HID reports from `/dev/hidraw0` and forwards
them through a configfs-based USB HID gadget on the DWC2 controller (at
`axi/usb@480000`). The host PC sees the Pi 500 as a standard USB HID keyboard
with VID `2e8a` / PID `0010`.

## Hardware Setup

1. Connect the Pi 500's **USB-C port** (the power/data port) to your host PC with
   a **USB-C to USB-A cable** that supports **data transfer** — not a charge-only
   cable. A phone sync/data cable works well.
2. The Pi 500 draws power through the same USB-C port. If your host PC can't
   supply enough current, use a **USB-C data + power splitter** to inject
   additional power from an official Raspberry Pi PSU.

## OS Configuration

### 1. Enable DWC2 Peripheral Mode

The Pi 500's USB-C port must be switched from host mode to device/peripheral
mode. Use the `rpi-usb-gadget` tool:

```bash
sudo rpi-usb-gadget on
sudo reboot
```

This adds `dtoverlay=dwc2,dr_mode=peripheral` to `/boot/firmware/config.txt`
and configures the legacy `g_ether` module to load at boot (which brings up
 the DWC2 UDC).

After reboot, verify the UDC exists:

```bash
ls /sys/class/udc/
# Output: 1000480000.usb
```

### 2. The `g_ether` Conflict

`rpi-usb-gadget` loads `g_ether` (USB Ethernet gadget) at boot, which claims the
UDC. `pi400kb` **automatically detects and unloads `g_ether`** so the HID gadget
can take over — no manual steps needed.

## Building

### Prerequisites

```bash
sudo apt install libconfig-dev git cmake
```

### Build

```bash
git clone https://github.com/Gadgetoid/pi400kb
cd pi400kb
git submodule update --init
mkdir build
cd build
cmake .. \
    -DKEYBOARD_VID=0x2e8a \
    -DKEYBOARD_PID=0x0010 \
    -DKEYBOARD_DEV=/dev/input/by-id/usb-Raspberry_Pi_Ltd_Pi_500_Keyboard-event-kbd
make -j$(nproc)
```

Or use the convenience script:

```bash
./configure-pi500.sh
cd build && make -j$(nproc)
```

## Usage

### Keyboard-Only Mode

```bash
sudo build/pi400kb --keyboard-only
# or:
sudo build/pi400kb -k
```

### Keyboard + Mouse Mode

```bash
sudo build/pi400kb
```

### Key Bindings

| Action | Key Combo |
|---|---|
| Toggle grab/release (Pi ↔ host) | `Ctrl + Raspberry` |
| Exit program | `Ctrl + Shift + Raspberry` |

When **grabbed**, keystrokes go to the **host PC**. Press `Ctrl + Raspberry` to
release and use the Pi locally again.

## Autostart (systemd)

```bash
sudo cp build/pi400kb /usr/bin/pi400kb
sudo cp pi400kb.service /etc/systemd/system/
sudo cp pi400kb.conf /etc/modules-load.d/
sudo systemctl enable --now pi400kb.service
```

## Build Options

| CMake Variable | Pi 500 Default | Description |
|---|---|---|
| `KEYBOARD_VID` | `0x2e8a` | Keyboard Vendor ID |
| `KEYBOARD_PID` | `0x0010` | Keyboard Product ID |
| `KEYBOARD_DEV` | `…Pi_500_Keyboard-event-kbd` | Keyboard evdev path |
| `MOUSE_VID` | `0x093a` | Mouse Vendor ID |
| `MOUSE_PID` | `0x2510` | Mouse Product ID |
| `MOUSE_DEV` | `/dev/input/by-id/usb-PixArt_USB_Optical_Mouse-event-mouse` | Mouse evdev path |
| `HOOK_PATH` | `/home/pi/pi400kb/hook.sh` | Shell hook on grab/release |
| `NO_OUTPUT` | `OFF` | Dry-run: read input but skip USB output |

## CLI Flags

| Flag | Description |
|---|---|
| `-k`, `--keyboard-only` | Skip mouse detection entirely |

## Troubleshooting

### Host PC doesn't see the Pi 500

1. **Cable:** The most common cause. Make sure the cable has **data lines**.
   Charge-only cables will power the Pi but won't transfer USB data.
   Test the cable by using it to transfer data between your phone and PC.
2. **Replug:** Start `pi400kb`, then **unplug and replug the USB-C cable**.
   This forces the host to re-enumerate the device (especially after switching
   from `g_ether` to the HID gadget).
3. **Verify on host:** On the host PC, run `lsusb` — you should see
   `Bus … Device …: ID 2e8a:0010` labelled "Pi400KB".
4. **Check UDC state:** On the Pi, run:
   ```bash
   cat /sys/class/udc/1000480000.usb/state
   ```
   Should show `configured` when a host is connected. `not attached` means no
   host detected.

### USBG_ERROR_BUSY / duplicate gadget name

A previous run left stale configfs state. `pi400kb` cleans up automatically,
but you can manually reset:

```bash
echo "" | sudo tee /sys/kernel/config/usb_gadget/pi400kb/UDC 2>/dev/null
sudo rm -rf /sys/kernel/config/usb_gadget/pi400kb
sudo rm -rf /sys/kernel/config/usb_gadget/g1
```

### No UDC (1000480000.usb missing)

```bash
sudo rpi-usb-gadget on && sudo reboot
```

### Segfault on exit (exit code 139)

Fixed in this version. If you see this, rebuild from the latest source.

## Hardware Details

- **SoC:** BCM2712 (Pi 5 / Pi 500)
- **USB Controller:** DWC2 OTG at `axi/usb@480000`, compatible `brcm,bcm2835-usb`
- **Southbridge:** RP1 (PCIe), provides XHCI host controllers for USB-A ports
- **Gadget UDC:** `1000480000.usb`
- **Internal Keyboard:** USB HID, appears as `hidraw0` and at
  `/dev/input/by-id/usb-Raspberry_Pi_Ltd_Pi_500_Keyboard-event-kbd`
  (VID `2e8a`, PID `0010`)

## Credits

Original gist by [Gadgetoid](https://github.com/Gadgetoid):
https://gist.github.com/Gadgetoid/5a8ceb714de8e630059d30612503653f

Pi 500 port with auto-cleanup, `--keyboard-only` flag, and legacy `g_ether`
module unloading by the community.


