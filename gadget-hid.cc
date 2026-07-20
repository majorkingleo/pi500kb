#include "gadget-hid.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <linux/usb/ch9.h>
#include <usbg/usbg.h>
#include <usbg/function/hid.h>

usbg_state *s;
usbg_gadget *g;
usbg_config *c;
usbg_function *f_hid;

static char report_desc[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x03,        //   Report Count (3)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x03,        //   Usage Maximum (Scroll Lock)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection

    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x02,        //   Report ID (2)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x03,        //     Usage Maximum (0x03)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x05,        //     Report Size (5)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xC0,              // End Collection
};

int initUSB() {
    int usbg_ret = -EINVAL;

    struct usbg_gadget_attrs g_attrs = {
        .bcdUSB = 0x0200,
        .bDeviceClass = USB_CLASS_PER_INTERFACE,
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
        .idVendor = KEYBOARD_VID,
        .idProduct = KEYBOARD_PID,
        .bcdDevice = 0x0001, /* Verson of device */
    };

    struct usbg_gadget_strs g_strs = {
        .manufacturer = "Gadgetoid", /* Manufacturer */
        .product = "Pi400KB",        /* Product string */
        .serial = "0123456789"       /* Serial number */
    };

    struct usbg_config_strs c_strs = {
        .configuration = "1xHID"
    };

    struct usbg_f_hid_attrs f_attrs = {
        .protocol = 1,
        .report_desc = {
            .desc = report_desc,
            .len = sizeof(report_desc),
        },
        .report_length = 16,
        .subclass = 0,
    };

    usbg_ret = usbg_init("/sys/kernel/config", &s);
    if (usbg_ret != USBG_SUCCESS) {
        fprintf(stderr, "Error on usbg init\n");
        fprintf(stderr, "Error: %s : %s\n", usbg_error_name((usbg_error)usbg_ret),
                usbg_strerror((usbg_error)usbg_ret));
        goto out1;
    }

    /* If a legacy g_* module has claimed the UDC, unload it */
    {
        usbg_udc *first_udc = usbg_get_first_udc(s);
        /* usbg_udc is opaque; check if the UDC is busy by trying to
         * see if there's a legacy gadget bound. We detect this by
         * checking if any g_* module is loaded. */
        FILE *fp = fopen("/proc/modules", "r");
        if (fp) {
            char line[256];
            int need_reinit = 0;
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "g_", 2) == 0) {
                    char modname[64];
                    sscanf(line, "%63s", modname);
                    fprintf(stderr, "Legacy gadget module '%s' is occupying "
                            "the UDC, unloading it...\n", modname);
                    char cmd[128];
                    snprintf(cmd, sizeof(cmd), "rmmod %s 2>/dev/null", modname);
                    system(cmd);
                    need_reinit = 1;
                }
            }
            fclose(fp);
            if (need_reinit) {
                usbg_cleanup(s);
                usbg_ret = usbg_init("/sys/kernel/config", &s);
                if (usbg_ret != USBG_SUCCESS) {
                    fprintf(stderr, "Error on usbg re-init\n");
                    goto out1;
                }
            }
        }
    }

    /* Clean up any existing gadget from a previous run */
    {
        /* Also clean up "g1" which rpi-usb-gadget may have created */
        const char *names[] = {"pi400kb", "g1"};
        for (int n = 0; n < 2; n++) {
            usbg_gadget *existing_g = usbg_get_gadget(s, names[n]);
            if (existing_g != NULL) {
                fprintf(stderr, "Cleaning up existing gadget '%s'\n", names[n]);

                /* Disable (unbind UDC) and check return */
                int clean_ret = usbg_disable_gadget(existing_g);
                if (clean_ret != USBG_SUCCESS) {
                    fprintf(stderr, "usbg_disable_gadget failed (%s), "
                            "forcing UDC unbind via configfs\n",
                            usbg_strerror((usbg_error)clean_ret));
                    /* Fallback: write directly to configfs to unbind the UDC */
                    char udc_path[512];
                    snprintf(udc_path, sizeof(udc_path),
                             "/sys/kernel/config/usb_gadget/%s/UDC", names[n]);
                    FILE *f = fopen(udc_path, "w");
                    if (f) {
                        fprintf(f, "\n");
                        fclose(f);
                    }
                }

                /* Remove the gadget recursively */
                clean_ret = usbg_rm_gadget(existing_g, USBG_RM_RECURSE);
                if (clean_ret != USBG_SUCCESS) {
                    fprintf(stderr, "usbg_rm_gadget failed (%s), "
                            "forcing removal via configfs\n",
                            usbg_strerror((usbg_error)clean_ret));
                    /* Fallback: remove the configfs directory */
                    char cmd[256];
                    snprintf(cmd, sizeof(cmd),
                             "rm -rf /sys/kernel/config/usb_gadget/%s 2>/dev/null",
                             names[n]);
                    system(cmd);
                }
            }
        }
    }

    usbg_ret = usbg_create_gadget(s, "pi400kb", &g_attrs, &g_strs, &g);
    if (usbg_ret != USBG_SUCCESS) {
        fprintf(stderr, "Error creating gadget\n");
        fprintf(stderr, "Error: %s : %s\n", usbg_error_name((usbg_error)usbg_ret),
                usbg_strerror((usbg_error)usbg_ret));
        goto out2;
    }

    usbg_ret = usbg_create_function(g, USBG_F_HID, "usb0", &f_attrs, &f_hid);
    if (usbg_ret != USBG_SUCCESS) {
        fprintf(stderr, "Error creating function: USBG_F_HID\n");
        fprintf(stderr, "Error: %s : %s\n", usbg_error_name((usbg_error)usbg_ret),
                usbg_strerror((usbg_error)usbg_ret));
        goto out2;
    }

    usbg_ret = usbg_create_config(g, 1, "config", NULL, &c_strs, &c);
    if (usbg_ret != USBG_SUCCESS) {
        fprintf(stderr, "Error creating config\n");
        fprintf(stderr, "Error: %s : %s\n", usbg_error_name((usbg_error)usbg_ret),
                usbg_strerror((usbg_error)usbg_ret));
        goto out2;
    }

    usbg_ret = usbg_add_config_function(c, "keyboard", f_hid);
    if (usbg_ret != USBG_SUCCESS) {
        fprintf(stderr, "Error adding function: keyboard\n");
        fprintf(stderr, "Error: %s : %s\n", usbg_error_name((usbg_error)usbg_ret),
                usbg_strerror((usbg_error)usbg_ret));
        goto out2;
    }

    /* Check if there are any UDCs available before enabling */
    {
        DIR *udc_dir = opendir("/sys/class/udc");
        if (udc_dir) {
            struct dirent *entry;
            int udc_count = 0;
            while ((entry = readdir(udc_dir)) != NULL) {
                if (entry->d_name[0] != '.') udc_count++;
            }
            closedir(udc_dir);

            if (udc_count == 0) {
                fprintf(stderr, "\n*** NO USB DEVICE CONTROLLER (UDC) FOUND ***\n");
                fprintf(stderr, "Your Pi's USB port is not configured for gadget mode.\n");
                fprintf(stderr, "Add this to /boot/firmware/config.txt and reboot:\n");
                fprintf(stderr, "  dtoverlay=dwc2,dr_mode=peripheral\n\n");
                usbg_ret = USBG_ERROR_OTHER_ERROR;
                goto out2;
            }
        }
    }

    usbg_ret = usbg_enable_gadget(g, DEFAULT_UDC);
    if (usbg_ret != USBG_SUCCESS) {
        fprintf(stderr, "Error enabling gadget\n");
        fprintf(stderr, "Error: %s : %s\n", usbg_error_name((usbg_error)usbg_ret),
                usbg_strerror((usbg_error)usbg_ret));
        goto out2;
    }

    /* Trigger soft_connect to ensure the gadget is visible to the host.
     * On DWC2/RP1, the gadget may need an explicit connect pulse. */
    {
        const char *udc_name = usbg_get_udc_name(
            usbg_get_gadget_udc(g));
        if (udc_name) {
            char sc_path[256];
            snprintf(sc_path, sizeof(sc_path),
                     "/sys/class/udc/%s/soft_connect", udc_name);
            FILE *f = fopen(sc_path, "w");
            if (f) {
                fprintf(f, "connect\n");
                fclose(f);
            }
        }
    }

    /* Success — keep s alive for later cleanup */
    return USBG_SUCCESS;

out2:
    usbg_cleanup(s);
    s = NULL;
    g = NULL;

out1:
    return usbg_ret;
}

int cleanupUSB(){
    if(g){
        usbg_disable_gadget(g);
        usbg_rm_gadget(g, USBG_RM_RECURSE);
    }
    if(s){
        usbg_cleanup(s);
    }
    return 0;
}
