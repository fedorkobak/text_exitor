#!/usr/bin/env bash
# Sourced by linuxdeploy's AppRun after its Qt environment hook.
# Diagnose OS dependencies before the dynamic loader can terminate the editor.
editor_bundle_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
editor_runtime_error=""
if command -v ldd >/dev/null 2>&1; then
    editor_loader_output="$(LC_ALL=C ldd "${editor_bundle_dir}/usr/bin/text_editor" \
        "${editor_bundle_dir}/usr/plugins/platforms/libqxcb.so" 2>&1 || true)"
    if [[ "$editor_loader_output" == *"not found"* ]]; then
        editor_runtime_error="The editor needs system graphics, font, and C/C++ runtime libraries to start.

Missing components reported by the system:
${editor_loader_output}

On Ubuntu or Linux Mint, the usual runtime packages are:
sudo apt install libgl1 libx11-6 libx11-xcb1 libxcb1 libfontconfig1 libfreetype6 libstdc++6 libc6 fonts-dejavu-core

If the missing component is a GLIBC/GLIBCXX version, use a newer supported distribution or rebuild on an older distribution. If a Qt library is missing from this AppImage, obtain a complete package or rebuild the AppImage."
    fi
fi
if [[ -z "$editor_runtime_error" && -z "${DISPLAY:-}" && -z "${QT_QPA_PLATFORM:-}" ]]; then
    editor_runtime_error="No X11 display is available. This package uses Qt's XCB display plugin.
Start the editor inside your graphical desktop session. On a Wayland desktop, enable XWayland (Ubuntu/Mint package: xwayland). For SSH, configure X forwarding and an X server on the client."
fi
if [[ -n "$editor_runtime_error" ]]; then
    printf '%s\n' "$editor_runtime_error" >&2
    if command -v zenity >/dev/null 2>&1; then
        env -u LD_LIBRARY_PATH -u QT_PLUGIN_PATH -u QT_QPA_PLATFORM_PLUGIN_PATH \
            zenity --error --no-markup --title="Text Editor could not start" --text="$editor_runtime_error" || true
    fi
    exit 1
fi
unset editor_bundle_dir editor_runtime_error editor_loader_output
