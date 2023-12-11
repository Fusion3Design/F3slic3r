#ifndef slic3r_TopBar_hpp_
#define slic3r_TopBar_hpp_

//#ifdef _WIN32

#include <wx/bookctrl.h>
#include "wxExtensions.hpp"

class ModeSizer;
//class ScalableButton;

// custom message the TopBarItemsCtrl sends to its parent (TopBar) to notify a selection change:
wxDECLARE_EVENT(wxCUSTOMEVT_TOPBAR_SEL_CHANGED, wxCommandEvent);

class TopBarItemsCtrl : public wxControl
{
    class Button : public ScalableButton
    {
        bool    m_is_selected{ false };
    public:
        Button() {};
        Button( wxWindow*           parent,
                const wxString&     label,
                const std::string&  icon_name = "",
                const int           px_cnt = 16);

        ~Button() {}

        void set_selected(bool selected);
    };

    class ButtonWithPopup : public Button
    {
    public:
        ButtonWithPopup() {};
        ButtonWithPopup(wxWindow*           parent,
                        const wxString&     label,
                        const std::string&  icon_name = "");
        ButtonWithPopup(wxWindow*           parent,
                        const std::string&  icon_name,
                        int                 icon_width = 20,
                        int                 icon_height = 20);

        ~ButtonWithPopup() {}

        void SetLabel(const wxString& label) override;
    };

    wxMenu          m_main_menu;
    wxMenu          m_workspaces_menu;
    wxMenu          m_auth_menu;

    wxMenuItem*     m_user_menu_item{ nullptr };
    wxMenuItem*     m_login_menu_item{ nullptr };
    wxMenuItem*     m_connect_dummy_menu_item{ nullptr };

public:
    TopBarItemsCtrl(wxWindow* parent);
    ~TopBarItemsCtrl() {}

    void OnPaint(wxPaintEvent&);
    void SetSelection(int sel);
    void UpdateMode();
    void Rescale();
    void OnColorsChanged();
    void UpdateModeMarkers();
    void UpdateSelection();
    bool InsertPage(size_t n, const wxString& text, bool bSelect = false, const std::string& bmp_name = "");
    void RemovePage(size_t n);
    void SetPageText(size_t n, const wxString& strText);
    wxString GetPageText(size_t n) const;

    void AppendMenuItem(wxMenu* menu, const wxString& title);
    void AppendMenuSeparaorItem();
    void ApplyWorkspacesMenu();
    void CreateAuthMenu();
    void UpdateAuthMenu();

private:
    wxWindow*                       m_parent;
    wxFlexGridSizer*                m_buttons_sizer;
    wxBoxSizer*                     m_sizer;
    ButtonWithPopup*                m_menu_btn {nullptr};
    ButtonWithPopup*                m_workspace_btn {nullptr};
    ButtonWithPopup*                m_auth_btn {nullptr};
    std::vector<Button*>            m_pageButtons;
    int                             m_selection {-1};
    int                             m_btn_margin;
    int                             m_line_margin;
};

class TopBar : public wxBookCtrlBase
{
public:
    TopBar(wxWindow * parent,
                 wxWindowID winid = wxID_ANY,
                 const wxPoint & pos = wxDefaultPosition,
                 const wxSize & size = wxDefaultSize,
                 long style = 0)
    {
        Init();
        Create(parent, winid, pos, size, style);
    }

    bool Create(wxWindow * parent,
                wxWindowID winid = wxID_ANY,
                const wxPoint & pos = wxDefaultPosition,
                const wxSize & size = wxDefaultSize,
                long style = 0)
    {
        if (!wxBookCtrlBase::Create(parent, winid, pos, size, style | wxBK_TOP))
            return false;

        m_bookctrl = new TopBarItemsCtrl(this);

        wxSizer* mainSizer = new wxBoxSizer(IsVertical() ? wxVERTICAL : wxHORIZONTAL);

        if (style & wxBK_RIGHT || style & wxBK_BOTTOM)
            mainSizer->Add(0, 0, 1, wxEXPAND, 0);

        m_controlSizer = new wxBoxSizer(IsVertical() ? wxHORIZONTAL : wxVERTICAL);
        m_controlSizer->Add(m_bookctrl, wxSizerFlags(1).Expand());
        wxSizerFlags flags;
        if (IsVertical())
            flags.Expand();
        else
            flags.CentreVertical();
        mainSizer->Add(m_controlSizer, flags.Border(wxALL, m_controlMargin));
        SetSizer(mainSizer);

        this->Bind(wxCUSTOMEVT_TOPBAR_SEL_CHANGED, [this](wxCommandEvent& evt)
        {                    
            if (int page_idx = evt.GetId(); page_idx >= 0)
                SetSelection(page_idx);
        });

        this->Bind(wxEVT_NAVIGATION_KEY, &TopBar::OnNavigationKey, this);

        return true;
    }


    // Methods specific to this class.

    // A method allowing to add a new page without any label (which is unused
    // by this control) and show it immediately.
    bool ShowNewPage(wxWindow * page)
    {
        return AddPage(page, wxString(), ""/*true *//* select it */);
    }


    // Set effect to use for showing/hiding pages.
    void SetEffects(wxShowEffect showEffect, wxShowEffect hideEffect)
    {
        m_showEffect = showEffect;
        m_hideEffect = hideEffect;
    }

    // Or the same effect for both of them.
    void SetEffect(wxShowEffect effect)
    {
        SetEffects(effect, effect);
    }

    // And the same for time outs.
    void SetEffectsTimeouts(unsigned showTimeout, unsigned hideTimeout)
    {
        m_showTimeout = showTimeout;
        m_hideTimeout = hideTimeout;
    }

    void SetEffectTimeout(unsigned timeout)
    {
        SetEffectsTimeouts(timeout, timeout);
    }


    // Implement base class pure virtual methods.

    // adds a new page to the control
    bool AddPage(wxWindow* page,
                 const wxString& text,
                 const std::string& bmp_name,
                 bool bSelect = false)
    {
        DoInvalidateBestSize();
        return InsertPage(GetPageCount(), page, text, bmp_name, bSelect);
    }

    // Page management
    virtual bool InsertPage(size_t n,
                            wxWindow * page,
                            const wxString & text,
                            bool bSelect = false,
                            int imageId = NO_IMAGE) override
    {
        if (!wxBookCtrlBase::InsertPage(n, page, text, bSelect, imageId))
            return false;

        GetTopBarItemsCtrl()->InsertPage(n, text, bSelect);

        if (!DoSetSelectionAfterInsertion(n, bSelect))
            page->Hide();

        return true;
    }

    bool InsertPage(size_t n,
                    wxWindow * page,
                    const wxString & text,
                    const std::string& bmp_name = "",
                    bool bSelect = false)
    {
        if (!wxBookCtrlBase::InsertPage(n, page, text, bSelect))
            return false;

        GetTopBarItemsCtrl()->InsertPage(n, text, bSelect, bmp_name);

        if (bSelect)
            SetSelection(n);

        return true;
    }

    virtual int SetSelection(size_t n) override
    {
        GetTopBarItemsCtrl()->SetSelection(n);
        int ret = DoSetSelection(n, SetSelection_SendEvent);

        // check that only the selected page is visible and others are hidden:
        for (size_t page = 0; page < m_pages.size(); page++)
            if (page != n)
                m_pages[page]->Hide();

        return ret;
    }

    virtual int ChangeSelection(size_t n) override
    {
        GetTopBarItemsCtrl()->SetSelection(n);
        return DoSetSelection(n);
    }

    // Neither labels nor images are supported but we still store the labels
    // just in case the user code attaches some importance to them.
    virtual bool SetPageText(size_t n, const wxString & strText) override
    {
        wxCHECK_MSG(n < GetPageCount(), false, wxS("Invalid page"));

        GetTopBarItemsCtrl()->SetPageText(n, strText);

        return true;
    }

    virtual wxString GetPageText(size_t n) const override
    {
        wxCHECK_MSG(n < GetPageCount(), wxString(), wxS("Invalid page"));
        return GetTopBarItemsCtrl()->GetPageText(n);
    }

    virtual bool SetPageImage(size_t WXUNUSED(n), int WXUNUSED(imageId)) override
    {
        return false;
    }

    virtual int GetPageImage(size_t WXUNUSED(n)) const override
    {
        return NO_IMAGE;
    }

    // Override some wxWindow methods too.
    virtual void SetFocus() override
    {
        wxWindow* const page = GetCurrentPage();
        if (page)
            page->SetFocus();
    }

    TopBarItemsCtrl* GetTopBarItemsCtrl() const { return static_cast<TopBarItemsCtrl*>(m_bookctrl); }

    void UpdateMode()
    {
        GetTopBarItemsCtrl()->UpdateMode();
    }

    void Rescale()
    {
        GetTopBarItemsCtrl()->Rescale();
    }

    void OnColorsChanged()
    {
        GetTopBarItemsCtrl()->OnColorsChanged();
    }

    void UpdateModeMarkers()
    {
        GetTopBarItemsCtrl()->UpdateModeMarkers();
    }

    void OnNavigationKey(wxNavigationKeyEvent& event)
    {
        if (event.IsWindowChange()) {
            // change pages
            AdvanceSelection(event.GetDirection());
        }
        else {
            // we get this event in 3 cases
            //
            // a) one of our pages might have generated it because the user TABbed
            // out from it in which case we should propagate the event upwards and
            // our parent will take care of setting the focus to prev/next sibling
            //
            // or
            //
            // b) the parent panel wants to give the focus to us so that we
            // forward it to our selected page. We can't deal with this in
            // OnSetFocus() because we don't know which direction the focus came
            // from in this case and so can't choose between setting the focus to
            // first or last panel child
            //
            // or
            //
            // c) we ourselves (see MSWTranslateMessage) generated the event
            //
            wxWindow* const parent = GetParent();

            // the wxObject* casts are required to avoid MinGW GCC 2.95.3 ICE
            const bool isFromParent = event.GetEventObject() == (wxObject*)parent;
            const bool isFromSelf = event.GetEventObject() == (wxObject*)this;
            const bool isForward = event.GetDirection();

            if (isFromSelf && !isForward)
            {
                // focus is currently on notebook tab and should leave
                // it backwards (Shift-TAB)
                event.SetCurrentFocus(this);
                parent->HandleWindowEvent(event);
            }
            else if (isFromParent || isFromSelf)
            {
                // no, it doesn't come from child, case (b) or (c): forward to a
                // page but only if entering notebook page (i.e. direction is
                // backwards (Shift-TAB) comething from out-of-notebook, or
                // direction is forward (TAB) from ourselves),
                if (m_selection != wxNOT_FOUND &&
                    (!event.GetDirection() || isFromSelf))
                {
                    // so that the page knows that the event comes from it's parent
                    // and is being propagated downwards
                    event.SetEventObject(this);

                    wxWindow* page = m_pages[m_selection];
                    if (!page->HandleWindowEvent(event))
                    {
                        page->SetFocus();
                    }
                    //else: page manages focus inside it itself
                }
                else // otherwise set the focus to the notebook itself
                {
                    SetFocus();
                }
            }
            else
            {
                // it comes from our child, case (a), pass to the parent, but only
                // if the direction is forwards. Otherwise set the focus to the
                // notebook itself. The notebook is always the 'first' control of a
                // page.
                if (!isForward)
                {
                    SetFocus();
                }
                else if (parent)
                {
                    event.SetCurrentFocus(this);
                    parent->HandleWindowEvent(event);
                }
            }
        }
    }

    // Methods for extensions of this class

    void AppendMenuItem(wxMenu* menu, const wxString& title) {
        GetTopBarItemsCtrl()->AppendMenuItem(menu, title);
    }

    void AppendMenuSeparaorItem() {
        GetTopBarItemsCtrl()->AppendMenuSeparaorItem();
    }


protected:
    virtual void UpdateSelectedPage(size_t WXUNUSED(newsel)) override
    {
        // Nothing to do here, but must be overridden to avoid the assert in
        // the base class version.
    }

    virtual wxBookCtrlEvent * CreatePageChangingEvent() const override
    {
        return new wxBookCtrlEvent(wxEVT_BOOKCTRL_PAGE_CHANGING,
                                   GetId());
    }

    virtual void MakeChangedEvent(wxBookCtrlEvent & event) override
    {
        event.SetEventType(wxEVT_BOOKCTRL_PAGE_CHANGED);
    }

    virtual wxWindow * DoRemovePage(size_t page) override
    {
        wxWindow* const win = wxBookCtrlBase::DoRemovePage(page);
        if (win)
        {
            GetTopBarItemsCtrl()->RemovePage(page);
            DoSetSelectionAfterRemoval(page);
        }

        return win;
    }

    virtual void DoSize() override
    {
        wxWindow* const page = GetCurrentPage();
        if (page)
            page->SetSize(GetPageRect());
    }

    virtual void DoShowPage(wxWindow * page, bool show) override
    {
        if (show)
            page->ShowWithEffect(m_showEffect, m_showTimeout);
        else
            page->HideWithEffect(m_hideEffect, m_hideTimeout);
    }

private:
    void Init()
    {
        // We don't need any border as we don't have anything to separate the
        // page contents from.
        SetInternalBorder(0);

        // No effects by default.
        m_showEffect =
        m_hideEffect = wxSHOW_EFFECT_NONE;

        m_showTimeout =
        m_hideTimeout = 0;
    }

    wxShowEffect m_showEffect,
                 m_hideEffect;

    unsigned m_showTimeout,
             m_hideTimeout;

    TopBarItemsCtrl* m_ctrl{ nullptr };

};
//#endif // _WIN32
#endif // slic3r_TopBar_hpp_
