#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../Date.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Intro.h"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Message.h"
#include "../MessageManager.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/Dropdown.h"
#include "../Ui/ScrollView.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::MessageWindow
{
    static loco_global<Ui::WindowNumber_t, 0x00523390> _toolWindowNumber;
    static loco_global<Ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<CompanyId_t, 0x00525E3C> _playerCompany;
    static loco_global<uint16_t, 0x005271CE> _messageCount;
    static loco_global<uint16_t, 0x005271D0> _activeMessageIndex;

    namespace Common
    {
        enum widx
        {
            frame = 0,
            caption = 1,
            close_button = 2,
            panel = 3,
            tab_messages,
            tab_settings,
        };

        const uint64_t enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_messages) | (1 << widx::tab_settings);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                                           \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, WidgetType::frame, WindowColour::primary),                                                          \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, WidgetType::caption_24, WindowColour::primary, windowCaptionId),                                     \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, WidgetType::wt_9, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { 366, 175 }, WidgetType::panel, WindowColour::secondary),                                                                  \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::wt_8, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_recent_messages),             \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::wt_8, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_message_options)

        static WindowEventList _events;

        static void prepareDraw(Window* self);
        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
        static void onUpdate(Window* self);
        static void drawTabs(Window* self, Gfx::Context* context);
        static void initEvents();
    }

    namespace Messages
    {
        static const Ui::Size minWindowSize = { 366, 217 };
        static const Ui::Size maxWindowSize = { 366, 1200 };
        static int8_t messageHeight = 39;

        enum widx
        {
            scrollview = 6,
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << scrollview);

        Widget widgets[] = {
            commonWidgets(366, 217, StringIds::title_messages),
            makeWidget({ 3, 45 }, { 360, 146 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x0042A6F5
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_messages:
                case Common::widx::tab_settings:
                    Common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x0042A95A
        static void onResize(Window* self)
        {
            auto scrollview = self->widgets[widx::scrollview];
            auto scrollarea = self->scroll_areas[0];

            auto y = scrollarea.contentHeight - scrollview.height() - 1;
            y = std::max(0, y);

            if (y < scrollarea.contentOffsetY)
            {
                scrollarea.contentOffsetY = y;
                Ui::ScrollView::updateThumbs(self, widx::scrollview);
                self->invalidate();
            }
        }

        // 0x0042A847
        static void event_08(Window* self)
        {
            self->flags |= WindowFlags::not_scroll_view;
        }

        // 0x0042A84F
        static void event_09(Window* self)
        {
            if (!(self->flags & WindowFlags::not_scroll_view))
                return;

            if (self->row_hover == -1)
                return;

            self->row_hover = -1;
            self->invalidate();
        }

        // 0x0042A871
        static void getScrollSize(Window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = _messageCount * messageHeight;
        }

        // 0x0042A8B9
        static void scrollMouseDown(Ui::Window* self, int16_t x, int16_t y, uint8_t scrollIndex)
        {
            auto messageIndex = y / messageHeight;

            if (messageIndex >= _messageCount)
                return;

            if (_activeMessageIndex != 0xFFFF)
            {
                auto message = MessageManager::get(_activeMessageIndex);
                if (message->var_C8 != 0xFFFF)
                {
                    if (message->var_C8 & (1 << 15))
                        message->var_C8 = 0xFFFF;
                }
            }

            _activeMessageIndex = 0xFFFF;
            WindowManager::close(WindowType::news, 0);

            auto message = MessageManager::get(messageIndex);
            message->var_C8 = (1 << 15) | (1 << 0);

            NewsWindow::open(messageIndex);

            int32_t pan = self->width / 2 + self->x;
            Audio::playSound(Audio::SoundId::clickDown, pan);
        }

        // 0x0042A87C
        static void scrollMouseOver(Ui::Window* self, int16_t x, int16_t y, uint8_t scrollIndex)
        {
            self->flags &= ~(WindowFlags::not_scroll_view);

            auto messageIndex = y / messageHeight;
            auto messageId = 0xFFFF;

            if (messageIndex < _messageCount)
                messageId = messageIndex;

            if (self->row_hover != messageId)
            {
                self->row_hover = messageId;
                self->invalidate();
            }
        }

        // 0x0042A70C
        static std::optional<FormatArguments> tooltip(Ui::Window* self, WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_message_list);
            return args;
        }

        // 0x0042A545
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            self->widgets[widx::scrollview].right = self->width - 4;
            self->widgets[widx::scrollview].bottom = self->height - 14;
        }

        // 0x0042A5CC
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);
        }

        // 0x0042A5D7
        static void drawScroll(Ui::Window& self, Gfx::Context& context, const uint32_t scrollIndex)
        {
            auto colour = Colour::getShade(self.getColour(WindowColour::secondary), 4);

            Gfx::clearSingle(context, colour);

            auto height = 0;
            for (auto i = 0; i < _messageCount; i++)
            {
                if (height + messageHeight <= context.y)
                {
                    height += messageHeight;
                    continue;
                }

                if (height >= context.y + context.height)
                {
                    height += messageHeight;
                    continue;
                }
                auto message = MessageManager::get(i);
                char* buffer = message->messageString;
                auto str = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

                const size_t bufferLength = 512;
                strncpy(str, buffer, bufferLength);

                auto stringId = StringIds::black_stringid;

                if (self.row_hover == i)
                {
                    Gfx::drawRect(context, 0, height, self.width, 38, (1 << 25) | PaletteIndex::index_30);
                    stringId = StringIds::wcolour2_stringid;
                }

                {
                    auto args = FormatArguments();
                    args.push(StringIds::tiny_font_date);
                    args.push(message->date);

                    Gfx::drawString_494B3F(context, 0, height, Colour::black, stringId, &args);
                }
                {
                    auto args = FormatArguments();
                    args.push(StringIds::buffer_2039);

                    auto width = self.widgets[widx::scrollview].width() - 14;
                    Gfx::drawString_495224(context, 0, height + 6, width, Colour::black, stringId, &args);
                    height += messageHeight;
                }
            }
        }

        // 0x0042A7B9
        static void tabReset(Window* self)
        {
            self->min_width = minWindowSize.width;
            self->min_height = minWindowSize.height;
            self->max_width = maxWindowSize.width;
            self->max_height = maxWindowSize.height;
            self->width = minWindowSize.width;
            self->height = minWindowSize.height;
            self->row_hover = -1;
        }

        static void initEvents()
        {
            events.on_mouse_up = onMouseUp;
            events.on_resize = onResize;
            events.on_update = Common::onUpdate;
            events.event_08 = event_08;
            events.event_09 = event_09;
            events.get_scroll_size = getScrollSize;
            events.scroll_mouse_down = scrollMouseDown;
            events.scroll_mouse_over = scrollMouseOver;
            events.tooltip = tooltip;
            events.prepare_draw = prepareDraw;
            events.draw = draw;
            events.draw_scroll = drawScroll;
        }
    }

    // 0x0042A3FF
    void open()
    {
        auto window = WindowManager::bringToFront(WindowType::messages);

        if (window != nullptr)
        {
            if (Input::isToolActive(window->type, window->number))
            {
                Input::toolCancel();
                window = WindowManager::bringToFront(WindowType::messages);
            }
        }

        if (window == nullptr)
        {
            int16_t y = 29;
            int16_t x = Ui::width() - 366;
            Ui::Point origin = { x, y };

            window = WindowManager::createWindow(
                WindowType::messages,
                origin,
                { 366, 217 },
                WindowFlags::flag_11,
                &Messages::events);

            window->enabled_widgets = Messages::enabledWidgets;
            window->number = 0;
            window->current_tab = 0;
            window->frame_no = 0;
            window->row_hover = -1;
            window->disabled_widgets = 0;

            WindowManager::sub_4CEE0B(window);

            window->min_width = Messages::minWindowSize.width;
            window->min_height = Messages::minWindowSize.height;
            window->max_width = Messages::maxWindowSize.width;
            window->max_height = Messages::maxWindowSize.height;
            window->flags |= WindowFlags::resizable;

            window->owner = _playerCompany;
            auto skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::secondary, skin->colour_0A);

            window->width = Messages::minWindowSize.width;
            window->height = Messages::minWindowSize.height;
        }

        window->current_tab = 0;
        window->invalidate();

        window->widgets = Messages::widgets;
        window->enabled_widgets = Messages::enabledWidgets;
        window->holdable_widgets = 0;
        window->event_handlers = &Messages::events;
        window->disabled_widgets = 0;

        Common::initEvents();

        window->callOnResize();
        window->callPrepareDraw();
        window->initScrollWidgets();

        uint16_t scrollHeight = 0;
        window->callGetScrollSize(0, 0, &scrollHeight);

        scrollHeight -= window->widgets[Messages::widx::scrollview].height();

        if (static_cast<int16_t>(scrollHeight) < 0)
            scrollHeight = 0;

        window->scroll_areas[0].contentOffsetY = scrollHeight;

        Ui::ScrollView::updateThumbs(window, Messages::widx::scrollview);
    }

    namespace Settings
    {
        static const Ui::Size windowSize = { 366, 139 };

        enum widx
        {
            company_major_news = 6,
            company_major_news_dropdown,
            competitor_major_news,
            competitor_major_news_dropdown,
            company_minor_news,
            company_minor_news_dropdown,
            competitor_minor_news,
            competitor_minor_news_dropdown,
            general_news,
            general_news_dropdown,
            advice,
            advice_dropdown,
        };

        static constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << widx::company_major_news) | (1 << widx::company_major_news_dropdown) | (1 << widx::competitor_major_news) | (1 << widx::competitor_major_news_dropdown) | (1 << widx::company_minor_news) | (1 << widx::company_minor_news_dropdown) | (1 << widx::competitor_minor_news) | (1 << widx::competitor_minor_news_dropdown) | (1 << widx::general_news) | (1 << widx::general_news_dropdown) | (1 << widx::advice) | (1 << widx::advice_dropdown);

        Widget widgets[] = {
            commonWidgets(366, 217, StringIds::title_messages),
            makeWidget({ 236, 47 }, { 124, 12 }, WidgetType::wt_18, WindowColour::secondary),
            makeWidget({ 348, 48 }, { 11, 10 }, WidgetType::wt_11, WindowColour::secondary, StringIds::dropdown),
            makeWidget({ 236, 62 }, { 124, 12 }, WidgetType::wt_18, WindowColour::secondary),
            makeWidget({ 348, 63 }, { 11, 10 }, WidgetType::wt_11, WindowColour::secondary, StringIds::dropdown),
            makeWidget({ 236, 77 }, { 124, 12 }, WidgetType::wt_18, WindowColour::secondary),
            makeWidget({ 348, 78 }, { 11, 10 }, WidgetType::wt_11, WindowColour::secondary, StringIds::dropdown),
            makeWidget({ 236, 92 }, { 124, 12 }, WidgetType::wt_18, WindowColour::secondary),
            makeWidget({ 348, 93 }, { 11, 10 }, WidgetType::wt_11, WindowColour::secondary, StringIds::dropdown),
            makeWidget({ 236, 107 }, { 124, 12 }, WidgetType::wt_18, WindowColour::secondary),
            makeWidget({ 348, 108 }, { 11, 10 }, WidgetType::wt_11, WindowColour::secondary, StringIds::dropdown),
            makeWidget({ 236, 122 }, { 124, 12 }, WidgetType::wt_18, WindowColour::secondary),
            makeWidget({ 348, 123 }, { 11, 10 }, WidgetType::wt_11, WindowColour::secondary, StringIds::dropdown),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x0042AA84
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_messages:
                case Common::widx::tab_settings:
                    Common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x0042AA9F
        static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::company_major_news:
                case widx::company_major_news_dropdown:
                case widx::competitor_major_news:
                case widx::competitor_major_news_dropdown:
                case widx::company_minor_news:
                case widx::company_minor_news_dropdown:
                case widx::competitor_minor_news:
                case widx::competitor_minor_news_dropdown:
                case widx::general_news:
                case widx::general_news_dropdown:
                case widx::advice:
                case widx::advice_dropdown:
                {
                    auto widget = self->widgets[widgetIndex - 1];
                    auto xPos = widget.left + self->x;
                    auto yPos = widget.top + self->y;
                    auto width = widget.width() - 2;
                    auto height = widget.height() + 2;
                    auto flags = 1 << 7;

                    Dropdown::show(xPos, yPos, width, height, self->getColour(WindowColour::secondary), 3, flags);

                    Dropdown::add(0, StringIds::dropdown_stringid, StringIds::message_off);
                    Dropdown::add(1, StringIds::dropdown_stringid, StringIds::message_ticker);
                    Dropdown::add(2, StringIds::dropdown_stringid, StringIds::message_window);

                    auto dropdownIndex = Config::get().news_settings[(widgetIndex - 7) / 2];

                    Dropdown::setItemSelected(static_cast<size_t>(dropdownIndex));
                    break;
                }
            }
        }

        // 0x0042AAAC
        static void onDropdown(Window* self, Ui::WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::company_major_news:
                case widx::company_major_news_dropdown:
                case widx::competitor_major_news:
                case widx::competitor_major_news_dropdown:
                case widx::company_minor_news:
                case widx::company_minor_news_dropdown:
                case widx::competitor_minor_news:
                case widx::competitor_minor_news_dropdown:
                case widx::general_news:
                case widx::general_news_dropdown:
                case widx::advice:
                case widx::advice_dropdown:
                {
                    if (itemIndex == -1)
                        return;

                    auto dropdownIndex = (widgetIndex - 7) / 2;

                    if (static_cast<Config::NewsType>(itemIndex) != Config::get().news_settings[dropdownIndex])
                    {
                        Config::get().news_settings[dropdownIndex] = static_cast<Config::NewsType>(itemIndex);
                        Config::write();
                        Gfx::invalidateScreen();
                    }
                    break;
                }
            }
        }

        // 0x0042AA02
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);
            auto yPos = self->widgets[widx::company_major_news].top + self->y;

            const string_id newsStringIds[] = {
                StringIds::company_major_news,
                StringIds::competitor_major_news,
                StringIds::company_minor_news,
                StringIds::competitor_minor_news,
                StringIds::general_news,
                StringIds::advice,
            };

            const string_id newsDropdownStringIds[] = {
                StringIds::message_off,
                StringIds::message_ticker,
                StringIds::message_window,
            };

            for (auto i = 0; i < 6; i++)
            {
                {
                    auto args = FormatArguments();
                    args.push(newsStringIds[i]);

                    Gfx::drawString_494B3F(*context, self->x + 4, yPos, Colour::black, StringIds::wcolour2_stringid, &args);
                }

                {
                    auto xPos = self->widgets[widx::company_major_news].left + self->x + 1;
                    auto args = FormatArguments();
                    args.push(newsDropdownStringIds[static_cast<uint8_t>(Config::get().news_settings[i])]);

                    Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::black_stringid, &args);
                }
                yPos += 15;
            }
        }

        // 0x0042A7E8
        static void tabReset(Window* self)
        {
            self->min_width = windowSize.width;
            self->min_height = windowSize.height;
            self->max_width = windowSize.width;
            self->max_height = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
        }

        static void initEvents()
        {
            events.on_mouse_up = onMouseUp;
            events.on_mouse_down = onMouseDown;
            events.on_dropdown = onDropdown;
            events.on_update = Common::onUpdate;
            events.prepare_draw = Common::prepareDraw;
            events.draw = draw;
        }
    }

    namespace Common
    {
        struct TabInformation
        {
            Widget* widgets;
            const widx widgetIndex;
            WindowEventList* events;
            const uint64_t enabledWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { Messages::widgets, widx::tab_messages, &Messages::events, Messages::enabledWidgets },
            { Settings::widgets, widx::tab_settings, &Settings::events, Settings::enabledWidgets },
        };

        static void prepareDraw(Window* self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->initScrollWidgets();
            }

            // Activate the current tab..
            self->activated_widgets &= ~((1ULL << tab_messages) | (1ULL << tab_settings));
            self->activated_widgets |= (1ULL << tabInformationByTabOffset[self->current_tab].widgetIndex);

            self->widgets[Common::widx::frame].right = self->width - 1;
            self->widgets[Common::widx::frame].bottom = self->height - 1;

            self->widgets[Common::widx::panel].right = self->width - 1;
            self->widgets[Common::widx::panel].bottom = self->height - 1;

            self->widgets[Common::widx::caption].right = self->width - 2;

            self->widgets[Common::widx::close_button].left = self->width - 15;
            self->widgets[Common::widx::close_button].right = self->width - 3;
        }

        // 0x0042A716
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

            self->current_tab = widgetIndex - widx::tab_messages;
            self->frame_no = 0;
            self->flags &= ~(WindowFlags::flag_16);

            self->viewportRemove(0);

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_messages];

            self->enabled_widgets = tabInfo.enabledWidgets;
            self->holdable_widgets = 0;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;
            self->disabled_widgets = 0;

            self->invalidate();

            if (self->current_tab == widx::tab_messages - widx::tab_messages)
                Messages::tabReset(self);
            if (self->current_tab == widx::tab_settings - widx::tab_messages)
                Settings::tabReset(self);

            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x0042AB92
        static void drawTabs(Window* self, Gfx::Context* context)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Message Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::tab_messages;

                Widget::drawTab(self, context, imageId, widx::tab_messages);
            }

            // Setting Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::tab_message_settings;

                Widget::drawTab(self, context, imageId, widx::tab_settings);
            }
        }

        // 0x0042A826 and 0x0042AB6A
        static void onUpdate(Window* self)
        {
            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::messages, self->number, self->current_tab + Common::widx::tab_messages);
        }

        static void initEvents()
        {
            Messages::initEvents();
            Settings::initEvents();
        }
    }
}
