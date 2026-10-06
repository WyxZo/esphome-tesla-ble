#include "test_helper.h"
#include "comfort_climate_policy.h"

using namespace esphome::tesla_ble_vehicle;

static void test_ack_and_on_state_are_both_required() {
  ComfortClimatePolicy policy;
  CHECK(policy.request_front_seats(2));
  CHECK(!policy.on_climate_state(true));
  CHECK(policy.acknowledge(policy.generation(), true));
  CHECK(!policy.on_climate_state(false));
  const auto actions = policy.on_climate_state(true);
  CHECK(actions && actions->front_seat_mode == 2);
  CHECK(!actions->steering_heat);
  CHECK(!policy.waiting());
  CHECK(!policy.on_climate_state(true));  // No duplicate application.
}

static void test_latest_seat_choice_and_shared_climate_start() {
  ComfortClimatePolicy policy;
  CHECK(policy.request_front_seats(1));
  const auto generation = policy.generation();
  CHECK(!policy.request_front_seats(6));  // Heat -> cool while starting.
  CHECK(!policy.request_steering_heat(true));
  CHECK(policy.generation() == generation);
  CHECK(policy.acknowledge(generation, true));
  const auto actions = policy.on_climate_state(true);
  CHECK(actions && actions->front_seat_mode == 6 && actions->steering_heat);
}

static void test_off_does_not_start_hvac_or_cancel_other_accessory() {
  ComfortClimatePolicy policy;
  CHECK(!policy.request_front_seats(0));
  CHECK(!policy.request_steering_heat(false));
  CHECK(!policy.waiting());
  CHECK(policy.request_front_seats(3));
  CHECK(!policy.request_steering_heat(true));
  CHECK(!policy.request_front_seats(0));
  CHECK(policy.waiting());
  CHECK(policy.acknowledge(policy.generation(), true));
  const auto actions = policy.on_climate_state(true);
  CHECK(actions && !actions->front_seat_mode && actions->steering_heat);

  CHECK(policy.request_front_seats(3));
  CHECK(!policy.request_steering_heat(true));
  CHECK(!policy.request_steering_heat(false));
  CHECK(policy.acknowledge(policy.generation(), true));
  const auto seats = policy.on_climate_state(true);
  CHECK(seats && seats->front_seat_mode == 3 && !seats->steering_heat);
}

static void test_failure_cancel_and_late_callbacks() {
  ComfortClimatePolicy policy;
  CHECK(policy.request_steering_heat(true));
  const auto failed = policy.generation();
  CHECK(!policy.acknowledge(failed, false));
  CHECK(!policy.on_climate_state(true));
  CHECK(!policy.waiting());

  CHECK(policy.request_front_seats(1));
  const auto cancelled = policy.generation();
  policy.cancel();  // Explicit climate OFF, disconnect, or startup timeout.
  CHECK(!policy.acknowledge(cancelled, true));
  CHECK(!policy.on_climate_state(true));
  CHECK(policy.request_front_seats(2));
  CHECK(!policy.acknowledge(cancelled, true));
  CHECK(!policy.acknowledge(failed, true));
  CHECK(!policy.on_climate_state(true));
  CHECK(policy.acknowledge(policy.generation(), true));
  CHECK(policy.on_climate_state(true)->front_seat_mode == 2);

  CHECK(policy.request_steering_heat(true));
  const auto stopped = policy.generation();
  CHECK(!policy.request_steering_heat(false));
  CHECK(!policy.waiting());
  CHECK(!policy.acknowledge(stopped, true));
  CHECK(!policy.on_climate_state(true));
}

int main() {
  test_ack_and_on_state_are_both_required();
  test_latest_seat_choice_and_shared_climate_start();
  test_off_does_not_start_hvac_or_cancel_other_accessory();
  test_failure_cancel_and_late_callbacks();
  return test_summary();
}
