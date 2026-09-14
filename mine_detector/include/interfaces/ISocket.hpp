#pragma once

namespace networking {
    class ISocket {
    public:
        virtual ~ISocket() = default;
        virtual bool init() = 0;
    };
} //namespace networking