  UiNode createPanel(RectRole role, Bounds const& bounds);
  UiNode createPanel(RectRole role, SizeSpec const& size);
  UiNode createLabel(std::string_view text, TextRole role, Bounds const& bounds);
  UiNode createLabel(std::string_view text, TextRole role, SizeSpec const& size);
  UiNode createParagraph(Bounds const& bounds,
                         std::string_view text,
                         TextRole role = TextRole::BodyMuted);
  UiNode createTextLine(Bounds const& bounds,
                        std::string_view text,
                        TextRole role = TextRole::BodyBright,
                        PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start);
  UiNode createTextLine(std::string_view text,
                        TextRole role,
                        SizeSpec const& size,
                        PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start);
  UiNode createTable(TableSpec const& spec);
  UiNode createTable(Bounds const& bounds,
                     std::vector<TableColumn> columns,
                     std::vector<std::vector<std::string>> rows);
  UiNode createSectionHeader(SectionHeaderSpec const& spec);
  UiNode createSectionHeader(Bounds const& bounds,
                             std::string_view title,
                             TextRole role = TextRole::SmallBright);
  UiNode createSectionHeader(Bounds const& bounds,
                             std::string_view title,
                             TextRole role,
                             bool addDivider,
                             float dividerOffsetY = 0.0f);
  SectionPanel createSectionPanel(SectionPanelSpec const& spec);
  SectionPanel createSectionPanel(Bounds const& bounds, std::string_view title);
  UiNode createPropertyList(PropertyListSpec const& spec);
  UiNode createPropertyList(Bounds const& bounds, std::vector<PropertyRow> rows);
  UiNode createPropertyRow(Bounds const& bounds,
                           std::string_view label,
                           std::string_view value,
                           TextRole role = TextRole::SmallBright);
  UiNode createProgressBar(ProgressBarSpec const& spec);
  UiNode createProgressBar(Bounds const& bounds, float value);
  UiNode createStatusBar(StatusBarSpec const& spec);
  UiNode createStatusBar(Bounds const& bounds,
                         std::string_view leftText,
                         std::string_view rightText);
  UiNode createCardGrid(CardGridSpec const& spec);
  UiNode createCardGrid(Bounds const& bounds, std::vector<CardSpec> cards);
  UiNode createButton(Bounds const& bounds,
                      std::string_view label,
                      ButtonVariant variant = ButtonVariant::Default);
  UiNode createButton(std::string_view label,
                      ButtonVariant variant,
                      SizeSpec const& size);
  UiNode createTextField(Bounds const& bounds, std::string_view placeholder);
  UiNode createTextField(std::string_view placeholder, SizeSpec const& size);
  UiNode createScrollHints(ScrollHintsSpec const& spec);
  UiNode createScrollHints(Bounds const& bounds);
  UiNode createTreeView(TreeViewSpec const& spec);
