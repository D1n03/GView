#include "LexicalViewer.hpp"

namespace GView::View::LexicalViewer::StringOperationsPlugins
{
void Reverse(TextEditor& editor, uint32 start, uint32 end)
{
    if (end == 0)
        return;
    end--;
    while (start < end)
    {
        std::swap(editor[start], editor[end]);
        start++;
        end--;
    }
}
void UpperCase(TextEditor& editor, uint32 start, uint32 end)
{
    for (auto index = start; index < end; index++)
        if ((editor[index] >= 'a') && (editor[index] <= 'z'))
            editor[index] -= 32;
}
void LowerCase(TextEditor& editor, uint32 start, uint32 end)
{
    for (auto index = start; index < end; index++)
        if ((editor[index] >= 'A') && (editor[index] <= 'Z'))
            editor[index] |= 0x20;
}
void RemoveUnnecesaryWhiteSpaces(TextEditor& editor, uint32 start, uint32 end)
{
    auto len = editor.Len();
    auto pos = 0;
    while (pos < len)
    {
        if ((editor[pos] == ' ') || (editor[pos] == '\t'))
        {
            // check to see if there are multiple ones
            auto next = pos;
            while ((next < len) && ((editor[next] == ' ') || (editor[next] == '\t')))
                next++;
            if (next-pos>=2)
            {
                editor.Replace(pos, next - pos, " ");
                len = editor.Len();                
            }
        }
        if ((editor[pos] == '\n') || (editor[pos] == '\r'))
        {
            // check to see if there are multiple ones
            auto next = pos;
            while ((next < len) && ((editor[next] == '\n') || (editor[next] == '\r')))
                next++;
            if (next - pos >= 2)
            {
                editor.Replace(pos, next - pos, "\n");
                len = editor.Len();
            }
        }
        pos++;
    }
}
} // namespace GView::View::LexicalViewer::StringOperationsPlugins