#include "PrimeStage/Ui.h"
#include "src/PrimeStageCollectionInternals.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <algorithm>
#include <array>
#include <functional>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeFrame::Node* node = frame.getNode(rootId);
  if (node) {
    node->layout = PrimeFrame::LayoutType::Overlay;
    node->sizeHint.width.preferred = 640.0f;
    node->sizeHint.height.preferred = 360.0f;
  }
  return PrimeStage::UiNode(frame, rootId, true);
}

PrimeFrame::LayoutOutput layoutFrame(PrimeFrame::Frame& frame) {
  PrimeFrame::LayoutOutput output;
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 640.0f;
  options.rootHeight = 360.0f;
  engine.layout(frame, output, options);
  return output;
}

PrimeFrame::Event makePointerEvent(PrimeFrame::EventType type, int pointerId, float x, float y) {
  PrimeFrame::Event event;
  event.type = type;
  event.pointerId = pointerId;
  event.x = x;
  event.y = y;
  return event;
}

PrimeFrame::RectStyleToken firstRectToken(PrimeFrame::Frame const& frame, PrimeFrame::NodeId nodeId) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return 0;
  }
  for (PrimeFrame::PrimitiveId primId : node->primitives) {
    PrimeFrame::Primitive const* prim = frame.getPrimitive(primId);
    if (prim && prim->type == PrimeFrame::PrimitiveType::Rect) {
      return prim->rect.token;
    }
  }
  return 0;
}

PrimeFrame::Primitive const* firstTextPrimitive(PrimeFrame::Frame const& frame,
                                                PrimeFrame::NodeId nodeId) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return nullptr;
  }
  for (PrimeFrame::PrimitiveId primId : node->primitives) {
    PrimeFrame::Primitive const* prim = frame.getPrimitive(primId);
    if (prim && prim->type == PrimeFrame::PrimitiveType::Text) {
      return prim;
    }
  }
  return nullptr;
}

std::string firstChildText(PrimeFrame::Frame const& frame, PrimeFrame::NodeId parent) {
  PrimeFrame::Node const* parentNode = frame.getNode(parent);
  if (!parentNode) {
    return {};
  }
  for (PrimeFrame::NodeId childId : parentNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child) {
      continue;
    }
    for (PrimeFrame::PrimitiveId primId : child->primitives) {
      PrimeFrame::Primitive const* prim = frame.getPrimitive(primId);
      if (prim && prim->type == PrimeFrame::PrimitiveType::Text) {
        return prim->textBlock.text;
      }
    }
  }
  return {};
}

PrimeFrame::Primitive const* findRectPrimitiveByToken(PrimeFrame::Frame const& frame,
                                                      PrimeFrame::NodeId nodeId,
                                                      PrimeFrame::RectStyleToken token) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return nullptr;
  }
  for (PrimeFrame::PrimitiveId primId : node->primitives) {
    PrimeFrame::Primitive const* prim = frame.getPrimitive(primId);
    if (prim && prim->type == PrimeFrame::PrimitiveType::Rect && prim->rect.token == token) {
      return prim;
    }
  }
  for (PrimeFrame::NodeId childId : node->children) {
    PrimeFrame::Primitive const* child = findRectPrimitiveByToken(frame, childId, token);
    if (child) {
      return child;
    }
  }
  return nullptr;
}

size_t countRectToken(PrimeFrame::Frame const& frame,
                      PrimeFrame::NodeId nodeId,
                      PrimeFrame::RectStyleToken token) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return 0u;
  }
  size_t count = 0u;
  for (PrimeFrame::PrimitiveId primId : node->primitives) {
    PrimeFrame::Primitive const* prim = frame.getPrimitive(primId);
    if (prim && prim->type == PrimeFrame::PrimitiveType::Rect && prim->rect.token == token) {
      ++count;
    }
  }
  for (PrimeFrame::NodeId childId : node->children) {
    count += countRectToken(frame, childId, token);
  }
  return count;
}

size_t countTextValue(PrimeFrame::Frame const& frame,
                      PrimeFrame::NodeId nodeId,
                      std::string_view text) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return 0u;
  }
  size_t count = 0u;
  for (PrimeFrame::PrimitiveId primId : node->primitives) {
    PrimeFrame::Primitive const* prim = frame.getPrimitive(primId);
    if (prim && prim->type == PrimeFrame::PrimitiveType::Text &&
        prim->textBlock.text == text) {
      ++count;
    }
  }
  for (PrimeFrame::NodeId childId : node->children) {
    count += countTextValue(frame, childId, text);
  }
  return count;
}

std::string boolString(bool value) {
  return value ? "true" : "false";
}

std::string optionalBoolString(std::optional<bool> const& value) {
  if (!value.has_value()) {
    return "-";
  }
  return value.value() ? "true" : "false";
}

std::string optionalIntString(std::optional<int> const& value) {
  if (!value.has_value()) {
    return "-";
  }
  return std::to_string(value.value());
}

std::string optionalFloatString(std::optional<float> const& value) {
  if (!value.has_value()) {
    return "-";
  }
  std::ostringstream out;
  out << std::fixed << std::setprecision(2) << value.value();
  return out.str();
}

std::string roleString(PrimeStage::AccessibilityRole role) {
  switch (role) {
    case PrimeStage::AccessibilityRole::Unspecified:
      return "unspecified";
    case PrimeStage::AccessibilityRole::Group:
      return "group";
    case PrimeStage::AccessibilityRole::StaticText:
      return "static_text";
    case PrimeStage::AccessibilityRole::Button:
      return "button";
    case PrimeStage::AccessibilityRole::TextField:
      return "text_field";
    case PrimeStage::AccessibilityRole::Toggle:
      return "toggle";
    case PrimeStage::AccessibilityRole::Checkbox:
      return "checkbox";
    case PrimeStage::AccessibilityRole::Slider:
      return "slider";
    case PrimeStage::AccessibilityRole::TabList:
      return "tab_list";
    case PrimeStage::AccessibilityRole::Tab:
      return "tab";
    case PrimeStage::AccessibilityRole::ComboBox:
      return "combo_box";
    case PrimeStage::AccessibilityRole::ProgressBar:
      return "progress_bar";
    case PrimeStage::AccessibilityRole::Table:
      return "table";
    case PrimeStage::AccessibilityRole::Tree:
      return "tree";
    case PrimeStage::AccessibilityRole::TreeItem:
      return "tree_item";
  }
  return "unknown";
}

std::string exportSemanticsRow(std::string_view name,
                               PrimeStage::AccessibilitySemantics const& semantics) {
  std::ostringstream out;
  out << name;
  out << " role=" << roleString(semantics.role);
  out << " disabled=" << boolString(semantics.state.disabled);
  out << " checked=" << optionalBoolString(semantics.state.checked);
  out << " selected=" << optionalBoolString(semantics.state.selected);
  out << " expanded=" << optionalBoolString(semantics.state.expanded);
  out << " valueNow=" << optionalFloatString(semantics.state.valueNow);
  out << " valueMin=" << optionalFloatString(semantics.state.valueMin);
  out << " valueMax=" << optionalFloatString(semantics.state.valueMax);
  out << " position=" << optionalIntString(semantics.state.positionInSet);
  out << " setSize=" << optionalIntString(semantics.state.setSize);
  return out.str();
}

std::string exportFocusRow(std::string_view name,
                           PrimeStage::AccessibilitySemantics const& semantics,
                           bool focused) {
  std::ostringstream out;
  out << name;
  out << " role=" << roleString(semantics.role);
  out << " focused=" << boolString(focused);
  out << " disabled=" << boolString(semantics.state.disabled);
  return out.str();
}

constexpr uint64_t SanitizationFuzzSeed = 0x51A715A11C0FFEEu;
constexpr int SanitizationFuzzIterations = 192;

float fuzzFloat(std::mt19937_64& rng, float minValue, float maxValue) {
  std::uniform_real_distribution<float> dist(minValue, maxValue);
  return dist(rng);
}

int fuzzInt(std::mt19937_64& rng, int minValue, int maxValue) {
  std::uniform_int_distribution<int> dist(minValue, maxValue);
  return dist(rng);
}

std::optional<float> fuzzOptionalFloat(std::mt19937_64& rng, float minValue, float maxValue) {
  if ((rng() % 3u) == 0u) {
    return std::nullopt;
  }
  return fuzzFloat(rng, minValue, maxValue);
}

void fuzzSizeSpec(PrimeStage::SizeSpec& size, std::mt19937_64& rng) {
  size.minWidth = fuzzOptionalFloat(rng, -240.0f, 240.0f);
  size.maxWidth = fuzzOptionalFloat(rng, -240.0f, 240.0f);
  size.preferredWidth = fuzzOptionalFloat(rng, -240.0f, 240.0f);
  size.stretchX = fuzzFloat(rng, -4.0f, 4.0f);
  size.minHeight = fuzzOptionalFloat(rng, -240.0f, 240.0f);
  size.maxHeight = fuzzOptionalFloat(rng, -240.0f, 240.0f);
  size.preferredHeight = fuzzOptionalFloat(rng, -240.0f, 240.0f);
  size.stretchY = fuzzFloat(rng, -4.0f, 4.0f);
}

void checkSanitizedSizeSpec(PrimeStage::SizeSpec const& size) {
  if (size.minWidth.has_value()) {
    CHECK(size.minWidth.value() >= 0.0f);
  }
  if (size.maxWidth.has_value()) {
    CHECK(size.maxWidth.value() >= 0.0f);
  }
  if (size.preferredWidth.has_value()) {
    CHECK(size.preferredWidth.value() >= 0.0f);
  }
  CHECK(size.stretchX >= 0.0f);

  if (size.minHeight.has_value()) {
    CHECK(size.minHeight.value() >= 0.0f);
  }
  if (size.maxHeight.has_value()) {
    CHECK(size.maxHeight.value() >= 0.0f);
  }
  if (size.preferredHeight.has_value()) {
    CHECK(size.preferredHeight.value() >= 0.0f);
  }
  CHECK(size.stretchY >= 0.0f);

  if (size.minWidth.has_value() && size.maxWidth.has_value()) {
    CHECK(size.minWidth.value() <= size.maxWidth.value());
  }
  if (size.minHeight.has_value() && size.maxHeight.has_value()) {
    CHECK(size.minHeight.value() <= size.maxHeight.value());
  }
  if (size.preferredWidth.has_value() && size.minWidth.has_value()) {
    CHECK(size.preferredWidth.value() >= size.minWidth.value());
  }
  if (size.preferredWidth.has_value() && size.maxWidth.has_value()) {
    CHECK(size.preferredWidth.value() <= size.maxWidth.value());
  }
  if (size.preferredHeight.has_value() && size.minHeight.has_value()) {
    CHECK(size.preferredHeight.value() >= size.minHeight.value());
  }
  if (size.preferredHeight.has_value() && size.maxHeight.has_value()) {
    CHECK(size.preferredHeight.value() <= size.maxHeight.value());
  }
}

int expectedSelectedIndex(int value, int count) {
  if (count <= 0) {
    return 0;
  }
  return std::clamp(value, 0, count - 1);
}

int expectedSelectedRowOrNone(int value, int count) {
  if (count <= 0) {
    return -1;
  }
  if (value < 0 || value >= count) {
    return -1;
  }
  return value;
}

int expectedTabIndex(int value) {
  return std::max(value, -1);
}

bool inUnitInterval(float value) {
  return value >= 0.0f && value <= 1.0f;
}

} // namespace

TEST_CASE("PrimeStage size validation clamps invalid ranges and negative values") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::PanelSpec panel;
  panel.rectStyle = 100u;
  panel.size.minWidth = 80.0f;
  panel.size.maxWidth = 40.0f;
  panel.size.preferredWidth = 12.0f;
  panel.size.stretchX = -1.0f;
  panel.size.minHeight = -10.0f;
  panel.size.maxHeight = 20.0f;
  panel.size.preferredHeight = 50.0f;
  panel.size.stretchY = -2.0f;
  panel.padding.left = -4.0f;
  panel.padding.top = -3.0f;
  panel.gap = -5.0f;

  PrimeStage::UiNode node = root.createPanel(panel);
  PrimeFrame::Node const* panelNode = frame.getNode(node.nodeId());
  REQUIRE(panelNode != nullptr);
  REQUIRE(panelNode->sizeHint.width.min.has_value());
  REQUIRE(panelNode->sizeHint.width.max.has_value());
  REQUIRE(panelNode->sizeHint.width.preferred.has_value());
  REQUIRE(panelNode->sizeHint.height.min.has_value());
  REQUIRE(panelNode->sizeHint.height.max.has_value());
  REQUIRE(panelNode->sizeHint.height.preferred.has_value());

  CHECK(panelNode->sizeHint.width.min.value() == doctest::Approx(80.0f));
  CHECK(panelNode->sizeHint.width.max.value() == doctest::Approx(80.0f));
  CHECK(panelNode->sizeHint.width.preferred.value() == doctest::Approx(80.0f));
  CHECK(panelNode->sizeHint.width.stretch == doctest::Approx(0.0f));

  CHECK(panelNode->sizeHint.height.min.value() == doctest::Approx(0.0f));
  CHECK(panelNode->sizeHint.height.max.value() == doctest::Approx(20.0f));
  CHECK(panelNode->sizeHint.height.preferred.value() == doctest::Approx(20.0f));
  CHECK(panelNode->sizeHint.height.stretch == doctest::Approx(0.0f));

  CHECK(panelNode->padding.left == doctest::Approx(0.0f));
  CHECK(panelNode->padding.top == doctest::Approx(0.0f));
  CHECK(panelNode->gap == doctest::Approx(0.0f));
}

TEST_CASE("PrimeStage extension primitive seam clamps invalid layout sizing inputs") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::Internal::WidgetRuntimeContext runtime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          true,
          true,
          7);

  PrimeStage::Internal::ExtensionPrimitiveSpec spec;
  spec.layout = PrimeFrame::LayoutType::VerticalStack;
  spec.rectStyle = 944u;
  spec.focusable = true;
  spec.hitTestVisible = true;
  spec.size.minWidth = 80.0f;
  spec.size.maxWidth = 40.0f;
  spec.size.preferredWidth = 12.0f;
  spec.size.stretchX = -1.0f;
  spec.size.minHeight = -10.0f;
  spec.size.maxHeight = 24.0f;
  spec.size.preferredHeight = 50.0f;
  spec.size.stretchY = -2.0f;
  spec.padding.left = -4.0f;
  spec.padding.top = -3.0f;
  spec.padding.right = -2.0f;
  spec.padding.bottom = -1.0f;
  spec.gap = -5.0f;

  PrimeStage::UiNode extension = PrimeStage::Internal::createExtensionPrimitive(runtime, spec);
  PrimeFrame::Node const* extensionNode = frame.getNode(extension.nodeId());
  REQUIRE(extensionNode != nullptr);
  REQUIRE(extensionNode->sizeHint.width.min.has_value());
  REQUIRE(extensionNode->sizeHint.width.max.has_value());
  REQUIRE(extensionNode->sizeHint.width.preferred.has_value());
  REQUIRE(extensionNode->sizeHint.height.min.has_value());
  REQUIRE(extensionNode->sizeHint.height.max.has_value());
  REQUIRE(extensionNode->sizeHint.height.preferred.has_value());

  CHECK(extensionNode->layout == PrimeFrame::LayoutType::VerticalStack);
  CHECK(extensionNode->sizeHint.width.min.value() == doctest::Approx(80.0f));
  CHECK(extensionNode->sizeHint.width.max.value() == doctest::Approx(80.0f));
  CHECK(extensionNode->sizeHint.width.preferred.value() == doctest::Approx(80.0f));
  CHECK(extensionNode->sizeHint.width.stretch == doctest::Approx(0.0f));
  CHECK(extensionNode->sizeHint.height.min.value() == doctest::Approx(0.0f));
  CHECK(extensionNode->sizeHint.height.max.value() == doctest::Approx(24.0f));
  CHECK(extensionNode->sizeHint.height.preferred.value() == doctest::Approx(24.0f));
  CHECK(extensionNode->sizeHint.height.stretch == doctest::Approx(0.0f));
  CHECK(extensionNode->padding.left == doctest::Approx(0.0f));
  CHECK(extensionNode->padding.top == doctest::Approx(0.0f));
  CHECK(extensionNode->padding.right == doctest::Approx(0.0f));
  CHECK(extensionNode->padding.bottom == doctest::Approx(0.0f));
  CHECK(extensionNode->gap == doctest::Approx(0.0f));
  CHECK(extensionNode->focusable);
  CHECK(extensionNode->hitTestVisible);
  CHECK(extensionNode->tabIndex == 7);

  PrimeFrame::Primitive const* rect = findRectPrimitiveByToken(frame, extension.nodeId(), 944u);
  REQUIRE(rect != nullptr);
}

TEST_CASE("PrimeStage helper widgets clamp invalid helper spec inputs") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::LabelSpec label;
  label.text = "Helper label";
  label.maxWidth = -120.0f;
  label.size.preferredWidth = -60.0f;
  label.size.preferredHeight = -24.0f;
  label.size.stretchX = -1.0f;
  label.size.stretchY = -2.0f;

  PrimeStage::UiNode labelNode = root.createLabel(label);
  PrimeFrame::Node const* labelFrameNode = frame.getNode(labelNode.nodeId());
  REQUIRE(labelFrameNode != nullptr);
  REQUIRE(labelFrameNode->sizeHint.width.preferred.has_value());
  REQUIRE(labelFrameNode->sizeHint.height.preferred.has_value());
  CHECK(labelFrameNode->sizeHint.width.preferred.value() == doctest::Approx(0.0f));
  CHECK(labelFrameNode->sizeHint.height.preferred.value() == doctest::Approx(0.0f));
  CHECK(labelFrameNode->sizeHint.width.stretch == doctest::Approx(0.0f));
  CHECK(labelFrameNode->sizeHint.height.stretch == doctest::Approx(0.0f));

  PrimeFrame::Primitive const* labelText = firstTextPrimitive(frame, labelNode.nodeId());
  REQUIRE(labelText != nullptr);
  CHECK(labelText->textBlock.maxWidth == doctest::Approx(0.0f));

  PrimeStage::DividerSpec divider;
  divider.size.preferredWidth = -20.0f;
  divider.size.preferredHeight = -4.0f;
  PrimeStage::UiNode dividerNode = root.createDivider(divider);
  PrimeFrame::Node const* dividerFrameNode = frame.getNode(dividerNode.nodeId());
  REQUIRE(dividerFrameNode != nullptr);
  REQUIRE(dividerFrameNode->sizeHint.width.preferred.has_value());
  REQUIRE(dividerFrameNode->sizeHint.height.preferred.has_value());
  CHECK(dividerFrameNode->sizeHint.width.preferred.value() == doctest::Approx(0.0f));
  CHECK(dividerFrameNode->sizeHint.height.preferred.value() == doctest::Approx(0.0f));
  CHECK_FALSE(dividerFrameNode->hitTestVisible);

  PrimeStage::SpacerSpec spacer;
  spacer.size.preferredWidth = -18.0f;
  spacer.size.preferredHeight = -8.0f;
  PrimeStage::UiNode spacerNode = root.createSpacer(spacer);
  PrimeFrame::Node const* spacerFrameNode = frame.getNode(spacerNode.nodeId());
  REQUIRE(spacerFrameNode != nullptr);
  REQUIRE(spacerFrameNode->sizeHint.width.preferred.has_value());
  REQUIRE(spacerFrameNode->sizeHint.height.preferred.has_value());
  CHECK(spacerFrameNode->sizeHint.width.preferred.value() == doctest::Approx(0.0f));
  CHECK(spacerFrameNode->sizeHint.height.preferred.value() == doctest::Approx(0.0f));
  CHECK_FALSE(spacerFrameNode->hitTestVisible);
}

TEST_CASE("PrimeStage divider overload maps style token and size to a divider node") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::SizeSpec size;
  size.preferredWidth = 96.0f;
  size.preferredHeight = 3.0f;
  PrimeStage::UiNode divider = root.createDivider(777u, size);

  PrimeFrame::Node const* dividerNode = frame.getNode(divider.nodeId());
  REQUIRE(dividerNode != nullptr);
  REQUIRE(dividerNode->sizeHint.width.preferred.has_value());
  REQUIRE(dividerNode->sizeHint.height.preferred.has_value());
  CHECK(dividerNode->sizeHint.width.preferred.value() == doctest::Approx(96.0f));
  CHECK(dividerNode->sizeHint.height.preferred.value() == doctest::Approx(3.0f));
  CHECK_FALSE(dividerNode->hitTestVisible);
  CHECK(findRectPrimitiveByToken(frame, divider.nodeId(), 777u) != nullptr);
}

TEST_CASE("PrimeStage panel overload maps style token and size to a panel node") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::SizeSpec size;
  size.preferredWidth = 140.0f;
  size.preferredHeight = 44.0f;
  PrimeStage::UiNode panel = root.createPanel(888u, size);

  PrimeFrame::Node const* panelNode = frame.getNode(panel.nodeId());
  REQUIRE(panelNode != nullptr);
  REQUIRE(panelNode->sizeHint.width.preferred.has_value());
  REQUIRE(panelNode->sizeHint.height.preferred.has_value());
  CHECK(panelNode->sizeHint.width.preferred.value() == doctest::Approx(140.0f));
  CHECK(panelNode->sizeHint.height.preferred.value() == doctest::Approx(44.0f));
  CHECK(findRectPrimitiveByToken(frame, panel.nodeId(), 888u) != nullptr);
}

TEST_CASE("PrimeStage panel overload clamps invalid size inputs") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::SizeSpec size;
  size.preferredWidth = -32.0f;
  size.preferredHeight = -12.0f;
  PrimeStage::UiNode panel = root.createPanel(889u, size);

  PrimeFrame::Node const* panelNode = frame.getNode(panel.nodeId());
  REQUIRE(panelNode != nullptr);
  REQUIRE(panelNode->sizeHint.width.preferred.has_value());
  REQUIRE(panelNode->sizeHint.height.preferred.has_value());
  CHECK(panelNode->sizeHint.width.preferred.value() == doctest::Approx(0.0f));
  CHECK(panelNode->sizeHint.height.preferred.value() == doctest::Approx(0.0f));
  CHECK(findRectPrimitiveByToken(frame, panel.nodeId(), 889u) != nullptr);
}

TEST_CASE("PrimeStage label overload maps text style and explicit size") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::SizeSpec size;
  size.preferredWidth = 120.0f;
  size.preferredHeight = 22.0f;
  PrimeStage::UiNode label = root.createLabel("Overload label", 901u, size);

  PrimeFrame::Node const* labelNode = frame.getNode(label.nodeId());
  REQUIRE(labelNode != nullptr);
  REQUIRE(labelNode->sizeHint.width.preferred.has_value());
  REQUIRE(labelNode->sizeHint.height.preferred.has_value());
  CHECK(labelNode->sizeHint.width.preferred.value() == doctest::Approx(120.0f));
  CHECK(labelNode->sizeHint.height.preferred.value() == doctest::Approx(22.0f));
  CHECK_FALSE(labelNode->hitTestVisible);

  PrimeFrame::Primitive const* text = firstTextPrimitive(frame, label.nodeId());
  REQUIRE(text != nullptr);
  CHECK(text->textBlock.text == "Overload label");
  CHECK(text->textStyle.token == 901u);
  CHECK(text->width == doctest::Approx(120.0f));
  CHECK(text->height == doctest::Approx(22.0f));
}

TEST_CASE("PrimeStage paragraph overload maps text style and explicit size") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::SizeSpec size;
  size.preferredWidth = 180.0f;
  size.preferredHeight = 60.0f;
  PrimeStage::UiNode paragraph = root.createParagraph("Paragraph overload", 902u, size);

  PrimeFrame::Node const* paragraphNode = frame.getNode(paragraph.nodeId());
  REQUIRE(paragraphNode != nullptr);
  REQUIRE(paragraphNode->sizeHint.width.preferred.has_value());
  REQUIRE(paragraphNode->sizeHint.height.preferred.has_value());
  CHECK(paragraphNode->sizeHint.width.preferred.value() == doctest::Approx(180.0f));
  CHECK(paragraphNode->sizeHint.height.preferred.value() == doctest::Approx(60.0f));
  CHECK_FALSE(paragraphNode->hitTestVisible);
  REQUIRE(!paragraphNode->children.empty());

  PrimeFrame::NodeId lineNodeId = paragraphNode->children.front();
  PrimeFrame::Primitive const* text = firstTextPrimitive(frame, lineNodeId);
  REQUIRE(text != nullptr);
  CHECK(text->textBlock.text == "Paragraph overload");
  CHECK(text->textStyle.token == 902u);
  CHECK(text->textBlock.maxWidth == doctest::Approx(180.0f));
}

TEST_CASE("PrimeStage text line overload maps text style size and manual alignment") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::SizeSpec size;
  size.preferredWidth = 160.0f;
  size.preferredHeight = 28.0f;
  PrimeStage::UiNode line =
      root.createTextLine("Hi", 903u, size, PrimeFrame::TextAlign::End);

  PrimeFrame::Node const* lineNode = frame.getNode(line.nodeId());
  REQUIRE(lineNode != nullptr);
  CHECK(lineNode->localX >= 0.0f);

  PrimeFrame::Primitive const* text = firstTextPrimitive(frame, line.nodeId());
  REQUIRE(text != nullptr);
  CHECK(text->textBlock.text == "Hi");
  CHECK(text->textStyle.token == 903u);
  CHECK(text->textBlock.align == PrimeFrame::TextAlign::Start);
  CHECK(text->height > 0.0f);
}

TEST_CASE("PrimeStage progress bar binding overload clamps state and applies defaults") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::State<float> state;
  state.value = 1.75f;
  PrimeStage::UiNode progress = root.createProgressBar(PrimeStage::bind(state));

  CHECK(state.value == doctest::Approx(1.0f));

  PrimeFrame::Node const* progressNode = frame.getNode(progress.nodeId());
  REQUIRE(progressNode != nullptr);
  REQUIRE(progressNode->sizeHint.width.preferred.has_value());
  REQUIRE(progressNode->sizeHint.height.preferred.has_value());
  CHECK(progressNode->sizeHint.width.preferred.value() == doctest::Approx(140.0f));
  CHECK(progressNode->sizeHint.height.preferred.value() == doctest::Approx(12.0f));
  CHECK(progressNode->callbacks != PrimeFrame::InvalidCallbackId);
  CHECK(countRectToken(frame, progress.nodeId(), 0u) >= 2u);
}

TEST_CASE("PrimeStage toggle binding overload uses bound state with default sizing") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::State<bool> state;
  state.value = true;
  PrimeStage::UiNode toggle = root.createToggle(PrimeStage::bind(state));

  PrimeFrame::Node const* toggleNode = frame.getNode(toggle.nodeId());
  REQUIRE(toggleNode != nullptr);
  REQUIRE(toggleNode->sizeHint.width.preferred.has_value());
  REQUIRE(toggleNode->sizeHint.height.preferred.has_value());
  CHECK(toggleNode->sizeHint.width.preferred.value() == doctest::Approx(40.0f));
  CHECK(toggleNode->sizeHint.height.preferred.value() == doctest::Approx(20.0f));
  CHECK(toggleNode->callbacks != PrimeFrame::InvalidCallbackId);
  CHECK(state.value);
}

TEST_CASE("PrimeStage toggle binding overload key activation updates bound state and knob position") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::State<bool> state;
  state.value = false;
  PrimeStage::UiNode toggle = root.createToggle(PrimeStage::bind(state));

  PrimeFrame::Node const* toggleNode = frame.getNode(toggle.nodeId());
  REQUIRE(toggleNode != nullptr);
  REQUIRE(toggleNode->callbacks != PrimeFrame::InvalidCallbackId);

  auto findKnobNode = [&]() -> PrimeFrame::Node const* {
    PrimeFrame::Node const* rootNode = frame.getNode(toggle.nodeId());
    if (!rootNode) {
      return nullptr;
    }
    for (PrimeFrame::NodeId childId : rootNode->children) {
      PrimeFrame::Node const* child = frame.getNode(childId);
      if (!child || !child->sizeHint.width.preferred.has_value() ||
          !child->sizeHint.height.preferred.has_value()) {
        continue;
      }
      float width = child->sizeHint.width.preferred.value();
      float height = child->sizeHint.height.preferred.value();
      if (width > 0.0f && width <= 20.0f && height > 0.0f && height <= 20.0f) {
        return child;
      }
    }
    return nullptr;
  };

  PrimeFrame::Node const* knobBefore = findKnobNode();
  REQUIRE(knobBefore != nullptr);
  CHECK(knobBefore->localX == doctest::Approx(2.0f));
  CHECK(knobBefore->localY == doctest::Approx(2.0f));

  PrimeFrame::Callback const* callback = frame.getCallback(toggleNode->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  PrimeFrame::Event keyDown;
  keyDown.type = PrimeFrame::EventType::KeyDown;
  keyDown.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Space);
  CHECK(callback->onEvent(keyDown));
  CHECK(state.value);

  PrimeFrame::Node const* knobAfter = findKnobNode();
  REQUIRE(knobAfter != nullptr);
  CHECK(knobAfter->localX == doctest::Approx(22.0f));
  CHECK(knobAfter->localY == doctest::Approx(2.0f));
}

TEST_CASE("PrimeStage toggle binding overload ignores non-activation key events") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::State<bool> state;
  state.value = false;
  PrimeStage::UiNode toggle = root.createToggle(PrimeStage::bind(state));

  PrimeFrame::Node const* toggleNode = frame.getNode(toggle.nodeId());
  REQUIRE(toggleNode != nullptr);
  REQUIRE(toggleNode->callbacks != PrimeFrame::InvalidCallbackId);

  auto findKnobNode = [&]() -> PrimeFrame::Node const* {
    PrimeFrame::Node const* rootNode = frame.getNode(toggle.nodeId());
    if (!rootNode) {
      return nullptr;
    }
    for (PrimeFrame::NodeId childId : rootNode->children) {
      PrimeFrame::Node const* child = frame.getNode(childId);
      if (!child || !child->sizeHint.width.preferred.has_value() ||
          !child->sizeHint.height.preferred.has_value()) {
        continue;
      }
      float width = child->sizeHint.width.preferred.value();
      float height = child->sizeHint.height.preferred.value();
      if (width > 0.0f && width <= 20.0f && height > 0.0f && height <= 20.0f) {
        return child;
      }
    }
    return nullptr;
  };

  PrimeFrame::Node const* knobBefore = findKnobNode();
  REQUIRE(knobBefore != nullptr);
  CHECK(knobBefore->localX == doctest::Approx(2.0f));
  CHECK(knobBefore->localY == doctest::Approx(2.0f));

  PrimeFrame::Callback const* callback = frame.getCallback(toggleNode->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  PrimeFrame::Event keyDown;
  keyDown.type = PrimeFrame::EventType::KeyDown;
  keyDown.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Left);
  CHECK_FALSE(callback->onEvent(keyDown));
  CHECK_FALSE(state.value);

  PrimeFrame::Node const* knobAfter = findKnobNode();
  REQUIRE(knobAfter != nullptr);
  CHECK(knobAfter->localX == doctest::Approx(2.0f));
  CHECK(knobAfter->localY == doctest::Approx(2.0f));
}

TEST_CASE("PrimeStage toggle binding overload pointer cancel suppresses activation") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::State<bool> state;
  state.value = false;
  PrimeStage::UiNode toggle = root.createToggle(PrimeStage::bind(state));

  PrimeFrame::Node const* toggleNode = frame.getNode(toggle.nodeId());
  REQUIRE(toggleNode != nullptr);
  REQUIRE(toggleNode->callbacks != PrimeFrame::InvalidCallbackId);

  auto findKnobNode = [&]() -> PrimeFrame::Node const* {
    PrimeFrame::Node const* rootNode = frame.getNode(toggle.nodeId());
    if (!rootNode) {
      return nullptr;
    }
    for (PrimeFrame::NodeId childId : rootNode->children) {
      PrimeFrame::Node const* child = frame.getNode(childId);
      if (!child || !child->sizeHint.width.preferred.has_value() ||
          !child->sizeHint.height.preferred.has_value()) {
        continue;
      }
      float width = child->sizeHint.width.preferred.value();
      float height = child->sizeHint.height.preferred.value();
      if (width > 0.0f && width <= 20.0f && height > 0.0f && height <= 20.0f) {
        return child;
      }
    }
    return nullptr;
  };

  PrimeFrame::Node const* knobBefore = findKnobNode();
  REQUIRE(knobBefore != nullptr);
  CHECK(knobBefore->localX == doctest::Approx(2.0f));

  PrimeFrame::Callback const* callback = frame.getCallback(toggleNode->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  PrimeFrame::Event pointerDown;
  pointerDown.type = PrimeFrame::EventType::PointerDown;
  CHECK(callback->onEvent(pointerDown));

  PrimeFrame::Event pointerCancel;
  pointerCancel.type = PrimeFrame::EventType::PointerCancel;
  CHECK(callback->onEvent(pointerCancel));

  PrimeFrame::Event pointerUp;
  pointerUp.type = PrimeFrame::EventType::PointerUp;
  pointerUp.localX = 10.0f;
  pointerUp.localY = 10.0f;
  pointerUp.targetW = 40.0f;
  pointerUp.targetH = 20.0f;
  CHECK(callback->onEvent(pointerUp));
  CHECK_FALSE(state.value);

  PrimeFrame::Node const* knobAfter = findKnobNode();
  REQUIRE(knobAfter != nullptr);
  CHECK(knobAfter->localX == doctest::Approx(2.0f));
}

TEST_CASE("PrimeStage checkbox binding overload uses bound state and label") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::State<bool> state;
  state.value = true;
  PrimeStage::UiNode checkbox = root.createCheckbox("BoundCheck", PrimeStage::bind(state));

  PrimeFrame::Node const* rowNode = frame.getNode(checkbox.nodeId());
  REQUIRE(rowNode != nullptr);
  CHECK(rowNode->callbacks != PrimeFrame::InvalidCallbackId);
  CHECK(firstChildText(frame, checkbox.nodeId()) == "BoundCheck");
  REQUIRE(!rowNode->children.empty());

  PrimeFrame::Node const* boxNode = frame.getNode(rowNode->children.front());
  REQUIRE(boxNode != nullptr);
  REQUIRE(!boxNode->children.empty());
  PrimeFrame::Node const* checkNode = frame.getNode(boxNode->children.front());
  REQUIRE(checkNode != nullptr);
  CHECK(checkNode->visible);
  CHECK(state.value);
}

TEST_CASE("PrimeStage checkbox binding overload hides check when bound state is false") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::State<bool> state;
  state.value = false;
  PrimeStage::UiNode checkbox = root.createCheckbox("", PrimeStage::bind(state));

  PrimeFrame::Node const* rowNode = frame.getNode(checkbox.nodeId());
  REQUIRE(rowNode != nullptr);
  CHECK(rowNode->callbacks != PrimeFrame::InvalidCallbackId);
  CHECK(firstChildText(frame, checkbox.nodeId()).empty());
  REQUIRE(!rowNode->children.empty());

  PrimeFrame::Node const* boxNode = frame.getNode(rowNode->children.front());
  REQUIRE(boxNode != nullptr);
  REQUIRE(!boxNode->children.empty());
  PrimeFrame::Node const* checkNode = frame.getNode(boxNode->children.front());
  REQUIRE(checkNode != nullptr);
  CHECK_FALSE(checkNode->visible);
  CHECK_FALSE(state.value);
}

TEST_CASE("PrimeStage checkbox binding overload key activation updates bound state and check visibility") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::State<bool> state;
  state.value = false;
  PrimeStage::UiNode checkbox = root.createCheckbox("Activate", PrimeStage::bind(state));

  PrimeFrame::Node const* rowNode = frame.getNode(checkbox.nodeId());
  REQUIRE(rowNode != nullptr);
  REQUIRE(rowNode->callbacks != PrimeFrame::InvalidCallbackId);
  REQUIRE(!rowNode->children.empty());

  PrimeFrame::Node const* boxNode = frame.getNode(rowNode->children.front());
  REQUIRE(boxNode != nullptr);
  REQUIRE(!boxNode->children.empty());
  PrimeFrame::Node const* checkNodeBefore = frame.getNode(boxNode->children.front());
  REQUIRE(checkNodeBefore != nullptr);
  CHECK_FALSE(checkNodeBefore->visible);
  CHECK_FALSE(state.value);

  PrimeFrame::Callback const* callback = frame.getCallback(rowNode->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  PrimeFrame::Event keyDown;
  keyDown.type = PrimeFrame::EventType::KeyDown;
  keyDown.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Space);
  CHECK(callback->onEvent(keyDown));
  CHECK(state.value);

  PrimeFrame::Node const* checkNodeAfter = frame.getNode(boxNode->children.front());
  REQUIRE(checkNodeAfter != nullptr);
  CHECK(checkNodeAfter->visible);
}

TEST_CASE("PrimeStage checkbox binding overload ignores non-activation key events") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::State<bool> state;
  state.value = false;
  PrimeStage::UiNode checkbox = root.createCheckbox("Ignore", PrimeStage::bind(state));

  PrimeFrame::Node const* rowNode = frame.getNode(checkbox.nodeId());
  REQUIRE(rowNode != nullptr);
  REQUIRE(rowNode->callbacks != PrimeFrame::InvalidCallbackId);
  REQUIRE(!rowNode->children.empty());

  PrimeFrame::Node const* boxNode = frame.getNode(rowNode->children.front());
  REQUIRE(boxNode != nullptr);
  REQUIRE(!boxNode->children.empty());
  PrimeFrame::Node const* checkNodeBefore = frame.getNode(boxNode->children.front());
  REQUIRE(checkNodeBefore != nullptr);
  CHECK_FALSE(checkNodeBefore->visible);

  PrimeFrame::Callback const* callback = frame.getCallback(rowNode->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  PrimeFrame::Event keyDown;
  keyDown.type = PrimeFrame::EventType::KeyDown;
  keyDown.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Left);
  CHECK_FALSE(callback->onEvent(keyDown));
  CHECK_FALSE(state.value);

  PrimeFrame::Node const* checkNodeAfter = frame.getNode(boxNode->children.front());
  REQUIRE(checkNodeAfter != nullptr);
  CHECK_FALSE(checkNodeAfter->visible);
}

TEST_CASE("PrimeStage checkbox binding overload pointer leave suppresses activation") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::State<bool> state;
  state.value = false;
  PrimeStage::UiNode checkbox = root.createCheckbox("Leave", PrimeStage::bind(state));

  PrimeFrame::Node const* rowNode = frame.getNode(checkbox.nodeId());
  REQUIRE(rowNode != nullptr);
  REQUIRE(rowNode->callbacks != PrimeFrame::InvalidCallbackId);
  REQUIRE(!rowNode->children.empty());

  PrimeFrame::Node const* boxNode = frame.getNode(rowNode->children.front());
  REQUIRE(boxNode != nullptr);
  REQUIRE(!boxNode->children.empty());
  PrimeFrame::Node const* checkNodeBefore = frame.getNode(boxNode->children.front());
  REQUIRE(checkNodeBefore != nullptr);
  CHECK_FALSE(checkNodeBefore->visible);

  PrimeFrame::Callback const* callback = frame.getCallback(rowNode->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  PrimeFrame::Event pointerDown;
  pointerDown.type = PrimeFrame::EventType::PointerDown;
  CHECK(callback->onEvent(pointerDown));

  PrimeFrame::Event pointerLeave;
  pointerLeave.type = PrimeFrame::EventType::PointerLeave;
  CHECK(callback->onEvent(pointerLeave));

  PrimeFrame::Event pointerUp;
  pointerUp.type = PrimeFrame::EventType::PointerUp;
  pointerUp.localX = 8.0f;
  pointerUp.localY = 8.0f;
  pointerUp.targetW = 20.0f;
  pointerUp.targetH = 20.0f;
  CHECK(callback->onEvent(pointerUp));
  CHECK_FALSE(state.value);

  PrimeFrame::Node const* checkNodeAfter = frame.getNode(boxNode->children.front());
  REQUIRE(checkNodeAfter != nullptr);
  CHECK_FALSE(checkNodeAfter->visible);
}

TEST_CASE("PrimeStage checkbox binding overload pointer cancel suppresses activation") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::State<bool> state;
  state.value = false;
  PrimeStage::UiNode checkbox = root.createCheckbox("Cancel", PrimeStage::bind(state));

  PrimeFrame::Node const* rowNode = frame.getNode(checkbox.nodeId());
  REQUIRE(rowNode != nullptr);
  REQUIRE(rowNode->callbacks != PrimeFrame::InvalidCallbackId);
  REQUIRE(!rowNode->children.empty());

  PrimeFrame::Node const* boxNode = frame.getNode(rowNode->children.front());
  REQUIRE(boxNode != nullptr);
  REQUIRE(!boxNode->children.empty());
  PrimeFrame::Node const* checkNodeBefore = frame.getNode(boxNode->children.front());
  REQUIRE(checkNodeBefore != nullptr);
  CHECK_FALSE(checkNodeBefore->visible);

  PrimeFrame::Callback const* callback = frame.getCallback(rowNode->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  PrimeFrame::Event pointerDown;
  pointerDown.type = PrimeFrame::EventType::PointerDown;
  CHECK(callback->onEvent(pointerDown));

  PrimeFrame::Event pointerCancel;
  pointerCancel.type = PrimeFrame::EventType::PointerCancel;
  CHECK(callback->onEvent(pointerCancel));

  PrimeFrame::Event pointerUp;
  pointerUp.type = PrimeFrame::EventType::PointerUp;
  pointerUp.localX = 8.0f;
  pointerUp.localY = 8.0f;
  pointerUp.targetW = 20.0f;
  pointerUp.targetH = 20.0f;
  CHECK(callback->onEvent(pointerUp));
  CHECK_FALSE(state.value);

  PrimeFrame::Node const* checkNodeAfter = frame.getNode(boxNode->children.front());
  REQUIRE(checkNodeAfter != nullptr);
  CHECK_FALSE(checkNodeAfter->visible);
}

TEST_CASE("PrimeStage interactive helper overloads build expected widgets") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::SizeSpec buttonSize;
  buttonSize.preferredWidth = 120.0f;
  buttonSize.preferredHeight = 28.0f;
  PrimeStage::UiNode button = root.createButton("Apply", 601u, 602u, buttonSize);
  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  CHECK(firstRectToken(frame, button.nodeId()) == 601u);
  CHECK(firstChildText(frame, button.nodeId()) == "Apply");
  CHECK(buttonNode->focusable);

  PrimeStage::TextFieldState fieldState;
  fieldState.text = "Prime";
  fieldState.cursor = static_cast<uint32_t>(fieldState.text.size());
  PrimeStage::SizeSpec fieldSize;
  fieldSize.preferredWidth = 180.0f;
  fieldSize.preferredHeight = 24.0f;
  PrimeStage::UiNode field = root.createTextField(fieldState, "Name", 611u, 612u, fieldSize);
  PrimeFrame::Node const* fieldNode = frame.getNode(field.nodeId());
  REQUIRE(fieldNode != nullptr);
  CHECK(firstRectToken(frame, field.nodeId()) == 611u);
  CHECK(firstChildText(frame, field.nodeId()) == "Prime");
  CHECK(fieldNode->focusable);

  PrimeStage::SizeSpec toggleSize;
  toggleSize.preferredWidth = 48.0f;
  toggleSize.preferredHeight = 24.0f;
  PrimeStage::UiNode toggle = root.createToggle(true, 621u, 622u, toggleSize);
  CHECK(firstRectToken(frame, toggle.nodeId()) == 621u);
  CHECK(findRectPrimitiveByToken(frame, toggle.nodeId(), 622u) != nullptr);

  PrimeStage::SizeSpec checkboxSize;
  checkboxSize.preferredWidth = 180.0f;
  checkboxSize.preferredHeight = 24.0f;
  PrimeStage::UiNode checkbox =
      root.createCheckbox("Enable", true, 631u, 632u, 633u, checkboxSize);
  CHECK(findRectPrimitiveByToken(frame, checkbox.nodeId(), 631u) != nullptr);
  CHECK(findRectPrimitiveByToken(frame, checkbox.nodeId(), 632u) != nullptr);
  CHECK(firstChildText(frame, checkbox.nodeId()) == "Enable");

  PrimeStage::SizeSpec sliderSize;
  sliderSize.preferredWidth = 120.0f;
  sliderSize.preferredHeight = 20.0f;
  PrimeStage::UiNode slider =
      root.createSlider(2.0f, false, 641u, 642u, 643u, sliderSize);
  CHECK(firstRectToken(frame, slider.nodeId()) == 641u);
  PrimeFrame::Primitive const* fill = findRectPrimitiveByToken(frame, slider.nodeId(), 642u);
  PrimeFrame::Primitive const* thumb = findRectPrimitiveByToken(frame, slider.nodeId(), 643u);
  REQUIRE(fill != nullptr);
  REQUIRE(thumb != nullptr);
  CHECK(fill->width == doctest::Approx(120.0f));
}

TEST_CASE("PrimeStage collection helpers and list adapter build expected widgets") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::SizeSpec scrollSize;
  scrollSize.preferredWidth = 220.0f;
  scrollSize.preferredHeight = 120.0f;
  PrimeStage::ScrollView scrollView = root.createScrollView(scrollSize, true, false);
  CHECK(frame.getNode(scrollView.root.nodeId()) != nullptr);
  CHECK(frame.getNode(scrollView.content.nodeId()) != nullptr);

  std::vector<PrimeStage::TableColumn> columns;
  columns.push_back({"Name", 0.0f, 701u, 702u});
  std::vector<std::vector<std::string_view>> rows;
  rows.push_back({"Alpha"});
  rows.push_back({"Beta"});
  PrimeStage::SizeSpec tableSize;
  tableSize.preferredWidth = 240.0f;
  tableSize.preferredHeight = 120.0f;
  PrimeStage::UiNode table = root.createTable(std::move(columns), std::move(rows), 0, tableSize);
  PrimeFrame::Node const* tableNode = frame.getNode(table.nodeId());
  REQUIRE(tableNode != nullptr);
  CHECK(countTextValue(frame, table.nodeId(), "Alpha") == 1u);
  CHECK(countTextValue(frame, table.nodeId(), "Beta") == 1u);

  std::vector<PrimeStage::TreeNode> nodes;
  nodes.push_back(PrimeStage::TreeNode{"Root", {}, true, false});
  PrimeStage::SizeSpec treeSize;
  treeSize.preferredWidth = 220.0f;
  treeSize.preferredHeight = 140.0f;
  PrimeStage::UiNode tree = root.createTreeView(std::move(nodes), treeSize);
  CHECK(frame.getNode(tree.nodeId()) != nullptr);
  CHECK(countTextValue(frame, tree.nodeId(), "Root") == 1u);

  PrimeStage::ListSpec invalidList;
  invalidList.items = {"One", "Two"};
  invalidList.selectedIndex = 99;
  invalidList.selectionStyle = 711u;
  invalidList.focusStyle = 714u;
  invalidList.rowStyle = 712u;
  invalidList.rowAltStyle = 713u;
  invalidList.size.preferredWidth = 220.0f;
  invalidList.size.preferredHeight = 100.0f;
  PrimeStage::UiNode invalidListNode = root.createList(invalidList);
  CHECK(countRectToken(frame, invalidListNode.nodeId(), invalidList.selectionStyle) == 0u);

  int clickedRow = -1;
  std::string clickedItem;
  PrimeStage::ListSpec listSpec;
  listSpec.items = {"One", "Two"};
  listSpec.selectedIndex = 1;
  listSpec.selectionStyle = 721u;
  listSpec.focusStyle = 724u;
  listSpec.rowStyle = 722u;
  listSpec.rowAltStyle = 723u;
  listSpec.size.preferredWidth = 220.0f;
  listSpec.size.preferredHeight = 100.0f;
  listSpec.callbacks.onSelected = [&](PrimeStage::ListRowInfo const& info) {
    clickedRow = info.rowIndex;
    clickedItem = std::string(info.item);
  };
  PrimeStage::UiNode list = root.createList(listSpec);
  CHECK(countRectToken(frame, list.nodeId(), listSpec.selectionStyle) == 1u);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::LayoutOut const* listOut = layout.get(list.nodeId());
  REQUIRE(listOut != nullptr);
  float clickX = listOut->absX + listOut->absW * 0.5f;
  float clickY = listOut->absY + listSpec.rowHeight + listSpec.rowGap + listSpec.rowHeight * 0.5f;
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, clickX, clickY),
                  frame,
                  layout,
                  &focus);
  CHECK(clickedRow == 1);
  CHECK(clickedItem == "Two");
}

TEST_CASE("PrimeStage tabs clamp invalid selected index") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TabsSpec spec;
  spec.labels = {"One", "Two", "Three"};
  spec.selectedIndex = 999;
  spec.tabStyle = 201u;
  spec.activeTabStyle = 202u;
  spec.gap = -2.0f;
  spec.tabPaddingX = -8.0f;
  spec.tabPaddingY = -4.0f;
  spec.size.preferredHeight = 24.0f;

  PrimeStage::UiNode tabs = root.createTabs(spec);
  PrimeFrame::Node const* row = frame.getNode(tabs.nodeId());
  REQUIRE(row != nullptr);
  REQUIRE(row->children.size() == 3u);

  CHECK(firstRectToken(frame, row->children[0]) == spec.tabStyle);
  CHECK(firstRectToken(frame, row->children[1]) == spec.tabStyle);
  CHECK(firstRectToken(frame, row->children[2]) == spec.activeTabStyle);
}

TEST_CASE("PrimeStage dropdown clamps invalid selected index") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::DropdownSpec spec;
  spec.options = {"Alpha", "Beta"};
  spec.selectedIndex = 42;
  spec.paddingX = -9.0f;
  spec.indicatorGap = -3.0f;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 24.0f;

  PrimeStage::UiNode dropdown = root.createDropdown(spec);
  std::string label = firstChildText(frame, dropdown.nodeId());
  CHECK(label == "Beta");
}

TEST_CASE("PrimeStage text field clamps out-of-range indices and negative cursor width") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TextFieldState state;
  state.text = "Prime";
  state.cursor = 200u;
  state.selectionAnchor = 100u;
  state.selectionStart = 150u;
  state.selectionEnd = 250u;
  state.focused = true;
  state.cursorVisible = true;

  PrimeStage::TextFieldSpec spec;
  spec.state = &state;
  spec.paddingX = -12.0f;
  spec.cursorWidth = -4.0f;
  spec.cursorStyle = 301u;
  spec.selectionStyle = 302u;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 24.0f;

  PrimeStage::UiNode field = root.createTextField(spec);
  REQUIRE(state.cursor == 5u);
  REQUIRE(state.selectionAnchor == 5u);
  REQUIRE(state.selectionStart == 5u);
  REQUIRE(state.selectionEnd == 5u);

  PrimeFrame::Primitive const* cursor = findRectPrimitiveByToken(frame, field.nodeId(), spec.cursorStyle);
  REQUIRE(cursor != nullptr);
  CHECK(cursor->width == doctest::Approx(0.0f));
}

TEST_CASE("PrimeStage table clamps invalid selected row to none") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TableSpec spec;
  spec.columns = {{"Name", 0.0f, 0u, 0u}};
  spec.rows = {{"Row A"}, {"Row B"}};
  spec.selectedRow = 99;
  spec.selectionStyle = 501u;
  spec.focusStyle = 502u;
  spec.rowHeight = -20.0f;
  spec.headerHeight = -8.0f;
  spec.headerInset = -4.0f;
  spec.rowGap = -2.0f;
  spec.headerPaddingX = -6.0f;
  spec.cellPaddingX = -7.0f;
  spec.size.preferredWidth = 260.0f;
  spec.size.preferredHeight = 120.0f;

  PrimeStage::UiNode table = root.createTable(spec);
  CHECK(countRectToken(frame, table.nodeId(), spec.selectionStyle) == 0u);
}

TEST_CASE("PrimeStage widget-spec sanitization keeps invariants under deterministic fuzz") {
  std::mt19937_64 rng(SanitizationFuzzSeed);
  std::array<std::string_view, 5> itemPool = {"A", "B", "C", "D", "E"};

  for (int iteration = 0; iteration < SanitizationFuzzIterations; ++iteration) {
    PrimeStage::State<float> sliderBindingState;
    sliderBindingState.value = fuzzFloat(rng, -4.0f, 4.0f);
    PrimeStage::SliderState sliderState;
    sliderState.value = fuzzFloat(rng, -4.0f, 4.0f);
    PrimeStage::SliderSpec slider;
    fuzzSizeSpec(slider.size, rng);
    slider.value = fuzzFloat(rng, -4.0f, 4.0f);
    slider.trackThickness = fuzzFloat(rng, -12.0f, 12.0f);
    slider.thumbSize = fuzzFloat(rng, -12.0f, 12.0f);
    slider.fillHoverOpacity = fuzzOptionalFloat(rng, -2.0f, 2.0f);
    slider.fillPressedOpacity = fuzzOptionalFloat(rng, -2.0f, 2.0f);
    slider.trackHoverOpacity = fuzzOptionalFloat(rng, -2.0f, 2.0f);
    slider.trackPressedOpacity = fuzzOptionalFloat(rng, -2.0f, 2.0f);
    slider.thumbHoverOpacity = fuzzOptionalFloat(rng, -2.0f, 2.0f);
    slider.thumbPressedOpacity = fuzzOptionalFloat(rng, -2.0f, 2.0f);
    slider.tabIndex = fuzzInt(rng, -12, 12);
    int sliderMode = fuzzInt(rng, 0, 2);
    if (sliderMode == 1) {
      slider.binding = PrimeStage::bind(sliderBindingState);
    } else if (sliderMode == 2) {
      slider.state = &sliderState;
    }
    PrimeStage::SliderSpec normalizedSlider = PrimeStage::Internal::normalizeSliderSpec(slider);
    checkSanitizedSizeSpec(normalizedSlider.size);
    CHECK(inUnitInterval(normalizedSlider.value));
    CHECK(normalizedSlider.trackThickness >= 0.0f);
    CHECK(normalizedSlider.thumbSize >= 0.0f);
    CHECK(normalizedSlider.tabIndex == expectedTabIndex(slider.tabIndex));
    if (normalizedSlider.fillHoverOpacity.has_value()) {
      CHECK(inUnitInterval(*normalizedSlider.fillHoverOpacity));
    }
    if (normalizedSlider.fillPressedOpacity.has_value()) {
      CHECK(inUnitInterval(*normalizedSlider.fillPressedOpacity));
    }
    if (normalizedSlider.trackHoverOpacity.has_value()) {
      CHECK(inUnitInterval(*normalizedSlider.trackHoverOpacity));
    }
    if (normalizedSlider.trackPressedOpacity.has_value()) {
      CHECK(inUnitInterval(*normalizedSlider.trackPressedOpacity));
    }
    if (normalizedSlider.thumbHoverOpacity.has_value()) {
      CHECK(inUnitInterval(*normalizedSlider.thumbHoverOpacity));
    }
    if (normalizedSlider.thumbPressedOpacity.has_value()) {
      CHECK(inUnitInterval(*normalizedSlider.thumbPressedOpacity));
    }
    if (sliderMode == 1) {
      CHECK(sliderBindingState.value == doctest::Approx(normalizedSlider.value));
    } else if (sliderMode == 2) {
      CHECK(sliderState.value == doctest::Approx(normalizedSlider.value));
    }

    PrimeStage::State<float> progressBindingState;
    progressBindingState.value = fuzzFloat(rng, -4.0f, 4.0f);
    PrimeStage::ProgressBarState progressState;
    progressState.value = fuzzFloat(rng, -4.0f, 4.0f);
    PrimeStage::ProgressBarSpec progress;
    fuzzSizeSpec(progress.size, rng);
    progress.value = fuzzFloat(rng, -4.0f, 4.0f);
    progress.minFillWidth = fuzzFloat(rng, -16.0f, 16.0f);
    progress.tabIndex = fuzzInt(rng, -12, 12);
    int progressMode = fuzzInt(rng, 0, 2);
    if (progressMode == 1) {
      progress.binding = PrimeStage::bind(progressBindingState);
    } else if (progressMode == 2) {
      progress.state = &progressState;
    }
    PrimeStage::ProgressBarSpec normalizedProgress =
        PrimeStage::Internal::normalizeProgressBarSpec(progress);
    checkSanitizedSizeSpec(normalizedProgress.size);
    CHECK(inUnitInterval(normalizedProgress.value));
    CHECK(normalizedProgress.minFillWidth >= 0.0f);
    CHECK(normalizedProgress.tabIndex == expectedTabIndex(progress.tabIndex));
    if (progressMode == 1) {
      CHECK(progressBindingState.value == doctest::Approx(normalizedProgress.value));
    } else if (progressMode == 2) {
      CHECK(progressState.value == doctest::Approx(normalizedProgress.value));
    }

    int tabCount = fuzzInt(rng, 0, static_cast<int>(itemPool.size()));
    std::vector<std::string_view> labels(itemPool.begin(), itemPool.begin() + tabCount);
    PrimeStage::State<int> tabsBindingState;
    tabsBindingState.value = fuzzInt(rng, -20, 20);
    PrimeStage::TabsState tabsState;
    tabsState.selectedIndex = fuzzInt(rng, -20, 20);
    PrimeStage::TabsSpec tabs;
    tabs.labels = labels;
    fuzzSizeSpec(tabs.size, rng);
    tabs.selectedIndex = fuzzInt(rng, -20, 20);
    tabs.tabPaddingX = fuzzFloat(rng, -20.0f, 20.0f);
    tabs.tabPaddingY = fuzzFloat(rng, -20.0f, 20.0f);
    tabs.gap = fuzzFloat(rng, -20.0f, 20.0f);
    tabs.tabIndex = fuzzInt(rng, -12, 12);
    int tabsMode = fuzzInt(rng, 0, 2);
    if (tabsMode == 1) {
      tabs.binding = PrimeStage::bind(tabsBindingState);
    } else if (tabsMode == 2) {
      tabs.state = &tabsState;
    }
    int tabsInputSelected = tabs.selectedIndex;
    int tabsBindingInput = tabsBindingState.value;
    int tabsStateInput = tabsState.selectedIndex;
    PrimeStage::TabsSpec normalizedTabs = PrimeStage::Internal::normalizeTabsSpec(tabs);
    checkSanitizedSizeSpec(normalizedTabs.size);
    CHECK(normalizedTabs.tabPaddingX >= 0.0f);
    CHECK(normalizedTabs.tabPaddingY >= 0.0f);
    CHECK(normalizedTabs.gap >= 0.0f);
    CHECK(normalizedTabs.tabIndex == expectedTabIndex(tabs.tabIndex));
    CHECK(normalizedTabs.selectedIndex == expectedSelectedIndex(
                                          tabsMode == 1 ? tabsBindingInput
                                                        : (tabsMode == 2 ? tabsStateInput
                                                                         : tabsInputSelected),
                                          tabCount));
    if (tabsMode == 1) {
      CHECK(tabsBindingState.value == normalizedTabs.selectedIndex);
    } else if (tabsMode == 2) {
      CHECK(tabsState.selectedIndex == normalizedTabs.selectedIndex);
    }

    int optionCount = fuzzInt(rng, 0, static_cast<int>(itemPool.size()));
    std::vector<std::string_view> options(itemPool.begin(), itemPool.begin() + optionCount);
    PrimeStage::State<int> dropdownBindingState;
    dropdownBindingState.value = fuzzInt(rng, -20, 20);
    PrimeStage::DropdownState dropdownState;
    dropdownState.selectedIndex = fuzzInt(rng, -20, 20);
    PrimeStage::DropdownSpec dropdown;
    dropdown.options = options;
    fuzzSizeSpec(dropdown.size, rng);
    dropdown.selectedIndex = fuzzInt(rng, -20, 20);
    dropdown.paddingX = fuzzFloat(rng, -20.0f, 20.0f);
    dropdown.indicatorGap = fuzzFloat(rng, -20.0f, 20.0f);
    dropdown.tabIndex = fuzzInt(rng, -12, 12);
    int dropdownMode = fuzzInt(rng, 0, 2);
    if (dropdownMode == 1) {
      dropdown.binding = PrimeStage::bind(dropdownBindingState);
    } else if (dropdownMode == 2) {
      dropdown.state = &dropdownState;
    }
    int dropdownInputSelected = dropdown.selectedIndex;
    int dropdownBindingInput = dropdownBindingState.value;
    int dropdownStateInput = dropdownState.selectedIndex;
    PrimeStage::DropdownSpec normalizedDropdown = PrimeStage::Internal::normalizeDropdownSpec(dropdown);
    checkSanitizedSizeSpec(normalizedDropdown.size);
    CHECK(normalizedDropdown.paddingX >= 0.0f);
    CHECK(normalizedDropdown.indicatorGap >= 0.0f);
    CHECK(normalizedDropdown.tabIndex == expectedTabIndex(dropdown.tabIndex));
    CHECK(normalizedDropdown.selectedIndex == expectedSelectedIndex(
                                               dropdownMode == 1 ? dropdownBindingInput
                                                                 : (dropdownMode == 2
                                                                        ? dropdownStateInput
                                                                        : dropdownInputSelected),
                                               optionCount));
    if (dropdownMode == 1) {
      CHECK(dropdownBindingState.value == normalizedDropdown.selectedIndex);
    } else if (dropdownMode == 2) {
      CHECK(dropdownState.selectedIndex == normalizedDropdown.selectedIndex);
    }

    int itemCount = fuzzInt(rng, 0, static_cast<int>(itemPool.size()));
    PrimeStage::ListSpec list;
    list.items.assign(itemPool.begin(), itemPool.begin() + itemCount);
    fuzzSizeSpec(list.size, rng);
    list.rowHeight = fuzzFloat(rng, -32.0f, 32.0f);
    list.rowGap = fuzzFloat(rng, -32.0f, 32.0f);
    list.rowPaddingX = fuzzFloat(rng, -32.0f, 32.0f);
    list.selectedIndex = fuzzInt(rng, -20, 20);
    list.tabIndex = fuzzInt(rng, -12, 12);
    PrimeStage::ListSpec normalizedList = PrimeStage::Internal::normalizeListSpec(list);
    checkSanitizedSizeSpec(normalizedList.size);
    CHECK(normalizedList.rowHeight >= 0.0f);
    CHECK(normalizedList.rowGap >= 0.0f);
    CHECK(normalizedList.rowPaddingX >= 0.0f);
    CHECK(normalizedList.tabIndex == expectedTabIndex(list.tabIndex));
    CHECK(normalizedList.selectedIndex == expectedSelectedRowOrNone(list.selectedIndex, itemCount));

    int rowCount = fuzzInt(rng, 0, static_cast<int>(itemPool.size()));
    PrimeStage::TableSpec table;
    table.columns = {{"Name", 0.0f, 0u, 0u}};
    table.rows.clear();
    for (int row = 0; row < rowCount; ++row) {
      table.rows.push_back({itemPool[static_cast<size_t>(row)]});
    }
    fuzzSizeSpec(table.size, rng);
    table.headerInset = fuzzFloat(rng, -32.0f, 32.0f);
    table.headerHeight = fuzzFloat(rng, -32.0f, 32.0f);
    table.rowHeight = fuzzFloat(rng, -32.0f, 32.0f);
    table.rowGap = fuzzFloat(rng, -32.0f, 32.0f);
    table.headerPaddingX = fuzzFloat(rng, -32.0f, 32.0f);
    table.cellPaddingX = fuzzFloat(rng, -32.0f, 32.0f);
    table.selectedRow = fuzzInt(rng, -20, 20);
    table.tabIndex = fuzzInt(rng, -12, 12);
    PrimeStage::TableSpec normalizedTable = PrimeStage::Internal::normalizeTableSpec(table);
    checkSanitizedSizeSpec(normalizedTable.size);
    CHECK(normalizedTable.headerInset >= 0.0f);
    CHECK(normalizedTable.headerHeight >= 0.0f);
    CHECK(normalizedTable.rowHeight >= 0.0f);
    CHECK(normalizedTable.rowGap >= 0.0f);
    CHECK(normalizedTable.headerPaddingX >= 0.0f);
    CHECK(normalizedTable.cellPaddingX >= 0.0f);
    CHECK(normalizedTable.tabIndex == expectedTabIndex(table.tabIndex));
    CHECK(normalizedTable.selectedRow == expectedSelectedRowOrNone(table.selectedRow, rowCount));

    PrimeStage::ButtonSpec button;
    fuzzSizeSpec(button.size, rng);
    button.textInsetX = fuzzFloat(rng, -32.0f, 32.0f);
    button.baseOpacity = fuzzFloat(rng, -2.0f, 2.0f);
    button.hoverOpacity = fuzzFloat(rng, -2.0f, 2.0f);
    button.pressedOpacity = fuzzFloat(rng, -2.0f, 2.0f);
    button.tabIndex = fuzzInt(rng, -12, 12);
    PrimeStage::ButtonSpec normalizedButton = PrimeStage::Internal::normalizeButtonSpec(button);
    checkSanitizedSizeSpec(normalizedButton.size);
    CHECK(normalizedButton.textInsetX >= 0.0f);
    CHECK(inUnitInterval(normalizedButton.baseOpacity));
    CHECK(inUnitInterval(normalizedButton.hoverOpacity));
    CHECK(inUnitInterval(normalizedButton.pressedOpacity));
    CHECK(normalizedButton.tabIndex == expectedTabIndex(button.tabIndex));

    PrimeStage::TextFieldSpec field;
    fuzzSizeSpec(field.size, rng);
    field.paddingX = fuzzFloat(rng, -32.0f, 32.0f);
    field.cursorWidth = fuzzFloat(rng, -8.0f, 8.0f);
    field.cursorBlinkInterval = std::chrono::milliseconds(fuzzInt(rng, -2000, 2000));
    field.tabIndex = fuzzInt(rng, -12, 12);
    PrimeStage::TextFieldSpec normalizedField = PrimeStage::Internal::normalizeTextFieldSpec(field);
    checkSanitizedSizeSpec(normalizedField.size);
    CHECK(normalizedField.paddingX >= 0.0f);
    CHECK(normalizedField.cursorWidth >= 0.0f);
    CHECK(normalizedField.cursorBlinkInterval.count() >= 0);
    CHECK(normalizedField.tabIndex == expectedTabIndex(field.tabIndex));

    PrimeStage::PanelSpec panel;
    fuzzSizeSpec(panel.size, rng);
    panel.padding.left = fuzzFloat(rng, -32.0f, 32.0f);
    panel.padding.top = fuzzFloat(rng, -32.0f, 32.0f);
    panel.padding.right = fuzzFloat(rng, -32.0f, 32.0f);
    panel.padding.bottom = fuzzFloat(rng, -32.0f, 32.0f);
    panel.gap = fuzzFloat(rng, -32.0f, 32.0f);
    PrimeStage::PanelSpec normalizedPanel = PrimeStage::Internal::normalizePanelSpec(panel);
    checkSanitizedSizeSpec(normalizedPanel.size);
    CHECK(normalizedPanel.padding.left >= 0.0f);
    CHECK(normalizedPanel.padding.top >= 0.0f);
    CHECK(normalizedPanel.padding.right >= 0.0f);
    CHECK(normalizedPanel.padding.bottom >= 0.0f);
    CHECK(normalizedPanel.gap >= 0.0f);

    PrimeStage::WindowSpec window;
    window.width = fuzzFloat(rng, -320.0f, 320.0f);
    window.height = fuzzFloat(rng, -320.0f, 320.0f);
    window.minWidth = fuzzFloat(rng, -320.0f, 320.0f);
    window.minHeight = fuzzFloat(rng, -320.0f, 320.0f);
    window.titleBarHeight = fuzzFloat(rng, -64.0f, 64.0f);
    window.contentPadding = fuzzFloat(rng, -64.0f, 64.0f);
    window.resizeHandleSize = fuzzFloat(rng, -64.0f, 64.0f);
    window.tabIndex = fuzzInt(rng, -12, 12);
    PrimeStage::WindowSpec normalizedWindow = PrimeStage::Internal::normalizeWindowSpec(window);
    CHECK(normalizedWindow.width >= 0.0f);
    CHECK(normalizedWindow.height >= 0.0f);
    CHECK(normalizedWindow.minWidth >= 0.0f);
    CHECK(normalizedWindow.minHeight >= 0.0f);
    CHECK(normalizedWindow.width >= normalizedWindow.minWidth);
    CHECK(normalizedWindow.height >= normalizedWindow.minHeight);
    CHECK(normalizedWindow.titleBarHeight >= 0.0f);
    CHECK(normalizedWindow.contentPadding >= 0.0f);
    CHECK(normalizedWindow.resizeHandleSize >= 0.0f);
    CHECK(normalizedWindow.tabIndex == expectedTabIndex(window.tabIndex));

    PrimeStage::ScrollViewSpec scroll;
    fuzzSizeSpec(scroll.size, rng);
    scroll.vertical.thickness = fuzzFloat(rng, -32.0f, 32.0f);
    scroll.vertical.inset = fuzzFloat(rng, -32.0f, 32.0f);
    scroll.vertical.startPadding = fuzzFloat(rng, -32.0f, 32.0f);
    scroll.vertical.endPadding = fuzzFloat(rng, -32.0f, 32.0f);
    scroll.vertical.thumbLength = fuzzFloat(rng, -32.0f, 32.0f);
    scroll.vertical.thumbOffset = fuzzFloat(rng, -32.0f, 32.0f);
    scroll.horizontal.thickness = fuzzFloat(rng, -32.0f, 32.0f);
    scroll.horizontal.inset = fuzzFloat(rng, -32.0f, 32.0f);
    scroll.horizontal.startPadding = fuzzFloat(rng, -32.0f, 32.0f);
    scroll.horizontal.endPadding = fuzzFloat(rng, -32.0f, 32.0f);
    scroll.horizontal.thumbLength = fuzzFloat(rng, -32.0f, 32.0f);
    scroll.horizontal.thumbOffset = fuzzFloat(rng, -32.0f, 32.0f);
    PrimeStage::ScrollViewSpec normalizedScroll = PrimeStage::Internal::normalizeScrollViewSpec(scroll);
    checkSanitizedSizeSpec(normalizedScroll.size);
    CHECK(normalizedScroll.vertical.thickness >= 0.0f);
    CHECK(normalizedScroll.vertical.inset >= 0.0f);
    CHECK(normalizedScroll.vertical.startPadding >= 0.0f);
    CHECK(normalizedScroll.vertical.endPadding >= 0.0f);
    CHECK(normalizedScroll.vertical.thumbLength >= 0.0f);
    CHECK(normalizedScroll.vertical.thumbOffset >= 0.0f);
    CHECK(normalizedScroll.horizontal.thickness >= 0.0f);
    CHECK(normalizedScroll.horizontal.inset >= 0.0f);
    CHECK(normalizedScroll.horizontal.startPadding >= 0.0f);
    CHECK(normalizedScroll.horizontal.endPadding >= 0.0f);
    CHECK(normalizedScroll.horizontal.thumbLength >= 0.0f);
    CHECK(normalizedScroll.horizontal.thumbOffset >= 0.0f);
  }
}

TEST_CASE("PrimeStage widget-spec sanitization regression corpus preserves invariants") {
  std::vector<std::function<void()>> corpus;

  corpus.push_back([]() {
    PrimeStage::State<float> bindingState;
    bindingState.value = -0.25f;
    PrimeStage::SliderSpec spec;
    spec.value = 2.5f;
    spec.binding = PrimeStage::bind(bindingState);
    PrimeStage::SliderSpec normalized = PrimeStage::Internal::normalizeSliderSpec(spec);
    CHECK(normalized.value == doctest::Approx(0.0f));
    CHECK(bindingState.value == doctest::Approx(0.0f));
  });

  corpus.push_back([]() {
    PrimeStage::ProgressBarState state;
    state.value = 1.75f;
    PrimeStage::ProgressBarSpec spec;
    spec.state = &state;
    spec.minFillWidth = -4.0f;
    PrimeStage::ProgressBarSpec normalized = PrimeStage::Internal::normalizeProgressBarSpec(spec);
    CHECK(normalized.value == doctest::Approx(1.0f));
    CHECK(state.value == doctest::Approx(1.0f));
    CHECK(normalized.minFillWidth == doctest::Approx(0.0f));
  });

  corpus.push_back([]() {
    PrimeStage::TabsState state;
    state.selectedIndex = -9;
    PrimeStage::TabsSpec spec;
    spec.labels = {};
    spec.state = &state;
    PrimeStage::TabsSpec normalized = PrimeStage::Internal::normalizeTabsSpec(spec);
    CHECK(normalized.selectedIndex == 0);
    CHECK(state.selectedIndex == 0);
  });

  corpus.push_back([]() {
    PrimeStage::State<int> bindingState;
    bindingState.value = 99;
    PrimeStage::DropdownSpec spec;
    spec.options = {"One", "Two"};
    spec.binding = PrimeStage::bind(bindingState);
    PrimeStage::DropdownSpec normalized = PrimeStage::Internal::normalizeDropdownSpec(spec);
    CHECK(normalized.selectedIndex == 1);
    CHECK(bindingState.value == 1);
  });

  corpus.push_back([]() {
    PrimeStage::ListSpec spec;
    spec.items = {"Alpha", "Beta"};
    spec.selectedIndex = 99;
    PrimeStage::ListSpec normalized = PrimeStage::Internal::normalizeListSpec(spec);
    CHECK(normalized.selectedIndex == -1);
  });

  corpus.push_back([]() {
    PrimeStage::TableSpec spec;
    spec.columns = {{"Name", 0.0f, 0u, 0u}};
    spec.rows = {{"A"}, {"B"}};
    spec.selectedRow = -4;
    PrimeStage::TableSpec normalized = PrimeStage::Internal::normalizeTableSpec(spec);
    CHECK(normalized.selectedRow == -1);
  });

  corpus.push_back([]() {
    PrimeStage::TextFieldSpec spec;
    spec.cursorBlinkInterval = std::chrono::milliseconds(-300);
    spec.cursorWidth = -2.0f;
    PrimeStage::TextFieldSpec normalized = PrimeStage::Internal::normalizeTextFieldSpec(spec);
    CHECK(normalized.cursorBlinkInterval.count() == 0);
    CHECK(normalized.cursorWidth == doctest::Approx(0.0f));
  });

  corpus.push_back([]() {
    PrimeStage::PanelSpec spec;
    spec.size.minWidth = 120.0f;
    spec.size.maxWidth = 40.0f;
    spec.size.preferredWidth = 10.0f;
    spec.gap = -8.0f;
    PrimeStage::PanelSpec normalized = PrimeStage::Internal::normalizePanelSpec(spec);
    REQUIRE(normalized.size.minWidth.has_value());
    REQUIRE(normalized.size.maxWidth.has_value());
    REQUIRE(normalized.size.preferredWidth.has_value());
    CHECK(normalized.size.maxWidth.value() == doctest::Approx(normalized.size.minWidth.value()));
    CHECK(normalized.size.preferredWidth.value() == doctest::Approx(normalized.size.minWidth.value()));
    CHECK(normalized.gap == doctest::Approx(0.0f));
  });

  corpus.push_back([]() {
    PrimeStage::WindowSpec spec;
    spec.width = 120.0f;
    spec.height = 80.0f;
    spec.minWidth = 220.0f;
    spec.minHeight = 140.0f;
    spec.resizeHandleSize = -3.0f;
    PrimeStage::WindowSpec normalized = PrimeStage::Internal::normalizeWindowSpec(spec);
    CHECK(normalized.width == doctest::Approx(220.0f));
    CHECK(normalized.height == doctest::Approx(140.0f));
    CHECK(normalized.resizeHandleSize == doctest::Approx(0.0f));
  });

  for (auto const& run : corpus) {
    run();
  }
}

TEST_CASE("PrimeStage toggle and checkbox normalization prefer binding state and clamp geometry") {
  PrimeStage::State<bool> toggleBinding;
  toggleBinding.value = true;
  PrimeStage::ToggleState toggleState;
  toggleState.on = false;
  PrimeStage::ToggleSpec toggleSpec;
  toggleSpec.on = false;
  toggleSpec.state = &toggleState;
  toggleSpec.binding = PrimeStage::bind(toggleBinding);
  toggleSpec.knobInset = -2.5f;
  PrimeStage::ToggleSpec normalizedToggle = PrimeStage::Internal::normalizeToggleSpec(toggleSpec);
  CHECK(normalizedToggle.on);
  CHECK(normalizedToggle.knobInset == doctest::Approx(0.0f));
  CHECK_FALSE(toggleState.on);
  CHECK(toggleBinding.value);
  CHECK(normalizedToggle.accessibility.role == PrimeStage::AccessibilityRole::Toggle);
  REQUIRE(normalizedToggle.accessibility.state.checked.has_value());
  CHECK(normalizedToggle.accessibility.state.checked.value());

  PrimeStage::State<bool> checkboxBinding;
  checkboxBinding.value = false;
  PrimeStage::CheckboxState checkboxState;
  checkboxState.checked = true;
  PrimeStage::CheckboxSpec checkboxSpec;
  checkboxSpec.checked = true;
  checkboxSpec.state = &checkboxState;
  checkboxSpec.binding = PrimeStage::bind(checkboxBinding);
  checkboxSpec.boxSize = -4.0f;
  checkboxSpec.checkInset = -3.0f;
  checkboxSpec.gap = -8.0f;
  PrimeStage::CheckboxSpec normalizedCheckbox =
      PrimeStage::Internal::normalizeCheckboxSpec(checkboxSpec);
  CHECK_FALSE(normalizedCheckbox.checked);
  CHECK(normalizedCheckbox.boxSize == doctest::Approx(0.0f));
  CHECK(normalizedCheckbox.checkInset == doctest::Approx(0.0f));
  CHECK(normalizedCheckbox.gap == doctest::Approx(0.0f));
  CHECK(checkboxState.checked);
  CHECK_FALSE(checkboxBinding.value);
  CHECK(normalizedCheckbox.accessibility.role == PrimeStage::AccessibilityRole::Checkbox);
  REQUIRE(normalizedCheckbox.accessibility.state.checked.has_value());
  CHECK_FALSE(normalizedCheckbox.accessibility.state.checked.value());
}

TEST_CASE("PrimeStage accessibility semantics export snapshot covers default disabled and selected contracts") {
  PrimeStage::ButtonSpec buttonDefault;
  PrimeStage::ButtonSpec buttonDisabled;
  buttonDisabled.enabled = false;
  PrimeStage::ButtonSpec buttonSelected;
  buttonSelected.accessibility.state.selected = true;

  PrimeStage::ToggleSpec toggleChecked;
  toggleChecked.on = true;
  PrimeStage::ToggleSpec toggleDisabled;
  toggleDisabled.on = true;
  toggleDisabled.enabled = false;

  PrimeStage::DropdownSpec dropdownSelected;
  dropdownSelected.options = {"Red", "Green", "Blue"};
  dropdownSelected.selectedIndex = 1;

  PrimeStage::TabsSpec tabsSelected;
  tabsSelected.labels = {"A", "B", "C"};
  tabsSelected.selectedIndex = 2;

  PrimeStage::SliderSpec sliderDefault;
  sliderDefault.value = 0.75f;

  PrimeStage::ProgressBarSpec progressDefault;
  progressDefault.value = 0.25f;

  PrimeStage::TableSpec tableDefault;
  tableDefault.columns = {{"Name", 0.0f, 0u, 0u}};
  tableDefault.rows = {{"Row"}};

  PrimeStage::TreeViewSpec treeDefault;
  treeDefault.nodes = {PrimeStage::TreeNode{"Root", {}, true, false}};

  PrimeStage::TextFieldSpec fieldDefault;
  PrimeStage::LabelSpec labelDefault;
  PrimeStage::ListSpec listDefault;
  listDefault.items = {"One", "Two"};
  listDefault.selectedIndex = 1;

  std::string snapshot;
  snapshot += exportSemanticsRow("button.default",
                                 PrimeStage::Internal::normalizeButtonSpec(buttonDefault).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("button.disabled",
                                 PrimeStage::Internal::normalizeButtonSpec(buttonDisabled).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("button.selected_override",
                                 PrimeStage::Internal::normalizeButtonSpec(buttonSelected).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("toggle.checked",
                                 PrimeStage::Internal::normalizeToggleSpec(toggleChecked).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("toggle.disabled_checked",
                                 PrimeStage::Internal::normalizeToggleSpec(toggleDisabled).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("dropdown.selected_index",
                                 PrimeStage::Internal::normalizeDropdownSpec(dropdownSelected).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("tabs.selected_index",
                                 PrimeStage::Internal::normalizeTabsSpec(tabsSelected).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("slider.default",
                                 PrimeStage::Internal::normalizeSliderSpec(sliderDefault).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("progress.default",
                                 PrimeStage::Internal::normalizeProgressBarSpec(progressDefault).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("table.default",
                                 PrimeStage::Internal::normalizeTableSpec(tableDefault).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("tree.default",
                                 PrimeStage::Internal::normalizeTreeViewSpec(treeDefault).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("text_field.default",
                                 PrimeStage::Internal::normalizeTextFieldSpec(fieldDefault).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("label.default",
                                 PrimeStage::Internal::normalizeLabelSpec(labelDefault).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("list.default",
                                 PrimeStage::Internal::normalizeListSpec(listDefault).accessibility);

  CHECK(snapshot == R"(button.default role=button disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
button.disabled role=button disabled=true checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
button.selected_override role=button disabled=false checked=- selected=true expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
toggle.checked role=toggle disabled=false checked=true selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
toggle.disabled_checked role=toggle disabled=true checked=true selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
dropdown.selected_index role=combo_box disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=2 setSize=3
tabs.selected_index role=tab_list disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=3 setSize=3
slider.default role=slider disabled=false checked=- selected=- expanded=- valueNow=0.75 valueMin=0.00 valueMax=1.00 position=- setSize=-
progress.default role=progress_bar disabled=false checked=- selected=- expanded=- valueNow=0.25 valueMin=0.00 valueMax=1.00 position=- setSize=-
table.default role=table disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
tree.default role=tree disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
text_field.default role=text_field disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
label.default role=static_text disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
list.default role=table disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-)");
}

TEST_CASE("PrimeStage accessibility semantics focus snapshot covers focused and disabled navigation states") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Run";
  buttonSpec.size.preferredWidth = 140.0f;
  buttonSpec.size.preferredHeight = 28.0f;

  PrimeStage::ButtonSpec disabledButtonSpec;
  disabledButtonSpec.label = "Disabled";
  disabledButtonSpec.enabled = false;
  disabledButtonSpec.size.preferredWidth = 140.0f;
  disabledButtonSpec.size.preferredHeight = 28.0f;

  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.text = "Prime";
  fieldSpec.size.preferredWidth = 180.0f;
  fieldSpec.size.preferredHeight = 24.0f;

  PrimeStage::UiNode button = root.createButton(buttonSpec);
  PrimeStage::UiNode disabledButton = root.createButton(disabledButtonSpec);
  PrimeStage::UiNode field = root.createTextField(fieldSpec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::FocusManager focus;
  REQUIRE(focus.setActiveRoot(frame, layout, root.nodeId()));

  PrimeStage::AccessibilitySemantics buttonSemantics =
      PrimeStage::Internal::normalizeButtonSpec(buttonSpec).accessibility;
  PrimeStage::AccessibilitySemantics disabledButtonSemantics =
      PrimeStage::Internal::normalizeButtonSpec(disabledButtonSpec).accessibility;
  PrimeStage::AccessibilitySemantics fieldSemantics =
      PrimeStage::Internal::normalizeTextFieldSpec(fieldSpec).accessibility;

  std::string snapshot;
  snapshot += exportFocusRow("focus.first.button",
                             buttonSemantics,
                             focus.focusedNode() == button.nodeId()) +
              "\n";
  snapshot += exportFocusRow("focus.first.disabled_button",
                             disabledButtonSemantics,
                             focus.focusedNode() == disabledButton.nodeId()) +
              "\n";
  snapshot += exportFocusRow("focus.first.text_field",
                             fieldSemantics,
                             focus.focusedNode() == field.nodeId()) +
              "\n";

  REQUIRE(focus.handleTab(frame, layout, true));

  snapshot += exportFocusRow("focus.after_tab.button",
                             buttonSemantics,
                             focus.focusedNode() == button.nodeId()) +
              "\n";
  snapshot += exportFocusRow("focus.after_tab.disabled_button",
                             disabledButtonSemantics,
                             focus.focusedNode() == disabledButton.nodeId()) +
              "\n";
  snapshot += exportFocusRow("focus.after_tab.text_field",
                             fieldSemantics,
                             focus.focusedNode() == field.nodeId());

  CHECK(snapshot == R"(focus.first.button role=button focused=true disabled=false
focus.first.disabled_button role=button focused=false disabled=true
focus.first.text_field role=text_field focused=false disabled=false
focus.after_tab.button role=button focused=false disabled=false
focus.after_tab.disabled_button role=button focused=false disabled=true
focus.after_tab.text_field role=text_field focused=true disabled=false)");
}
