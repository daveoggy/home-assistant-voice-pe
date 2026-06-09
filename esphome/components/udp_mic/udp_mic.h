#pragma once
#include "esphome/core/component.h"
#include "esphome/components/microphone/microphone.h"
#include <lwip/sockets.h>
#include <string>
#include <vector>
#include <cstdint>

namespace esphome {
namespace udp_mic {

class UdpMic : public Component {
 public:
  void set_microphone(microphone::Microphone *mic) { mic_ = mic; }
  void set_target(const std::string &ip, uint16_t port) { ip_ = ip; port_ = port; }
  void set_channels(uint8_t ch) { channels_ = ch; }
  void set_bits_shift(uint8_t s) { bits_shift_ = s; }
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }

 protected:
  void on_data_(const std::vector<uint8_t> &data);
  microphone::Microphone *mic_{nullptr};
  std::string ip_;
  uint16_t port_{10500};
  uint8_t channels_{2};
  uint8_t bits_shift_{16};
  int sock_{-1};
  struct sockaddr_in dest_ {};
  std::vector<uint8_t> out_;
  // diagnostics
  uint32_t calls_{0}, sent_{0}, errs_{0};
  uint32_t in_bytes_{0}, out_bytes_{0};
  uint32_t last_log_{0};
};

}  // namespace udp_mic
}  // namespace esphome
