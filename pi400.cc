#include "pi400.h"

#include "gadget-hid.h"

#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <signal.h>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>

#include <iostream>
#include <format>
#include <string>
#include <string_view>
#include <atomic>
#include <thread>
#include <chrono>

constexpr int EVIOC_GRAB = 1;
constexpr int EVIOC_UNGRAB = 0;

int hid_output;
std::atomic<bool> running{false};
std::atomic<bool> grabbed{false};

int ret;
int keyboard_fd;
int mouse_fd;
int uinput_keyboard_fd;
int uinput_mouse_fd;
struct hid_buf keyboard_buf;
struct hid_buf mouse_buf;

static bool keyboard_only = false;

void signal_handler(int /*dummy*/) {
    running = false;
}

bool modprobe_libcomposite() {
    pid_t pid;

    pid = fork();

    if (pid < 0) return false;
    if (pid == 0) {
        const char* const argv[] = {"modprobe", "libcomposite", nullptr};
        execv("/usr/sbin/modprobe", const_cast<char *const *>(argv));
        std::exit(0);
    }
    waitpid(pid, nullptr, 0);
    return true;
}

bool trigger_hook() {
    auto cmd = std::format("{} {}", HOOK_PATH, grabbed.load() ? 1u : 0u);
    std::system(cmd.c_str());
    return true;
}

int find_hidraw_device(const char *device_type, int16_t vid, int16_t pid) {
    struct hidraw_devinfo hidinfo;

    for(int x = 0; x < 16; x++){
        auto path = std::format("/dev/hidraw{}", x);

        int fd = open(path.c_str(), O_RDWR | O_NONBLOCK);
        if (fd == -1) {
            continue;
        }

        if (ioctl(fd, HIDIOCGRAWINFO, &hidinfo) == 0 &&
            hidinfo.vendor == vid && hidinfo.product == pid) {
            std::cout << "Found " << device_type << " at: " << path << std::endl;
            return fd;
        }

        close(fd);
    }

    return -1;
}

int grab(const char *dev) {
    std::cout << "Grabbing: " << dev << std::endl;
    int fd = open(dev, O_RDONLY);
    ioctl(fd, EVIOCGRAB, EVIOC_UNGRAB);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ioctl(fd, EVIOCGRAB, EVIOC_GRAB);
    return fd;
}

void ungrab(int fd) {
    ioctl(fd, EVIOCGRAB, EVIOC_UNGRAB);
    close(fd);
}

void printhex(unsigned char *buf, size_t len) {
    for(size_t x = 0; x < len; x++) {
        std::cout << std::format("{:x} ", buf[x]);
    }
    std::cout << std::endl;
}

void ungrab_both() {
    std::cout << "Releasing Keyboard and/or Mouse" << std::endl;

    if(uinput_keyboard_fd > -1) {
        ungrab(uinput_keyboard_fd);
    }

    if(uinput_mouse_fd > -1) {
        ungrab(uinput_mouse_fd);
    }

    grabbed = false;

    trigger_hook();
}

void grab_both() {
    std::cout << "Grabbing Keyboard and/or Mouse" << std::endl;

    if(keyboard_fd > -1) {
        uinput_keyboard_fd = grab(KEYBOARD_DEV);
    }

    if(mouse_fd > -1) {
        uinput_mouse_fd = grab(MOUSE_DEV);
    }

    if (uinput_keyboard_fd > -1 || uinput_mouse_fd > -1) {
        grabbed = true;
    }

    trigger_hook();
}

void send_empty_hid_reports_both() {
    if(keyboard_fd > -1) {
#ifndef NO_OUTPUT
        keyboard_buf = {};
        write(hid_output, reinterpret_cast<unsigned char *>(&keyboard_buf), KEYBOARD_HID_REPORT_SIZE + 1);
#endif
    }

    if(mouse_fd > -1) {
#ifndef NO_OUTPUT
        mouse_buf = {};
        write(hid_output, reinterpret_cast<unsigned char *>(&mouse_buf), MOUSE_HID_REPORT_SIZE + 1);
#endif
    }
}

int main(int argc, char *argv[]) {
    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        std::string_view arg{argv[i]};
        if (arg == "-k" || arg == "--keyboard-only") {
            keyboard_only = true;
        }
    }

    modprobe_libcomposite();

    keyboard_buf.report_id = 1;
    mouse_buf.report_id = 2;

    keyboard_fd = find_hidraw_device("keyboard", KEYBOARD_VID, KEYBOARD_PID);
    if(keyboard_fd == -1) {
        std::cout << "Failed to open keyboard device" << std::endl;
    }
    
    if (!keyboard_only) {
        mouse_fd = find_hidraw_device("mouse", MOUSE_VID, MOUSE_PID);
        if(mouse_fd == -1) {
            std::cout << "No mouse device found (keyboard-only mode)" << std::endl;
        }
    } else {
        mouse_fd = -1;
        std::cout << "Mouse disabled (--keyboard-only)" << std::endl;
    }

    if(mouse_fd == -1 && keyboard_fd == -1) {
        std::cout << "No devices to forward, bailing out!" << std::endl;
        return 1;
    }

#ifndef NO_OUTPUT
    ret = initUSB();
    if(ret != USBG_SUCCESS && ret != USBG_ERROR_EXIST) {
        return 1;
    }
#endif

    grab_both();


#ifndef NO_OUTPUT
    /* Find the first available /dev/hidg* device */
    do {
        for (int i = 0; i < 16; i++) {
            auto devpath = std::format("/dev/hidg{}", i);
            hid_output = open(devpath.c_str(), O_WRONLY | O_NDELAY);
            if (hid_output != -1) {
                std::cout << "Opened " << devpath << " for output" << std::endl;
                break;
            }
        }
    } while (hid_output == -1 && errno == EINTR);

    if (hid_output == -1){
        std::cout << "Error opening /dev/hidg* for writing." << std::endl;
        return 1;
    }
#endif

    std::cout << "Running..." << std::endl;
    running = true;
    signal(SIGINT, signal_handler);

    struct pollfd pollFd[2];
    pollFd[0].fd = keyboard_fd;
    pollFd[0].events = POLLIN;
    pollFd[1].fd = mouse_fd;
    pollFd[1].events = POLLIN;

    while (running.load()){
        poll(pollFd, 2, -1);
        if(keyboard_fd > -1) {
            auto c = read(keyboard_fd, keyboard_buf.data, KEYBOARD_HID_REPORT_SIZE);

            if(c == KEYBOARD_HID_REPORT_SIZE){
                std::cout << "K:";
                printhex(keyboard_buf.data, KEYBOARD_HID_REPORT_SIZE);

#ifndef NO_OUTPUT
                if(grabbed.load()) {
                    write(hid_output, reinterpret_cast<unsigned char *>(&keyboard_buf), KEYBOARD_HID_REPORT_SIZE + 1);
                    std::this_thread::sleep_for(std::chrono::microseconds(1000));
                }
#endif

                // Trap Ctrl + Raspberry and toggle capture on/off
                if(keyboard_buf.data[0] == 0x09){
                    if(grabbed.load()) {
                        ungrab_both();
                        send_empty_hid_reports_both();
                    } else {
                        grab_both();
                    }
                }
                // Trap Ctrl + Shift + Raspberry and exit
                if(keyboard_buf.data[0] == 0x0b){
                    running = false;
                    break;
                }
            }
        }
        if(mouse_fd > -1) {
            auto c = read(mouse_fd, mouse_buf.data, MOUSE_HID_REPORT_SIZE);

            if(c == MOUSE_HID_REPORT_SIZE){
                std::cout << "M:";
                printhex(mouse_buf.data, MOUSE_HID_REPORT_SIZE);

#ifndef NO_OUTPUT
                if(grabbed.load()) {
                    write(hid_output, reinterpret_cast<unsigned char *>(&mouse_buf), MOUSE_HID_REPORT_SIZE + 1);
                    std::this_thread::sleep_for(std::chrono::microseconds(1000));
                }
#endif
            }
        }
    }

    ungrab_both();
    send_empty_hid_reports_both();

#ifndef NO_OUTPUT
    std::cout << "Cleanup USB" << std::endl;
    cleanupUSB();
#endif

    return 0;
}
