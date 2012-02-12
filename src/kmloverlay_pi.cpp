/***************************************************************************
 * $Id: kmloverlay_pi.cpp, v0.1 2012-01-20 SethDart Exp $
 *
 * Project:  OpenCPN
 * Purpose:  KML overlay plugin
 * Author:   Jean-Eudes Onfray
 *
 ***************************************************************************
 *   Copyright (C) 2012 by Jean-Eudes Onfray                               *
 *   je@onfray.fr                                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

#include <wx/wxprec.h>

#ifndef  WX_PRECOMP
  #include <wx/wx.h>
#endif //precompiled headers

#include "kmloverlay_pi.h"
#include "icons.h"
#include "prefdlg.h"

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr)
{
    return (opencpn_plugin *)new kmloverlay_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p)
{
    delete p;
}

//---------------------------------------------------------------------------------------------------------
//
//    KMLOverlay plugin implementation
//
//---------------------------------------------------------------------------------------------------------

wxString toSDMM ( int NEflag, double a )
{
      short neg = 0;
      int d;
      long m;

      if ( a < 0.0 )
      {
            a = -a;
            neg = 1;
      }
      d = ( int ) a;
      m = ( long ) ( ( a - ( double ) d ) * 60000.0 );

      if ( neg )
            d = -d;

      wxString s;

      if ( !NEflag )
            s.Printf ( _T ( "%d %02ld.%03ld'" ), d, m / 1000, m % 1000 );
      else
      {
            if ( NEflag == 1 )
            {
                  char c = 'N';

                  if ( neg )
                  {
                        d = -d;
                        c = 'S';
                  }

                  s.Printf ( _T ( "%03d %02ld.%03ld %c" ), d, m / 1000, ( m % 1000 ), c );
            }
            else if ( NEflag == 2 )
            {
                  char c = 'E';

                  if ( neg )
                  {
                        d = -d;
                        c = 'W';
                  }
                  s.Printf ( _T ( "%03d %02ld.%03ld %c" ), d, m / 1000, ( m % 1000 ), c );
            }
      }
      return s;
}


kmloverlay_pi::kmloverlay_pi(void *ppimgr)
      : opencpn_plugin(ppimgr)
{
      // Create the PlugIn icons
      initialize_images();
}

int kmloverlay_pi::Init(void)
{
      m_puserinput = NULL;

      AddLocaleCatalog( _T("opencpn-kmloverlay_pi") );

      m_toolbar_item_id  = InsertPlugInTool( _T(""), _img_kmloverlay, _img_kmloverlay, wxITEM_NORMAL,
            _("KML overlay"), _T(""), NULL, KMLOVERLAY_TOOL_POSITION, 0, this );

      m_pauimgr = GetFrameAuiManager();

      m_puserinput = new KMLOverlayUI( GetOCPNCanvasWindow(), wxID_ANY, _T("") );
      wxAuiPaneInfo pane = wxAuiPaneInfo().Name(_T("KMLOverlay")).Caption(_("KML overlay")).CaptionVisible(true).Float().FloatingPosition(50,150).Dockable(false).Resizable().CloseButton(true).Show(false);
      m_pauimgr->AddPane( m_puserinput, pane );
      m_pauimgr->Update();

      //    Get a pointer to the opencpn configuration object
      m_pconfig = GetOCPNConfigObject();
      //    And load the configuration items
      LoadConfig();
      ApplyConfig();

      return (
           WANTS_OVERLAY_CALLBACK    |
           WANTS_TOOLBAR_CALLBACK    |
           INSTALLS_TOOLBAR_TOOL     |
// nothing yet //           WANTS_PREFERENCES         |
           WANTS_CONFIG
            );
}

bool kmloverlay_pi::DeInit(void)
{
      SaveConfig();
      if ( m_puserinput )
      {
            m_pauimgr->DetachPane( m_puserinput );
            m_puserinput->Close();
            m_puserinput->Destroy();
            m_puserinput = NULL;
      }

      return true;
}

int kmloverlay_pi::GetAPIVersionMajor()
{
      return MY_API_VERSION_MAJOR;
}

int kmloverlay_pi::GetAPIVersionMinor()
{
      return MY_API_VERSION_MINOR;
}

int kmloverlay_pi::GetPlugInVersionMajor()
{
      return PLUGIN_VERSION_MAJOR;
}

int kmloverlay_pi::GetPlugInVersionMinor()
{
      return PLUGIN_VERSION_MINOR;
}

wxBitmap *kmloverlay_pi::GetPlugInBitmap()
{
      return _img_kmloverlay_pi;
}

wxString kmloverlay_pi::GetCommonName()
{
      return _("KMLOverlay");
}


wxString kmloverlay_pi::GetShortDescription()
{
      return _("KML overlay plugin for OpenCPN");
}

wxString kmloverlay_pi::GetLongDescription()
{
      return _("KML overlay plugin for OpenCPN");
}

int kmloverlay_pi::GetToolbarToolCount(void)
{
      return 1;
}

void kmloverlay_pi::OnToolbarToolCallback(int id)
{
      if ( m_puserinput )
      {
            wxAuiPaneInfo &pane = m_pauimgr->GetPane( m_puserinput );
            if ( pane.IsOk() && !pane.IsShown() )
            {
                  pane.Show();
                  m_pauimgr->Update();
            }
      }
}

void kmloverlay_pi::SetColorScheme( PI_ColorScheme cs )
{
      if ( m_puserinput )
      {
            m_puserinput->SetColorScheme( cs );
      }
}

bool kmloverlay_pi::RenderOverlay( wxDC &dc, PlugIn_ViewPort *vp )
{
      if ( m_puserinput )
      {
            return m_puserinput->RenderOverlay( dc, vp );
      }
      return false;
}

void kmloverlay_pi::ShowPreferencesDialog( wxWindow* parent )
{
      KMLOverlayPreferencesDialog *dialog = new KMLOverlayPreferencesDialog( parent, wxID_ANY, m_interval );

      if ( dialog->ShowModal() == wxID_OK )
      {
            // OnClose should handle that for us normally but it doesn't seems to do so
            // We must save changes first
            dialog->SaveKMLOverlayConfig();

            m_interval = dialog->m_interval;
            SaveConfig();
            ApplyConfig();
      }
      dialog->Destroy();
}

bool kmloverlay_pi::LoadConfig(void)
{
      wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

      if ( pConf )
      {
            pConf->SetPath( _T("/PlugIns/KMLOverlay") );

            pConf->Read( _T("Interval"), &m_interval, -1 );
            int d_cnt;
            pConf->Read( _T("FileCount"), &d_cnt, -1 );
            for ( int i = 0; i < d_cnt; i++ )
            {
                  pConf->SetPath( wxString::Format( _T("/PlugIns/KMLOverlay/KMLOverlay%d"), i+1 ) );
                  wxString filename;
                  pConf->Read( _T("FileName"), &filename, _T("") );
                  wxString visible;
                  pConf->Read( _T("Visible"), &visible, _T("Y") );
                  m_puserinput->AddFile( filename, (visible==_T("Y")) );
            }
            return true;
      }
      else
            return false;
}

bool kmloverlay_pi::SaveConfig(void)
{
      wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

      if ( pConf )
      {
            pConf->SetPath( _T("/PlugIns/KMLOverlay") );

            pConf->Write( _T("Interval"), m_interval );
            pConf->Write( _T("FileCount" ), m_puserinput->GetCount() );
            for ( int i = 0; i < m_puserinput->GetCount(); i++ )
            {
                  pConf->SetPath( wxString::Format( _T("/PlugIns/KMLOverlay/KMLOverlay%d"), i+1 ) );
                  pConf->Write( _T("FileName"), m_puserinput->GetFilename( i ) );
                  pConf->Write( _T("Visible"), (m_puserinput->GetVisibility( i )?_T("Y"):_T("N")) );
            }

            return true;
      }
      else
            return false;
}

void kmloverlay_pi::ApplyConfig(void)
{
      if ( m_interval != -1 )
      {
      }
}

