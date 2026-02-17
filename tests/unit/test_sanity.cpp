#include "PrimeStage/PrimeStage.h"

#include "third_party/doctest.h"

TEST_CASE("PrimeStage exposes version") {
  PrimeStage::Version version = PrimeStage::getVersion();
  CHECK(version.major == 0);
  CHECK(version.minor == 1);
  CHECK(version.patch == 0);
  CHECK(PrimeStage::getVersionString() == "0.1.0");
}
