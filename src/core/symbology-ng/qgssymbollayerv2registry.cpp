
#include "qgssymbollayerv2registry.h"

#include "qgsellipsesymbollayerv2.h"
#include "qgsmarkersymbollayerv2.h"
#include "qgslinesymbollayerv2.h"
#include "qgsfillsymbollayerv2.h"
#include "qgsvectorfieldsymbollayer.h"

QgsSymbolLayerV2Registry* QgsSymbolLayerV2Registry::mInstance = NULL;

QgsSymbolLayerV2Registry::QgsSymbolLayerV2Registry()
{
  // init registry with known symbol layers
  addSymbolLayerType( new QgsSymbolLayerV2Metadata( "SimpleLine", QObject::tr( "Simple line" ), QgsSymbolV2::Line,
                      QgsSimpleLineSymbolLayerV2::create, QgsSimpleLineSymbolLayerV2::createFromSld ) );
  addSymbolLayerType( new QgsSymbolLayerV2Metadata( "MarkerLine", QObject::tr( "Marker line" ), QgsSymbolV2::Line,
                      QgsMarkerLineSymbolLayerV2::create, QgsMarkerLineSymbolLayerV2::createFromSld ) );
  addSymbolLayerType( new QgsSymbolLayerV2Metadata( "LineDecoration", QObject::tr( "Line decoration" ), QgsSymbolV2::Line,
                      QgsLineDecorationSymbolLayerV2::create ) );

  addSymbolLayerType( new QgsSymbolLayerV2Metadata( "SimpleMarker", QObject::tr( "Simple marker" ), QgsSymbolV2::Marker,
                      QgsSimpleMarkerSymbolLayerV2::create, QgsSimpleMarkerSymbolLayerV2::createFromSld ) );
  addSymbolLayerType( new QgsSymbolLayerV2Metadata( "SvgMarker", QObject::tr( "SVG marker" ), QgsSymbolV2::Marker,
                      QgsSvgMarkerSymbolLayerV2::create, QgsSvgMarkerSymbolLayerV2::createFromSld ) );
  addSymbolLayerType( new QgsSymbolLayerV2Metadata( "FontMarker", QObject::tr( "Font marker" ), QgsSymbolV2::Marker,
                      QgsFontMarkerSymbolLayerV2::create, QgsFontMarkerSymbolLayerV2::createFromSld ) );
  addSymbolLayerType( new QgsSymbolLayerV2Metadata( "EllipseMarker", QObject::tr( "Ellipse marker" ), QgsSymbolV2::Marker,
                      QgsEllipseSymbolLayerV2::create, QgsEllipseSymbolLayerV2::createFromSld ) );
  addSymbolLayerType( new QgsSymbolLayerV2Metadata( "VectorField", QObject::tr( "Vector Field marker" ), QgsSymbolV2::Marker,
                      QgsVectorFieldSymbolLayer::create ) );

  addSymbolLayerType( new QgsSymbolLayerV2Metadata( "SimpleFill", QObject::tr( "Simple fill" ), QgsSymbolV2::Fill,
                      QgsSimpleFillSymbolLayerV2::create, QgsSimpleFillSymbolLayerV2::createFromSld ) );
  addSymbolLayerType( new QgsSymbolLayerV2Metadata( "SVGFill", QObject::tr( "SVG fill" ), QgsSymbolV2::Fill,
                      QgsSVGFillSymbolLayer::create, QgsSVGFillSymbolLayer::createFromSld ) );
  addSymbolLayerType( new QgsSymbolLayerV2Metadata( "CentroidFill", QObject::tr( "Centroid fill" ), QgsSymbolV2::Fill,
                      QgsCentroidFillSymbolLayerV2::create, QgsCentroidFillSymbolLayerV2::createFromSld ) );
  addSymbolLayerType( new QgsSymbolLayerV2Metadata( "LinePatternFill", QObject::tr( "Line pattern fill" ), QgsSymbolV2::Fill,
                      QgsLinePatternFillSymbolLayer::create, QgsLinePatternFillSymbolLayer::createFromSld ) );
  addSymbolLayerType( new QgsSymbolLayerV2Metadata( "PointPatternFill", QObject::tr( "Point pattern fill" ), QgsSymbolV2::Fill,
                      QgsPointPatternFillSymbolLayer::create, QgsPointPatternFillSymbolLayer::createFromSld ) );
}

QgsSymbolLayerV2Registry::~QgsSymbolLayerV2Registry()
{
  foreach( QString name, mMetadata.keys() )
  {
    delete mMetadata[name];
  }
  mMetadata.clear();
}

bool QgsSymbolLayerV2Registry::addSymbolLayerType( QgsSymbolLayerV2AbstractMetadata* metadata )
{
  if ( metadata == NULL || mMetadata.contains( metadata->name() ) )
    return false;

  mMetadata[metadata->name()] = metadata;
  return true;
}


QgsSymbolLayerV2AbstractMetadata* QgsSymbolLayerV2Registry::symbolLayerMetadata( QString name ) const
{
  if ( mMetadata.contains( name ) )
    return mMetadata.value( name );
  else
    return NULL;
}

QgsSymbolLayerV2Registry* QgsSymbolLayerV2Registry::instance()
{
  if ( !mInstance )
    mInstance = new QgsSymbolLayerV2Registry();
  return mInstance;
}

QgsSymbolLayerV2* QgsSymbolLayerV2Registry::defaultSymbolLayer( QgsSymbolV2::SymbolType type )
{
  switch ( type )
  {
    case QgsSymbolV2::Marker:
      return QgsSimpleMarkerSymbolLayerV2::create();

    case QgsSymbolV2::Line:
      return QgsSimpleLineSymbolLayerV2::create();

    case QgsSymbolV2::Fill:
      return QgsSimpleFillSymbolLayerV2::create();
  }
  return NULL;
}


QgsSymbolLayerV2* QgsSymbolLayerV2Registry::createSymbolLayer( QString name, const QgsStringMap& properties ) const
{
  if ( !mMetadata.contains( name ) )
    return NULL;

  return mMetadata[name]->createSymbolLayer( properties );
}

QgsSymbolLayerV2* QgsSymbolLayerV2Registry::createSymbolLayerFromSld( QString name, QDomElement& element ) const
{
  if ( !mMetadata.contains( name ) )
    return NULL;

  return mMetadata[name]->createSymbolLayerFromSld( element );
}

QStringList QgsSymbolLayerV2Registry::symbolLayersForType( QgsSymbolV2::SymbolType type )
{
  QStringList lst;
  QMap<QString, QgsSymbolLayerV2AbstractMetadata*>::ConstIterator it = mMetadata.begin();
  for ( ; it != mMetadata.end(); ++it )
  {
    if (( *it )->type() == type )
      lst.append( it.key() );
  }
  return lst;
}
