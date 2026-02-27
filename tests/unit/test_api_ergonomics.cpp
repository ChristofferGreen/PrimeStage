#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <cmath>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {

constexpr float RootWidth = 320.0f;
constexpr float RootHeight = 180.0f;

struct ListModelRow {
  std::string key;
  std::string label;
};

struct TableModelRow {
  std::string key;
  std::string name;
  std::string type;
  std::string size;
};

struct TreeModelRow {
  std::string key;
  std::string label;
  bool expanded = true;
  bool selected = false;
  std::vector<TreeModelRow> children;
};

PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  if (PrimeFrame::Node* node = frame.getNode(rootId)) {
    node->layout = PrimeFrame::LayoutType::Overlay;
    node->sizeHint.width.preferred = RootWidth;
    node->sizeHint.height.preferred = RootHeight;
  }
  return PrimeStage::UiNode(frame, rootId, true);
}

PrimeFrame::LayoutOutput layoutFrame(PrimeFrame::Frame& frame) {
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = RootWidth;
  options.rootHeight = RootHeight;
  engine.layout(frame, layout, options);
  return layout;
}

PrimeFrame::Event makePointerEvent(PrimeFrame::EventType type, float x, float y) {
  PrimeFrame::Event event;
  event.type = type;
  event.pointerId = 1;
  event.x = x;
  event.y = y;
  return event;
}

float srgbChannelToLinear(float value) {
  value = std::clamp(value, 0.0f, 1.0f);
  if (value <= 0.04045f) {
    return value / 12.92f;
  }
  return std::pow((value + 0.055f) / 1.055f, 2.4f);
}

float contrastRatio(PrimeFrame::Color const& lhs, PrimeFrame::Color const& rhs) {
  auto luminance = [](PrimeFrame::Color const& color) -> float {
    float r = srgbChannelToLinear(color.r);
    float g = srgbChannelToLinear(color.g);
    float b = srgbChannelToLinear(color.b);
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
  };

  float lhsLum = luminance(lhs);
  float rhsLum = luminance(rhs);
  float hi = std::max(lhsLum, rhsLum);
  float lo = std::min(lhsLum, rhsLum);
  return (hi + 0.05f) / (lo + 0.05f);
}

struct ExampleErgonomicsMetrics {
  size_t nonEmptyCodeLines = 0u;
  size_t widgetInstantiationCalls = 0u;
  float averageLinesPerWidget = 0.0f;
};

std::string extractFunctionBody(std::string const& source, std::string_view signature) {
  size_t signaturePos = source.find(signature);
  if (signaturePos == std::string::npos) {
    return {};
  }
  size_t bodyOpen = source.find('{', signaturePos);
  if (bodyOpen == std::string::npos) {
    return {};
  }

  size_t depth = 0u;
  size_t bodyStart = bodyOpen + 1u;
  for (size_t i = bodyOpen; i < source.size(); ++i) {
    if (source[i] == '{') {
      ++depth;
      continue;
    }
    if (source[i] == '}') {
      if (depth == 0u) {
        return {};
      }
      --depth;
      if (depth == 0u) {
        return source.substr(bodyStart, i - bodyStart);
      }
    }
  }
  return {};
}

size_t countNonEmptyCodeLines(std::string const& source) {
  std::istringstream stream(source);
  std::string line;
  size_t count = 0u;
  while (std::getline(stream, line)) {
    size_t begin = line.find_first_not_of(" \t\r");
    if (begin == std::string::npos) {
      continue;
    }
    if (line.compare(begin, 2u, "//") == 0) {
      continue;
    }
    ++count;
  }
  return count;
}

size_t countWidgetInstantiationCalls(std::string const& source) {
  static std::regex const WidgetCallPattern(
      R"(\.(?:create(?:Button|Checkbox|Divider|Dropdown|HorizontalStack|Label|List|Overlay|Panel|Paragraph|ProgressBar|ScrollView|SelectableText|Slider|Spacer|Table|Tabs|TextField|TextLine|Toggle|TreeView|VerticalStack|Window)|button|checkbox|column|divider|dropdown|form|formField|label|overlay|panel|paragraph|progressBar|row|slider|spacer|tabs|textLine|toggle|window)\s*\()",
      std::regex::ECMAScript);
  return static_cast<size_t>(
      std::distance(std::sregex_iterator(source.begin(), source.end(), WidgetCallPattern),
                    std::sregex_iterator()));
}

ExampleErgonomicsMetrics measureExampleErgonomics(std::string const& source,
                                                  std::string_view functionSignature) {
  ExampleErgonomicsMetrics metrics;
  std::string body = extractFunctionBody(source, functionSignature);
  metrics.nonEmptyCodeLines = countNonEmptyCodeLines(body);
  metrics.widgetInstantiationCalls = countWidgetInstantiationCalls(body);
  if (metrics.widgetInstantiationCalls > 0u) {
    metrics.averageLinesPerWidget =
        static_cast<float>(metrics.nonEmptyCodeLines) /
        static_cast<float>(metrics.widgetInstantiationCalls);
  }
  return metrics;
}

} // namespace

static_assert(std::is_base_of_v<PrimeStage::WidgetSpec, PrimeStage::LabelSpec>);
static_assert(std::is_base_of_v<PrimeStage::WidgetSpec, PrimeStage::ParagraphSpec>);
static_assert(std::is_base_of_v<PrimeStage::WidgetSpec, PrimeStage::TextLineSpec>);
static_assert(std::is_base_of_v<PrimeStage::FocusableWidgetSpec, PrimeStage::ButtonSpec>);
static_assert(std::is_base_of_v<PrimeStage::FocusableWidgetSpec, PrimeStage::TextFieldSpec>);
static_assert(std::is_base_of_v<PrimeStage::EnableableWidgetSpec, PrimeStage::SelectableTextSpec>);
static_assert(std::is_base_of_v<PrimeStage::FocusableWidgetSpec, PrimeStage::ToggleSpec>);
static_assert(std::is_base_of_v<PrimeStage::FocusableWidgetSpec, PrimeStage::ProgressBarSpec>);
static_assert(std::is_base_of_v<PrimeStage::FocusableWidgetSpec, PrimeStage::TableSpec>);
static_assert(std::is_base_of_v<PrimeStage::FocusableWidgetSpec, PrimeStage::TreeViewSpec>);

TEST_CASE("PrimeStage installs readable defaults for untouched PrimeFrame themes") {
  PrimeFrame::Frame frame;
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  REQUIRE(theme != nullptr);

  theme->palette.assign(1u, PrimeFrame::Color{0.0f, 0.0f, 0.0f, 1.0f});
  theme->rectStyles.assign(1u, PrimeFrame::RectStyle{0u, 1.0f});
  theme->textStyles.assign(1u, PrimeFrame::TextStyle{});

  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeStage::UiNode root(frame, rootId, true);
  (void)root;

  theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  REQUIRE(theme != nullptr);
  REQUIRE(theme->palette.size() >= 2u);
  REQUIRE(theme->rectStyles.size() >= 1u);
  REQUIRE(theme->textStyles.size() >= 1u);

  size_t fillIndex = theme->rectStyles[0].fill;
  size_t textIndex = theme->textStyles[0].color;
  REQUIRE(fillIndex < theme->palette.size());
  REQUIRE(textIndex < theme->palette.size());

  PrimeFrame::Color fill = theme->palette[fillIndex];
  PrimeFrame::Color text = theme->palette[textIndex];
  float contrast = contrastRatio(text, fill);
  CHECK(contrast >= 4.5f);
}

TEST_CASE("PrimeStage list model adapter binds typed rows and key extractors") {
  std::vector<ListModelRow> rows = {
      {"asset.alpha", "Alpha"},
      {"asset.beta", "Beta"},
      {"asset.gamma", "Gamma"},
  };

  PrimeStage::ListModelAdapter adapter = PrimeStage::makeListModel(
      rows,
      [](ListModelRow const& row) -> std::string_view { return row.label; },
      [](ListModelRow const& row) -> std::string_view { return row.key; });

  CHECK(adapter.items().size() == 3u);
  CHECK(adapter.items()[0] == "Alpha");
  CHECK(adapter.items()[2] == "Gamma");
  CHECK(adapter.keys().size() == 3u);
  CHECK(adapter.keyForRow(0) == PrimeStage::widgetIdentityId("asset.alpha"));
  CHECK(adapter.keyForRow(2) == PrimeStage::widgetIdentityId("asset.gamma"));
  CHECK(adapter.keyForRow(9) == PrimeStage::InvalidWidgetIdentityId);

  PrimeStage::ListSpec list;
  adapter.bind(list);
  CHECK(list.items.size() == 3u);
  CHECK(list.items[1] == "Beta");
}

TEST_CASE("PrimeStage table model adapter binds typed rows and deterministic columns") {
  std::vector<TableModelRow> rows = {
      {"icons.png", "icons.png", "Texture", "512 KB"},
      {"theme.ogg", "theme.ogg", "Audio", "3.1 MB"},
  };

  PrimeStage::TableModelAdapter adapter = PrimeStage::makeTableModel(
      rows,
      3u,
      [](TableModelRow const& row, size_t columnIndex) -> std::string_view {
        switch (columnIndex) {
          case 0:
            return row.name;
          case 1:
            return row.type;
          case 2:
            return row.size;
          default:
            return {};
        }
      },
      [](TableModelRow const& row) -> std::string_view { return row.key; });

  CHECK(adapter.columnCount() == 3u);
  CHECK(adapter.rows().size() == 2u);
  CHECK(adapter.rows()[0].size() == 3u);
  CHECK(adapter.rows()[0][0] == "icons.png");
  CHECK(adapter.rows()[1][1] == "Audio");
  CHECK(adapter.keyForRow(0) == PrimeStage::widgetIdentityId("icons.png"));
  CHECK(adapter.keyForRow(-1) == PrimeStage::InvalidWidgetIdentityId);

  PrimeStage::TableSpec table;
  adapter.bindRows(table);
  CHECK(table.rows.size() == 2u);
  CHECK(table.rows[1][2] == "3.1 MB");
}

TEST_CASE("PrimeStage tree model adapter binds typed nodes and flattened keys") {
  std::vector<TreeModelRow> nodes = {
      {"root.assets",
       "Assets",
       true,
       false,
       {
           {"assets.textures", "Textures", true, false, {}},
           {"assets.audio", "Audio", false, true, {}},
       }},
      {"root.scripts", "Scripts", false, false, {}},
  };

  PrimeStage::TreeModelAdapter adapter = PrimeStage::makeTreeModel(
      nodes,
      [](TreeModelRow const& node) -> std::string_view { return node.label; },
      [](TreeModelRow const& node) -> std::vector<TreeModelRow> const& { return node.children; },
      [](TreeModelRow const& node) { return node.expanded; },
      [](TreeModelRow const& node) { return node.selected; },
      [](TreeModelRow const& node) -> std::string_view { return node.key; });

  CHECK(adapter.nodes().size() == 2u);
  CHECK(adapter.nodes()[0].label == "Assets");
  CHECK(adapter.nodes()[0].children.size() == 2u);
  CHECK(adapter.nodes()[0].children[1].selected);
  CHECK(adapter.keys().size() == 4u);
  CHECK(adapter.keyForRow(0) == PrimeStage::widgetIdentityId("root.assets"));
  CHECK(adapter.keyForRow(1) == PrimeStage::widgetIdentityId("assets.textures"));
  CHECK(adapter.keyForRow(2) == PrimeStage::widgetIdentityId("assets.audio"));
  CHECK(adapter.keyForRow(3) == PrimeStage::widgetIdentityId("root.scripts"));
  CHECK(adapter.keyForRow(12) == PrimeStage::InvalidWidgetIdentityId);

  PrimeStage::TreeViewSpec tree;
  adapter.bind(tree);
  CHECK(tree.nodes.size() == 2u);
  CHECK(tree.nodes[0].children[0].label == "Textures");
}

TEST_CASE("PrimeStage button interactions wire through spec callbacks") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  int clickCount = 0;
  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Apply";
  buttonSpec.size.preferredWidth = 120.0f;
  buttonSpec.size.preferredHeight = 28.0f;
  buttonSpec.backgroundStyle = 101u;
  buttonSpec.hoverStyle = 102u;
  buttonSpec.pressedStyle = 103u;
  buttonSpec.focusStyle = 104u;
  buttonSpec.callbacks.onActivate = [&]() { clickCount += 1; };

  PrimeStage::UiNode button = root.createButton(buttonSpec);
  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  CHECK(buttonNode->focusable);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float centerX = out->absX + out->absW * 0.5f;
  float centerY = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, centerX, centerY),
                  frame,
                  layout,
                  &focus);
  CHECK(focus.focusedNode() == button.nodeId());

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, centerX, centerY),
                  frame,
                  layout,
                  &focus);
  CHECK(clickCount == 1);
}

TEST_CASE("PrimeStage text field state-backed editing remains supported") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TextFieldState state;
  state.text = "Prime";
  state.cursor = static_cast<uint32_t>(state.text.size());
  state.focused = true;

  int stateChangedCount = 0;
  std::string lastText;

  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.state = &state;
  fieldSpec.size.preferredWidth = 220.0f;
  fieldSpec.size.preferredHeight = 28.0f;
  fieldSpec.callbacks.onStateChanged = [&]() { stateChangedCount += 1; };
  fieldSpec.callbacks.onChange = [&](std::string_view text) { lastText = std::string(text); };

  PrimeStage::UiNode field = root.createTextField(fieldSpec);
  PrimeFrame::Node const* node = frame.getNode(field.nodeId());
  REQUIRE(node != nullptr);
  REQUIRE(node->callbacks != PrimeFrame::InvalidCallbackId);

  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  PrimeFrame::Event textInput;
  textInput.type = PrimeFrame::EventType::TextInput;
  textInput.text = " Stage";
  CHECK(callback->onEvent(textInput));
  CHECK(state.text == "Prime Stage");
  CHECK(state.cursor == 11u);
  CHECK(lastText == "Prime Stage");

  PrimeFrame::Event newlineFilteredInput;
  newlineFilteredInput.type = PrimeFrame::EventType::TextInput;
  newlineFilteredInput.text = "\n!";
  CHECK(callback->onEvent(newlineFilteredInput));
  CHECK(state.text == "Prime Stage!");
  CHECK(state.cursor == 12u);
  CHECK(lastText == "Prime Stage!");
  CHECK(stateChangedCount >= 2);
}

TEST_CASE("PrimeStage text field without explicit state uses owned defaults") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.text = "Preview";
  fieldSpec.size.preferredWidth = 180.0f;
  fieldSpec.size.preferredHeight = 24.0f;
  std::string lastText;
  fieldSpec.callbacks.onChange = [&](std::string_view text) { lastText = std::string(text); };

  PrimeStage::UiNode field = root.createTextField(fieldSpec);
  PrimeFrame::Node const* node = frame.getNode(field.nodeId());
  REQUIRE(node != nullptr);
  CHECK(node->focusable);
  CHECK(node->callbacks != PrimeFrame::InvalidCallbackId);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::LayoutOut const* out = layout.get(field.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  float centerX = out->absX + out->absW * 0.5f;
  float centerY = out->absY + out->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, centerX, centerY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, centerX, centerY),
                  frame,
                  layout,
                  &focus);
  CHECK(focus.focusedNode() == field.nodeId());

  PrimeFrame::Event textInput;
  textInput.type = PrimeFrame::EventType::TextInput;
  textInput.text = "!";
  router.dispatch(textInput, frame, layout, &focus);
  CHECK(lastText == "Preview!");
}

TEST_CASE("PrimeStage text field owned state can persist across rebuilds") {
  auto owned = std::make_shared<PrimeStage::TextFieldState>();

  {
    PrimeFrame::Frame frame;
    PrimeStage::UiNode root = createRoot(frame);

    PrimeStage::TextFieldSpec fieldSpec;
    fieldSpec.ownedState = owned;
    fieldSpec.text = "Prime";
    fieldSpec.size.preferredWidth = 180.0f;
    fieldSpec.size.preferredHeight = 24.0f;
    PrimeStage::UiNode field = root.createTextField(fieldSpec);

    PrimeFrame::Node const* node = frame.getNode(field.nodeId());
    REQUIRE(node != nullptr);
    REQUIRE(node->callbacks != PrimeFrame::InvalidCallbackId);
    PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
    REQUIRE(callback != nullptr);
    REQUIRE(callback->onEvent);

    CHECK(owned->text == "Prime");
    owned->cursor = static_cast<uint32_t>(owned->text.size());
    owned->selectionAnchor = owned->cursor;
    owned->selectionStart = owned->cursor;
    owned->selectionEnd = owned->cursor;
    owned->focused = true;

    PrimeFrame::Event textInput;
    textInput.type = PrimeFrame::EventType::TextInput;
    textInput.text = " Stage";
    CHECK(callback->onEvent(textInput));
    CHECK(owned->text == "Prime Stage");
  }

  {
    PrimeFrame::Frame frame;
    PrimeStage::UiNode root = createRoot(frame);

    PrimeStage::TextFieldSpec fieldSpec;
    fieldSpec.ownedState = owned;
    fieldSpec.text = "Reset";
    fieldSpec.size.preferredWidth = 180.0f;
    fieldSpec.size.preferredHeight = 24.0f;
    (void)root.createTextField(fieldSpec);
    CHECK(owned->text == "Prime Stage");
  }
}

TEST_CASE("PrimeStage selectable text without explicit state installs owned defaults") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  uint32_t selectionStart = 0u;
  uint32_t selectionEnd = 0u;
  int selectionChanges = 0;

  PrimeStage::SelectableTextSpec spec;
  spec.text = "Selectable text sample";
  spec.size.preferredWidth = 220.0f;
  spec.size.preferredHeight = 42.0f;
  spec.callbacks.onSelectionChanged = [&](uint32_t start, uint32_t end) {
    selectionStart = start;
    selectionEnd = end;
    selectionChanges += 1;
  };

  PrimeStage::UiNode selectable = root.createSelectableText(spec);
  PrimeFrame::Node const* node = frame.getNode(selectable.nodeId());
  REQUIRE(node != nullptr);
  CHECK(node->callbacks != PrimeFrame::InvalidCallbackId);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::LayoutOut const* out = layout.get(selectable.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  float y = out->absY + out->absH * 0.5f;
  float beginX = out->absX + out->absW * 0.1f;
  float endX = out->absX + out->absW * 0.8f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, beginX, y),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDrag, endX, y),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, endX, y),
                  frame,
                  layout,
                  &focus);

  CHECK(selectionChanges > 0);
  CHECK(selectionEnd >= selectionStart);
}

TEST_CASE("PrimeStage UiNode fluent builder supports nested frame authoring") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Build";
  buttonSpec.size.preferredWidth = 96.0f;
  buttonSpec.size.preferredHeight = 28.0f;

  int withCallCount = 0;
  PrimeStage::UiNode button = root.createButton(buttonSpec).with([&](PrimeStage::UiNode& node) {
    withCallCount += 1;
    node.setVisible(false);
  });
  CHECK(withCallCount == 1);
  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  CHECK_FALSE(buttonNode->visible);

  PrimeStage::StackSpec columnSpec;
  columnSpec.size.preferredWidth = 260.0f;
  columnSpec.size.preferredHeight = 140.0f;

  PrimeStage::PanelSpec panelSpec;
  panelSpec.layout = PrimeFrame::LayoutType::VerticalStack;
  panelSpec.gap = 4.0f;

  PrimeStage::LabelSpec labelSpec;
  labelSpec.text = "Fluent";
  labelSpec.size.preferredWidth = 80.0f;
  labelSpec.size.preferredHeight = 20.0f;

  int stackCallCount = 0;
  int panelCallCount = 0;
  int labelCallCount = 0;
  PrimeFrame::NodeId panelId{};
  PrimeFrame::NodeId labelId{};
  PrimeStage::UiNode stack = root.createVerticalStack(columnSpec, [&](PrimeStage::UiNode& col) {
    stackCallCount += 1;
    col.createPanel(panelSpec, [&](PrimeStage::UiNode& panel) {
      panelCallCount += 1;
      panelId = panel.nodeId();
      panel.createLabel(labelSpec, [&](PrimeStage::UiNode& label) {
        labelCallCount += 1;
        labelId = label.nodeId();
      });
    });
  });
  CHECK(stackCallCount == 1);
  CHECK(panelCallCount == 1);
  CHECK(labelCallCount == 1);
  CHECK(frame.getNode(stack.nodeId()) != nullptr);
  CHECK(frame.getNode(panelId) != nullptr);
  CHECK(frame.getNode(labelId) != nullptr);

  PrimeStage::ScrollViewSpec scrollSpec;
  scrollSpec.size.preferredWidth = 220.0f;
  scrollSpec.size.preferredHeight = 80.0f;
  int scrollCallCount = 0;
  PrimeFrame::NodeId scrollRootId{};
  PrimeFrame::NodeId scrollContentId{};
  PrimeStage::ScrollView scrollView =
      root.createScrollView(scrollSpec, [&](PrimeStage::ScrollView& view) {
        scrollCallCount += 1;
        scrollRootId = view.root.nodeId();
        scrollContentId = view.content.nodeId();
        view.content.createLabel(labelSpec);
      });
  CHECK(scrollCallCount == 1);
  CHECK(scrollView.root.nodeId() == scrollRootId);
  CHECK(scrollView.content.nodeId() == scrollContentId);
  CHECK(frame.getNode(scrollView.root.nodeId()) != nullptr);
  CHECK(frame.getNode(scrollView.content.nodeId()) != nullptr);

  PrimeStage::WindowSpec windowSpec;
  windowSpec.title = "Fluent Window";
  windowSpec.width = 240.0f;
  windowSpec.height = 140.0f;
  int windowCallCount = 0;
  PrimeFrame::NodeId windowContentId{};
  PrimeStage::Window window = root.createWindow(windowSpec, [&](PrimeStage::Window& built) {
    windowCallCount += 1;
    windowContentId = built.content.nodeId();
    built.content.createSpacer(PrimeStage::SizeSpec{});
  });
  CHECK(windowCallCount == 1);
  CHECK(window.content.nodeId() == windowContentId);
  CHECK(frame.getNode(window.root.nodeId()) != nullptr);
  CHECK(frame.getNode(window.titleBar.nodeId()) != nullptr);
  CHECK(frame.getNode(window.content.nodeId()) != nullptr);
  CHECK(frame.getNode(window.resizeHandleId) != nullptr);
}

TEST_CASE("PrimeStage fluent builder API remains documented") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path uiHeaderPath = repoRoot / "include" / "PrimeStage" / "Ui.h";
  std::filesystem::path apiRefPath = repoRoot / "docs" / "minimal-api-reference.md";
  std::filesystem::path designPath = repoRoot / "docs" / "prime-stage-design.md";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  REQUIRE(std::filesystem::exists(uiHeaderPath));
  REQUIRE(std::filesystem::exists(apiRefPath));
  REQUIRE(std::filesystem::exists(designPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));

  std::ifstream uiHeaderInput(uiHeaderPath);
  REQUIRE(uiHeaderInput.good());
  std::string uiHeader((std::istreambuf_iterator<char>(uiHeaderInput)),
                       std::istreambuf_iterator<char>());
  REQUIRE(!uiHeader.empty());
  CHECK(uiHeader.find("UiNode with(Fn&& fn)") != std::string::npos);
  CHECK(uiHeader.find("UiNode column()") != std::string::npos);
  CHECK(uiHeader.find("UiNode row()") != std::string::npos);
  CHECK(uiHeader.find("UiNode overlay()") != std::string::npos);
  CHECK(uiHeader.find("UiNode panel()") != std::string::npos);
  CHECK(uiHeader.find("struct FormSpec") != std::string::npos);
  CHECK(uiHeader.find("struct FormFieldSpec") != std::string::npos);
  CHECK(uiHeader.find("UiNode label(std::string_view text)") != std::string::npos);
  CHECK(uiHeader.find("UiNode paragraph(std::string_view text, float maxWidth = 0.0f)") !=
        std::string::npos);
  CHECK(uiHeader.find("UiNode textLine(std::string_view text)") != std::string::npos);
  CHECK(uiHeader.find("UiNode button(std::string_view text, std::function<void()> onActivate = {})") !=
        std::string::npos);
  CHECK(uiHeader.find("UiNode form()") != std::string::npos);
  CHECK(uiHeader.find("UiNode form(FormSpec const& spec)") != std::string::npos);
  CHECK(uiHeader.find("UiNode formField(FormFieldSpec const& spec, Fn&& buildControl)") !=
        std::string::npos);
  CHECK(uiHeader.find("UiNode toggle(Binding<bool> binding)") != std::string::npos);
  CHECK(uiHeader.find("UiNode checkbox(std::string_view label, Binding<bool> binding)") !=
        std::string::npos);
  CHECK(uiHeader.find("UiNode slider(Binding<float> binding, bool vertical = false)") !=
        std::string::npos);
  CHECK(uiHeader.find("UiNode tabs(std::vector<std::string_view> labels, Binding<int> binding)") !=
        std::string::npos);
  CHECK(uiHeader.find("UiNode dropdown(std::vector<std::string_view> options, Binding<int> binding)") !=
        std::string::npos);
  CHECK(uiHeader.find("UiNode progressBar(Binding<float> binding)") != std::string::npos);
  CHECK(uiHeader.find("Window window(WindowSpec const& spec)") != std::string::npos);
  CHECK(uiHeader.find("UiNode createPanel(PanelSpec const& spec, Fn&& fn)") != std::string::npos);
  CHECK(uiHeader.find("UiNode createButton(ButtonSpec const& spec, Fn&& fn)") != std::string::npos);
  CHECK(uiHeader.find("UiNode createButton(std::string_view label,") != std::string::npos);
  CHECK(uiHeader.find("UiNode createTextField(TextFieldState& state,") != std::string::npos);
  CHECK(uiHeader.find("UiNode createToggle(bool on,") != std::string::npos);
  CHECK(uiHeader.find("UiNode createCheckbox(std::string_view label,") != std::string::npos);
  CHECK(uiHeader.find("UiNode createSlider(float value,") != std::string::npos);
  CHECK(uiHeader.find("class WidgetFocusHandle") != std::string::npos);
  CHECK(uiHeader.find("class WidgetVisibilityHandle") != std::string::npos);
  CHECK(uiHeader.find("class WidgetActionHandle") != std::string::npos);
  CHECK(uiHeader.find("class ListModelAdapter") != std::string::npos);
  CHECK(uiHeader.find("class TableModelAdapter") != std::string::npos);
  CHECK(uiHeader.find("class TreeModelAdapter") != std::string::npos);
  CHECK(uiHeader.find("WidgetFocusHandle focusHandle() const") != std::string::npos);
  CHECK(uiHeader.find("WidgetVisibilityHandle visibilityHandle() const") != std::string::npos);
  CHECK(uiHeader.find("WidgetActionHandle actionHandle() const") != std::string::npos);
  CHECK(uiHeader.find("PrimeFrame::NodeId lowLevelNodeId() const") != std::string::npos);
  CHECK(uiHeader.find("ListModelAdapter makeListModel(") != std::string::npos);
  CHECK(uiHeader.find("TableModelAdapter makeTableModel(") != std::string::npos);
  CHECK(uiHeader.find("TreeModelAdapter makeTreeModel(") != std::string::npos);
  CHECK(uiHeader.find("template <typename T>\nstruct State") != std::string::npos);
  CHECK(uiHeader.find("template <typename T>\nstruct Binding") != std::string::npos);
  CHECK(uiHeader.find("Binding<T> bind(State<T>& state)") != std::string::npos);
  CHECK(uiHeader.find("Binding<T> bind(State<T> const&)") != std::string::npos);
  CHECK(uiHeader.find("Binding<T> bind(State<T>&&)") != std::string::npos);
  CHECK(uiHeader.find("PrimeStage::bind(...) requires an lvalue PrimeStage::State<T> with stable lifetime.") !=
        std::string::npos);
  CHECK(uiHeader.find("PrimeStage::makeListModel label extractor must be callable as labelOf(item)") !=
        std::string::npos);
  CHECK(uiHeader.find("PrimeStage::makeTableModel cell extractor must be callable as cellOf(row, columnIndex)") !=
        std::string::npos);
  CHECK(uiHeader.find("PrimeStage::makeTreeModel children extractor must be callable as childrenOf(node)") !=
        std::string::npos);
  CHECK(uiHeader.find("See docs/minimal-api-reference.md") != std::string::npos);
  CHECK(uiHeader.find("std::function<void()> onActivate;") != std::string::npos);
  CHECK(uiHeader.find("std::function<void(bool)> onChange;") != std::string::npos);
  CHECK(uiHeader.find("std::function<void(int)> onSelect;") != std::string::npos);
  CHECK(uiHeader.find("std::function<void()> onOpen;") != std::string::npos);
  CHECK(uiHeader.find("UiNode createToggle(Binding<bool> binding);") != std::string::npos);
  CHECK(uiHeader.find("UiNode createCheckbox(std::string_view label, Binding<bool> binding);") !=
        std::string::npos);
  CHECK(uiHeader.find("UiNode createSlider(Binding<float> binding, bool vertical = false);") !=
        std::string::npos);
  CHECK(uiHeader.find("UiNode createTabs(std::vector<std::string_view> labels, Binding<int> binding);") !=
        std::string::npos);
  CHECK(uiHeader.find("UiNode createDropdown(std::vector<std::string_view> options, Binding<int> binding);") !=
        std::string::npos);
  CHECK(uiHeader.find("UiNode createProgressBar(Binding<float> binding);") != std::string::npos);
  CHECK(uiHeader.find("struct SliderState") != std::string::npos);
  CHECK(uiHeader.find("SliderState* state = nullptr;") != std::string::npos);
  CHECK(uiHeader.find("UiNode createList(ListSpec const& spec);") != std::string::npos);
  CHECK(uiHeader.find("UiNode createTable(std::vector<TableColumn> columns,") != std::string::npos);
  CHECK(uiHeader.find("UiNode createTreeView(std::vector<TreeNode> nodes, SizeSpec const& size);") !=
        std::string::npos);
  CHECK(uiHeader.find("ScrollView createScrollView(SizeSpec const& size,") != std::string::npos);
  CHECK(uiHeader.find("ScrollView createScrollView(ScrollViewSpec const& spec, Fn&& fn)") !=
        std::string::npos);
  CHECK(uiHeader.find("Window createWindow(WindowSpec const& spec, Fn&& fn)") !=
        std::string::npos);

  std::ifstream apiRefInput(apiRefPath);
  REQUIRE(apiRefInput.good());
  std::string apiRef((std::istreambuf_iterator<char>(apiRefInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!apiRef.empty());
  CHECK(apiRef.find("Fluent helpers") != std::string::npos);
  CHECK(apiRef.find("createX(spec, lambda)") != std::string::npos);
  CHECK(apiRef.find("Declarative helpers") != std::string::npos);
  CHECK(apiRef.find("column(...)") != std::string::npos);
  CHECK(apiRef.find("row(...)") != std::string::npos);
  CHECK(apiRef.find("window(spec, lambda)") != std::string::npos);
  CHECK(apiRef.find("Semantic Callback Surface") != std::string::npos);
  CHECK(apiRef.find("Typed Widget Handles") != std::string::npos);
  CHECK(apiRef.find("Collection Model Adapters") != std::string::npos);
  CHECK(apiRef.find("Compile-time Diagnostics") != std::string::npos);
  CHECK(apiRef.find("`PrimeStage::bind(...)` requires an lvalue") != std::string::npos);
  CHECK(apiRef.find("`PrimeStage::makeListModel(...)` validates") != std::string::npos);
  CHECK(apiRef.find("`PrimeStage::makeTableModel(...)` validates") != std::string::npos);
  CHECK(apiRef.find("`PrimeStage::makeTreeModel(...)` validates") != std::string::npos);
  CHECK(apiRef.find("makeListModel(...)") != std::string::npos);
  CHECK(apiRef.find("makeTableModel(...)") != std::string::npos);
  CHECK(apiRef.find("makeTreeModel(...)") != std::string::npos);
  CHECK(apiRef.find("focusWidget(...)") != std::string::npos);
  CHECK(apiRef.find("setWidgetVisible(...)") != std::string::npos);
  CHECK(apiRef.find("dispatchWidgetEvent(...)") != std::string::npos);
  CHECK(apiRef.find("onActivate") != std::string::npos);
  CHECK(apiRef.find("onChange") != std::string::npos);
  CHECK(apiRef.find("onOpen") != std::string::npos);
  CHECK(apiRef.find("onSelect") != std::string::npos);
  CHECK(apiRef.find("FormSpec") != std::string::npos);
  CHECK(apiRef.find("FormFieldSpec") != std::string::npos);
  CHECK(apiRef.find("form(...)") != std::string::npos);
  CHECK(apiRef.find("formField(...)") != std::string::npos);
  CHECK(apiRef.find("toggle(binding)") != std::string::npos);
  CHECK(apiRef.find("checkbox(label, binding)") != std::string::npos);
  CHECK(apiRef.find("tabs(labels, binding)") != std::string::npos);
  CHECK(apiRef.find("dropdown(options, binding)") != std::string::npos);
  CHECK(apiRef.find("progressBar(binding)") != std::string::npos);

  std::ifstream designInput(designPath);
  REQUIRE(designInput.good());
  std::string design((std::istreambuf_iterator<char>(designInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!design.empty());
  CHECK(design.find("createX(spec, fn)") != std::string::npos);
  CHECK(design.find("createWindow(spec, fn)") != std::string::npos);
  CHECK(design.find("column(...)") != std::string::npos);
  CHECK(design.find("row(...)") != std::string::npos);
  CHECK(design.find("form(...)") != std::string::npos);
  CHECK(design.find("formField(...)") != std::string::npos);
  CHECK(design.find("window(spec, fn)") != std::string::npos);
  CHECK(design.find("onActivate") != std::string::npos);
  CHECK(design.find("onChange") != std::string::npos);
  CHECK(design.find("onSelect") != std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("Fluent Builder Authoring") != std::string::npos);
  CHECK(guidelines.find("Current Gaps") != std::string::npos);
  CHECK(guidelines.find("- none.") != std::string::npos);
  CHECK(guidelines.find("UiNode::with(...)") != std::string::npos);
  CHECK(guidelines.find("Declarative Composition Helpers") != std::string::npos);
  CHECK(guidelines.find("form(...)") != std::string::npos);
  CHECK(guidelines.find("formField(...)") != std::string::npos);
  CHECK(guidelines.find("toggle(bind(flag))") != std::string::npos);
  CHECK(guidelines.find("tabs({\"A\", \"B\"}, bind(index))") != std::string::npos);
  CHECK(guidelines.find("onActivate") != std::string::npos);
  CHECK(guidelines.find("onChange") != std::string::npos);
  CHECK(guidelines.find("onOpen") != std::string::npos);
  CHECK(guidelines.find("onSelect") != std::string::npos);
}

TEST_CASE("PrimeStage examples stay split between canonical and advanced tiers") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path modernExamplePath = repoRoot / "examples" / "canonical" / "primestage_modern_api.cpp";
  std::filesystem::path widgetsExamplePath = repoRoot / "examples" / "advanced" / "primestage_widgets.cpp";
  std::filesystem::path basicExamplePath = repoRoot / "examples" / "canonical" / "primestage_example.cpp";
  std::filesystem::path sceneExamplePath = repoRoot / "examples" / "advanced" / "primestage_scene.cpp";
  std::filesystem::path cmakePath = repoRoot / "CMakeLists.txt";
  std::filesystem::path checklistPath =
      repoRoot / "docs" / "example-app-consumer-checklist.md";
  REQUIRE(std::filesystem::exists(modernExamplePath));
  REQUIRE(std::filesystem::exists(widgetsExamplePath));
  REQUIRE(std::filesystem::exists(basicExamplePath));
  REQUIRE(std::filesystem::exists(sceneExamplePath));
  REQUIRE(std::filesystem::exists(cmakePath));
  REQUIRE(std::filesystem::exists(checklistPath));

  std::ifstream widgetsInput(widgetsExamplePath);
  REQUIRE(widgetsInput.good());
  std::string widgetsSource((std::istreambuf_iterator<char>(widgetsInput)),
                            std::istreambuf_iterator<char>());
  REQUIRE(!widgetsSource.empty());

  std::ifstream modernInput(modernExamplePath);
  REQUIRE(modernInput.good());
  std::string modernSource((std::istreambuf_iterator<char>(modernInput)),
                           std::istreambuf_iterator<char>());
  REQUIRE(!modernSource.empty());

  auto countOccurrences = [&](std::string_view needle) {
    size_t count = 0u;
    size_t pos = widgetsSource.find(needle);
    while (pos != std::string::npos) {
      ++count;
      pos = widgetsSource.find(needle, pos + needle.size());
    }
    return count;
  };

  auto countModernOccurrences = [&](std::string_view needle) {
    size_t count = 0u;
    size_t pos = modernSource.find(needle);
    while (pos != std::string::npos) {
      ++count;
      pos = modernSource.find(needle, pos + needle.size());
    }
    return count;
  };

  CHECK(modernSource.find("#include \"PrimeFrame/") == std::string::npos);
  CHECK(modernSource.find("#include \"PrimeHost/") == std::string::npos);
  CHECK(modernSource.find("PrimeFrame::") == std::string::npos);
  CHECK(modernSource.find("PrimeHost::") == std::string::npos);
  CHECK(modernSource.find(".nodeId(") == std::string::npos);
  CHECK(modernSource.find(".lowLevelNodeId(") == std::string::npos);
  CHECK(modernSource.find(".frame()") == std::string::npos);
  CHECK(modernSource.find(".layout()") == std::string::npos);
  CHECK(modernSource.find(".focus()") == std::string::npos);
  CHECK(modernSource.find(".router()") == std::string::npos);
  CHECK(modernSource.find("PrimeStage::LowLevel::") == std::string::npos);
  CHECK(modernSource.find("requestRebuild") == std::string::npos);
  CHECK(modernSource.find("requestLayout") == std::string::npos);
  CHECK(modernSource.find("requestFrame") == std::string::npos);
  CHECK(modernSource.find("makeListModel(") != std::string::npos);
  CHECK(modernSource.find("makeTableModel(") != std::string::npos);
  CHECK(modernSource.find("makeTreeModel(") != std::string::npos);
  CHECK(modernSource.find("runRebuildIfNeeded") != std::string::npos);
  CHECK(modernSource.find("renderToPng") != std::string::npos);
  CHECK(countModernOccurrences("Spec ") <= 8u);
  CHECK(countModernOccurrences("create") <= 12u);
  size_t modernLines =
      static_cast<size_t>(std::count(modernSource.begin(), modernSource.end(), '\n')) + 1u;
  CHECK(modernLines <= 220u);

  // Advanced tier carries host/runtime integration concerns.
  CHECK(widgetsSource.find("#include \"PrimeHost/PrimeHost.h\"") != std::string::npos);
  CHECK(widgetsSource.find("PrimeHost::EventBuffer") != std::string::npos);

  CHECK(widgetsSource.find("tabs.callbacks.onSelect") == std::string::npos);
  CHECK(widgetsSource.find("tabs.callbacks.onTabChanged") == std::string::npos);
  CHECK(widgetsSource.find("dropdown.callbacks.onSelect") == std::string::npos);
  CHECK(widgetsSource.find("dropdown.callbacks.onSelected") == std::string::npos);
  CHECK(widgetsSource.find("toggle.callbacks.onChange") == std::string::npos);
  CHECK(widgetsSource.find("checkbox.callbacks.onChange") == std::string::npos);
  CHECK(widgetsSource.find("app.ui.bridgeHostInputEvent") != std::string::npos);
  CHECK(widgetsSource.find("HostKey::Escape") != std::string::npos);
  CHECK(widgetsSource.find("scrollDirectionSign") != std::string::npos);
  CHECK(widgetsSource.find("PrimeStage::App ui;") != std::string::npos);
  CHECK(widgetsSource.find("root.column(") != std::string::npos);
  CHECK(widgetsSource.find("columns.column(") != std::string::npos);
  CHECK(widgetsSource.find("actions.row(") != std::string::npos);
  CHECK(widgetsSource.find("settings.form(formSpec,") != std::string::npos);
  CHECK(widgetsSource.find("form.formField(nameField,") != std::string::npos);
  CHECK(widgetsSource.find("form.formField(\"Release channel\",") != std::string::npos);
  CHECK(widgetsSource.find("form.formField(\"Selectable notes\",") != std::string::npos);
  CHECK(widgetsSource.find("root.window(") != std::string::npos);
  CHECK(widgetsSource.find("size.maxWidth") != std::string::npos);
  CHECK(countOccurrences("size.preferredWidth") <= 3u);
  CHECK(countOccurrences("size.preferredHeight") <= 3u);
  CHECK(widgetsSource.find("PrimeStage::LabelSpec") == std::string::npos);
  CHECK(widgetsSource.find("PrimeStage::TextLineSpec") == std::string::npos);
  CHECK(widgetsSource.find("app.ui.runRebuildIfNeeded") != std::string::npos);
  CHECK(widgetsSource.find("app.ui.renderToTarget") != std::string::npos);
  CHECK(widgetsSource.find("app.ui.renderToPng") != std::string::npos);
  CHECK(widgetsSource.find("app.ui.lifecycle().requestRebuild()") != std::string::npos);
  CHECK(widgetsSource.find("Advanced lifecycle orchestration (documented exception):") !=
        std::string::npos);
  CHECK(widgetsSource.find("app.ui.lifecycle().framePending()") != std::string::npos);
  CHECK(widgetsSource.find("if (bridgeResult.requestFrame)") == std::string::npos);
  CHECK(widgetsSource.find("#include \"PrimeFrame/") == std::string::npos);
  CHECK(widgetsSource.find("PrimeFrame::Frame frame") == std::string::npos);
  CHECK(widgetsSource.find("PrimeFrame::LayoutEngine") == std::string::npos);
  CHECK(widgetsSource.find("PrimeFrame::EventRouter") == std::string::npos);
  CHECK(widgetsSource.find("PrimeFrame::FocusManager") == std::string::npos);
  CHECK(widgetsSource.find(".nodeId(") == std::string::npos);
  CHECK(widgetsSource.find("PrimeStage::LowLevel::") == std::string::npos);
  CHECK(widgetsSource.find("makeListModel(") != std::string::npos);
  CHECK(widgetsSource.find("makeTableModel(") != std::string::npos);
  CHECK(widgetsSource.find("makeTreeModel(") != std::string::npos);
  CHECK(widgetsSource.find("listViews") == std::string::npos);
  CHECK(widgetsSource.find("PrimeStage::State<bool> toggle{};") != std::string::npos);
  CHECK(widgetsSource.find("PrimeStage::State<int> tabs{};") != std::string::npos);
  CHECK(widgetsSource.find("PrimeStage::State<float> sliderValue{};") != std::string::npos);
  CHECK(widgetsSource.find("row.toggle(PrimeStage::bind(app.state.toggle));") !=
        std::string::npos);
  CHECK(widgetsSource.find("row.checkbox(\"Checkbox\", PrimeStage::bind(app.state.checkbox));") !=
        std::string::npos);
  CHECK(widgetsSource.find("range.slider(PrimeStage::bind(app.state.sliderValue));") !=
        std::string::npos);
  CHECK(widgetsSource.find("range.progressBar(PrimeStage::bind(app.state.progressValue));") !=
        std::string::npos);
  CHECK(widgetsSource.find("choice.tabs({\"Overview\", \"Assets\", \"Settings\"},") !=
        std::string::npos);
  CHECK(widgetsSource.find("choice.dropdown({\"Preview\", \"Edit\", \"Export\", \"Publish\"},") !=
        std::string::npos);
  CHECK(widgetsSource.find("registerAction(ActionNextTab") != std::string::npos);
  CHECK(widgetsSource.find("registerAction(ActionToggleCheckbox") != std::string::npos);
  CHECK(widgetsSource.find("bindShortcut(nextTabShortcut, ActionNextTab);") != std::string::npos);
  CHECK(widgetsSource.find("bindShortcut(toggleShortcut, ActionToggleCheckbox);") !=
        std::string::npos);
  CHECK(widgetsSource.find("makeActionCallback(std::string(ActionNextTab))") != std::string::npos);
  CHECK(widgetsSource.find("makeActionCallback(std::string(ActionToggleCheckbox))") !=
        std::string::npos);
  CHECK(widgetsSource.find("PrimeStage::ToggleSpec toggle;") == std::string::npos);
  CHECK(widgetsSource.find("PrimeStage::CheckboxSpec checkbox;") == std::string::npos);
  CHECK(widgetsSource.find("PrimeStage::SliderSpec slider;") == std::string::npos);
  CHECK(widgetsSource.find("PrimeStage::ProgressBarSpec progress;") == std::string::npos);
  CHECK(widgetsSource.find("PrimeStage::TabsSpec tabs;") == std::string::npos);
  CHECK(widgetsSource.find("PrimeStage::DropdownSpec dropdown;") == std::string::npos);
  CHECK(widgetsSource.find("textInput.createTextField(") == std::string::npos);
  CHECK(widgetsSource.find("textInput.createSelectableText(") == std::string::npos);
  CHECK(widgetsSource.find("tabViews.reserve(") == std::string::npos);
  CHECK(widgetsSource.find("dropdownViews.reserve(") == std::string::npos);
  CHECK(widgetsSource.find("app.ui.connectHostServices(*app.host, app.surfaceId);") !=
        std::string::npos);
  CHECK(widgetsSource.find("app.ui.applyPlatformServices(field);") != std::string::npos);
  CHECK(widgetsSource.find("app.ui.applyPlatformServices(selectable);") != std::string::npos);
  CHECK(widgetsSource.find("PrimeStage::TextFieldClipboard") == std::string::npos);
  CHECK(widgetsSource.find("PrimeStage::SelectableTextClipboard") == std::string::npos);
  CHECK(widgetsSource.find("callbacks.onCursorHintChanged") == std::string::npos);
  CHECK(widgetsSource.find("slider.state = &app.state.slider;") == std::string::npos);
  CHECK(widgetsSource.find("progress.state = &app.state.progress;") == std::string::npos);
  CHECK(widgetsSource.find("slider.callbacks.onValueChanged") == std::string::npos);
  CHECK(widgetsSource.find("progress.callbacks.onValueChanged") == std::string::npos);

  // Examples must not bootstrap theme/palette defaults in app code.
  CHECK(widgetsSource.find("getTheme(PrimeFrame::DefaultThemeId)") == std::string::npos);
  CHECK(widgetsSource.find("theme->palette") == std::string::npos);
  CHECK(widgetsSource.find("theme->rectStyles") == std::string::npos);
  CHECK(widgetsSource.find("theme->textStyles") == std::string::npos);

  // App-level widget usage should not rely on raw PrimeFrame callback mutation.
  CHECK(widgetsSource.find("appendNodeEventCallback") == std::string::npos);
  CHECK(widgetsSource.find("node->callbacks =") == std::string::npos);
  CHECK(widgetsSource.find("PrimeFrame::Callback callback") == std::string::npos);
  CHECK(widgetsSource.find("RestoreFocusTarget") == std::string::npos);
  CHECK(widgetsSource.find("std::get_if<PrimeHost::PointerEvent>") == std::string::npos);
  CHECK(widgetsSource.find("std::get_if<PrimeHost::KeyEvent>") == std::string::npos);
  CHECK(widgetsSource.find("KeyEscape") == std::string::npos);
  CHECK(widgetsSource.find("0x28") == std::string::npos);
  CHECK(widgetsSource.find("0x50") == std::string::npos);
  CHECK(widgetsSource.find("needsRebuild") == std::string::npos);
  CHECK(widgetsSource.find("needsLayout") == std::string::npos);
  CHECK(widgetsSource.find("needsFrame") == std::string::npos);

  std::ifstream basicExampleInput(basicExamplePath);
  REQUIRE(basicExampleInput.good());
  std::string basicExample((std::istreambuf_iterator<char>(basicExampleInput)),
                           std::istreambuf_iterator<char>());
  REQUIRE(!basicExample.empty());
  CHECK(basicExample.find("#include \"PrimeFrame/") == std::string::npos);
  CHECK(basicExample.find("#include \"PrimeHost/") == std::string::npos);
  CHECK(basicExample.find("PrimeFrame::") == std::string::npos);
  CHECK(basicExample.find("PrimeHost::") == std::string::npos);
  CHECK(basicExample.find("PrimeStage::LowLevel::") == std::string::npos);
  CHECK(basicExample.find(".nodeId(") == std::string::npos);
  CHECK(basicExample.find("PrimeStage::getVersionString") != std::string::npos);

  std::ifstream sceneExampleInput(sceneExamplePath);
  REQUIRE(sceneExampleInput.good());
  std::string sceneExample((std::istreambuf_iterator<char>(sceneExampleInput)),
                           std::istreambuf_iterator<char>());
  REQUIRE(!sceneExample.empty());
  CHECK(sceneExample.find("createWindow(") != std::string::npos);
  CHECK(sceneExample.find("createButton(") != std::string::npos);
  CHECK(sceneExample.find("createTextField(") != std::string::npos);
  CHECK(sceneExample.find("createToggle(") != std::string::npos);
  CHECK(sceneExample.find("createCheckbox(") != std::string::npos);
  CHECK(sceneExample.find("createSlider(") != std::string::npos);
  CHECK(sceneExample.find("createList(") != std::string::npos);
  CHECK(sceneExample.find("createTreeView(") != std::string::npos);
  CHECK(sceneExample.find("createScrollView(") != std::string::npos);
  CHECK(sceneExample.find("#include \"PrimeFrame/Frame.h\"") != std::string::npos);
  CHECK(sceneExample.find("PrimeFrame::LayoutEngine") != std::string::npos);

  std::ifstream cmakeInput(cmakePath);
  REQUIRE(cmakeInput.good());
  std::string cmakeSource((std::istreambuf_iterator<char>(cmakeInput)),
                          std::istreambuf_iterator<char>());
  REQUIRE(!cmakeSource.empty());
  CHECK(cmakeSource.find("add_executable(primestage_modern_api") != std::string::npos);
  CHECK(cmakeSource.find("examples/canonical/primestage_modern_api.cpp") != std::string::npos);
  CHECK(cmakeSource.find("examples/canonical/primestage_example.cpp") != std::string::npos);
  CHECK(cmakeSource.find("examples/advanced/primestage_widgets.cpp") != std::string::npos);
  CHECK(cmakeSource.find("add_executable(primestage_scene") != std::string::npos);
  CHECK(cmakeSource.find("examples/advanced/primestage_scene.cpp") != std::string::npos);

  std::ifstream checklistInput(checklistPath);
  REQUIRE(checklistInput.good());
  std::string checklist((std::istreambuf_iterator<char>(checklistInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!checklist.empty());
  CHECK(checklist.find("Canonical Rules") != std::string::npos);
  CHECK(checklist.find("bridgeHostInputEvent") != std::string::npos);
  CHECK(checklist.find("PrimeStage::App") != std::string::npos);
  CHECK(checklist.find("WidgetIdentityReconciler") != std::string::npos);
  CHECK(checklist.find("node->callbacks = ...") != std::string::npos);
  CHECK(checklist.find("registerAction") != std::string::npos);
  CHECK(checklist.find("bindShortcut") != std::string::npos);
  CHECK(checklist.find("form(...)") != std::string::npos);
  CHECK(checklist.find("formField(...)") != std::string::npos);
  CHECK(checklist.find("examples/canonical/primestage_modern_api.cpp") != std::string::npos);
  CHECK(checklist.find("examples/advanced/*.cpp") != std::string::npos);
  CHECK(checklist.find("Keep canonical examples out of `PrimeStage::LowLevel`") !=
        std::string::npos);
  CHECK(checklist.find("Advanced lifecycle orchestration (documented exception):") !=
        std::string::npos);
  CHECK(checklist.find("theme token/palette construction") == std::string::npos);
  CHECK(checklist.find("tests/unit/test_api_ergonomics.cpp") != std::string::npos);

  std::filesystem::path readmePath = repoRoot / "README.md";
  REQUIRE(std::filesystem::exists(readmePath));
  std::ifstream readmeInput(readmePath);
  REQUIRE(readmeInput.good());
  std::string readme((std::istreambuf_iterator<char>(readmeInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!readme.empty());
  CHECK(readme.find("Canonical tier (start here)") != std::string::npos);
  CHECK(readme.find("Advanced tier (host/frame integration samples)") != std::string::npos);
  CHECK(readme.find("primestage_modern_api") != std::string::npos);
  size_t canonicalTierPos = readme.find("Canonical tier (start here)");
  size_t advancedTierPos = readme.find("Advanced tier (host/frame integration samples)");
  size_t modernPos = readme.find("primestage_modern_api");
  size_t widgetsPos = readme.find("primestage_widgets");
  REQUIRE(canonicalTierPos != std::string::npos);
  REQUIRE(advancedTierPos != std::string::npos);
  REQUIRE(modernPos != std::string::npos);
  REQUIRE(widgetsPos != std::string::npos);
  CHECK(canonicalTierPos < advancedTierPos);
  CHECK(modernPos < widgetsPos);
}

TEST_CASE("PrimeStage API ergonomics scorecard thresholds stay within budget") {
  constexpr size_t ModernUiBodyLineBudget = 70u;
  constexpr size_t WidgetsUiBodyLineBudget = 220u;
  constexpr float MaxAverageLinesPerWidget = 6.0f;
  constexpr size_t ModernMinWidgetCalls = 10u;
  constexpr size_t WidgetsMinWidgetCalls = 35u;

  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path scorecardPath = repoRoot / "docs" / "api-ergonomics-scorecard.md";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  std::filesystem::path checklistPath =
      repoRoot / "docs" / "example-app-consumer-checklist.md";
  std::filesystem::path modernExamplePath = repoRoot / "examples" / "canonical" / "primestage_modern_api.cpp";
  std::filesystem::path widgetsExamplePath = repoRoot / "examples" / "advanced" / "primestage_widgets.cpp";
  REQUIRE(std::filesystem::exists(scorecardPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));
  REQUIRE(std::filesystem::exists(checklistPath));
  REQUIRE(std::filesystem::exists(modernExamplePath));
  REQUIRE(std::filesystem::exists(widgetsExamplePath));

  std::ifstream scorecardInput(scorecardPath);
  REQUIRE(scorecardInput.good());
  std::string scorecard((std::istreambuf_iterator<char>(scorecardInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!scorecard.empty());
  CHECK(scorecard.find("Canonical UI LOC (modern)") != std::string::npos);
  CHECK(scorecard.find("Advanced UI LOC (widgets)") != std::string::npos);
  CHECK(scorecard.find("Average lines per widget instantiation (modern)") != std::string::npos);
  CHECK(scorecard.find("Average lines per widget instantiation (advanced widgets)") != std::string::npos);
  CHECK(scorecard.find("Required spec fields per standard widget") != std::string::npos);
  CHECK(scorecard.find("`<= 70`") != std::string::npos);
  CHECK(scorecard.find("`<= 220`") != std::string::npos);
  CHECK(scorecard.find("`<= 6.0`") != std::string::npos);
  CHECK(scorecard.find("`>= 10`") != std::string::npos);
  CHECK(scorecard.find("`>= 35`") != std::string::npos);
  CHECK(scorecard.find("tests/unit/test_api_ergonomics.cpp") != std::string::npos);
  CHECK(scorecard.find("tests/unit/test_builder_api.cpp") != std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("docs/api-ergonomics-scorecard.md") != std::string::npos);

  std::ifstream checklistInput(checklistPath);
  REQUIRE(checklistInput.good());
  std::string checklist((std::istreambuf_iterator<char>(checklistInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!checklist.empty());
  CHECK(checklist.find("docs/api-ergonomics-scorecard.md") != std::string::npos);

  std::ifstream modernInput(modernExamplePath);
  REQUIRE(modernInput.good());
  std::string modernSource((std::istreambuf_iterator<char>(modernInput)),
                           std::istreambuf_iterator<char>());
  REQUIRE(!modernSource.empty());

  std::ifstream widgetsInput(widgetsExamplePath);
  REQUIRE(widgetsInput.good());
  std::string widgetsSource((std::istreambuf_iterator<char>(widgetsInput)),
                            std::istreambuf_iterator<char>());
  REQUIRE(!widgetsSource.empty());

  ExampleErgonomicsMetrics modernMetrics =
      measureExampleErgonomics(modernSource,
                               "void buildUi(PrimeStage::UiNode root, DemoState& state)");
  ExampleErgonomicsMetrics widgetsMetrics =
      measureExampleErgonomics(widgetsSource,
                               "void rebuildUi(PrimeStage::UiNode root, DemoApp& app)");

  CHECK(modernMetrics.nonEmptyCodeLines <= ModernUiBodyLineBudget);
  CHECK(widgetsMetrics.nonEmptyCodeLines <= WidgetsUiBodyLineBudget);
  CHECK(modernMetrics.widgetInstantiationCalls >= ModernMinWidgetCalls);
  CHECK(widgetsMetrics.widgetInstantiationCalls >= WidgetsMinWidgetCalls);
  CHECK(modernMetrics.averageLinesPerWidget <= MaxAverageLinesPerWidget);
  CHECK(widgetsMetrics.averageLinesPerWidget <= MaxAverageLinesPerWidget);
}

TEST_CASE("PrimeStage input bridge exposes normalized key and scroll semantics") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path inputBridgePath = repoRoot / "include" / "PrimeStage" / "InputBridge.h";
  std::filesystem::path appPath = repoRoot / "include" / "PrimeStage" / "App.h";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  std::filesystem::path apiRefPath = repoRoot / "docs" / "minimal-api-reference.md";
  REQUIRE(std::filesystem::exists(inputBridgePath));
  REQUIRE(std::filesystem::exists(appPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));
  REQUIRE(std::filesystem::exists(apiRefPath));

  std::ifstream inputBridgeInput(inputBridgePath);
  REQUIRE(inputBridgeInput.good());
  std::string inputBridge((std::istreambuf_iterator<char>(inputBridgeInput)),
                          std::istreambuf_iterator<char>());
  REQUIRE(!inputBridge.empty());
  CHECK(inputBridge.find("using HostKey = KeyCode;") != std::string::npos);
  CHECK(inputBridge.find("scrollLinePixels") != std::string::npos);
  CHECK(inputBridge.find("scrollDirectionSign") != std::string::npos);
  CHECK(inputBridge.find("scroll->deltaY * deltaScale * directionSign") != std::string::npos);

  std::ifstream appInput(appPath);
  REQUIRE(appInput.good());
  std::string appHeader((std::istreambuf_iterator<char>(appInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!appHeader.empty());
  CHECK(appHeader.find("enum class AppActionSource") != std::string::npos);
  CHECK(appHeader.find("struct AppShortcut") != std::string::npos);
  CHECK(appHeader.find("struct AppActionInvocation") != std::string::npos);
  CHECK(appHeader.find("std::string actionId;") != std::string::npos);
  CHECK(appHeader.find("using AppActionCallback = std::function<void(AppActionInvocation const&)>;") !=
        std::string::npos);
  CHECK(appHeader.find("bool registerAction(std::string_view actionId, AppActionCallback callback);") !=
        std::string::npos);
  CHECK(appHeader.find("bool bindShortcut(AppShortcut const& shortcut, std::string_view actionId);") !=
        std::string::npos);
  CHECK(appHeader.find("bool invokeAction(std::string_view actionId,") != std::string::npos);
  CHECK(appHeader.find("std::function<void()> makeActionCallback(std::string actionId);") !=
        std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("KeyCode") != std::string::npos);
  CHECK(guidelines.find("scrollLinePixels") != std::string::npos);
  CHECK(guidelines.find("scrollDirectionSign") != std::string::npos);
  CHECK(guidelines.find("registerAction(...)") != std::string::npos);
  CHECK(guidelines.find("bindShortcut(...)") != std::string::npos);
  CHECK(guidelines.find("AppActionInvocation::actionId") != std::string::npos);

  std::ifstream apiRefInput(apiRefPath);
  REQUIRE(apiRefInput.good());
  std::string apiRef((std::istreambuf_iterator<char>(apiRefInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!apiRef.empty());
  CHECK(apiRef.find("registerAction(...)") != std::string::npos);
  CHECK(apiRef.find("bindShortcut(...)") != std::string::npos);
  CHECK(apiRef.find("makeActionCallback(...)") != std::string::npos);
  CHECK(apiRef.find("AppActionInvocation::actionId") != std::string::npos);
}

TEST_CASE("PrimeStage design docs record resolved architecture decisions") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path decisionPath = repoRoot / "docs" / "design-decisions.md";
  std::filesystem::path designPath = repoRoot / "docs" / "prime-stage-design.md";
  REQUIRE(std::filesystem::exists(decisionPath));
  REQUIRE(std::filesystem::exists(designPath));

  std::ifstream decisionInput(decisionPath);
  REQUIRE(decisionInput.good());
  std::string decisions((std::istreambuf_iterator<char>(decisionInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!decisions.empty());
  CHECK(decisions.find("Decision 1: Table Remains a First-Class Widget") != std::string::npos);
  CHECK(decisions.find("Decision 2: Window Chrome Is Composed Explicitly") != std::string::npos);
  CHECK(decisions.find("Decision 3: Patch Operations Use a Strict Safety Whitelist") !=
        std::string::npos);
  CHECK(decisions.find("Whitelisted patch fields") != std::string::npos);

  std::ifstream designInput(designPath);
  REQUIRE(designInput.good());
  std::string design((std::istreambuf_iterator<char>(designInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!design.empty());
  CHECK(design.find("docs/design-decisions.md") != std::string::npos);
  CHECK(design.find("## Open Questions") == std::string::npos);
}

TEST_CASE("PrimeStage window builder API is stateless and callback-driven") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path uiHeaderPath = repoRoot / "include" / "PrimeStage" / "Ui.h";
  std::filesystem::path sourceCppPath = repoRoot / "src" / "PrimeStage.cpp";
  std::filesystem::path designPath = repoRoot / "docs" / "prime-stage-design.md";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  std::filesystem::path agentsPath = repoRoot / "AGENTS.md";
  REQUIRE(std::filesystem::exists(uiHeaderPath));
  REQUIRE(std::filesystem::exists(sourceCppPath));
  REQUIRE(std::filesystem::exists(designPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));
  REQUIRE(std::filesystem::exists(agentsPath));

  std::ifstream uiHeaderInput(uiHeaderPath);
  REQUIRE(uiHeaderInput.good());
  std::string uiHeader((std::istreambuf_iterator<char>(uiHeaderInput)),
                       std::istreambuf_iterator<char>());
  REQUIRE(!uiHeader.empty());
  CHECK(uiHeader.find("struct WindowCallbacks") != std::string::npos);
  CHECK(uiHeader.find("struct WindowSpec") != std::string::npos);
  CHECK(uiHeader.find("Window createWindow(WindowSpec const& spec);") != std::string::npos);
  CHECK(uiHeader.find("std::function<void(float, float)> onMoved;") != std::string::npos);
  CHECK(uiHeader.find("std::function<void(float, float)> onResized;") != std::string::npos);

  std::ifstream sourceCppInput(sourceCppPath);
  REQUIRE(sourceCppInput.good());
  std::string sourceCpp((std::istreambuf_iterator<char>(sourceCppInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!sourceCpp.empty());
  CHECK(sourceCpp.find("Window UiNode::createWindow(WindowSpec const& specInput)") !=
        std::string::npos);
  CHECK(sourceCpp.find("callbacks.onMoved") != std::string::npos);
  CHECK(sourceCpp.find("callbacks.onResized") != std::string::npos);
  CHECK(sourceCpp.find("callbacks.onFocusRequested") != std::string::npos);

  std::ifstream designInput(designPath);
  REQUIRE(designInput.good());
  std::string design((std::istreambuf_iterator<char>(designInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!design.empty());
  CHECK(design.find("createWindow(WindowSpec const& spec)") != std::string::npos);
  CHECK(design.find("onMoved(deltaX, deltaY)") != std::string::npos);
  CHECK(design.find("onResized(deltaWidth, deltaHeight)") != std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("createWindow(WindowSpec)") != std::string::npos);
  CHECK(guidelines.find("stateless") != std::string::npos);

  std::ifstream agentsInput(agentsPath);
  REQUIRE(agentsInput.good());
  std::string agents((std::istreambuf_iterator<char>(agentsInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!agents.empty());
  CHECK(agents.find("createWindow(WindowSpec)") != std::string::npos);
}

TEST_CASE("PrimeStage widget interactions support patch-first frame updates") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path sourceCppPath = repoRoot / "src" / "PrimeStage.cpp";
  std::filesystem::path designPath = repoRoot / "docs" / "prime-stage-design.md";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  std::filesystem::path apiRefPath = repoRoot / "docs" / "minimal-api-reference.md";
  std::filesystem::path examplePath = repoRoot / "examples" / "advanced" / "primestage_widgets.cpp";
  REQUIRE(std::filesystem::exists(sourceCppPath));
  REQUIRE(std::filesystem::exists(designPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));
  REQUIRE(std::filesystem::exists(apiRefPath));
  REQUIRE(std::filesystem::exists(examplePath));

  std::ifstream sourceInput(sourceCppPath);
  REQUIRE(sourceInput.good());
  std::string source((std::istreambuf_iterator<char>(sourceInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!source.empty());
  CHECK(source.find("patchTextFieldVisuals") != std::string::npos);
  CHECK(source.find("TextFieldPatchState") != std::string::npos);
  CHECK(source.find("notify_state = [&]()") != std::string::npos);
  CHECK(source.find("applyToggleVisual") != std::string::npos);
  CHECK(source.find("applyCheckboxVisual") != std::string::npos);
  CHECK(source.find("applyProgressVisual") != std::string::npos);

  std::ifstream designInput(designPath);
  REQUIRE(designInput.good());
  std::string design((std::istreambuf_iterator<char>(designInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!design.empty());
  CHECK(design.find("TextField") != std::string::npos);
  CHECK(design.find("request frame-only updates") != std::string::npos);
  CHECK(design.find("State<T>") != std::string::npos);
  CHECK(design.find("bind(...)") != std::string::npos);
  CHECK(design.find("createSlider") != std::string::npos);
  CHECK(design.find("ProgressBar") != std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("TextField") != std::string::npos);
  CHECK(guidelines.find("patch-first") != std::string::npos);
  CHECK(guidelines.find("Binding mode") != std::string::npos);
  CHECK(guidelines.find("applyPlatformServices") != std::string::npos);
  CHECK(guidelines.find("State<T>") != std::string::npos);
  CHECK(guidelines.find("bind(...)") != std::string::npos);
  CHECK(guidelines.find("only a frame in typical app loops") != std::string::npos);
  CHECK(guidelines.find("Toggle") != std::string::npos);
  CHECK(guidelines.find("ProgressBar") != std::string::npos);

  std::ifstream apiRefInput(apiRefPath);
  REQUIRE(apiRefInput.good());
  std::string apiRef((std::istreambuf_iterator<char>(apiRefInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!apiRef.empty());
  CHECK(apiRef.find("Patch-First Widget Interaction Paths") != std::string::npos);
  CHECK(apiRef.find("PrimeStage::State<T>") != std::string::npos);
  CHECK(apiRef.find("PrimeStage::Binding<T>") != std::string::npos);
  CHECK(apiRef.find("AppPlatformServices") != std::string::npos);
  CHECK(apiRef.find("connectHostServices") != std::string::npos);
  CHECK(apiRef.find("applyPlatformServices") != std::string::npos);
  CHECK(apiRef.find("bind(...)") != std::string::npos);
  CHECK(apiRef.find("requestFrame()") != std::string::npos);

  std::ifstream exampleInput(examplePath);
  REQUIRE(exampleInput.good());
  std::string example((std::istreambuf_iterator<char>(exampleInput)),
                      std::istreambuf_iterator<char>());
  REQUIRE(!example.empty());
  CHECK(example.find("field.callbacks.onStateChanged") == std::string::npos);
  CHECK(example.find("app.ui.applyPlatformServices(field);") != std::string::npos);
  CHECK(example.find("app.ui.applyPlatformServices(selectable);") != std::string::npos);
  CHECK(example.find("field.callbacks.onCursorHintChanged") == std::string::npos);
  CHECK(example.find("app.ui.lifecycle().requestFrame();") != std::string::npos);
  CHECK(example.find("if (bridgeResult.requestFrame)") == std::string::npos);
}

TEST_CASE("PrimeStage README and design docs match shipped workflow and API names") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path readmePath = repoRoot / "README.md";
  std::filesystem::path designPath = repoRoot / "docs" / "prime-stage-design.md";
  std::filesystem::path policyPath = repoRoot / "docs" / "api-evolution-policy.md";
  std::filesystem::path apiRefPath = repoRoot / "docs" / "minimal-api-reference.md";
  REQUIRE(std::filesystem::exists(readmePath));
  REQUIRE(std::filesystem::exists(designPath));
  REQUIRE(std::filesystem::exists(policyPath));
  REQUIRE(std::filesystem::exists(apiRefPath));

  std::ifstream readmeInput(readmePath);
  REQUIRE(readmeInput.good());
  std::string readme((std::istreambuf_iterator<char>(readmeInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!readme.empty());
  CHECK(readme.find("./scripts/compile.sh") != std::string::npos);
  CHECK(readme.find("./scripts/compile.sh --test") != std::string::npos);
  CHECK(readme.find("./scripts/lint_canonical_api_surface.sh") != std::string::npos);
  CHECK(readme.find("docs/api-evolution-policy.md") != std::string::npos);
  CHECK(readme.find("cmake --install build-release --prefix") != std::string::npos);
  CHECK(readme.find("find_package(PrimeStage CONFIG REQUIRED)") != std::string::npos);
  CHECK(readme.find("docs/cmake-packaging.md") != std::string::npos);
  CHECK(readme.find("docs/callback-reentrancy-threading.md") != std::string::npos);
  CHECK(readme.find("docs/5-minute-app.md") != std::string::npos);
  CHECK(readme.find("docs/advanced-escape-hatches.md") != std::string::npos);
  CHECK(readme.find("docs/widget-spec-defaults-audit.md") != std::string::npos);
  CHECK(readme.find("docs/example-app-consumer-checklist.md") != std::string::npos);
  CHECK(readme.find("docs/widget-api-review-checklist.md") != std::string::npos);
  CHECK(readme.find("docs/minimal-api-reference.md") != std::string::npos);

  std::ifstream designInput(designPath);
  REQUIRE(designInput.good());
  std::string design((std::istreambuf_iterator<char>(designInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!design.empty());
  CHECK(design.find("createTextField(TextFieldSpec const& spec)") != std::string::npos);
  CHECK(design.find("PrimeStage::App") != std::string::npos);
  CHECK(design.find("createEditBox") == std::string::npos);
  CHECK(design.find("ButtonCallbacks const& callbacks") == std::string::npos);
  CHECK(design.find("## Focus Behavior (Current)") != std::string::npos);
  CHECK(design.find("Focusable by default") != std::string::npos);
  CHECK(design.find("docs/api-evolution-policy.md") != std::string::npos);
  CHECK(design.find("docs/example-app-consumer-checklist.md") != std::string::npos);
  CHECK(design.find("docs/minimal-api-reference.md") != std::string::npos);

  std::ifstream apiRefInput(apiRefPath);
  REQUIRE(apiRefInput.good());
  std::string apiRef((std::istreambuf_iterator<char>(apiRefInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!apiRef.empty());
  CHECK(apiRef.find("include/PrimeStage/App.h") != std::string::npos);
  CHECK(apiRef.find("PrimeStage::App") != std::string::npos);
  CHECK(apiRef.find("PrimeStage::FrameLifecycle") != std::string::npos);
  CHECK(apiRef.find("createWindow(...)") != std::string::npos);
  CHECK(apiRef.find("createPanel(rectStyle, size)") != std::string::npos);
  CHECK(apiRef.find("createLabel(text, textStyle, size)") != std::string::npos);
  CHECK(apiRef.find("createDivider(rectStyle, size)") != std::string::npos);
  CHECK(apiRef.find("createSpacer(size)") != std::string::npos);
  CHECK(apiRef.find("createButton(label, backgroundStyle, textStyle, size)") != std::string::npos);
  CHECK(apiRef.find("createTextField(state, placeholder, backgroundStyle, textStyle, size)") !=
        std::string::npos);
  CHECK(apiRef.find("createToggle(on, trackStyle, knobStyle, size)") != std::string::npos);
  CHECK(apiRef.find("createCheckbox(label, checked, boxStyle, checkStyle, textStyle, size)") !=
        std::string::npos);
  CHECK(apiRef.find("createSlider(value, vertical, trackStyle, fillStyle, thumbStyle, size)") !=
        std::string::npos);
  CHECK(apiRef.find("createTable(columns, rows, selectedRow, size)") != std::string::npos);
  CHECK(apiRef.find("createList(...)") != std::string::npos);
  CHECK(apiRef.find("createTreeView(nodes, size)") != std::string::npos);
  CHECK(apiRef.find("createScrollView(size, showVertical, showHorizontal)") != std::string::npos);
  CHECK(apiRef.find("renderFrameToTarget(...)") != std::string::npos);
}

TEST_CASE("PrimeStage onboarding docs separate canonical and advanced usage") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path fiveMinutePath = repoRoot / "docs" / "5-minute-app.md";
  std::filesystem::path advancedPath = repoRoot / "docs" / "advanced-escape-hatches.md";
  std::filesystem::path readmePath = repoRoot / "README.md";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  REQUIRE(std::filesystem::exists(fiveMinutePath));
  REQUIRE(std::filesystem::exists(advancedPath));
  REQUIRE(std::filesystem::exists(readmePath));
  REQUIRE(std::filesystem::exists(guidelinesPath));

  std::ifstream fiveMinuteInput(fiveMinutePath);
  REQUIRE(fiveMinuteInput.good());
  std::string fiveMinute((std::istreambuf_iterator<char>(fiveMinuteInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!fiveMinute.empty());
  CHECK(fiveMinute.find("examples/canonical/primestage_modern_api.cpp") != std::string::npos);
  CHECK(fiveMinute.find("PrimeStage::App") != std::string::npos);
  CHECK(fiveMinute.find("runRebuildIfNeeded") != std::string::npos);
  CHECK(fiveMinute.find("renderToPng") != std::string::npos);
  CHECK(fiveMinute.find("docs/advanced-escape-hatches.md") != std::string::npos);
  CHECK(fiveMinute.find("PrimeHost::Host") == std::string::npos);
  CHECK(fiveMinute.find("PrimeFrame::LayoutEngine") == std::string::npos);
  CHECK(fiveMinute.find("PrimeStage::LowLevel") == std::string::npos);

  std::ifstream advancedInput(advancedPath);
  REQUIRE(advancedInput.good());
  std::string advanced((std::istreambuf_iterator<char>(advancedInput)),
                       std::istreambuf_iterator<char>());
  REQUIRE(!advanced.empty());
  CHECK(advanced.find("docs/5-minute-app.md") != std::string::npos);
  CHECK(advanced.find("examples/advanced/primestage_widgets.cpp") != std::string::npos);
  CHECK(advanced.find("examples/advanced/primestage_scene.cpp") != std::string::npos);
  CHECK(advanced.find("PrimeHost::Host") != std::string::npos);
  CHECK(advanced.find("PrimeFrame::LayoutEngine") != std::string::npos);
  CHECK(advanced.find("PrimeStage::LowLevel") != std::string::npos);
  CHECK(advanced.find("lowLevelNodeId") != std::string::npos);
  CHECK(advanced.find("examples/canonical/*.cpp") != std::string::npos);

  std::ifstream readmeInput(readmePath);
  REQUIRE(readmeInput.good());
  std::string readme((std::istreambuf_iterator<char>(readmeInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!readme.empty());
  CHECK(readme.find("docs/5-minute-app.md") != std::string::npos);
  CHECK(readme.find("docs/advanced-escape-hatches.md") != std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("docs/5-minute-app.md") != std::string::npos);
  CHECK(guidelines.find("docs/advanced-escape-hatches.md") != std::string::npos);
  CHECK(guidelines.find("[74]") == std::string::npos);
}

TEST_CASE("PrimeStage widget API review checklist is documented and PR-gated") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path checklistPath = repoRoot / "docs" / "widget-api-review-checklist.md";
  std::filesystem::path prTemplatePath = repoRoot / ".github" / "pull_request_template.md";
  std::filesystem::path agentsPath = repoRoot / "AGENTS.md";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  REQUIRE(std::filesystem::exists(checklistPath));
  REQUIRE(std::filesystem::exists(prTemplatePath));
  REQUIRE(std::filesystem::exists(agentsPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));

  std::ifstream checklistInput(checklistPath);
  REQUIRE(checklistInput.good());
  std::string checklist((std::istreambuf_iterator<char>(checklistInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!checklist.empty());
  CHECK(checklist.find("Default Readability") != std::string::npos);
  CHECK(checklist.find("Minimal Constructor Path") != std::string::npos);
  CHECK(checklist.find("Optional Callback Surface") != std::string::npos);
  CHECK(checklist.find("State And Binding Story") != std::string::npos);
  CHECK(checklist.find("PR Gating") != std::string::npos);
  CHECK(checklist.find("docs/widget-spec-defaults-audit.md") != std::string::npos);
  CHECK(checklist.find("tests/unit/test_api_ergonomics.cpp") != std::string::npos);
  CHECK(checklist.find(".github/pull_request_template.md") != std::string::npos);

  std::ifstream prTemplateInput(prTemplatePath);
  REQUIRE(prTemplateInput.good());
  std::string prTemplate((std::istreambuf_iterator<char>(prTemplateInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!prTemplate.empty());
  CHECK(prTemplate.find("Widget API Checklist (Required For New/Changed Widgets)") !=
        std::string::npos);
  CHECK(prTemplate.find("Default readability") != std::string::npos);
  CHECK(prTemplate.find("Minimal constructor path") != std::string::npos);
  CHECK(prTemplate.find("Optional callbacks") != std::string::npos);
  CHECK(prTemplate.find("State/binding story") != std::string::npos);
  CHECK(prTemplate.find("Regression/docs gate") != std::string::npos);
  CHECK(prTemplate.find("./scripts/compile.sh") != std::string::npos);

  std::ifstream agentsInput(agentsPath);
  REQUIRE(agentsInput.good());
  std::string agents((std::istreambuf_iterator<char>(agentsInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!agents.empty());
  CHECK(agents.find("docs/widget-api-review-checklist.md") != std::string::npos);
  CHECK(agents.find(".github/pull_request_template.md") != std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("docs/widget-api-review-checklist.md") != std::string::npos);
  CHECK(guidelines.find("docs/widget-spec-defaults-audit.md") != std::string::npos);
}

TEST_CASE("PrimeStage public naming rules remain aligned with AGENTS guidance") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path includeDir = repoRoot / "include" / "PrimeStage";
  std::filesystem::path agentsPath = repoRoot / "AGENTS.md";
  REQUIRE(std::filesystem::exists(includeDir));
  REQUIRE(std::filesystem::exists(agentsPath));

  std::ifstream agentsInput(agentsPath);
  REQUIRE(agentsInput.good());
  std::string agents((std::istreambuf_iterator<char>(agentsInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!agents.empty());
  CHECK(agents.find("lowerCamelCase") != std::string::npos);
  CHECK(agents.find("avoid `k`-prefixes") != std::string::npos);

  std::regex kPrefixConstantPattern("\\bk[A-Z][A-Za-z0-9_]*\\b");
  std::regex macroPattern("^\\s*#\\s*define\\s+([A-Za-z_][A-Za-z0-9_]*)",
                          std::regex::ECMAScript);
  std::vector<std::filesystem::path> headerPaths;
  for (auto const& entry : std::filesystem::directory_iterator(includeDir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".h") {
      headerPaths.push_back(entry.path());
    }
  }
  REQUIRE(!headerPaths.empty());

  for (auto const& headerPath : headerPaths) {
    std::ifstream input(headerPath);
    REQUIRE(input.good());
    std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    REQUIRE(!source.empty());
    CHECK(source.find("namespace PrimeStage") != std::string::npos);
    CHECK(std::regex_search(source, kPrefixConstantPattern) == false);
    CHECK(source.find("create_edit") == std::string::npos);
    CHECK(source.find("Edit_Box") == std::string::npos);

    std::smatch macroMatch;
    std::string::const_iterator cursor = source.cbegin();
    while (std::regex_search(cursor, source.cend(), macroMatch, macroPattern)) {
      std::string macroName = macroMatch[1].str();
      bool allowed = macroName == "PRAGMA_ONCE" ||
                     macroName.rfind("PS_", 0) == 0 ||
                     macroName.rfind("PRIMESTAGE_", 0) == 0;
      CHECK(allowed);
      cursor = macroMatch.suffix().first;
    }
  }
}

TEST_CASE("PrimeStage API evolution policy defines semver deprecation and migration notes") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path policyPath = repoRoot / "docs" / "api-evolution-policy.md";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  std::filesystem::path designPath = repoRoot / "docs" / "prime-stage-design.md";
  std::filesystem::path agentsPath = repoRoot / "AGENTS.md";
  REQUIRE(std::filesystem::exists(policyPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));
  REQUIRE(std::filesystem::exists(designPath));
  REQUIRE(std::filesystem::exists(agentsPath));

  std::ifstream policyInput(policyPath);
  REQUIRE(policyInput.good());
  std::string policy((std::istreambuf_iterator<char>(policyInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!policy.empty());
  CHECK(policy.find("Versioning Expectations") != std::string::npos);
  CHECK(policy.find("Patch release") != std::string::npos);
  CHECK(policy.find("Minor release") != std::string::npos);
  CHECK(policy.find("Major release") != std::string::npos);
  CHECK(policy.find("Deprecation Process") != std::string::npos);
  CHECK(policy.find("Migration Notes") != std::string::npos);
  CHECK(policy.find("EditBox") != std::string::npos);
  CHECK(policy.find("TextField") != std::string::npos);
  CHECK(policy.find("createEditBox") != std::string::npos);
  CHECK(policy.find("createTextField") != std::string::npos);
  CHECK(policy.find("Compatibility Review Checklist") != std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("docs/api-evolution-policy.md") != std::string::npos);

  std::ifstream designInput(designPath);
  REQUIRE(designInput.good());
  std::string design((std::istreambuf_iterator<char>(designInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!design.empty());
  CHECK(design.find("## API Evolution Policy") != std::string::npos);
  CHECK(design.find("docs/api-evolution-policy.md") != std::string::npos);

  std::ifstream agentsInput(agentsPath);
  REQUIRE(agentsInput.good());
  std::string agents((std::istreambuf_iterator<char>(agentsInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!agents.empty());
  CHECK(agents.find("docs/api-evolution-policy.md") != std::string::npos);
}

TEST_CASE("PrimeStage callback threading and reentrancy contract is documented and wired") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path callbackDocPath =
      repoRoot / "docs" / "callback-reentrancy-threading.md";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  std::filesystem::path designPath = repoRoot / "docs" / "prime-stage-design.md";
  std::filesystem::path apiRefPath = repoRoot / "docs" / "minimal-api-reference.md";
  std::filesystem::path uiHeaderPath = repoRoot / "include" / "PrimeStage" / "Ui.h";
  std::filesystem::path sourceCppPath = repoRoot / "src" / "PrimeStage.cpp";
  REQUIRE(std::filesystem::exists(callbackDocPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));
  REQUIRE(std::filesystem::exists(designPath));
  REQUIRE(std::filesystem::exists(apiRefPath));
  REQUIRE(std::filesystem::exists(uiHeaderPath));
  REQUIRE(std::filesystem::exists(sourceCppPath));

  std::ifstream callbackDocInput(callbackDocPath);
  REQUIRE(callbackDocInput.good());
  std::string callbackDoc((std::istreambuf_iterator<char>(callbackDocInput)),
                          std::istreambuf_iterator<char>());
  REQUIRE(!callbackDoc.empty());
  CHECK(callbackDoc.find("Execution Context") != std::string::npos);
  CHECK(callbackDoc.find("Rebuild/Layout Requests From Callbacks") != std::string::npos);
  CHECK(callbackDoc.find("Reentrancy Guardrails") != std::string::npos);
  CHECK(callbackDoc.find("PrimeStage::LowLevel::appendNodeOnEvent") != std::string::npos);
  CHECK(callbackDoc.find("PrimeStage::LowLevel::appendNodeOnFocus") != std::string::npos);
  CHECK(callbackDoc.find("PrimeStage::LowLevel::appendNodeOnBlur") != std::string::npos);
  CHECK(callbackDoc.find("PrimeStage::LowLevel::NodeCallbackHandle") != std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("Callback Threading/Reentrancy Contract") != std::string::npos);
  CHECK(guidelines.find("docs/callback-reentrancy-threading.md") != std::string::npos);

  std::ifstream designInput(designPath);
  REQUIRE(designInput.good());
  std::string design((std::istreambuf_iterator<char>(designInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!design.empty());
  CHECK(design.find("docs/callback-reentrancy-threading.md") != std::string::npos);
  CHECK(design.find("LowLevel::NodeCallbackHandle") != std::string::npos);

  std::ifstream apiRefInput(apiRefPath);
  REQUIRE(apiRefInput.good());
  std::string apiRef((std::istreambuf_iterator<char>(apiRefInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!apiRef.empty());
  CHECK(apiRef.find("PrimeStage::LowLevel::NodeCallbackTable") != std::string::npos);
  CHECK(apiRef.find("PrimeStage::LowLevel::NodeCallbackHandle") != std::string::npos);
  CHECK(apiRef.find("Low-Level API Quarantine") != std::string::npos);

  std::ifstream uiHeaderInput(uiHeaderPath);
  REQUIRE(uiHeaderInput.good());
  std::string uiHeader((std::istreambuf_iterator<char>(uiHeaderInput)),
                       std::istreambuf_iterator<char>());
  REQUIRE(!uiHeader.empty());
  CHECK(uiHeader.find("Direct reentrant invocation of the same") != std::string::npos);
  CHECK(uiHeader.find("namespace LowLevel") != std::string::npos);
  CHECK(uiHeader.find("struct NodeCallbackTable") != std::string::npos);
  CHECK(uiHeader.find("class NodeCallbackHandle") != std::string::npos);
  CHECK(uiHeader.find("Use PrimeStage::LowLevel::NodeCallbackTable") != std::string::npos);

  std::ifstream sourceCppInput(sourceCppPath);
  REQUIRE(sourceCppInput.good());
  std::string sourceCpp((std::istreambuf_iterator<char>(sourceCppInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!sourceCpp.empty());
  CHECK(sourceCpp.find("CallbackReentryScope") != std::string::npos);
  CHECK(sourceCpp.find("reentrant %s invocation suppressed") != std::string::npos);
  CHECK(sourceCpp.find("LowLevel::NodeCallbackHandle::bind") != std::string::npos);
  CHECK(sourceCpp.find("LowLevel::NodeCallbackHandle::reset") != std::string::npos);
}

TEST_CASE("PrimeStage data ownership and lifetime contract is documented and wired") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path ownershipDocPath =
      repoRoot / "docs" / "data-ownership-lifetime.md";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  std::filesystem::path designPath = repoRoot / "docs" / "prime-stage-design.md";
  std::filesystem::path readmePath = repoRoot / "README.md";
  std::filesystem::path agentsPath = repoRoot / "AGENTS.md";
  std::filesystem::path sourceCppPath = repoRoot / "src" / "PrimeStage.cpp";
  std::filesystem::path todoPath = repoRoot / "docs" / "todo.md";
  REQUIRE(std::filesystem::exists(ownershipDocPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));
  REQUIRE(std::filesystem::exists(designPath));
  REQUIRE(std::filesystem::exists(readmePath));
  REQUIRE(std::filesystem::exists(agentsPath));
  REQUIRE(std::filesystem::exists(sourceCppPath));
  REQUIRE(std::filesystem::exists(todoPath));

  std::ifstream ownershipInput(ownershipDocPath);
  REQUIRE(ownershipInput.good());
  std::string ownershipDoc((std::istreambuf_iterator<char>(ownershipInput)),
                           std::istreambuf_iterator<char>());
  REQUIRE(!ownershipDoc.empty());
  CHECK(ownershipDoc.find("In Widget Specs") != std::string::npos);
  CHECK(ownershipDoc.find("Callback Capture Ownership") != std::string::npos);
  CHECK(ownershipDoc.find("TableRowInfo::row") != std::string::npos);
  CHECK(ownershipDoc.find("callback payload data that can outlive build-call source buffers") !=
        std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("callback-capture lifetime rules") != std::string::npos);
  CHECK(guidelines.find("Lifetime Contract") != std::string::npos);
  CHECK(guidelines.find("TableRowInfo::row") != std::string::npos);
  CHECK(guidelines.find("docs/data-ownership-lifetime.md") != std::string::npos);

  std::ifstream designInput(designPath);
  REQUIRE(designInput.good());
  std::string design((std::istreambuf_iterator<char>(designInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!design.empty());
  CHECK(design.find("docs/data-ownership-lifetime.md") != std::string::npos);

  std::ifstream readmeInput(readmePath);
  REQUIRE(readmeInput.good());
  std::string readme((std::istreambuf_iterator<char>(readmeInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!readme.empty());
  CHECK(readme.find("docs/data-ownership-lifetime.md") != std::string::npos);

  std::ifstream agentsInput(agentsPath);
  REQUIRE(agentsInput.good());
  std::string agents((std::istreambuf_iterator<char>(agentsInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!agents.empty());
  CHECK(agents.find("docs/data-ownership-lifetime.md") != std::string::npos);

  std::ifstream sourceCppInput(sourceCppPath);
  REQUIRE(sourceCppInput.good());
  std::string sourceCpp((std::istreambuf_iterator<char>(sourceCppInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!sourceCpp.empty());
  CHECK(sourceCpp.find("ownedRows") != std::string::npos);
  CHECK(sourceCpp.find("rowViewScratch") != std::string::npos);
  CHECK(sourceCpp.find("interaction->ownedRows.push_back(std::move(ownedRow));") !=
        std::string::npos);
  CHECK(sourceCpp.find("info.row = std::span<const std::string_view>(interaction->rowViewScratch);") !=
        std::string::npos);
  CHECK(sourceCpp.find("interaction->rows = spec.rows;") == std::string::npos);

  std::ifstream todoInput(todoPath);
  REQUIRE(todoInput.good());
  std::string todo((std::istreambuf_iterator<char>(todoInput)),
                   std::istreambuf_iterator<char>());
  REQUIRE(!todo.empty());
  CHECK(todo.find("[48] Define data ownership/lifetime contracts in public specs.") !=
        std::string::npos);
  CHECK(todo.find("docs/data-ownership-lifetime.md") != std::string::npos);
}

TEST_CASE("PrimeStage install export package workflow supports find_package consumers") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path cmakePath = repoRoot / "CMakeLists.txt";
  std::filesystem::path configTemplatePath = repoRoot / "cmake" / "PrimeStageConfig.cmake.in";
  std::filesystem::path packagingDocsPath = repoRoot / "docs" / "cmake-packaging.md";
  std::filesystem::path smokeScriptPath =
      repoRoot / "tests" / "cmake" / "run_find_package_smoke.cmake";
  std::filesystem::path smokeProjectPath =
      repoRoot / "tests" / "cmake" / "find_package_smoke" / "CMakeLists.txt";
  std::filesystem::path smokeMainPath =
      repoRoot / "tests" / "cmake" / "find_package_smoke" / "main.cpp";
  REQUIRE(std::filesystem::exists(cmakePath));
  REQUIRE(std::filesystem::exists(configTemplatePath));
  REQUIRE(std::filesystem::exists(packagingDocsPath));
  REQUIRE(std::filesystem::exists(smokeScriptPath));
  REQUIRE(std::filesystem::exists(smokeProjectPath));
  REQUIRE(std::filesystem::exists(smokeMainPath));

  std::ifstream cmakeInput(cmakePath);
  REQUIRE(cmakeInput.good());
  std::string cmake((std::istreambuf_iterator<char>(cmakeInput)), std::istreambuf_iterator<char>());
  REQUIRE(!cmake.empty());
  CHECK(cmake.find("install(TARGETS PrimeStage") != std::string::npos);
  CHECK(cmake.find("install(EXPORT PrimeStageTargets") != std::string::npos);
  CHECK(cmake.find("configure_package_config_file") != std::string::npos);
  CHECK(cmake.find("write_basic_package_version_file") != std::string::npos);
  CHECK(cmake.find("PrimeStage_find_package_smoke") != std::string::npos);
  CHECK(cmake.find("tests/cmake/run_find_package_smoke.cmake") != std::string::npos);

  std::ifstream configTemplateInput(configTemplatePath);
  REQUIRE(configTemplateInput.good());
  std::string configTemplate((std::istreambuf_iterator<char>(configTemplateInput)),
                             std::istreambuf_iterator<char>());
  REQUIRE(!configTemplate.empty());
  CHECK(configTemplate.find("find_package(PrimeFrame CONFIG QUIET)") != std::string::npos);
  CHECK(configTemplate.find("PrimeStage::PrimeFrame") != std::string::npos);
  CHECK(configTemplate.find("find_package(PrimeManifest CONFIG QUIET)") != std::string::npos);
  CHECK(configTemplate.find("PrimeStageTargets.cmake") != std::string::npos);
  CHECK(configTemplate.find("check_required_components(PrimeStage)") != std::string::npos);

  std::ifstream packagingDocsInput(packagingDocsPath);
  REQUIRE(packagingDocsInput.good());
  std::string packagingDocs((std::istreambuf_iterator<char>(packagingDocsInput)),
                            std::istreambuf_iterator<char>());
  REQUIRE(!packagingDocs.empty());
  CHECK(packagingDocs.find("find_package(PrimeStage CONFIG REQUIRED)") != std::string::npos);
  CHECK(packagingDocs.find("PrimeStage_find_package_smoke") != std::string::npos);

  std::ifstream smokeProjectInput(smokeProjectPath);
  REQUIRE(smokeProjectInput.good());
  std::string smokeProject((std::istreambuf_iterator<char>(smokeProjectInput)),
                           std::istreambuf_iterator<char>());
  REQUIRE(!smokeProject.empty());
  CHECK(smokeProject.find("find_package(PrimeStage CONFIG REQUIRED)") != std::string::npos);
  CHECK(smokeProject.find("PrimeStage::PrimeStage") != std::string::npos);

  std::ifstream smokeMainInput(smokeMainPath);
  REQUIRE(smokeMainInput.good());
  std::string smokeMain((std::istreambuf_iterator<char>(smokeMainInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!smokeMain.empty());
  CHECK(smokeMain.find("PrimeStage/AppRuntime.h") != std::string::npos);
}

TEST_CASE("PrimeStage presubmit workflow covers build matrix and compatibility path") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path workflowPath = repoRoot / ".github" / "workflows" / "presubmit.yml";
  REQUIRE(std::filesystem::exists(workflowPath));

  std::ifstream input(workflowPath);
  REQUIRE(input.good());
  std::string workflow((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  REQUIRE(!workflow.empty());

  CHECK(workflow.find("build_type") != std::string::npos);
  CHECK(workflow.find("relwithdebinfo") != std::string::npos);
  CHECK(workflow.find("release") != std::string::npos);
  CHECK(workflow.find("canonical-api-surface-lint") != std::string::npos);
  CHECK(workflow.find("Canonical API Surface Lint") != std::string::npos);
  CHECK(workflow.find("./scripts/lint_canonical_api_surface.sh") != std::string::npos);
  CHECK(workflow.find("./scripts/compile.sh --${{ matrix.build_type }} --test") != std::string::npos);

  CHECK(workflow.find("PRIMESTAGE_HEADLESS_ONLY=ON") != std::string::npos);
  CHECK(workflow.find("PRIMESTAGE_ENABLE_PRIMEMANIFEST=OFF") != std::string::npos);

  CHECK(workflow.find("--test-case=\"*focus*,*interaction*,*ergonomics*\"") != std::string::npos);
}

TEST_CASE("PrimeStage canonical API surface lint script is wired and documented") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path lintScriptPath = repoRoot / "scripts" / "lint_canonical_api_surface.sh";
  std::filesystem::path workflowPath = repoRoot / ".github" / "workflows" / "presubmit.yml";
  std::filesystem::path checklistPath = repoRoot / "docs" / "example-app-consumer-checklist.md";
  std::filesystem::path readmePath = repoRoot / "README.md";
  std::filesystem::path agentsPath = repoRoot / "AGENTS.md";
  REQUIRE(std::filesystem::exists(lintScriptPath));
  REQUIRE(std::filesystem::exists(workflowPath));
  REQUIRE(std::filesystem::exists(checklistPath));
  REQUIRE(std::filesystem::exists(readmePath));
  REQUIRE(std::filesystem::exists(agentsPath));

  std::ifstream lintInput(lintScriptPath);
  REQUIRE(lintInput.good());
  std::string lint((std::istreambuf_iterator<char>(lintInput)), std::istreambuf_iterator<char>());
  REQUIRE(!lint.empty());
  CHECK(lint.find("examples/canonical") != std::string::npos);
  CHECK(lint.find("README.md") != std::string::npos);
  CHECK(lint.find("docs/5-minute-app.md") != std::string::npos);
  CHECK(lint.find("PrimeFrame::") != std::string::npos);
  CHECK(lint.find("NodeId") != std::string::npos);
  CHECK(lint.find("std::get_if<PrimeHost::") != std::string::npos);
  CHECK(lint.find("mktemp") != std::string::npos);
  CHECK(lint.find("canonical API surface lint failed") != std::string::npos);

  std::ifstream workflowInput(workflowPath);
  REQUIRE(workflowInput.good());
  std::string workflow((std::istreambuf_iterator<char>(workflowInput)),
                       std::istreambuf_iterator<char>());
  REQUIRE(!workflow.empty());
  CHECK(workflow.find("canonical-api-surface-lint") != std::string::npos);
  CHECK(workflow.find("./scripts/lint_canonical_api_surface.sh") != std::string::npos);

  std::ifstream checklistInput(checklistPath);
  REQUIRE(checklistInput.good());
  std::string checklist((std::istreambuf_iterator<char>(checklistInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!checklist.empty());
  CHECK(checklist.find("./scripts/lint_canonical_api_surface.sh") != std::string::npos);

  std::ifstream readmeInput(readmePath);
  REQUIRE(readmeInput.good());
  std::string readme((std::istreambuf_iterator<char>(readmeInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!readme.empty());
  CHECK(readme.find("./scripts/lint_canonical_api_surface.sh") != std::string::npos);

  std::ifstream agentsInput(agentsPath);
  REQUIRE(agentsInput.good());
  std::string agents((std::istreambuf_iterator<char>(agentsInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!agents.empty());
  CHECK(agents.find("scripts/lint_canonical_api_surface.sh") != std::string::npos);
}

TEST_CASE("PrimeStage end-to-end ergonomics suite is wired and guarded") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path cmakePath = repoRoot / "CMakeLists.txt";
  std::filesystem::path workflowPath = repoRoot / ".github" / "workflows" / "presubmit.yml";
  std::filesystem::path suitePath = repoRoot / "tests" / "unit" / "test_end_to_end_ergonomics.cpp";
  std::filesystem::path agentsPath = repoRoot / "AGENTS.md";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  REQUIRE(std::filesystem::exists(cmakePath));
  REQUIRE(std::filesystem::exists(workflowPath));
  REQUIRE(std::filesystem::exists(suitePath));
  REQUIRE(std::filesystem::exists(agentsPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));

  std::ifstream cmakeInput(cmakePath);
  REQUIRE(cmakeInput.good());
  std::string cmake((std::istreambuf_iterator<char>(cmakeInput)), std::istreambuf_iterator<char>());
  REQUIRE(!cmake.empty());
  CHECK(cmake.find("tests/unit/test_end_to_end_ergonomics.cpp") != std::string::npos);

  std::ifstream workflowInput(workflowPath);
  REQUIRE(workflowInput.good());
  std::string workflow((std::istreambuf_iterator<char>(workflowInput)),
                       std::istreambuf_iterator<char>());
  REQUIRE(!workflow.empty());
  CHECK(workflow.find("--test-case=\"*focus*,*interaction*,*ergonomics*\"") != std::string::npos);

  std::ifstream suiteInput(suitePath);
  REQUIRE(suiteInput.good());
  std::string suite((std::istreambuf_iterator<char>(suiteInput)),
                    std::istreambuf_iterator<char>());
  REQUIRE(!suite.empty());
  CHECK(suite.find("end-to-end ergonomics high-level app flow") != std::string::npos);
  CHECK(suite.find("bridgeHostInputEvent") != std::string::npos);
  CHECK(suite.find("PointerPhase::Down") != std::string::npos);
  CHECK(suite.find("HostKey::Backspace") != std::string::npos);
  CHECK(suite.find("TextFieldSpec") != std::string::npos);
  CHECK(suite.find("static_assert(SupportsDeclarativeConvenienceErgonomics<PrimeStage::UiNode>)") !=
        std::string::npos);
  CHECK(suite.find("PrimeStage::LowLevel::") == std::string::npos);
  CHECK(suite.find(".lowLevelNodeId(") == std::string::npos);
  CHECK(suite.find(".nodeId(") == std::string::npos);

  std::ifstream agentsInput(agentsPath);
  REQUIRE(agentsInput.good());
  std::string agents((std::istreambuf_iterator<char>(agentsInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!agents.empty());
  CHECK(agents.find("tests/unit/test_end_to_end_ergonomics.cpp") != std::string::npos);
  CHECK(agents.find("no `PrimeStage::LowLevel` usage") != std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("tests/unit/test_end_to_end_ergonomics.cpp") != std::string::npos);
}

TEST_CASE("PrimeStage deterministic visual-test harness and workflow are wired") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path harnessPath = repoRoot / "tests" / "unit" / "visual_test_harness.h";
  std::filesystem::path visualTestPath = repoRoot / "tests" / "unit" / "test_visual_regression.cpp";
  std::filesystem::path snapshotPath = repoRoot / "tests" / "snapshots" / "interaction_visuals.snap";
  std::filesystem::path docsPath = repoRoot / "docs" / "visual-test-harness.md";
  REQUIRE(std::filesystem::exists(harnessPath));
  REQUIRE(std::filesystem::exists(visualTestPath));
  REQUIRE(std::filesystem::exists(snapshotPath));
  REQUIRE(std::filesystem::exists(docsPath));

  std::ifstream harnessInput(harnessPath);
  REQUIRE(harnessInput.good());
  std::string harness((std::istreambuf_iterator<char>(harnessInput)),
                      std::istreambuf_iterator<char>());
  REQUIRE(!harness.empty());
  CHECK(harness.find("struct VisualHarnessConfig") != std::string::npos);
  CHECK(harness.find("snapshotVersion = \"interaction_v2\"") != std::string::npos);
  CHECK(harness.find("fontPolicy = \"command_batch_no_raster\"") != std::string::npos);
  CHECK(harness.find("layoutScale = 1.0f") != std::string::npos);
  CHECK(harness.find("deterministicSnapshotHeader") != std::string::npos);

  std::ifstream visualTestInput(visualTestPath);
  REQUIRE(visualTestInput.good());
  std::string visualTests((std::istreambuf_iterator<char>(visualTestInput)),
                          std::istreambuf_iterator<char>());
  REQUIRE(!visualTests.empty());
  CHECK(visualTests.find("visual harness metadata pins deterministic inputs") != std::string::npos);
  CHECK(visualTests.find("PRIMESTAGE_UPDATE_SNAPSHOTS") != std::string::npos);
  CHECK(visualTests.find("deterministicSnapshotHeader") != std::string::npos);

  std::ifstream snapshotInput(snapshotPath);
  REQUIRE(snapshotInput.good());
  std::string snapshot((std::istreambuf_iterator<char>(snapshotInput)),
                       std::istreambuf_iterator<char>());
  REQUIRE(!snapshot.empty());
  CHECK(snapshot.find("[harness]") != std::string::npos);
  CHECK(snapshot.find("version=interaction_v2") != std::string::npos);
  CHECK(snapshot.find("font_policy=command_batch_no_raster") != std::string::npos);
  CHECK(snapshot.find("layout_scale=1.00") != std::string::npos);

  std::ifstream docsInput(docsPath);
  REQUIRE(docsInput.good());
  std::string docs((std::istreambuf_iterator<char>(docsInput)), std::istreambuf_iterator<char>());
  REQUIRE(!docs.empty());
  CHECK(docs.find("Golden Update Workflow") != std::string::npos);
  CHECK(docs.find("Failure Triage Guidance") != std::string::npos);
  CHECK(docs.find("PRIMESTAGE_UPDATE_SNAPSHOTS=1 ./scripts/compile.sh --test") !=
        std::string::npos);
}

TEST_CASE("PrimeStage performance benchmark harness and budget gate are wired") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path benchmarkPath = repoRoot / "tests" / "perf" / "PrimeStage_benchmarks.cpp";
  std::filesystem::path budgetPath = repoRoot / "tests" / "perf" / "perf_budgets.txt";
  std::filesystem::path docsPath = repoRoot / "docs" / "performance-benchmarks.md";
  std::filesystem::path cmakePath = repoRoot / "CMakeLists.txt";
  std::filesystem::path scriptPath = repoRoot / "scripts" / "compile.sh";
  std::filesystem::path workflowPath = repoRoot / ".github" / "workflows" / "presubmit.yml";
  REQUIRE(std::filesystem::exists(benchmarkPath));
  REQUIRE(std::filesystem::exists(budgetPath));
  REQUIRE(std::filesystem::exists(docsPath));
  REQUIRE(std::filesystem::exists(cmakePath));
  REQUIRE(std::filesystem::exists(scriptPath));
  REQUIRE(std::filesystem::exists(workflowPath));

  std::ifstream benchmarkInput(benchmarkPath);
  REQUIRE(benchmarkInput.good());
  std::string benchmark((std::istreambuf_iterator<char>(benchmarkInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!benchmark.empty());
  CHECK(benchmark.find("scene.dashboard.rebuild.p95_us") != std::string::npos);
  CHECK(benchmark.find("scene.tree.render.p95_us") != std::string::npos);
  CHECK(benchmark.find("interaction.typing.p95_us") != std::string::npos);
  CHECK(benchmark.find("interaction.drag.p95_us") != std::string::npos);
  CHECK(benchmark.find("interaction.wheel.p95_us") != std::string::npos);
  CHECK(benchmark.find("PrimeStage::renderFrameToTarget") != std::string::npos);

  std::ifstream budgetInput(budgetPath);
  REQUIRE(budgetInput.good());
  std::string budgets((std::istreambuf_iterator<char>(budgetInput)),
                      std::istreambuf_iterator<char>());
  REQUIRE(!budgets.empty());
  CHECK(budgets.find("scene.dashboard.rebuild.p95_us") != std::string::npos);
  CHECK(budgets.find("scene.tree.render.p95_us") != std::string::npos);
  CHECK(budgets.find("interaction.typing.p95_us") != std::string::npos);
  CHECK(budgets.find("interaction.drag.p95_us") != std::string::npos);
  CHECK(budgets.find("interaction.wheel.p95_us") != std::string::npos);

  std::ifstream docsInput(docsPath);
  REQUIRE(docsInput.good());
  std::string docs((std::istreambuf_iterator<char>(docsInput)), std::istreambuf_iterator<char>());
  REQUIRE(!docs.empty());
  CHECK(docs.find("typing, slider drag, and wheel scrolling") != std::string::npos);
  CHECK(docs.find("./scripts/compile.sh --release --perf-budget") != std::string::npos);

  std::ifstream cmakeInput(cmakePath);
  REQUIRE(cmakeInput.good());
  std::string cmake((std::istreambuf_iterator<char>(cmakeInput)), std::istreambuf_iterator<char>());
  REQUIRE(!cmake.empty());
  CHECK(cmake.find("PRIMESTAGE_BUILD_BENCHMARKS") != std::string::npos);
  CHECK(cmake.find("PrimeStage_benchmarks") != std::string::npos);

  std::ifstream scriptInput(scriptPath);
  REQUIRE(scriptInput.good());
  std::string script((std::istreambuf_iterator<char>(scriptInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!script.empty());
  CHECK(script.find("--perf") != std::string::npos);
  CHECK(script.find("--perf-budget") != std::string::npos);
  CHECK(script.find("tests/perf/perf_budgets.txt") != std::string::npos);
  CHECK(script.find("PrimeStage_benchmarks") != std::string::npos);

  std::ifstream workflowInput(workflowPath);
  REQUIRE(workflowInput.good());
  std::string workflow((std::istreambuf_iterator<char>(workflowInput)),
                       std::istreambuf_iterator<char>());
  REQUIRE(!workflow.empty());
  CHECK(workflow.find("Performance Budget Gate") != std::string::npos);
  CHECK(workflow.find("PrimeStage_benchmarks") != std::string::npos);
  CHECK(workflow.find("--budget-file tests/perf/perf_budgets.txt") != std::string::npos);
  CHECK(workflow.find("--check-budgets") != std::string::npos);
}

TEST_CASE("PrimeStage render diagnostics expose structured status contracts") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path renderHeader = repoRoot / "include" / "PrimeStage" / "Render.h";
  std::filesystem::path renderSource = repoRoot / "src" / "Render.cpp";
  std::filesystem::path renderTests = repoRoot / "tests" / "unit" / "test_render.cpp";
  std::filesystem::path cmakePath = repoRoot / "CMakeLists.txt";
  std::filesystem::path benchmarkPath = repoRoot / "tests" / "perf" / "PrimeStage_benchmarks.cpp";
  std::filesystem::path docsPath = repoRoot / "docs" / "render-diagnostics.md";
  std::filesystem::path designPath = repoRoot / "docs" / "prime-stage-design.md";
  std::filesystem::path readmePath = repoRoot / "README.md";
  std::filesystem::path todoPath = repoRoot / "docs" / "todo.md";
  REQUIRE(std::filesystem::exists(renderHeader));
  REQUIRE(std::filesystem::exists(renderSource));
  REQUIRE(std::filesystem::exists(renderTests));
  REQUIRE(std::filesystem::exists(cmakePath));
  REQUIRE(std::filesystem::exists(benchmarkPath));
  REQUIRE(std::filesystem::exists(docsPath));
  REQUIRE(std::filesystem::exists(designPath));
  REQUIRE(std::filesystem::exists(readmePath));
  REQUIRE(std::filesystem::exists(todoPath));

  std::ifstream headerInput(renderHeader);
  REQUIRE(headerInput.good());
  std::string header((std::istreambuf_iterator<char>(headerInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!header.empty());
  CHECK(header.find("enum class RenderStatusCode") != std::string::npos);
  CHECK(header.find("struct RenderStatus") != std::string::npos);
  CHECK(header.find("struct CornerStyleMetadata") != std::string::npos);
  CHECK(header.find("CornerStyleMetadata cornerStyle{};") != std::string::npos);
  CHECK(header.find("RenderStatus renderFrameToTarget") != std::string::npos);
  CHECK(header.find("RenderStatus renderFrameToPng") != std::string::npos);
  CHECK(header.find("std::string_view renderStatusMessage") != std::string::npos);
  CHECK(header.find("InvalidTargetStride") != std::string::npos);
  CHECK(header.find("PngWriteFailed") != std::string::npos);

  std::ifstream sourceInput(renderSource);
  REQUIRE(sourceInput.good());
  std::string source((std::istreambuf_iterator<char>(sourceInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!source.empty());
  CHECK(source.find("target stride must be at least width * 4 bytes") != std::string::npos);
  CHECK(source.find("LayoutMissingRootMetrics") != std::string::npos);
  CHECK(source.find("PngPathEmpty") != std::string::npos);
  CHECK(source.find("PngWriteFailed") != std::string::npos);
  CHECK(source.find("renderStatusMessage(RenderStatusCode code)") != std::string::npos);
  CHECK(source.find("resolve_corner_radius") != std::string::npos);
  CHECK(source.find("theme_color(") == std::string::npos);
  CHECK(source.find("colors_close(") == std::string::npos);

  std::ifstream testsInput(renderTests);
  REQUIRE(testsInput.good());
  std::string tests((std::istreambuf_iterator<char>(testsInput)),
                    std::istreambuf_iterator<char>());
  REQUIRE(!tests.empty());
  CHECK(tests.find("render target diagnostics expose actionable status") != std::string::npos);
  CHECK(tests.find("PNG diagnostics report layout and path failures") != std::string::npos);
  CHECK(tests.find("RenderStatusCode::InvalidTargetStride") != std::string::npos);
  CHECK(tests.find("render path overloads and png write failures are covered") !=
        std::string::npos);
  CHECK(tests.find("RenderStatusCode::PngWriteFailed") != std::string::npos);
  CHECK(tests.find("renderFrameToTarget(frame, target, options)") != std::string::npos);
  CHECK(tests.find("renderFrameToPng(frame, \"headless_frame.png\", options)") !=
        std::string::npos);
  CHECK(tests.find("rounded-corner policy is deterministic under theme changes") !=
        std::string::npos);

  std::ifstream cmakeInput(cmakePath);
  REQUIRE(cmakeInput.good());
  std::string cmake((std::istreambuf_iterator<char>(cmakeInput)),
                    std::istreambuf_iterator<char>());
  REQUIRE(!cmake.empty());
  CHECK(cmake.find("tests/unit/test_render.cpp") != std::string::npos);

  std::ifstream benchmarkInput(benchmarkPath);
  REQUIRE(benchmarkInput.good());
  std::string benchmark((std::istreambuf_iterator<char>(benchmarkInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!benchmark.empty());
  CHECK(benchmark.find("PrimeStage::RenderStatus status = PrimeStage::renderFrameToTarget") !=
        std::string::npos);
  CHECK(benchmark.find("if (!status.ok())") != std::string::npos);

  std::ifstream docsInput(docsPath);
  REQUIRE(docsInput.good());
  std::string docs((std::istreambuf_iterator<char>(docsInput)), std::istreambuf_iterator<char>());
  REQUIRE(!docs.empty());
  CHECK(docs.find("RenderStatusCode") != std::string::npos);
  CHECK(docs.find("renderStatusMessage") != std::string::npos);
  CHECK(docs.find("InvalidTargetBuffer") != std::string::npos);
  CHECK(docs.find("CornerStyleMetadata") != std::string::npos);
  CHECK(docs.find("deterministic under theme palette changes") != std::string::npos);

  std::ifstream designInput(designPath);
  REQUIRE(designInput.good());
  std::string design((std::istreambuf_iterator<char>(designInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!design.empty());
  CHECK(design.find("docs/render-diagnostics.md") != std::string::npos);

  std::ifstream readmeInput(readmePath);
  REQUIRE(readmeInput.good());
  std::string readme((std::istreambuf_iterator<char>(readmeInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!readme.empty());
  CHECK(readme.find("docs/render-diagnostics.md") != std::string::npos);

  std::ifstream todoInput(todoPath);
  REQUIRE(todoInput.good());
  std::string todo((std::istreambuf_iterator<char>(todoInput)),
                   std::istreambuf_iterator<char>());
  REQUIRE(!todo.empty());
  CHECK(todo.find("[43] Improve render API diagnostics and failure reporting.") !=
        std::string::npos);
  CHECK(todo.find("[44] Remove renderer style heuristics tied to theme color indices.") !=
        std::string::npos);
  CHECK(todo.find("[45] Add render-path test coverage.") != std::string::npos);
}

TEST_CASE("PrimeStage versioning derives runtime version from CMake metadata") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path cmakePath = repoRoot / "CMakeLists.txt";
  std::filesystem::path versionTemplatePath = repoRoot / "cmake" / "PrimeStageVersion.h.in";
  std::filesystem::path sourceCppPath = repoRoot / "src" / "PrimeStage.cpp";
  std::filesystem::path sanityTestPath = repoRoot / "tests" / "unit" / "test_sanity.cpp";
  std::filesystem::path todoPath = repoRoot / "docs" / "todo.md";
  REQUIRE(std::filesystem::exists(cmakePath));
  REQUIRE(std::filesystem::exists(versionTemplatePath));
  REQUIRE(std::filesystem::exists(sourceCppPath));
  REQUIRE(std::filesystem::exists(sanityTestPath));
  REQUIRE(std::filesystem::exists(todoPath));

  std::ifstream cmakeInput(cmakePath);
  REQUIRE(cmakeInput.good());
  std::string cmake((std::istreambuf_iterator<char>(cmakeInput)), std::istreambuf_iterator<char>());
  REQUIRE(!cmake.empty());
  CHECK(cmake.find("project(PrimeStage VERSION") != std::string::npos);
  CHECK(cmake.find("PrimeStageVersion.h.in") != std::string::npos);
  CHECK(cmake.find("GeneratedVersion.h") != std::string::npos);
  CHECK(cmake.find("PRIMESTAGE_GENERATED_INCLUDE_DIR") != std::string::npos);

  std::ifstream versionTemplateInput(versionTemplatePath);
  REQUIRE(versionTemplateInput.good());
  std::string versionTemplate((std::istreambuf_iterator<char>(versionTemplateInput)),
                              std::istreambuf_iterator<char>());
  REQUIRE(!versionTemplate.empty());
  CHECK(versionTemplate.find("PRIMESTAGE_VERSION_MAJOR @PROJECT_VERSION_MAJOR@") !=
        std::string::npos);
  CHECK(versionTemplate.find("PRIMESTAGE_VERSION_MINOR @PROJECT_VERSION_MINOR@") !=
        std::string::npos);
  CHECK(versionTemplate.find("PRIMESTAGE_VERSION_PATCH @PROJECT_VERSION_PATCH@") !=
        std::string::npos);
  CHECK(versionTemplate.find("PRIMESTAGE_VERSION_STRING \"@PROJECT_VERSION@\"") !=
        std::string::npos);

  std::ifstream sourceCppInput(sourceCppPath);
  REQUIRE(sourceCppInput.good());
  std::string sourceCpp((std::istreambuf_iterator<char>(sourceCppInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!sourceCpp.empty());
  CHECK(sourceCpp.find("PrimeStage/GeneratedVersion.h") != std::string::npos);
  CHECK(sourceCpp.find("PRIMESTAGE_VERSION_MAJOR") != std::string::npos);
  CHECK(sourceCpp.find("PRIMESTAGE_VERSION_STRING") != std::string::npos);
  CHECK(sourceCpp.find("return \"0.1.0\";") == std::string::npos);

  std::ifstream sanityTestInput(sanityTestPath);
  REQUIRE(sanityTestInput.good());
  std::string sanityTest((std::istreambuf_iterator<char>(sanityTestInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!sanityTest.empty());
  CHECK(sanityTest.find("project(PrimeStage VERSION") != std::string::npos);
  CHECK(sanityTest.find("PrimeStage::getVersionString()") != std::string::npos);

  std::ifstream todoInput(todoPath);
  REQUIRE(todoInput.good());
  std::string todo((std::istreambuf_iterator<char>(todoInput)),
                   std::istreambuf_iterator<char>());
  REQUIRE(!todo.empty());
  CHECK(todo.find("[46] Establish single-source versioning.") != std::string::npos);
}

TEST_CASE("PrimeStage dependency refs are pinned and policy is documented") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path cmakePath = repoRoot / "CMakeLists.txt";
  std::filesystem::path policyPath = repoRoot / "docs" / "dependency-resolution-policy.md";
  std::filesystem::path readmePath = repoRoot / "README.md";
  std::filesystem::path agentsPath = repoRoot / "AGENTS.md";
  std::filesystem::path todoPath = repoRoot / "docs" / "todo.md";
  REQUIRE(std::filesystem::exists(cmakePath));
  REQUIRE(std::filesystem::exists(policyPath));
  REQUIRE(std::filesystem::exists(readmePath));
  REQUIRE(std::filesystem::exists(agentsPath));
  REQUIRE(std::filesystem::exists(todoPath));

  std::ifstream cmakeInput(cmakePath);
  REQUIRE(cmakeInput.good());
  std::string cmake((std::istreambuf_iterator<char>(cmakeInput)), std::istreambuf_iterator<char>());
  REQUIRE(!cmake.empty());
  CHECK(cmake.find("set(PRIMEFRAME_GIT_TAG \"master\"") == std::string::npos);
  CHECK(cmake.find("set(PRIMEHOST_GIT_TAG \"master\"") == std::string::npos);
  CHECK(cmake.find("set(PRIMESTAGE_PRIMEMANIFEST_GIT_TAG \"master\"") == std::string::npos);
  CHECK(cmake.find("set(PRIMEFRAME_GIT_TAG \"180a3c2ec0af4b56eba1f5e74b4e74ba90efdecc\"") !=
        std::string::npos);
  CHECK(cmake.find("set(PRIMEHOST_GIT_TAG \"762dbfbc77cd46a009e8a9b352404ffe7b81e66e\"") !=
        std::string::npos);
  CHECK(cmake.find("set(PRIMESTAGE_PRIMEMANIFEST_GIT_TAG \"4e65e2b393b63ec798f35fdd89f6f32d2205675c\"") !=
        std::string::npos);

  std::ifstream policyInput(policyPath);
  REQUIRE(policyInput.good());
  std::string policy((std::istreambuf_iterator<char>(policyInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!policy.empty());
  CHECK(policy.find("Default Pins") != std::string::npos);
  CHECK(policy.find("Do not use floating defaults such as `master`") != std::string::npos);
  CHECK(policy.find("PRIMEFRAME_GIT_TAG") != std::string::npos);
  CHECK(policy.find("PRIMEHOST_GIT_TAG") != std::string::npos);
  CHECK(policy.find("PRIMESTAGE_PRIMEMANIFEST_GIT_TAG") != std::string::npos);

  std::ifstream readmeInput(readmePath);
  REQUIRE(readmeInput.good());
  std::string readme((std::istreambuf_iterator<char>(readmeInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!readme.empty());
  CHECK(readme.find("docs/dependency-resolution-policy.md") != std::string::npos);

  std::ifstream agentsInput(agentsPath);
  REQUIRE(agentsInput.good());
  std::string agents((std::istreambuf_iterator<char>(agentsInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!agents.empty());
  CHECK(agents.find("docs/dependency-resolution-policy.md") != std::string::npos);

  std::ifstream todoInput(todoPath);
  REQUIRE(todoInput.good());
  std::string todo((std::istreambuf_iterator<char>(todoInput)),
                   std::istreambuf_iterator<char>());
  REQUIRE(!todo.empty());
  CHECK(todo.find("[47] Make dependency resolution reproducible.") != std::string::npos);
}

TEST_CASE("PrimeStage input focus property fuzz coverage is wired and deterministic") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path fuzzTestPath = repoRoot / "tests" / "unit" / "test_state_machine_fuzz.cpp";
  std::filesystem::path cmakePath = repoRoot / "CMakeLists.txt";
  std::filesystem::path todoPath = repoRoot / "docs" / "todo.md";
  std::filesystem::path agentsPath = repoRoot / "AGENTS.md";
  REQUIRE(std::filesystem::exists(fuzzTestPath));
  REQUIRE(std::filesystem::exists(cmakePath));
  REQUIRE(std::filesystem::exists(todoPath));
  REQUIRE(std::filesystem::exists(agentsPath));

  std::ifstream fuzzInput(fuzzTestPath);
  REQUIRE(fuzzInput.good());
  std::string fuzz((std::istreambuf_iterator<char>(fuzzInput)),
                   std::istreambuf_iterator<char>());
  REQUIRE(!fuzz.empty());
  CHECK(fuzz.find("FuzzSeed = 0xD1CEB00Cu") != std::string::npos);
  CHECK(fuzz.find("input focus state machine keeps invariants under deterministic fuzz") !=
        std::string::npos);
  CHECK(fuzz.find("input focus regression corpus preserves invariants") != std::string::npos);
  CHECK(fuzz.find("assertInvariants") != std::string::npos);
  CHECK(fuzz.find("handleTab") != std::string::npos);
  CHECK(fuzz.find("renderFrameTo") == std::string::npos);

  std::ifstream cmakeInput(cmakePath);
  REQUIRE(cmakeInput.good());
  std::string cmake((std::istreambuf_iterator<char>(cmakeInput)), std::istreambuf_iterator<char>());
  REQUIRE(!cmake.empty());
  CHECK(cmake.find("tests/unit/test_state_machine_fuzz.cpp") != std::string::npos);

  std::ifstream todoInput(todoPath);
  REQUIRE(todoInput.good());
  std::string todo((std::istreambuf_iterator<char>(todoInput)),
                   std::istreambuf_iterator<char>());
  REQUIRE(!todo.empty());
  CHECK(todo.find("[49] Add property/fuzz testing for input and focus state machines.") !=
        std::string::npos);

  std::ifstream agentsInput(agentsPath);
  REQUIRE(agentsInput.good());
  std::string agents((std::istreambuf_iterator<char>(agentsInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!agents.empty());
  CHECK(agents.find("fuzz/property tests deterministic") != std::string::npos);
}

TEST_CASE("PrimeStage toolchain quality gates wire sanitizer and warning checks") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path cmakePath = repoRoot / "CMakeLists.txt";
  std::filesystem::path scriptPath = repoRoot / "scripts" / "compile.sh";
  std::filesystem::path workflowPath = repoRoot / ".github" / "workflows" / "presubmit.yml";
  REQUIRE(std::filesystem::exists(cmakePath));
  REQUIRE(std::filesystem::exists(scriptPath));
  REQUIRE(std::filesystem::exists(workflowPath));

  std::ifstream cmakeInput(cmakePath);
  REQUIRE(cmakeInput.good());
  std::string cmake((std::istreambuf_iterator<char>(cmakeInput)), std::istreambuf_iterator<char>());
  REQUIRE(!cmake.empty());
  CHECK(cmake.find("PRIMESTAGE_WARNINGS_AS_ERRORS") != std::string::npos);
  CHECK(cmake.find("PRIMESTAGE_ENABLE_ASAN") != std::string::npos);
  CHECK(cmake.find("PRIMESTAGE_ENABLE_UBSAN") != std::string::npos);
  CHECK(cmake.find("ps_enable_project_warnings") != std::string::npos);
  CHECK(cmake.find("ps_enable_sanitizers") != std::string::npos);
  CHECK(cmake.find("-Wall -Wextra -Wpedantic") != std::string::npos);
  CHECK(cmake.find("-fsanitize=address") != std::string::npos);
  CHECK(cmake.find("-fsanitize=undefined") != std::string::npos);

  std::ifstream scriptInput(scriptPath);
  REQUIRE(scriptInput.good());
  std::string script((std::istreambuf_iterator<char>(scriptInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!script.empty());
  CHECK(script.find("--warnings-as-errors") != std::string::npos);
  CHECK(script.find("--asan") != std::string::npos);
  CHECK(script.find("--ubsan") != std::string::npos);
  CHECK(script.find("PRIMESTAGE_WARNINGS_AS_ERRORS") != std::string::npos);
  CHECK(script.find("PRIMESTAGE_ENABLE_ASAN") != std::string::npos);
  CHECK(script.find("PRIMESTAGE_ENABLE_UBSAN") != std::string::npos);
  CHECK(script.find("build_examples=\"OFF\"") != std::string::npos);
  CHECK(script.find("PRIMESTAGE_BUILD_EXAMPLES=\"$build_examples\"") != std::string::npos);
  CHECK(script.find("CMAKE_EXPORT_COMPILE_COMMANDS=ON") != std::string::npos);
  CHECK(script.find("compile_commands.json") != std::string::npos);
  CHECK(script.find("ln -sfn") != std::string::npos);

  std::ifstream workflowInput(workflowPath);
  REQUIRE(workflowInput.good());
  std::string workflow((std::istreambuf_iterator<char>(workflowInput)),
                       std::istreambuf_iterator<char>());
  REQUIRE(!workflow.empty());
  CHECK(workflow.find("toolchain-quality") != std::string::npos);
  CHECK(workflow.find("./scripts/compile.sh --debug --warnings-as-errors") != std::string::npos);
  CHECK(workflow.find("./scripts/compile.sh --debug --asan --ubsan --test") != std::string::npos);
}

TEST_CASE("PrimeStage build artifact hygiene workflow is documented and scripted") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path gitignorePath = repoRoot / ".gitignore";
  std::filesystem::path scriptPath = repoRoot / "scripts" / "clean.sh";
  std::filesystem::path docsPath = repoRoot / "docs" / "build-artifact-hygiene.md";
  std::filesystem::path readmePath = repoRoot / "README.md";
  std::filesystem::path agentsPath = repoRoot / "AGENTS.md";
  std::filesystem::path todoPath = repoRoot / "docs" / "todo.md";
  REQUIRE(std::filesystem::exists(gitignorePath));
  REQUIRE(std::filesystem::exists(scriptPath));
  REQUIRE(std::filesystem::exists(docsPath));
  REQUIRE(std::filesystem::exists(readmePath));
  REQUIRE(std::filesystem::exists(agentsPath));
  REQUIRE(std::filesystem::exists(todoPath));

  std::ifstream gitignoreInput(gitignorePath);
  REQUIRE(gitignoreInput.good());
  std::string gitignore((std::istreambuf_iterator<char>(gitignoreInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!gitignore.empty());
  CHECK(gitignore.find("build-*/") != std::string::npos);
  CHECK(gitignore.find("build_*/") != std::string::npos);
  CHECK(gitignore.find(".cache/") != std::string::npos);
  CHECK(gitignore.find("compile_commands.json") != std::string::npos);
  CHECK(gitignore.find("*.profraw") != std::string::npos);
  CHECK(gitignore.find("*.profdata") != std::string::npos);

  std::ifstream scriptInput(scriptPath);
  REQUIRE(scriptInput.good());
  std::string script((std::istreambuf_iterator<char>(scriptInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!script.empty());
  CHECK(script.find("--dry-run") != std::string::npos);
  CHECK(script.find("--all") != std::string::npos);
  CHECK(script.find("build-*") != std::string::npos);
  CHECK(script.find("compile_commands.json") != std::string::npos);

  std::ifstream docsInput(docsPath);
  REQUIRE(docsInput.good());
  std::string docs((std::istreambuf_iterator<char>(docsInput)),
                   std::istreambuf_iterator<char>());
  REQUIRE(!docs.empty());
  CHECK(docs.find("./scripts/clean.sh --dry-run") != std::string::npos);
  CHECK(docs.find("tests/snapshots/*.snap") != std::string::npos);
  CHECK(docs.find("screenshots/*.png") != std::string::npos);

  std::ifstream readmeInput(readmePath);
  REQUIRE(readmeInput.good());
  std::string readme((std::istreambuf_iterator<char>(readmeInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!readme.empty());
  CHECK(readme.find("./scripts/clean.sh --dry-run") != std::string::npos);
  CHECK(readme.find("docs/build-artifact-hygiene.md") != std::string::npos);

  std::ifstream agentsInput(agentsPath);
  REQUIRE(agentsInput.good());
  std::string agents((std::istreambuf_iterator<char>(agentsInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!agents.empty());
  CHECK(agents.find("scripts/clean.sh") != std::string::npos);

  std::ifstream todoInput(todoPath);
  REQUIRE(todoInput.good());
  std::string todo((std::istreambuf_iterator<char>(todoInput)),
                   std::istreambuf_iterator<char>());
  REQUIRE(!todo.empty());
  CHECK(todo.find("[34] Keep generated/build artifacts out of source control workflows.") !=
        std::string::npos);
}

TEST_CASE("PrimeStage core widget ids enums and shared specs are exposed") {
  CHECK(PrimeStage::widgetIdentityId("") == PrimeStage::InvalidWidgetIdentityId);
  CHECK(PrimeStage::widgetIdentityId("demo.button") == PrimeStage::widgetIdentityId("demo.button"));
  CHECK(PrimeStage::widgetIdentityId("demo.button") != PrimeStage::widgetIdentityId("demo.slider"));
  CHECK(PrimeStage::widgetKindName(PrimeStage::WidgetKind::Button) == "button");
  CHECK(PrimeStage::widgetKindName(PrimeStage::WidgetKind::TreeView) == "tree_view");

  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeFrame::NodeId buttonId = frame.createNode();
  frame.addChild(rootId, buttonId);

  PrimeStage::WidgetIdentityReconciler identity;
  PrimeStage::WidgetIdentityId buttonIdentity = PrimeStage::widgetIdentityId("demo.button");
  identity.registerNode(buttonIdentity, buttonId);
  CHECK(identity.findNode(buttonIdentity) == buttonId);
  CHECK(identity.findNode("demo.button") == buttonId);

  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path uiPath = repoRoot / "include" / "PrimeStage" / "Ui.h";
  std::filesystem::path sourceCppPath = repoRoot / "src" / "PrimeStage.cpp";
  std::filesystem::path apiRefPath = repoRoot / "docs" / "minimal-api-reference.md";
  std::filesystem::path designPath = repoRoot / "docs" / "prime-stage-design.md";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  std::filesystem::path todoPath = repoRoot / "docs" / "todo.md";
  REQUIRE(std::filesystem::exists(uiPath));
  REQUIRE(std::filesystem::exists(sourceCppPath));
  REQUIRE(std::filesystem::exists(apiRefPath));
  REQUIRE(std::filesystem::exists(designPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));
  REQUIRE(std::filesystem::exists(todoPath));

  std::ifstream uiInput(uiPath);
  REQUIRE(uiInput.good());
  std::string ui((std::istreambuf_iterator<char>(uiInput)),
                 std::istreambuf_iterator<char>());
  REQUIRE(!ui.empty());
  CHECK(ui.find("enum class WidgetKind") != std::string::npos);
  CHECK(ui.find("using WidgetIdentityId = uint64_t;") != std::string::npos);
  CHECK(ui.find("struct WidgetSpec") != std::string::npos);
  CHECK(ui.find("struct FocusableWidgetSpec") != std::string::npos);
  CHECK(ui.find("registerNode(WidgetIdentityId identity") != std::string::npos);
  CHECK(ui.find("findNode(WidgetIdentityId identity) const") != std::string::npos);

  std::ifstream sourceInput(sourceCppPath);
  REQUIRE(sourceInput.good());
  std::string source((std::istreambuf_iterator<char>(sourceInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!source.empty());
  CHECK(source.find("WidgetIdentityReconciler::registerNode(WidgetIdentityId identity") !=
        std::string::npos);
  CHECK(source.find("WidgetIdentityReconciler::findNode(WidgetIdentityId identity) const") !=
        std::string::npos);
  CHECK(source.find("pendingFocusedIdentityId_") != std::string::npos);

  std::ifstream apiRefInput(apiRefPath);
  REQUIRE(apiRefInput.good());
  std::string apiRef((std::istreambuf_iterator<char>(apiRefInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!apiRef.empty());
  CHECK(apiRef.find("Core Ids And Shared Specs") != std::string::npos);
  CHECK(apiRef.find("WidgetKind") != std::string::npos);
  CHECK(apiRef.find("WidgetSpec") != std::string::npos);

  std::ifstream designInput(designPath);
  REQUIRE(designInput.good());
  std::string design((std::istreambuf_iterator<char>(designInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!design.empty());
  CHECK(design.find("WidgetSpec") != std::string::npos);
  CHECK(design.find("WidgetIdentityId") != std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("widgetIdentityId") != std::string::npos);

  std::ifstream todoInput(todoPath);
  REQUIRE(todoInput.good());
  std::string todo((std::istreambuf_iterator<char>(todoInput)),
                   std::istreambuf_iterator<char>());
  REQUIRE(!todo.empty());
  CHECK(todo.find("[1] Establish core ids, enums, and shared widget specs.") != std::string::npos);
}

TEST_CASE("PrimeStage spec validation guards clamp invalid indices and ranges") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path sourceFile = repoRoot / "src" / "PrimeStage.cpp";
  std::filesystem::path uiHeader = repoRoot / "include" / "PrimeStage" / "Ui.h";
  std::filesystem::path textFieldTest = repoRoot / "tests" / "unit" / "test_text_field.cpp";
  std::filesystem::path imePlan = repoRoot / "docs" / "ime-composition-plan.md";
  std::filesystem::path todoPath = repoRoot / "docs" / "todo.md";
  REQUIRE(std::filesystem::exists(sourceFile));
  REQUIRE(std::filesystem::exists(uiHeader));
  REQUIRE(std::filesystem::exists(textFieldTest));
  REQUIRE(std::filesystem::exists(imePlan));
  REQUIRE(std::filesystem::exists(todoPath));

  std::ifstream sourceInput(sourceFile);
  REQUIRE(sourceInput.good());
  std::string source((std::istreambuf_iterator<char>(sourceInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!source.empty());
  CHECK(source.find("sanitize_size_spec") != std::string::npos);
  CHECK(source.find("clamp_selected_index") != std::string::npos);
  CHECK(source.find("clamp_selected_row_or_none") != std::string::npos);
  CHECK(source.find("clamp_text_index") != std::string::npos);
  CHECK(source.find("PrimeStage validation:") != std::string::npos);
  CHECK(source.find("add_state_scrim_overlay") != std::string::npos);
  CHECK(source.find("clamp_tab_index") != std::string::npos);

  std::ifstream uiInput(uiHeader);
  REQUIRE(uiInput.good());
  std::string ui((std::istreambuf_iterator<char>(uiInput)),
                 std::istreambuf_iterator<char>());
  REQUIRE(!ui.empty());
  CHECK(ui.find("bool enabled = true;") != std::string::npos);
  CHECK(ui.find("bool readOnly = false;") != std::string::npos);
  CHECK(ui.find("int tabIndex = -1;") != std::string::npos);
  CHECK(ui.find("ToggleState* state = nullptr;") != std::string::npos);
  CHECK(ui.find("CheckboxState* state = nullptr;") != std::string::npos);
  CHECK(ui.find("TabsState* state = nullptr;") != std::string::npos);
  CHECK(ui.find("DropdownState* state = nullptr;") != std::string::npos);
  CHECK(ui.find("ProgressBarState* state = nullptr;") != std::string::npos);
  CHECK(ui.find("ProgressBarCallbacks callbacks{};") != std::string::npos);
  CHECK(ui.find("struct TextCompositionState") != std::string::npos);
  CHECK(ui.find("struct TextCompositionCallbacks") != std::string::npos);
  CHECK(ui.find("TextCompositionState* compositionState = nullptr;") != std::string::npos);
  CHECK(ui.find("TextCompositionCallbacks compositionCallbacks{};") != std::string::npos);
  CHECK(ui.find("enum class WidgetKind") != std::string::npos);
  CHECK(ui.find("using WidgetIdentityId = uint64_t;") != std::string::npos);
  CHECK(ui.find("constexpr WidgetIdentityId widgetIdentityId") != std::string::npos);
  CHECK(ui.find("struct WidgetSpec") != std::string::npos);
  CHECK(ui.find("struct EnableableWidgetSpec") != std::string::npos);
  CHECK(ui.find("struct FocusableWidgetSpec") != std::string::npos);
  CHECK(ui.find("struct ButtonSpec : FocusableWidgetSpec") != std::string::npos);
  CHECK(ui.find("struct LabelSpec : WidgetSpec") != std::string::npos);

  std::ifstream textFieldInput(textFieldTest);
  REQUIRE(textFieldInput.good());
  std::string textFieldTests((std::istreambuf_iterator<char>(textFieldInput)),
                             std::istreambuf_iterator<char>());
  REQUIRE(!textFieldTests.empty());
  CHECK(textFieldTests.find("non-ASCII text input and backspace") != std::string::npos);
  CHECK(textFieldTests.find("composition-like replacement workflows") != std::string::npos);
  CHECK(textFieldTests.find("日本語") != std::string::npos);

  std::ifstream imePlanInput(imePlan);
  REQUIRE(imePlanInput.good());
  std::string ime((std::istreambuf_iterator<char>(imePlanInput)),
                  std::istreambuf_iterator<char>());
  REQUIRE(!ime.empty());
  CHECK(ime.find("composition start/update/commit/cancel") != std::string::npos);
  CHECK(ime.find("TextCompositionCallbacks") != std::string::npos);
  CHECK(ime.find("TextCompositionState") != std::string::npos);

  std::ifstream todoInput(todoPath);
  REQUIRE(todoInput.good());
  std::string todo((std::istreambuf_iterator<char>(todoInput)),
                   std::istreambuf_iterator<char>());
  REQUIRE(!todo.empty());
  CHECK(todo.find("[37] Add API validation and diagnostics for widget specs.") != std::string::npos);
}

TEST_CASE("PrimeStage accessibility roadmap defines semantics model and behavior contract") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path uiHeader = repoRoot / "include" / "PrimeStage" / "Ui.h";
  std::filesystem::path roadmapPath = repoRoot / "docs" / "accessibility-semantics-roadmap.md";
  std::filesystem::path interactionTests = repoRoot / "tests" / "unit" / "test_interaction.cpp";
  REQUIRE(std::filesystem::exists(uiHeader));
  REQUIRE(std::filesystem::exists(roadmapPath));
  REQUIRE(std::filesystem::exists(interactionTests));

  std::ifstream uiInput(uiHeader);
  REQUIRE(uiInput.good());
  std::string ui((std::istreambuf_iterator<char>(uiInput)),
                 std::istreambuf_iterator<char>());
  REQUIRE(!ui.empty());
  CHECK(ui.find("enum class AccessibilityRole") != std::string::npos);
  CHECK(ui.find("struct AccessibilityState") != std::string::npos);
  CHECK(ui.find("struct AccessibilitySemantics") != std::string::npos);
  CHECK(ui.find("AccessibilitySemantics accessibility{};") != std::string::npos);

  std::ifstream roadmapInput(roadmapPath);
  REQUIRE(roadmapInput.good());
  std::string roadmap((std::istreambuf_iterator<char>(roadmapInput)),
                      std::istreambuf_iterator<char>());
  REQUIRE(!roadmap.empty());
  CHECK(roadmap.find("Metadata Model") != std::string::npos);
  CHECK(roadmap.find("Focus Order Contract") != std::string::npos);
  CHECK(roadmap.find("Activation Contract") != std::string::npos);
  CHECK(roadmap.find("AccessibilityRole") != std::string::npos);

  std::ifstream interactionInput(interactionTests);
  REQUIRE(interactionInput.good());
  std::string interaction((std::istreambuf_iterator<char>(interactionInput)),
                          std::istreambuf_iterator<char>());
  REQUIRE(!interaction.empty());
  CHECK(interaction.find("accessibility keyboard focus and activation contract") !=
        std::string::npos);
}

TEST_CASE("PrimeStage default behavior matrix is documented and enforced") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path matrixPath = repoRoot / "docs" / "default-widget-behavior-matrix.md";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  std::filesystem::path apiRefPath = repoRoot / "docs" / "minimal-api-reference.md";
  std::filesystem::path sourceCppPath = repoRoot / "src" / "PrimeStage.cpp";
  std::filesystem::path interactionPath = repoRoot / "tests" / "unit" / "test_interaction.cpp";
  REQUIRE(std::filesystem::exists(matrixPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));
  REQUIRE(std::filesystem::exists(apiRefPath));
  REQUIRE(std::filesystem::exists(sourceCppPath));
  REQUIRE(std::filesystem::exists(interactionPath));

  std::ifstream matrixInput(matrixPath);
  REQUIRE(matrixInput.good());
  std::string matrix((std::istreambuf_iterator<char>(matrixInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!matrix.empty());
  CHECK(matrix.find("Default Widget Behavior Matrix") != std::string::npos);
  CHECK(matrix.find("| Widget | Focusable Default | Keyboard Default | Pointer Default | Accessibility Role Default |") !=
        std::string::npos);
  CHECK(matrix.find("| `Button` |") != std::string::npos);
  CHECK(matrix.find("| `Toggle` |") != std::string::npos);
  CHECK(matrix.find("| `Checkbox` |") != std::string::npos);
  CHECK(matrix.find("| `Slider` |") != std::string::npos);
  CHECK(matrix.find("| `ProgressBar` |") != std::string::npos);
  CHECK(matrix.find("| `Tabs` |") != std::string::npos);
  CHECK(matrix.find("| `Dropdown` |") != std::string::npos);
  CHECK(matrix.find("| `Table` |") != std::string::npos);
  CHECK(matrix.find("| `List` |") != std::string::npos);
  CHECK(matrix.find("| `TreeView` |") != std::string::npos);
  CHECK(matrix.find("| `Window` |") != std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("docs/default-widget-behavior-matrix.md") != std::string::npos);

  std::ifstream apiRefInput(apiRefPath);
  REQUIRE(apiRefInput.good());
  std::string apiRef((std::istreambuf_iterator<char>(apiRefInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!apiRef.empty());
  CHECK(apiRef.find("docs/default-widget-behavior-matrix.md") != std::string::npos);

  std::ifstream sourceCppInput(sourceCppPath);
  REQUIRE(sourceCppInput.good());
  std::string sourceCpp((std::istreambuf_iterator<char>(sourceCppInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!sourceCpp.empty());
  CHECK(sourceCpp.find("void apply_default_accessibility_semantics(") != std::string::npos);
  CHECK(sourceCpp.find("void apply_default_checked_semantics(") != std::string::npos);
  CHECK(sourceCpp.find("void apply_default_range_semantics(") != std::string::npos);
  CHECK(sourceCpp.find("AccessibilityRole::Button") != std::string::npos);
  CHECK(sourceCpp.find("AccessibilityRole::TextField") != std::string::npos);
  CHECK(sourceCpp.find("AccessibilityRole::StaticText") != std::string::npos);
  CHECK(sourceCpp.find("AccessibilityRole::Toggle") != std::string::npos);
  CHECK(sourceCpp.find("AccessibilityRole::Checkbox") != std::string::npos);
  CHECK(sourceCpp.find("AccessibilityRole::Slider") != std::string::npos);
  CHECK(sourceCpp.find("AccessibilityRole::TabList") != std::string::npos);
  CHECK(sourceCpp.find("AccessibilityRole::ComboBox") != std::string::npos);
  CHECK(sourceCpp.find("AccessibilityRole::ProgressBar") != std::string::npos);
  CHECK(sourceCpp.find("AccessibilityRole::Table") != std::string::npos);
  CHECK(sourceCpp.find("AccessibilityRole::Tree") != std::string::npos);
  CHECK(sourceCpp.find("AccessibilityRole::Group") != std::string::npos);
  CHECK(sourceCpp.find("bool needsPatchState = enabled ||") != std::string::npos);
  CHECK(sourceCpp.find("LowLevel::appendNodeOnEvent(frame(),") != std::string::npos);
  CHECK(sourceCpp.find("tableRoot.nodeId()") != std::string::npos);

  std::ifstream interactionInput(interactionPath);
  REQUIRE(interactionInput.good());
  std::string interaction((std::istreambuf_iterator<char>(interactionInput)),
                          std::istreambuf_iterator<char>());
  REQUIRE(!interaction.empty());
  CHECK(interaction.find("default progress bar supports pointer and keyboard adjustments") !=
        std::string::npos);
  CHECK(interaction.find("table and list keyboard selection matches pointer selection defaults") !=
        std::string::npos);
}

TEST_CASE("PrimeStage widget-spec defaults audit is documented and mapped to minimal paths") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path auditPath = repoRoot / "docs" / "widget-spec-defaults-audit.md";
  std::filesystem::path apiRefPath = repoRoot / "docs" / "minimal-api-reference.md";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  std::filesystem::path checklistPath = repoRoot / "docs" / "widget-api-review-checklist.md";
  std::filesystem::path uiHeaderPath = repoRoot / "include" / "PrimeStage" / "Ui.h";
  std::filesystem::path builderTestsPath = repoRoot / "tests" / "unit" / "test_builder_api.cpp";
  std::filesystem::path agentsPath = repoRoot / "AGENTS.md";
  REQUIRE(std::filesystem::exists(auditPath));
  REQUIRE(std::filesystem::exists(apiRefPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));
  REQUIRE(std::filesystem::exists(checklistPath));
  REQUIRE(std::filesystem::exists(uiHeaderPath));
  REQUIRE(std::filesystem::exists(builderTestsPath));
  REQUIRE(std::filesystem::exists(agentsPath));

  std::ifstream auditInput(auditPath);
  REQUIRE(auditInput.good());
  std::string audit((std::istreambuf_iterator<char>(auditInput)),
                    std::istreambuf_iterator<char>());
  REQUIRE(!audit.empty());
  CHECK(audit.find("Widget-Spec Defaults Audit") != std::string::npos);
  CHECK(audit.find("Classification Rules") != std::string::npos);
  CHECK(audit.find("required") != std::string::npos);
  CHECK(audit.find("optional") != std::string::npos);
  CHECK(audit.find("advanced") != std::string::npos);
  CHECK(audit.find("Required fields: none.") != std::string::npos);
  CHECK(audit.find("Noisy Defaults Policy") != std::string::npos);
  CHECK(audit.find("`onClick`") != std::string::npos);
  CHECK(audit.find("`onTextChanged`") != std::string::npos);
  CHECK(audit.find("`onChanged`") != std::string::npos);
  CHECK(audit.find("`onValueChanged`") != std::string::npos);
  CHECK(audit.find("`onTabChanged`") != std::string::npos);
  CHECK(audit.find("`onOpened`") != std::string::npos);
  CHECK(audit.find("`onSelected`") != std::string::npos);
  CHECK(audit.find("`onRowClicked`") != std::string::npos);

  CHECK(audit.find("### `ButtonSpec`") != std::string::npos);
  CHECK(audit.find("### `TextFieldSpec`") != std::string::npos);
  CHECK(audit.find("### `SelectableTextSpec`") != std::string::npos);
  CHECK(audit.find("### `ToggleSpec`") != std::string::npos);
  CHECK(audit.find("### `CheckboxSpec`") != std::string::npos);
  CHECK(audit.find("### `SliderSpec`") != std::string::npos);
  CHECK(audit.find("### `TabsSpec`") != std::string::npos);
  CHECK(audit.find("### `DropdownSpec`") != std::string::npos);
  CHECK(audit.find("### `ProgressBarSpec`") != std::string::npos);
  CHECK(audit.find("### `ListSpec`") != std::string::npos);
  CHECK(audit.find("### `TableSpec`") != std::string::npos);
  CHECK(audit.find("### `TreeViewSpec`") != std::string::npos);
  CHECK(audit.find("### `ScrollViewSpec`") != std::string::npos);
  CHECK(audit.find("### `WindowSpec`") != std::string::npos);

  CHECK(audit.find("root.createButton(PrimeStage::ButtonSpec{});") != std::string::npos);
  CHECK(audit.find("root.createTextField(PrimeStage::TextFieldSpec{});") != std::string::npos);
  CHECK(audit.find("root.createSelectableText(PrimeStage::SelectableTextSpec{});") !=
        std::string::npos);
  CHECK(audit.find("root.createToggle(PrimeStage::ToggleSpec{});") != std::string::npos);
  CHECK(audit.find("root.createCheckbox(PrimeStage::CheckboxSpec{});") != std::string::npos);
  CHECK(audit.find("root.createSlider(PrimeStage::SliderSpec{});") != std::string::npos);
  CHECK(audit.find("root.createTabs(PrimeStage::TabsSpec{});") != std::string::npos);
  CHECK(audit.find("root.createDropdown(PrimeStage::DropdownSpec{});") != std::string::npos);
  CHECK(audit.find("root.createProgressBar(PrimeStage::ProgressBarSpec{});") !=
        std::string::npos);
  CHECK(audit.find("root.createList(PrimeStage::ListSpec{});") != std::string::npos);
  CHECK(audit.find("root.createTable(PrimeStage::TableSpec{});") != std::string::npos);
  CHECK(audit.find("root.createTreeView(PrimeStage::TreeViewSpec{});") != std::string::npos);
  CHECK(audit.find("root.createScrollView(PrimeStage::ScrollViewSpec{});") != std::string::npos);
  CHECK(audit.find("root.createWindow(PrimeStage::WindowSpec{});") != std::string::npos);

  std::ifstream apiRefInput(apiRefPath);
  REQUIRE(apiRefInput.good());
  std::string apiRef((std::istreambuf_iterator<char>(apiRefInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!apiRef.empty());
  CHECK(apiRef.find("docs/widget-spec-defaults-audit.md") != std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("docs/widget-spec-defaults-audit.md") != std::string::npos);
  CHECK(guidelines.find("[76]") == std::string::npos);

  std::ifstream checklistInput(checklistPath);
  REQUIRE(checklistInput.good());
  std::string checklist((std::istreambuf_iterator<char>(checklistInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!checklist.empty());
  CHECK(checklist.find("docs/widget-spec-defaults-audit.md") != std::string::npos);

  std::ifstream uiInput(uiHeaderPath);
  REQUIRE(uiInput.good());
  std::string ui((std::istreambuf_iterator<char>(uiInput)), std::istreambuf_iterator<char>());
  REQUIRE(!ui.empty());
  CHECK(ui.find("struct ButtonSpec : FocusableWidgetSpec {") != std::string::npos);
  CHECK(ui.find("struct TextFieldSpec : FocusableWidgetSpec {") != std::string::npos);
  CHECK(ui.find("struct SelectableTextSpec : EnableableWidgetSpec {") != std::string::npos);
  CHECK(ui.find("struct ToggleSpec : FocusableWidgetSpec {") != std::string::npos);
  CHECK(ui.find("struct CheckboxSpec : FocusableWidgetSpec {") != std::string::npos);
  CHECK(ui.find("struct SliderSpec : FocusableWidgetSpec {") != std::string::npos);
  CHECK(ui.find("struct TabsSpec : FocusableWidgetSpec {") != std::string::npos);
  CHECK(ui.find("struct DropdownSpec : FocusableWidgetSpec {") != std::string::npos);
  CHECK(ui.find("struct ProgressBarSpec : FocusableWidgetSpec {") != std::string::npos);
  CHECK(ui.find("struct ListSpec : FocusableWidgetSpec {") != std::string::npos);
  CHECK(ui.find("struct TableSpec : FocusableWidgetSpec {") != std::string::npos);
  CHECK(ui.find("struct TreeViewSpec : FocusableWidgetSpec {") != std::string::npos);
  CHECK(ui.find("struct ScrollViewSpec {") != std::string::npos);
  CHECK(ui.find("struct WindowSpec {") != std::string::npos);

  std::ifstream builderInput(builderTestsPath);
  REQUIRE(builderInput.good());
  std::string builder((std::istreambuf_iterator<char>(builderInput)),
                      std::istreambuf_iterator<char>());
  REQUIRE(!builder.empty());
  CHECK(builder.find("builder API materializes default widget fallbacks") != std::string::npos);
  CHECK(builder.find("PrimeStage::ButtonSpec buttonSpec;") != std::string::npos);
  CHECK(builder.find("PrimeStage::WindowSpec windowSpec;") != std::string::npos);

  std::ifstream agentsInput(agentsPath);
  REQUIRE(agentsInput.good());
  std::string agents((std::istreambuf_iterator<char>(agentsInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!agents.empty());
  CHECK(agents.find("docs/widget-spec-defaults-audit.md") != std::string::npos);
  CHECK(agents.find("templated ergonomic entry points (`bind(...)`, `makeListModel(...)`, `makeTableModel(...)`, `makeTreeModel(...)`)") !=
        std::string::npos);
}

TEST_CASE("PrimeStage owned text widget defaults are documented and enforced") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path uiHeaderPath = repoRoot / "include" / "PrimeStage" / "Ui.h";
  std::filesystem::path sourceCppPath = repoRoot / "src" / "PrimeStage.cpp";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  std::filesystem::path apiRefPath = repoRoot / "docs" / "minimal-api-reference.md";
  std::filesystem::path designPath = repoRoot / "docs" / "prime-stage-design.md";
  std::filesystem::path widgetsExamplePath = repoRoot / "examples" / "advanced" / "primestage_widgets.cpp";
  REQUIRE(std::filesystem::exists(uiHeaderPath));
  REQUIRE(std::filesystem::exists(sourceCppPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));
  REQUIRE(std::filesystem::exists(apiRefPath));
  REQUIRE(std::filesystem::exists(designPath));
  REQUIRE(std::filesystem::exists(widgetsExamplePath));

  std::ifstream uiInput(uiHeaderPath);
  REQUIRE(uiInput.good());
  std::string ui((std::istreambuf_iterator<char>(uiInput)), std::istreambuf_iterator<char>());
  REQUIRE(!ui.empty());
  CHECK(ui.find("std::shared_ptr<TextFieldState> ownedState{};") != std::string::npos);
  CHECK(ui.find("std::shared_ptr<SelectableTextState> ownedState{};") != std::string::npos);

  std::ifstream sourceInput(sourceCppPath);
  REQUIRE(sourceInput.good());
  std::string source((std::istreambuf_iterator<char>(sourceInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!source.empty());
  CHECK(source.find("text_field_state_is_pristine") != std::string::npos);
  CHECK(source.find("seed_text_field_state_from_spec") != std::string::npos);
  CHECK(source.find("std::shared_ptr<TextFieldState> stateOwner") != std::string::npos);
  CHECK(source.find("std::shared_ptr<SelectableTextState> stateOwner") != std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("Owned-default mode (text widgets)") != std::string::npos);
  CHECK(guidelines.find("spec.ownedState") != std::string::npos);

  std::ifstream apiRefInput(apiRefPath);
  REQUIRE(apiRefInput.good());
  std::string apiRef((std::istreambuf_iterator<char>(apiRefInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!apiRef.empty());
  CHECK(apiRef.find("TextFieldSpec::ownedState") != std::string::npos);
  CHECK(apiRef.find("SelectableTextSpec::ownedState") != std::string::npos);

  std::ifstream designInput(designPath);
  REQUIRE(designInput.good());
  std::string design((std::istreambuf_iterator<char>(designInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!design.empty());
  CHECK(design.find("Owned-default (text widgets)") != std::string::npos);

  std::ifstream exampleInput(widgetsExamplePath);
  REQUIRE(exampleInput.good());
  std::string example((std::istreambuf_iterator<char>(exampleInput)),
                      std::istreambuf_iterator<char>());
  REQUIRE(!example.empty());
  CHECK(example.find("field.state = &app.state.textField") == std::string::npos);
  CHECK(example.find("selectable.state = &app.state.selectableText") == std::string::npos);
}

TEST_CASE("PrimeStage LowLevel appendNodeOnEvent composes without clobbering existing callback") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId nodeId = frame.createNode();
  frame.addRoot(nodeId);
  PrimeFrame::Node* node = frame.getNode(nodeId);
  REQUIRE(node != nullptr);

  int previousCalls = 0;
  PrimeFrame::Callback base;
  base.onEvent = [&](PrimeFrame::Event const& event) -> bool {
    previousCalls += 1;
    return event.key == 42;
  };
  node->callbacks = frame.addCallback(std::move(base));

  int appendedCalls = 0;
  bool appended = PrimeStage::LowLevel::appendNodeOnEvent(
      frame,
      nodeId,
      [&](PrimeFrame::Event const& event) -> bool {
        appendedCalls += 1;
        return event.key == 7;
      });
  CHECK(appended);

  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  PrimeFrame::Event handledByNew;
  handledByNew.type = PrimeFrame::EventType::KeyDown;
  handledByNew.key = 7;
  CHECK(callback->onEvent(handledByNew));
  CHECK(appendedCalls == 1);
  CHECK(previousCalls == 0);

  PrimeFrame::Event handledByPrevious;
  handledByPrevious.type = PrimeFrame::EventType::KeyDown;
  handledByPrevious.key = 42;
  CHECK(callback->onEvent(handledByPrevious));
  CHECK(appendedCalls == 2);
  CHECK(previousCalls == 1);
}

TEST_CASE("PrimeStage LowLevel NodeCallbackHandle installs callbacks and restores previous table") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId nodeId = frame.createNode();
  frame.addRoot(nodeId);
  PrimeFrame::Node* node = frame.getNode(nodeId);
  REQUIRE(node != nullptr);

  int previousEventCalls = 0;
  int previousFocusCalls = 0;
  PrimeFrame::Callback previous;
  previous.onEvent = [&](PrimeFrame::Event const&) -> bool {
    previousEventCalls += 1;
    return false;
  };
  previous.onFocus = [&]() { previousFocusCalls += 1; };
  node->callbacks = frame.addCallback(std::move(previous));
  PrimeFrame::CallbackId previousId = node->callbacks;

  int handleEventCalls = 0;
  int handleFocusCalls = 0;
  {
    PrimeStage::LowLevel::NodeCallbackTable table;
    table.onEvent = [&](PrimeFrame::Event const&) -> bool {
      handleEventCalls += 1;
      return true;
    };
    table.onFocus = [&]() { handleFocusCalls += 1; };

    PrimeStage::LowLevel::NodeCallbackHandle handle(frame, nodeId, std::move(table));
    CHECK(handle.active());
    CHECK(node->callbacks != previousId);

    PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
    REQUIRE(callback != nullptr);
    REQUIRE(callback->onEvent);
    REQUIRE(callback->onFocus);

    PrimeFrame::Event event;
    event.type = PrimeFrame::EventType::KeyDown;
    event.key = 55;
    CHECK(callback->onEvent(event));
    callback->onFocus();
  }

  CHECK(node->callbacks == previousId);
  CHECK(handleEventCalls == 1);
  CHECK(handleFocusCalls == 1);

  PrimeFrame::Callback const* restored = frame.getCallback(node->callbacks);
  REQUIRE(restored != nullptr);
  REQUIRE(restored->onEvent);
  REQUIRE(restored->onFocus);

  PrimeFrame::Event after;
  after.type = PrimeFrame::EventType::KeyDown;
  after.key = 56;
  CHECK_FALSE(restored->onEvent(after));
  restored->onFocus();
  CHECK(previousEventCalls == 1);
  CHECK(previousFocusCalls == 1);
}

TEST_CASE("PrimeStage LowLevel NodeCallbackHandle move and reset tolerate node destruction") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeFrame::NodeId childId = frame.createNode();
  REQUIRE(frame.addChild(rootId, childId));

  PrimeStage::LowLevel::NodeCallbackTable table;
  table.onEvent = [](PrimeFrame::Event const&) -> bool { return true; };

  PrimeStage::LowLevel::NodeCallbackHandle first;
  CHECK(first.bind(frame, childId, std::move(table)));
  CHECK(first.active());

  PrimeStage::LowLevel::NodeCallbackHandle second(std::move(first));
  CHECK_FALSE(first.active());
  CHECK(second.active());

  REQUIRE(frame.destroyNode(childId));
  second.reset();
  CHECK_FALSE(second.active());
}

TEST_CASE("PrimeStage LowLevel appendNodeOnFocus and appendNodeOnBlur compose callbacks") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId nodeId = frame.createNode();
  frame.addRoot(nodeId);
  PrimeFrame::Node* node = frame.getNode(nodeId);
  REQUIRE(node != nullptr);

  int previousFocus = 0;
  int previousBlur = 0;
  PrimeFrame::Callback base;
  base.onFocus = [&]() { previousFocus += 1; };
  base.onBlur = [&]() { previousBlur += 1; };
  node->callbacks = frame.addCallback(std::move(base));

  int appendedFocus = 0;
  int appendedBlur = 0;
  CHECK(PrimeStage::LowLevel::appendNodeOnFocus(frame, nodeId, [&]() { appendedFocus += 1; }));
  CHECK(PrimeStage::LowLevel::appendNodeOnBlur(frame, nodeId, [&]() { appendedBlur += 1; }));

  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onFocus);
  REQUIRE(callback->onBlur);

  callback->onFocus();
  callback->onBlur();

  CHECK(previousFocus == 1);
  CHECK(previousBlur == 1);
  CHECK(appendedFocus == 1);
  CHECK(appendedBlur == 1);
}

TEST_CASE("PrimeStage LowLevel appendNodeOnEvent suppresses direct reentrant recursion") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId nodeId = frame.createNode();
  frame.addRoot(nodeId);
  PrimeFrame::Node* node = frame.getNode(nodeId);
  REQUIRE(node != nullptr);

  int handlerCalls = 0;
  bool nestedHandled = true;
  CHECK(PrimeStage::LowLevel::appendNodeOnEvent(
      frame,
      nodeId,
      [&](PrimeFrame::Event const& event) -> bool {
        handlerCalls += 1;
        if (handlerCalls == 1) {
          PrimeFrame::Node const* currentNode = frame.getNode(nodeId);
          REQUIRE(currentNode != nullptr);
          PrimeFrame::Callback const* callback = frame.getCallback(currentNode->callbacks);
          REQUIRE(callback != nullptr);
          REQUIRE(callback->onEvent);
          nestedHandled = callback->onEvent(event);
        }
        return true;
      }));

  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  PrimeFrame::Event trigger;
  trigger.type = PrimeFrame::EventType::KeyDown;
  trigger.key = 11;
  CHECK(callback->onEvent(trigger));
  CHECK(handlerCalls == 1);
  CHECK_FALSE(nestedHandled);
}

TEST_CASE("PrimeStage LowLevel appendNodeOnFocus and appendNodeOnBlur suppress direct reentrancy") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId nodeId = frame.createNode();
  frame.addRoot(nodeId);
  PrimeFrame::Node* node = frame.getNode(nodeId);
  REQUIRE(node != nullptr);

  PrimeFrame::Callback base;
  int previousFocus = 0;
  int previousBlur = 0;
  base.onFocus = [&]() { previousFocus += 1; };
  base.onBlur = [&]() { previousBlur += 1; };
  node->callbacks = frame.addCallback(std::move(base));

  int appendedFocus = 0;
  int appendedBlur = 0;
  CHECK(PrimeStage::LowLevel::appendNodeOnFocus(frame, nodeId, [&]() {
    appendedFocus += 1;
    if (appendedFocus == 1) {
      PrimeFrame::Node const* currentNode = frame.getNode(nodeId);
      REQUIRE(currentNode != nullptr);
      PrimeFrame::Callback const* callback = frame.getCallback(currentNode->callbacks);
      REQUIRE(callback != nullptr);
      REQUIRE(callback->onFocus);
      callback->onFocus();
    }
  }));
  CHECK(PrimeStage::LowLevel::appendNodeOnBlur(frame, nodeId, [&]() {
    appendedBlur += 1;
    if (appendedBlur == 1) {
      PrimeFrame::Node const* currentNode = frame.getNode(nodeId);
      REQUIRE(currentNode != nullptr);
      PrimeFrame::Callback const* callback = frame.getCallback(currentNode->callbacks);
      REQUIRE(callback != nullptr);
      REQUIRE(callback->onBlur);
      callback->onBlur();
    }
  }));

  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onFocus);
  REQUIRE(callback->onBlur);

  callback->onFocus();
  callback->onBlur();

  CHECK(previousFocus == 1);
  CHECK(previousBlur == 1);
  CHECK(appendedFocus == 1);
  CHECK(appendedBlur == 1);
}
