#pragma once

namespace PrimeStage {

class FrameLifecycle {
public:
  bool rebuildPending() const { return rebuildPending_; }
  bool layoutPending() const { return layoutPending_; }
  bool framePending() const { return framePending_; }

  void requestRebuild() {
    rebuildPending_ = true;
    layoutPending_ = true;
    framePending_ = true;
  }

  void requestLayout() {
    layoutPending_ = true;
    framePending_ = true;
  }

  void requestFrame() { framePending_ = true; }

  void markRebuildComplete() {
    rebuildPending_ = false;
    layoutPending_ = true;
    framePending_ = true;
  }

  void markLayoutComplete() { layoutPending_ = false; }

  void markFramePresented() { framePending_ = false; }

  template <typename Fn>
  bool runRebuildIfNeeded(Fn&& rebuildFn) {
    if (!rebuildPending_) {
      return false;
    }
    rebuildFn();
    markRebuildComplete();
    return true;
  }

  template <typename Fn>
  bool runLayoutIfNeeded(Fn&& layoutFn) {
    if (!layoutPending_) {
      return false;
    }
    layoutFn();
    markLayoutComplete();
    return true;
  }

private:
  bool rebuildPending_ = true;
  bool layoutPending_ = true;
  bool framePending_ = true;
};

} // namespace PrimeStage
