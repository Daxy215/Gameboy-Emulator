﻿#pragma once

#include <cstdint>
#include <functional>
#include <optional>

using SerialCallback = std::function<std::optional<uint8_t>(uint8_t)>;

class Serial {
public:
    uint8_t fetch8(uint16_t address);
    void write8(uint16_t address, uint8_t data);

    void set_callback(SerialCallback cb) {
        callback = cb;
    }

    void unset_callback() {
        callback = noop;
    }

    static std::optional<uint8_t> noop(uint8_t) {
        return std::nullopt;
    }

public:
    uint8_t interrupt = 0;
    
private:
    uint8_t transferData = 0;
    uint8_t transferControl = 0;
    
    SerialCallback callback;
};
