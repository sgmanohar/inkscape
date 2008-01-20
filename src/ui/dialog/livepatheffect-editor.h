/**
 * \brief LivePathEffect dialog
 *
 * Author:
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *
 * Copyright (C) 2007 Author
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_LIVE_PATH_EFFECT_H
#define INKSCAPE_UI_DIALOG_LIVE_PATH_EFFECT_H

#include "ui/widget/panel.h"
#include "ui/widget/button.h"

#include <gtkmm/label.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/frame.h>
#include <gtkmm/tooltips.h>
#include "ui/widget/combo-enums.h"
#include "live_effects/effect.h"

class SPDesktop;

namespace Inkscape {

namespace UI {
namespace Dialog {

class LivePathEffectEditor : public UI::Widget::Panel {
public:
    LivePathEffectEditor();
    virtual ~LivePathEffectEditor();

    static LivePathEffectEditor &getInstance() { return *new LivePathEffectEditor(); }

    void onSelectionChanged(Inkscape::Selection *sel);
    void setDesktop(SPDesktop *desktop);

private:
    sigc::connection selection_changed_connection;
    sigc::connection selection_modified_connection;

    void set_sensitize_all(bool sensitive);

    void showParams(LivePathEffect::Effect* effect);
    void showText(Glib::ustring const &str);

    // callback methods for buttons on grids page.
    void onApply();
    void onRemove();

    Inkscape::UI::Widget::ComboBoxEnum<LivePathEffect::EffectType> combo_effecttype;
    Inkscape::UI::Widget::Button button_apply;
    Inkscape::UI::Widget::Button button_remove;
    Gtk::Widget * effectwidget;
    Gtk::Label explain_label;
    Gtk::Frame effectapplication_frame;
    Gtk::Frame effectcontrol_frame;
    Gtk::HBox effectapplication_hbox;
    Gtk::VBox effectcontrol_vbox;
    Gtk::Tooltips tooltips;

    SPDesktop * current_desktop;

    LivePathEffect::Effect* currect_effect;

    LivePathEffectEditor(LivePathEffectEditor const &d);
    LivePathEffectEditor& operator=(LivePathEffectEditor const &d);
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_LIVE_PATH_EFFECT_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
