/***************************************************************************
                              qgsmaptooloffsetcurve.cpp
    ------------------------------------------------------------
    begin                : February 2012
    copyright            : (C) 2012 by Marco Hugentobler
    email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmaptooloffsetcurve.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsrubberband.h"
#include "qgsvectorlayer.h"
#include <QDoubleSpinBox>
#include <QGraphicsProxyWidget>
#include <QMouseEvent>
#include "qgisapp.h"

QgsMapToolOffsetCurve::QgsMapToolOffsetCurve( QgsMapCanvas* canvas ): QgsMapToolEdit( canvas ), mRubberBand( 0 ),
    mOriginalGeometry( 0 ), mGeometryModified( false ), mDistanceItem( 0 ), mDistanceSpinBox( 0 )
{
}

QgsMapToolOffsetCurve::~QgsMapToolOffsetCurve()
{
  deleteRubberBandAndGeometry();
  deleteDistanceItem();
}

void QgsMapToolOffsetCurve::canvasPressEvent( QMouseEvent * e )
{
  deleteRubberBandAndGeometry();
  mGeometryModified = false;

  //get selected features or snap to nearest feature if no selection
  QgsVectorLayer* layer = currentVectorLayer();
  if ( !layer )
  {
    return;
  }

  QList<QgsSnappingResult> snapResults;
  QgsMapCanvasSnapper snapper( mCanvas );
  snapper.snapToBackgroundLayers( e->pos(), snapResults );
  if ( snapResults.size() > 0 )
  {
    QgsFeature fet;
    const QgsSnappingResult& snapResult = snapResults.at( 0 );
    if ( snapResult.layer )
    {
      mSourceLayerId = snapResult.layer->id();

      QgsVectorLayer* vl = dynamic_cast<QgsVectorLayer*>( QgsMapLayerRegistry::instance()->mapLayer( mSourceLayerId ) );
      if ( vl && vl->featureAtId( snapResult.snappedAtGeometry, fet ) )
      {
        mOriginalGeometry = createOriginGeometry( vl, snapResult, fet );
        mRubberBand = createRubberBand();
        if ( mRubberBand )
        {
          mRubberBand->setToGeometry( mOriginalGeometry, layer );
        }
        mModifiedFeature = fet.id();
        createDistanceItem();
      }
    }
  }
}

void QgsMapToolOffsetCurve::canvasReleaseEvent( QMouseEvent * e )
{
  Q_UNUSED( e );
  QgsVectorLayer* vlayer = currentVectorLayer();
  if ( !vlayer || !mGeometryModified )
  {
    deleteRubberBandAndGeometry();
    return;
  }

  vlayer->beginEditCommand( tr( "Offset curve" ) );

  bool editOk;
  if ( mSourceLayerId == vlayer->id() )
  {
    editOk = vlayer->changeGeometry( mModifiedFeature, &mModifiedGeometry );
  }
  else
  {
    QgsFeature f;
    f.setGeometry( mModifiedGeometry );
    editOk = vlayer->addFeature( f );
  }

  if ( editOk )
  {
    vlayer->endEditCommand();
  }
  else
  {
    vlayer->destroyEditCommand();
  }

  deleteRubberBandAndGeometry();
  deleteDistanceItem();
  mCanvas->refresh();
}

void QgsMapToolOffsetCurve::placeOffsetCurveToValue()
{
  if ( mOriginalGeometry && mRubberBand && mRubberBand->numberOfVertices() > 0 )
  {
    //is rubber band left or right of original geometry
    double leftOf = 0;
    const QgsPoint* firstPoint = mRubberBand->getPoint( 0 );
    if ( firstPoint )
    {
      QgsPoint minDistPoint;
      int beforeVertex;
      mOriginalGeometry->closestSegmentWithContext( *firstPoint, minDistPoint, beforeVertex, &leftOf );
    }
    setOffsetForRubberBand( mDistanceSpinBox->value(), leftOf < 0 );
  }
}

void QgsMapToolOffsetCurve::canvasMoveEvent( QMouseEvent * e )
{
  if ( !mOriginalGeometry || !mRubberBand )
  {
    return;
  }

  QgsVectorLayer* layer = currentVectorLayer();
  if ( !layer )
  {
    return;
  }

  if ( mDistanceItem )
  {
    mDistanceItem->show();
    mDistanceItem->setPos( e->posF() + QPointF( 10, 10 ) );
  }

  mGeometryModified = true;

  //get offset from current position rectangular to feature
  QgsPoint layerCoords = toLayerCoordinates( layer, e->pos() );
  QgsPoint minDistPoint;
  int beforeVertex;
  double leftOf;
  double offset = sqrt( mOriginalGeometry->closestSegmentWithContext( layerCoords, minDistPoint, beforeVertex, &leftOf ) );

  //create offset geometry using geos
  setOffsetForRubberBand( offset, leftOf < 0 );

  if ( mDistanceSpinBox )
  {
    mDistanceSpinBox->setValue( offset );
  }
}

QgsGeometry* QgsMapToolOffsetCurve::createOriginGeometry( QgsVectorLayer* vl, const QgsSnappingResult& sr, QgsFeature& snappedFeature )
{
  if ( !vl )
  {
    return 0;
  }

  if ( vl == currentVectorLayer() )
  {
    //don't consider selected geometries, only the snap result
    return snappedFeature.geometryAndOwnership();
  }
  else //snapped to a background layer
  {
    //if source layer is polygon / multipolygon, create a linestring from the snapped ring
    if ( vl->geometryType() == QGis::Polygon )
    {
      //make linestring from polygon ring and return this geometry
      return linestringFromPolygon( snappedFeature.geometry(), sr.snappedVertexNr );
    }


    //for background layers, try to merge selected entries together if snapped feature is contained in selection
    const QgsFeatureIds& selection = vl->selectedFeaturesIds();
    if ( selection.size() < 1 || !selection.contains( sr.snappedAtGeometry ) )
    {
      return snappedFeature.geometryAndOwnership();
    }
    else
    {
      //merge together if several features
      QgsFeatureList selectedFeatures = vl->selectedFeatures();
      QgsFeatureList::iterator selIt = selectedFeatures.begin();
      QgsGeometry* geom = selIt->geometryAndOwnership();
      ++selIt;
      for ( ; selIt != selectedFeatures.end(); ++selIt )
      {
        geom = geom->combine( selIt->geometry() );
      }

      //if multitype, return only the snaped to geometry
      if ( geom->isMultipart() )
      {
        delete geom;
        return snappedFeature.geometryAndOwnership();
      }

      return geom;
    }
  }
}

void QgsMapToolOffsetCurve::createDistanceItem()
{
  if ( !mCanvas )
  {
    return;
  }

  deleteDistanceItem();

  mDistanceSpinBox = new QDoubleSpinBox();
  mDistanceSpinBox->setMaximum( 99999999 );
  mDistanceSpinBox->setDecimals( 2 );
  mDistanceSpinBox->setPrefix( tr( "Offset: " ) );
#ifndef Q_WS_X11
  mDistanceItem = new QGraphicsProxyWidget();
  mDistanceItem->setWidget( mDistanceSpinBox );
  mCanvas->scene()->addItem( mDistanceItem );
  mDistanceItem->hide();
#else
  mDistanceItem = 0;
  QgisApp::instance()->statusBar()->addWidget( mDistanceSpinBox );
#endif
  mDistanceSpinBox->grabKeyboard();
  mDistanceSpinBox->setFocus( Qt::TabFocusReason );

  QObject::connect( mDistanceSpinBox, SIGNAL( editingFinished() ), this, SLOT( placeOffsetCurveToValue() ) );
}

void QgsMapToolOffsetCurve::deleteDistanceItem()
{
  if ( mDistanceSpinBox )
  {
    mDistanceSpinBox->releaseKeyboard();
  }
  delete mDistanceItem;
  mDistanceItem = 0;
#ifdef Q_WS_X11
  QgisApp::instance()->statusBar()->removeWidget( mDistanceSpinBox );
  delete mDistanceSpinBox;
#endif
  mDistanceSpinBox = 0;
}

void QgsMapToolOffsetCurve::deleteRubberBandAndGeometry()
{
  delete mRubberBand;
  mRubberBand = 0;
  delete mOriginalGeometry;
  mOriginalGeometry = 0;
}

void QgsMapToolOffsetCurve::setOffsetForRubberBand( double offset, bool leftSide )
{
  if ( !mRubberBand || !mOriginalGeometry )
  {
    return;
  }

  QgsVectorLayer* sourceLayer = dynamic_cast<QgsVectorLayer*>( QgsMapLayerRegistry::instance()->mapLayer( mSourceLayerId ) );
  if ( !sourceLayer )
  {
    return;
  }


  QgsGeometry geomCopy( *mOriginalGeometry );
  GEOSGeometry* geosGeom = geomCopy.asGeos();
  if ( geosGeom )
  {
    GEOSGeometry* offsetGeom = GEOSSingleSidedBuffer( geosGeom, offset, 8, 1, 1, leftSide ? 1 : 0 );
    if ( offsetGeom )
    {
      mModifiedGeometry.fromGeos( offsetGeom );
      mRubberBand->setToGeometry( &mModifiedGeometry, sourceLayer );
    }
  }
}

QgsGeometry* QgsMapToolOffsetCurve::linestringFromPolygon( QgsGeometry* featureGeom, int vertex )
{
  if ( !featureGeom )
  {
    return 0;
  }

  QGis::WkbType geomType = featureGeom->wkbType();
  int currentVertex = 0;
  QgsMultiPolygon multiPoly;

  if ( geomType == QGis::WKBPolygon || geomType == QGis::WKBPolygon25D )
  {
    QgsPolygon polygon = featureGeom->asPolygon();
    multiPoly.append( polygon );
  }
  else if ( geomType == QGis::WKBMultiPolygon || geomType == QGis::WKBMultiPolygon25D )
  {
    //iterate all polygons / rings
    QgsMultiPolygon multiPoly = featureGeom->asMultiPolygon();
  }
  else
  {
    return 0;
  }

  QgsMultiPolygon::const_iterator multiPolyIt = multiPoly.constBegin();
  for ( ; multiPolyIt != multiPoly.constEnd(); ++multiPolyIt )
  {
    QgsPolygon::const_iterator polyIt = multiPolyIt->constBegin();
    for ( ; polyIt != multiPolyIt->constEnd(); ++polyIt )
    {
      currentVertex += polyIt->size();
      if ( vertex < currentVertex )
      {
        //found, return ring
        return QgsGeometry::fromPolyline( *polyIt );
      }
    }
  }

  return 0;
}
