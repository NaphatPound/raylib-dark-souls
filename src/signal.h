#pragma once
// Minimal signal/slot, mirroring Godot's `signal` + `connect`/`emit` so the
// gameplay→UI→audio decoupling from the original ports over directly.
#include <functional>
#include <vector>

template <typename... A>
class Signal {
public:
    using Fn = std::function<void(A...)>;
    void connect(Fn f) { slots_.push_back(std::move(f)); }
    void emit(A... a) { for (auto& f : slots_) f(a...); }
    void clear() { slots_.clear(); }
private:
    std::vector<Fn> slots_;
};
