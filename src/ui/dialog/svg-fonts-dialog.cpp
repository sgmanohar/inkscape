/** @file
 * @brief SVG Fonts dialog - implementation
 */
/* Authors:
 *   Felipe C. da S. Sanches <felipe.sanches@gmail.com>
 *
 * Copyright (C) 2008 Authors
 * Released under GNU GPLv2 (or later).  Read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_SVG_FONTS

#include <gtkmm/notebook.h>
#include "svg-fonts-dialog.h"
#include <glibmm/i18n.h>
#include <string.h>
#include "document-private.h"
#include "xml/node.h"
#include "xml/repr.h"

SvgFontDrawingArea::SvgFontDrawingArea(){
	this->text = "";
	this->svgfont = NULL;
}

void SvgFontDrawingArea::set_svgfont(SvgFont* svgfont){
	this->svgfont = svgfont;
}

void SvgFontDrawingArea::set_text(Glib::ustring text){
	this->text = text;
}

void SvgFontDrawingArea::set_size(int x, int y){
    this->x = x;
    this->y = y;
    ((Gtk::Widget*) this)->set_size_request(x, y);
}

void SvgFontDrawingArea::redraw(){
	((Gtk::Widget*) this)->queue_draw();
}

bool SvgFontDrawingArea::on_expose_event (GdkEventExpose *event){
  if (this->svgfont){
    Glib::RefPtr<Gdk::Window> window = get_window();
    Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context();
    cr->set_font_face( Cairo::RefPtr<Cairo::FontFace>(new Cairo::FontFace(this->svgfont->get_font_face(), false /* does not have reference */)) );
    cr->set_font_size (this->y-20);
    cr->move_to (10, 10);
    cr->show_text (this->text.c_str());
  }
  return TRUE;
}

namespace Inkscape {
namespace UI {
namespace Dialog {

/*
Gtk::HBox* SvgFontsDialog::AttrEntry(gchar* lbl, const SPAttributeEnum attr){
    Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
    hbox->add(* Gtk::manage(new Gtk::Label(lbl)) );
    Gtk::Entry* entry = Gtk::manage(new Gtk::Entry());
    hbox->add(* entry );
    hbox->show_all();

    entry->signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::on_attr_changed));
    return hbox;
}
*/

SvgFontsDialog::AttrEntry::AttrEntry(SvgFontsDialog* d, gchar* lbl, const SPAttributeEnum attr){
    this->dialog = d;
    this->attr = attr;
    this->add(* Gtk::manage(new Gtk::Label(lbl)) );
    this->add(entry);
    this->show_all();

    entry.signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::AttrEntry::on_attr_changed));
}

void SvgFontsDialog::AttrEntry::on_attr_changed(){
	g_warning("attr entry changed: %s", this->entry.get_text().c_str());

	SPObject* o = NULL;
        for(SPObject* node = this->dialog->get_selected_spfont()->children; node; node=node->next){
            switch(this->attr){
		case SP_PROP_FONT_FAMILY:
			if (SP_IS_FONTFACE(node)){
				o = node;
				continue;
			}
			break;
		default:
			o = NULL;
	    }
        }

	const gchar* name = (const gchar*)sp_attribute_name(this->attr);
        if(name && o) {
            SP_OBJECT_REPR(o)->setAttribute((const gchar*) name, this->entry.get_text().c_str());
            o->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);

            Glib::ustring undokey = "svgfonts:";
            undokey += name;
            sp_document_maybe_done(o->document, undokey.c_str(), SP_VERB_DIALOG_SVG_FONTS,
                                   _("Set SVG Font attribute"));
        }

}

Gtk::HBox* SvgFontsDialog::AttrCombo(gchar* lbl, const SPAttributeEnum attr){
    Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
    hbox->add(* Gtk::manage(new Gtk::Label(lbl)) );
    hbox->add(* Gtk::manage(new Gtk::ComboBox()) );
    hbox->show_all();
    return hbox;
}

/*
Gtk::HBox* SvgFontsDialog::AttrSpin(gchar* lbl){
    Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
    hbox->add(* Gtk::manage(new Gtk::Label(lbl)) );
    hbox->add(* Gtk::manage(new Gtk::SpinBox()) );
    hbox->show_all();
    return hbox;
}*/

/*** SvgFontsDialog ***/

GlyphComboBox::GlyphComboBox(){
}

void GlyphComboBox::update(SPFont* spfont){
    if (spfont) {
        this->clear();
        for(SPObject* node = spfont->children; node; node=node->next){
            if (SP_IS_GLYPH(node)){
                this->append_text(((SPGlyph*)node)->unicode);
            }
        }
    }
}

void SvgFontsDialog::on_kerning_changed(){
    if (this->kerning_pair){
        this->kerning_pair->k = kerning_spin.get_value();
        kerning_preview.redraw();
        _font_da.redraw();
    }
}

void SvgFontsDialog::on_glyphs_changed(){
    std::string str1(first_glyph.get_active_text());
    std::string str2(second_glyph.get_active_text());
    kerning_preview.set_text((gchar*) (str1+str2).c_str());
    kerning_preview.redraw();


    //look for this kerning pair on the currently selected font
    this->kerning_pair = NULL;
    for(SPObject* node = this->get_selected_spfont()->children; node; node=node->next){
        if (SP_IS_HKERN(node) && ((SPGlyphKerning*)node)->u1->contains((gchar) first_glyph.get_active_text().c_str()[0])
                                  && ((SPGlyphKerning*)node)->u2->contains((gchar) second_glyph.get_active_text().c_str()[0]) ){
            this->kerning_pair = (SPGlyphKerning*)node;
            continue;
        }
    }

//TODO:
    //if not found,
      //create new kern node
    if (this->kerning_pair)
        kerning_spin.set_value(this->kerning_pair->k);
}

/* Add all fonts in the document to the combobox. */
void SvgFontsDialog::update_fonts()
{
    SPDesktop* desktop = this->getDesktop();
    SPDocument* document = sp_desktop_document(desktop);
    const GSList* fonts = sp_document_get_resource_list(document, "font");

    _model->clear();
    for(const GSList *l = fonts; l; l = l->next) {
        Gtk::TreeModel::Row row = *_model->append();
        SPFont* f = (SPFont*)l->data;
        row[_columns.spfont] = f;
        row[_columns.svgfont] = new SvgFont(f);
        const gchar* lbl = f->label();
        const gchar* id = SP_OBJECT_ID(f);
        row[_columns.label] = lbl ? lbl : (id ? id : "font");
    }
}

void SvgFontsDialog::on_preview_text_changed(){
    _font_da.set_text((gchar*) _preview_entry.get_text().c_str());
    _font_da.set_text(_preview_entry.get_text());
    _font_da.redraw();
}

void SvgFontsDialog::on_font_selection_changed(){
    SPFont* spfont = this->get_selected_spfont();
    SvgFont* svgfont = this->get_selected_svgfont();
    first_glyph.update(spfont);
    second_glyph.update(spfont);
    kerning_preview.set_svgfont(svgfont);
    _font_da.set_svgfont(svgfont);
    _font_da.redraw();

    int steps = 50;
    double set_width = spfont->horiz_adv_x;
    setwidth_spin.set_value(set_width);

    kerning_spin.set_range(0,set_width);
    kerning_spin.set_increments(int(set_width/steps),2*int(set_width/steps));
    kerning_spin.set_value(0);
}

void SvgFontsDialog::on_setwidth_changed(){
    SPFont* spfont = this->get_selected_spfont();
    if (spfont){
        spfont->horiz_adv_x = setwidth_spin.get_value();
        //TODO: tell cairo that the glyphs cache has to be invalidated
    }
}

SvgFont* SvgFontsDialog::get_selected_svgfont()
{
    Gtk::TreeModel::iterator i = _font_list.get_selection()->get_selected();
    if(i)
        return (*i)[_columns.svgfont];
    return NULL;
}

SPFont* SvgFontsDialog::get_selected_spfont()
{
    Gtk::TreeModel::iterator i = _font_list.get_selection()->get_selected();
    if(i)
        return (*i)[_columns.spfont];
    return NULL;
}

Gtk::VBox* SvgFontsDialog::global_settings_tab(){
    Gtk::VBox* global_vbox = Gtk::manage(new Gtk::VBox());

    AttrEntry* familyname;
    familyname = new AttrEntry(this, (gchar*) _("Family Name:"), SP_PROP_FONT_FAMILY);

    global_vbox->add(*familyname);
/*    global_vbox->add(*AttrCombo((gchar*) _("Style:"), SP_PROP_FONT_STYLE));
    global_vbox->add(*AttrCombo((gchar*) _("Variant:"), SP_PROP_FONT_VARIANT));
    global_vbox->add(*AttrCombo((gchar*) _("Weight:"), SP_PROP_FONT_WEIGHT));
*/
//Set Width (horiz_adv_x):
/*    Gtk::HBox* setwidth_hbox = Gtk::manage(new Gtk::HBox());
    setwidth_hbox->add(*Gtk::manage(new Gtk::Label(_("Set width:"))));
    setwidth_hbox->add(setwidth_spin);

    setwidth_spin.signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::on_setwidth_changed));
    setwidth_spin.set_range(0, 4096);
    setwidth_spin.set_increments(10, 100);
    global_vbox->add(*setwidth_hbox);
*/
    return global_vbox;
}

Gtk::VBox* SvgFontsDialog::glyphs_tab(){
    Gtk::VBox* glyphs_vbox = Gtk::manage(new Gtk::VBox());
    glyphs_vbox->add(*new SvgFontsDialog::AttrEntry(this, (gchar*) _("Glyph Name:"), SP_ATTR_GLYPH_NAME));
    glyphs_vbox->add(*new SvgFontsDialog::AttrEntry(this, (gchar*) _("Unicode:"), SP_ATTR_UNICODE));
    //glyphs_vbox->add(*AttrSpin((gchar*) "Horizontal Advance"), SP_ATTR_HORIZ_ADV_X);
    //glyphs_vbox->add(*AttrCombo((gchar*) "Missing Glyph"), SP_ATTR_); ?
    return glyphs_vbox;
}

Gtk::VBox* SvgFontsDialog::kerning_tab(){

//kerning setup:
    Gtk::VBox* kernvbox = Gtk::manage(new Gtk::VBox());

//Kerning Setup:
    kernvbox->add(*Gtk::manage(new Gtk::Label(_("Kerning Setup:"))));
    Gtk::HBox* kerning_selector = Gtk::manage(new Gtk::HBox());
    kerning_selector->add(*Gtk::manage(new Gtk::Label(_("1st Glyph:"))));
    kerning_selector->add(first_glyph);
    kerning_selector->add(*Gtk::manage(new Gtk::Label(_("2nd Glyph:"))));
    kerning_selector->add(second_glyph);
    first_glyph.signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::on_glyphs_changed));
    second_glyph.signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::on_glyphs_changed));
    kerning_spin.signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::on_kerning_changed));

    kernvbox->add(*kerning_selector);
    kernvbox->add((Gtk::Widget&) kerning_preview);

    Gtk::HBox* kerning_amount_hbox = Gtk::manage(new Gtk::HBox());
    kernvbox->add(*kerning_amount_hbox);
    kerning_amount_hbox->add(*Gtk::manage(new Gtk::Label(_("Kerning value:"))));
    kerning_amount_hbox->add(kerning_spin);

    kerning_preview.set_size(300 + 20, 150 + 20);
    _font_da.set_size(150 + 20, 50 + 20);

    return kernvbox;
}

SPFont *new_font(SPDocument *document)
{
    g_return_val_if_fail(document != NULL, NULL);

    SPDefs *defs = (SPDefs *) SP_DOCUMENT_DEFS(document);

    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(document);

    // create a new font
    Inkscape::XML::Node *repr;
    repr = xml_doc->createElement("svg:font");

    // Append the new font node to defs
    SP_OBJECT_REPR(defs)->appendChild(repr);
    Inkscape::GC::release(repr);

    // get corresponding object
    SPFont *f = SP_FONT( document->getObjectByRepr(repr) );
    
    g_assert(f != NULL);
    g_assert(SP_IS_FONT(f));

    return f;
}


void SvgFontsDialog::add_font(){
    SPDocument* doc = sp_desktop_document(this->getDesktop());
    SPFont* font = new_font(doc);

    const int count = _model->children().size();
    std::ostringstream os;
    os << "font" << count;
    font->setLabel(os.str().c_str());

    update_fonts();
//    select_font(font);
//	on_font_selection_changed();

    sp_document_done(doc, SP_VERB_DIALOG_SVG_FONTS, _("Add font"));
}

SvgFontsDialog::SvgFontsDialog()
 : UI::Widget::Panel("", "/dialogs/svgfonts", SP_VERB_DIALOG_SVG_FONTS), _add(Gtk::Stock::NEW)
{
    _add.signal_clicked().connect(sigc::mem_fun(*this, &SvgFontsDialog::add_font));

    Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
    Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox());

    vbox->pack_start(_font_list);
    vbox->pack_start(_add, false, false);
    hbox->add(*vbox);
    hbox->add(_font_settings);
    _getContents()->add(*hbox);

//List of SVGFonts declared in a document:
    _model = Gtk::ListStore::create(_columns);
    _font_list.set_model(_model);
    _font_list.append_column_editable(_("_Font"), _columns.label);
    _font_list.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::on_font_selection_changed));

    this->update_fonts();

    Gtk::Notebook *tabs = Gtk::manage(new Gtk::Notebook());
    tabs->set_scrollable();

    tabs->append_page(*global_settings_tab(), _("_Global Settings"), true);
    tabs->append_page(*glyphs_tab(), _("_Glyphs"), true);
    tabs->append_page(*kerning_tab(), _("_Kerning"), true);

    _font_settings.add(*tabs);

//Text Preview:
    _preview_entry.signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::on_preview_text_changed));
    _getContents()->add((Gtk::Widget&) _font_da);
    _preview_entry.set_text("Sample Text");
    _font_da.set_text("Sample Text");

    Gtk::HBox* preview_entry_hbox = Gtk::manage(new Gtk::HBox());
    _getContents()->add(*preview_entry_hbox);
    preview_entry_hbox->add(*Gtk::manage(new Gtk::Label(_("Preview Text:"))));
    preview_entry_hbox->add(_preview_entry);

    _getContents()->show_all();
}

SvgFontsDialog::~SvgFontsDialog(){}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif //#ifdef ENABLE_SVG_FONTS
