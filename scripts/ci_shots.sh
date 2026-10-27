#!/bin/sh
set -e

export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/tmp/loali-xdg}"
mkdir -p "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"

if [ -z "$DISPLAY" ]; then
    export DISPLAY=:99
    Xvfb :99 -screen 0 1280x800x24 -nolisten tcp &
    sleep 2
fi

export QT_QPA_PLATFORM=xcb
export LIBGL_ALWAYS_SOFTWARE=1
export QSG_INFO=1
unset QT_QUICK_BACKEND
unset WAYLAND_DISPLAY

mkdir -p build/shots

run_shot() {
    out="$1"
    demo="$2"
    clients="$3"
    echo "== shot: $out (demo: ${demo:-none}) =="
    pkill -f build/lodemo 2>/dev/null || true
    DEMO_ARGS="$demo" DEMO_CLIENTS="$clients" OUT="$out" dbus-run-session -- sh -c '
        ./build/loali $DEMO_ARGS >build/shots/loali.log 2>&1 &
        comp=$!
        sleep 5
        echo "--- loali.log ---"
        cat build/shots/loali.log || true
        echo "--- sockets ---"
        ls -la "$XDG_RUNTIME_DIR" || true
        if [ "$DEMO_CLIENTS" = "1" ]; then
            ./build/lodemo -t Files -c "#7C4DFF" -m "Browsing files" >/dev/null 2>&1 &
            ./build/lodemo -t Terminal -c "#2E7D32" -m "wl_shm capture ready" >/dev/null 2>&1 &
            sleep 3
        fi
        socket=$(ls "$XDG_RUNTIME_DIR" | grep "^wayland-" | head -n1)
        if [ -z "$socket" ]; then
            echo "no wayland socket found"
            kill $comp 2>/dev/null
            exit 1
        fi
        echo "socket: $socket"
        WAYLAND_DISPLAY="$socket" grim "$OUT"
        status=$?
        kill $comp 2>/dev/null || true
        wait $comp 2>/dev/null || true
        exit $status
    '
}

run_shot build/shots/desktop.png "" 1
run_shot build/shots/launcher.png "--demo launcher" 1
run_shot build/shots/settings.png "--demo settings" 1
run_shot build/shots/expo.png "--demo expo" 1

ls -la build/shots
