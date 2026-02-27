#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace {

constexpr float RootWidth = 320.0f;
constexpr float RootHeight = 180.0f;

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

} // namespace

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
  buttonSpec.callbacks.onClick = [&]() { clickCount += 1; };

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

TEST_CASE("PrimeStage text field editing is owned by app state") {
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
  fieldSpec.callbacks.onTextChanged = [&](std::string_view text) { lastText = std::string(text); };

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

TEST_CASE("PrimeStage text field without state is non-editable by default") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.text = "Preview";
  fieldSpec.size.preferredWidth = 180.0f;
  fieldSpec.size.preferredHeight = 24.0f;

  PrimeStage::UiNode field = root.createTextField(fieldSpec);
  PrimeFrame::Node const* node = frame.getNode(field.nodeId());
  REQUIRE(node != nullptr);
  CHECK_FALSE(node->focusable);
  CHECK(node->callbacks == PrimeFrame::InvalidCallbackId);

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
  CHECK_FALSE(focus.focusedNode().isValid());
}

TEST_CASE("PrimeStage widgets example uses widget callbacks without PrimeFrame callback plumbing") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path examplePath = repoRoot / "examples" / "primestage_widgets.cpp";
  REQUIRE(std::filesystem::exists(examplePath));

  std::ifstream input(examplePath);
  REQUIRE(input.good());
  std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  REQUIRE(!source.empty());

  CHECK(source.find("tabsSpec.callbacks.onTabChanged") != std::string::npos);
  CHECK(source.find("dropdownSpec.callbacks.onOpened") != std::string::npos);
  CHECK(source.find("dropdownSpec.callbacks.onSelected") != std::string::npos);
  CHECK(source.find("widgetIdentity.registerNode") != std::string::npos);
  CHECK(source.find("widgetIdentity.restoreFocus") != std::string::npos);
  CHECK(source.find("bridgeHostInputEvent") != std::string::npos);
  CHECK(source.find("HostKey::Escape") != std::string::npos);
  CHECK(source.find("scrollDirectionSign") != std::string::npos);
  CHECK(source.find("PrimeStage::FrameLifecycle") != std::string::npos);
  CHECK(source.find("runRebuildIfNeeded(app)") != std::string::npos);
  CHECK(source.find("app.runtime.requestRebuild()") != std::string::npos);
  CHECK(source.find("app.runtime.framePending()") != std::string::npos);

  // App-level widget usage should not rely on raw PrimeFrame callback mutation.
  CHECK(source.find("appendNodeEventCallback") == std::string::npos);
  CHECK(source.find("node->callbacks =") == std::string::npos);
  CHECK(source.find("PrimeFrame::Callback callback") == std::string::npos);
  CHECK(source.find("RestoreFocusTarget") == std::string::npos);
  CHECK(source.find("std::get_if<PrimeHost::PointerEvent>") == std::string::npos);
  CHECK(source.find("std::get_if<PrimeHost::KeyEvent>") == std::string::npos);
  CHECK(source.find("KeyEscape") == std::string::npos);
  CHECK(source.find("0x28") == std::string::npos);
  CHECK(source.find("0x50") == std::string::npos);
  CHECK(source.find("needsRebuild") == std::string::npos);
  CHECK(source.find("needsLayout") == std::string::npos);
  CHECK(source.find("needsFrame") == std::string::npos);
}

TEST_CASE("PrimeStage input bridge exposes normalized key and scroll semantics") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path inputBridgePath = repoRoot / "include" / "PrimeStage" / "InputBridge.h";
  std::filesystem::path guidelinesPath = repoRoot / "docs" / "api-ergonomics-guidelines.md";
  REQUIRE(std::filesystem::exists(inputBridgePath));
  REQUIRE(std::filesystem::exists(guidelinesPath));

  std::ifstream inputBridgeInput(inputBridgePath);
  REQUIRE(inputBridgeInput.good());
  std::string inputBridge((std::istreambuf_iterator<char>(inputBridgeInput)),
                          std::istreambuf_iterator<char>());
  REQUIRE(!inputBridge.empty());
  CHECK(inputBridge.find("using HostKey = KeyCode;") != std::string::npos);
  CHECK(inputBridge.find("scrollLinePixels") != std::string::npos);
  CHECK(inputBridge.find("scrollDirectionSign") != std::string::npos);
  CHECK(inputBridge.find("scroll->deltaY * deltaScale * directionSign") != std::string::npos);

  std::ifstream guidelinesInput(guidelinesPath);
  REQUIRE(guidelinesInput.good());
  std::string guidelines((std::istreambuf_iterator<char>(guidelinesInput)),
                         std::istreambuf_iterator<char>());
  REQUIRE(!guidelines.empty());
  CHECK(guidelines.find("KeyCode") != std::string::npos);
  CHECK(guidelines.find("scrollLinePixels") != std::string::npos);
  CHECK(guidelines.find("scrollDirectionSign") != std::string::npos);
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

TEST_CASE("PrimeStage README and design docs match shipped workflow and API names") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path readmePath = repoRoot / "README.md";
  std::filesystem::path designPath = repoRoot / "docs" / "prime-stage-design.md";
  std::filesystem::path policyPath = repoRoot / "docs" / "api-evolution-policy.md";
  REQUIRE(std::filesystem::exists(readmePath));
  REQUIRE(std::filesystem::exists(designPath));
  REQUIRE(std::filesystem::exists(policyPath));

  std::ifstream readmeInput(readmePath);
  REQUIRE(readmeInput.good());
  std::string readme((std::istreambuf_iterator<char>(readmeInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!readme.empty());
  CHECK(readme.find("./scripts/compile.sh") != std::string::npos);
  CHECK(readme.find("./scripts/compile.sh --test") != std::string::npos);
  CHECK(readme.find("docs/api-evolution-policy.md") != std::string::npos);
  CHECK(readme.find("cmake --install build-release --prefix") != std::string::npos);
  CHECK(readme.find("find_package(PrimeStage CONFIG REQUIRED)") != std::string::npos);
  CHECK(readme.find("docs/cmake-packaging.md") != std::string::npos);
  CHECK(readme.find("docs/callback-reentrancy-threading.md") != std::string::npos);

  std::ifstream designInput(designPath);
  REQUIRE(designInput.good());
  std::string design((std::istreambuf_iterator<char>(designInput)),
                     std::istreambuf_iterator<char>());
  REQUIRE(!design.empty());
  CHECK(design.find("createTextField(TextFieldSpec const& spec)") != std::string::npos);
  CHECK(design.find("createEditBox") == std::string::npos);
  CHECK(design.find("ButtonCallbacks const& callbacks") == std::string::npos);
  CHECK(design.find("## Focus Behavior (Current)") != std::string::npos);
  CHECK(design.find("Focusable by default") != std::string::npos);
  CHECK(design.find("docs/api-evolution-policy.md") != std::string::npos);
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
  std::filesystem::path uiHeaderPath = repoRoot / "include" / "PrimeStage" / "Ui.h";
  std::filesystem::path sourceCppPath = repoRoot / "src" / "PrimeStage.cpp";
  REQUIRE(std::filesystem::exists(callbackDocPath));
  REQUIRE(std::filesystem::exists(guidelinesPath));
  REQUIRE(std::filesystem::exists(designPath));
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
  CHECK(callbackDoc.find("appendNodeOnEvent") != std::string::npos);
  CHECK(callbackDoc.find("appendNodeOnFocus") != std::string::npos);
  CHECK(callbackDoc.find("appendNodeOnBlur") != std::string::npos);

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

  std::ifstream uiHeaderInput(uiHeaderPath);
  REQUIRE(uiHeaderInput.good());
  std::string uiHeader((std::istreambuf_iterator<char>(uiHeaderInput)),
                       std::istreambuf_iterator<char>());
  REQUIRE(!uiHeader.empty());
  CHECK(uiHeader.find("Direct reentrant invocation of the same") != std::string::npos);

  std::ifstream sourceCppInput(sourceCppPath);
  REQUIRE(sourceCppInput.good());
  std::string sourceCpp((std::istreambuf_iterator<char>(sourceCppInput)),
                        std::istreambuf_iterator<char>());
  REQUIRE(!sourceCpp.empty());
  CHECK(sourceCpp.find("CallbackReentryScope") != std::string::npos);
  CHECK(sourceCpp.find("reentrant %s invocation suppressed") != std::string::npos);
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
  CHECK(workflow.find("./scripts/compile.sh --${{ matrix.build_type }} --test") != std::string::npos);

  CHECK(workflow.find("PRIMESTAGE_HEADLESS_ONLY=ON") != std::string::npos);
  CHECK(workflow.find("PRIMESTAGE_ENABLE_PRIMEMANIFEST=OFF") != std::string::npos);

  CHECK(workflow.find("--test-case=\"*focus*,*interaction*\"") != std::string::npos);
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

  std::ifstream workflowInput(workflowPath);
  REQUIRE(workflowInput.good());
  std::string workflow((std::istreambuf_iterator<char>(workflowInput)),
                       std::istreambuf_iterator<char>());
  REQUIRE(!workflow.empty());
  CHECK(workflow.find("toolchain-quality") != std::string::npos);
  CHECK(workflow.find("./scripts/compile.sh --debug --warnings-as-errors") != std::string::npos);
  CHECK(workflow.find("./scripts/compile.sh --debug --asan --ubsan --test") != std::string::npos);
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
  CHECK(ui.find("struct TextCompositionState") != std::string::npos);
  CHECK(ui.find("struct TextCompositionCallbacks") != std::string::npos);
  CHECK(ui.find("TextCompositionState* compositionState = nullptr;") != std::string::npos);
  CHECK(ui.find("TextCompositionCallbacks compositionCallbacks{};") != std::string::npos);

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

TEST_CASE("PrimeStage appendNodeOnEvent composes without clobbering existing callback") {
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
  bool appended = PrimeStage::appendNodeOnEvent(
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

TEST_CASE("PrimeStage appendNodeOnFocus and appendNodeOnBlur compose callbacks") {
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
  CHECK(PrimeStage::appendNodeOnFocus(frame, nodeId, [&]() { appendedFocus += 1; }));
  CHECK(PrimeStage::appendNodeOnBlur(frame, nodeId, [&]() { appendedBlur += 1; }));

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

TEST_CASE("PrimeStage appendNodeOnEvent suppresses direct reentrant recursion") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId nodeId = frame.createNode();
  frame.addRoot(nodeId);
  PrimeFrame::Node* node = frame.getNode(nodeId);
  REQUIRE(node != nullptr);

  int handlerCalls = 0;
  bool nestedHandled = true;
  CHECK(PrimeStage::appendNodeOnEvent(
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

TEST_CASE("PrimeStage appendNodeOnFocus and appendNodeOnBlur suppress direct reentrancy") {
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
  CHECK(PrimeStage::appendNodeOnFocus(frame, nodeId, [&]() {
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
  CHECK(PrimeStage::appendNodeOnBlur(frame, nodeId, [&]() {
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
