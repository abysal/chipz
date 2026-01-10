#pragma once
#include "linear_scan_allocator.hpp"

namespace cip::comp {

    class Emitter {
    public:
        explicit Emitter(LinearScanAllocator allocator) : m_allocator(std::move(allocator)) {}

        JITFn emit(const hlir::HLIR& ir);

    private:
        LinearScanAllocator m_allocator{};
    };

} // namespace cip::comp
