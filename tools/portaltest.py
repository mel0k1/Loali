#!/usr/bin/env python3
import sys

import dbus


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else "screenshot"
    bus = dbus.SessionBus()
    obj = bus.get_object("org.freedesktop.impl.portal.desktop.loali",
                         "/org/freedesktop/portal/desktop")
    if mode == "screencast":
        iface = dbus.Interface(obj, "org.freedesktop.impl.portal.ScreenCast")
        code, results = iface.CreateSession(dbus.ObjectPath("/h"),
                                            dbus.ObjectPath("/s"), "lotest", dbus.Dictionary({}, signature="sv"))
        if code != 0:
            sys.exit(1)
        code, results = iface.Start(dbus.ObjectPath("/h"), dbus.ObjectPath("/s"),
                                    "lotest", "", dbus.Dictionary({}, signature="sv"))
        if code != 0:
            sys.exit(1)
        streams = results.get("streams", [])
        node = streams[0][0] if streams else 0
        print("PW_NODE=%u" % node)
        sys.exit(0)
    iface = dbus.Interface(obj, "org.freedesktop.impl.portal.Screenshot")
    code, results = iface.Screenshot(dbus.ObjectPath("/h"), "lotest", "", dbus.Dictionary({}, signature="sv"))
    if code != 0:
        sys.exit(1)
    uri = results.get("uri", "")
    print("SHOT_URI=%s" % uri)
    sys.exit(0 if uri.startswith("file://") else 2)


if __name__ == "__main__":
    main()
