#include "material-editor.hpp"

int main()
{
    ProjectEditor editor{};
    while (true)
    {
        editor.update();
    }
    ImGui::SFML::Shutdown(); 
}
