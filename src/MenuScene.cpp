/*=============================================================================
Leo Tamminen
shophorn @ internet

Description of menu scene.
=============================================================================*/

struct MenuScene
{
    ArenaArray<Rectangle> buttons;
    s32 selectedIndex;
    MaterialHandle material;

    static void
    load(   void * guiPtr,
            MemoryArena * persistentMemory,
            MemoryArena * transientMemory,
            platform::Graphics*,
            platform::Window*,
            platform::Functions*);

    static MenuResult 
    update( void *                      guiPtr,
            game::Input *               input,
            platform::Graphics*,
            platform::Window*,
            platform::Functions*);
};

SceneInfo get_menu_scene_info()
{
    return make_scene_info( get_size_of<MenuScene>,
                            MenuScene::load,
                            MenuScene::update);
}

///////////////////////////////////////
///             LOAD                ///
///////////////////////////////////////
void
MenuScene::load(void * guiPtr,
                            MemoryArena * persistentMemory,
                            MemoryArena * transientMemory,
                            platform::Graphics * graphics,
                            platform::Window*,
                            platform::Functions* functions)
{
    auto * gui = reinterpret_cast<MenuScene*>(guiPtr);

    auto textureAsset   = load_texture_asset("assets/textures/texture.jpg", transientMemory);
    auto texture        = functions->push_texture(graphics, &textureAsset);
    gui->material       = functions->push_gui_material(graphics, texture);

    gui->buttons = push_array(persistentMemory,{
                                Rectangle {810, 380, 300, 100},
                                Rectangle {810, 500, 300, 100},
                                Rectangle {810, 620, 300, 100}
                            });

    gui->selectedIndex = 0;
}

///////////////////////////////////////
///             UDPATE              ///
///////////////////////////////////////
MenuResult
MenuScene::update(  void *                      guiPtr,
                    game::Input *               input,
                    platform::Graphics * graphics,
                    platform::Window*,
                    platform::Functions * functions)
{
    auto * gui = reinterpret_cast<MenuScene*>(guiPtr);

    if (is_clicked(input->down))
    {
        gui->selectedIndex += 1;
        gui->selectedIndex %= gui->buttons.count();
    }

    if (is_clicked(input->up))
    {
        gui->selectedIndex -= 1;
        if (gui->selectedIndex < 0)
        {
            gui->selectedIndex = gui->buttons.count() - 1;
        }
    }


    for(int i = 0; i < gui->buttons.count(); ++i)
    {
        Rectangle rect = gui->buttons[i];
        float4 color = {1,1,1,1};

        if (i == gui->selectedIndex)
        {
            rect    = scale_rectangle(rect, {1.2f, 1.2f});
            color   = {1, 0.4f, 0.4f, 1}; 
        }
        functions->draw_gui(graphics, rect.position, rect.size, gui->material, color);
    }
    
    MenuResult result = MENU_NONE;
    if (is_clicked(input->confirm))
    {
        switch (gui->selectedIndex)
        {
            case 0: 
                std::cout << "Load 3d Scene\n";
                result = MENU_LOADLEVEL_3D;
                break;

            case 1: 
                std::cout << "Load 2d Scene\n";
                result = MENU_LOADLEVEL_2D;
                break;

            case 2:
                std::cout << "Exit\n";
                result = MENU_EXIT;
                break;
        }
    }

    return result;
}