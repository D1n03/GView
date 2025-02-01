#include "EntropyVisualizer.hpp"

#include <math.h>

namespace GView::GenericPlugins::EntropyVisualizer
{
Color Plugin::ShannonEntropyValueToColor(int32 value)
{
    switch (value) {
    case 0:
        return Color::White;
    case 1:
        return Color::Silver;
    case 2:
        return Color::Gray;
    case 3:
        return Color::Olive;
    case 4:
        return Color::Yellow;
    case 5:
        return Color::DarkGreen;
    case 6:
        return Color::DarkRed;
    case 7:
        return Color::Magenta;
    case 8:
        return Color::Red;
    default:
        if (value < 0) {
            return Color::Black;
        } else {
            return Color::Pink;
        }
    }
}

Color Plugin::ShannonEntropyDataTypeValueToColor(double value, double epsilon)
{
    if (value >= 8.0 - epsilon && value < 8.0) {
        return Color::Green;
    }

    if (value > 6.0 && value < 8.0 - epsilon) {
        return Color::Red;
    }

    return Color::Gray;
}

Color Plugin::ShannonEntropyDataTypeValueToColorName(std::string_view name)
{
    if (name == "Plain") {
        return Color::Gray;
    } else if (name == "Binary") {
        return Color::Red;
    } else if (name == "Encrypted") {
        return Color::Green;
    }
    return Color::Black;
}

double Plugin::ComputeEpsilon(uint64 sample_size)
{
    return 2.0 - (log2(sample_size) - 2.0) / 10;
}

// TODO: configurable colors using color picker
Color Plugin::EmbeddedObjectValueToColor(std::string_view name)
{
    if (name == "Archive") {
        return Color::White;
    } else if (name == "Cryptographic") {
        return Color::Silver;
    } else if (name == "Executable") {
        return Color::Red;
    } else if (name == "HTML Object") {
        return Color::Olive;
    } else if (name == "Image") {
        return Color::Yellow;
    } else if (name == "Multimedia") {
        return Color::DarkGreen;
    } else if (name == "Special Strings") {
        return Color::Aqua;
    } else {
        return Color::Gray;
    }
}

Plugin::Plugin(Reference<Object> object) : Window("Entropy Visualizer", "d:c,w:100%,h:98%", WindowFlags::FixedPosition)
{
    auto desktop = AppCUI::Application::GetDesktop();
    this->parent = desktop->GetFocusedChild();
    this->object = object;
    {
        this->canvasEntropy = Factory::CanvasViewer::Create(this, "d:lb,w:80%,h:100%", this->GetWidth(), this->GetHeight(), Controls::ViewerFlags::Border);
        auto canvas         = this->canvasEntropy->GetCanvas();
        canvas->Resize(this->canvasEntropy->GetWidth(), this->canvasEntropy->GetHeight());
        canvas->SetCursor(0, 0);
    }
    {
        Factory::Label::Create(this, "Entropy type", "x:81%, y:0,w:12,h:1");
        this->entropyComboBox = Factory::ComboBox::Create(this, "x:81%,y:1,w:19%,h:1", "");
        entropyComboBox->SetHotKey('E');
        entropyComboBox->AddItem(SHANNON_ENTROPY_OPTION_NAME, COMBO_BOX_ITEM_SHANNON_ENTROPY);
        entropyComboBox->AddItem(RENYI_ENTROPY_OPTION_NAME, COMBO_BOX_ITEM_RENYI_ENTROPY);
        entropyComboBox->AddSeparator();
        entropyComboBox->AddItem(SHANNON_ENTROPY_DATA_TYPE_OPTION_NAME, COMBO_BOX_ITEM_SHANNON_ENTROPY_DATA_TYPE);
        entropyComboBox->AddItem(EMBEDDED_OBJECTS_OPTION_NAME, COMBO_BOX_ITEM_EMBEDDED_OBJECTS);
        // TODO: add the rest
        entropyComboBox->SetCurentItemIndex(0);
    }
    {
        Factory::Label::Create(this, "Block size", "x:81%,y:3,w:19%,h:1");
        this->blockSizeSelector = Factory::NumericSelector::Create(this, MINIMUM_BLOCK_SIZE, object->GetData().GetSize(), 32, "x:81%,y:4,w:19%,h:1");
        blockSizeSelector->SetHotKey('B');
    }
    {
        Factory::Label::Create(this, "Alpha (Renyi) /10", "x:81%,y:6,w:19%,h:1");
        this->alphaSelector = Factory::NumericSelector::Create(this, -99, 99, 0, "x:81%,y:7,w:19%,h:1");
    }
    {
        this->canvasLegend = Factory::CanvasViewer::Create(
              this, "x:81%,y:9,w:19%,h:20%", this->GetWidth(), this->GetHeight(), Controls::ViewerFlags::Border | Controls::ViewerFlags::HideScrollBar);
        auto canvas = this->canvasLegend->GetCanvas();
        canvas->Resize(this->canvasLegend->GetWidth(), this->canvasLegend->GetHeight());
    }

    this->InitializeBlocksForCanvas();
    // raise events after all children are initialized
    this->entropyComboBox->RaiseEvent(Event::ComboBoxClosed);

    this->canvasEntropy->SetFocus();
}

bool Plugin::DrawEntropy(EntropyType type)
{
    CHECK(this->canvasEntropy.IsValid(), false, "");
    auto canvas = this->canvasEntropy->GetCanvas();

    auto& cache              = object->GetData();
    const auto size          = cache.GetSize();
    const auto epsilon       = ComputeEpsilon(this->blockSize);
    const uint32 blocksCount = static_cast<uint32>(size / this->blockSize + 1);

    uint32 x         = 0;
    uint32 y         = 0;
    uint32 maxX      = canvas->GetWidth();
    uint32 maxY      = std::max<uint32>(blocksCount / maxX + 1 + 1, canvas->GetHeight());
    const auto color = ColorPair{ Color::White, this->GetConfig()->Window.Background.Normal };
    canvas->Resize(maxX, maxY, 'X', color);
    canvas->ClearEntireSurface('X', color);

    for (uint32 i = 0; i < blocksCount; i++) {
        auto bf    = cache.Get(i * static_cast<uint64>(this->blockSize), this->blockSize, false);
        auto value = 0.0;
        switch (type) {
        case EntropyType::Shannon:
        case EntropyType::ShannonDataType:
            value = GView::Entropy::ShannonEntropy(bf);
            break;
        case EntropyType::Renyi:
            value = GView::Entropy::RenyiEntropy(bf, this->renyiAlpha);
        default:
            break;
        }

        auto fColor = Color::Black;
        switch (type) {
        case EntropyType::Shannon:
        case EntropyType::Renyi:
            fColor = ShannonEntropyValueToColor(static_cast<uint32>(std::llround(value)));
            break;
        case EntropyType::ShannonDataType:
            fColor = ShannonEntropyDataTypeValueToColor(value, epsilon);
        default:
            break;
        }

        canvas->WriteSpecialCharacter(x++, y, BLOCK_SPECIAL_CHARACTER, ColorPair{ fColor, CANVAS_ENTROPY_BACKGROUND });
        if (x == maxX) {
            x = 0;
            y++;
        }
    }

    return true;
}

bool Plugin::DrawEntropyLegend(EntropyType type)
{
    CHECK(this->canvasLegend.IsValid(), false, "");
    ResizeLegendCanvas();

    auto canvas      = this->canvasLegend->GetCanvas();
    const auto color = ColorPair{ Color::White, this->GetConfig()->Window.Background.Normal };
    canvas->Clear(' ', color);

    std::string_view name = "";
    switch (type) {
    case EntropyType::Shannon:
        name = "Shanon Legend [0-8]";
        break;
    case EntropyType::ShannonDataType:
        name = "Shanon Data Type Legend";
        break;
    case EntropyType::Renyi:
        name = "Renyi Legend [0-8]";
        break;
    default:
        break;
    }

    uint32 x = 0;
    uint32 y = 0;
    canvas->WriteSingleLineText(x, y++, name, color);
    canvas->FillHorizontalLineWithSpecialChar(x, y++, canvas->GetWidth(), SpecialChars::BoxHorizontalSingleLine, color);
    x = 0;

    switch (type) {
    case EntropyType::Shannon:
    case EntropyType::Renyi:
        for (uint32 i = 0; i <= SHANNON_ENTROPY_MAX_VALUE; i++) {
            canvas->WriteCharacter(x++, y, i + '0', color);
            canvas->WriteCharacter(x++, y, ' ', color);
            canvas->WriteCharacter(x++, y, '=', color);
            canvas->WriteCharacter(x++, y, '>', color);
            canvas->WriteCharacter(x++, y, ' ', color);

            while (x < canvas->GetWidth()) {
                canvas->WriteSpecialCharacter(x++, y, BLOCK_SPECIAL_CHARACTER, ColorPair{ ShannonEntropyValueToColor(i), CANVAS_ENTROPY_BACKGROUND });
            }
            y++;
            x = 0;
        }
        break;
    case EntropyType::ShannonDataType: {
        static std::vector<std::string_view> SHANNON_ENTROPY_DATA_TYPES{ "Plain", "Binary", "Encrypted" };

        for (uint32 i = 0; i <= SHANNON_ENTROPY_DATA_TYPE_MAX_VALUE; i++) {
            const auto& name = SHANNON_ENTROPY_DATA_TYPES.at(i);
            canvas->WriteSingleLineText(x, y++, name, color);

            while (x < canvas->GetWidth()) {
                canvas->WriteSpecialCharacter(
                      x++, y, BLOCK_SPECIAL_CHARACTER, ColorPair{ ShannonEntropyDataTypeValueToColorName(name), CANVAS_ENTROPY_BACKGROUND });
            }

            y++;
            x = 0;
        }
    } break;
    default:
        break;
    }

    return true;
}

bool Plugin::DrawEmbeddedObjects()
{
    constexpr std::string_view VIEW_NAME{ "Buffer View" };

    auto interface = this->parent.ToObjectRef<GView::View::WindowInterface>();

    auto currentView           = interface->GetCurrentView();
    const auto currentViewName = currentView->GetName();

    CHECK(currentViewName == VIEW_NAME, true, "");

    const auto& zones = currentView->GetObjectsHighlightingZonesList();

    CHECK(this->canvasEntropy.IsValid(), false, "");
    auto canvas = this->canvasEntropy->GetCanvas();

    auto& cache              = object->GetData();
    const auto size          = cache.GetSize();
    const uint32 blocksCount = static_cast<uint32>(size / this->blockSize + 1);

    uint32 x         = 0;
    uint32 y         = 0;
    uint32 maxX      = canvas->GetWidth();
    uint32 maxY      = std::max<uint32>(blocksCount / maxX + 1 + 1, canvas->GetHeight());
    const auto color = ColorPair{ Color::White, this->GetConfig()->Window.Background.Normal };
    canvas->Resize(maxX, maxY, 'X', color);
    canvas->ClearEntireSurface('X', color);

    for (uint32 i = 0; i < blocksCount; i++) {
        const auto fColor = EmbeddedObjectValueToColor("");
        canvas->WriteCharacter(x++, y, ' ', ColorPair{ fColor, fColor });
        if (x == maxX) {
            x = 0;
            y++;
        }
    }
    x = 0;
    y = 0;

    const auto zonesNo = zones.GetCount();
    for (uint32 i = 0; i < zonesNo; i++) {
        const auto& zone = zones.GetZone(i);
        if (zone.has_value()) {
            const auto blockStart  = zone->interval.low / this->blockSize;
            const auto blockEnd    = zone->interval.high / this->blockSize;
            const auto deltaBlocks = blockEnd - blockStart;

            x = blockStart % maxX;
            y = blockStart / maxX;
            y = blockStart / maxX;

            // bad.. TODO: change
            Color c       = EmbeddedObjectValueToColor("Executable");
            const auto zn = std::string_view{ zone->name };
            if (zn == "Email Address") {
                c = EmbeddedObjectValueToColor("Special Strings");
            } else if (zn == "Filepath") {
                c = EmbeddedObjectValueToColor("Special Strings");
            } else if (zn == "IFrame") {
                c = EmbeddedObjectValueToColor("HTML Object");
            } else if (zn == "IP Address") {
                c = EmbeddedObjectValueToColor("Special Strings");
            } else if (zn == "MZPE") {
                c = EmbeddedObjectValueToColor("Executable");
            } else if (zn == "PHP") {
                c = EmbeddedObjectValueToColor("HTML Object");
            } else if (zn == "PNG") {
                c = EmbeddedObjectValueToColor("Image");
            } else if (zn == "Registry") {
                c = EmbeddedObjectValueToColor("Special Strings");
            } else if (zn == "Script") {
                c = EmbeddedObjectValueToColor("HTML Object");
            } else if (zn == "URL") {
                c = EmbeddedObjectValueToColor("Special Strings");
            } else if (zn == "Wallet") {
                c = EmbeddedObjectValueToColor("Special Strings");
            } else if (zn == "XML") {
                c = EmbeddedObjectValueToColor("HTML Object");
            }

            for (uint32 j = 0; j <= deltaBlocks; j++) {
                canvas->WriteSpecialCharacter(x++, y, BLOCK_SPECIAL_CHARACTER, ColorPair{ c, CANVAS_ENTROPY_BACKGROUND });
                if (x == maxX) {
                    x = 0;
                    y++;
                }
            }
        }
    }

    return true;
}

bool Plugin::DrawEmbeddedObjectsLegend()
{
    CHECK(this->canvasLegend.IsValid(), false, "");
    ResizeLegendCanvas();

    auto canvas = this->canvasLegend->GetCanvas();

    const auto color = ColorPair{ Color::White, this->GetConfig()->Window.Background.Normal };

    uint32 x = 0;
    uint32 y = 0;
    canvas->Clear(' ', color);

    canvas->WriteSingleLineText(x, y++, "Embedded objects", color);
    canvas->FillHorizontalLineWithSpecialChar(x, y++, canvas->GetWidth(), SpecialChars::BoxHorizontalSingleLine, color);
    x = 0;

    static std::vector<std::string_view> EMBEDDED_OBJECTS{ "Archive", "Cryptographic", "Executable", "HTML Object", "Image", "Multimedia", "Special Strings" };

    for (uint32 i = 0; i <= EMBEDDED_OBJECTS_MAX_VALUE; i++) {
        const auto& name = EMBEDDED_OBJECTS.at(i);
        canvas->WriteSingleLineText(x, y++, name, color);

        while (x < canvas->GetWidth()) {
            canvas->WriteSpecialCharacter(x++, y, BLOCK_SPECIAL_CHARACTER, ColorPair{ EmbeddedObjectValueToColor(name), CANVAS_ENTROPY_BACKGROUND });
        }

        y++;
        x = 0;
    }
    canvas->WriteSingleLineText(x, y++, "Unknown", color);
    while (x < canvas->GetWidth()) {
        canvas->WriteSpecialCharacter(x++, y, BLOCK_SPECIAL_CHARACTER, ColorPair{ EmbeddedObjectValueToColor(""), CANVAS_ENTROPY_BACKGROUND });
    }

    return true;
}

std::optional<GView::Utils::Zone> Plugin::IsOffsetInZone(const GView::Utils::ZonesList& zones, uint64 offset) const
{
    const auto zonesNo = zones.GetCount();
    for (uint32 i = 0; i < zonesNo; i++) {
        const auto zone = zones.GetZone(i);
        if (zone.has_value()) {
            if (offset >= zone->interval.low && offset <= zone->interval.high) {
                return zone;
            }
        }
    }

    return std::nullopt;
}

void Plugin::OnAfterResize(int, int)
{
    ResizeLegendCanvas();

    // this->MoveTo(this->parent->GetX() + 10, this->parent->GetY() + 10);
    // this->Resize(this->parent->GetWidth() - 10, this->parent->GetHeight() - 10);
}

void Plugin::ResizeLegendCanvas()
{
    CHECKRET(canvasLegend.IsValid(), "");
    CHECKRET(canvasEntropy.IsValid(), "");
    // this->canvasLegend->MoveTo(this->canvasLegend->GetX(), this->canvasEntropy->GetY());

    uint32 newHeight   = this->canvasLegend->GetHeight();
    const auto entropy = this->entropyComboBox->GetCurrentItemUserData(-1);
    if (entropy == COMBO_BOX_ITEM_SHANNON_ENTROPY) {
        newHeight = SHANNON_ENTROPY_LEGEND_HEIGHT;
    } else if (entropy == COMBO_BOX_ITEM_SHANNON_ENTROPY_DATA_TYPE) {
        newHeight = SHANNON_ENTROPY_LEGEND_DATA_TYPE_HEIGHT;
    } else if (entropy == COMBO_BOX_ITEM_EMBEDDED_OBJECTS) {
        newHeight = EMBEDDED_OBJECTS_LEGEND_HEIGHT;
    }

    this->canvasLegend->Resize(this->canvasLegend->GetWidth(), newHeight);
    auto canvas = this->canvasLegend->GetCanvas();
    canvas->Resize(this->canvasLegend->GetWidth(), newHeight);
}

bool Plugin::OnEvent(Reference<Control> sender, Event eventType, int controlID)
{
    if (Window::OnEvent(sender, eventType, controlID)) {
        return true;
    }

    const auto drawSelectedEntropyType = [this]() -> bool {
        const auto entropy = this->entropyComboBox->GetCurrentItemUserData(-1);
        switch (entropy) {
        case COMBO_BOX_ITEM_SHANNON_ENTROPY:
            this->DrawEntropy(EntropyType::Shannon);
            this->DrawEntropyLegend(EntropyType::Shannon);
            break;
        case COMBO_BOX_ITEM_RENYI_ENTROPY:
            this->DrawEntropy(EntropyType::Renyi);
            this->DrawEntropyLegend(EntropyType::Renyi);
            break;
        case COMBO_BOX_ITEM_SHANNON_ENTROPY_DATA_TYPE:
            this->DrawEntropy(EntropyType::ShannonDataType);
            this->DrawEntropyLegend(EntropyType::ShannonDataType);
            break;
        case COMBO_BOX_ITEM_EMBEDDED_OBJECTS:
            this->DrawEmbeddedObjects();
            this->DrawEmbeddedObjectsLegend();
            break;
        default:
            break;
        }
        return true;
    };

    switch (eventType) {
    case AppCUI::Controls::Event::ComboBoxSelectedItemChanged:
        /* nothing, it is to costly computing entropy each time / on the fly */
        break;
    case AppCUI::Controls::Event::ComboBoxClosed:
        if (sender == this->entropyComboBox.ToBase<Control>()) {
            return drawSelectedEntropyType();
        }
        break;
    case AppCUI::Controls::Event::NumericSelectorValueChanged:
        if (sender == this->blockSizeSelector.ToBase<Control>()) {
            this->blockSize = this->blockSizeSelector->GetValue();
            return drawSelectedEntropyType();
        } else if (sender == this->alphaSelector.ToBase<Control>()) {
            this->renyiAlpha = this->alphaSelector->GetValue() / 10;
            return drawSelectedEntropyType();
        }
        break;
    default:
        break;
    }

    return false;
}

bool Plugin::InitializeBlocksForCanvas()
{
    CHECK(this->object.IsValid(), false, "");
    CHECK(this->canvasEntropy.IsValid(), false, "");

    const auto size         = this->object->GetData().GetSize();
    auto canvas             = this->canvasEntropy->GetCanvas();
    const auto canvasWidth  = canvas->GetWidth();
    const auto canvasHeight = canvas->GetHeight();

    uint32 blocksRows = 0;
    do {
        uint32 blocksCount = static_cast<uint32>(size / this->blockSize);
        blocksRows         = blocksCount / canvasWidth + 1;
        this->blockSize *= 2;
    } while (blocksRows > canvasHeight);
    this->blockSize /= 2;
    this->blockSizeSelector->SetValue(this->blockSize);

    return true;
}
} // namespace GView::GenericPlugins::EntropyVisualizer
