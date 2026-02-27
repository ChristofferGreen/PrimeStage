#include "PrimeStage/PrimeStage.h"

#include <iostream>

int main() {
  PrimeStage::Version version = PrimeStage::getVersion();
  std::cout << "PrimeStage " << PrimeStage::getVersionString()
            << " (" << version.major << "." << version.minor << "." << version.patch << ")\n";
  return 0;
}
