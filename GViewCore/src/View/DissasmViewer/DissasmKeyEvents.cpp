#include "DissasmViewer.hpp"
#include <cmath>

constexpr uint32 COMMAND_ADD_NEW_TYPE           = 100;
constexpr uint32 COMMAND_ADD_SHOW_FILE_CONTENT  = 101;
constexpr uint32 COMMAND_EXPORT_ASM_FILE        = 102;
constexpr uint32 COMMAND_JUMP_BACK              = 103;
constexpr uint32 COMMAND_JUMP_FORWARD           = 104;
constexpr uint32 COMMAND_DISSAM_GOTO_ENTRYPOINT = 105;

constexpr uint32 COMMAND_CTRL_CLICK = 200;
constexpr uint32 COMMAND_ALT_CLICK  = 201;

// TODO: fix remove duplicate with Instance.cpp
// constexpr int32 RIGHT_CLICK_MENU_CMD_NEW        = 0;
// constexpr int32 RIGHT_CLICK_MENU_CMD_EDIT       = 1;
// constexpr int32 RIGHT_CLICK_MENU_CMD_DELETE     = 2;
constexpr int32 RIGHT_CLICK_MENU_CMD_COLLAPSE   = 3;
constexpr int32 RIGHT_CLICK_ADD_COMMENT         = 4;
constexpr int32 RIGHT_CLICK_REMOVE_COMMENT      = 5;
constexpr int32 RIGHT_CLICK_DISSASM_ADD_ZONE    = 6;
constexpr int32 RIGHT_CLICK_DISSASM_REMOVE_ZONE = 7;

using namespace GView::View::DissasmViewer;
using namespace AppCUI::Input;

void Instance::AnalyzeMousePosition(int x, int y, MousePositionInfo& mpInfo)
{
    mpInfo.location = MouseLocation::Outside;
    if (y < 0)
    {
        mpInfo.location = MouseLocation::Outside;
        return;
    }
    if (y == 0)
    {
        mpInfo.location = MouseLocation::OnHeader;
        return;
    }
    // y>=1 --> check if in buffer
    auto yPoz = y - 1;
    if (x < 0)
    {
        mpInfo.location = MouseLocation::Outside;
        return;
    }
    const auto xPoz = static_cast<uint32>(x);
    if ((xPoz >= Layout.startingTextLineOffset) && (xPoz < Layout.startingTextLineOffset + Layout.textSize))
    {
        mpInfo.location = MouseLocation::OnView;
        mpInfo.offset   = xPoz - Layout.startingTextLineOffset;
        mpInfo.lines    = yPoz;
    }
    /*if (mpInfo.location == MouseLocation::OnView)
    {
        mpInfo.bufferOffset += Cursor.startView;
        if (mpInfo.bufferOffset >= this->obj->GetData().GetSize())
            mpInfo.location = MouseLocation::Outside;
    }*/
}

void Instance::MoveTo(int32 offset, int32 lines, AppCUI::Input::Key key, bool select)
{
    // TODO-- HERE!!
    //  if (offset == this->Cursor.currentPos)
    //{
    //      this->Cursor.startView = offset;
    //      return;
    //  }

    const bool ctrl_down = ((key & Key::Ctrl) != Key::None);
    const bool alt_down  = ((key & Key::Alt) != Key::None);

    auto zoneId = -1;

    if (select)
        zoneId = this->selection.BeginSelection(Cursor.ToLinePosition(), ctrl_down, alt_down);

    MoveScrollTo(offset, lines);

    if ((select) && (zoneId >= 0))
    {
        this->selection.UpdateSelection(zoneId, Cursor.ToLinePosition(), ctrl_down, alt_down);
        // UpdateCurrentSelection();
    }
    // return;

    // if (lines > 0 && static_cast<uint32>(lines) < Layout.visibleRows)
    //{
    //     this->Cursor.offset += offset;
    //     this->Cursor.lineInView += lines;
    //     if ((select) && (zoneId >= 0))
    //     {
    //         this->selection.UpdateSelection(zoneId, Cursor.GetOffset(Layout.textSize));
    //         // UpdateCurrentSelection();
    //         return; // nothing to do ... already in visual space
    //     }
    // }

    // if (Cursor.lineInView < Cursor.startViewLine)
    //     Cursor.startViewLine = Cursor.lineInView;
    //  else
    //{
    //      auto dif = this->Cursor.currentPos - this->Cursor.startView;
    //      if (offset >= dif)
    //          this->Cursor.startView = offset - dif;
    //      else
    //          this->Cursor.startView = 0;
    //  }
    //  this->Cursor.currentPos = offset;
}

void Instance::MoveScrollTo(int32 offset, int32 lines)
{
    if (!Layout.totalLinesSize)
        return;
    Cursor.hasMovedView = false;

    Cursor.offset += offset;
    // this->Cursor.startViewLine += lines;
    if (lines < 0)
    {
        if (lines * -1 >= static_cast<int32>(Cursor.lineInView))
        {
            lines += static_cast<int32>(Cursor.lineInView);
            Cursor.lineInView = 0;
            if (lines != 0)
            {
                Cursor.startViewLine += lines;
                Cursor.hasMovedView = true;
            }
        }
        else
        {
            Cursor.lineInView += lines;
        }
    }
    else
    {
        Cursor.lineInView += lines;
        if (Cursor.lineInView > Layout.visibleRows - 1)
        {
            const auto diff = abs(static_cast<int32>(Cursor.lineInView) - static_cast<int32>(Layout.visibleRows - 1));
            Cursor.startViewLine += diff;
            Cursor.lineInView -= diff;
            Cursor.hasMovedView = true;
        }
    }
    /*if (this->Cursor.startViewLine > old)
        MoveTo(offset, 0, false);
    else
    {
        auto dif = old - Cursor.startView;
        if (dif <= this->Cursor.currentPos)
            MoveTo(this->Cursor.currentPos - dif, false);
        else
            MoveTo(0, false);
    }*/
}

void Instance::OnMousePressed(int x, int y, AppCUI::Input::MouseButton button)
{
    MousePositionInfo mpInfo;
    AnalyzeMousePosition(x, y, mpInfo);
    // make sure that consecutive click on the same location will not scroll the view to that location
    if (mpInfo.location == MouseLocation::OnView)
    {
        if (button == MouseButton::Left && (mpInfo.lines != Cursor.lineInView || mpInfo.offset != Cursor.offset))
        {
            const int32 linesDiff  = static_cast<int32>(mpInfo.lines) - Cursor.lineInView;
            const int32 offsetDiff = static_cast<int32>(mpInfo.offset) - Cursor.offset;
            Key key                = Key::None;
            if (overriden_keys.is_alt_down)
                key = key | Key::Alt;
            if (overriden_keys.is_ctrl_down)
                key = key | Key::Ctrl;
            MoveTo(offsetDiff, linesDiff, key, false);
        }
        else if (button == MouseButton::Right)
        {
            // rightClickOffset = mpInfo.bufferOffset;
            rightClickMenu.Show(this, x, y);
        }
    }
    else if (mpInfo.location == MouseLocation::Outside && !MyLine.buttons.empty())
    {
        for (const auto& btn : MyLine.buttons)
            if (btn.x == x && btn.y == y)
            {
                ChangeZoneCollapseState(btn.zone);
                break;
            }
    }
}

bool Instance::OnMouseDrag(int x, int y, AppCUI::Input::MouseButton button)
{
    MousePositionInfo mpInfo;
    AnalyzeMousePosition(x, y, mpInfo);
    // make sure that consecutive click on the same location will not scroll the view to that location
    if (button == MouseButton::Left && mpInfo.location == MouseLocation::OnView && (mpInfo.lines != Cursor.lineInView || mpInfo.offset != Cursor.offset))
    {
        const int32 linesDiff  = static_cast<int32>(mpInfo.lines) - Cursor.lineInView;
        const int32 offsetDiff = static_cast<int32>(mpInfo.offset) - Cursor.offset;
        Key key                = Key::None;
        if (overriden_keys.is_alt_down)
            key = key | Key::Alt;
        if (overriden_keys.is_ctrl_down)
            key = key | Key::Ctrl;
        MoveTo(offsetDiff, linesDiff, key, true);
        return true;
    }
    return false;
}

bool Instance::OnMouseWheel(int, int, AppCUI::Input::MouseWheel direction)
{
    switch (direction)
    {
    case MouseWheel::Up:
        return OnKeyEvent(Key::Up | Key::Ctrl, false);
    case MouseWheel::Down:
        return OnKeyEvent(Key::Down | Key::Ctrl, false);
    case MouseWheel::Left:
        return OnKeyEvent(Key::PageUp, false);
    case MouseWheel::Right:
        return OnKeyEvent(Key::PageDown, false);
    }

    return false;
}

bool Instance::OnKeyEvent(AppCUI::Input::Key keyCode, char16 charCode)
{
    bool select = ((keyCode & Key::Shift) != Key::None);
    if (select)
        keyCode = static_cast<Key>((uint32) keyCode - (uint32) Key::Shift);

    switch (keyCode)
    {
    case Key::Down:
        if (Cursor.startViewLine + Cursor.lineInView + 1 <= Layout.totalLinesSize)
            MoveTo(0, 1, keyCode, select);
        return true;
    case Key::Up:
        if (Cursor.startViewLine + Cursor.lineInView > 0)
            MoveTo(0, -1, keyCode, select);
        else
            MoveTo(-static_cast<int32>(Cursor.offset), 0, keyCode, select);
        return true;
    case Key::Left:
        if (this->Cursor.offset > 0)
            MoveTo(-1, 0, keyCode, select);
        return true;
    case Key::Right:
        if (this->Cursor.offset < this->Layout.textSize)
            MoveTo(1, 0, keyCode, select);
        return true;
    case Key::PageDown:
        if (Cursor.startViewLine + Cursor.lineInView + this->Layout.visibleRows <= Layout.totalLinesSize)
            MoveTo(0, this->Layout.visibleRows, keyCode, select);
        else
            MoveTo(0, Layout.totalLinesSize - Cursor.startViewLine - Cursor.lineInView, keyCode, select);
        return true;
    case Key::PageUp:
        if (Cursor.startViewLine + Cursor.lineInView >= this->Layout.visibleRows)
            MoveTo(0, -static_cast<int32>(this->Layout.visibleRows), keyCode, select);
        else
            MoveTo(0, -static_cast<int32>(Cursor.startViewLine + Cursor.lineInView), keyCode, select);
        return true;
    case Key::Home:
        MoveTo(-static_cast<int32>(Cursor.offset), 0, keyCode, select);
        return true;
    case Key::End:
        MoveTo(this->Layout.textSize - 1 - Cursor.offset, select);
        return true;
    case Key::Ctrl | Key::Up:
        if (this->Cursor.lineInView + this->Cursor.startViewLine > 0)
            MoveScrollTo(0, -1);
        /*else
            MoveScrollTo(0, -(this->Layout.textSize - this->Cursor.offset));*/
        return true;
    case Key::Ctrl | Key::Down:
        if (Cursor.startViewLine + Cursor.lineInView + 1 < Layout.totalLinesSize)
            MoveScrollTo(0, 1);
        return true;
    case Key::Ctrl | Key::Left:
        if (this->Cursor.offset >= 1)
            MoveScrollTo(-1, 0);
        return true;
    case Key::Ctrl | Key::Right:
        if (this->Cursor.offset < Layout.textSize)
            MoveScrollTo(1, 0);
        return true;
    case Key::Delete:
        RemoveComment();
        return true;
    case Key::Space:
        ProcessSpaceKey();
        return true;
    case Key::Enter:
        OpenCurrentSelection();
        return true;
    }
    if (charCode == ';')
    {
        AddComment();
        return true;
    }

    return ViewControl::OnKeyEvent(select ? (keyCode | Key::Shift) : keyCode, charCode);
}

bool Instance::OnUpdateCommandBar(AppCUI::Application::CommandBar& commandBar)
{
    const AppCUI::Utils::ConstString ShowFileContentText = config.ShowFileContent ? "ShowFileContent" : "HideFileContent";
    commandBar.SetCommand(config.Keys.AddNewType, "AddNewType", COMMAND_ADD_NEW_TYPE);
    commandBar.SetCommand(config.Keys.ShowFileContentKey, ShowFileContentText, COMMAND_ADD_SHOW_FILE_CONTENT);
    commandBar.SetCommand(config.Keys.ExportAsmToFile, "Export asm file", COMMAND_EXPORT_ASM_FILE);
    commandBar.SetCommand(config.Keys.JumpBack, "Jump back", COMMAND_JUMP_BACK);
    commandBar.SetCommand(config.Keys.JumpForward, "Jump forward", COMMAND_JUMP_FORWARD);
    commandBar.SetCommand(config.Keys.DissasmGotoEntrypoint, "Entry point", COMMAND_DISSAM_GOTO_ENTRYPOINT);

    // TODO: remove these
    const char* ctrl_message = overriden_keys.is_ctrl_down ? "CTRL DOWN" : "CTRL UP";
    commandBar.SetCommand(Key::Ctrl | Key::Z, ctrl_message, COMMAND_CTRL_CLICK);

    // TODO: remove these
    const char* alt_message = overriden_keys.is_alt_down ? "ALT DOWN" : "ALT UP";
    commandBar.SetCommand(Key::Ctrl | Key::X, alt_message, COMMAND_ALT_CLICK);
    return false;
}

bool Instance::OnEvent(Reference<Control>, Event eventType, int ID)
{
    if (eventType == Event::Command)
    {
        switch (ID)
        {
        case COMMAND_ADD_NEW_TYPE:
            Dialogs::MessageBox::ShowNotification("Info", "OK!");
            return true;
        case COMMAND_ADD_SHOW_FILE_CONTENT:
            config.ShowFileContent = !config.ShowFileContent;
            this->RecomputeDissasmZones();
            return true;
        case RIGHT_CLICK_MENU_CMD_COLLAPSE:
            AddNewCollapsibleZone();
            return true;
        case RIGHT_CLICK_ADD_COMMENT:
            AddComment();
            return true;
        case RIGHT_CLICK_REMOVE_COMMENT:
            RemoveComment();
            return true;
        case COMMAND_EXPORT_ASM_FILE:
            CommandExportAsmFile();
            return true;
        case RIGHT_CLICK_DISSASM_ADD_ZONE:
            CommandDissasmAddZone();
            return true;
        case RIGHT_CLICK_DISSASM_REMOVE_ZONE:
            CommandDissasmRemoveZone();
            return true;
        case COMMAND_JUMP_BACK:
        {
            if (const auto [canJump, location] = jumps_holder.JumpBack(); canJump)
                Cursor.restorePosition(location);
            return true;
        }
        case COMMAND_JUMP_FORWARD:
        {
            if (const auto [canJump, location] = jumps_holder.JumpFront(); canJump)
                Cursor.restorePosition(location);
            return true;
        }
        case COMMAND_DISSAM_GOTO_ENTRYPOINT:
        {
            ProcessSpaceKey(true);
            return true;
        }
        case COMMAND_CTRL_CLICK:
        {
            overriden_keys.is_ctrl_down = !overriden_keys.is_ctrl_down;
            return true;
        }
        case COMMAND_ALT_CLICK:
        {
            overriden_keys.is_alt_down = !overriden_keys.is_alt_down;
            return true;
        }
        default:
            return false;
        }
    }

    return false;
}
