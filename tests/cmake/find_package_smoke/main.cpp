#include "PrimeStage/AppRuntime.h"

int main() {
  PrimeStage::FrameLifecycle lifecycle;
  return lifecycle.framePending() ? 0 : 1;
}
