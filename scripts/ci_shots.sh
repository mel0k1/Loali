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
                ./build/lodemo -a files -t Files -c "#7C4DFF" -m "Loali session" >build/shots/lodemo1.log 2>&1 &
            WAYLAND_DISPLAY="${socket:-wayland-0}" DISPLAY= QT_QPA_PLATFORM=wayland \
                ./build/lodemo -a terminal -t Terminal -c "#2E7D32" -m "wl_shm client" >build/shots/lodemo2.log 2>&1 &
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
        echo "--- lodemo logs ---"
        cat build/shots/lodemo1.log 2>/dev/null || true
        cat build/shots/lodemo2.log 2>/dev/null || true
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

echo "=== protocol tests (standalone session) ==="
mkdir -p build/shots
DEMO_ARGS="" dbus-run-session -- sh -c '
    pkill -x loali 2>/dev/null || true
    if command -v pipewire >/dev/null 2>&1; then
        pipewire >build/shots/pipewire.log 2>&1 &
        echo "pipewire started"
    fi
    ./build/loali >build/shots/loali_proto.log 2>&1 &
    comp=$!
    sleep 6
    socket=$(ls "$XDG_RUNTIME_DIR" | grep "^wayland-" | head -n1)
    WAYLAND_DISPLAY="${socket:-wayland-0}"
    export WAYLAND_DISPLAY
    pstatus=0
    if command -v python3 >/dev/null 2>&1 && python3 -c "import dbus" 2>/dev/null; then
        echo "-- portal screenshot --"
        out=$(timeout 15 python3 tools/portaltest.py screenshot 2>&1)
        echo "$out"
        case "$out" in
            SHOT_URI=file://*) echo "portal screenshot: PASS" ;;
            *) echo "portal screenshot: FAIL"; pstatus=1 ;;
        esac
        echo "-- portal screencast --"
        out=$(timeout 15 python3 tools/portaltest.py screencast 2>&1)
        echo "$out"
        node=$(echo "$out" | grep -oE "PW_NODE=[0-9]+" | cut -d= -f2)
        if [ -n "$node" ] && [ "$node" != "0" ]; then
            echo "portal screencast: PASS (node $node)"
        elif command -v pw-cli >/dev/null 2>&1 && pw-cli ls Node 2>/dev/null | grep -q loali_screen_cast; then
            echo "portal screencast: PASS (node in graph)"
        else
            echo "portal screencast: FAIL"; pstatus=1
        fi
        echo "-- screensaver power service --"
        out=$(timeout 15 python3 tools/portaltest.py screensaver 2>&1)
        echo "$out"
        case "$out" in
            SS_COOKIE=*) echo "screensaver: PASS" ;;
            *) echo "screensaver: FAIL"; pstatus=1 ;;
        esac
    fi
    if [ -x ./build/lotest ]; then
        echo "-- idle 1500ms --"
        timeout 10 ./build/lotest idle 1500 && echo "idle: PASS" || { echo "idle: FAIL"; pstatus=1; }
        echo "-- dpms get --"
        timeout 10 ./build/lotest dpms get || { echo "dpms get: FAIL"; pstatus=1; }
        echo "-- dpms off --"
        timeout 10 ./build/lotest dpms off || { echo "dpms off: FAIL"; pstatus=1; }
        echo "-- dpms on --"
        timeout 10 ./build/lotest dpms on || { echo "dpms on: FAIL"; pstatus=1; }
        echo "-- session lock --"
        timeout 10 ./build/lotest lock unlock && echo "lock: PASS" || { echo "lock: FAIL"; pstatus=1; }
        echo "-- input method --"
        WAYLAND_DISPLAY="$socket" DISPLAY= QT_QPA_PLATFORM=wayland \
            ./build/lodemo >build/shots/lodemo_im.log 2>&1 &
        imdemo=$!
        sleep 4
        timeout 15 ./build/lotest im && echo "im: PASS" || { echo "im: FAIL"; pstatus=1; }
        echo "-- virtual keyboard --"
        timeout 10 ./build/lotest vkey 30 && echo "vkey: PASS" || { echo "vkey: FAIL"; pstatus=1; }
        kill $imdemo 2>/dev/null || true
        echo "-- blur --"
        timeout 10 ./build/lotest blur && echo "blur: PASS" || { echo "blur: FAIL"; pstatus=1; }
    else
        echo "lotest missing, skip"
    fi
    if command -v wl-copy >/dev/null 2>&1; then
        echo "-- clipboard wl-copy -> wl-paste --"
        echo -n "loali-clipboard-test" | timeout 5 wl-copy 2>/dev/null &
        sleep 1
        pasted=$(timeout 5 wl-paste 2>/dev/null || true)
        echo "pasted=[$pasted]"
        [ "$pasted" = "loali-clipboard-test" ] && echo "clipboard: PASS" || { echo "clipboard: FAIL"; pstatus=1; }
    else
        echo "wl-clipboard missing, skip"
    fi
    kill $comp 2>/dev/null || true
    exit $pstatus
'
pcode=$?
echo "protocol tests exit: $pcode"
[ "$pcode" -eq 0 ] || exit 1

echo "=== foot real-client test (standalone session) ==="
dbus-run-session -- sh -c '
    status=0
    pkill -x loali 2>/dev/null || true
    ./build/loali >build/shots/loali_foot.log 2>&1 &
    comp=$!
    sleep 6
    if command -v foot >/dev/null 2>&1; then
        socket=$(ls "$XDG_RUNTIME_DIR" | grep "^wayland-" | head -n1)
        WAYLAND_DISPLAY="${socket:-wayland-0}" timeout 10 foot >build/shots/foot.log 2>&1 &
        fpid=$!
        sleep 5
        if kill -0 $fpid 2>/dev/null; then
            echo "foot: alive after 5s: PASS"
        else
            echo "foot: exited early: FAIL"; status=1
        fi
        if grep -q "toplevel created" build/shots/loali_foot.log; then
            echo "foot toplevel: PASS"
        else
            echo "foot toplevel: FAIL"; status=1
        fi
        if grep -qiE "too old|unsupported (seat|version)" build/shots/foot.log; then
            echo "foot protocol: FAIL"; status=1
        else
            echo "foot protocol: PASS"
        fi
        kill $fpid 2>/dev/null || true
    else
        echo "foot missing, skip"
    fi
    kill $comp 2>/dev/null || true
    exit $status
'
fcode=$?
echo "foot tests exit: $fcode"
[ "$fcode" -eq 0 ] || exit 1

echo "=== idle lock via Power (standalone session) ==="
dbus-run-session -- sh -c '
    status=0
    pkill -x loali 2>/dev/null || true
    LOALI_IDLE_BLANK_MS=2500 ./build/loali >build/shots/loali_idlelock.log 2>&1 &
    comp=$!
    sleep 5
    if grep -q "locked (power)" build/shots/loali_idlelock.log; then
        echo "idle lock: PASS"
    else
        echo "idle lock: FAIL"; status=1
    fi
    socket=$(ls "$XDG_RUNTIME_DIR" | grep "^wayland-" | head -n1)
    WAYLAND_DISPLAY="${socket:-wayland-0}" timeout 10 ./build/lotest dpms on >/dev/null 2>&1 || true
    sleep 1
    if command -v xdotool >/dev/null 2>&1; then
        DISPLAY="${DISPLAY:-:99}" xdotool mousemove 640 400 click 1 2>/dev/null || true
        sleep 1
        if grep -q "unlocked (user)" build/shots/loali_idlelock.log; then
            echo "idle unlock: PASS"
        else
            echo "idle unlock: FAIL"; status=1
        fi
    else
        echo "xdotool missing, unlock skipped"
    fi
    kill $comp 2>/dev/null || true
    exit $status
'
lcode=$?
echo "idle lock tests exit: $lcode"
[ "$lcode" -eq 0 ] || exit 1
