#include "pe.hpp"

using namespace GView::Type::PE;
using namespace AppCUI::Controls;
using namespace AppCUI::Input;

constexpr unsigned int PE_EXP_GOTO = 1;

Panels::Exports::Exports(Reference<GView::Type::PE::PEFile> _pe, Reference<GView::View::WindowInterface> _win) : TabPage("&Exports")
{
    pe  = _pe;
    win = _win;

    list = this->CreateChildControl<ListView>("d:c", ListViewFlags::None);
    list->AddColumn("Name", TextAlignament::Left, 60);
    list->AddColumn("Ord", TextAlignament::Left, 5);
    list->AddColumn("RVA", TextAlignament::Left, 12);

    Update();
}
void Panels::Exports::Update()
{
    LocalString<128> temp;
    NumericFormatter n;

    list->DeleteAllItems();
    for (auto& exp : pe->exp)
    {
        auto handle = list->AddItem(exp.Name, n.ToDec(exp.Ordinal), temp.Format("%u (0x%08X)", exp.RVA, exp.RVA));
        list->SetItemData(handle, (unsigned long long) pe->ConvertAddress(exp.RVA, ADDR_RVA, ADDR_FA));
    }
}
bool Panels::Exports::OnUpdateCommandBar(AppCUI::Application::CommandBar& commandBar)
{
    commandBar.SetCommand(Key::Enter, "GoTo", PE_EXP_GOTO);
    return true;
}
bool Panels::Exports::OnEvent(Reference<Control> ctrl, Event evnt, int controlID)
{
    if (TabPage::OnEvent(ctrl, evnt, controlID))
        return true;
    if ((evnt == Event::ListViewItemClicked) || ((evnt == Event::Command) && (controlID == PE_EXP_GOTO)))
    {
        auto addr = list->GetItemData(list->GetCurrentItem(), GView::Utils::INVALID_OFFSET);
        if (addr != GView::Utils::INVALID_OFFSET)
            win->GetCurrentView()->GoTo(addr);
        return true;
    }
    return false;
}