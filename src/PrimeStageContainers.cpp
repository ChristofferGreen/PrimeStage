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

UiNode UiNode::createVerticalStack(StackSpec const& spec) {
  PrimeFrame::NodeId nodeId = Internal::createNode(frame(),
                                                   id_,
                                                   Internal::InternalRect{},
                                                   &spec.size,
                                                   PrimeFrame::LayoutType::VerticalStack,
                                                   spec.padding,
                                                   spec.gap,
                                                   spec.clipChildren,
                                                   spec.visible);
  if (PrimeFrame::Node* node = frame().getNode(nodeId)) {
    node->hitTestVisible = false;
  }
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createHorizontalStack(StackSpec const& spec) {
  PrimeFrame::NodeId nodeId = Internal::createNode(frame(),
                                                   id_,
                                                   Internal::InternalRect{},
                                                   &spec.size,
                                                   PrimeFrame::LayoutType::HorizontalStack,
                                                   spec.padding,
                                                   spec.gap,
                                                   spec.clipChildren,
                                                   spec.visible);
  if (PrimeFrame::Node* node = frame().getNode(nodeId)) {
    node->hitTestVisible = false;
  }
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createOverlay(StackSpec const& spec) {
  PrimeFrame::NodeId nodeId = Internal::createNode(frame(),
                                                   id_,
                                                   Internal::InternalRect{},
                                                   &spec.size,
                                                   PrimeFrame::LayoutType::Overlay,
                                                   spec.padding,
                                                   spec.gap,
                                                   spec.clipChildren,
                                                   spec.visible);
  if (PrimeFrame::Node* node = frame().getNode(nodeId)) {
    node->hitTestVisible = false;
  }
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createPanel(PanelSpec const& specInput) {
  PanelSpec spec = Internal::normalizePanelSpec(specInput);
  PrimeFrame::NodeId nodeId = Internal::createNode(frame(),
                                                   id_,
                                                   Internal::InternalRect{},
                                                   &spec.size,
                                                   spec.layout,
                                                   spec.padding,
                                                   spec.gap,
                                                   spec.clipChildren,
                                                   spec.visible);
  addRectPrimitive(frame(), nodeId, spec.rectStyle, spec.rectStyleOverride);
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createPanel(PrimeFrame::RectStyleToken rectStyle, SizeSpec const& size) {
  PanelSpec spec;
  spec.rectStyle = rectStyle;
  spec.size = size;
  return createPanel(spec);
}

} // namespace PrimeStage
