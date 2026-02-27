#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

namespace {

PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  if (PrimeFrame::Node* node = frame.getNode(rootId)) {
    node->layout = PrimeFrame::LayoutType::Overlay;
    node->sizeHint.width.preferred = 640.0f;
    node->sizeHint.height.preferred = 360.0f;
  }
  return PrimeStage::UiNode(frame, rootId, true);
}

bool hasChild(PrimeFrame::Frame const& frame,
              PrimeFrame::NodeId parentId,
              PrimeFrame::NodeId childId) {
  PrimeFrame::Node const* parent = frame.getNode(parentId);
  if (!parent) {
    return false;
  }
  for (PrimeFrame::NodeId const candidate : parent->children) {
    if (candidate == childId) {
      return true;
    }
  }
  return false;
}

PrimeFrame::LayoutOutput layoutFrame(PrimeFrame::Frame& frame) {
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 640.0f;
  options.rootHeight = 360.0f;
  engine.layout(frame, layout, options);
  return layout;
}

} // namespace

TEST_CASE("PrimeStage builder API supports nested fluent composition") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::StackSpec stackSpec;
  stackSpec.size.preferredWidth = 260.0f;
  stackSpec.size.preferredHeight = 140.0f;
  stackSpec.gap = 4.0f;

  PrimeStage::PanelSpec panelSpec;
  panelSpec.layout = PrimeFrame::LayoutType::Overlay;
  panelSpec.size.preferredWidth = 200.0f;
  panelSpec.size.preferredHeight = 60.0f;

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Build";
  buttonSpec.size.preferredWidth = 120.0f;
  buttonSpec.size.preferredHeight = 28.0f;

  int stackCalls = 0;
  int panelCalls = 0;
  int buttonCalls = 0;
  int withCalls = 0;
  PrimeFrame::NodeId stackId{};
  PrimeFrame::NodeId panelId{};
  PrimeFrame::NodeId buttonId{};
  PrimeFrame::NodeId buttonReturnId{};
  PrimeFrame::NodeId withReturnId{};

  root.createVerticalStack(stackSpec, [&](PrimeStage::UiNode& stack) {
    ++stackCalls;
    stackId = stack.nodeId();
    stack.createPanel(panelSpec, [&](PrimeStage::UiNode& panel) {
      ++panelCalls;
      panelId = panel.nodeId();
      PrimeStage::UiNode button = panel.createButton(buttonSpec, [&](PrimeStage::UiNode& built) {
        ++buttonCalls;
        buttonId = built.nodeId();
        built.setVisible(false);
      });
      buttonReturnId = button.nodeId();
      PrimeStage::UiNode chained = button.with([&](PrimeStage::UiNode& node) {
        ++withCalls;
        node.setHitTestVisible(false);
      });
      withReturnId = chained.nodeId();
    });
  });

  CHECK(stackCalls == 1);
  CHECK(panelCalls == 1);
  CHECK(buttonCalls == 1);
  CHECK(withCalls == 1);
  CHECK(buttonId == buttonReturnId);
  CHECK(buttonId == withReturnId);
  CHECK(hasChild(frame, root.nodeId(), stackId));
  CHECK(hasChild(frame, stackId, panelId));
  CHECK(hasChild(frame, panelId, buttonId));

  PrimeFrame::Node const* buttonNode = frame.getNode(buttonId);
  REQUIRE(buttonNode != nullptr);
  CHECK_FALSE(buttonNode->visible);
  CHECK_FALSE(buttonNode->hitTestVisible);
}

TEST_CASE("PrimeStage builder API materializes default widget fallbacks") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::ButtonSpec buttonSpec;
  PrimeStage::UiNode button = root.createButton(buttonSpec);
  CHECK(button.nodeId() != root.nodeId());
  CHECK(frame.getNode(button.nodeId()) != nullptr);

  PrimeStage::TextFieldSpec fieldSpec;
  PrimeStage::UiNode field = root.createTextField(fieldSpec);
  CHECK(field.nodeId() != root.nodeId());
  CHECK(frame.getNode(field.nodeId()) != nullptr);

  PrimeStage::SelectableTextSpec selectableSpec;
  PrimeStage::UiNode selectable = root.createSelectableText(selectableSpec);
  CHECK(selectable.nodeId() != root.nodeId());
  CHECK(frame.getNode(selectable.nodeId()) != nullptr);

  PrimeStage::ToggleSpec toggleSpec;
  PrimeStage::UiNode toggle = root.createToggle(toggleSpec);
  CHECK(toggle.nodeId() != root.nodeId());
  CHECK(frame.getNode(toggle.nodeId()) != nullptr);

  PrimeStage::CheckboxSpec checkboxSpec;
  PrimeStage::UiNode checkbox = root.createCheckbox(checkboxSpec);
  CHECK(checkbox.nodeId() != root.nodeId());
  CHECK(frame.getNode(checkbox.nodeId()) != nullptr);

  PrimeStage::SliderSpec sliderSpec;
  PrimeStage::UiNode slider = root.createSlider(sliderSpec);
  CHECK(slider.nodeId() != root.nodeId());
  CHECK(frame.getNode(slider.nodeId()) != nullptr);

  PrimeStage::ProgressBarSpec progressSpec;
  PrimeStage::UiNode progress = root.createProgressBar(progressSpec);
  CHECK(progress.nodeId() != root.nodeId());
  CHECK(frame.getNode(progress.nodeId()) != nullptr);

  PrimeStage::TabsSpec tabsSpec;
  PrimeStage::UiNode tabs = root.createTabs(tabsSpec);
  CHECK(tabs.nodeId() != root.nodeId());
  CHECK(frame.getNode(tabs.nodeId()) != nullptr);

  PrimeStage::DropdownSpec dropdownSpec;
  PrimeStage::UiNode dropdown = root.createDropdown(dropdownSpec);
  CHECK(dropdown.nodeId() != root.nodeId());
  CHECK(frame.getNode(dropdown.nodeId()) != nullptr);

  PrimeStage::ListSpec listSpec;
  PrimeStage::UiNode list = root.createList(listSpec);
  CHECK(list.nodeId() != root.nodeId());
  CHECK(frame.getNode(list.nodeId()) != nullptr);

  PrimeStage::TableSpec tableSpec;
  PrimeStage::UiNode table = root.createTable(tableSpec);
  CHECK(table.nodeId() != root.nodeId());
  CHECK(frame.getNode(table.nodeId()) != nullptr);

  PrimeStage::TreeViewSpec treeSpec;
  PrimeStage::UiNode tree = root.createTreeView(treeSpec);
  CHECK(tree.nodeId() != root.nodeId());
  CHECK(frame.getNode(tree.nodeId()) != nullptr);

  PrimeStage::ScrollViewSpec scrollSpec;
  PrimeStage::ScrollView scrollView = root.createScrollView(scrollSpec);
  CHECK(scrollView.root.nodeId() != root.nodeId());
  CHECK(scrollView.content.nodeId().isValid());
  CHECK(frame.getNode(scrollView.root.nodeId()) != nullptr);
  CHECK(frame.getNode(scrollView.content.nodeId()) != nullptr);

  PrimeStage::WindowSpec windowSpec;
  PrimeStage::Window window = root.createWindow(windowSpec);
  CHECK(window.root.nodeId() != root.nodeId());
  CHECK(window.content.nodeId().isValid());
  CHECK(frame.getNode(window.root.nodeId()) != nullptr);
  CHECK(frame.getNode(window.content.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage declarative helpers support nested composition ergonomics") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  int clickCount = 0;
  PrimeFrame::NodeId columnId{};
  PrimeFrame::NodeId rowId{};
  PrimeFrame::NodeId buttonId{};
  PrimeFrame::NodeId spacerId{};
  PrimeFrame::NodeId windowContentId{};

  root.column([&](PrimeStage::UiNode& column) {
    columnId = column.nodeId();
    column.label("Declarative");
    rowId = column.row([&](PrimeStage::UiNode& row) {
      buttonId = row.button("Apply", [&]() { clickCount += 1; }).nodeId();
      spacerId = row.spacer(-8.0f).nodeId();
    }).nodeId();

    PrimeStage::WindowSpec windowSpec;
    windowSpec.title = "Panel";
    windowSpec.width = 220.0f;
    windowSpec.height = 140.0f;
    column.window(windowSpec, [&](PrimeStage::Window& window) {
      windowContentId = window.content.label("Window content").nodeId();
    });
  });

  CHECK(hasChild(frame, root.nodeId(), columnId));
  CHECK(hasChild(frame, columnId, rowId));
  CHECK(hasChild(frame, rowId, buttonId));
  CHECK(frame.getNode(windowContentId) != nullptr);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::LayoutOut const* buttonOut = layout.get(buttonId);
  REQUIRE(buttonOut != nullptr);
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = buttonOut->absX + buttonOut->absW * 0.5f;
  down.y = buttonOut->absY + buttonOut->absH * 0.5f;
  PrimeFrame::Event up = down;
  up.type = PrimeFrame::EventType::PointerUp;

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  router.dispatch(down, frame, layout, &focus);
  router.dispatch(up, frame, layout, &focus);
  CHECK(clickCount == 1);

  // Diagnostic path: declarative helper invalid spacer height should clamp safely.
  PrimeFrame::LayoutOut const* spacerOut = layout.get(spacerId);
  REQUIRE(spacerOut != nullptr);
  CHECK(spacerOut->absH >= 0.0f);
}

TEST_CASE("PrimeStage declarative value helpers bind common widgets") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::State<bool> toggleState{false};
  PrimeStage::State<bool> checkboxState{false};
  PrimeStage::State<float> sliderState{0.2f};
  PrimeStage::State<int> tabsState{2};
  PrimeStage::State<int> dropdownState{0};
  PrimeStage::State<float> progressState{0.1f};

  PrimeFrame::NodeId toggleId{};
  PrimeFrame::NodeId checkboxId{};
  PrimeFrame::NodeId tabsId{};
  PrimeFrame::NodeId dropdownId{};
  PrimeFrame::NodeId sliderId{};
  PrimeFrame::NodeId progressId{};

  root.column([&](PrimeStage::UiNode& column) {
    toggleId = column.toggle(PrimeStage::bind(toggleState)).nodeId();
    checkboxId = column.checkbox("Enabled", PrimeStage::bind(checkboxState)).nodeId();
    tabsId = column.tabs({"One", "Two", "Three"}, PrimeStage::bind(tabsState)).nodeId();
    dropdownId =
        column.dropdown({"Preview", "Edit", "Export"}, PrimeStage::bind(dropdownState)).nodeId();
    sliderId = column.slider(PrimeStage::bind(sliderState)).nodeId();
    progressId = column.progressBar(PrimeStage::bind(progressState)).nodeId();
  });

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  auto dispatchPointer = [&](PrimeFrame::EventType type,
                             PrimeFrame::NodeId target,
                             float xRatio,
                             int pointerId = 1) {
    PrimeFrame::LayoutOut const* out = layout.get(target);
    REQUIRE(out != nullptr);
    PrimeFrame::Event event;
    event.type = type;
    event.pointerId = pointerId;
    event.x = out->absX + out->absW * xRatio;
    event.y = out->absY + out->absH * 0.5f;
    router.dispatch(event, frame, layout, &focus);
  };

  auto clickNode = [&](PrimeFrame::NodeId target, int pointerId = 1) {
    dispatchPointer(PrimeFrame::EventType::PointerDown, target, 0.5f, pointerId);
    dispatchPointer(PrimeFrame::EventType::PointerUp, target, 0.5f, pointerId);
  };

  clickNode(toggleId, 1);
  clickNode(checkboxId, 2);
  clickNode(dropdownId, 3);

  PrimeFrame::Node const* tabsNode = frame.getNode(tabsId);
  REQUIRE(tabsNode != nullptr);
  REQUIRE(!tabsNode->children.empty());
  clickNode(tabsNode->children.front(), 4);

  dispatchPointer(PrimeFrame::EventType::PointerDown, sliderId, 0.05f, 5);
  dispatchPointer(PrimeFrame::EventType::PointerDrag, sliderId, 0.9f, 5);
  dispatchPointer(PrimeFrame::EventType::PointerUp, sliderId, 0.9f, 5);

  dispatchPointer(PrimeFrame::EventType::PointerDown, progressId, 0.05f, 6);
  dispatchPointer(PrimeFrame::EventType::PointerDrag, progressId, 0.9f, 6);
  dispatchPointer(PrimeFrame::EventType::PointerUp, progressId, 0.9f, 6);

  CHECK(toggleState.value);
  CHECK(checkboxState.value);
  CHECK(dropdownState.value == 1);
  CHECK(tabsState.value == 0);
  CHECK(sliderState.value > 0.8f);
  CHECK(progressState.value > 0.8f);
}

TEST_CASE("PrimeStage declarative tabs/dropdown helpers clamp empty choices") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::State<int> tabsState{5};
  PrimeStage::State<int> dropdownState{-4};

  PrimeStage::UiNode tabs = root.tabs({}, PrimeStage::bind(tabsState));
  PrimeStage::UiNode dropdown = root.dropdown({}, PrimeStage::bind(dropdownState));

  CHECK(frame.getNode(tabs.nodeId()) != nullptr);
  CHECK(frame.getNode(dropdown.nodeId()) != nullptr);
  CHECK(tabsState.value == 0);
  CHECK(dropdownState.value == 0);
}

TEST_CASE("PrimeStage form helpers compose label control help and validation") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TextFieldState nameState;
  nameState.text = "PrimeStage";
  nameState.cursor = static_cast<uint32_t>(nameState.text.size());
  PrimeStage::State<bool> notifications{false};

  PrimeFrame::NodeId formId{};
  PrimeFrame::NodeId nameFieldId{};
  PrimeFrame::NodeId nameControlId{};
  PrimeFrame::NodeId notificationsFieldId{};
  PrimeFrame::NodeId notificationsControlId{};

  root.form([&](PrimeStage::UiNode& form) {
    formId = form.nodeId();

    PrimeStage::FormFieldSpec nameField;
    nameField.label = "Display name";
    nameField.helpText = "Used for project labels.";
    nameField.invalid = true;
    nameField.errorText = "Display name cannot be empty.";
    nameFieldId = form.formField(nameField, [&](PrimeStage::UiNode& field) {
      PrimeStage::TextFieldSpec spec;
      spec.state = &nameState;
      nameControlId = field.createTextField(spec).nodeId();
    }).nodeId();

    notificationsFieldId = form.formField(
        "Notifications",
        [&](PrimeStage::UiNode& field) {
          notificationsControlId = field.toggle(PrimeStage::bind(notifications)).nodeId();
        },
        "Enable badge updates.").nodeId();
  });

  CHECK(frame.getNode(formId) != nullptr);
  CHECK(frame.getNode(nameFieldId) != nullptr);
  CHECK(frame.getNode(notificationsFieldId) != nullptr);
  CHECK(hasChild(frame, formId, nameFieldId));
  CHECK(hasChild(frame, formId, notificationsFieldId));
  CHECK(hasChild(frame, nameFieldId, nameControlId));
  CHECK(hasChild(frame, notificationsFieldId, notificationsControlId));

  PrimeFrame::Node const* nameFieldNode = frame.getNode(nameFieldId);
  REQUIRE(nameFieldNode != nullptr);
  CHECK(nameFieldNode->children.size() == 4u);

  PrimeFrame::Node const* notificationsFieldNode = frame.getNode(notificationsFieldId);
  REQUIRE(notificationsFieldNode != nullptr);
  CHECK(notificationsFieldNode->children.size() == 3u);
}

TEST_CASE("PrimeStage form helpers clamp invalid spacing values") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::FormSpec formSpec;
  formSpec.rowGap = -12.0f;
  PrimeFrame::NodeId formId{};
  PrimeFrame::NodeId fieldId{};

  root.form(formSpec, [&](PrimeStage::UiNode& form) {
    formId = form.nodeId();
    PrimeStage::FormFieldSpec fieldSpec;
    fieldSpec.label = "Name";
    fieldSpec.gap = -6.0f;
    fieldId = form.formField(fieldSpec, [&](PrimeStage::UiNode& field) {
      field.textLine("Control");
    }).nodeId();
  });

  PrimeFrame::Node const* formNode = frame.getNode(formId);
  REQUIRE(formNode != nullptr);
  CHECK(formNode->gap == doctest::Approx(0.0f));

  PrimeFrame::Node const* fieldNode = frame.getNode(fieldId);
  REQUIRE(fieldNode != nullptr);
  CHECK(fieldNode->gap == doctest::Approx(0.0f));
}
