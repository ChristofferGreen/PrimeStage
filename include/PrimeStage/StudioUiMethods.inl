  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createPanel(RectRole role, Bounds const& bounds);
  UiNode createPanel(RectRole role, SizeSpec const& size);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createLabel(std::string_view text, TextRole role, Bounds const& bounds);
  UiNode createLabel(std::string_view text, TextRole role, SizeSpec const& size);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createParagraph(Bounds const& bounds,
                         std::string_view text,
                         TextRole role = TextRole::BodyMuted);
  UiNode createParagraph(std::string_view text,
                         TextRole role,
                         SizeSpec const& size);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createTextLine(Bounds const& bounds,
                        std::string_view text,
                        TextRole role = TextRole::BodyBright,
                        PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start);
  UiNode createTextLine(std::string_view text,
                        TextRole role,
                        SizeSpec const& size,
                        PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start);
  UiNode createTable(TableSpec const& spec);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createTable(Bounds const& bounds,
                     std::vector<TableColumn> columns,
                     std::vector<std::vector<std::string>> rows);
  UiNode createSectionHeader(SectionHeaderSpec const& spec);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createSectionHeader(Bounds const& bounds,
                             std::string_view title,
                             TextRole role = TextRole::SmallBright);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createSectionHeader(Bounds const& bounds,
                             std::string_view title,
                             TextRole role,
                             bool addDivider,
                             float dividerOffsetY = 0.0f);
  UiNode createSectionHeader(SizeSpec const& size,
                             std::string_view title,
                             TextRole role = TextRole::SmallBright);
  UiNode createSectionHeader(SizeSpec const& size,
                             std::string_view title,
                             TextRole role,
                             bool addDivider,
                             float dividerOffsetY = 0.0f);
  SectionPanel createSectionPanel(SectionPanelSpec const& spec);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  SectionPanel createSectionPanel(Bounds const& bounds, std::string_view title);
  SectionPanel createSectionPanel(SizeSpec const& size, std::string_view title);
  UiNode createPropertyList(PropertyListSpec const& spec);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createPropertyList(Bounds const& bounds, std::vector<PropertyRow> rows);
  UiNode createPropertyList(SizeSpec const& size, std::vector<PropertyRow> rows);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createPropertyRow(Bounds const& bounds,
                           std::string_view label,
                           std::string_view value,
                           TextRole role = TextRole::SmallBright);
  UiNode createPropertyRow(SizeSpec const& size,
                           std::string_view label,
                           std::string_view value,
                           TextRole role = TextRole::SmallBright);
  UiNode createProgressBar(ProgressBarSpec const& spec);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createProgressBar(Bounds const& bounds, float value);
  UiNode createProgressBar(SizeSpec const& size, float value);
  UiNode createStatusBar(StatusBarSpec const& spec);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createStatusBar(Bounds const& bounds,
                         std::string_view leftText,
                         std::string_view rightText);
  UiNode createStatusBar(SizeSpec const& size,
                         std::string_view leftText,
                         std::string_view rightText);
  UiNode createCardGrid(CardGridSpec const& spec);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createCardGrid(Bounds const& bounds, std::vector<CardSpec> cards);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createButton(Bounds const& bounds,
                      std::string_view label,
                      ButtonVariant variant = ButtonVariant::Default);
  UiNode createButton(std::string_view label,
                      ButtonVariant variant,
                      SizeSpec const& size);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createTextField(Bounds const& bounds, std::string_view placeholder);
  UiNode createTextField(std::string_view placeholder, SizeSpec const& size);
  UiNode createScrollHints(ScrollHintsSpec const& spec);
  [[deprecated("Prefer size-based layout; absolute bounds are discouraged.")]]
  UiNode createScrollHints(Bounds const& bounds);
  UiNode createTreeView(TreeViewSpec const& spec);
