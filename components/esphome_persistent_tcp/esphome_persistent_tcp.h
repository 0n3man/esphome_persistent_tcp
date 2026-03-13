#pragma once
#include "esphome.h"
#include "AsyncTCP.h"
#include <functional>

namespace esphome {
namespace esphome_persistent_tcp {

class PersistentTCPClient : public Component {
 public:
  void set_host(const char* host) { host_ = host; }
  void set_port(uint16_t port) { port_ = port; }

  // Register a callback from YAML lambda
  void on_message(std::function<void(const std::string&)> callback) {
    message_callback_ = callback;
  }

  void setup() override {
    ESP_LOGD("tcp", "TCP Client setup");
    client_ = new AsyncClient();

    client_->onConnect([](void* arg, AsyncClient* c) {
      auto* self = (PersistentTCPClient*)arg;
      ESP_LOGI("tcp", "Connected to %s:%d", self->host_, self->port_);
      self->connected_ = true;
    }, this);

    client_->onDisconnect([](void* arg, AsyncClient* c) {
      auto* self = (PersistentTCPClient*)arg;
      ESP_LOGW("tcp", "Disconnected, will retry...");
      self->connected_ = false;
      self->last_attempt_ = 0;
    }, this);

    client_->onError([](void* arg, AsyncClient* c, int8_t err) {
      auto* self = (PersistentTCPClient*)arg;
      ESP_LOGE("tcp", "Error: %s", c->errorToString(err));
      self->connected_ = false;
    }, this);

    client_->onData([](void* arg, AsyncClient* c, void* data, size_t len) {
      auto* self = (PersistentTCPClient*)arg;
      self->rx_buf_.append((char*)data, len);
      size_t pos;
      while ((pos = self->rx_buf_.find('\n')) != std::string::npos) {
        std::string line = self->rx_buf_.substr(0, pos);
        self->rx_buf_.erase(0, pos + 1);
        self->handle_message(line);
      }
    }, this);

    connect();
  }

  void loop() override {
    if (!connected_) {
      uint32_t now = millis();
      if (now - last_attempt_ > 5000) {
        last_attempt_ = now;
        ESP_LOGW("tcp", "Reconnecting...");
        connect();
      }
    }
  }

  void connect() {
    if (!client_->connecting()) {
      if (!client_->connect(host_, port_)) {
        ESP_LOGW("tcp", "Connection failed, will retry...");
      }
    }
  }

  void send_message(const std::string& message) {
    if (connected_ && client_->canSend()) {
      client_->write(message.c_str());
      ESP_LOGD("tcp", "Sent: %s", message.c_str());
    } else {
      ESP_LOGW("tcp", "Cannot send, not connected");
    }
  }

  void handle_message(const std::string& msg) {
    ESP_LOGD("tcp", "Received: %s", msg.c_str());
    if (message_callback_) {
      message_callback_(msg);
    }
  }

  void on_shutdown() override {
    if (client_) {
      client_->close();
    }
  }

  float get_setup_priority() const override {
    return setup_priority::AFTER_WIFI;
  }

 protected:
  AsyncClient* client_{nullptr};
  const char* host_;
  uint16_t port_;
  bool connected_{false};
  uint32_t last_attempt_{0};
  std::string rx_buf_;
  std::function<void(const std::string&)> message_callback_{nullptr};
};

}  // namespace esphome_persistent_tcp
}  // namespace esphome

