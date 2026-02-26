#include "PrimeStage/Render.h"
#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using SteadyClock = std::chrono::steady_clock;

constexpr float DashboardRootWidth = 1280.0f;
constexpr float DashboardRootHeight = 720.0f;
constexpr float TreeRootWidth = 1100.0f;
constexpr float TreeRootHeight = 760.0f;

constexpr PrimeFrame::ColorToken ColorBackground = 1u;
constexpr PrimeFrame::ColorToken ColorSurface = 2u;
constexpr PrimeFrame::ColorToken ColorAccent = 3u;
constexpr PrimeFrame::ColorToken ColorFocus = 4u;
constexpr PrimeFrame::ColorToken ColorText = 5u;

constexpr PrimeFrame::RectStyleToken StyleBackground = 1u;
constexpr PrimeFrame::RectStyleToken StyleSurface = 2u;
constexpr PrimeFrame::RectStyleToken StyleAccent = 3u;
constexpr PrimeFrame::RectStyleToken StyleFocus = 4u;

constexpr int KeyBackspace = 0x2A;

volatile uint64_t PerfSink = 0u;

struct BenchmarkOptions {
  size_t warmupIterations = 16u;
  size_t benchmarkIterations = 96u;
  std::string budgetFile;
  std::string outputFile;
  bool checkBudgets = false;
};

struct MetricResult {
  std::string name;
  double meanUs = 0.0;
  double p95Us = 0.0;
  double maxUs = 0.0;
  size_t samples = 0u;
};

PrimeFrame::Color makeColor(float r, float g, float b) {
  PrimeFrame::Color color;
  color.r = r;
  color.g = g;
  color.b = b;
  color.a = 1.0f;
  return color;
}

void configureTheme(PrimeFrame::Frame& frame) {
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return;
  }

  theme->palette.assign(8u, PrimeFrame::Color{});
  theme->palette[ColorBackground] = makeColor(0.10f, 0.12f, 0.16f);
  theme->palette[ColorSurface] = makeColor(0.18f, 0.22f, 0.29f);
  theme->palette[ColorAccent] = makeColor(0.24f, 0.68f, 0.94f);
  theme->palette[ColorFocus] = makeColor(0.90f, 0.28f, 0.12f);
  theme->palette[ColorText] = makeColor(0.95f, 0.96f, 0.98f);

  theme->rectStyles.assign(8u, PrimeFrame::RectStyle{});
  theme->rectStyles[StyleBackground].fill = ColorBackground;
  theme->rectStyles[StyleSurface].fill = ColorSurface;
  theme->rectStyles[StyleAccent].fill = ColorAccent;
  theme->rectStyles[StyleFocus].fill = ColorFocus;

  theme->textStyles.assign(1u, PrimeFrame::TextStyle{});
  theme->textStyles[0].color = ColorText;
}

PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame, float width, float height) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeFrame::Node* root = frame.getNode(rootId);
  if (root) {
    root->layout = PrimeFrame::LayoutType::Overlay;
    root->sizeHint.width.preferred = width;
    root->sizeHint.height.preferred = height;
  }
  return PrimeStage::UiNode(frame, rootId, true);
}

PrimeFrame::LayoutOutput layoutFrame(PrimeFrame::Frame& frame, float width, float height) {
  PrimeFrame::LayoutOutput output;
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = width;
  options.rootHeight = height;
  engine.layout(frame, output, options);
  return output;
}

std::vector<std::vector<std::string_view>> const& benchmarkTableRows() {
  static std::vector<std::vector<std::string_view>> rows = [] {
    std::vector<std::vector<std::string_view>> value;
    value.reserve(180u);
    for (int index = 0; index < 180; ++index) {
      if ((index % 3) == 0) {
        value.push_back({"Pending", "Asset", "Normal", "Design"});
      } else if ((index % 3) == 1) {
        value.push_back({"Active", "Widget", "High", "Runtime"});
      } else {
        value.push_back({"Done", "Layout", "Low", "Platform"});
      }
    }
    return value;
  }();
  return rows;
}

std::vector<PrimeStage::TreeNode> makeBenchmarkTreeNodes(int sections,
                                                          int itemsPerSection,
                                                          int leavesPerItem) {
  std::vector<PrimeStage::TreeNode> roots;
  roots.reserve(static_cast<size_t>(sections));
  for (int section = 0; section < sections; ++section) {
    PrimeStage::TreeNode sectionNode;
    sectionNode.label = "Section";
    sectionNode.expanded = true;
    sectionNode.selected = false;

    sectionNode.children.reserve(static_cast<size_t>(itemsPerSection));
    for (int item = 0; item < itemsPerSection; ++item) {
      PrimeStage::TreeNode itemNode;
      itemNode.label = "Item";
      itemNode.expanded = true;
      itemNode.selected = false;

      itemNode.children.reserve(static_cast<size_t>(leavesPerItem));
      for (int leaf = 0; leaf < leavesPerItem; ++leaf) {
        (void)leaf;
        PrimeStage::TreeNode leafNode;
        leafNode.label = "Leaf";
        leafNode.expanded = false;
        leafNode.selected = false;
        itemNode.children.push_back(std::move(leafNode));
      }
      sectionNode.children.push_back(std::move(itemNode));
    }
    roots.push_back(std::move(sectionNode));
  }
  return roots;
}

std::vector<PrimeStage::TreeNode> const& benchmarkDashboardTreeNodes() {
  static std::vector<PrimeStage::TreeNode> nodes = makeBenchmarkTreeNodes(16, 6, 2);
  return nodes;
}

std::vector<PrimeStage::TreeNode> const& benchmarkHeavyTreeNodes() {
  static std::vector<PrimeStage::TreeNode> nodes = makeBenchmarkTreeNodes(32, 10, 4);
  return nodes;
}

PrimeFrame::Event makePointerEvent(PrimeFrame::EventType type, int pointerId, float x, float y) {
  PrimeFrame::Event event;
  event.type = type;
  event.pointerId = pointerId;
  event.x = x;
  event.y = y;
  return event;
}

PrimeFrame::Event makePointerScrollEvent(float x, float y, float scrollY) {
  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::PointerScroll;
  event.x = x;
  event.y = y;
  event.scrollY = scrollY;
  return event;
}

PrimeFrame::Event makeTextInputEvent(std::string_view text) {
  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::TextInput;
  event.text = std::string(text);
  return event;
}

PrimeFrame::Event makeKeyDownEvent(int key) {
  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::KeyDown;
  event.key = key;
  return event;
}

bool parseSizeValue(std::string const& valueText, size_t& outValue) {
  if (valueText.empty()) {
    return false;
  }
  try {
    size_t parsed = static_cast<size_t>(std::stoul(valueText));
    if (parsed == 0u) {
      return false;
    }
    outValue = parsed;
    return true;
  } catch (...) {
    return false;
  }
}

std::string trim(std::string value) {
  auto isSpace = [](unsigned char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
  };
  while (!value.empty() && isSpace(static_cast<unsigned char>(value.front()))) {
    value.erase(value.begin());
  }
  while (!value.empty() && isSpace(static_cast<unsigned char>(value.back()))) {
    value.pop_back();
  }
  return value;
}

void printUsage(char const* programName) {
  std::cout << "Usage: " << programName
            << " [--warmup N] [--iterations N] [--budget-file PATH]"
            << " [--check-budgets] [--output PATH]\n";
}

std::optional<BenchmarkOptions> parseOptions(int argc, char** argv) {
  BenchmarkOptions options;
  for (int index = 1; index < argc; ++index) {
    std::string arg = argv[index];
    if (arg == "--help" || arg == "-h") {
      printUsage(argv[0]);
      return std::nullopt;
    }
    if (arg == "--check-budgets") {
      options.checkBudgets = true;
      continue;
    }
    if (arg == "--warmup" || arg == "--iterations" || arg == "--budget-file" ||
        arg == "--output") {
      if ((index + 1) >= argc) {
        std::cerr << "Missing value for " << arg << "\n";
        return std::nullopt;
      }
      std::string value = argv[++index];
      if (arg == "--warmup") {
        size_t parsed = 0u;
        if (!parseSizeValue(value, parsed)) {
          std::cerr << "Invalid --warmup value: " << value << "\n";
          return std::nullopt;
        }
        options.warmupIterations = parsed;
      } else if (arg == "--iterations") {
        size_t parsed = 0u;
        if (!parseSizeValue(value, parsed)) {
          std::cerr << "Invalid --iterations value: " << value << "\n";
          return std::nullopt;
        }
        options.benchmarkIterations = parsed;
      } else if (arg == "--budget-file") {
        options.budgetFile = value;
      } else if (arg == "--output") {
        options.outputFile = value;
      }
      continue;
    }

    std::cerr << "Unknown argument: " << arg << "\n";
    return std::nullopt;
  }

  if (options.checkBudgets && options.budgetFile.empty()) {
    std::cerr << "--check-budgets requires --budget-file PATH\n";
    return std::nullopt;
  }

  return options;
}

template <typename Fn>
std::optional<MetricResult> runMetric(std::string name,
                                      size_t warmupIterations,
                                      size_t benchmarkIterations,
                                      Fn&& fn,
                                      std::string& error) {
  for (size_t warmup = 0u; warmup < warmupIterations; ++warmup) {
    if (!fn()) {
      error = "Warmup failed for metric " + name;
      return std::nullopt;
    }
  }

  std::vector<double> samples;
  samples.reserve(benchmarkIterations);
  for (size_t iteration = 0u; iteration < benchmarkIterations; ++iteration) {
    SteadyClock::time_point start = SteadyClock::now();
    if (!fn()) {
      error = "Benchmark iteration failed for metric " + name;
      return std::nullopt;
    }
    SteadyClock::time_point end = SteadyClock::now();
    std::chrono::duration<double, std::micro> elapsed = end - start;
    samples.push_back(elapsed.count());
  }

  if (samples.empty()) {
    error = "No benchmark samples were collected for metric " + name;
    return std::nullopt;
  }

  double total = 0.0;
  double maximum = 0.0;
  for (double sample : samples) {
    total += sample;
    maximum = std::max(maximum, sample);
  }

  std::vector<double> sorted = samples;
  std::sort(sorted.begin(), sorted.end());
  double percentileIndex = std::ceil(0.95 * static_cast<double>(sorted.size())) - 1.0;
  size_t p95Index = static_cast<size_t>(std::max(0.0, percentileIndex));
  p95Index = std::min(p95Index, sorted.size() - 1u);

  MetricResult result;
  result.name = std::move(name);
  result.meanUs = total / static_cast<double>(samples.size());
  result.p95Us = sorted[p95Index];
  result.maxUs = maximum;
  result.samples = samples.size();
  return result;
}

void printMetrics(std::vector<MetricResult> const& metrics,
                  size_t warmupIterations,
                  size_t benchmarkIterations) {
  std::cout << "PrimeStage benchmarks"
            << " (warmup=" << warmupIterations
            << ", iterations=" << benchmarkIterations << ")\n";
  for (MetricResult const& metric : metrics) {
    std::cout << std::left << std::setw(42) << metric.name
              << " mean_us=" << std::fixed << std::setprecision(2) << metric.meanUs
              << " p95_us=" << std::fixed << std::setprecision(2) << metric.p95Us
              << " max_us=" << std::fixed << std::setprecision(2) << metric.maxUs << "\n";
  }
}

bool writeMetricsJson(std::string const& outputPath,
                      std::vector<MetricResult> const& metrics,
                      BenchmarkOptions const& options) {
  std::filesystem::path path(outputPath);
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }

  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output.good()) {
    return false;
  }

  output << "{\n";
  output << "  \"generator\": \"PrimeStage_benchmarks\",\n";
  output << "  \"warmupIterations\": " << options.warmupIterations << ",\n";
  output << "  \"benchmarkIterations\": " << options.benchmarkIterations << ",\n";
  output << "  \"metrics\": [\n";

  for (size_t index = 0; index < metrics.size(); ++index) {
    MetricResult const& metric = metrics[index];
    output << "    {\"name\":\"" << metric.name << "\","
           << "\"meanUs\":" << std::fixed << std::setprecision(3) << metric.meanUs << ","
           << "\"p95Us\":" << std::fixed << std::setprecision(3) << metric.p95Us << ","
           << "\"maxUs\":" << std::fixed << std::setprecision(3) << metric.maxUs << ","
           << "\"samples\":" << metric.samples << "}";
    if ((index + 1u) < metrics.size()) {
      output << ',';
    }
    output << "\n";
  }

  output << "  ]\n";
  output << "}\n";
  return output.good();
}

std::optional<std::unordered_map<std::string, double>> loadBudgets(std::string const& budgetPath,
                                                                    std::string& error) {
  std::ifstream input(budgetPath);
  if (!input.good()) {
    error = "Failed to read budget file: " + budgetPath;
    return std::nullopt;
  }

  std::unordered_map<std::string, double> budgets;
  std::string line;
  int lineNumber = 0;
  while (std::getline(input, line)) {
    ++lineNumber;
    std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed.front() == '#') {
      continue;
    }

    std::istringstream stream(trimmed);
    std::string metric;
    double p95BudgetUs = 0.0;
    if (!(stream >> metric >> p95BudgetUs)) {
      error = "Malformed budget entry at line " + std::to_string(lineNumber);
      return std::nullopt;
    }

    std::string trailing;
    if (stream >> trailing) {
      error = "Unexpected trailing content at line " + std::to_string(lineNumber);
      return std::nullopt;
    }

    if (!(p95BudgetUs > 0.0) || !std::isfinite(p95BudgetUs)) {
      error = "Invalid budget value at line " + std::to_string(lineNumber);
      return std::nullopt;
    }

    budgets[metric] = p95BudgetUs;
  }

  if (budgets.empty()) {
    error = "Budget file contains no entries: " + budgetPath;
    return std::nullopt;
  }

  return budgets;
}

bool enforceBudgets(std::vector<MetricResult> const& metrics,
                    std::unordered_map<std::string, double> const& budgets) {
  bool hasFailure = false;
  for (auto const& [metricName, budgetValue] : budgets) {
    auto it = std::find_if(metrics.begin(),
                           metrics.end(),
                           [&](MetricResult const& metric) { return metric.name == metricName; });
    if (it == metrics.end()) {
      std::cerr << "Missing benchmark metric for budget entry: " << metricName << "\n";
      hasFailure = true;
      continue;
    }

    if (it->p95Us > budgetValue) {
      std::cerr << "Budget exceeded for " << metricName << ": p95=" << it->p95Us
                << "us budget=" << budgetValue << "us\n";
      hasFailure = true;
    }
  }

  return !hasFailure;
}

struct DashboardRuntime {
  PrimeFrame::Frame frame;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  PrimeStage::TextFieldState textFieldState{};
  PrimeStage::DropdownState dropdownState{};
  float sliderValue = 0.42f;
  float progressValue = 0.42f;
  bool needsRebuild = true;

  PrimeFrame::NodeId textFieldNode{};
  PrimeFrame::NodeId sliderNode{};

  void initializeState() {
    textFieldState.text = "Benchmark";
    textFieldState.cursor = static_cast<uint32_t>(textFieldState.text.size());
    dropdownState.selectedIndex = 1;
    sliderValue = 0.42f;
    progressValue = 0.42f;
    needsRebuild = true;
    textFieldNode = {};
    sliderNode = {};
  }

  void buildFrame(bool wireCallbacks) {
    frame = PrimeFrame::Frame();
    configureTheme(frame);

    PrimeStage::UiNode root = createRoot(frame, DashboardRootWidth, DashboardRootHeight);

    PrimeStage::PanelSpec background;
    background.size.stretchX = 1.0f;
    background.size.stretchY = 1.0f;
    background.rectStyle = StyleBackground;
    PrimeStage::UiNode backgroundNode = root.createPanel(background);
    backgroundNode.setHitTestVisible(false);

    PrimeStage::StackSpec shell;
    shell.size.stretchX = 1.0f;
    shell.size.stretchY = 1.0f;
    shell.padding.left = 18.0f;
    shell.padding.top = 18.0f;
    shell.padding.right = 18.0f;
    shell.padding.bottom = 18.0f;
    shell.gap = 12.0f;
    PrimeStage::UiNode content = root.createVerticalStack(shell);

    PrimeStage::TabsSpec tabs;
    tabs.labels = {"Overview", "Assets", "Settings", "Metrics"};
    tabs.selectedIndex = 0;
    tabs.tabStyle = StyleSurface;
    tabs.activeTabStyle = StyleAccent;
    tabs.textStyle = 0u;
    tabs.activeTextStyle = 0u;
    tabs.size.preferredWidth = 520.0f;
    tabs.size.preferredHeight = 32.0f;
    content.createTabs(tabs);

    PrimeStage::PanelSpec panel;
    panel.size.stretchX = 1.0f;
    panel.size.stretchY = 1.0f;
    panel.layout = PrimeFrame::LayoutType::VerticalStack;
    panel.padding.left = 12.0f;
    panel.padding.top = 12.0f;
    panel.padding.right = 12.0f;
    panel.padding.bottom = 12.0f;
    panel.gap = 10.0f;
    panel.rectStyle = StyleSurface;
    PrimeStage::UiNode page = content.createPanel(panel);

    PrimeStage::StackSpec controlsRow;
    controlsRow.gap = 10.0f;
    controlsRow.size.preferredHeight = 34.0f;
    PrimeStage::UiNode controls = page.createHorizontalStack(controlsRow);

    PrimeStage::TextFieldSpec field;
    field.state = &textFieldState;
    field.backgroundStyle = StyleSurface;
    field.focusStyle = StyleFocus;
    field.selectionStyle = StyleAccent;
    field.textStyle = 0u;
    field.placeholderStyle = 0u;
    field.cursorStyle = StyleAccent;
    field.size.preferredWidth = 300.0f;
    field.size.preferredHeight = 30.0f;
    if (wireCallbacks) {
      field.callbacks.onStateChanged = [this]() { needsRebuild = true; };
      field.callbacks.onTextChanged = [this](std::string_view) { needsRebuild = true; };
    }
    PrimeStage::UiNode fieldNode = controls.createTextField(field);
    textFieldNode = fieldNode.nodeId();

    PrimeStage::DropdownSpec dropdown;
    dropdown.state = &dropdownState;
    dropdown.options = {"Preview", "Edit", "Export", "Archive"};
    dropdown.backgroundStyle = StyleSurface;
    dropdown.textStyle = 0u;
    dropdown.indicatorStyle = 0u;
    dropdown.focusStyle = StyleFocus;
    dropdown.size.preferredWidth = 180.0f;
    dropdown.size.preferredHeight = 30.0f;
    if (wireCallbacks) {
      dropdown.callbacks.onSelected = [this](int nextIndex) {
        dropdownState.selectedIndex = nextIndex;
        needsRebuild = true;
      };
    }
    controls.createDropdown(dropdown);

    PrimeStage::SliderSpec slider;
    slider.value = sliderValue;
    slider.trackStyle = StyleBackground;
    slider.fillStyle = StyleAccent;
    slider.thumbStyle = StyleAccent;
    slider.focusStyle = StyleFocus;
    slider.size.preferredWidth = 260.0f;
    slider.size.preferredHeight = 18.0f;
    if (wireCallbacks) {
      slider.callbacks.onValueChanged = [this](float nextValue) {
        sliderValue = nextValue;
        progressValue = nextValue;
        needsRebuild = true;
      };
    }
    PrimeStage::UiNode sliderUi = controls.createSlider(slider);
    sliderNode = sliderUi.nodeId();

    PrimeStage::ProgressBarSpec progress;
    progress.value = progressValue;
    progress.trackStyle = StyleBackground;
    progress.fillStyle = StyleAccent;
    progress.focusStyle = StyleFocus;
    progress.size.preferredWidth = 180.0f;
    progress.size.preferredHeight = 14.0f;
    controls.createProgressBar(progress);

    PrimeStage::TableSpec table;
    table.size.stretchX = 1.0f;
    table.size.preferredHeight = 360.0f;
    table.headerStyle = StyleBackground;
    table.rowStyle = StyleSurface;
    table.rowAltStyle = StyleBackground;
    table.selectionStyle = StyleAccent;
    table.dividerStyle = StyleBackground;
    table.focusStyle = StyleFocus;
    table.columns = {{"State", 120.0f, 0u, 0u},
                     {"Name", 220.0f, 0u, 0u},
                     {"Priority", 120.0f, 0u, 0u},
                     {"Area", 140.0f, 0u, 0u}};
    table.rows = benchmarkTableRows();
    table.selectedRow = 8;
    page.createTable(table);

    PrimeStage::TreeViewSpec tree;
    tree.size.stretchX = 1.0f;
    tree.size.stretchY = 1.0f;
    tree.size.minHeight = 180.0f;
    tree.rowStyle = StyleSurface;
    tree.rowAltStyle = StyleBackground;
    tree.hoverStyle = StyleAccent;
    tree.selectionStyle = StyleAccent;
    tree.selectionAccentStyle = StyleAccent;
    tree.caretBackgroundStyle = StyleSurface;
    tree.caretLineStyle = StyleAccent;
    tree.connectorStyle = StyleBackground;
    tree.focusStyle = StyleFocus;
    tree.textStyle = 0u;
    tree.selectedTextStyle = 0u;
    tree.scrollBar.enabled = true;
    tree.scrollBar.autoThumb = true;
    tree.scrollBar.width = 7.0f;
    tree.scrollBar.padding = 6.0f;
    tree.scrollBar.trackStyle = StyleBackground;
    tree.scrollBar.thumbStyle = StyleAccent;
    tree.nodes = benchmarkDashboardTreeNodes();
    page.createTreeView(tree);
  }

  void runLayoutPass() {
    layout = layoutFrame(frame, DashboardRootWidth, DashboardRootHeight);
    focus.updateAfterRebuild(frame, layout);
  }

  void rebuild(bool wireCallbacks) {
    buildFrame(wireCallbacks);
    runLayoutPass();
    needsRebuild = false;
  }

  bool runTypingInteraction() {
    if (!textFieldNode.isValid()) {
      return false;
    }

    if (textFieldState.text.size() > 40u) {
      textFieldState.text = "Benchmark";
      textFieldState.cursor = static_cast<uint32_t>(textFieldState.text.size());
      needsRebuild = true;
    }

    if (focus.focusedNode() != textFieldNode) {
      focus.setFocus(frame, layout, textFieldNode);
    }

    router.dispatch(makeTextInputEvent("x"), frame, layout, &focus);
    if (textFieldState.text.size() > 44u) {
      router.dispatch(makeKeyDownEvent(KeyBackspace), frame, layout, &focus);
    }
    if (needsRebuild) {
      rebuild(true);
    }

    PerfSink += static_cast<uint64_t>(textFieldState.text.size());
    return true;
  }

  bool runSliderDragInteraction() {
    if (!sliderNode.isValid()) {
      return false;
    }

    PrimeFrame::LayoutOut const* sliderOut = layout.get(sliderNode);
    if (!sliderOut) {
      return false;
    }

    float y = sliderOut->absY + sliderOut->absH * 0.5f;
    float startX = sliderOut->absX + sliderOut->absW * 0.24f;
    float endX = sliderOut->absX + sliderOut->absW * 0.82f;
    router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 2, startX, y),
                    frame,
                    layout,
                    &focus);
    router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDrag, 2, endX, y),
                    frame,
                    layout,
                    &focus);
    router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 2, endX, y),
                    frame,
                    layout,
                    &focus);

    if (needsRebuild) {
      rebuild(true);
    }

    PerfSink += static_cast<uint64_t>(sliderValue * 1000.0f);
    return true;
  }
};

struct TreeRuntime {
  PrimeFrame::Frame frame;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  PrimeFrame::NodeId treeNode{};
  bool needsRebuild = true;
  PrimeStage::TreeViewScrollInfo lastScroll{};
  int scrollEvents = 0;

  void buildFrame(bool wireCallbacks) {
    frame = PrimeFrame::Frame();
    configureTheme(frame);

    PrimeStage::UiNode root = createRoot(frame, TreeRootWidth, TreeRootHeight);

    PrimeStage::PanelSpec background;
    background.size.stretchX = 1.0f;
    background.size.stretchY = 1.0f;
    background.rectStyle = StyleBackground;
    PrimeStage::UiNode bg = root.createPanel(background);
    bg.setHitTestVisible(false);

    PrimeStage::StackSpec shell;
    shell.size.stretchX = 1.0f;
    shell.size.stretchY = 1.0f;
    shell.padding.left = 16.0f;
    shell.padding.top = 16.0f;
    shell.padding.right = 16.0f;
    shell.padding.bottom = 16.0f;
    shell.gap = 10.0f;

    PrimeStage::UiNode page = root.createVerticalStack(shell);

    PrimeStage::TreeViewSpec tree;
    tree.nodes = benchmarkHeavyTreeNodes();
    tree.rowStyle = StyleSurface;
    tree.rowAltStyle = StyleBackground;
    tree.hoverStyle = StyleAccent;
    tree.selectionStyle = StyleAccent;
    tree.selectionAccentStyle = StyleAccent;
    tree.caretBackgroundStyle = StyleSurface;
    tree.caretLineStyle = StyleAccent;
    tree.connectorStyle = StyleBackground;
    tree.focusStyle = StyleFocus;
    tree.textStyle = 0u;
    tree.selectedTextStyle = 0u;
    tree.size.stretchX = 1.0f;
    tree.size.stretchY = 1.0f;
    tree.size.preferredHeight = 660.0f;
    tree.scrollBar.enabled = true;
    tree.scrollBar.autoThumb = true;
    tree.scrollBar.width = 9.0f;
    tree.scrollBar.padding = 7.0f;
    tree.scrollBar.trackStyle = StyleSurface;
    tree.scrollBar.thumbStyle = StyleAccent;
    if (wireCallbacks) {
      tree.callbacks.onScrollChanged = [this](PrimeStage::TreeViewScrollInfo const& info) {
        scrollEvents += 1;
        lastScroll = info;
      };
    }

    PrimeStage::UiNode treeUi = page.createTreeView(tree);
    treeNode = treeUi.nodeId();
  }

  void runLayoutPass() {
    layout = layoutFrame(frame, TreeRootWidth, TreeRootHeight);
    focus.updateAfterRebuild(frame, layout);
  }

  void rebuild(bool wireCallbacks) {
    buildFrame(wireCallbacks);
    runLayoutPass();
    needsRebuild = false;
  }

  bool runWheelInteraction() {
    if (!treeNode.isValid()) {
      return false;
    }

    PrimeFrame::LayoutOut const* out = layout.get(treeNode);
    if (!out) {
      return false;
    }

    float x = out->absX + out->absW * 0.5f;
    float y = out->absY + out->absH * 0.5f;
    router.dispatch(makePointerScrollEvent(x, y, 52.0f), frame, layout, &focus);

    if (needsRebuild) {
      rebuild(true);
    }

    PerfSink += static_cast<uint64_t>(std::max(lastScroll.offset, 0.0f));
    return true;
  }
};

bool runBenchmarks(BenchmarkOptions const& options,
                   std::vector<MetricResult>& results,
                   std::string& error) {
  results.clear();

  DashboardRuntime dashboard;
  dashboard.initializeState();

  if (auto metric = runMetric("scene.dashboard.rebuild.p95_us",
                              options.warmupIterations,
                              options.benchmarkIterations,
                              [&]() {
                                dashboard.buildFrame(false);
                                PerfSink += dashboard.textFieldNode.isValid() ? 1u : 0u;
                                return dashboard.textFieldNode.isValid();
                              },
                              error)) {
    results.push_back(*metric);
  } else {
    return false;
  }

  dashboard.buildFrame(false);
  if (auto metric = runMetric("scene.dashboard.layout.p95_us",
                              options.warmupIterations,
                              options.benchmarkIterations,
                              [&]() {
                                dashboard.runLayoutPass();
                                PerfSink += dashboard.layout.get(dashboard.textFieldNode) ? 1u : 0u;
                                return true;
                              },
                              error)) {
    results.push_back(*metric);
  } else {
    return false;
  }

  dashboard.rebuild(false);
  std::vector<uint8_t> dashboardPixels(
      static_cast<size_t>(DashboardRootWidth * DashboardRootHeight * 4.0f),
      0u);
  PrimeStage::RenderTarget dashboardTarget;
  dashboardTarget.pixels = std::span<uint8_t>(dashboardPixels);
  dashboardTarget.width = static_cast<uint32_t>(DashboardRootWidth);
  dashboardTarget.height = static_cast<uint32_t>(DashboardRootHeight);
  dashboardTarget.stride = dashboardTarget.width * 4u;
  dashboardTarget.scale = 1.0f;

  if (auto metric = runMetric("scene.dashboard.render.p95_us",
                              options.warmupIterations,
                              options.benchmarkIterations,
                              [&]() {
                                bool ok = PrimeStage::renderFrameToTarget(
                                    dashboard.frame,
                                    dashboard.layout,
                                    dashboardTarget,
                                    PrimeStage::RenderOptions{});
                                if (!ok) {
                                  return false;
                                }
                                PerfSink += dashboardPixels[0];
                                return true;
                              },
                              error)) {
    results.push_back(*metric);
  } else {
    return false;
  }

  TreeRuntime tree;
  tree.rebuild(false);

  if (auto metric = runMetric("scene.tree.rebuild.p95_us",
                              options.warmupIterations,
                              options.benchmarkIterations,
                              [&]() {
                                tree.buildFrame(false);
                                PerfSink += tree.treeNode.isValid() ? 1u : 0u;
                                return tree.treeNode.isValid();
                              },
                              error)) {
    results.push_back(*metric);
  } else {
    return false;
  }

  tree.buildFrame(false);
  if (auto metric = runMetric("scene.tree.layout.p95_us",
                              options.warmupIterations,
                              options.benchmarkIterations,
                              [&]() {
                                tree.runLayoutPass();
                                PerfSink += tree.layout.get(tree.treeNode) ? 1u : 0u;
                                return true;
                              },
                              error)) {
    results.push_back(*metric);
  } else {
    return false;
  }

  tree.rebuild(false);
  std::vector<uint8_t> treePixels(static_cast<size_t>(TreeRootWidth * TreeRootHeight * 4.0f), 0u);
  PrimeStage::RenderTarget treeTarget;
  treeTarget.pixels = std::span<uint8_t>(treePixels);
  treeTarget.width = static_cast<uint32_t>(TreeRootWidth);
  treeTarget.height = static_cast<uint32_t>(TreeRootHeight);
  treeTarget.stride = treeTarget.width * 4u;
  treeTarget.scale = 1.0f;

  if (auto metric = runMetric("scene.tree.render.p95_us",
                              options.warmupIterations,
                              options.benchmarkIterations,
                              [&]() {
                                bool ok = PrimeStage::renderFrameToTarget(tree.frame,
                                                                           tree.layout,
                                                                           treeTarget,
                                                                           PrimeStage::RenderOptions{});
                                if (!ok) {
                                  return false;
                                }
                                PerfSink += treePixels[0];
                                return true;
                              },
                              error)) {
    results.push_back(*metric);
  } else {
    return false;
  }

  dashboard.initializeState();
  dashboard.rebuild(true);
  if (auto metric = runMetric("interaction.typing.p95_us",
                              options.warmupIterations,
                              options.benchmarkIterations,
                              [&]() { return dashboard.runTypingInteraction(); },
                              error)) {
    results.push_back(*metric);
  } else {
    return false;
  }

  dashboard.initializeState();
  dashboard.rebuild(true);
  if (auto metric = runMetric("interaction.drag.p95_us",
                              options.warmupIterations,
                              options.benchmarkIterations,
                              [&]() { return dashboard.runSliderDragInteraction(); },
                              error)) {
    results.push_back(*metric);
  } else {
    return false;
  }

  TreeRuntime wheelTree;
  wheelTree.rebuild(true);
  if (auto metric = runMetric("interaction.wheel.p95_us",
                              options.warmupIterations,
                              options.benchmarkIterations,
                              [&]() { return wheelTree.runWheelInteraction(); },
                              error)) {
    results.push_back(*metric);
  } else {
    return false;
  }

  return true;
}

} // namespace

int main(int argc, char** argv) {
  std::optional<BenchmarkOptions> options = parseOptions(argc, argv);
  if (!options.has_value()) {
    return 2;
  }

  std::vector<MetricResult> metrics;
  std::string error;
  if (!runBenchmarks(*options, metrics, error)) {
    std::cerr << "Benchmark run failed: " << error << "\n";
    return 1;
  }

  printMetrics(metrics, options->warmupIterations, options->benchmarkIterations);

  if (!options->outputFile.empty()) {
    if (!writeMetricsJson(options->outputFile, metrics, *options)) {
      std::cerr << "Failed to write benchmark output file: " << options->outputFile << "\n";
      return 1;
    }
  }

  if (options->checkBudgets) {
    std::string budgetError;
    std::optional<std::unordered_map<std::string, double>> budgets =
        loadBudgets(options->budgetFile, budgetError);
    if (!budgets.has_value()) {
      std::cerr << "Failed to parse budgets: " << budgetError << "\n";
      return 1;
    }
    if (!enforceBudgets(metrics, *budgets)) {
      return 1;
    }
    std::cout << "Performance budgets passed.\n";
  }

  return PerfSink == std::numeric_limits<uint64_t>::max() ? 3 : 0;
}
