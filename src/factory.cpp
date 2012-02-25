/***************************************************************************
 * $Id: factory.cpp, v0.1 2012-02-08 SethDart Exp $
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

#include <iostream>
#include <kml/base/file.h>
#include "factory.h"
#include <wx/mstream.h>
#include "icons.h"

const wxColor KMLOverlayDefaultColor( 144, 144, 144 );

KMLOverlayFactory::KMLOverlayFactory()
{
}

KMLOverlayFactory::~KMLOverlayFactory()
{
      for ( size_t i = m_Objects.GetCount(); i > 0; i-- )
      {
            Container *cont = m_Objects.Item( i-1 );
            m_Objects.Remove( cont );
            delete cont;
      }
}

bool KMLOverlayFactory::RenderOverlay( ocpnDC &dc, PlugIn_ViewPort *vp )
{
      for ( size_t i = 0; i < m_Objects.GetCount(); i++ )
      {
            m_Objects.Item( i )->Render( dc, vp );
      }
      return true;
}

bool KMLOverlayFactory::Add( wxString filename, bool visible )
{
      Container *cont = new Container( filename, visible );
      if ( cont->Parse() )
      {
            m_Objects.Add( cont );
            RequestRefresh( GetOCPNCanvasWindow() );
            return true;
      }
      else
      {
            delete cont;
            return false;
      }
}

void KMLOverlayFactory::SetVisibility( int idx, bool visible )
{
      m_Objects.Item( idx )->SetVisibility( visible );
      RequestRefresh( GetOCPNCanvasWindow() );
}

void KMLOverlayFactory::Delete( int idx )
{
      Container *cont = m_Objects.Item( idx );
      m_Objects.Remove( cont );
      delete cont;
      RequestRefresh( GetOCPNCanvasWindow() );
}

wxString KMLOverlayFactory::GetFilename( int idx )
{
      return m_Objects.Item( idx )->GetFilename();
}

bool KMLOverlayFactory::GetVisibility( int idx )
{
      return m_Objects.Item( idx )->GetVisibility();
}

int KMLOverlayFactory::GetCount()
{
      return m_Objects.GetCount();
}

KMLOverlayFactory::Container::Container( wxString filename, bool visible )
     : m_ready( false ), m_filename( filename ), m_visible( visible ),
      m_kmz_file( NULL), m_kml_file( NULL ), m_root( NULL )
{
}

bool KMLOverlayFactory::Container::Parse()
{
      std::string _filename( m_filename.mb_str() );
      std::string file_data;
      if ( !kmlbase::File::ReadFileToString( _filename, &file_data ) ) {
            wxLogMessage( _T("KMLOverlayFactory::Container::Parse Failed to read file content") );
            return false;
      }

      std::string kml;
      if ( kmlengine::KmzFile::IsKmz( file_data ) ) {
            m_kmz_file = kmlengine::KmzFile::OpenFromString( file_data );
            if ( !m_kmz_file.get() ) {
                  wxLogMessage( _T("KMLOverlayFactory::Container::Parse Failed opening KMZ file") );
                  return false;
            }
            if ( !m_kmz_file->ReadKml( &kml ) ) {
                  wxLogMessage( _T("KMLOverlayFactory::Container::Parse Failed to read KML from KMZ") );
                  return false;
            }
      } else {
            kml = file_data;
      }

      std::string errors;
      m_kml_file = kmlengine::KmlFile::CreateFromParse( kml, &errors );
      if ( !m_kml_file ) {
            // TODO: display error message
            return false;
      }

/*
      kmldom::ElementPtr element = m_kml_file->get_root();
      const kmldom::KmlPtr kml = kmldom::AsKml( element );
      m_root = kml->get_feature();
*/
      m_root = kmlengine::GetRootFeature( m_kml_file->get_root() );

      m_ready = true;
      return true;
}

const kmldom::StylePtr KMLOverlayFactory::Container::GetFeatureStylePtr( const kmldom::FeaturePtr& feature )
{
      kmldom::StylePtr style = kmlengine::CreateResolvedStyle( feature, m_kml_file, kmldom::STYLESTATE_NORMAL );

      // Some inline styles are not found by CreateResolvedStyle
      // Try to find them directly
      if ( style->get_id().empty() && feature->has_styleurl() ) {
            std::string style_id;  // fragment
            if ( kmlengine::SplitUriFragment( feature->get_styleurl(), &style_id ) ) {
                  const kmldom::ObjectPtr object = m_kml_file->GetObjectById( style_id );
                  if ( style = kmldom::AsStyle( object ) ) {
                        return style;
                  }
            }
      }

      return style;
}

void KMLOverlayFactory::Container::RenderPoint( ocpnDC &dc, PlugIn_ViewPort *vp, const kmldom::PointPtr& point, const kmldom::StylePtr& style )
{
      if ( point->has_coordinates() ) {
            int radius = 5;
            wxPoint pt;

            kmldom::CoordinatesPtr coord = point->get_coordinates();
            if ( coord->get_coordinates_array_size() != 1 )
            {
                  // TODO: log error: a point should have only one coord
                  return;
            }
            kmlbase::Vec3 vec = coord->get_coordinates_array_at( 0 );
            GetCanvasPixLL( vp,  &pt, vec.get_latitude(), vec.get_longitude() );

            wxPen pen( KMLOverlayDefaultColor, 1 );
            dc.SetPen( pen );
            dc.SetBrush( KMLOverlayDefaultColor );
            dc.DrawCircle( pt, radius );
      }
}

void KMLOverlayFactory::Container::RenderLineString( ocpnDC &dc, PlugIn_ViewPort *vp, const kmldom::LineStringPtr& linestring, const kmldom::StylePtr& style )
{
      if ( linestring->has_coordinates() ) {
            kmldom::CoordinatesPtr coord = linestring->get_coordinates();
            wxPoint *pts;
            size_t sz = coord->get_coordinates_array_size();
            pts = new wxPoint[sz];
            for ( size_t i = 0; i < sz; ++i ) {
                  kmlbase::Vec3 vec = coord->get_coordinates_array_at( i );
                  GetCanvasPixLL( vp,  &pts[i], vec.get_latitude(), vec.get_longitude() );
            }

            wxPen pen( KMLOverlayDefaultColor, 2 );
            if ( style->has_linestyle() ) {
                  const kmldom::LineStylePtr& linestyle = style->get_linestyle();
/* TODO: Implement colormode=random
 * see http://code.google.com/apis/kml/documentation/kmlreference.html#colorstyle
                  has_colormode
                  int get_colormode()
                     COLORMODE_NORMAL = 0,
                     COLORMODE_RANDOM
*/
                  if ( linestyle->has_color() ) {
                        kmlbase::Color32 col32 = linestyle->get_color();

                        pen = wxPen( wxColor( col32.get_red(), col32.get_green(), col32.get_blue(), col32.get_alpha() ),
                                     linestyle->get_width() );
                  }
            }
            dc.SetPen( pen );
            dc.DrawLines( coord->get_coordinates_array_size(), pts );
            delete [] pts;
            pts = NULL;
      }
}

void KMLOverlayFactory::Container::RenderLinearRing( ocpnDC &dc, PlugIn_ViewPort *vp, const kmldom::LinearRingPtr& linearring, const kmldom::StylePtr& style )
{
      if ( linearring->has_coordinates() ) {
            kmldom::CoordinatesPtr coord = linearring->get_coordinates();
            wxPoint *pts;
            size_t sz = coord->get_coordinates_array_size();
            pts = new wxPoint[sz];
            for ( size_t i = 0; i < sz; ++i ) {
                  kmlbase::Vec3 vec = coord->get_coordinates_array_at( i );
                  GetCanvasPixLL( vp,  &pts[i], vec.get_latitude(), vec.get_longitude() );
            }

            wxPen pen( KMLOverlayDefaultColor, 2 );
            wxBrush brush( *wxTRANSPARENT_BRUSH );
            if ( style->has_polystyle() ) {
                  const kmldom::PolyStylePtr polystyle = style->get_polystyle();
                  if ( !polystyle->has_fill() || polystyle->get_fill() ) {
                        if ( polystyle->has_color() ) {
                              kmlbase::Color32 col32 = polystyle->get_color();

                              if ( col32.get_alpha() ) {
                                    brush = wxBrush( wxColor( col32.get_red(), col32.get_green(), col32.get_blue(), col32.get_alpha() ) );
                              }
                        }
                  }
                  if ( !polystyle->has_outline() || polystyle->get_outline() ) {
                        if ( style->has_linestyle() ) {
                              const kmldom::LineStylePtr& linestyle = style->get_linestyle();
/*
                  has_colormode
                  int get_colormode()
                     COLORMODE_NORMAL = 0,
                     COLORMODE_RANDOM
*/
                              if ( linestyle->has_color() ) {
                                    kmlbase::Color32 col32 = linestyle->get_color();

                                    if ( col32.get_alpha() ) {
                                          pen = wxPen( wxColor( col32.get_red(), col32.get_green(), col32.get_blue(), col32.get_alpha() ),
                                                       linestyle->get_width() );
                                    } else {
                                          pen = *wxTRANSPARENT_PEN;
                                    }
                              }
                        }
                  }
            }

            dc.SetPen( pen );
            dc.SetBrush( brush );
            dc.DrawPolygon( coord->get_coordinates_array_size(), pts );
            delete [] pts;
            pts = NULL;
      }
}

void KMLOverlayFactory::Container::RenderPolygon( ocpnDC &dc, PlugIn_ViewPort *vp, const kmldom::PolygonPtr& polygon, const kmldom::StylePtr& style )
{
      if ( polygon->has_outerboundaryis() ) {
            kmldom::OuterBoundaryIsPtr bound = polygon->get_outerboundaryis();
            if ( bound->has_linearring() ) {
                  RenderLinearRing( dc, vp, bound->get_linearring(), style );
            }
// TODO: handle inner boundary (array)
      }
}

void KMLOverlayFactory::Container::RenderGroundOverlay( ocpnDC &dc, PlugIn_ViewPort *vp, const kmldom::GroundOverlayPtr& groundoverlay )
{
      int alpha = 255;
      if ( groundoverlay->has_color() ) {
            kmlbase::Color32 col32 = groundoverlay->get_color();
            alpha = col32.get_alpha();
      }

/* TODO
 should we handle <gx:LatLonQuad> (Used for nonrectangular quadrilateral ground overlays.)
   has_gx_latlonquad get_gx_latlonquad
      CoordinatesPtr& latlonquad.get_coordinates
 */
      if ( groundoverlay->has_latlonbox() ) {
            const kmldom::LatLonBoxPtr latlonbox = groundoverlay->get_latlonbox();
            wxPoint ptNW, ptSE;
            GetCanvasPixLL( vp,  &ptNW, latlonbox->get_north(), latlonbox->get_west() );
            GetCanvasPixLL( vp,  &ptSE, latlonbox->get_south(), latlonbox->get_east() );

/* TODO:
has_refreshmode
get_refreshmode
has_refreshinterval
get_refreshinterval
has_httpquery
get_httpquery
*/
            std::string href;
            if ( kmlengine::GetIconParentHref( groundoverlay, &href ) ) {
                  //kml_cache->FetchDataRelative(kml_file->get_url(), href, data);

                  std::string content;
                  //if ( kmlengine::FetchIcon( m_kml_file, groundoverlay, &content ) ) {
                  if ( m_kmz_file && m_kmz_file->ReadFile( href.c_str(), &content ) ) {
                        wxMemoryInputStream is( content.c_str(), content.size() );
                        wxImage image( is, wxBITMAP_TYPE_ANY );
                        if ( image.IsOk() ) {
                              if ( latlonbox->has_rotation() ) {
                                    latlonbox->get_rotation();
                              }
                              int dx = ptSE.x - ptNW.x + 1;
                              int dy = ptSE.y - ptNW.y + 1;
                              if ( dx < 32 || dy < 32 ) {
                                    // Overlay is very small, let's draw a default KML picture instead
                                    dc.DrawBitmap( *_img_undersized, ptNW.x, ptNW.y, false );
                                    return;
                              }
                              image.Rescale( dx, dy );
                              image.InitAlpha();
                              int size = image.GetWidth() * image.GetHeight();
                              unsigned char *alphad = (unsigned char *)malloc ( size * sizeof ( unsigned char ) );
                              unsigned char *a = alphad;
                              for ( int i=0 ; i<size; i++ ) {
                                    *a++ = ( unsigned char )alpha;
                              }
                              image.SetAlpha( alphad );
                              wxBitmap bitmap( image );
                              dc.DrawBitmap( bitmap, ptNW.x, ptNW.y, true );
                        }
                  }
            }
      }
}

void KMLOverlayFactory::Container::RenderGeometry( ocpnDC &dc, PlugIn_ViewPort *vp, const kmldom::GeometryPtr& geometry, const kmldom::StylePtr& style )
{
      if ( !geometry ) {
            return;
      }

      switch ( geometry->Type() ) {
      case kmldom::Type_Point:
      {
            if ( const kmldom::PointPtr point = kmldom::AsPoint( geometry ) ) {
                  RenderPoint( dc, vp, point, style );
            }
      }
      break;
      case kmldom::Type_LineString:
      {
            if ( const kmldom::LineStringPtr linestring = kmldom::AsLineString( geometry ) ) {
                  RenderLineString( dc, vp, linestring, style );
            }
      }
      break;
      case kmldom::Type_LinearRing:
      {
            if ( const kmldom::LinearRingPtr linearring = kmldom::AsLinearRing( geometry ) ) {
                  RenderLinearRing( dc, vp, linearring, style );
            }
      }
      break;
      case kmldom::Type_Polygon:
      {
            if ( const kmldom::PolygonPtr polygon = kmldom::AsPolygon( geometry ) ) {
                  RenderPolygon( dc, vp, polygon, style );
            }
      }
      break;
      case kmldom::Type_MultiGeometry:
      {
            if ( const kmldom::MultiGeometryPtr multigeometry = kmldom::AsMultiGeometry( geometry ) )
            {
                  for ( size_t i = 0; i < multigeometry->get_geometry_array_size(); ++i ) {
                        RenderGeometry( dc, vp, multigeometry->get_geometry_array_at( i ), style );
                  }
            }
      }
      break;
      case kmldom::Type_Model:
      break;
      default:  // KML has 6 types of Geometry.
      break;
      }
}

void KMLOverlayFactory::Container::RenderFeature( ocpnDC &dc, PlugIn_ViewPort *vp, const kmldom::FeaturePtr& feature)
{
      if ( !feature )
      {
            return;
      }
      if ( !feature->get_visibility() )
      {
            return;
      }

/* Can we do something with name?
      if ( feature->has_name() ) {
            std::cout << "Render " << feature->get_name() << std::endl;
      }
 */

      switch ( feature->Type() ) {
      case kmldom::Type_Document:
            // Document is a container, handled below.
      break;
      case kmldom::Type_Folder:
            // Folder is a container, handled below.
      break;
      case kmldom::Type_GroundOverlay:
      {
            if ( const kmldom::GroundOverlayPtr groundoverlay = kmldom::AsGroundOverlay( feature ) ) {
                  RenderGroundOverlay( dc, vp, groundoverlay );
            }
      }
      break;
      case kmldom::Type_Placemark:
      {
            if ( const kmldom::PlacemarkPtr placemark = kmldom::AsPlacemark( feature ) ) {
                  RenderGeometry( dc, vp, placemark->get_geometry(), GetFeatureStylePtr( feature ) );
            }
      }
      break;
      default:
      break;
      }

      if ( const kmldom::ContainerPtr container = kmldom::AsContainer( feature ) ) {
            for ( size_t i = 0; i < container->get_feature_array_size(); ++i ) {
                  RenderFeature( dc, vp, container->get_feature_array_at( i ) );
            }
      }
}

bool KMLOverlayFactory::Container::Render( ocpnDC &dc, PlugIn_ViewPort *vp )
{
      if ( !m_ready )
            return false;

      if ( !m_visible )
            return true;

      RenderFeature( dc, vp, m_root );
      return true;
}

void KMLOverlayFactory::Container::SetVisibility( bool visible )
{
      m_visible = visible;
}

wxString KMLOverlayFactory::Container::GetFilename()
{
      return m_filename;
}

bool KMLOverlayFactory::Container::GetVisibility()
{
      return m_visible;
}

