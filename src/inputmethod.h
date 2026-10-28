#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QTimer>

#include <QtWaylandCompositor/QWaylandCompositorExtensionTemplate>
#include <QtWaylandCompositor/QWaylandQuickExtension>
#include <QtWaylandCompositor/QWaylandResource>
#include <QtWaylandCompositor/QWaylandSeat>
#include <QtWaylandCompositor/QWaylandSurface>

#include "qwayland-server-input-method-unstable-v2.h"
#include "qwayland-server-virtual-keyboard-unstable-v1.h"

#ifdef HAVE_QT_TEXTINPUT_BRIDGE
#include <QtWaylandCompositor/private/qwayland-server-qt-text-input-method-unstable-v1.h>
#endif

class QWaylandCompositor;
class InputMethodManager;
class InputMethodKeyboardGrab;
class VirtualKeyboard;

class InputMethod : public QObject, public QtWaylandServer::zwp_input_method_v2
{
    Q_OBJECT
public:
    InputMethod(InputMethodManager *manager, QWaylandSeat *seat, const QWaylandResource &resource);
    ~InputMethod() override;

    using QtWaylandServer::zwp_input_method_v2::interface;
    using QtWaylandServer::zwp_input_method_v2::interfaceName;

    static InputMethod *fromResource(::wl_resource *resource);

    void syncFocus(QWaylandSurface *surface);

signals:
    void unavailable(InputMethod *method);

protected:
    void zwp_input_method_v2_commit_string(Resource *resource, const QString &text) override;
    void zwp_input_method_v2_set_preedit_string(Resource *resource, const QString &text,
                                                int32_t cursor, int32_t anchor) override;
    void zwp_input_method_v2_delete_surrounding_text(Resource *resource, uint32_t before_length,
                                                     uint32_t after_length) override;
    void zwp_input_method_v2_commit(Resource *resource, uint32_t serial) override;
    void zwp_input_method_v2_grab_keyboard(Resource *resource, uint32_t keyboard) override;
    void zwp_input_method_v2_destroy(Resource *resource) override;
    void zwp_input_method_v2_destroy_resource(Resource *resource) override;

private:
    void deliverText(const QString &commit, const QString &preedit);

    InputMethodManager *m_manager = nullptr;
    QPointer<QWaylandSeat> m_seat;
    QPointer<QWaylandSurface> m_focused;
    bool m_active = false;
    QList<InputMethodKeyboardGrab *> m_grabs;
};

class InputMethodKeyboardGrab : public QObject, public QtWaylandServer::zwp_input_method_keyboard_grab_v2
{
    Q_OBJECT
public:
    InputMethodKeyboardGrab(InputMethod *method, QWaylandSeat *seat, const QWaylandResource &resource);
    ~InputMethodKeyboardGrab() override;

    using QtWaylandServer::zwp_input_method_keyboard_grab_v2::interface;
    using QtWaylandServer::zwp_input_method_keyboard_grab_v2::interfaceName;

    void forwardKey(QKeyEvent *event);
    void sendInitialState();

signals:
    void grabEnded(InputMethodKeyboardGrab *grab);

protected:
    void zwp_input_method_keyboard_grab_v2_release(Resource *resource) override;
    void zwp_input_method_keyboard_grab_v2_destroy_resource(Resource *resource) override;

private:
    QPointer<QWaylandSeat> m_seat;
    uint32_t m_serial = 1;
};

class VirtualKeyboard : public QObject, public QtWaylandServer::zwp_virtual_keyboard_v1
{
    Q_OBJECT
public:
    VirtualKeyboard(QWaylandSeat *seat, const QWaylandResource &resource);
    ~VirtualKeyboard() override;

    using QtWaylandServer::zwp_virtual_keyboard_v1::interface;
    using QtWaylandServer::zwp_virtual_keyboard_v1::interfaceName;

protected:
    void zwp_virtual_keyboard_v1_keymap(Resource *resource, uint32_t format, int32_t fd,
                                        uint32_t size) override;
    void zwp_virtual_keyboard_v1_key(Resource *resource, uint32_t time, uint32_t key,
                                     uint32_t state) override;
    void zwp_virtual_keyboard_v1_modifiers(Resource *resource, uint32_t mods_depressed,
                                           uint32_t mods_latched, uint32_t mods_locked,
                                           uint32_t group) override;
    void zwp_virtual_keyboard_v1_destroy(Resource *resource) override;
    void zwp_virtual_keyboard_v1_destroy_resource(Resource *resource) override;

private:
    QPointer<QWaylandSeat> m_seat;
};

class InputMethodManager : public QWaylandCompositorExtensionTemplate<InputMethodManager>,
                          public QtWaylandServer::zwp_input_method_manager_v2
{
    Q_OBJECT
public:
    explicit InputMethodManager(QObject *parent = nullptr);
    ~InputMethodManager() override;
    void initialize() override;

    using QtWaylandServer::zwp_input_method_manager_v2::interface;
    using QtWaylandServer::zwp_input_method_manager_v2::interfaceName;

    static InputMethodManager *instance() { return s_instance; }

    const QList<InputMethodKeyboardGrab *> &grabs() const { return m_grabs; }
    void removeGrab(InputMethodKeyboardGrab *grab);

    void syncFocus(QWaylandSurface *surface);
    bool eventFilter(QObject *watched, QEvent *event) override;

protected:
    void zwp_input_method_manager_v2_get_input_method(Resource *resource, ::wl_resource *seat,
                                                      uint32_t input_method) override;
    void zwp_input_method_manager_v2_destroy(Resource *resource) override;

private:
    static InputMethodManager *s_instance;
    QList<InputMethod *> m_methods;
    QList<InputMethodKeyboardGrab *> m_grabs;

    friend class InputMethod;
    uint32_t m_keySerial = 1;
    uint32_t m_lastModifiers = 0;
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(InputMethodManager)

class VirtualKeyboardManager : public QWaylandCompositorExtensionTemplate<VirtualKeyboardManager>,
                              public QtWaylandServer::zwp_virtual_keyboard_manager_v1
{
    Q_OBJECT
public:
    explicit VirtualKeyboardManager(QObject *parent = nullptr);
    ~VirtualKeyboardManager() override;
    void initialize() override;

    using QtWaylandServer::zwp_virtual_keyboard_manager_v1::interface;
    using QtWaylandServer::zwp_virtual_keyboard_manager_v1::interfaceName;

protected:
    void zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(Resource *resource,
                                                                 ::wl_resource *seat,
                                                                 uint32_t id) override;
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(VirtualKeyboardManager)

#ifdef HAVE_QT_TEXTINPUT_BRIDGE
class QtTextInput : public QObject, public QtWaylandServer::qt_text_input_method_v1
{
    Q_OBJECT
public:
    QtTextInput(QtWaylandServer::qt_text_input_method_manager_v1 *manager, QWaylandSeat *seat,
                const QWaylandResource &resource);
    ~QtTextInput() override;

    using QtWaylandServer::qt_text_input_method_v1::interface;
    using QtWaylandServer::qt_text_input_method_v1::interfaceName;

    bool isEnabledFor(QWaylandSurface *surface) const { return m_enabled == surface; }
    QWaylandSurface *enabledSurface() const { return m_enabled; }

    void deliverCommit(const QString &commit, const QString &preedit);

protected:
    void text_input_method_v1_enable(Resource *resource, ::wl_resource *surface) override;
    void text_input_method_v1_disable(Resource *resource, ::wl_resource *surface) override;
    void text_input_method_v1_destroy(Resource *resource) override;
    void text_input_method_v1_reset(Resource *resource) override;
    void text_input_method_v1_commit(Resource *resource) override;
    void text_input_method_v1_update_surrounding_text(Resource *resource,
                                                      const QString &surroundingText,
                                                      int32_t surroundingTextOffset) override;
    void text_input_method_v1_acknowledge_input_method(Resource *resource) override;
    void text_input_method_v1_destroy_resource(Resource *resource) override;

private:
    QWaylandSeat *m_seat = nullptr;
    QPointer<QWaylandSurface> m_enabled;

    friend class QtTextInputBridge;
    QString m_surroundingText;
    int32_t m_surroundingOffset = 0;
    uint32_t m_serial = 0;
};

class QtTextInputBridge : public QWaylandCompositorExtensionTemplate<QtTextInputBridge>,
                          public QtWaylandServer::qt_text_input_method_manager_v1
{
    Q_OBJECT
public:
    explicit QtTextInputBridge(QObject *parent = nullptr);
    ~QtTextInputBridge() override;
    void initialize() override;

    using QtWaylandServer::qt_text_input_method_manager_v1::interface;
    using QtWaylandServer::qt_text_input_method_manager_v1::interfaceName;

    static QtTextInputBridge *instance() { return s_instance; }

    void deliverToFocused(QWaylandSurface *focused, const QString &commit, const QString &preedit);
    void syncFocus(QWaylandSurface *focused);

protected:
    void text_input_method_manager_v1_get_text_input_method(Resource *resource, uint32_t id,
                                                             ::wl_resource *seat) override;
    void text_input_method_manager_v1_destroy(Resource *resource) override;

private:
    static QtTextInputBridge *s_instance;
    QList<QtTextInput *> m_inputs;
};
#endif
