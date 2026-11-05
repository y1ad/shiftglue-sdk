/**
 * @file        input/touch/touch_input_driver.cpp
 * @brief       Host-fed virtual pad for on-screen touch controls (Android).
 */

#include <cstring>
#include <mutex>

#include <rex/input/flags.h>
#include <rex/input/touch/touch_input_driver.h>
#include <rex/logging.h>

namespace rex::input::touch {

namespace {

// One device, so its handle is a constant ('TOUC').
constexpr rex::input::DeviceId kTouchDevice = static_cast<rex::input::DeviceId>(0x544F5543);

struct PadSnapshot {
  uint16_t buttons = 0;
  uint8_t left_trigger = 0;
  uint8_t right_trigger = 0;
  int16_t thumb_lx = 0;
  int16_t thumb_ly = 0;
  int16_t thumb_rx = 0;
  int16_t thumb_ry = 0;

  bool operator==(const PadSnapshot& o) const { return std::memcmp(this, &o, sizeof(*this)) == 0; }
};

std::mutex g_mutex;
PadSnapshot g_snapshot;
uint32_t g_packet_number = 1;  // XInput starts at 1

}  // namespace

void SetPadState(uint16_t buttons, uint8_t left_trigger, uint8_t right_trigger, int16_t thumb_lx,
                 int16_t thumb_ly, int16_t thumb_rx, int16_t thumb_ry) {
  PadSnapshot next;
  std::memset(&next, 0, sizeof(next));
  next.buttons = buttons;
  next.left_trigger = left_trigger;
  next.right_trigger = right_trigger;
  next.thumb_lx = thumb_lx;
  next.thumb_ly = thumb_ly;
  next.thumb_rx = thumb_rx;
  next.thumb_ry = thumb_ry;

  std::lock_guard<std::mutex> lock(g_mutex);
  if (!(next == g_snapshot)) {
    g_snapshot = next;
    g_packet_number++;
  }
}

void ClearPadState() {
  SetPadState(0, 0, 0, 0, 0, 0, 0);
}

TouchInputDriver::TouchInputDriver(rex::ui::Window* window, size_t window_z_order)
    : InputDriver(window, window_z_order) {}

TouchInputDriver::~TouchInputDriver() = default;

X_STATUS TouchInputDriver::Setup() {
  return X_STATUS_SUCCESS;
}

void TouchInputDriver::EnumerateDevices(std::vector<DeviceInfo>& out) {
  DeviceInfo info;
  info.id = kTouchDevice;
  info.name = "Touch Controls";
  // Synthetic: routed to guest user 0 and merged with a physical pad if present.
  info.synthetic = true;
  out.push_back(info);
}

X_RESULT TouchInputDriver::GetDeviceCapabilities(DeviceId id, uint32_t flags,
                                                 X_INPUT_CAPABILITIES* out_caps) {
  if (id != kTouchDevice) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  if (out_caps) {
    std::memset(out_caps, 0, sizeof(*out_caps));
    out_caps->type = 0x01;      // XINPUT_DEVTYPE_GAMEPAD
    out_caps->sub_type = 0x01;  // XINPUT_DEVSUBTYPE_GAMEPAD
    out_caps->flags = 0;
    out_caps->gamepad.buttons = 0xF3FF;
    out_caps->gamepad.left_trigger = 0xFF;
    out_caps->gamepad.right_trigger = 0xFF;
    out_caps->gamepad.thumb_lx = static_cast<int16_t>(0xFFFFu);
    out_caps->gamepad.thumb_ly = static_cast<int16_t>(0xFFFFu);
    out_caps->gamepad.thumb_rx = static_cast<int16_t>(0xFFFFu);
    out_caps->gamepad.thumb_ry = static_cast<int16_t>(0xFFFFu);
    out_caps->vibration.left_motor_speed = 0xFFFFu;
    out_caps->vibration.right_motor_speed = 0xFFFFu;
  }
  return X_ERROR_SUCCESS;
}

X_RESULT TouchInputDriver::GetDeviceState(DeviceId id, X_INPUT_STATE* out_state) {
  if (id != kTouchDevice) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  if (!out_state) {
    return X_ERROR_SUCCESS;
  }

  PadSnapshot snap;
  uint32_t packet;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    snap = g_snapshot;
    packet = g_packet_number;
  }

  std::memset(out_state, 0, sizeof(*out_state));
  out_state->packet_number = packet;
  // Untouched pad while the window is inactive (dialogs, app in background).
  if (is_active()) {
    out_state->gamepad.buttons = snap.buttons;
    out_state->gamepad.left_trigger = snap.left_trigger;
    out_state->gamepad.right_trigger = snap.right_trigger;
    out_state->gamepad.thumb_lx = snap.thumb_lx;
    out_state->gamepad.thumb_ly = snap.thumb_ly;
    out_state->gamepad.thumb_rx = snap.thumb_rx;
    out_state->gamepad.thumb_ry = snap.thumb_ry;
  }
  return X_ERROR_SUCCESS;
}

X_RESULT TouchInputDriver::SetDeviceVibration(DeviceId id, X_INPUT_VIBRATION* vibration) {
  if (id != kTouchDevice) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  // Accepted and ignored (synthetic devices never decide rumble results).
  return X_ERROR_SUCCESS;
}

X_RESULT TouchInputDriver::GetDeviceKeystroke(DeviceId id, uint32_t flags,
                                              X_INPUT_KEYSTROKE* out_keystroke) {
  if (id != kTouchDevice) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  return X_ERROR_EMPTY;
}

}  // namespace rex::input::touch
