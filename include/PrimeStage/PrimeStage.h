#pragma once

#include "PrimeStage/Ui.h"

#include <cstdint>
#include <string_view>

namespace PrimeStage {

struct Version {
  uint32_t major = 0;
  uint32_t minor = 1;
  uint32_t patch = 0;
};

Version getVersion();
std::string_view getVersionString();

} // namespace PrimeStage
