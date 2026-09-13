#pragma once

#include <memory>
#include <string>
#include <cstdint>

#include "interfaces/ISocket.hpp"

namespace networking {
class HttpClient : public ISocket {
    public:
        explicit HttpClient(std::string baseUrl, uint16_t port = 443);
        ~HttpClient() override;

        HttpClient(const HttpClient&) = delete;
        HttpClient& operator=(const HttpClient&) = delete;
        HttpClient(HttpClient&&) noexcept;
        HttpClient& operator=(HttpClient&&) noexcept;

        bool init() override;
        bool sendMineAlert(int32_t latitude, int32_t longitude, int8_t gp_type);
    private:
        struct Impl;
        std::unique_ptr<Impl> pImpl;
};
} //namespace networking