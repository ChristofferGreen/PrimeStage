#pragma once

#include "PrimeStage/StudioUiTypes.h"

#include <string_view>
#include <vector>

namespace PrimeStage::Studio {

UiNode createPanel(UiNode& parent, RectRole role, SizeSpec const& size);
UiNode createLabel(UiNode& parent, std::string_view text, TextRole role, SizeSpec const& size);
UiNode createParagraph(UiNode& parent, std::string_view text, TextRole role, SizeSpec const& size);
UiNode createTextLine(UiNode& parent,
                      std::string_view text,
                      TextRole role,
                      SizeSpec const& size,
                      PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start);
UiNode createTable(UiNode& parent, TableSpec const& spec);
UiNode createSectionHeader(UiNode& parent, SectionHeaderSpec const& spec);
UiNode createSectionHeader(UiNode& parent,
                           SizeSpec const& size,
                           std::string_view title,
                           TextRole role = TextRole::SmallBright);
UiNode createSectionHeader(UiNode& parent,
                           SizeSpec const& size,
                           std::string_view title,
                           TextRole role,
                           bool addDivider,
                           float dividerOffsetY = 0.0f);
SectionPanel createSectionPanel(UiNode& parent, SectionPanelSpec const& spec);
SectionPanel createSectionPanel(UiNode& parent, SizeSpec const& size, std::string_view title);
UiNode createPropertyList(UiNode const& parent, PropertyListSpec const& spec);
UiNode createPropertyList(UiNode const& parent, SizeSpec const& size, std::vector<PropertyRow> rows);
UiNode createPropertyRow(UiNode const& parent,
                         SizeSpec const& size,
                         std::string_view label,
                         std::string_view value,
                         TextRole role = TextRole::SmallBright);
UiNode createProgressBar(UiNode& parent, ProgressBarSpec const& spec);
UiNode createProgressBar(UiNode& parent, SizeSpec const& size, float value);
UiNode createStatusBar(UiNode& parent, StatusBarSpec const& spec);
UiNode createStatusBar(UiNode& parent,
                       SizeSpec const& size,
                       std::string_view leftText,
                       std::string_view rightText);
UiNode createCardGrid(UiNode& parent, CardGridSpec const& spec);
UiNode createButton(UiNode& parent,
                    std::string_view label,
                    ButtonVariant variant,
                    SizeSpec const& size);
UiNode createTextField(UiNode& parent, std::string_view placeholder, SizeSpec const& size);
UiNode createScrollHints(UiNode& parent, ScrollHintsSpec const& spec);
UiNode createTreeView(UiNode& parent, TreeViewSpec const& spec);

UiNode createRoot(PrimeFrame::Frame& frame, SizeSpec const& size);
UiNode createStudioRoot(PrimeFrame::Frame& frame, SizeSpec const& size);
ShellLayout createShell(PrimeFrame::Frame& frame, ShellSpec const& spec);
ShellSpec makeShellSpec(SizeSpec const& size);

} // namespace PrimeStage::Studio
