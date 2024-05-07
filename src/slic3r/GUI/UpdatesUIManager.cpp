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
#include "MsgDialog.hpp"
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

    fill_entries(true);
    fill_grids();
}

void UIManager::fill_entries(bool init_selection/* = false*/)
{
    m_online_entries.clear();
    m_offline_entries.clear();

    const ArchiveRepositoryVector&  archs               = m_pad->get_archive_repositories();
    const std::map<std::string, bool>& selected_repos   = m_pad->get_selected_repositories_uuid();

    for (const auto& archive : archs) {
        const std::string&  uuid   = archive->get_uuid();
        auto                sel_it = selected_repos.find(uuid);
        assert(sel_it != selected_repos.end());
        if (init_selection && sel_it->second)
            m_selected_uuids.emplace(uuid);

        const bool  is_selected = m_selected_uuids.find(uuid) != m_selected_uuids.end();
        const auto& data        = archive->get_manifest();

        if (data.source_path.empty()) {
            // online repo
            m_online_entries.push_back({ is_selected, uuid, data.name, data.description, data.visibility });
        }
        else {
            // offline repo
            m_offline_entries.push_back({ is_selected, uuid, data.name, data.description, data.source_path.filename().string(), fs::exists(data.source_path) });
        }
    }
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
                    m_selected_uuids.emplace(entry.id);
                else
                    m_selected_uuids.erase(entry.id);
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

        for (const wxString& l : std::initializer_list<wxString>{ _L("Use"), _L("Name"), _L("Descrition"), "", _L("Source file"), "" }) {
            auto text = new wxStaticText(m_parent, wxID_ANY, l);
            text->SetFont(wxGetApp().bold_font());
            add(text);
        }

        // data1

        for (const auto& entry : m_offline_entries)
        {
            auto chb = CheckBox::GetNewWin(m_parent, "");
            CheckBox::SetValue(chb, entry.use);
            chb->Bind(wxEVT_CHECKBOX, [this, chb, &entry](wxCommandEvent e) {
                if (CheckBox::GetValue(chb))
                    m_selected_uuids.emplace(entry.id);
                else
                    m_selected_uuids.erase(entry.id);
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

    wxWindowUpdateLocker freeze_guard(m_parent);

    fill_grids();

    m_parent->GetSizer()->Layout();

    if (wxDialog* dlg = dynamic_cast<wxDialog*>(m_parent)) {
        m_parent->Layout();
        m_parent->Refresh();
        dlg->Fit();
    }
    else if (wxWindow* top_parent = m_parent->GetParent()) {
        top_parent->Layout();
        top_parent->Refresh();
    }
}

void UIManager::remove_offline_repos(const std::string& id)
{
    m_pad->remove_local_archive(id);
    m_selected_uuids.erase(id);

    if (wxDialog* dlg = dynamic_cast<wxDialog*>(m_parent)) {
        // Invalidate min_size for correct next Layout()
        dlg->SetMinSize(wxDefaultSize);
    }

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
            std::string msg;
            std::string uuid = m_pad->add_local_archive(input_path, msg);
            if (uuid.empty()) {
                ErrorDialog(m_parent, msg, false).ShowModal();
            }
            else {
                m_selected_uuids.emplace(uuid);
                update();
            }
        }
        catch (fs::filesystem_error const& e) {
            std::cerr << e.what() << '\n';
        }
    }
}

bool UIManager::set_selected_repositories()
{
    std::vector<std::string> used_ids;
    std::copy(m_selected_uuids.begin(), m_selected_uuids.end(), std::back_inserter(used_ids));

    std::string msg;
    if (m_pad->set_selected_repositories(used_ids, msg))
        return true;

    ErrorDialog(m_parent, msg, false).ShowModal();
    // update selection on UI
    update();
    return false;
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
    if (m_manager->set_selected_repositories())
        this->EndModal(wxID_CLOSE);
}

} // namespace GUI
} // namespace Slic3r
