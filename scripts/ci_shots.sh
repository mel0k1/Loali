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
        ./build/loali $DEMO_ARGS --screenshot "$OUT" --screenshot-delay 8000 >build/shots/loali.log 2>&1 &
        comp=$!
        sleep 5
        echo "--- loali.log ---"
        tail -6 build/shots/loali.log || true
        if [ "$DEMO_CLIENTS" = "1" ]; then
            socket=$(ls "$XDG_RUNTIME_DIR" | grep "^wayland-" | head -n1)
            WAYLAND_DISPLAY="${socket:-wayland-0}" DISPLAY= QT_QPA_PLATFORM=wayland \
                ./build/lodemo -t Files -c "#7C4DFF" -m "native capture" >build/shots/lodemo1.log 2>&1 &
            WAYLAND_DISPLAY="${socket:-wayland-0}" DISPLAY= QT_QPA_PLATFORM=wayland \
                ./build/lodemo -t Terminal -c "#2E7D32" -m "wl_shm client" >build/shots/lodemo2.log 2>&1 &
        fi
        wait $comp
        status=$?
        head -4 build/shots/lodemo1.log 2>/dev/null || true
        if [ -f "$OUT" ]; then
            echo "saved: $OUT"
        else
            echo "screenshot missing"
            status=1
        fi
        exit $status
    '
}

run_shot build/shots/desktop.png "" 1
run_shot build/shots/launcher.png "--demo launcher" 1
run_shot build/shots/settings.png "--demo settings" 1
run_shot build/shots/expo.png "--demo expo" 1

ls -la build/shots
