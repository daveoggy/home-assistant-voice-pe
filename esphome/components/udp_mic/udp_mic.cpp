#include "udp_mic.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <lwip/inet.h>
#include <cerrno>
#include <cstring>

namespace esphome {
namespace udp_mic {

static const char *const TAG = "udp_mic";

void UdpMic::setup() {
  this->sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
  std::memset(&this->dest_, 0, sizeof(this->dest_));
  this->dest_.sin_family = AF_INET;
  this->dest_.sin_port = htons(this->port_);
  this->dest_.sin_addr.s_addr = inet_addr(this->ip_.c_str());
  ESP_LOGI(TAG, "setup: mic=%p sock=%d -> %s:%u (src_ch=%u shift=%u)", (void *) this->mic_, this->sock_,
           this->ip_.c_str(), this->port_, this->channels_, this->bits_shift_);
  if (this->sock_ < 0 || this->mic_ == nullptr) {
    ESP_LOGE(TAG, "setup FAILED (sock=%d mic=%p)", this->sock_, (void *) this->mic_);
    this->mark_failed();
    return;
  }
  this->mic_->add_data_callback([this](const std::vector<uint8_t> &data) { this->on_data_(data); });
  this->mic_->start();
  ESP_LOGI(TAG, "callback registered + mic started; is_running=%d", this->mic_->is_running());
}

void UdpMic::loop() {
  uint32_t now = millis();
  if (now - this->last_log_ >= 2000) {
    this->last_log_ = now;
    ESP_LOGI(TAG, "stats: calls=%u sent=%u errs=%u in=%uB out=%uB mic_running=%d", this->calls_, this->sent_,
             this->errs_, this->in_bytes_, this->out_bytes_, this->mic_ ? this->mic_->is_running() : -1);
  }
}

void UdpMic::on_data_(const std::vector<uint8_t> &data) {
  this->calls_++;
  this->in_bytes_ += data.size();
  if (data.size() < 4 || this->sock_ < 0)
    return;
  const int32_t *in = reinterpret_cast<const int32_t *>(data.data());
  size_t total = data.size() / 4;
  size_t frames = total / this->channels_;
  this->out_.resize(frames * 2);
  int16_t *o = reinterpret_cast<int16_t *>(this->out_.data());
  for (size_t f = 0; f < frames; f++) {
    int32_t s = in[f * this->channels_];
    o[f] = static_cast<int16_t>(s >> this->bits_shift_);
  }
  size_t remaining = this->out_.size(), off = 0;
  const size_t MAXP = 1024;
  while (remaining > 0) {
    size_t chunk = remaining < MAXP ? remaining : MAXP;
    int ret = ::sendto(this->sock_, this->out_.data() + off, chunk, 0,
                       reinterpret_cast<struct sockaddr *>(&this->dest_), sizeof(this->dest_));
    if (ret < 0) {
      this->errs_++;
      if (this->errs_ <= 3)
        ESP_LOGW(TAG, "sendto errno=%d", errno);
    } else {
      this->sent_++;
      this->out_bytes_ += ret;
    }
    off += chunk;
    remaining -= chunk;
  }
}

}  // namespace udp_mic
}  // namespace esphome
