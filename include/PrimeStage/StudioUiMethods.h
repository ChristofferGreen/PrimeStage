#pragma once

#include "PrimeStage/StudioUiTypes.h"

#include <string_view>
#include <vector>

namespace PrimeStage {

[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createPanel(UiNode const& parent, RectRole role, Bounds const& bounds);
UiNode createPanel(UiNode const& parent, RectRole role, SizeSpec const& size);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createLabel(UiNode const& parent, std::string_view text, TextRole role, Bounds const& bounds);
UiNode createLabel(UiNode const& parent, std::string_view text, TextRole role, SizeSpec const& size);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createParagraph(UiNode const& parent,
                       Bounds const& bounds,
                       std::string_view text,
                       TextRole role = TextRole::BodyMuted);
UiNode createParagraph(UiNode const& parent, std::string_view text, TextRole role, SizeSpec const& size);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createTextLine(UiNode const& parent,
                      Bounds const& bounds,
                      std::string_view text,
                      TextRole role = TextRole::BodyBright,
                      PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start);
UiNode createTextLine(UiNode const& parent,
                      std::string_view text,
                      TextRole role,
                      SizeSpec const& size,
                      PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start);
UiNode createTable(UiNode const& parent, TableSpec const& spec);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createTable(UiNode const& parent,
                   Bounds const& bounds,
                   std::vector<TableColumn> columns,
                   std::vector<std::vector<std::string>> rows);
UiNode createSectionHeader(UiNode const& parent, SectionHeaderSpec const& spec);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createSectionHeader(UiNode const& parent,
                           Bounds const& bounds,
                           std::string_view title,
                           TextRole role = TextRole::SmallBright);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createSectionHeader(UiNode const& parent,
                           Bounds const& bounds,
                           std::string_view title,
                           TextRole role,
                           bool addDivider,
                           float dividerOffsetY = 0.0f);
UiNode createSectionHeader(UiNode const& parent,
                           SizeSpec const& size,
                           std::string_view title,
                           TextRole role = TextRole::SmallBright);
UiNode createSectionHeader(UiNode const& parent,
                           SizeSpec const& size,
                           std::string_view title,
                           TextRole role,
                           bool addDivider,
                           float dividerOffsetY = 0.0f);
SectionPanel createSectionPanel(UiNode const& parent, SectionPanelSpec const& spec);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
SectionPanel createSectionPanel(UiNode const& parent, Bounds const& bounds, std::string_view title);
SectionPanel createSectionPanel(UiNode const& parent, SizeSpec const& size, std::string_view title);
UiNode createPropertyList(UiNode const& parent, PropertyListSpec const& spec);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createPropertyList(UiNode const& parent, Bounds const& bounds, std::vector<PropertyRow> rows);
UiNode createPropertyList(UiNode const& parent, SizeSpec const& size, std::vector<PropertyRow> rows);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createPropertyRow(UiNode const& parent,
                         Bounds const& bounds,
                         std::string_view label,
                         std::string_view value,
                         TextRole role = TextRole::SmallBright);
UiNode createPropertyRow(UiNode const& parent,
                         SizeSpec const& size,
                         std::string_view label,
                         std::string_view value,
                         TextRole role = TextRole::SmallBright);
UiNode createProgressBar(UiNode const& parent, ProgressBarSpec const& spec);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createProgressBar(UiNode const& parent, Bounds const& bounds, float value);
UiNode createProgressBar(UiNode const& parent, SizeSpec const& size, float value);
UiNode createStatusBar(UiNode const& parent, StatusBarSpec const& spec);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createStatusBar(UiNode const& parent,
                       Bounds const& bounds,
                       std::string_view leftText,
                       std::string_view rightText);
UiNode createStatusBar(UiNode const& parent,
                       SizeSpec const& size,
                       std::string_view leftText,
                       std::string_view rightText);
UiNode createCardGrid(UiNode const& parent, CardGridSpec const& spec);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createCardGrid(UiNode const& parent, Bounds const& bounds, std::vector<CardSpec> cards);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createButton(UiNode const& parent,
                    Bounds const& bounds,
                    std::string_view label,
                    ButtonVariant variant = ButtonVariant::Default);
UiNode createButton(UiNode const& parent,
                    std::string_view label,
                    ButtonVariant variant,
                    SizeSpec const& size);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createTextField(UiNode const& parent, Bounds const& bounds, std::string_view placeholder);
UiNode createTextField(UiNode const& parent, std::string_view placeholder, SizeSpec const& size);
UiNode createScrollHints(UiNode const& parent, ScrollHintsSpec const& spec);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createScrollHints(UiNode const& parent, Bounds const& bounds);
UiNode createTreeView(UiNode const& parent, TreeViewSpec const& spec);

[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
UiNode createRoot(PrimeFrame::Frame& frame, Bounds const& bounds);
UiNode createRoot(PrimeFrame::Frame& frame, SizeSpec const& size);
ShellLayout createShell(PrimeFrame::Frame& frame, ShellSpec const& spec);
[[deprecated("Use SizeSpec-based overloads to avoid absolute layout.")]]
ShellSpec makeShellSpec(Bounds const& bounds);
ShellSpec makeShellSpec(SizeSpec const& size);

} // namespace PrimeStage
