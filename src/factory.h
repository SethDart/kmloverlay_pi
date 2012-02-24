/***************************************************************************
 * $Id: factory.h, v0.1 2012-02-08 SethDart Exp $
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

#ifndef _KMLOverlayFactory_H_
#define _KMLOverlayFactory_H_

#include <wx/wxprec.h>

#ifndef  WX_PRECOMP
  #include <wx/wx.h>
#endif //precompiled headers

#include <kml/engine.h>
#include "../../../include/ocpn_plugin.h"

class KMLOverlayFactory
{
public:
      KMLOverlayFactory();
      ~KMLOverlayFactory();

      bool RenderOverlay( ocpnDC &dc, PlugIn_ViewPort *vp );

      bool Add( wxString filename, bool visible );
      void SetVisibility( int idx, bool visible );
      void Delete( int idx );
      wxString GetFilename( int idx );
      bool GetVisibility( int idx );
      int GetCount();

private:
      class Container
      {
      public:
            Container(wxString filename, bool visible);
            bool Parse();
            bool Render( ocpnDC &dc, PlugIn_ViewPort *vp );
            void SetVisibility( bool visible );
            wxString GetFilename();
            bool GetVisibility();

      private:
            const kmldom::StylePtr GetFeatureStylePtr( const kmldom::FeaturePtr& feature );
            void RenderPoint( ocpnDC &dc, PlugIn_ViewPort *vp, const kmldom::PointPtr& point, const kmldom::StylePtr& style );
            void RenderLineString( ocpnDC &dc, PlugIn_ViewPort *vp, const kmldom::LineStringPtr& linestring, const kmldom::StylePtr& style );
            void RenderLinearRing( ocpnDC &dc, PlugIn_ViewPort *vp, const kmldom::LinearRingPtr& linearring, const kmldom::StylePtr& style );
            void RenderPolygon( ocpnDC &dc, PlugIn_ViewPort *vp, const kmldom::PolygonPtr& polygon, const kmldom::StylePtr& style );
            void RenderGroundOverlay( ocpnDC &dc, PlugIn_ViewPort *vp, const kmldom::GroundOverlayPtr& groundoverlay );
            void RenderGeometry( ocpnDC &dc, PlugIn_ViewPort *vp, const kmldom::GeometryPtr& geometry, const kmldom::StylePtr& style );
            void RenderFeature( ocpnDC &dc, PlugIn_ViewPort *vp, const kmldom::FeaturePtr& feature );
            bool       m_ready;
            wxString   m_filename;
            bool       m_visible;
            kmlengine::KmlFilePtr m_kml_file;
            kmldom::FeaturePtr m_root;

      };
      WX_DEFINE_ARRAY(Container *, ContainerArray);

      ContainerArray m_Objects;

};

#endif
