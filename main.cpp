#include "core/host.hpp"

// If this fails, this means your target is not little endian! This is not supported
static_assert(std::endian::native == std::endian::little);

int main() {
    cip::Host host;
    return 0;
}