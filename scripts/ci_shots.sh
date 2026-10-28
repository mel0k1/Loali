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
export LANG=C.UTF-8
unset QT_QUICK_BACKEND
unset WAYLAND_DISPLAY

SHOT_TOOL="${SHOT_TOOL:-native}"
mkdir -p build/shots

APPDIR="$XDG_RUNTIME_DIR/loali-apps"
mkdir -p "$APPDIR/applications"
write_desktop() {
    cat > "$APPDIR/applications/loali-$1.desktop" <<EOT
[Desktop Entry]
Type=Application
Name=$1
Comment=Loali demo app
Exec=true
Icon=$2
Terminal=false
Categories=Utility;
EOT
}
write_desktop Files folder
write_desktop Terminal utilities-terminal
write_desktop Settings preferences-system
export XDG_DATA_DIRS="$APPDIR:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"

run_shot() {
    out="$1"
    demo="$2"
    extra="$3"
    echo "== shot: $out (demo: ${demo:-none}, tool: $SHOT_TOOL) =="
    pkill -f build/lodemo 2>/dev/null || true
    pkill -f build/lolayer 2>/dev/null || true
    DEMO_ARGS="$demo" EXTRA_ARGS="$extra" DEMO_CLIENTS="$4" OUT="$out" SHOT_TOOL="$SHOT_TOOL" LOALI_LAYER="$layer" SHOT_DELAY="$delay" dbus-run-session -- sh -c '
        if [ "$SHOT_TOOL" = "grim" ]; then
            gdb -q -batch -ex run -ex "echo === BACKTRACE ===" -ex "thread apply all bt 20" \
                --args ./build/loali $DEMO_ARGS $EXTRA_ARGS >build/shots/loali.log 2>&1 &
        else
            ./build/loali $DEMO_ARGS $EXTRA_ARGS --screenshot "$OUT" --screenshot-delay "${SHOT_DELAY:-8000}" >build/shots/loali.log 2>&1 &
        fi
        comp=$!
        sleep 5
        echo "--- loali.log (tail 40) ---"
        tail -40 build/shots/loali.log || true
        if kill -0 $comp 2>/dev/null; then
            echo "loali: running"
        else
            echo "loali: exited early"
        fi
        if [ "$DEMO_CLIENTS" = "1" ]; then
            socket=$(ls "$XDG_RUNTIME_DIR" | grep "^wayland-" | head -n1)
            WAYLAND_DISPLAY="${socket:-wayland-0}" DISPLAY= QT_QPA_PLATFORM=wayland \
                ./build/lodemo -t Files -c "#7C4DFF" -m "native capture" >build/shots/lodemo1.log 2>&1 &
            WAYLAND_DISPLAY="${socket:-wayland-0}" DISPLAY= QT_QPA_PLATFORM=wayland \
                ./build/lodemo -t Terminal -c "#2E7D32" -m "wl_shm client" >build/shots/lodemo2.log 2>&1 &
            if [ "$LOALI_LAYER" = "1" ]; then
                WAYLAND_DISPLAY="${socket:-wayland-0}" DISPLAY= ./build/lolayer >build/shots/lolayer.log 2>&1 &
                echo "--- lolayer.log ---"
                sleep 2
                cat build/shots/lolayer.log 2>/dev/null || true
            fi
            if [ "$LOALI_XEYES" = "1" ]; then
                xd=$(grep -oE "window manager ready on :[0-9]+" build/shots/loali.log | head -n1 | grep -oE "[0-9]+")
                sleep 1
                DISPLAY=":${xd:-1}" xeyes >build/shots/xeyes.log 2>&1 &
                sleep 4
            fi
            sleep 3
            echo "--- loali.log after clients (tail 12) ---"
            tail -12 build/shots/loali.log || true
            if kill -0 $comp 2>/dev/null; then
                echo "loali: running after clients"
            else
                echo "loali: DIED after clients"
            fi
        fi
        status=0
        if [ "$SHOT_TOOL" = "grim" ]; then
            socket=$(ls "$XDG_RUNTIME_DIR" | grep "^wayland-" | head -n1)
            echo "grim: WAYLAND_DISPLAY=${socket:-wayland-0}"
            WAYLAND_DISPLAY="${socket:-wayland-0}" DISPLAY= WAYLAND_DEBUG=1 grim "$OUT" >build/shots/grim.log 2>&1
            gcode=$?
            echo "grim exit: $gcode"
            [ "$gcode" -eq 0 ] || status=1
            echo "--- grim.log (tail 60) ---"
            tail -60 build/shots/grim.log 2>/dev/null || true
            if kill -0 $comp 2>/dev/null; then
                echo "loali: running after grim"
            else
                echo "loali: DIED during grim"
            fi
            kill $comp 2>/dev/null || true
            wait $comp 2>/dev/null || true
        else
            wait $comp || status=$?
        fi
        echo "--- lodemo1.log ---"
        cat build/shots/lodemo1.log 2>/dev/null || true
        if [ -f "$OUT" ]; then
            echo "saved: $OUT"
        else
            echo "screenshot missing"
            status=1
        fi
        exit $status
    '
}

run_shot build/shots/desktop.png "" "" 1
LOALI_LAYER=1 run_shot build/shots/layer.png "" "" 1
SHOT_DELAY=14000 LOALI_XEYES=1 run_shot build/shots/xwayland.png "" "" 1
run_shot build/shots/launcher.png "--demo launcher" "" 1
run_shot build/shots/settings.png "--demo settings" "" 1
run_shot build/shots/expo.png "--demo expo" "" 1
run_shot build/shots/taskbar.png "--demo preview" "--demo-delay 7000 --screenshot-delay 15000" 1

ls -la build/shots
