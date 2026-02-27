#include "PrimeStage/Ui.h"

// Advanced PrimeFrame integration (documented exception): this sample intentionally demonstrates
// direct PrimeFrame frame + layout hosting APIs.
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include <iostream>
#include <string_view>
#include <vector>

namespace {

constexpr float SceneWidth = 960.0f;
constexpr float SceneHeight = 640.0f;

struct SceneState {
  PrimeStage::TextFieldState nameField{};
  PrimeStage::ToggleState notifications{};
  PrimeStage::CheckboxState autoSave{};
};

PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  if (PrimeFrame::Node* root = frame.getNode(rootId)) {
    root->layout = PrimeFrame::LayoutType::Overlay;
    root->sizeHint.width.preferred = SceneWidth;
    root->sizeHint.height.preferred = SceneHeight;
  }
  return PrimeStage::UiNode(frame, rootId, true);
}

PrimeStage::Window buildWindowScene(PrimeStage::UiNode root, SceneState& state) {
  PrimeStage::WindowSpec windowSpec;
  windowSpec.title = "PrimeStage Scene";
  windowSpec.width = 760.0f;
  windowSpec.height = 520.0f;
  windowSpec.contentPadding = 14.0f;

  PrimeStage::StackSpec contentStack;
  contentStack.gap = 10.0f;
  contentStack.size.stretchX = 1.0f;

  PrimeStage::LabelSpec heading;
  heading.text = "Window + Widgets Scene";
  heading.size.preferredHeight = 26.0f;

  PrimeStage::SizeSpec fieldSize;
  fieldSize.preferredWidth = 280.0f;
  fieldSize.preferredHeight = 28.0f;

  PrimeStage::SizeSpec toggleSize;
  toggleSize.preferredWidth = 56.0f;
  toggleSize.preferredHeight = 26.0f;

  PrimeStage::SizeSpec checkboxSize;
  checkboxSize.preferredWidth = 220.0f;
  checkboxSize.preferredHeight = 26.0f;

  PrimeStage::SizeSpec sliderSize;
  sliderSize.preferredWidth = 280.0f;
  sliderSize.preferredHeight = 24.0f;

  PrimeStage::SizeSpec buttonSize;
  buttonSize.preferredWidth = 180.0f;
  buttonSize.preferredHeight = 28.0f;

  PrimeStage::SizeSpec listSize;
  listSize.preferredWidth = 280.0f;
  listSize.preferredHeight = 120.0f;

  PrimeStage::SizeSpec treeSize;
  treeSize.preferredWidth = 280.0f;
  treeSize.preferredHeight = 160.0f;

  PrimeStage::SizeSpec scrollSize;
  scrollSize.preferredWidth = 300.0f;
  scrollSize.preferredHeight = 160.0f;

  PrimeStage::ListSpec listSpec;
  listSpec.items = {"Low", "Medium", "High"};
  listSpec.selectedIndex = 1;

  std::vector<PrimeStage::TreeNode> treeNodes;
  treeNodes.push_back(PrimeStage::TreeNode{"Assets",
                                           {PrimeStage::TreeNode{"Textures", {}, true, false},
                                            PrimeStage::TreeNode{"Audio", {}, true, false}},
                                           true,
                                           false});

  return root.createWindow(windowSpec, [&](PrimeStage::Window& window) {
    window.content.createVerticalStack(contentStack, [&](PrimeStage::UiNode& column) {
      column.createLabel(heading);
      column.createButton("Apply", 0u, 0u, buttonSize);
      column.createTextField(state.nameField, "Project Name", 0u, 0u, fieldSize);
      column.createToggle(state.notifications.on, 0u, 0u, toggleSize);
      column.createCheckbox("Auto Save", state.autoSave.checked, 0u, 0u, 0u, checkboxSize);
      column.createSlider(0.35f, false, 0u, 0u, 0u, sliderSize);
      column.createList(listSpec);
      column.createTreeView(std::move(treeNodes), treeSize);
      PrimeStage::ScrollView view = column.createScrollView(scrollSize, true, false);
      PrimeStage::LabelSpec note;
      note.text = "Scrollable content host";
      note.size.preferredHeight = 22.0f;
      view.content.createLabel(note);
    });
  });
}

} // namespace

int main() {
  // Advanced PrimeFrame integration (documented exception): this sample intentionally demonstrates
  // direct PrimeFrame frame + layout hosting APIs.
  PrimeFrame::Frame frame;
  SceneState state;
  state.nameField.text = "PrimeScene";
  state.nameField.cursor = static_cast<uint32_t>(state.nameField.text.size());
  state.notifications.on = true;
  state.autoSave.checked = true;

  PrimeStage::UiNode root = createRoot(frame);
  PrimeStage::Window sceneWindow = buildWindowScene(root, state);

  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = SceneWidth;
  options.rootHeight = SceneHeight;
  engine.layout(frame, layout, options);

  bool hasLayout = layout.get(sceneWindow.root.nodeId()) != nullptr;
  std::cout << "Built scene with " << frame.nodeCount() << " nodes; window layout "
            << (hasLayout ? "ok" : "missing") << "\n";
  return hasLayout ? 0 : 1;
}
