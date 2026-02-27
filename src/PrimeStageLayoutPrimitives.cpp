#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"

namespace PrimeStage {
namespace {

PrimeFrame::PrimitiveId addRectPrimitive(PrimeFrame::Frame& frame,
                                         PrimeFrame::NodeId nodeId,
                                         PrimeFrame::RectStyleToken token,
                                         PrimeFrame::RectStyleOverride const& overrideStyle) {
  PrimeFrame::Primitive prim;
  prim.type = PrimeFrame::PrimitiveType::Rect;
  prim.rect.token = token;
  prim.rect.overrideStyle = overrideStyle;
  PrimeFrame::PrimitiveId pid = frame.addPrimitive(prim);
  if (PrimeFrame::Node* node = frame.getNode(nodeId)) {
    node->primitives.push_back(pid);
  }
  return pid;
}

} // namespace

UiNode UiNode::createDivider(DividerSpec const& specInput) {
  DividerSpec spec = Internal::normalizeDividerSpec(specInput);
  Internal::WidgetRuntimeContext runtime =
      Internal::makeWidgetRuntimeContext(frame(), nodeId(), allowAbsolute(), true, spec.visible, -1);
  PrimeFrame::Frame& runtimeFrame = Internal::runtimeFrame(runtime);
  Internal::InternalRect rect = Internal::resolveRect(spec.size);
  PrimeFrame::NodeId nodeId = Internal::createNode(runtimeFrame,
                                                   runtime.parentId,
                                                   rect,
                                                   &spec.size,
                                                   PrimeFrame::LayoutType::None,
                                                   PrimeFrame::Insets{},
                                                   0.0f,
                                                   false,
                                                   spec.visible);
  if (PrimeFrame::Node* node = runtimeFrame.getNode(nodeId)) {
    node->hitTestVisible = false;
  }
  addRectPrimitive(runtimeFrame, nodeId, spec.rectStyle, spec.rectStyleOverride);
  return UiNode(runtimeFrame, nodeId, runtime.allowAbsolute);
}

UiNode UiNode::createDivider(PrimeFrame::RectStyleToken rectStyle, SizeSpec const& size) {
  DividerSpec spec;
  spec.rectStyle = rectStyle;
  spec.size = size;
  return createDivider(spec);
}

UiNode UiNode::createSpacer(SpacerSpec const& specInput) {
  SpacerSpec spec = Internal::normalizeSpacerSpec(specInput);
  Internal::WidgetRuntimeContext runtime =
      Internal::makeWidgetRuntimeContext(frame(), nodeId(), allowAbsolute(), true, spec.visible, -1);
  PrimeFrame::Frame& runtimeFrame = Internal::runtimeFrame(runtime);
  Internal::InternalRect rect = Internal::resolveRect(spec.size);
  PrimeFrame::NodeId nodeId = Internal::createNode(runtimeFrame,
                                                   runtime.parentId,
                                                   rect,
                                                   &spec.size,
                                                   PrimeFrame::LayoutType::None,
                                                   PrimeFrame::Insets{},
                                                   0.0f,
                                                   false,
                                                   spec.visible);
  if (PrimeFrame::Node* node = runtimeFrame.getNode(nodeId)) {
    node->hitTestVisible = false;
  }
  return UiNode(runtimeFrame, nodeId, runtime.allowAbsolute);
}

UiNode UiNode::createSpacer(SizeSpec const& size) {
  SpacerSpec spec;
  spec.size = size;
  return createSpacer(spec);
}

} // namespace PrimeStage
