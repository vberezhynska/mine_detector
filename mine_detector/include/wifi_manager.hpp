#pragma once

#include <memory>
#include <string>

namespace networking {

class WifiManager {
public:
    WifiManager(std::string ssid, std::string password);
    ~WifiManager();

    WifiManager(const WifiManager&) = delete;
    WifiManager& operator=(const WifiManager&) = delete;

    /**
     * @brief Connects to the AP and blocks until IP is acquired or connection fails.
     * @return true on successful connection, false on failure/timeout.
     */
    bool connect();

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace networking