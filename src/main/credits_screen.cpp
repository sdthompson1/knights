/*
 * credits_screen.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Knights is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Knights.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "misc.hpp"

#include "credits_screen.hpp"
#include "gui_text_wrap.hpp"
#include "knights_app.hpp"
#include "rstream.hpp"
#include "title_screen.hpp"

// coercri
#include "gcn/cg_font.hpp"

#include "guichan.hpp"

#include "boost/scoped_ptr.hpp"
using namespace boost;

class CreditsScreenImpl : public gcn::MouseListener, public gcn::WidgetListener {
public:
    explicit CreditsScreenImpl(KnightsApp &kg, gcn::Gui &gui, const std::string &filename, int wlimit_);
    void mouseClicked(gcn::MouseEvent &event);
    void widgetResized(const gcn::Event &event);
    
private:
    KnightsApp &knights_app;
    scoped_ptr<GuiTextWrap> text_wrap;
    scoped_ptr<gcn::ScrollArea> scroll_area;
    scoped_ptr<gcn::Container> container;
    int wlimit;
};

CreditsScreenImpl::CreditsScreenImpl(KnightsApp &app, gcn::Gui &gui, const std::string &filename, int wlimit_)
    : knights_app(app), wlimit(wlimit_)
{
    RStream str(filename.c_str());
    std::string credits;
    while (str) {
        char c = 0;
        str.get(c);
        if (c != '\r') credits += c;
    }
    
    text_wrap.reset(new GuiTextWrap);
    text_wrap->setForegroundColor(gcn::Color(255,255,255));
    text_wrap->setRich(true);
    text_wrap->setText(credits);

    // Use an intermediate container between the TextWrap and the ScrollArea.
    // This allows us to make the TextWrap smaller than the whole screen while keeping
    // the ScrollArea the same size as the whole screen. (Without this, mouse clicks
    // outside the ScrollArea won't work.)
    container.reset(new gcn::Container);
    container->setOpaque(false);
    container->add(text_wrap.get(), 0, 0);
    
    scroll_area.reset(new gcn::ScrollArea);
    scroll_area->setContent(container.get());
    scroll_area->addWidgetListener(this);
    scroll_area->setBackgroundColor(gcn::Color(0,0,0));
    scroll_area->setOpaque(true);
    scroll_area->setScrollbarWidth(16);
    scroll_area->addMouseListener(this);
    gui.setTop(scroll_area.get());
}

void CreditsScreenImpl::mouseClicked(gcn::MouseEvent &event)
{
    if (event.getX() < scroll_area->getChildrenArea().width) {
        // click is not within the actual scrollbar
        auto_ptr<Screen> scr(new TitleScreen);
        knights_app.requestScreenChange(scr);
    }
}

void CreditsScreenImpl::widgetResized(const gcn::Event &event)
{
    const int max_w = wlimit * text_wrap->getFont()->getWidth("M");
    const int avail_w = scroll_area->getWidth() - scroll_area->getScrollbarWidth();
    
    const int w = std::max(30, std::min(max_w, avail_w));
    
    text_wrap->setWidth(w);
    text_wrap->adjustHeight();

    container->setWidth(avail_w);
    container->setHeight(std::max(scroll_area->getHeight(), text_wrap->getHeight()));
    
    // centre text_wrap horizontally within the container
    if (w < avail_w) {
        text_wrap->setX((avail_w - w) / 2);
    }
    
    // ... and place slightly above centre vertically
    if (text_wrap->getHeight() < scroll_area->getHeight()) {
        text_wrap->setY((scroll_area->getHeight() - text_wrap->getHeight())*2/5);
    }
    
    scroll_area->logic();
}

bool CreditsScreen::start(KnightsApp &knights_app, shared_ptr<Coercri::Window> w, gcn::Gui &gui)
{
    pimpl.reset(new CreditsScreenImpl(knights_app, gui, filename, width));
    return true;
}
