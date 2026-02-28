#include "PrimeStage/Render.h"
#include "PrimeStage/Ui.h"

#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace {

PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame, float width, float height) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeFrame::Node* rootNode = frame.getNode(rootId);
  REQUIRE(rootNode != nullptr);
  rootNode->layout = PrimeFrame::LayoutType::Overlay;
  rootNode->sizeHint.width.preferred = width;
  rootNode->sizeHint.height.preferred = height;
  return PrimeStage::UiNode(frame, rootId, true);
}

PrimeFrame::LayoutOutput layoutFrame(PrimeFrame::Frame& frame, float width, float height) {
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = width;
  options.rootHeight = height;
  engine.layout(frame, layout, options);
  return layout;
}

PrimeFrame::Frame makeRenderableFrame(float width, float height) {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, width, height);
  PrimeStage::PanelSpec panel;
  panel.rectStyle = 1u;
  panel.size.stretchX = 1.0f;
  panel.size.stretchY = 1.0f;
  root.createPanel(panel);
  return frame;
}

void configureThemeForSingleRect(PrimeFrame::Frame& frame,
                                 PrimeFrame::Color baseFill,
                                 PrimeFrame::Color accentColor) {
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  REQUIRE(theme != nullptr);
  theme->palette.assign(16u, PrimeFrame::Color{});
  theme->palette[2] = baseFill;
  theme->palette[8] = accentColor;
  theme->rectStyles.assign(4u, PrimeFrame::RectStyle{});
  theme->rectStyles[1].fill = 2u;
}

size_t countNonZeroAlpha(std::vector<uint8_t> const& rgba) {
  size_t count = 0u;
  for (size_t index = 3u; index < rgba.size(); index += 4u) {
    if (rgba[index] != 0u) {
      count += 1u;
    }
  }
  return count;
}

std::filesystem::path makeTempPngPath(std::string_view tag) {
  std::error_code error;
  std::filesystem::path directory = std::filesystem::temp_directory_path(error);
  if (error) {
    directory = std::filesystem::current_path(error);
  }
  auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  return directory / ("primestage_render_" + std::string(tag) + "_" + std::to_string(stamp) + ".png");
}

} // namespace

TEST_CASE("PrimeStage render target diagnostics expose actionable status") {
  PrimeFrame::Frame frame = makeRenderableFrame(64.0f, 32.0f);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 64.0f, 32.0f);

  PrimeStage::RenderTarget invalidSize;
  invalidSize.width = 0;
  invalidSize.height = 32;
  invalidSize.stride = 0;
  PrimeStage::RenderStatus invalidSizeStatus =
      PrimeStage::renderFrameToTarget(frame, layout, invalidSize, PrimeStage::RenderOptions{});

  PrimeStage::RenderTarget invalidStride;
  std::vector<uint8_t> stridePixels(64u * 32u * 4u, 0u);
  invalidStride.pixels = std::span<uint8_t>(stridePixels);
  invalidStride.width = 64u;
  invalidStride.height = 32u;
  invalidStride.stride = 64u;
  PrimeStage::RenderStatus invalidStrideStatus =
      PrimeStage::renderFrameToTarget(frame, layout, invalidStride, PrimeStage::RenderOptions{});

  PrimeStage::RenderTarget invalidBuffer;
  std::vector<uint8_t> shortPixels(8u, 0u);
  invalidBuffer.pixels = std::span<uint8_t>(shortPixels);
  invalidBuffer.width = 64u;
  invalidBuffer.height = 32u;
  invalidBuffer.stride = 256u;
  PrimeStage::RenderStatus invalidBufferStatus =
      PrimeStage::renderFrameToTarget(frame, layout, invalidBuffer, PrimeStage::RenderOptions{});

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  CHECK(invalidSizeStatus.code == PrimeStage::RenderStatusCode::InvalidTargetDimensions);
  CHECK(invalidStrideStatus.code == PrimeStage::RenderStatusCode::InvalidTargetStride);
  CHECK(invalidStrideStatus.requiredStride == 256u);
  CHECK(invalidBufferStatus.code == PrimeStage::RenderStatusCode::InvalidTargetBuffer);
  CHECK(invalidBufferStatus.requiredStride == 256u);
  CHECK(PrimeStage::renderStatusMessage(invalidBufferStatus.code) == "Render target pixel buffer is empty or undersized");
#else
  CHECK(invalidSizeStatus.code == PrimeStage::RenderStatusCode::BackendUnavailable);
  CHECK(invalidStrideStatus.code == PrimeStage::RenderStatusCode::BackendUnavailable);
  CHECK(invalidBufferStatus.code == PrimeStage::RenderStatusCode::BackendUnavailable);
  CHECK(PrimeStage::renderStatusMessage(invalidBufferStatus.code) ==
        "Render backend unavailable (PrimeManifest disabled)");
#endif
}

TEST_CASE("PrimeStage render target diagnostics include invalid-size payload details") {
  PrimeFrame::Frame frame = makeRenderableFrame(64.0f, 32.0f);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 64.0f, 32.0f);

  PrimeStage::RenderTarget invalidSize;
  invalidSize.width = 0u;
  invalidSize.height = 32u;
  invalidSize.stride = 0u;
  PrimeStage::RenderStatus status =
      PrimeStage::renderFrameToTarget(frame, layout, invalidSize, PrimeStage::RenderOptions{});

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  CHECK_FALSE(status.ok());
  CHECK(status.code == PrimeStage::RenderStatusCode::InvalidTargetDimensions);
  CHECK(status.targetWidth == 0u);
  CHECK(status.targetHeight == 32u);
  CHECK(status.targetStride == 0u);
  CHECK(status.requiredStride == 0u);
  CHECK(status.detail == "target width/height must be greater than zero");
#else
  CHECK(status.code == PrimeStage::RenderStatusCode::BackendUnavailable);
#endif
}

TEST_CASE("PrimeStage render target diagnostics include height-zero payload details") {
  PrimeFrame::Frame frame = makeRenderableFrame(64.0f, 32.0f);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 64.0f, 32.0f);

  PrimeStage::RenderTarget invalidSize;
  invalidSize.width = 64u;
  invalidSize.height = 0u;
  invalidSize.stride = 256u;
  PrimeStage::RenderStatus status =
      PrimeStage::renderFrameToTarget(frame, layout, invalidSize, PrimeStage::RenderOptions{});

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  CHECK_FALSE(status.ok());
  CHECK(status.code == PrimeStage::RenderStatusCode::InvalidTargetDimensions);
  CHECK(status.targetWidth == 64u);
  CHECK(status.targetHeight == 0u);
  CHECK(status.targetStride == 256u);
  CHECK(status.requiredStride == 256u);
  CHECK(status.detail == "target width/height must be greater than zero");
#else
  CHECK(status.code == PrimeStage::RenderStatusCode::BackendUnavailable);
#endif
}

TEST_CASE("PrimeStage render target diagnostics include stride/buffer payload details") {
  PrimeFrame::Frame frame = makeRenderableFrame(64.0f, 32.0f);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 64.0f, 32.0f);

  PrimeStage::RenderTarget invalidStride;
  std::vector<uint8_t> stridePixels(64u * 32u * 4u, 0u);
  invalidStride.pixels = std::span<uint8_t>(stridePixels);
  invalidStride.width = 64u;
  invalidStride.height = 32u;
  invalidStride.stride = 64u;
  PrimeStage::RenderStatus invalidStrideStatus =
      PrimeStage::renderFrameToTarget(frame, layout, invalidStride, PrimeStage::RenderOptions{});

  PrimeStage::RenderTarget invalidBuffer;
  std::vector<uint8_t> shortPixels(8u, 0u);
  invalidBuffer.pixels = std::span<uint8_t>(shortPixels);
  invalidBuffer.width = 64u;
  invalidBuffer.height = 32u;
  invalidBuffer.stride = 256u;
  PrimeStage::RenderStatus invalidBufferStatus =
      PrimeStage::renderFrameToTarget(frame, layout, invalidBuffer, PrimeStage::RenderOptions{});

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  CHECK(invalidStrideStatus.code == PrimeStage::RenderStatusCode::InvalidTargetStride);
  CHECK(invalidStrideStatus.targetWidth == 64u);
  CHECK(invalidStrideStatus.targetHeight == 32u);
  CHECK(invalidStrideStatus.targetStride == 64u);
  CHECK(invalidStrideStatus.requiredStride == 256u);
  CHECK(invalidStrideStatus.detail == "target stride must be at least width * 4 bytes");

  CHECK(invalidBufferStatus.code == PrimeStage::RenderStatusCode::InvalidTargetBuffer);
  CHECK(invalidBufferStatus.targetWidth == 64u);
  CHECK(invalidBufferStatus.targetHeight == 32u);
  CHECK(invalidBufferStatus.targetStride == 256u);
  CHECK(invalidBufferStatus.requiredStride == 256u);
  CHECK(invalidBufferStatus.detail ==
        "target pixel span is smaller than required stride * height bytes");
#else
  CHECK(invalidStrideStatus.code == PrimeStage::RenderStatusCode::BackendUnavailable);
  CHECK(invalidBufferStatus.code == PrimeStage::RenderStatusCode::BackendUnavailable);
#endif
}

TEST_CASE("PrimeStage render target diagnostics flag empty pixel spans") {
  PrimeFrame::Frame frame = makeRenderableFrame(64.0f, 32.0f);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 64.0f, 32.0f);

  PrimeStage::RenderTarget emptyBuffer;
  emptyBuffer.width = 64u;
  emptyBuffer.height = 32u;
  emptyBuffer.stride = 256u;
  PrimeStage::RenderStatus status =
      PrimeStage::renderFrameToTarget(frame, layout, emptyBuffer, PrimeStage::RenderOptions{});

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  CHECK(status.code == PrimeStage::RenderStatusCode::InvalidTargetBuffer);
  CHECK(status.targetWidth == 64u);
  CHECK(status.targetHeight == 32u);
  CHECK(status.targetStride == 256u);
  CHECK(status.requiredStride == 256u);
  CHECK(status.detail == "target pixel span is smaller than required stride * height bytes");
#else
  CHECK(status.code == PrimeStage::RenderStatusCode::BackendUnavailable);
#endif
}

TEST_CASE("PrimeStage render status messages include zero-extent and unknown fallbacks") {
  CHECK(PrimeStage::renderStatusMessage(PrimeStage::RenderStatusCode::LayoutZeroExtent) ==
        "Layout produced zero-sized render bounds");
  CHECK(PrimeStage::renderStatusMessage(static_cast<PrimeStage::RenderStatusCode>(255u)) ==
        "Unknown render status");

  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 0.0f, 0.0f);
  PrimeStage::PanelSpec panel;
  panel.rectStyle = 1u;
  panel.size.stretchX = 1.0f;
  panel.size.stretchY = 1.0f;
  root.createPanel(panel);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 0.0f, 0.0f);

  PrimeStage::RenderStatus status =
      PrimeStage::renderFrameToPng(frame, layout, "unused.png", PrimeStage::RenderOptions{});

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  CHECK(status.code == PrimeStage::RenderStatusCode::LayoutZeroExtent);
  CHECK(status.targetWidth == 0u);
  CHECK(status.targetHeight == 0u);
#else
  CHECK(status.code == PrimeStage::RenderStatusCode::BackendUnavailable);
#endif
}

TEST_CASE("PrimeStage render status messages cover all documented status codes") {
  CHECK(PrimeStage::renderStatusMessage(PrimeStage::RenderStatusCode::Success) == "Success");
  CHECK(PrimeStage::renderStatusMessage(PrimeStage::RenderStatusCode::BackendUnavailable) ==
        "Render backend unavailable (PrimeManifest disabled)");
  CHECK(PrimeStage::renderStatusMessage(PrimeStage::RenderStatusCode::InvalidTargetDimensions) ==
        "Invalid render target dimensions");
  CHECK(PrimeStage::renderStatusMessage(PrimeStage::RenderStatusCode::InvalidTargetStride) ==
        "Render target stride is smaller than width * 4");
  CHECK(PrimeStage::renderStatusMessage(PrimeStage::RenderStatusCode::InvalidTargetBuffer) ==
        "Render target pixel buffer is empty or undersized");
  CHECK(PrimeStage::renderStatusMessage(PrimeStage::RenderStatusCode::LayoutHasNoRoots) ==
        "Frame has no root nodes");
  CHECK(PrimeStage::renderStatusMessage(PrimeStage::RenderStatusCode::LayoutMissingRootMetrics) ==
        "Layout output missing metrics for frame roots");
  CHECK(PrimeStage::renderStatusMessage(PrimeStage::RenderStatusCode::LayoutZeroExtent) ==
        "Layout produced zero-sized render bounds");
  CHECK(PrimeStage::renderStatusMessage(PrimeStage::RenderStatusCode::PngPathEmpty) ==
        "PNG output path is empty");
  CHECK(PrimeStage::renderStatusMessage(PrimeStage::RenderStatusCode::PngWriteFailed) ==
        "PNG write failed");
}

TEST_CASE("PrimeStage render status bool conversion mirrors success state") {
  PrimeStage::RenderStatus successStatus;
  successStatus.code = PrimeStage::RenderStatusCode::Success;
  CHECK(successStatus.ok());
  CHECK(static_cast<bool>(successStatus));

  PrimeStage::RenderStatus failureStatus;
  failureStatus.code = PrimeStage::RenderStatusCode::BackendUnavailable;
  CHECK_FALSE(failureStatus.ok());
  CHECK_FALSE(static_cast<bool>(failureStatus));
}

TEST_CASE("PrimeStage PNG diagnostics report layout and path failures") {
#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  {
    PrimeFrame::Frame frame;
    PrimeFrame::LayoutOutput layout;
    PrimeStage::RenderStatus status =
        PrimeStage::renderFrameToPng(frame, layout, "unused.png", PrimeStage::RenderOptions{});
    CHECK(status.code == PrimeStage::RenderStatusCode::LayoutHasNoRoots);
  }

  {
    PrimeFrame::Frame frame;
    PrimeStage::UiNode root = createRoot(frame, 80.0f, 40.0f);
    PrimeStage::PanelSpec panel;
    panel.rectStyle = 1u;
    panel.size.stretchX = 1.0f;
    panel.size.stretchY = 1.0f;
    root.createPanel(panel);

    PrimeFrame::LayoutOutput missingLayout;
    PrimeStage::RenderStatus status =
        PrimeStage::renderFrameToPng(frame, missingLayout, "unused.png", PrimeStage::RenderOptions{});
    CHECK(status.code == PrimeStage::RenderStatusCode::LayoutMissingRootMetrics);
  }

  {
    PrimeFrame::Frame frame;
    PrimeStage::RenderStatus status =
        PrimeStage::renderFrameToPng(frame, std::string_view{}, PrimeStage::RenderOptions{});
    CHECK(status.code == PrimeStage::RenderStatusCode::PngPathEmpty);
  }
#else
  PrimeFrame::Frame frame;
  PrimeFrame::LayoutOutput layout;
  PrimeStage::RenderStatus status =
      PrimeStage::renderFrameToPng(frame, layout, "unused.png", PrimeStage::RenderOptions{});
  CHECK(status.code == PrimeStage::RenderStatusCode::BackendUnavailable);
#endif
}

TEST_CASE("PrimeStage render status is successful for valid render targets") {
  PrimeFrame::Frame frame = makeRenderableFrame(96.0f, 64.0f);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 96.0f, 64.0f);

  std::vector<uint8_t> pixels(96u * 64u * 4u, 0u);
  PrimeStage::RenderTarget target;
  target.pixels = std::span<uint8_t>(pixels);
  target.width = 96u;
  target.height = 64u;
  target.stride = 96u * 4u;

  PrimeStage::RenderStatus status =
      PrimeStage::renderFrameToTarget(frame, layout, target, PrimeStage::RenderOptions{});

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  CHECK(status.ok());
  CHECK(status.code == PrimeStage::RenderStatusCode::Success);
  CHECK(status.targetWidth == 96u);
  CHECK(status.targetHeight == 64u);
  CHECK(PrimeStage::renderStatusMessage(status.code) == "Success");
#else
  CHECK_FALSE(status.ok());
  CHECK(status.code == PrimeStage::RenderStatusCode::BackendUnavailable);
#endif
}

TEST_CASE("PrimeStage render overload treats non-positive scale as 1x fallback") {
  PrimeFrame::Frame frame = makeRenderableFrame(96.0f, 64.0f);
  PrimeStage::RenderOptions options;

  std::vector<uint8_t> referencePixels(96u * 64u * 4u, 0u);
  PrimeStage::RenderTarget referenceTarget;
  referenceTarget.pixels = std::span<uint8_t>(referencePixels);
  referenceTarget.width = 96u;
  referenceTarget.height = 64u;
  referenceTarget.stride = 96u * 4u;
  referenceTarget.scale = 1.0f;

  std::vector<uint8_t> fallbackPixels(96u * 64u * 4u, 0u);
  PrimeStage::RenderTarget fallbackTarget;
  fallbackTarget.pixels = std::span<uint8_t>(fallbackPixels);
  fallbackTarget.width = 96u;
  fallbackTarget.height = 64u;
  fallbackTarget.stride = 96u * 4u;
  fallbackTarget.scale = 0.0f;

  PrimeStage::RenderStatus referenceStatus =
      PrimeStage::renderFrameToTarget(frame, referenceTarget, options);
  PrimeStage::RenderStatus fallbackStatus =
      PrimeStage::renderFrameToTarget(frame, fallbackTarget, options);

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  REQUIRE(referenceStatus.ok());
  REQUIRE(fallbackStatus.ok());
  CHECK(referenceStatus.targetWidth == 96u);
  CHECK(referenceStatus.targetHeight == 64u);
  CHECK(fallbackStatus.targetWidth == 96u);
  CHECK(fallbackStatus.targetHeight == 64u);
  CHECK(referencePixels == fallbackPixels);
#else
  CHECK(referenceStatus.code == PrimeStage::RenderStatusCode::BackendUnavailable);
  CHECK(fallbackStatus.code == PrimeStage::RenderStatusCode::BackendUnavailable);
#endif
}

TEST_CASE("PrimeStage render overload keeps invalid-dimension diagnostics when width is zero") {
  PrimeFrame::Frame frame = makeRenderableFrame(96.0f, 64.0f);
  PrimeStage::RenderOptions options;

  std::vector<uint8_t> pixels(64u * 64u * 4u, 0u);
  PrimeStage::RenderTarget target;
  target.pixels = std::span<uint8_t>(pixels);
  target.width = 0u;
  target.height = 64u;
  target.stride = 256u;
  target.scale = 2.0f;

  PrimeStage::RenderStatus status =
      PrimeStage::renderFrameToTarget(frame, target, options);

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  CHECK_FALSE(status.ok());
  CHECK(status.code == PrimeStage::RenderStatusCode::InvalidTargetDimensions);
  CHECK(status.targetWidth == 0u);
  CHECK(status.targetHeight == 64u);
  CHECK(status.targetStride == 256u);
  CHECK(status.requiredStride == 0u);
  CHECK(status.detail == "target width/height must be greater than zero");
#else
  CHECK(status.code == PrimeStage::RenderStatusCode::BackendUnavailable);
#endif
}

TEST_CASE("PrimeStage render overload keeps invalid-dimension diagnostics when height is zero") {
  PrimeFrame::Frame frame = makeRenderableFrame(128.0f, 64.0f);
  PrimeStage::RenderOptions options;

  std::vector<uint8_t> pixels(128u * 64u * 4u, 0u);
  PrimeStage::RenderTarget target;
  target.pixels = std::span<uint8_t>(pixels);
  target.width = 128u;
  target.height = 0u;
  target.stride = 512u;
  target.scale = 2.0f;

  PrimeStage::RenderStatus status =
      PrimeStage::renderFrameToTarget(frame, target, options);

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  CHECK_FALSE(status.ok());
  CHECK(status.code == PrimeStage::RenderStatusCode::InvalidTargetDimensions);
  CHECK(status.targetWidth == 128u);
  CHECK(status.targetHeight == 0u);
  CHECK(status.targetStride == 512u);
  CHECK(status.requiredStride == 512u);
  CHECK(status.detail == "target width/height must be greater than zero");
#else
  CHECK(status.code == PrimeStage::RenderStatusCode::BackendUnavailable);
#endif
}

TEST_CASE("PrimeStage render overload preserves invalid-stride diagnostics") {
  PrimeFrame::Frame frame = makeRenderableFrame(64.0f, 32.0f);
  PrimeStage::RenderOptions options;

  std::vector<uint8_t> pixels(64u * 32u * 4u, 0u);
  PrimeStage::RenderTarget target;
  target.pixels = std::span<uint8_t>(pixels);
  target.width = 64u;
  target.height = 32u;
  target.stride = 64u;
  target.scale = 1.0f;

  PrimeStage::RenderStatus status =
      PrimeStage::renderFrameToTarget(frame, target, options);

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  CHECK_FALSE(status.ok());
  CHECK(status.code == PrimeStage::RenderStatusCode::InvalidTargetStride);
  CHECK(status.targetWidth == 64u);
  CHECK(status.targetHeight == 32u);
  CHECK(status.targetStride == 64u);
  CHECK(status.requiredStride == 256u);
  CHECK(status.detail == "target stride must be at least width * 4 bytes");
#else
  CHECK(status.code == PrimeStage::RenderStatusCode::BackendUnavailable);
#endif
}

TEST_CASE("PrimeStage render overload preserves invalid-buffer diagnostics") {
  PrimeFrame::Frame frame = makeRenderableFrame(64.0f, 32.0f);
  PrimeStage::RenderOptions options;

  std::vector<uint8_t> shortPixels(8u, 0u);
  PrimeStage::RenderTarget target;
  target.pixels = std::span<uint8_t>(shortPixels);
  target.width = 64u;
  target.height = 32u;
  target.stride = 256u;
  target.scale = 1.0f;

  PrimeStage::RenderStatus status =
      PrimeStage::renderFrameToTarget(frame, target, options);

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  CHECK_FALSE(status.ok());
  CHECK(status.code == PrimeStage::RenderStatusCode::InvalidTargetBuffer);
  CHECK(status.targetWidth == 64u);
  CHECK(status.targetHeight == 32u);
  CHECK(status.targetStride == 256u);
  CHECK(status.requiredStride == 256u);
  CHECK(status.detail ==
        "target pixel span is smaller than required stride * height bytes");
#else
  CHECK(status.code == PrimeStage::RenderStatusCode::BackendUnavailable);
#endif
}

TEST_CASE("PrimeStage render path overloads and png write failures are covered") {
  PrimeFrame::Frame frame = makeRenderableFrame(96.0f, 64.0f);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 96.0f, 64.0f);

  std::vector<uint8_t> pixels(96u * 64u * 4u, 0u);
  PrimeStage::RenderTarget target;
  target.pixels = std::span<uint8_t>(pixels);
  target.width = 96u;
  target.height = 64u;
  target.stride = 96u * 4u;
  PrimeStage::RenderOptions options{};

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  PrimeStage::RenderStatus targetOverload =
      PrimeStage::renderFrameToTarget(frame, target, options);
  CHECK(targetOverload.ok());

  std::filesystem::path withLayoutPath = makeTempPngPath("layout");
  std::string withLayoutPathText = withLayoutPath.string();
  PrimeStage::RenderStatus pngWithLayout =
      PrimeStage::renderFrameToPng(frame, layout, withLayoutPathText, options);
  CHECK(pngWithLayout.ok());
  REQUIRE(std::filesystem::exists(withLayoutPath));
  std::error_code fileError;
  auto withLayoutSize = std::filesystem::file_size(withLayoutPath, fileError);
  CHECK_FALSE(fileError);
  CHECK(withLayoutSize > 0u);

  std::filesystem::path noLayoutPath = makeTempPngPath("frame");
  std::string noLayoutPathText = noLayoutPath.string();
  PrimeStage::RenderStatus pngNoLayout =
      PrimeStage::renderFrameToPng(frame, noLayoutPathText, options);
  CHECK(pngNoLayout.ok());
  REQUIRE(std::filesystem::exists(noLayoutPath));
  fileError.clear();
  auto noLayoutSize = std::filesystem::file_size(noLayoutPath, fileError);
  CHECK_FALSE(fileError);
  CHECK(noLayoutSize > 0u);

  std::error_code removeError;
  std::filesystem::remove(withLayoutPath, removeError);
  removeError.clear();
  std::filesystem::remove(noLayoutPath, removeError);

  std::filesystem::path missingParent =
      std::filesystem::temp_directory_path() /
      ("primestage_render_missing_parent_" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::path failurePath = missingParent / "out.png";
  std::string failurePathText = failurePath.string();
  std::filesystem::remove_all(missingParent, removeError);
  PrimeStage::RenderStatus pngWriteFailure =
      PrimeStage::renderFrameToPng(frame, layout, failurePathText, options);
  CHECK_FALSE(pngWriteFailure.ok());
  CHECK(pngWriteFailure.code == PrimeStage::RenderStatusCode::PngWriteFailed);
#else
  CHECK(PrimeStage::renderFrameToTarget(frame, layout, target, options).code ==
        PrimeStage::RenderStatusCode::BackendUnavailable);
  CHECK(PrimeStage::renderFrameToTarget(frame, target, options).code ==
        PrimeStage::RenderStatusCode::BackendUnavailable);
  CHECK(PrimeStage::renderFrameToPng(frame, layout, "headless_layout.png", options).code ==
        PrimeStage::RenderStatusCode::BackendUnavailable);
  CHECK(PrimeStage::renderFrameToPng(frame, "headless_frame.png", options).code ==
        PrimeStage::RenderStatusCode::BackendUnavailable);
#endif
}

TEST_CASE("PrimeStage rounded-corner policy is deterministic under theme changes") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 96.0f, 64.0f);
  PrimeStage::PanelSpec panel;
  panel.rectStyle = 1u;
  panel.size.preferredWidth = 80.0f;
  panel.size.preferredHeight = 32.0f;
  root.createPanel(panel);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 96.0f, 64.0f);
  std::vector<uint8_t> pixels(96u * 64u * 4u, 0u);
  PrimeStage::RenderTarget target;
  target.pixels = std::span<uint8_t>(pixels);
  target.width = 96u;
  target.height = 64u;
  target.stride = 96u * 4u;

  PrimeStage::RenderOptions options;
  options.clear = false;

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  configureThemeForSingleRect(frame,
                              PrimeFrame::Color{0.2f, 0.4f, 0.8f, 1.0f},
                              PrimeFrame::Color{0.9f, 0.2f, 0.2f, 1.0f});
  std::fill(pixels.begin(), pixels.end(), 0u);
  PrimeStage::RenderStatus first = PrimeStage::renderFrameToTarget(frame, layout, target, options);
  REQUIRE(first.ok());
  size_t alphaWithoutMatch = countNonZeroAlpha(pixels);

  configureThemeForSingleRect(frame,
                              PrimeFrame::Color{0.2f, 0.4f, 0.8f, 1.0f},
                              PrimeFrame::Color{0.2f, 0.4f, 0.8f, 1.0f});
  std::fill(pixels.begin(), pixels.end(), 0u);
  PrimeStage::RenderStatus second = PrimeStage::renderFrameToTarget(frame, layout, target, options);
  REQUIRE(second.ok());
  size_t alphaWithMatch = countNonZeroAlpha(pixels);

  CHECK(alphaWithoutMatch > 0u);
  CHECK(alphaWithoutMatch == alphaWithMatch);
#else
  PrimeStage::RenderStatus status = PrimeStage::renderFrameToTarget(frame, layout, target, options);
  CHECK(status.code == PrimeStage::RenderStatusCode::BackendUnavailable);
#endif
}
