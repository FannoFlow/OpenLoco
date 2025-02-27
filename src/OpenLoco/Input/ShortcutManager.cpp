#include "ShortcutManager.h"
#include "../CompanyManager.h"
#include "../GameCommands/GameCommands.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../S5/S5.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Ui/WindowManager.h"
#include <array>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;

namespace OpenLoco::Input::ShortcutManager
{
    static constexpr size_t _count = 35;

    static void closeTopmostWindow();
    static void closeAllFloatingWindows();
    static void cancelConstructionMode();
    static void pauseUnpauseGame();
    static void zoomViewOut();
    static void zoomViewIn();
    static void rotateView();
    static void rotateConstructionObject();
    static void toggleUndergroundView();
    static void toggleHideForegroundTracks();
    static void toggleHideForegroundScenery();
    static void toggleHeightMarksOnLand();
    static void toggleHeightMarksOnTracks();
    static void toggleDirArrowsOnTracks();
    static void adjustLand();
    static void adjustWater();
    static void plantTrees();
    static void bulldozeArea();
    static void buildTracks();
    static void buildRoads();
    static void buildAirports();
    static void buildShipPorts();
    static void buildNewVehicles();
    static void showVehiclesList();
    static void showStationsList();
    static void showTownsList();
    static void showIndustriesList();
    static void showMap();
    static void showCompaniesList();
    static void showCompanyInformation();
    static void showFinances();
    static void showAnnouncementsList();
    static void makeScreenshot();
    static void toggleLastAnnouncement();
    static void sendMessage();

    static std::array<void (*)(), _count> _shortcuts = { {
        closeTopmostWindow,
        closeAllFloatingWindows,
        cancelConstructionMode,
        pauseUnpauseGame,
        zoomViewOut,
        zoomViewIn,
        rotateView,
        rotateConstructionObject,
        toggleUndergroundView,
        toggleHideForegroundTracks,
        toggleHideForegroundScenery,
        toggleHeightMarksOnLand,
        toggleHeightMarksOnTracks,
        toggleDirArrowsOnTracks,
        adjustLand,
        adjustWater,
        plantTrees,
        bulldozeArea,
        buildTracks,
        buildRoads,
        buildAirports,
        buildShipPorts,
        buildNewVehicles,
        showVehiclesList,
        showStationsList,
        showTownsList,
        showIndustriesList,
        showMap,
        showCompaniesList,
        showCompanyInformation,
        showFinances,
        showAnnouncementsList,
        makeScreenshot,
        toggleLastAnnouncement,
        sendMessage,
    } };

    static std::array<string_id, _count> _shortcutNames = { {
        StringIds::shortcut_close_topmost_window,
        StringIds::shortcut_close_all_floating_windows,
        StringIds::shortcut_cancel_construction_mode,
        StringIds::shortcut_pause_unpause_game,
        StringIds::shortcut_zoom_view_out,
        StringIds::shortcut_zoom_view_in,
        StringIds::shortcut_rotate_view,
        StringIds::shortcut_rotate_construction_object,
        StringIds::shortcut_toggle_underground_view,
        StringIds::shortcut_toggle_hide_foreground_tracks,
        StringIds::shortcut_toggle_hide_foreground_scenery,
        StringIds::shortcut_toggle_height_marks_on_land,
        StringIds::shortcut_toggle_height_marks_on_tracks,
        StringIds::shortcut_toggle_dir_arrows_on_tracks,
        StringIds::shortcut_adjust_land,
        StringIds::shortcut_adjust_water,
        StringIds::shortcut_plant_trees,
        StringIds::shortcut_bulldoze_area,
        StringIds::shortcut_build_tracks,
        StringIds::shortcut_build_roads,
        StringIds::shortcut_build_airports,
        StringIds::shortcut_build_ship_ports,
        StringIds::shortcut_build_new_vehicles,
        StringIds::shortcut_show_vehicles_list,
        StringIds::shortcut_show_stations_list,
        StringIds::shortcut_show_towns_list,
        StringIds::shortcut_show_industries_list,
        StringIds::shortcut_show_map,
        StringIds::shortcut_show_companies_list,
        StringIds::shortcut_show_company_information,
        StringIds::shortcut_show_finances,
        StringIds::shortcut_show_announcements_list,
        StringIds::shortcut_screenshot,
        StringIds::shortcut_toggle_last_announcement,
        StringIds::shortcut_send_message,
    } };

    size_t count()
    {
        return _count;
    }

    void execute(Shortcut s)
    {
        _shortcuts[s]();
    }

    string_id getName(Shortcut s)
    {
        return _shortcutNames[s];
    }

    // 0x004BF089
    static void closeTopmostWindow()
    {
        WindowManager::closeTopmost();
    }

    // 004BF0B6
    static void closeAllFloatingWindows()
    {
        WindowManager::closeAllFloatingWindows();
    }

    // 0x4BF0BC
    static void cancelConstructionMode()
    {
        call(0x004BF0BC);
    }

    // 0x4BF0E6
    static void pauseUnpauseGame()
    {
        if (isEditorMode())
            return;

        GameCommands::do_20();
    }

    // 0x004BF0FE
    static void zoomViewOut()
    {
        Window* main = WindowManager::getMainWindow();
        if (main == nullptr)
            return;

        main->viewportZoomOut(false);
        TownManager::updateLabels();
        StationManager::updateLabels();
    }

    // 0x004BF115
    static void zoomViewIn()
    {
        Window* main = WindowManager::getMainWindow();
        if (main == nullptr)
            return;

        main->viewportZoomIn(false);
        TownManager::updateLabels();
        StationManager::updateLabels();
    }

    // 0x004BF12C
    static void rotateView()
    {
        Window* main = WindowManager::getMainWindow();
        if (main == nullptr)
            return;

        main->viewportRotateRight();
        TownManager::updateLabels();
        StationManager::updateLabels();
        Windows::MapWindow::centerOnViewPoint();
    }

    // 0x004BF148
    static void rotateConstructionObject()
    {
        if (Windows::Vehicle::rotate())
            return;

        auto window = WindowManager::find(WindowType::terraform);
        if (window != nullptr)
        {
            if (Ui::Windows::Terraform::rotate(window))
                return;
        }

        // 0x004A5D48
        window = WindowManager::find(WindowType::construction);
        if (window != nullptr)
        {
            if (Ui::Windows::Construction::rotate(window))
                return;
        }

        window = WindowManager::find(WindowType::townList);
        if (window != nullptr)
        {
            if (Ui::Windows::TownList::rotate(window))
                return;
        }
    }

    // 0x004BF18A
    static void toggleUndergroundView()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::underground_view;
        window->invalidate();
    }

    // 0x004BF194
    static void toggleHideForegroundTracks()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::hide_foreground_tracks_roads;
        window->invalidate();
    }

    // 0x004BF19E
    static void toggleHideForegroundScenery()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::hide_foreground_scenery_buildings;
        window->invalidate();
    }

    // 0x004BF1A8
    static void toggleHeightMarksOnLand()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::height_marks_on_land;
        window->invalidate();
    }

    // 0x004BF1B2
    static void toggleHeightMarksOnTracks()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::height_marks_on_tracks_roads;
        window->invalidate();
    }

    // 0x004BF1BC
    static void toggleDirArrowsOnTracks()
    {
        auto window = WindowManager::getMainWindow();
        if (window == nullptr)
            return;

        auto viewport = WindowManager::getMainWindow()->viewports[0];
        viewport->flags ^= ViewportFlags::one_way_direction_arrows;
        window->invalidate();
    }

    // 0x004BF1C6
    static void adjustLand()
    {
        if (isEditorMode() && S5::getOptions().editorStep == 0)
            return;

        Windows::Terraform::openAdjustLand();
    }

    // 0x004BF1E1
    static void adjustWater()
    {
        if (isEditorMode() && S5::getOptions().editorStep == 0)
            return;

        Windows::Terraform::openAdjustWater();
    }

    // 0x004BF1FC
    static void plantTrees()
    {
        if (isEditorMode() && S5::getOptions().editorStep == 0)
            return;

        Windows::Terraform::openPlantTrees();
    }

    // 0x004BF217
    static void bulldozeArea()
    {
        if (isEditorMode() && S5::getOptions().editorStep == 0)
            return;

        Windows::Terraform::openClearArea();
    }

    // 0x004BF232
    static void buildTracks()
    {
        if (isEditorMode())
            return;

        loco_global<uint8_t, 0x00525FAA> last_railroad_option;
        if (last_railroad_option == 0xFF)
            return;

        Windows::Construction::openWithFlags(*last_railroad_option);
    }

    // 0x004BF24F
    static void buildRoads()
    {
        if (isEditorMode() && S5::getOptions().editorStep == 0)
            return;

        loco_global<uint8_t, 0x00525FAB> last_road_option;
        if (last_road_option == 0xFF)
            return;

        Windows::Construction::openWithFlags(*last_road_option);
    }

    // 0x004BF276
    static void buildAirports()
    {
        if (isEditorMode())
            return;

        loco_global<uint8_t, 0x00525FAC> have_airports;
        if (have_airports == 0xFF)
            return;

        Windows::Construction::openWithFlags(1 << 31);
    }

    // 0x004BF295
    static void buildShipPorts()
    {
        if (isEditorMode())
            return;

        loco_global<uint8_t, 0x00525FAD> have_ship_ports;
        if (have_ship_ports == 0xFF)
            return;

        Windows::Construction::openWithFlags(1 << 30);
    }

    // 0x004BF2B4
    static void buildNewVehicles()
    {
        if (isEditorMode())
            return;

        loco_global<uint8_t, 0x0052622C> last_build_vehicles_option;
        if (last_build_vehicles_option == 0xFF)
            return;

        Windows::BuildVehicle::open(last_build_vehicles_option, 1 << 31);
    }

    // 0x004BF2D1
    static void showVehiclesList()
    {
        if (isEditorMode())
            return;

        loco_global<VehicleType, 0x00525FAF> last_vehicles_option;
        Windows::VehicleList::open(CompanyManager::getControllingId(), *last_vehicles_option);
    }

    // 0x004BF2F0
    static void showStationsList()
    {
        if (isEditorMode())
            return;

        Windows::StationList::open(CompanyManager::getControllingId(), 0);
    }

    // 0x004BF308
    static void showTownsList()
    {
        if (isEditorMode() && S5::getOptions().editorStep == 0)
            return;

        Windows::TownList::open();
    }

    // 0x004BF323
    static void showIndustriesList()
    {
        if (isEditorMode() && S5::getOptions().editorStep == 0)
            return;

        Windows::IndustryList::open();
    }

    // 0x004BF33E
    static void showMap()
    {
        if (isEditorMode() && S5::getOptions().editorStep == 0)
            return;

        Windows::MapWindow::open();
    }

    // 0x004BF359
    static void showCompaniesList()
    {
        if (isEditorMode())
            return;

        Ui::Windows::CompanyList::open();
    }

    // 0x004BF36A
    static void showCompanyInformation()
    {
        if (isEditorMode())
            return;

        Ui::Windows::CompanyWindow::open(CompanyManager::getControllingId());
    }

    // 0x004BF382
    static void showFinances()
    {
        if (isEditorMode())
            return;

        Windows::CompanyWindow::openFinances(CompanyManager::getControllingId());
    }

    // 0x004BF39A
    static void showAnnouncementsList()
    {
        if (isEditorMode())
            return;

        Windows::MessageWindow::open();
    }

    // 0x004BF3AB
    static void makeScreenshot()
    {
        call(0x004BF3AB);
    }

    // 0x004BF3B3
    static void toggleLastAnnouncement()
    {
        if (isEditorMode())
            return;

        auto window = WindowManager::find(WindowType::news);
        if (window)
        {
            Windows::NewsWindow::close(window);
        }
        else
        {
            Windows::NewsWindow::openLastMessage();
        }
    }

    // 0x004BF3DC
    static void sendMessage()
    {
        call(0x004BF3DC);
    }
}
