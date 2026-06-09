#include "udp_mic.h"
#include "esphome/core/log.h"
#include <lwip/inet.h>
#include <cstring>

namespace esphome {
namespace udp_mic {

static const char *const TAG = "udp_mic";

void UdpMic::setup() {
  this->sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (this->sock_ < 0) {
    ESP_LOGE(TAG, "socket() failed");
    this->mark_failed();
    return;
  }
  std::memset(&this->dest_, 0, sizeof(this->dest_));
  this->dest_.sin_family = AF_INET;
  this->dest_.sin_port = htons(this->port_);
  this->dest_.sin_addr.s_addr = inet_addr(this->ip_.c_str());

  if (this->mic_ == nullptr) {
    ESP_LOGE(TAG, "no microphone");
    this->mark_failed();
    return;
  }
  this->mic_->add_data_callback([this](const std::vector<uint8_t> &data) { this->on_data_(data); });
  this->mic_->start();
  ESP_LOGI(TAG, "udp_mic streaming -> %s:%u (src_ch=%u shift=%u)", this->ip_.c_str(), this->port_,
           this->channels_, this->bits_shift_);
}

void UdpMic::on_data_(const std::vector<uint8_t> &data) {
  if (data.size() < 4 || this->sock_ < 0)
    return;
  // source: interleaved int32 samples, `channels_` per frame -> take channel 0, emit int16 mono
  const int32_t *in = reinterpret_cast<const int32_t *>(data.data());
  size_t total = data.size() / 4;
  size_t frames = total / this->channels_;
  this->out_.resize(frames * 2);
  int16_t *o = reinterpret_cast<int16_t *>(this->out_.data());
  for (size_t f = 0; f < frames; f++) {
    int32_t s = in[f * this->channels_];
    o[f] = static_cast<int16_t>(s >> this->bits_shift_);
  }
  // send in <=1024-byte datagrams (512 samples = 32ms)
  size_t remaining = this->out_.size(), off = 0;
  const size_t MAXP = 1024;
  while (remaining > 0) {
    size_t chunk = remaining < MAXP ? remaining : MAXP;
    ::sendto(this->sock_, this->out_.data() + off, chunk, 0, reinterpret_cast<struct sockaddr *>(&this->dest_),
             sizeof(this->dest_));
    off += chunk;
    remaining -= chunk;
  }
}

}  // namespace udp_mic
}  // namespace esphome
