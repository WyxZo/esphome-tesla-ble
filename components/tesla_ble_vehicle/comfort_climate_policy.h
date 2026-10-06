#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

namespace esphome {
namespace tesla_ble_vehicle {

struct PendingComfortActions {
  std::optional<size_t> front_seat_mode;
  bool steering_heat{false};
};

// A command acknowledgement alone does not mean HVAC has finished starting.
// Hold the latest accessory choices until a subsequent vehicle state says ON.
class ComfortClimatePolicy {
 public:
  bool request_front_seats(size_t mode) {
    pending_.front_seat_mode = mode == 0 ? std::nullopt : std::optional<size_t>(mode);
    return update_waiting_();
  }

  bool request_steering_heat(bool enable) {
    pending_.steering_heat = enable;
    return update_waiting_();
  }

  bool acknowledge(uint32_t generation, bool succeeded) {
    if (!is_current(generation)) return false;
    if (!succeeded) {
      cancel();
      return false;
    }
    acknowledged_ = true;
    return true;
  }

  std::optional<PendingComfortActions> on_climate_state(bool on) {
    if (!waiting_ || !acknowledged_ || !on) return std::nullopt;
    auto actions = pending_;
    cancel();
    return actions;
  }

  void cancel() {
    pending_ = {};
    waiting_ = false;
    acknowledged_ = false;
    ++generation_;
  }

  bool waiting() const { return waiting_; }
  uint32_t generation() const { return generation_; }
  bool is_current(uint32_t generation) const { return waiting_ && generation == generation_; }

 private:
  bool update_waiting_() {
    if (!pending_.front_seat_mode && !pending_.steering_heat) {
      cancel();
      return false;
    }
    if (waiting_) return false;
    waiting_ = true;
    acknowledged_ = false;
    ++generation_;
    return true;
  }

  PendingComfortActions pending_;
  bool waiting_{false};
  bool acknowledged_{false};
  uint32_t generation_{0};
};

}  // namespace tesla_ble_vehicle
}  // namespace esphome
