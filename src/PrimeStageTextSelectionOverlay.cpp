#include "PrimeStage/PrimeStage.h"

#include "PrimeStage/TextSelection.h"
#include "PrimeStageCollectionInternals.h"

#include <algorithm>
#include <cmath>

namespace PrimeStage {

UiNode UiNode::createTextSelectionOverlay(TextSelectionOverlaySpec const& specInput) {
  TextSelectionOverlaySpec spec = Internal::normalizeTextSelectionOverlaySpec(specInput);
  Internal::WidgetRuntimeContext runtime =
      Internal::makeWidgetRuntimeContext(frame(), nodeId(), allowAbsolute(), true, spec.visible, -1);
  PrimeFrame::Frame& runtimeFrame = Internal::runtimeFrame(runtime);

  Internal::InternalRect bounds = Internal::resolveRect(spec.size);
  float maxWidth = spec.maxWidth;
  if (maxWidth <= 0.0f && bounds.width > 0.0f) {
    maxWidth = bounds.width;
  }

  TextSelectionLayout computedLayout;
  TextSelectionLayout const* layout = spec.layout;
  if (!layout) {
    computedLayout =
        buildTextSelectionLayout(runtimeFrame, spec.textStyle, spec.text, maxWidth, spec.wrap);
    layout = &computedLayout;
  }

  float lineHeight = layout->lineHeight > 0.0f ? layout->lineHeight
                                                : textLineHeight(runtimeFrame, spec.textStyle);
  if (lineHeight <= 0.0f) {
    lineHeight = 1.0f;
  }
  size_t lineCount = std::max<size_t>(1u, layout->lines.size());

  float inferredWidth = bounds.width;
  if (inferredWidth <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    for (auto const& line : layout->lines) {
      inferredWidth = std::max(inferredWidth, line.width);
    }
  }
  float inferredHeight = bounds.height;
  if (inferredHeight <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    inferredHeight = lineHeight * static_cast<float>(lineCount);
  }

  StackSpec columnSpec;
  columnSpec.size = spec.size;
  if (!columnSpec.size.preferredWidth.has_value() && inferredWidth > 0.0f) {
    columnSpec.size.preferredWidth = inferredWidth;
  }
  if (!columnSpec.size.preferredHeight.has_value() && inferredHeight > 0.0f) {
    columnSpec.size.preferredHeight = inferredHeight;
  }
  columnSpec.gap = 0.0f;
  columnSpec.clipChildren = spec.clipChildren;
  columnSpec.visible = spec.visible;
  UiNode parentNode = Internal::makeParentNode(runtime);
  UiNode column = parentNode.createVerticalStack(columnSpec);
  column.setHitTestVisible(false);

  if (spec.selectionStyle == 0 || spec.selectionStart == spec.selectionEnd || spec.text.empty()) {
    return column;
  }

  auto selectionRects = buildSelectionRects(runtimeFrame,
                                            spec.textStyle,
                                            spec.text,
                                            *layout,
                                            spec.selectionStart,
                                            spec.selectionEnd,
                                            spec.paddingX);
  if (selectionRects.empty()) {
    return column;
  }

  size_t rectIndex = 0u;
  float rowWidth = columnSpec.size.preferredWidth.value_or(inferredWidth);

  for (size_t lineIndex = 0; lineIndex < lineCount; ++lineIndex) {
    StackSpec lineSpec;
    if (rowWidth > 0.0f) {
      lineSpec.size.preferredWidth = rowWidth;
    } else {
      lineSpec.size.stretchX = 1.0f;
    }
    lineSpec.size.preferredHeight = lineHeight;
    lineSpec.gap = 0.0f;
    UiNode lineRow = column.createHorizontalStack(lineSpec);
    lineRow.setHitTestVisible(false);

    float leftWidth = 0.0f;
    float selectWidth = 0.0f;
    if (rectIndex < selectionRects.size()) {
      const auto& rect = selectionRects[rectIndex];
      float lineY = static_cast<float>(lineIndex) * lineHeight;
      if (std::abs(rect.y - lineY) <= 0.5f) {
        leftWidth = rect.x;
        selectWidth = rect.width;
        rectIndex++;
      }
    }

    if (leftWidth > 0.0f) {
      SizeSpec leftSize;
      leftSize.preferredWidth = leftWidth;
      leftSize.preferredHeight = lineHeight;
      lineRow.createSpacer(leftSize);
    }
    if (selectWidth > 0.0f) {
      SizeSpec selectSize;
      selectSize.preferredWidth = selectWidth;
      selectSize.preferredHeight = lineHeight;
      PanelSpec selectSpec;
      selectSpec.rectStyle = spec.selectionStyle;
      selectSpec.rectStyleOverride = spec.selectionStyleOverride;
      selectSpec.size = selectSize;
      UiNode selectPanel = lineRow.createPanel(selectSpec);
      selectPanel.setHitTestVisible(false);
    }
    SizeSpec fillSize;
    fillSize.stretchX = 1.0f;
    fillSize.preferredHeight = lineHeight;
    lineRow.createSpacer(fillSize);
  }

  return column;
}

} // namespace PrimeStage
