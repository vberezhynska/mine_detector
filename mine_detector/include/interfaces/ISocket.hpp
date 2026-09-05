#pragma once

namespace networking {
    class ISocket {
    public:
        virtual ~ISocket() = default;
        virtual bool initSocket() = 0;
    };
} //namespace networking