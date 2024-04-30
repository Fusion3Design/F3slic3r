///|/ Copyright (c) Prusa Research 2018 - 2023 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "UpdatesUIManager.hpp"
#include "I18N.hpp"
#include "wxExtensions.hpp"
#include "PresetArchiveDatabase.hpp"

#include "GUI.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "format.hpp"

#include "Widgets/CheckBox.hpp"

namespace fs = boost::filesystem;

namespace Slic3r { 
namespace GUI {

UIManager::UIManager(wxWindow* parent, PresetArchiveDatabase* pad, int em) :
    m_parent(parent)
    ,m_pad(pad)
    ,m_main_sizer(new wxBoxSizer(wxVERTICAL))
{
    auto online_label = new wxStaticText(m_parent, wxID_ANY, _L("Online Repositories"));
    online_label->SetFont(wxGetApp().bold_font());

    m_main_sizer->Add(online_label, 0, wxTOP | wxLEFT, 2 * em);

    m_online_sizer = new wxFlexGridSizer(4, 0.75 * em, 1.5 * em);
    m_online_sizer->AddGrowableCol(2);
    m_online_sizer->AddGrowableCol(3);
    m_online_sizer->SetFlexibleDirection(/*wxHORIZONTAL*/wxBOTH);

    m_main_sizer->Add(m_online_sizer, 0, wxALL, 2 * em);

    m_main_sizer->AddSpacer(em);

    auto offline_label = new wxStaticText(m_parent, wxID_ANY, _L("Offline Repositories"));
    offline_label->SetFont(wxGetApp().bold_font());

    m_main_sizer->Add(offline_label, 0, wxTOP | wxLEFT, 2 * em);

    m_offline_sizer = new wxFlexGridSizer(6, 0.75 * em, 1.5 * em);
    m_offline_sizer->AddGrowableCol(1);
    m_offline_sizer->AddGrowableCol(2);
    m_offline_sizer->AddGrowableCol(4);
    m_offline_sizer->SetFlexibleDirection(wxHORIZONTAL);

    m_main_sizer->Add(m_offline_sizer, 0, wxALL, 2 * em);

    fill_entries();
    fill_grids();
}

void UIManager::fill_entries()
{
    m_online_entries.clear();
    m_offline_entries.clear();

    const ArchiveRepositoryVector&  archs       = m_pad->get_archives();
    const std::vector<std::string>& used_archs  = m_pad->get_used_archives();

    for (const auto& archive : archs) {
        const auto& data = archive->get_manifest();
        if (data.local_path.empty()) {
            // online repo
            bool is_used = std::find(used_archs.begin(), used_archs.end(), data.id) != used_archs.end();
            m_online_entries.push_back({is_used, data.id, data.name, data.description, data.visibility });
        } 
        else {
            // offline repo
            bool is_used = std::find(used_archs.begin(), used_archs.end(), data.id) != used_archs.end();
            m_offline_entries.push_back({is_used, data.id, data.name, data.description, data.local_path.filename().string(), fs::exists(data.local_path)});
        }
    }

#if 0 // ysFIXME_delete 
    // Next code is just for testing

    if (m_offline_entries.empty())
        m_offline_entries = {
            {true,  "333", "Prusa AFS"    , "Prusa FDM Prusa FDM Prusa FDM" , "/path/field/file1.zip", false},
            {false, "444", "Prusa Trilab" , "Prusa sla Prusa sla Prusa sla" , "/path/field/file2.zip", true},
        };
#endif
}


void UIManager::fill_grids()
{
    // clear grids
    m_online_sizer->Clear(true);
    m_offline_sizer->Clear(true);

    // Fill Online repository

    if (!m_online_entries.empty()) {

        auto add = [this](wxWindow* win) { m_online_sizer->Add(win, 0, wxALIGN_CENTER_VERTICAL); };

        // header

        for (const wxString& l : std::initializer_list<wxString>{ _L("Use"), "", _L("Name"), _L("Descrition") }) {
            auto text = new wxStaticText(m_parent, wxID_ANY, l);
            text->SetFont(wxGetApp().bold_font());
            add(text);
        }

        // data

        for (const auto& entry : m_online_entries)
        {
            auto chb = CheckBox::GetNewWin(m_parent, "");
            CheckBox::SetValue(chb, entry.use);
            chb->Bind(wxEVT_CHECKBOX, [this, chb, &entry](wxCommandEvent e) {
                if (CheckBox::GetValue(chb))
                    m_online_selections.emplace(entry.id);
                else
                    m_online_selections.erase(entry.id);
                });
            add(chb);

            if (entry.visibility.empty())
                add(new wxStaticText(m_parent, wxID_ANY, ""));
            else {
                wxStaticBitmap* bmp = new wxStaticBitmap(m_parent, wxID_ANY, *get_bmp_bundle("info"));
                bmp->SetToolTip(from_u8(entry.visibility));
                add(bmp);
            }

            add(new wxStaticText(m_parent, wxID_ANY, from_u8(entry.name)));

            add(new wxStaticText(m_parent, wxID_ANY, from_u8(entry.description)));
        }
    }

    if (!m_offline_entries.empty()) {

        auto add = [this](wxWindow* win) { m_offline_sizer->Add(win, 0, wxALIGN_CENTER_VERTICAL); };

        // header

        for (const wxString& l : std::initializer_list<wxString>{ _L("Use"), _L("Name"), _L("Descrition"), "", _L("Sourse file"), "" }) {
            auto text = new wxStaticText(m_parent, wxID_ANY, l);
            text->SetFont(wxGetApp().bold_font());
            add(text);
        }

        // data

        for (const auto& entry : m_offline_entries)
        {
            auto chb = CheckBox::GetNewWin(m_parent, "");
            CheckBox::SetValue(chb, entry.use);
            chb->Bind(wxEVT_CHECKBOX, [this, chb, &entry](wxCommandEvent e) {
                if (CheckBox::GetValue(chb))
                    m_offline_selections.emplace(entry.id);
                else
                    m_offline_selections.erase(entry.id);
                });
            add(chb);

            add(new wxStaticText(m_parent, wxID_ANY, from_u8(entry.name)));

            add(new wxStaticText(m_parent, wxID_ANY, from_u8(entry.description)));

            {
                wxStaticBitmap* bmp = new wxStaticBitmap(m_parent, wxID_ANY, *get_bmp_bundle(entry.is_ok ? "tick_mark" : "exclamation"));
                bmp->SetToolTip(entry.is_ok ? _L("Exists") : _L("Doesn't exist"));
                add(bmp);
            }

            add(new wxStaticText(m_parent, wxID_ANY, from_u8(entry.source)));

            {
                ScalableButton* btn = new ScalableButton(m_parent, wxID_ANY, "", "  " + _L("Remove") + "  ");
                wxGetApp().UpdateDarkUI(btn, true);
                btn->Bind(wxEVT_BUTTON, [this, &entry](wxCommandEvent& event) { remove_offline_repos(entry.id); });
                add(btn);
            }
        }
    }

    {
        ScalableButton* btn = new ScalableButton(m_parent, wxID_ANY, "", "  " + _L("Load") + "...  ");
        wxGetApp().UpdateDarkUI(btn, true);
        btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) { load_offline_repos(); });
        m_offline_sizer->Add(btn);
    }

}

void UIManager::update()
{
    fill_entries();
    fill_grids();

    m_main_sizer->Layout();
}

void UIManager::remove_offline_repos(const std::string& id)
{
    m_pad->remove_local_archive(id);

    update();
}

void UIManager::load_offline_repos()
{
    wxArrayString input_files;
    wxFileDialog dialog(m_parent, _L("Choose one or more YIP-files") + ":",
        from_u8(wxGetApp().app_config->get_last_dir()), "",
        file_wildcards(FT_ZIP), wxFD_OPEN | /*wxFD_MULTIPLE | */wxFD_FILE_MUST_EXIST);

    if (dialog.ShowModal() == wxID_OK)
        dialog.GetPaths(input_files);

    if (input_files.IsEmpty())
        return;

    // Iterate through the input files
    for (size_t i = 0; i < input_files.size(); ++i) {
        std::string input_file = into_u8(input_files.Item(i));
        try {
            fs::path input_path = fs::path(input_file);
            m_pad->add_local_archive(input_path);
        }
        catch (fs::filesystem_error const& e) {
            std::cerr << e.what() << '\n';
        }
    }

    update();
}

void UIManager::set_used_archives()
{
    std::vector<std::string> used_ids;
    for (const std::string& id : m_online_selections)
        used_ids.push_back(id);
    for (const std::string& id : m_offline_selections)
        used_ids.push_back(id);

    m_pad->set_used_archives(used_ids);
}


ManageUpdatesDialog::ManageUpdatesDialog(PresetArchiveDatabase* pad)
    : DPIDialog(static_cast<wxWindow*>(wxGetApp().mainframe), wxID_ANY,
        format_wxstr("%1% - %2%", SLIC3R_APP_NAME, _L("Manage Updates")),
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    this->SetFont(wxGetApp().normal_font());
    const int em = em_unit();

    m_manager = std::make_unique<UIManager>(this, pad, em);

    auto sizer = m_manager->get_sizer();

    wxStdDialogButtonSizer* buttons = this->CreateStdDialogButtonSizer(wxOK | wxCLOSE);
    wxGetApp().SetWindowVariantForButton(buttons->GetCancelButton());
    wxGetApp().UpdateDlgDarkUI(this, true);
    this->SetEscapeId(wxID_CLOSE);
    this->Bind(wxEVT_BUTTON, &ManageUpdatesDialog::onCloseDialog, this, wxID_CLOSE);
    this->Bind(wxEVT_BUTTON, &ManageUpdatesDialog::onOkDialog, this, wxID_OK);
    sizer->Add(buttons, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, em);

    SetSizer(sizer);
    sizer->SetSizeHints(this);
}

void ManageUpdatesDialog::on_dpi_changed(const wxRect &suggested_rect)
{
    SetMinSize(GetBestSize());
    Fit();
    Refresh();
}

void ManageUpdatesDialog::onCloseDialog(wxEvent &)
{
     this->EndModal(wxID_CLOSE);
}

void ManageUpdatesDialog::onOkDialog(wxEvent&)
{
    m_manager->set_used_archives();
    this->EndModal(wxID_CLOSE);
}

} // namespace GUI
} // namespace Slic3r
