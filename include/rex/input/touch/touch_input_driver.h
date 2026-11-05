#pragma once
/**
 * @file        input/touch/touch_input_driver.h
 * @brief       Host-fed virtual pad for on-screen touch controls (Android).
 *
 * The platform overlay (a Java view on Android) calls SetPadState() whenever the
 * touch controls change. The driver exposes that state as a synthetic Xbox 360
 * pad. Synthetic devices are assigned to guest user 0 and merged with any
 * physical pad, so a Bluetooth/USB controller and the touch overlay coexist.
 *
 * Button bits and axis ranges are the raw XInput ones:
 *   buttons: DPAD_UP 0x0001, DOWN 0x0002, LEFT 0x0004, RIGHT 0x0008, START 0x0010,
 *            BACK 0x0020, LTHUMB 0x0040, RTHUMB 0x0080, LB 0x0100, RB 0x0200,
 *            A 0x1000, B 0x2000, X 0x4000, Y 0x8000
 *   triggers: 0..255, sticks: -32768..32767 (Y up is positive)
 */

#include <cstdint>

#include <rex/input/input_driver.h>

namespace rex::input::touch {

// Thread-safe; callable from any thread (JNI/UI thread).
void SetPadState(uint16_t buttons, uint8_t left_trigger, uint8_t right_trigger, int16_t thumb_lx,
                 int16_t thumb_ly, int16_t thumb_rx, int16_t thumb_ry);

// Releases everything (e.g. when the overlay is hidden or the app is paused).
void ClearPadState();

class TouchInputDriver final : public InputDriver {
 public:
  explicit TouchInputDriver(rex::ui::Window* window, size_t window_z_order);
  ~TouchInputDriver() override;

  X_STATUS Setup() override;

  void EnumerateDevices(std::vector<DeviceInfo>& out) override;
  X_RESULT GetDeviceState(DeviceId id, X_INPUT_STATE* out_state) override;
  X_RESULT GetDeviceCapabilities(DeviceId id, uint32_t flags,
                                 X_INPUT_CAPABILITIES* out_caps) override;
  X_RESULT SetDeviceVibration(DeviceId id, X_INPUT_VIBRATION* vibration) override;
  X_RESULT GetDeviceKeystroke(DeviceId id, uint32_t flags,
                              X_INPUT_KEYSTROKE* out_keystroke) override;
};

}  // namespace rex::input::touch
