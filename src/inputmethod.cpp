#include "inputmethod.h"

#include <QEvent>
#include <QGuiApplication>
#include <QKeyEvent>

#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandKeyboard>

#if __has_include(<QtWaylandCompositor/private/qwaylandkeyboard_p.h>)
#define LOALI_QT_KEYBOARD_BRIDGE 1
#pragma push_macro("private")
#undef private
#define private public
#include <QtWaylandCompositor/private/qwaylandkeyboard_p.h>
#pragma pop_macro("private")
#endif

#include <QtWaylandCompositor/QWaylandOutput>

InputMethodManager *InputMethodManager::s_instance = nullptr;
#ifdef HAVE_QT_TEXTINPUT_BRIDGE
QtTextInputBridge *QtTextInputBridge::s_instance = nullptr;
#endif

InputMethod::InputMethod(InputMethodManager *manager, QWaylandSeat *seat,
                         const QWaylandResource &resource)
    : m_manager(manager)
    , m_seat(seat)
{
    init(resource.resource());
}

InputMethod::~InputMethod()
{
    if (m_active) {
        m_active = false;
        emit unavailable(this);
    }
}

InputMethod *InputMethod::fromResource(::wl_resource *resource)
{
    auto *res = Resource::fromResource(resource);
    return res ? static_cast<InputMethod *>(res->object()) : nullptr;
}

void InputMethod::syncFocus(QWaylandSurface *surface)
{
    if (m_focused == surface)
        return;
    m_focused = surface;
    if (surface && !m_active) {
        m_active = true;
        send_activate();
        send_done();
    } else if (!surface && m_active) {
        m_active = false;
        send_deactivate();
        send_done();
    }
}

void InputMethod::deliverText(const QString &commit, const QString &preedit)
{
#ifdef HAVE_QT_TEXTINPUT_BRIDGE
    if (!m_focused)
        return;
    if (QtTextInputBridge::instance())
        QtTextInputBridge::instance()->deliverToFocused(m_focused, commit, preedit);
#else
    Q_UNUSED(commit)
    Q_UNUSED(preedit)
#endif
}

void InputMethod::zwp_input_method_v2_commit_string(Resource *, const QString &text)
{
    deliverText(text, QString());
}

void InputMethod::zwp_input_method_v2_set_preedit_string(Resource *, const QString &text,
                                                         int32_t cursor, int32_t anchor)
{
    Q_UNUSED(cursor)
    Q_UNUSED(anchor)
    deliverText(QString(), text);
}

void InputMethod::zwp_input_method_v2_delete_surrounding_text(Resource *, uint32_t before_length,
                                                              uint32_t after_length)
{
    Q_UNUSED(before_length)
    Q_UNUSED(after_length)
}

void InputMethod::zwp_input_method_v2_commit(Resource *, uint32_t serial)
{
    Q_UNUSED(serial)
}

void InputMethod::zwp_input_method_v2_grab_keyboard(Resource *resource, uint32_t keyboard)
{
    if (!m_seat)
        return;
    QWaylandResource grabResource(wl_resource_create(resource->client(),
                                                     &::zwp_input_method_keyboard_grab_v2_interface,
                                                     wl_resource_get_version(resource->handle),
                                                     keyboard));
    if (!grabResource.resource())
        return;
    auto *grab = new InputMethodKeyboardGrab(this, m_seat, grabResource);
    m_grabs.append(grab);
    if (m_manager)
        m_manager->m_grabs.append(grab);
    connect(grab, &InputMethodKeyboardGrab::grabEnded, this, [this](InputMethodKeyboardGrab *g) {
        m_grabs.removeAll(g);
        if (m_manager)
            m_manager->removeGrab(g);
    });
    grab->sendInitialState();
}

void InputMethod::zwp_input_method_v2_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void InputMethod::zwp_input_method_v2_destroy_resource(Resource *)
{
    deleteLater();
}

InputMethodKeyboardGrab::InputMethodKeyboardGrab(InputMethod *method, QWaylandSeat *seat,
                                                 const QWaylandResource &resource)
    : m_seat(seat)
{
    Q_UNUSED(method)
    init(resource.resource());
}

InputMethodKeyboardGrab::~InputMethodKeyboardGrab() = default;

void InputMethodKeyboardGrab::sendInitialState()
{
#ifdef LOALI_QT_KEYBOARD_BRIDGE
    if (!m_seat || !m_seat->keyboard())
        return;
    auto *keyboardPrivate = QWaylandKeyboardPrivate::get(m_seat->keyboard());
    if (keyboardPrivate->keymap_fd >= 0) {
        send_keymap(1, keyboardPrivate->keymap_fd, uint32_t(keyboardPrivate->keymap_size));
    }
    send_modifiers(m_serial++, keyboardPrivate->modsDepressed, keyboardPrivate->modsLatched,
                   keyboardPrivate->modsLocked, 0);
    send_repeat_info(25, 250);
#endif
}

void InputMethodKeyboardGrab::forwardKey(QKeyEvent *event)
{
#ifdef LOALI_QT_KEYBOARD_BRIDGE
    const uint32_t key = QWaylandKeyboardPrivate::toWaylandKey(event->nativeScanCode());
    const uint32_t state = event->type() == QEvent::KeyPress ? 1u : 0u;
    send_key(m_serial++, uint32_t(event->timestamp()), key, state);
    if (m_seat && m_seat->keyboard()) {
        auto *keyboardPrivate = QWaylandKeyboardPrivate::get(m_seat->keyboard());
        send_modifiers(m_serial++, keyboardPrivate->modsDepressed, keyboardPrivate->modsLatched,
                       keyboardPrivate->modsLocked, 0);
    }
#endif
}

void InputMethodKeyboardGrab::zwp_input_method_keyboard_grab_v2_release(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void InputMethodKeyboardGrab::zwp_input_method_keyboard_grab_v2_destroy_resource(Resource *)
{
    emit grabEnded(this);
    deleteLater();
}

VirtualKeyboard::VirtualKeyboard(QWaylandSeat *seat, const QWaylandResource &resource)
    : m_seat(seat)
{
    init(resource.resource());
}

VirtualKeyboard::~VirtualKeyboard() = default;

void VirtualKeyboard::zwp_virtual_keyboard_v1_keymap(Resource *, uint32_t format, int32_t fd,
                                                     uint32_t size)
{
    Q_UNUSED(format)
    Q_UNUSED(fd)
    Q_UNUSED(size)
}

void VirtualKeyboard::zwp_virtual_keyboard_v1_key(Resource *, uint32_t time, uint32_t key,
                                                  uint32_t state)
{
    Q_UNUSED(time)
#ifdef LOALI_QT_KEYBOARD_BRIDGE
    if (!m_seat || !m_seat->keyboard())
        return;
    auto *keyboardPrivate = QWaylandKeyboardPrivate::get(m_seat->keyboard());
    keyboardPrivate->keyEvent(key, state);
#else
    Q_UNUSED(key)
    Q_UNUSED(state)
#endif
}

void VirtualKeyboard::zwp_virtual_keyboard_v1_modifiers(Resource *, uint32_t mods_depressed,
                                                        uint32_t mods_latched, uint32_t mods_locked,
                                                        uint32_t group)
{
#ifdef LOALI_QT_KEYBOARD_BRIDGE
    if (!m_seat || !m_seat->keyboard())
        return;
    auto *keyboardPrivate = QWaylandKeyboardPrivate::get(m_seat->keyboard());
    keyboardPrivate->modsDepressed = mods_depressed;
    keyboardPrivate->modsLatched = mods_latched;
    keyboardPrivate->modsLocked = mods_locked;
    keyboardPrivate->group = group;
#else
    Q_UNUSED(mods_depressed)
    Q_UNUSED(mods_latched)
    Q_UNUSED(mods_locked)
    Q_UNUSED(group)
#endif
}

void VirtualKeyboard::zwp_virtual_keyboard_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void VirtualKeyboard::zwp_virtual_keyboard_v1_destroy_resource(Resource *)
{
    deleteLater();
}

InputMethodManager::InputMethodManager(QObject *parent)
    : QWaylandCompositorExtensionTemplate<InputMethodManager>(qobject_cast<QWaylandObject *>(parent))
{
}

InputMethodManager::~InputMethodManager()
{
    if (s_instance == this)
        s_instance = nullptr;
    qApp->removeEventFilter(this);
}

void InputMethodManager::initialize()
{
    QWaylandCompositorExtensionTemplate<InputMethodManager>::initialize();
    auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor)
        return;
    init(compositor->display(), 1);
    s_instance = this;
#ifdef HAVE_QT_TEXTINPUT_BRIDGE
    auto *bridge = new QtTextInputBridge(compositor);
    bridge->initialize();
#endif
    if (compositor->defaultSeat() && compositor->defaultSeat()->keyboard()) {
        connect(compositor->defaultSeat()->keyboard(), &QWaylandKeyboard::focusChanged, this,
                &InputMethodManager::syncFocus);
    }
    qApp->installEventFilter(this);
    qInfo() << "InputMethod: zwp_input_method_v2 ready";
}

void InputMethodManager::syncFocus(QWaylandSurface *surface)
{
    for (InputMethod *method : std::as_const(m_methods))
        method->syncFocus(surface);
}

bool InputMethodManager::eventFilter(QObject *watched, QEvent *event)
{
    if (m_grabs.isEmpty())
        return QObject::eventFilter(watched, event);
    if (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease)
        return QObject::eventFilter(watched, event);
    auto *keyEvent = static_cast<QKeyEvent *>(event);
    for (InputMethodKeyboardGrab *grab : std::as_const(m_grabs))
        grab->forwardKey(keyEvent);
    return true;
}

void InputMethodManager::removeGrab(InputMethodKeyboardGrab *grab)
{
    m_grabs.removeAll(grab);
}

void InputMethodManager::zwp_input_method_manager_v2_get_input_method(Resource *resource,
                                                                      ::wl_resource *seat,
                                                                      uint32_t input_method)
{
    QWaylandSeat *waylandSeat = QWaylandSeat::fromSeatResource(seat);
    if (!waylandSeat) {
        wl_resource_post_error(resource->handle, 0, "invalid seat");
        return;
    }
    QWaylandResource methodResource(wl_resource_create(resource->client(),
                                                       &::zwp_input_method_v2_interface,
                                                       wl_resource_get_version(resource->handle),
                                                       input_method));
    if (!methodResource.resource())
        return;
    auto *method = new InputMethod(this, waylandSeat, methodResource);
    m_methods.append(method);
    if (waylandSeat->keyboard())
        method->syncFocus(waylandSeat->keyboard()->focus());
}

void InputMethodManager::zwp_input_method_manager_v2_destroy(Resource *)
{
}

VirtualKeyboardManager::VirtualKeyboardManager(QObject *parent)
    : QWaylandCompositorExtensionTemplate<VirtualKeyboardManager>(qobject_cast<QWaylandObject *>(parent))
{
}

VirtualKeyboardManager::~VirtualKeyboardManager() = default;

void VirtualKeyboardManager::initialize()
{
    QWaylandCompositorExtensionTemplate<VirtualKeyboardManager>::initialize();
    auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor)
        return;
    init(compositor->display(), 1);
    qInfo() << "VirtualKeyboard: zwp_virtual_keyboard_v1 ready";
}

void VirtualKeyboardManager::zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(
    Resource *resource, ::wl_resource *seat, uint32_t id)
{
    QWaylandSeat *waylandSeat = QWaylandSeat::fromSeatResource(seat);
    if (!waylandSeat) {
        wl_resource_post_error(resource->handle, 0, "invalid seat");
        return;
    }
    QWaylandResource keyboardResource(wl_resource_create(resource->client(),
                                                         &::zwp_virtual_keyboard_v1_interface,
                                                         wl_resource_get_version(resource->handle),
                                                         id));
    if (!keyboardResource.resource())
        return;
    new VirtualKeyboard(waylandSeat, keyboardResource);
}


#ifdef HAVE_QT_TEXTINPUT_BRIDGE

QtTextInput::QtTextInput(QtWaylandServer::qt_text_input_method_manager_v1 *manager,
                         QWaylandSeat *seat, const QWaylandResource &resource)
    : m_seat(seat)
{
    Q_UNUSED(manager)
    init(resource.resource());
}

QtTextInput::~QtTextInput() = default;

void QtTextInput::text_input_method_v1_enable(Resource *resource, ::wl_resource *surface)
{
    QWaylandSurface *qs = QWaylandSurface::fromResource(surface);
    if (qs && resource->handle && wl_resource_get_client(resource->handle)
               == wl_resource_get_client(surface))
        m_enabled = qs;
}

void QtTextInput::text_input_method_v1_disable(Resource *, ::wl_resource *surface)
{
    QWaylandSurface *qs = QWaylandSurface::fromResource(surface);
    if (qs && m_enabled == qs)
        m_enabled = nullptr;
}

void QtTextInput::text_input_method_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void QtTextInput::text_input_method_v1_reset(Resource *)
{
    m_surroundingText.clear();
    m_surroundingOffset = 0;
}

void QtTextInput::text_input_method_v1_commit(Resource *)
{
}

void QtTextInput::text_input_method_v1_update_surrounding_text(Resource *,
                                                               const QString &surroundingText,
                                                               int32_t textOffset)
{
    m_surroundingText = surroundingText;
    m_surroundingOffset = textOffset;
}

void QtTextInput::text_input_method_v1_acknowledge_input_method(Resource *)
{
}

void QtTextInput::text_input_method_v1_destroy_resource(Resource *)
{
    deleteLater();
}

void QtTextInput::deliverCommit(const QString &commit, const QString &preedit)
{
    Resource *res = resource();
    if (!res || !res->handle)
        return;
    ++m_serial;
    send_start_input_method_event(res->handle, m_serial, m_surroundingOffset);
    if (!preedit.isEmpty())
        send_input_method_event_attribute(res->handle, m_serial, 1, 0, preedit.length(), preedit);
    send_end_input_method_event(res->handle, m_serial, commit, preedit, 0, 0);
}

QtTextInputBridge::QtTextInputBridge(QObject *parent)
    : QWaylandCompositorExtensionTemplate<QtTextInputBridge>(qobject_cast<QWaylandObject *>(parent))
{
}

QtTextInputBridge::~QtTextInputBridge()
{
    if (s_instance == this)
        s_instance = nullptr;
}

void QtTextInputBridge::initialize()
{
    QWaylandCompositorExtensionTemplate<QtTextInputBridge>::initialize();
    auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor)
        return;
    init(compositor->display(), 1);
    s_instance = this;
    if (compositor->defaultSeat() && compositor->defaultSeat()->keyboard()) {
        connect(compositor->defaultSeat()->keyboard(), &QWaylandKeyboard::focusChanged, this,
                &QtTextInputBridge::syncFocus);
    }
    qInfo() << "InputMethod: qt text input bridge ready";
}

void QtTextInputBridge::syncFocus(QWaylandSurface *focused)
{
    for (QtTextInput *input : std::as_const(m_inputs)) {
        if (input->enabledSurface() && input->enabledSurface() != focused)
            input->m_enabled = nullptr;
    }
}

void QtTextInputBridge::deliverToFocused(QWaylandSurface *focused, const QString &commit,
                                         const QString &preedit)
{
    for (QtTextInput *input : std::as_const(m_inputs)) {
        if (input->isEnabledFor(focused)) {
            input->deliverCommit(commit, preedit);
            return;
        }
    }
}

void QtTextInputBridge::text_input_method_manager_v1_get_text_input_method(Resource *resource,
                                                                           uint32_t id,
                                                                           ::wl_resource *seat)
{
    QWaylandSeat *waylandSeat = QWaylandSeat::fromSeatResource(seat);
    if (!waylandSeat) {
        auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
        waylandSeat = compositor ? compositor->defaultSeat() : nullptr;
    }
    QWaylandResource inputResource(wl_resource_create(resource->client(),
                                                      &::qt_text_input_method_v1_interface,
                                                      wl_resource_get_version(resource->handle),
                                                      id));
    if (!inputResource.resource())
        return;
    m_inputs.append(new QtTextInput(this, waylandSeat, inputResource));
}

void QtTextInputBridge::text_input_method_manager_v1_destroy(Resource *)
{
}

#endif
