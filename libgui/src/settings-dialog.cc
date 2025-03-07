////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011-2023 The Octave Project Developers
//
// See the file COPYRIGHT.md in the top-level directory of this
// distribution or <https://octave.org/copyright/>.
//
// This file is part of Octave.
//
// Octave is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Octave is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Octave; see the file COPYING.  If not, see
// <https://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////

// Programming Note: this file has many lines longer than 80 characters
// due to long function, variable, and property names.  Please don't
// break those lines as it tends to make this code even harder to read.

#if defined (HAVE_CONFIG_H)
#  include "config.h"
#endif

#include <QButtonGroup>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
#include <QMessageBox>
#include <QScrollBar>
#include <QStyleFactory>
#include <QThread>
#include <QVector>

#if defined (HAVE_QSCINTILLA)
#  include "octave-qscintilla.h"
#  include "octave-txt-lexer.h"
#  include <QScrollArea>

#  if defined (HAVE_QSCI_QSCILEXEROCTAVE_H)
#    define HAVE_LEXER_OCTAVE 1
#    include <Qsci/qscilexeroctave.h>
#  elif defined (HAVE_QSCI_QSCILEXERMATLAB_H)
#    define HAVE_LEXER_MATLAB 1
#    include <Qsci/qscilexermatlab.h>
#  endif

#  include <Qsci/qscilexercpp.h>
#  include <Qsci/qscilexerjava.h>
#  include <Qsci/qscilexerbash.h>
#  include <Qsci/qscilexerperl.h>
#  include <Qsci/qscilexerbatch.h>
#  include <Qsci/qscilexerdiff.h>
#endif

#include "gui-preferences-all.h"
#include "gui-settings.h"
#include "settings-dialog.h"
#include "shortcuts-tree-widget.h"
#include "variable-editor.h"
#include "workspace-model.h"

OCTAVE_BEGIN_NAMESPACE(octave)

settings_dialog::settings_dialog (QWidget *p, const QString& desired_tab)
  : QDialog (p), Ui::settings_dialog ()
{
  setupUi (this);

  QMessageBox *info = wait_message_box (
                      tr ("Loading current preferences ... "), this);

  read_settings (true);  // it's the first read, prepare everything

  close_wait_message_box (info);

  // which tab is the desired one?
  show_tab (desired_tab);

  // connect button box signal
  connect (button_box, &QDialogButtonBox::clicked,
           this, &settings_dialog::button_clicked);

  // restore last geometry
  gui_settings settings;

  if (settings.contains (sd_geometry.settings_key ()))
    restoreGeometry (settings.byte_array_value (sd_geometry));
  else
    setGeometry (QRect (10, 50, 1000, 600));

  // show as non-modal dialog
  setModal (false);
  setAttribute (Qt::WA_DeleteOnClose);
  show ();
}

void settings_dialog::read_settings (bool first)
{
  gui_settings settings;

  if (first)
    {
      // look for available language files and the actual settings
      QString qm_dir_name = settings.get_gui_translation_dir ();

      QDir qm_dir (qm_dir_name);
      QFileInfoList qm_files = qm_dir.entryInfoList (QStringList ("*.qm"),
                               QDir::Files | QDir::Readable, QDir::Name);

      for (int i = 0; i < qm_files.length (); i++)   // insert available languages
        comboBox_language->addItem (qm_files.at (i).baseName ());
      // System at beginning
      comboBox_language->insertItem (0, tr ("System setting"));
      comboBox_language->insertSeparator (1);    // separator after System
    }

  QString language = settings.string_value (global_language);
  if (language == global_language.def ().toString ())
    language = tr ("System setting");
  int selected = comboBox_language->findText (language);
  if (selected >= 0)
    comboBox_language->setCurrentIndex (selected);
  else
    comboBox_language->setCurrentIndex (0);  // System is default

  if (first)
    {
      // Global style
      QStringList styles = QStyleFactory::keys();
      styles.append (global_extra_styles);
      combo_styles->addItems (styles);
      combo_styles->insertItem (0, global_style.def ().toString ());
      combo_styles->insertSeparator (1);
    }

  QString current_style = settings.string_value (global_style);
  if (current_style == global_style.def ().toString ())
    current_style = global_style.def ().toString ();
  selected = combo_styles->findText (current_style);
  if (selected >= 0)
    combo_styles->setCurrentIndex (selected);
  else
    combo_styles->setCurrentIndex (0);

  if (first)
    {
      // icon size and theme
      QButtonGroup *icon_size_group = new QButtonGroup (this);
      icon_size_group->addButton (icon_size_small);
      icon_size_group->addButton (icon_size_normal);
      icon_size_group->addButton (icon_size_large);
    }
  int icon_size = settings.int_value (global_icon_size);
  icon_size_normal->setChecked (true);  // the default
  icon_size_small->setChecked (icon_size < 0);
  icon_size_large->setChecked (icon_size > 0);

  if (first)
    combo_box_icon_theme->addItems (global_all_icon_theme_names);
  int theme = settings.value (global_icon_theme_index.settings_key ()).toInt ();
  combo_box_icon_theme->setCurrentIndex (theme);

  if (first)
    {
      // which icon has to be selected
      QButtonGroup *icon_group = new QButtonGroup (this);
      icon_group->addButton (general_icon_octave);
      icon_group->addButton (general_icon_graphic);
      icon_group->addButton (general_icon_letter);
    }
  QString widget_icon_set =
    settings.string_value (dw_icon_set);
  general_icon_octave->setChecked (true);  // the default (if invalid set)
  general_icon_octave->setChecked (widget_icon_set == "NONE");
  general_icon_graphic->setChecked (widget_icon_set == "GRAPHIC");
  general_icon_letter->setChecked (widget_icon_set == "LETTER");

  if (first)
    {
      // custom title bar of dock widget
      m_widget_title_bg_color = new color_picker ();
      m_widget_title_bg_color->setEnabled (false);
      layout_widget_bgtitle->addWidget (m_widget_title_bg_color, 0);

      connect (cb_widget_custom_style, &QCheckBox::toggled,
               m_widget_title_bg_color, &color_picker::setEnabled);

      m_widget_title_bg_color_active = new color_picker ();
      m_widget_title_bg_color_active->setEnabled (false);
      layout_widget_bgtitle_active->addWidget (m_widget_title_bg_color_active, 0);

      connect (cb_widget_custom_style, &QCheckBox::toggled,
               m_widget_title_bg_color_active, &color_picker::setEnabled);

      m_widget_title_fg_color = new color_picker ();
      m_widget_title_fg_color->setEnabled (false);
      layout_widget_fgtitle->addWidget (m_widget_title_fg_color, 0);

      connect (cb_widget_custom_style, &QCheckBox::toggled,
               m_widget_title_fg_color, &color_picker::setEnabled);

      m_widget_title_fg_color_active = new color_picker ();
      m_widget_title_fg_color_active->setEnabled (false);
      layout_widget_fgtitle_active->addWidget (m_widget_title_fg_color_active, 0);

      connect (cb_widget_custom_style, &QCheckBox::toggled,
               m_widget_title_fg_color_active, &color_picker::setEnabled);
    }

  m_widget_title_bg_color->set_color (settings.color_value (dw_title_bg_color));
  m_widget_title_bg_color_active->set_color (settings.color_value (dw_title_bg_color_active));
  m_widget_title_fg_color->set_color (settings.color_value (dw_title_fg_color));
  m_widget_title_fg_color_active->set_color (settings.color_value (dw_title_fg_color_active));

  sb_3d_title->setValue (settings.int_value (dw_title_3d));
  cb_widget_custom_style->setChecked (settings.bool_value (dw_title_custom_style));

  // Native file dialogs.
  // FIXME: This preference can be deprecated / removed if all display
  //       managers, especially KDE, run those dialogs without hangs or
  //       delays from the start (bug #54607).
  cb_use_native_file_dialogs->setChecked (settings.bool_value (global_use_native_dialogs));

  // Cursor blinking: consider old terminal related setting if not yet set
  // FIXME: This pref. can be deprecated / removed if Qt adds support for
  //       getting the cursor blink preferences from all OS environments
  if (settings.contains (global_cursor_blinking.settings_key ()))
    {
      // Preference exists, read its value
      cb_cursor_blinking->setChecked (settings.bool_value (global_cursor_blinking));
    }
  else
    {
      // Pref. does not exist, so take old terminal related pref.
      cb_cursor_blinking->setChecked (settings.bool_value (cs_cursor_blinking));
    }

  // focus follows mouse
  cb_focus_follows_mouse->setChecked (settings.bool_value (dw_focus_follows_mouse));

  // prompt on exit
  cb_prompt_to_exit->setChecked (settings.bool_value (global_prompt_to_exit));

  // Main status bar
  cb_status_bar->setChecked (settings.bool_value (global_status_bar));

  // Octave startup
  cb_restore_octave_dir->setChecked (settings.bool_value (global_restore_ov_dir));
  le_octave_dir->setText (settings.string_value (global_ov_startup_dir));

  if (first)
    connect (pb_octave_dir, &QPushButton::pressed,
             this, &settings_dialog::get_octave_dir);

  //
  // editor
  //
  useCustomFileEditor->setChecked (settings.bool_value (global_use_custom_editor));
  customFileEditor->setText (settings.string_value (global_custom_editor));
  editor_showLineNumbers->setChecked (settings.bool_value (ed_show_line_numbers));
  editor_linenr_size->setValue (settings.int_value (ed_line_numbers_size));

  settings.combo_encoding (editor_combo_encoding);

  editor_highlightCurrentLine->setChecked (settings.bool_value (ed_highlight_current_line));
  editor_long_line_marker->setChecked (settings.bool_value (ed_long_line_marker));
  bool long_line =
    settings.bool_value (ed_long_line_marker_line);
  editor_long_line_marker_line->setChecked (long_line);
  bool long_back =
    settings.bool_value (ed_long_line_marker_background);
  editor_long_line_marker_background->setChecked (long_back);
  if (! (long_line || long_back))
    editor_long_line_marker_line->setChecked (true);
  editor_long_line_column->setValue (settings.int_value (ed_long_line_column));
  editor_break_checkbox->setChecked (settings.bool_value (ed_break_lines));
  editor_break_comments_checkbox->setChecked (settings.bool_value (ed_break_lines_comments));
  editor_wrap_checkbox->setChecked (settings.bool_value (ed_wrap_lines));
  cb_edit_status_bar->setChecked (settings.bool_value (ed_show_edit_status_bar));
  cb_edit_tool_bar->setChecked (settings.bool_value (ed_show_toolbar));
  cb_code_folding->setChecked (settings.bool_value (ed_code_folding));
  editor_highlight_all_occurrences->setChecked (settings.bool_value (ed_highlight_all_occurrences));

  editor_auto_endif->setCurrentIndex (settings.int_value (ed_auto_endif) );
  editor_codeCompletion->setChecked (settings.bool_value (ed_code_completion));
  editor_spinbox_ac_threshold->setValue (settings.int_value (ed_code_completion_threshold));
  editor_checkbox_ac_keywords->setChecked (settings.bool_value (ed_code_completion_keywords));
  editor_checkbox_ac_builtins->setEnabled (editor_checkbox_ac_keywords->isChecked ());
  editor_checkbox_ac_functions->setEnabled (editor_checkbox_ac_keywords->isChecked ());
  editor_checkbox_ac_builtins->setChecked (settings.bool_value (ed_code_completion_octave_builtins));
  editor_checkbox_ac_functions->setChecked (settings.bool_value (ed_code_completion_octave_functions));
  editor_checkbox_ac_document->setChecked (settings.bool_value (ed_code_completion_document));
  editor_checkbox_ac_case->setChecked (settings.bool_value (ed_code_completion_case));
  editor_checkbox_ac_replace->setChecked (settings.bool_value (ed_code_completion_replace));
  editor_ws_checkbox->setChecked (settings.bool_value (ed_show_white_space));
  editor_ws_indent_checkbox->setChecked (settings.bool_value (ed_show_white_space_indent));
  cb_show_eol->setChecked (settings.bool_value (ed_show_eol_chars));
  cb_show_hscrollbar->setChecked (settings.bool_value (ed_show_hscroll_bar));

  if (first)
    {
      for (int i = 0; i < ed_tab_position_names.length (); i++)
        editor_combox_tab_pos->insertItem (i,
                tr (ed_tab_position_names.at (i).toStdString ().data ()));
    }
  editor_combox_tab_pos->setCurrentIndex (settings.int_value (ed_tab_position));

  editor_cb_tabs_rotated->setChecked (settings.bool_value (ed_tabs_rotated));
  editor_sb_tabs_max_width->setValue (settings.int_value (ed_tabs_max_width));

  int selected_comment_string, selected_uncomment_string;

  if (settings.contains (ed_comment_str.settings_key ()))   // new version (radio buttons)
    selected_comment_string = settings.int_value (ed_comment_str);
  else                                         // old version (combo box)
    selected_comment_string = settings.value (ed_comment_str_old.settings_key (),                                                 ed_comment_str.def ()).toInt ();

  selected_uncomment_string = settings.int_value (ed_uncomment_str);

  for (int i = 0; i < ed_comment_strings_count; i++)
    {
      if (first)
        {
          m_rb_comment_strings[i] = new QRadioButton ();
          m_rb_uncomment_strings[i] = new QCheckBox ();
          layout_comment_strings->addWidget (m_rb_comment_strings[i]);
          layout_uncomment_strings->addWidget (m_rb_uncomment_strings[i]);

          connect (m_rb_comment_strings[i], &QRadioButton::clicked,
                   m_rb_uncomment_strings[i], &QCheckBox::setChecked);
          connect (m_rb_comment_strings[i], &QRadioButton::toggled,
                   m_rb_uncomment_strings[i], &QCheckBox::setDisabled);
        }

      m_rb_comment_strings[i]->setText (ed_comment_strings.at(i));
      m_rb_comment_strings[i]->setChecked (i == selected_comment_string);

      m_rb_uncomment_strings[i]->setText (ed_comment_strings.at(i));
      m_rb_uncomment_strings[i]->setAutoExclusive (false);
      m_rb_uncomment_strings[i]->setChecked ( 1 << i & selected_uncomment_string);
    }

  combo_eol_mode->setCurrentIndex (settings.int_value (ed_default_eol_mode));
  editor_auto_ind_checkbox->setChecked (settings.bool_value (ed_auto_indent));
  editor_tab_ind_checkbox->setChecked (settings.bool_value (ed_tab_indents_line));
  editor_bs_unind_checkbox->setChecked (settings.bool_value (ed_backspace_unindents_line));
  editor_ind_guides_checkbox->setChecked (settings.bool_value (ed_show_indent_guides));
  editor_ind_width_spinbox->setValue (settings.int_value (ed_indent_width));
  editor_ind_uses_tabs_checkbox->setChecked (settings.bool_value (ed_indent_uses_tabs));
  editor_tab_width_spinbox->setValue (settings.int_value (ed_tab_width));
  editor_restoreSession->setChecked (settings.bool_value (ed_restore_session));
  editor_create_new_file->setChecked (settings.bool_value (ed_create_new_file));
  editor_reload_changed_files->setChecked (settings.bool_value (ed_always_reload_changed_files));
  editor_force_newline->setChecked (settings.bool_value (ed_force_newline));
  editor_remove_trailing_spaces->setChecked (settings.bool_value (ed_rm_trailing_spaces));
  editor_hiding_closes_files->setChecked (settings.bool_value (ed_hiding_closes_files));
  editor_show_dbg_file->setChecked (settings.bool_value (ed_show_dbg_file));

  // terminal
  QString default_font = settings.string_value (global_mono_font);
  terminal_fontName->setCurrentFont (QFont (settings.value (cs_font.settings_key (), default_font).toString ()));
  terminal_fontSize->setValue (settings.int_value (cs_font_size));
  terminal_history_buffer->setValue (settings.int_value (cs_hist_buffer));
  terminal_cursorUseForegroundColor->setChecked (settings.bool_value (cs_cursor_use_fgcol));
  terminal_focus_command->setChecked (settings.bool_value (cs_focus_cmd));
  terminal_print_dbg_location->setChecked (settings.bool_value (cs_dbg_location));

  QString cursor_type = settings.string_value (cs_cursor);

  QStringList items;
  items << QString ("0") << QString ("1") << QString ("2");
  terminal_cursorType->addItems (items);
  terminal_cursorType->setItemText (0, tr ("IBeam Cursor"));
  terminal_cursorType->setItemText (1, tr ("Block Cursor"));
  terminal_cursorType->setItemText (2, tr ("Underline Cursor"));

  for (unsigned int i = 0; i < cs_cursor_types.size (); i++)
    {
      if (cursor_type.toStdString () == cs_cursor_types[i])
        {
          terminal_cursorType->setCurrentIndex (i);
          break;
        }
    }

  if (first)
    read_terminal_colors ();
  else
    {
      QCheckBox *cb_color_mode
        = terminal_colors_box->findChild <QCheckBox *> (cs_color_mode.settings_key ());
      bool sec_color_mode = settings.bool_value (cs_color_mode);
      if (cb_color_mode->isChecked () == sec_color_mode)
        {
          // color mode does not change, update colors manually
          update_terminal_colors ();
        }
      else
        {
          // toggling check-state calls related slot updating colors
          cb_color_mode->setChecked (sec_color_mode);
        }
    }

  // file browser
  if (first)
    {
      connect (sync_octave_directory, &QCheckBox::toggled,
               this, &settings_dialog::set_disabled_pref_file_browser_dir);
      connect (pb_file_browser_dir, &QPushButton::pressed,
               this, &settings_dialog::get_file_browser_dir);
    }

  sync_octave_directory->setChecked (settings.bool_value (fb_sync_octdir));
  cb_restore_file_browser_dir->setChecked (settings.bool_value (fb_restore_last_dir));
  le_file_browser_dir->setText (settings.value (fb_startup_dir.settings_key ()).toString ());
  le_file_browser_extensions->setText (settings.string_value (fb_txt_file_ext));
  checkbox_allow_web_connect->setChecked (settings.bool_value (nr_allow_connection));

  // Proxy
  bool use_proxy = settings.bool_value (global_use_proxy);
  use_proxy_server->setChecked (use_proxy);
  // Fill combo box and activate current one
  if (first)
    {
      proxy_type->addItems (global_proxy_all_types);
      // Connect relevant signals for dis-/enabling some elements
      connect (proxy_type, QOverload<int>::of (&QComboBox::currentIndexChanged),
               this, &settings_dialog::proxy_items_update);
      connect (use_proxy_server, &QCheckBox::toggled,
               this, &settings_dialog::proxy_items_update);
    }
  QString proxy_type_string = settings.string_value (global_proxy_type);
  for (int i = 0; i < global_proxy_all_types.length (); i++)
    {
      if (proxy_type->itemText (i) == proxy_type_string)
        {
          proxy_type->setCurrentIndex (i);
          break;
        }
    }
  // Fill all line edits
  proxy_host_name->setText (settings.string_value (global_proxy_host));
  proxy_port->setText (settings.string_value (global_proxy_port));
  proxy_username->setText (settings.string_value (global_proxy_user));
  proxy_password->setText (settings.string_value (global_proxy_pass));
  // Check whehter line edits have to be enabled
  proxy_items_update ();

  // Workspace
  if (first)
    read_workspace_colors ();
  else
    {
      m_ws_enable_colors->setChecked (settings.bool_value (ws_enable_colors));
      QCheckBox *cb_color_mode
        = workspace_colors_box->findChild <QCheckBox *> (ws_color_mode.settings_key ());
      bool sec_color_mode = settings.bool_value (ws_color_mode);
      if (cb_color_mode->isChecked () == sec_color_mode)
        {
          // color mode does not change, update colors manually
          update_workspace_colors ();
        }
      else
        {
          // toggling check-state calls related slot updating colors
          cb_color_mode->setChecked (sec_color_mode);
        }
    }

  // variable editor
  if (first)
    {
      connect (varedit_useTerminalFont, &QCheckBox::toggled,
               varedit_font, &QFontComboBox::setDisabled);
      connect (varedit_useTerminalFont, &QCheckBox::toggled,
               varedit_fontSize, &QSpinBox::setDisabled);
    }
  varedit_columnWidth->setValue (settings.int_value (ve_column_width));
  varedit_rowHeight->setValue (settings.int_value (ve_row_height));
  varedit_font->setCurrentFont (QFont (settings.value (ve_font_name.settings_key (),
                                                        settings.value (cs_font.settings_key (), default_font)).toString ()));
  varedit_fontSize->setValue (settings.int_value (ve_font_size));
  varedit_useTerminalFont->setChecked (settings.bool_value (ve_use_terminal_font));
  varedit_font->setDisabled (varedit_useTerminalFont->isChecked ());
  varedit_fontSize->setDisabled (varedit_useTerminalFont->isChecked ());
  varedit_alternate->setChecked (settings.bool_value (ve_alternate_rows));

  // variable editor colors
  if (first)
    read_varedit_colors ();
  else
    {
      QCheckBox *cb_color_mode
        = varedit_colors_box->findChild <QCheckBox *> (ve_color_mode.settings_key ());
      bool sec_color_mode = settings.bool_value (ve_color_mode);
      if (cb_color_mode->isChecked () == sec_color_mode)
        {
          // color mode does not change, update colors manually
          update_varedit_colors ();
        }
      else
        {
          // toggling check-state calls related slot updating colors
          cb_color_mode->setChecked (sec_color_mode);
        }
    }

  // shortcuts

  cb_prevent_readline_conflicts->setChecked (settings.bool_value (sc_prevent_rl_conflicts));
  cb_prevent_readline_conflicts_menu->setChecked (settings.bool_value (sc_prevent_rl_conflicts_menu));

  // connect the buttons for import/export of the shortcut sets
  // FIXME: Should there also be a button to discard changes?

  if (first)
    {
      connect (btn_import_shortcut_set, &QPushButton::clicked,
               this, &settings_dialog::import_shortcut_set);

      connect (btn_export_shortcut_set, &QPushButton::clicked,
               this, &settings_dialog::export_shortcut_set);

      connect (btn_default_shortcut_set, &QPushButton::clicked,
               this, &settings_dialog::default_shortcut_set);
    }

#if defined (HAVE_QSCINTILLA)

  if (first)
    {
      int mode = settings.int_value (ed_color_mode);

      QCheckBox *cb_color_mode = new QCheckBox (tr (settings_color_modes.toStdString ().data ()),
                                                group_box_editor_styles);
      cb_color_mode->setToolTip (tr (settings_color_modes_tooltip.toStdString ().data ()));
      cb_color_mode->setChecked (mode > 0);
      cb_color_mode->setObjectName (ed_color_mode.settings_key ());

      QPushButton *pb_reload_default_colors = new QPushButton (tr (settings_reload_styles.toStdString ().data ()));
      pb_reload_default_colors->setToolTip (tr (settings_reload_styles_tooltip.toStdString ().data ()));

      color_picker *current_line_color = new color_picker (
        settings.value (ed_highlight_current_line_color.settings_key ()
                        + settings_color_modes_ext[mode],
                        ed_highlight_current_line_color.def ()).value<QColor> ());
      current_line_color->setObjectName (ed_highlight_current_line_color.settings_key ());

      QLabel *current_line_color_label
        = new QLabel(tr ("Color of highlighted current line (magenta (255,0,255) for automatic color)"));

      QHBoxLayout *color_mode = new QHBoxLayout ();
      color_mode->addWidget (cb_color_mode);
      color_mode->addItem (new QSpacerItem (5, 5, QSizePolicy::Expanding));
      color_mode->addWidget (pb_reload_default_colors);

      QHBoxLayout *current_line = new QHBoxLayout ();
      current_line->addWidget (current_line_color_label);
      current_line->addWidget (current_line_color);
      current_line->addItem (new QSpacerItem (5, 5, QSizePolicy::Expanding));

      editor_styles_layout->addLayout (color_mode);
      editor_styles_layout->addLayout (current_line);

      // update colors depending on second theme selection
      connect (cb_color_mode, &QCheckBox::stateChanged,
               this, &settings_dialog::update_editor_lexers);
      connect (pb_reload_default_colors, &QPushButton::clicked,
               [=] () { update_editor_lexers (settings_reload_default_colors_flag); });

      // finally read the lexer colors using the update slot
      update_editor_lexers ();
    }
  else
    {
      QCheckBox *cb_color_mode
        = group_box_editor_styles->findChild <QCheckBox *> (ed_color_mode.settings_key ());
      bool sec_color_mode = settings.bool_value (ed_color_mode);
      if (cb_color_mode->isChecked () == sec_color_mode)
        {
          // color mode does not change, update colors manually
          update_editor_lexers ();
        }
      else
        {
          // toggling check-state calls related slot updating colors
          cb_color_mode->setChecked (sec_color_mode);
        }
    }

#endif
}

void settings_dialog::show_tab (const QString& tab)
{
  gui_settings settings;

  if (tab.isEmpty ())
    tabWidget->setCurrentIndex (settings.int_value (sd_last_tab));
  else
    {
      QHash <QString, QWidget *> tab_hash;
      tab_hash["editor"] = tab_editor;
      tab_hash["editor_styles"] = tab_editor;
      tabWidget->setCurrentIndex (tabWidget->indexOf (tab_hash.value (tab)));
      if (tab == "editor_styles")
        tab_editor_scroll_area->ensureWidgetVisible (group_box_editor_styles);
    }
}

void settings_dialog::get_octave_dir ()
{
  get_dir (le_octave_dir, tr ("Set Octave Startup Directory"));
}

void settings_dialog::get_file_browser_dir ()
{
  get_dir (le_file_browser_dir, tr ("Set File Browser Startup Directory"));
}

void settings_dialog::get_dir (QLineEdit *line_edit, const QString& title)
{
  // FIXME: Remove, if for all common KDE versions (bug #54607) is resolved.
  int opts = QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks;

  gui_settings settings;

  if (! settings.bool_value (global_use_native_dialogs))
    opts |= QFileDialog::DontUseNativeDialog;

  QString dir = QFileDialog::getExistingDirectory
    (this, title, line_edit->text (), QFileDialog::Option (opts));

  line_edit->setText (dir);
}

void settings_dialog::button_clicked (QAbstractButton *button)
{
  QDialogButtonBox::ButtonRole button_role = button_box->buttonRole (button);

  if (button_role == QDialogButtonBox::ApplyRole
      || button_role == QDialogButtonBox::AcceptRole)
    {
      write_changed_settings ();
      if (button_role == QDialogButtonBox::AcceptRole)
        hide ();  // already hide here, reloading settings takes some time

      QMessageBox *info = wait_message_box (tr ("Applying preferences ... "), this);
      emit apply_new_settings ();
      close_wait_message_box (info);
    }

  if (button_role == QDialogButtonBox::RejectRole
      || button_role == QDialogButtonBox::AcceptRole)
    {
      // save last settings dialog's geometry and close

      gui_settings settings;

      settings.setValue (sd_last_tab.settings_key (), tabWidget->currentIndex ());
      settings.setValue (sd_geometry.settings_key (), saveGeometry ());
      settings.sync ();

      close ();
    }

  if (button_role == QDialogButtonBox::ResetRole)
    {
      read_settings (false);  // not the first read, only update existing items
    }
}

void settings_dialog::set_disabled_pref_file_browser_dir (bool disable)
{
  cb_restore_file_browser_dir->setDisabled (disable);

  if (! disable)
    {
      le_file_browser_dir->setDisabled (cb_restore_file_browser_dir->isChecked ());
      pb_file_browser_dir->setDisabled (cb_restore_file_browser_dir->isChecked ());
    }
  else
    {
      le_file_browser_dir->setDisabled (disable);
      pb_file_browser_dir->setDisabled (disable);
    }
}

// slot for updating enabled state of proxy settings
void settings_dialog::proxy_items_update ()
{
  bool use_proxy = use_proxy_server->isChecked ();

  bool manual = false;
  for (int i = 0; i < global_proxy_manual_types.length (); i++)
    {
      if (proxy_type->currentIndex () == global_proxy_manual_types.at (i))
        {
          manual = true;
          break;
        }
    }

  proxy_type->setEnabled (use_proxy);
  proxy_host_name_label->setEnabled (use_proxy && manual);
  proxy_host_name->setEnabled (use_proxy && manual);
  proxy_port_label->setEnabled (use_proxy && manual);
  proxy_port->setEnabled (use_proxy && manual);
  proxy_username_label->setEnabled (use_proxy && manual);
  proxy_username->setEnabled (use_proxy && manual);
  proxy_password_label->setEnabled (use_proxy && manual);
  proxy_password->setEnabled (use_proxy && manual);
}

// slots for import/export of shortcut sets

// Prompt for file name and import shortcuts from it.  Importing will
// change values in tree view but does not apply values to
// gui_settings_object so that the user may choose to apply or cancel
// the action.

void settings_dialog::import_shortcut_set ()
{
  if (! overwrite_all_shortcuts ())
    return;

  QString file = get_shortcuts_file_name (OSC_IMPORT);

  gui_settings osc_settings (file, QSettings::IniFormat);

  if (osc_settings.status () ==  QSettings::NoError)
    shortcuts_treewidget->import_shortcuts (osc_settings);
  else
    qWarning () << (tr ("Failed to open %1 as Octave shortcut file")
                    .arg (file));
}

// Prompt for file name and export shortcuts to it.

// FIXME: Should exported settings values come from the gui_settings
// object or the tree view?  If modified values in the tree view have
// not been applied, should we offer to apply them first?  Offer a
// choice to save current application settings or the modified values
// in the dialog?

void settings_dialog::export_shortcut_set ()
{
  QString file = get_shortcuts_file_name (OSC_EXPORT);

  gui_settings osc_settings (file, QSettings::IniFormat);

  if (osc_settings.status () ==  QSettings::NoError)
    shortcuts_treewidget->export_shortcuts (osc_settings);
  else
    qWarning () << (tr ("Failed to open %1 as Octave shortcut file")
                    .arg (file));
}

// Reset the tree view to default values.  Does not apply values to
// gui_settings object so that the user may choose to apply or cancel
// the action.

void settings_dialog::default_shortcut_set ()
{
  if (! overwrite_all_shortcuts ())
    return;

  shortcuts_treewidget->set_default_shortcuts ();
}

void settings_dialog::update_editor_lexers (int def)
{
#if defined (HAVE_QSCINTILLA)

  QCheckBox *cb_color_mode
    = group_box_editor_styles->findChild <QCheckBox *> (ed_color_mode.settings_key ());

  int m = 0;
  if (cb_color_mode && cb_color_mode->isChecked ())
    m = 1;

  color_picker *c_picker = findChild <color_picker *> (ed_highlight_current_line_color.settings_key ());
  if (c_picker)
    {
      gui_settings settings;

      if (def != settings_reload_default_colors_flag)
        {
          // Get current value from settings or the default
          c_picker->set_color (settings.color_value (ed_highlight_current_line_color, m));
        }
      else
        {
          // Get the default value
          c_picker->set_color (settings.get_color_value (ed_highlight_current_line_color.def (), m));
        }
    }

  // editor styles: create lexer, read settings, and
  // create or update dialog elements
  QsciLexer *lexer;

#  if defined (HAVE_LEXER_OCTAVE)
  lexer = new QsciLexerOctave ();
  update_lexer (lexer, m, def);
  delete lexer;
#  elif defined (HAVE_LEXER_MATLAB)
  lexer = new QsciLexerMatlab ();
  update_lexer (lexer, m, def);
  delete lexer;
#  endif

  lexer = new QsciLexerCPP ();
  update_lexer (lexer, m, def);
  delete lexer;

  lexer = new QsciLexerJava ();
  update_lexer (lexer, m, def);
  delete lexer;

  lexer = new QsciLexerPerl ();
  update_lexer (lexer, m, def);
  delete lexer;

  lexer = new QsciLexerBatch ();
  update_lexer (lexer, m, def);
  delete lexer;

  lexer = new QsciLexerDiff ();
  update_lexer (lexer, m, def);
  delete lexer;

  lexer = new QsciLexerBash ();
  update_lexer (lexer, m, def);
  delete lexer;

  lexer = new octave_txt_lexer ();
  update_lexer (lexer, m, def);
  delete lexer;

#else

  octave_unused_parameter (def);

#endif
}

#if defined (HAVE_QSCINTILLA)

void settings_dialog::update_lexer (QsciLexer *lexer, int mode, int def)
{
  // Get lexer settings and copy from default settings if not yet
  // available in normal settings file
  gui_settings settings;
  settings.read_lexer_settings (lexer, mode, def);

  // When reloading default styles, the style tabs do already exists.
  // Otherwise, check if they exist or not.
  QString lexer_name = lexer->language ();

  int index = -1;
  for (int i = 0; i < tabs_editor_lexers->count (); i++)
    {
      if (tabs_editor_lexers->tabText (i) == lexer_name)
        {
          index = i;
          break;
        }
    }

  if (index == -1)
    {
      // This is not an update, call get_lexer_settings for building
      // the settings tab
      get_lexer_settings (lexer);
      return;
    }

  // Update the styles elements in all styles
  int styles[ed_max_lexer_styles];  // array for saving valid styles
  int max_style = settings.get_valid_lexer_styles (lexer, styles);
  QWidget *tab = tabs_editor_lexers->widget (index);
  int default_size = 0;
  QString default_family;

  for (int i = 0; i < max_style; i++)  // create dialog elements for all styles
    {
      QString actual_name = lexer->description (styles[i]);
      color_picker *bg_color
        = tab->findChild <color_picker *> (actual_name + "_bg_color");
      if (bg_color)
        {
          // Update
          if (styles[i] == 0)
            bg_color->set_color (lexer->defaultPaper ());
          else
            {
              if (lexer->paper (styles[i]) == lexer->defaultPaper ())
                bg_color->set_color (settings_color_no_change);
              else
                bg_color->set_color (lexer->paper (styles[i]));
            }
        }

      color_picker *color = tab->findChild <color_picker *> (actual_name + "_color");
      if (color)
        color->set_color (lexer->color (styles[i]));

      QFont font = lexer->font (styles[i]);

      QCheckBox *cb = tab->findChild <QCheckBox *> (actual_name + "_bold");
      if (cb)
        cb->setChecked (font.bold ());
      cb = tab->findChild <QCheckBox *> (actual_name + "_italic");
      if (cb)
        cb->setChecked (font.italic ());
      cb = tab->findChild <QCheckBox *> (actual_name + "_underline");
      if (cb)
        cb->setChecked (font.underline ());

      QFontComboBox *fcb = tab->findChild <QFontComboBox *> (actual_name + "_font");
      if (fcb)
        {
          if (styles[i] == 0)
            {
              default_family = font.family ();
              fcb->setEditText (default_family);
            }
          else
            {
              if (font.family () == default_family)
                fcb->setEditText (lexer->description (0));
              else
                fcb->setEditText (font.family ());
            }
        }
      QSpinBox *fs = tab->findChild <QSpinBox *> (actual_name + "_size");
      if (fs)
        {
          if (styles[i] == 0)
            {
              default_size = font.pointSize ();
              fs->setValue (default_size);
            }
          else
            fs->setValue (font.pointSize () - default_size);
        }
    }

}

void settings_dialog::get_lexer_settings (QsciLexer *lexer)
{
  gui_settings settings;

  int styles[ed_max_lexer_styles];  // array for saving valid styles
  // (enum is not continuous)
  int max_style = settings.get_valid_lexer_styles (lexer, styles);
  QGridLayout *style_grid = new QGridLayout ();
  QVector<QLabel *> description (max_style);
  QVector<QFontComboBox *> select_font (max_style);
  QVector<QSpinBox *> font_size (max_style);
  QVector<QCheckBox *> attrib_font (3 * max_style);
  QVector<color_picker *> color (max_style);
  QVector<color_picker *> bg_color (max_style);
  int default_size = 10;
  QFont default_font = QFont ();
  int label_width;
  QColor default_color = QColor ();

  for (int i = 0; i < max_style; i++)  // create dialog elements for all styles
    {
      QString actual_name = lexer->description (styles[i]);
      QFont   actual_font = lexer->font (styles[i]);
      description[i] = new QLabel (actual_name);
      description[i]->setWordWrap (true);
      label_width = 24*description[i]->fontMetrics ().averageCharWidth ();
      description[i]->setMaximumSize (label_width, QWIDGETSIZE_MAX);
      description[i]->setMinimumSize (label_width, 1);
      select_font[i] = new QFontComboBox ();
      select_font[i]->setObjectName (actual_name + "_font");
      select_font[i]->setMaximumSize (label_width, QWIDGETSIZE_MAX);
      select_font[i]->setMinimumSize (label_width, 1);
      select_font[i]->setSizeAdjustPolicy (QComboBox::AdjustToMinimumContentsLengthWithIcon);
      font_size[i] = new QSpinBox ();
      font_size[i]->setObjectName (actual_name + "_size");
      if (styles[i] == 0) // the default
        {
          select_font[i]->setCurrentFont (actual_font);
          default_font = actual_font;
          font_size[i]->setRange (6, 24);
          default_size = actual_font.pointSize ();
          font_size[i]->setValue (default_size);
          default_color = lexer->defaultPaper ();
          bg_color[i] = new color_picker (default_color);
        }
      else   // other styles
        {
          select_font[i]->setCurrentFont (actual_font);
          if (actual_font.family () == default_font.family ())
            select_font[i]->setEditText (lexer->description (0));
          font_size[i]->setRange (-4, 4);
          font_size[i]->setValue (actual_font.pointSize ()-default_size);
          font_size[i]->setToolTip (QObject::tr ("Difference to the default size"));
          if (lexer->paper (styles[i]) == default_color)
            bg_color[i] = new color_picker (settings_color_no_change);
          else
            bg_color[i] = new color_picker (lexer->paper (styles[i]));
          bg_color[i]->setToolTip
            (QObject::tr ("Background color, magenta (255, 0, 255) means default"));
        }
      attrib_font[0+3*i] = new QCheckBox (QObject::tr ("b", "short form for bold"));
      attrib_font[1+3*i] = new QCheckBox (QObject::tr ("i", "short form for italic"));
      attrib_font[2+3*i] = new QCheckBox (QObject::tr ("u", "short form for underlined"));
      attrib_font[0+3*i]->setChecked (actual_font.bold ());
      attrib_font[0+3*i]->setObjectName (actual_name + "_bold");
      attrib_font[1+3*i]->setChecked (actual_font.italic ());
      attrib_font[1+3*i]->setObjectName (actual_name + "_italic");
      attrib_font[2+3*i]->setChecked (actual_font.underline ());
      attrib_font[2+3*i]->setObjectName (actual_name + "_underline");
      color[i] = new color_picker (lexer->color (styles[i]));
      color[i]->setObjectName (actual_name + "_color");
      bg_color[i]->setObjectName (actual_name + "_bg_color");
      int column = 1;
      style_grid->addWidget (description[i], i, column++);
      style_grid->addWidget (select_font[i], i, column++);
      style_grid->addWidget (font_size[i], i, column++);
      style_grid->addWidget (attrib_font[0+3*i], i, column++);
      style_grid->addWidget (attrib_font[1+3*i], i, column++);
      style_grid->addWidget (attrib_font[2+3*i], i, column++);
      style_grid->addWidget (color[i], i, column++);
      style_grid->addWidget (bg_color[i], i, column++);
    }

  // place grid with elements into the tab
  QScrollArea *scroll_area = new QScrollArea ();
  QWidget *scroll_area_contents = new QWidget ();
  scroll_area_contents->setObjectName (QString (lexer->language ()) + "_styles");
  scroll_area_contents->setLayout (style_grid);
  scroll_area->setWidget (scroll_area_contents);
  tabs_editor_lexers->addTab (scroll_area, lexer->language ());

  tabs_editor_lexers->setCurrentIndex (settings.int_value (sd_last_editor_styles_tab));
}

void settings_dialog::write_lexer_settings (QsciLexer *lexer)
{
  gui_settings settings;

  QCheckBox *cb_color_mode
    = group_box_editor_styles->findChild <QCheckBox *> (ed_color_mode.settings_key ());
  int mode = 0;
  if (cb_color_mode && cb_color_mode->isChecked ())
    mode = 1;

  settings.setValue (ed_color_mode.settings_key (), mode);

  QWidget *tab = tabs_editor_lexers->
    findChild <QWidget *> (QString (lexer->language ()) + "_styles");
  int styles[ed_max_lexer_styles];  // array for saving valid styles
  // (enum is not continuous)

  int max_style = settings.get_valid_lexer_styles (lexer, styles);

  QFontComboBox *select_font;
  QSpinBox *font_size;
  QCheckBox *attrib_font[3];
  color_picker *color;
  color_picker *bg_color;
  int default_size = 10;

  color = findChild <color_picker *> (ed_highlight_current_line_color.settings_key ());
  if (color)
    settings.setValue (ed_highlight_current_line_color.settings_key ()
                        + settings_color_modes_ext[mode], color->color ());

  QString default_font_name
    = settings.string_value (global_mono_font);
  QFont default_font = QFont (default_font_name, 10, -1, 0);
  QColor default_color = QColor ();

  for (int i = 0; i < max_style; i++)  // get dialog elements and their contents
    {
      QString actual_name = lexer->description (styles[i]);
      select_font = tab->findChild <QFontComboBox *> (actual_name + "_font");
      font_size = tab->findChild <QSpinBox *> (actual_name + "_size");
      attrib_font[0] = tab->findChild <QCheckBox *> (actual_name + "_bold");
      attrib_font[1] = tab->findChild <QCheckBox *> (actual_name + "_italic");
      attrib_font[2] = tab->findChild <QCheckBox *> (actual_name + "_underline");
      color = tab->findChild <color_picker *> (actual_name + "_color");
      bg_color = tab->findChild <color_picker *> (actual_name + "_bg_color");
      QFont new_font = default_font;
      if (select_font)
        {
          new_font = select_font->currentFont ();
          if (styles[i] == 0)
            default_font = new_font;
          else if (select_font->currentText () == lexer->description (0))
            new_font = default_font;
        }
      if (font_size)
        {
          if (styles[i] == 0)
            {
              default_size = font_size->value ();
              new_font.setPointSize (font_size->value ());
            }
          else
            new_font.setPointSize (font_size->value ()+default_size);
        }
      if (attrib_font[0])
        new_font.setBold (attrib_font[0]->isChecked ());
      if (attrib_font[1])
        new_font.setItalic (attrib_font[1]->isChecked ());
      if (attrib_font[2])
        new_font.setUnderline (attrib_font[2]->isChecked ());
      lexer->setFont (new_font, styles[i]);
      if (styles[i] == 0)
        lexer->setDefaultFont (new_font);
      if (color)
        lexer->setColor (color->color (), styles[i]);
      if (bg_color)
        {
          if (styles[i] == 0)
            {
              default_color = bg_color->color ();
              lexer->setPaper (default_color, styles[i]);
              lexer->setDefaultPaper (default_color);
            }
          else
            {
              if (bg_color->color () == settings_color_no_change)
                lexer->setPaper (default_color, styles[i]);
              else
                lexer->setPaper (bg_color->color (), styles[i]);
            }
        }
    }

  const std::string group =
    QString ("Scintilla" + settings_color_modes_ext[mode]).toStdString ();

  lexer->writeSettings (settings, group.c_str ());

  settings.setValue (sd_last_editor_styles_tab.settings_key (),
                     tabs_editor_lexers->currentIndex ());
  settings.sync ();
}

#endif

void settings_dialog::write_changed_settings ()
{

  gui_settings settings;

  // the icon set
  QString widget_icon_set = "NONE";
  if (general_icon_letter->isChecked ())
    widget_icon_set = "LETTER";
  else if (general_icon_graphic->isChecked ())
    widget_icon_set = "GRAPHIC";
  settings.setValue (dw_icon_set.settings_key (), widget_icon_set);

  // language
  QString language = comboBox_language->currentText ();
  if (language == tr ("System setting"))
    language = global_language.def ().toString ();
  settings.setValue (global_language.settings_key (), language);

  // style
  QString selected_style = combo_styles->currentText ();
  if (selected_style == global_style.def ().toString ())
    selected_style = global_style.def ().toString ();
  settings.setValue (global_style.settings_key (), selected_style);

  // dock widget title bar
  settings.setValue (dw_title_custom_style.settings_key (), cb_widget_custom_style->isChecked ());
  settings.setValue (dw_title_3d.settings_key (), sb_3d_title->value ());
  settings.setValue (dw_title_bg_color.settings_key (), m_widget_title_bg_color->color ());
  settings.setValue (dw_title_bg_color_active.settings_key (), m_widget_title_bg_color_active->color ());
  settings.setValue (dw_title_fg_color.settings_key (), m_widget_title_fg_color->color ());
  settings.setValue (dw_title_fg_color_active.settings_key (), m_widget_title_fg_color_active->color ());

  // icon size and theme
  int icon_size = icon_size_large->isChecked () - icon_size_small->isChecked ();
  settings.setValue (global_icon_size.settings_key (), icon_size);
  settings.setValue (global_icon_theme_index.settings_key (), combo_box_icon_theme->currentIndex ());

  // native file dialogs
  settings.setValue (global_use_native_dialogs.settings_key (), cb_use_native_file_dialogs->isChecked ());

  // cursor blinking
  settings.setValue (global_cursor_blinking.settings_key (), cb_cursor_blinking->isChecked ());

  // focus follows mouse
  settings.setValue (dw_focus_follows_mouse.settings_key (), cb_focus_follows_mouse->isChecked ());

  // promp to exit
  settings.setValue (global_prompt_to_exit.settings_key (), cb_prompt_to_exit->isChecked ());

  // status bar
  settings.setValue (global_status_bar.settings_key (), cb_status_bar->isChecked ());

  // Octave startup
  settings.setValue (global_restore_ov_dir.settings_key (), cb_restore_octave_dir->isChecked ());
  settings.setValue (global_ov_startup_dir.settings_key (), le_octave_dir->text ());

  //editor
  settings.setValue (global_use_custom_editor.settings_key (), useCustomFileEditor->isChecked ());
  settings.setValue (global_custom_editor.settings_key (), customFileEditor->text ());
  settings.setValue (ed_show_line_numbers.settings_key (), editor_showLineNumbers->isChecked ());
  settings.setValue (ed_line_numbers_size.settings_key (), editor_linenr_size->value ());
  settings.setValue (ed_highlight_current_line.settings_key (), editor_highlightCurrentLine->isChecked ());
  settings.setValue (ed_long_line_marker.settings_key (), editor_long_line_marker->isChecked ());
  settings.setValue (ed_long_line_marker_line.settings_key (), editor_long_line_marker_line->isChecked ());
  settings.setValue (ed_long_line_marker_background.settings_key (), editor_long_line_marker_background->isChecked ());
  settings.setValue (ed_long_line_column.settings_key (), editor_long_line_column->value ());
  settings.setValue (ed_break_lines.settings_key (), editor_break_checkbox->isChecked ());
  settings.setValue (ed_break_lines_comments.settings_key (), editor_break_comments_checkbox->isChecked ());
  settings.setValue (ed_wrap_lines.settings_key (), editor_wrap_checkbox->isChecked ());
  settings.setValue (ed_code_folding.settings_key (), cb_code_folding->isChecked ());
  settings.setValue (ed_show_edit_status_bar.settings_key (), cb_edit_status_bar->isChecked ());
  settings.setValue (ed_show_toolbar.settings_key (), cb_edit_tool_bar->isChecked ());
  settings.setValue (ed_highlight_all_occurrences.settings_key (), editor_highlight_all_occurrences->isChecked ());
  settings.setValue (ed_code_completion.settings_key (), editor_codeCompletion->isChecked ());
  settings.setValue (ed_code_completion_threshold.settings_key (), editor_spinbox_ac_threshold->value ());
  settings.setValue (ed_code_completion_keywords.settings_key (), editor_checkbox_ac_keywords->isChecked ());
  settings.setValue (ed_code_completion_octave_builtins.settings_key (), editor_checkbox_ac_builtins->isChecked ());
  settings.setValue (ed_code_completion_octave_functions.settings_key (), editor_checkbox_ac_functions->isChecked ());
  settings.setValue (ed_code_completion_document.settings_key (), editor_checkbox_ac_document->isChecked ());
  settings.setValue (ed_code_completion_case.settings_key (), editor_checkbox_ac_case->isChecked ());
  settings.setValue (ed_code_completion_replace.settings_key (), editor_checkbox_ac_replace->isChecked ());
  settings.setValue (ed_auto_endif.settings_key (), editor_auto_endif->currentIndex ());
  settings.setValue (ed_show_white_space.settings_key (), editor_ws_checkbox->isChecked ());
  settings.setValue (ed_show_white_space_indent.settings_key (), editor_ws_indent_checkbox->isChecked ());
  settings.setValue (ed_show_eol_chars.settings_key (), cb_show_eol->isChecked ());
  settings.setValue (ed_show_hscroll_bar.settings_key (), cb_show_hscrollbar->isChecked ());
  settings.setValue (ed_default_eol_mode.settings_key (), combo_eol_mode->currentIndex ());

  settings.setValue (ed_tab_position.settings_key (), editor_combox_tab_pos->currentIndex ());
  settings.setValue (ed_tabs_rotated.settings_key (), editor_cb_tabs_rotated->isChecked ());
  settings.setValue (ed_tabs_max_width.settings_key (), editor_sb_tabs_max_width->value ());

  // Comment strings
  int rb_uncomment = 0;
  for (int i = 0; i < ed_comment_strings_count; i++)
    {
      if (m_rb_comment_strings[i]->isChecked ())
        {
          settings.setValue (ed_comment_str.settings_key (), i);
          if (i < 3)
            settings.setValue (ed_comment_str_old.settings_key (), i);
          else
            settings.setValue (ed_comment_str_old.settings_key (), ed_comment_str.def ());
        }
      if (m_rb_uncomment_strings[i]->isChecked ())
        rb_uncomment = rb_uncomment + (1 << i);
    }
  settings.setValue (ed_uncomment_str.settings_key (), rb_uncomment);

  settings.setValue (ed_default_enc.settings_key (), editor_combo_encoding->currentText ());
  settings.setValue (ed_auto_indent.settings_key (), editor_auto_ind_checkbox->isChecked ());
  settings.setValue (ed_tab_indents_line.settings_key (), editor_tab_ind_checkbox->isChecked ());
  settings.setValue (ed_backspace_unindents_line.settings_key (), editor_bs_unind_checkbox->isChecked ());
  settings.setValue (ed_show_indent_guides.settings_key (), editor_ind_guides_checkbox->isChecked ());
  settings.setValue (ed_indent_width.settings_key (), editor_ind_width_spinbox->value ());
  settings.setValue (ed_indent_uses_tabs.settings_key (), editor_ind_uses_tabs_checkbox->isChecked ());
  settings.setValue (ed_tab_width.settings_key (), editor_tab_width_spinbox->value ());
  settings.setValue (ed_restore_session.settings_key (), editor_restoreSession->isChecked ());
  settings.setValue (ed_create_new_file.settings_key (), editor_create_new_file->isChecked ());
  settings.setValue (ed_hiding_closes_files.settings_key (), editor_hiding_closes_files->isChecked ());
  settings.setValue (ed_always_reload_changed_files.settings_key (), editor_reload_changed_files->isChecked ());
  settings.setValue (ed_force_newline.settings_key (), editor_force_newline->isChecked ());
  settings.setValue (ed_rm_trailing_spaces.settings_key (), editor_remove_trailing_spaces->isChecked ());
  settings.setValue (ed_show_dbg_file.settings_key (), editor_show_dbg_file->isChecked ());

  // file browser
  settings.setValue (fb_sync_octdir.settings_key (), sync_octave_directory->isChecked ());
  settings.setValue (fb_restore_last_dir.settings_key (), cb_restore_file_browser_dir->isChecked ());
  settings.setValue (fb_startup_dir.settings_key (), le_file_browser_dir->text ());
  settings.setValue (fb_txt_file_ext.settings_key (), le_file_browser_extensions->text ());

  // network
  settings.setValue (nr_allow_connection.settings_key (), checkbox_allow_web_connect->isChecked ());
  settings.setValue (global_use_proxy.settings_key (), use_proxy_server->isChecked ());
  settings.setValue (global_proxy_type.settings_key (), proxy_type->currentText ());
  settings.setValue (global_proxy_host.settings_key (), proxy_host_name->text ());
  settings.setValue (global_proxy_port.settings_key (), proxy_port->text ());
  settings.setValue (global_proxy_user.settings_key (), proxy_username->text ());
  settings.setValue (global_proxy_pass.settings_key (), proxy_password->text ());

  // command window
  settings.setValue (cs_font_size.settings_key (), terminal_fontSize->value ());
  settings.setValue (cs_font.settings_key (), terminal_fontName->currentFont ().family ());
  settings.setValue (cs_cursor_use_fgcol.settings_key (), terminal_cursorUseForegroundColor->isChecked ());
  settings.setValue (cs_focus_cmd.settings_key (), terminal_focus_command->isChecked ());
  settings.setValue (cs_dbg_location.settings_key (), terminal_print_dbg_location->isChecked ());
  settings.setValue (cs_hist_buffer.settings_key (), terminal_history_buffer->value ());
  write_terminal_colors ();

  // the cursor
  QString cursor_type;
  unsigned int cursor_int = terminal_cursorType->currentIndex ();
  if ((cursor_int > 0) && (cursor_int < cs_cursor_types.size ()))
    cursor_type = QString (cs_cursor_types[cursor_int].data ());
  else
    cursor_type = cs_cursor.def ().toString ();

  settings.setValue (cs_cursor.settings_key (), cursor_type);

#if defined (HAVE_QSCINTILLA)
  // editor styles: create lexer, get dialog contents, and write settings
  QsciLexer *lexer;

#if defined (HAVE_LEXER_OCTAVE)

  lexer = new QsciLexerOctave ();
  write_lexer_settings (lexer);
  delete lexer;

#elif defined (HAVE_LEXER_MATLAB)

  lexer = new QsciLexerMatlab ();
  write_lexer_settings (lexer);
  delete lexer;

#endif

  lexer = new QsciLexerCPP ();
  write_lexer_settings (lexer);
  delete lexer;

  lexer = new QsciLexerJava ();
  write_lexer_settings (lexer);
  delete lexer;

  lexer = new QsciLexerPerl ();
  write_lexer_settings (lexer);
  delete lexer;

  lexer = new QsciLexerBatch ();
  write_lexer_settings (lexer);
  delete lexer;

  lexer = new QsciLexerDiff ();
  write_lexer_settings (lexer);
  delete lexer;

  lexer = new QsciLexerBash ();
  write_lexer_settings (lexer);
  delete lexer;

  lexer = new octave_txt_lexer ();
  write_lexer_settings (lexer);
  delete lexer;

#endif

  // Workspace
  write_workspace_colors ();

  // Variable editor
  settings.setValue (ve_column_width.settings_key (), varedit_columnWidth->value ());
  settings.setValue (ve_row_height.settings_key (), varedit_rowHeight->value ());
  settings.setValue (ve_use_terminal_font.settings_key (), varedit_useTerminalFont->isChecked ());
  settings.setValue (ve_alternate_rows.settings_key (), varedit_alternate->isChecked ());
  settings.setValue (ve_font_name.settings_key (), varedit_font->currentFont ().family ());
  settings.setValue (ve_font_size.settings_key (), varedit_fontSize->value ());
  write_varedit_colors ();

  // shortcuts

  settings.setValue (sc_prevent_rl_conflicts.settings_key (), cb_prevent_readline_conflicts->isChecked ());
  settings.setValue (sc_prevent_rl_conflicts_menu.settings_key (), cb_prevent_readline_conflicts_menu->isChecked ());

  shortcuts_treewidget->write_settings ();

  settings.sync ();
}

void settings_dialog::read_workspace_colors ()
{
  gui_settings settings;

  // Construct the grid with all color related settings
  QGridLayout *style_grid = new QGridLayout ();
  QVector<QLabel *> description (ws_colors_count);
  QVector<color_picker *> color (ws_colors_count);

  int column = 0;
  const int color_columns = 3;  // place colors in so many columns
  int row = 0;
  int mode = settings.int_value (ws_color_mode);

  m_ws_enable_colors = new QCheckBox (tr ("Enable attribute colors"));
  style_grid->addWidget (m_ws_enable_colors, row++, column, 1, 4);

  m_ws_hide_tool_tips = new QCheckBox (tr ("Hide tools tips"));
  style_grid->addWidget (m_ws_hide_tool_tips, row++, column, 1, 4);
  connect (m_ws_enable_colors, &QCheckBox::toggled,
           m_ws_hide_tool_tips, &QCheckBox::setEnabled);
  m_ws_hide_tool_tips->setChecked
    (settings.bool_value (ws_hide_tool_tips));

  QCheckBox *cb_color_mode = new QCheckBox (tr (settings_color_modes.toStdString ().data ()));
  cb_color_mode->setToolTip (tr (settings_color_modes_tooltip.toStdString ().data ()));
  cb_color_mode->setChecked (mode == 1);
  cb_color_mode->setObjectName (ws_color_mode.settings_key ());
  connect (m_ws_enable_colors, &QCheckBox::toggled,
           cb_color_mode, &QCheckBox::setEnabled);
  style_grid->addWidget (cb_color_mode, row, column);

  QPushButton *pb_reload_default_colors = new QPushButton (tr (settings_reload_colors.toStdString ().data ()));
  pb_reload_default_colors->setToolTip (tr (settings_reload_colors_tooltip.toStdString ().data ()));
  connect (m_ws_enable_colors, &QCheckBox::toggled,
           pb_reload_default_colors, &QPushButton::setEnabled);
  style_grid->addWidget (pb_reload_default_colors, row+1, column++);

  bool colors_enabled = settings.bool_value (ws_enable_colors);

  for (int i = 0; i < ws_colors_count; i++)
    {
      description[i] = new QLabel ("    "
        + tr (ws_color_names.at (i).toStdString ().data ()));
      description[i]->setAlignment (Qt::AlignRight);
      description[i]->setEnabled (colors_enabled);
      connect (m_ws_enable_colors, &QCheckBox::toggled,
               description[i], &QLabel::setEnabled);

      QColor setting_color = settings.color_value (ws_colors[i], mode);
      color[i] = new color_picker (setting_color);
      color[i]->setObjectName (ws_colors[i].settings_key ());
      color[i]->setMinimumSize (30, 10);
      color[i]->setEnabled (colors_enabled);
      connect (m_ws_enable_colors, &QCheckBox::toggled,
               color[i], &color_picker::setEnabled);

      style_grid->addWidget (description[i], row, 3*column);
      style_grid->addWidget (color[i], row, 3*column+1);
      if (++column > color_columns)
        {
          style_grid->setColumnStretch (4*column, 10);
          row++;
          column = 1;
        }
    }

  // Load enable settings at the end for having signals already connected
  m_ws_enable_colors->setChecked (colors_enabled);
  m_ws_hide_tool_tips->setEnabled (colors_enabled);
  cb_color_mode->setEnabled (colors_enabled);
  pb_reload_default_colors->setEnabled (colors_enabled);

  // place grid with elements into the tab
  workspace_colors_box->setLayout (style_grid);

  // update colors depending on second theme selection or reloading
  // the dfault values
  connect (cb_color_mode, &QCheckBox::stateChanged,
           this, &settings_dialog::update_workspace_colors);
  connect (pb_reload_default_colors, &QPushButton::clicked,
           [=] () { update_workspace_colors (settings_reload_default_colors_flag); });
}

void settings_dialog::update_workspace_colors (int def)
{
  QCheckBox *cb_color_mode
    = workspace_colors_box->findChild <QCheckBox *> (ws_color_mode.settings_key ());

  int m = 0;
  if (cb_color_mode && cb_color_mode->isChecked ())
    m = 1;

  gui_settings settings;

  color_picker *c_picker;

  for (unsigned int i = 0; i < ws_colors_count; i++)
    {
      c_picker = workspace_colors_box->findChild <color_picker *> (ws_colors[i].settings_key ());
      if (c_picker)
        {
          if (def != settings_reload_default_colors_flag)
            {
              // Get current value from settings or the default
              c_picker->set_color (settings.color_value (ws_colors[i], m));
            }
          else
            {
              // Get the default value
              c_picker->set_color (settings.get_color_value (ws_colors[i].def (), m));
            }
        }
    }
}

void settings_dialog::write_workspace_colors ()
{
  gui_settings settings;

  settings.setValue (ws_enable_colors.settings_key (), m_ws_enable_colors->isChecked ());
  settings.setValue (ws_hide_tool_tips.settings_key (), m_ws_hide_tool_tips->isChecked ());

  QCheckBox *cb_color_mode
    = workspace_colors_box->findChild <QCheckBox *> (ws_color_mode.settings_key ());

  int mode = 0;
  if (cb_color_mode && cb_color_mode->isChecked ())
    mode = 1;

  color_picker *color;

  for (int i = 0; i < ws_colors_count; i++)
    {
      color = workspace_colors_box->findChild <color_picker *> (ws_colors[i].settings_key ());
      if (color)
        settings.set_color_value (ws_colors[i], color->color (), mode);
    }

  settings.setValue (ws_color_mode.settings_key (), mode);

  settings.sync ();
}

void settings_dialog::read_terminal_colors ()
{
  gui_settings settings;

  QGridLayout *style_grid = new QGridLayout ();
  QVector<QLabel *> description (cs_colors_count);
  QVector<color_picker *> color (cs_colors_count);

  int mode = settings.int_value (cs_color_mode);

  QCheckBox *cb_color_mode = new QCheckBox (tr (settings_color_modes.toStdString ().data ()));
  cb_color_mode->setToolTip (tr (settings_color_modes_tooltip.toStdString ().data ()));
  cb_color_mode->setChecked (mode == 1);
  cb_color_mode->setObjectName (cs_color_mode.settings_key ());
  style_grid->addWidget (cb_color_mode, 0, 0);

  QPushButton *pb_reload_default_colors = new QPushButton (tr (settings_reload_colors.toStdString ().data ()));
  pb_reload_default_colors->setToolTip (tr (settings_reload_colors_tooltip.toStdString ().data ()));
  style_grid->addWidget (pb_reload_default_colors, 1, 0);

  int column = 1;               // column 0 is for the color mode checkbox
  const int color_columns = 2;  // place colors in so many columns
  int row = 0;
  for (unsigned int i = 0; i < cs_colors_count; i++)
    {
      description[i] = new QLabel ("    "
          + tr (cs_color_names.at (i).toStdString ().data ()));
      description[i]->setAlignment (Qt::AlignRight);
      QColor setting_color = settings.color_value (cs_colors[i], mode);
      color[i] = new color_picker (setting_color);
      color[i]->setObjectName (cs_colors[i].settings_key ());
      color[i]->setMinimumSize (30, 10);
      style_grid->addWidget (description[i], row, 2*column);
      style_grid->addWidget (color[i], row, 2*column+1);
      if (++column > color_columns)
        {
          style_grid->setColumnStretch (3*column, 10);
          row++;
          column = 1;
        }
    }

  // place grid with elements into the tab
  terminal_colors_box->setLayout (style_grid);

  // update colors depending on second theme selection
  connect (cb_color_mode, &QCheckBox::stateChanged,
           this, &settings_dialog::update_terminal_colors);
  connect (pb_reload_default_colors, &QPushButton::clicked,
           [=] () { update_terminal_colors (settings_reload_default_colors_flag); });
}

void settings_dialog::update_terminal_colors (int def)
{
  QCheckBox *cb_color_mode
    = terminal_colors_box->findChild <QCheckBox *> (cs_color_mode.settings_key ());

  int m = 0;
  if (cb_color_mode && cb_color_mode->isChecked ())
    m = 1;

  gui_settings settings;

  color_picker *c_picker;

  for (unsigned int i = 0; i < cs_colors_count; i++)
    {
      c_picker = terminal_colors_box->findChild <color_picker *> (cs_colors[i].settings_key ());
      if (c_picker)
        {
          if (def != settings_reload_default_colors_flag)
            {
              // Get current value from settings or the default
              c_picker->set_color (settings.color_value (cs_colors[i], m));
            }
          else
            {
              // Get the default value
              c_picker->set_color (settings.get_color_value (cs_colors[i].def (), m));
            }
        }
    }
}

void settings_dialog::write_terminal_colors ()
{
  QCheckBox *cb_color_mode
    = terminal_colors_box->findChild <QCheckBox *> (cs_color_mode.settings_key ());

  int mode = 0;
  if (cb_color_mode && cb_color_mode->isChecked ())
    mode = 1;

  gui_settings settings;

  color_picker *color;

  for (int i = 0; i < cs_color_names.size (); i++)
    {
      color = terminal_colors_box->findChild <color_picker *> (cs_colors[i].settings_key ());
      if (color)
        settings.set_color_value (cs_colors[i], color->color (), mode);
    }

  settings.setValue (cs_color_mode.settings_key (), mode);

  settings.sync ();
}

void settings_dialog::read_varedit_colors ()
{
  gui_settings settings;

  QGridLayout *style_grid = new QGridLayout ();
  QVector<QLabel *> description (ve_colors_count);
  QVector<color_picker *> color (ve_colors_count);

  int mode = settings.int_value (ve_color_mode);

  QCheckBox *cb_color_mode = new QCheckBox (tr (settings_color_modes.toStdString ().data ()));
  cb_color_mode->setToolTip (tr (settings_color_modes_tooltip.toStdString ().data ()));
  cb_color_mode->setChecked (mode == 1);
  cb_color_mode->setObjectName (ve_color_mode.settings_key ());
  style_grid->addWidget (cb_color_mode, 0, 0);

  QPushButton *pb_reload_default_colors = new QPushButton (tr (settings_reload_colors.toStdString ().data ()));
  pb_reload_default_colors->setToolTip (tr (settings_reload_colors_tooltip.toStdString ().data ()));
  style_grid->addWidget (pb_reload_default_colors, 1, 0);

  int column = 1;
  int color_columns = 2;
  int row = 0;
  for (int i = 0; i < ve_colors_count; i++)
    {
      description[i] = new QLabel ("    "
          + tr (ve_color_names.at (i).toStdString ().data ()));
      description[i]->setAlignment (Qt::AlignRight);

      QColor setting_color = settings.color_value (ve_colors[i], mode);
      color[i] = new color_picker (setting_color);
      color[i]->setObjectName (ve_colors[i].settings_key ());
      color[i]->setMinimumSize (30, 10);
      style_grid->addWidget (description[i], row, 2*column);
      style_grid->addWidget (color[i], row, 2*column+1);
      if (++column > color_columns)
        {
          style_grid->setColumnStretch (3*column, 10);
          row++;
          column = 1;
        }
    }

  // place grid with elements into the tab
  varedit_colors_box->setLayout (style_grid);

  // update colors depending on second theme selection
  connect (cb_color_mode, &QCheckBox::stateChanged,
           this, &settings_dialog::update_varedit_colors);
  connect (pb_reload_default_colors, &QPushButton::clicked,
           [=] () { update_varedit_colors (settings_reload_default_colors_flag); });
}

void settings_dialog::update_varedit_colors (int def)
{
  QCheckBox *cb_color_mode
    = varedit_colors_box->findChild <QCheckBox *> (ve_color_mode.settings_key ());

  int m = 0;
  if (cb_color_mode && cb_color_mode->isChecked ())
    m = 1;

  gui_settings settings;

  color_picker *c_picker;

  for (unsigned int i = 0; i < ve_colors_count; i++)
    {
      c_picker = varedit_colors_box->findChild <color_picker *> (ve_colors[i].settings_key ());
      if (c_picker)
        {
          if (def != settings_reload_default_colors_flag)
            {
              // Get current value from settings or the default
              c_picker->set_color (settings.color_value (ve_colors[i], m));
            }
          else
            {
              // Get the default value
              c_picker->set_color (settings.get_color_value (ve_colors[i].def (), m));
            }
        }
    }
}

void settings_dialog::write_varedit_colors ()
{
  QCheckBox *cb_color_mode
    = varedit_colors_box->findChild <QCheckBox *> (ve_color_mode.settings_key ());

  int mode = 0;
  if (cb_color_mode && cb_color_mode->isChecked ())
    mode = 1;

  gui_settings settings;

  color_picker *color;

  for (int i = 0; i < ve_colors_count; i++)
    {
      color = varedit_colors_box->findChild <color_picker *> (ve_colors[i].settings_key ());
      if (color)
        settings.set_color_value (ve_colors[i], color->color (), mode);
    }

  settings.setValue (ve_color_mode.settings_key (), mode);

  settings.sync ();
}

QString settings_dialog::get_shortcuts_file_name (import_export_action action)
{
  QString file;

  // FIXME: Remove, if for all common KDE versions (bug #54607) is resolved.
  int opts = 0;  // No options by default.

  gui_settings settings;

  if (! settings.bool_value (global_use_native_dialogs))
    opts = QFileDialog::DontUseNativeDialog;

  if (action == OSC_IMPORT)
    file = QFileDialog::getOpenFileName
      (this, tr ("Import shortcuts from file..."), QString (),
       tr ("Octave Shortcut Files (*.osc);;All Files (*)"),
       nullptr, QFileDialog::Option (opts));

  else
    file = QFileDialog::getSaveFileName
      (this, tr ("Export shortcuts to file..."), QString (),
       tr ("Octave Shortcut Files (*.osc);;All Files (*)"),
       nullptr, QFileDialog::Option (opts));

  return file;
}

// Ask whether to overwrite current shortcuts with settings from an
// imported file.  Optionally allow current shortcuts to be saved to a
// file.

// FIXME: If the tree view contains changes that have not yet been
//        saved to the application settings object, should we
//
//   * allow the user to choose whether to
//     - cancel the operation (X)
//     - save the modified settings (X)
//     - save the current application settings (XX)
//
//   * unconditionally display an error dialog and cancel the
//     export operation
//
//   (X) - already an option, but not based on whether the tree view
//         contains unsaved changes
//   (XX) - already possible (cancel operation, cancel settings
//          dialog, re-open settings dialog and export changes).

bool settings_dialog::overwrite_all_shortcuts ()
{
  QMessageBox msg_box;

  msg_box.setWindowTitle (tr ("Overwriting Shortcuts"));
  msg_box.setIcon (QMessageBox::Warning);
  msg_box.setText (tr ("You are about to overwrite all shortcuts.\n"
                       "Would you like to save the current shortcut set or cancel the action?"));
  msg_box.setStandardButtons (QMessageBox::Save | QMessageBox::Cancel);

  QPushButton *discard
    = msg_box.addButton (tr ("Don't save"), QMessageBox::DestructiveRole);

  msg_box.setDefaultButton (QMessageBox::Save);

  int ret = msg_box.exec ();

  if (msg_box.clickedButton () == discard)
    return true;

  if (ret == QMessageBox::Save)
    {
      QString file = get_shortcuts_file_name (OSC_EXPORT);

      gui_settings osc_settings (file, QSettings::IniFormat);

      if (osc_settings.status () ==  QSettings::NoError)
        {
          shortcuts_treewidget->export_shortcuts (osc_settings);
          return true;
        }
      else
        qWarning () << (tr ("Failed to open %1 as Octave shortcut file")
                        .arg (file));
    }

  return false;
}

QMessageBox* settings_dialog::wait_message_box (const QString& text, QWidget *p)
{
  QMessageBox *info = new QMessageBox (p);

  info->setIcon (QMessageBox::Information);
  info->setWindowTitle (tr ("Octave GUI preferences"));
  info->setText (text);
  info->setStandardButtons (QMessageBox::Ok);
  info->setAttribute (Qt::WA_DeleteOnClose);
  info->setWindowModality (Qt::NonModal);

  info->show ();
  QThread::msleep (100);
  QCoreApplication::processEvents ();

  QApplication::setOverrideCursor (Qt::WaitCursor);

  return info;
}

void settings_dialog::close_wait_message_box (QMessageBox *mbox)
{
  QApplication::restoreOverrideCursor ();
  mbox->close ();
}

OCTAVE_END_NAMESPACE(octave)
