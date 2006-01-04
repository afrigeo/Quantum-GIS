/***************************************************************************
                          qgsmarkercatalogue.cpp
                             -------------------
    begin                : March 2005
    copyright            : (C) 2005 by Radim Blazek
    email                : blazek@itc.it
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <cmath>
#include <iostream>

#include <qpainter.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qbitmap.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qrect.h>
#include <q3pointarray.h>
#include <qdir.h>

#include "qgsapplication.h"
#include "qgssvgcache.h"
#include "qgsmarkercatalogue.h"

QgsMarkerCatalogue *QgsMarkerCatalogue::mMarkerCatalogue = 0;

QgsMarkerCatalogue::QgsMarkerCatalogue()
{
    // Init list
    
    // Hardcoded markers
    mList.append ( "hard:circle" );
    mList.append ( "hard:rectangle" );
    mList.append ( "hard:diamond" );
    mList.append ( "hard:cross" );
    mList.append ( "hard:cross2" );

    // SVG
    QString svgPath = QgsApplication::svgPath();

    // TODO recursiv ?
    QDir dir ( svgPath );
    
    QStringList dl = dir.entryList(QDir::Dirs);
    
    for ( QStringList::iterator it = dl.begin(); it != dl.end(); ++it ) {
	if ( *it == "." || *it == ".." ) continue;

	QDir dir2 ( svgPath + *it );

	QStringList dl2 = dir2.entryList("*.svg",QDir::Files);
	
	for ( QStringList::iterator it2 = dl2.begin(); it2 != dl2.end(); ++it2 ) {
	    // TODO test if it is correct SVG
	    mList.append ( "svg:" + svgPath + *it + "/" + *it2 );
	}
    }
}

QStringList QgsMarkerCatalogue::list()
{
    return mList;
}

QgsMarkerCatalogue::~QgsMarkerCatalogue()
{
}

QgsMarkerCatalogue *QgsMarkerCatalogue::instance()
{
    if ( !QgsMarkerCatalogue::mMarkerCatalogue ) {
	QgsMarkerCatalogue::mMarkerCatalogue = new QgsMarkerCatalogue();
    }
	
    return QgsMarkerCatalogue::mMarkerCatalogue;
}

Q3Picture QgsMarkerCatalogue::marker ( QString fullName, int size, QPen pen, QBrush brush, int oversampling, bool qtBug )
{
    //std::cerr << "QgsMarkerCatalogue::marker" << std::endl;
    Q3Picture picture;
    
    if ( fullName.left(5) == "hard:" ) {
        return hardMarker ( fullName.mid(5), size, pen, brush, oversampling, qtBug ); 
    } else if ( fullName.left(4) == "svg:" ) {
        return svgMarker ( fullName.mid(4), size, oversampling ); 
    }

    return picture; // empty
}

Q3Picture QgsMarkerCatalogue::svgMarker ( QString name, int s, int oversampling )
{
    Q3Picture picture;
    QPainter painter;
    painter.begin(&picture);

    if ( oversampling <= 1 ) 
    {
	Q3Picture pic = QgsSVGCache::instance().getPicture(name);

	QRect br = pic.boundingRect();

	double scale = 1. * s / ( ( br.width() + br.height() ) / 2 ) ;

	painter.scale ( scale, scale );
	painter.drawPicture ( 0, 0, pic );

    } 
    else
    {
	QPixmap pixmap = QgsSVGCache::instance().getPixmap(name,1.);
	
	double scale = 1. * s / ( ( pixmap.width() + pixmap.height() ) / 2 ) ;
	
	pixmap = QgsSVGCache::instance().getPixmap(name,scale);

	painter.drawPixmap ( 0, 0, pixmap );

    }
    painter.end();

    return picture;
}

Q3Picture QgsMarkerCatalogue::hardMarker ( QString name, int s, QPen pen, QBrush brush, int oversampling, bool qtBug )
{
    // Size of polygon symbols is calculated so that the area is equal to circle with 
    // diameter mPointSize
    
    Q3Picture picture;
    
    // Size for circle
    int half = (int)floor(s/2.0); // number of points from center
    int size = 2*half + 1;  // must be odd
    double area = 3.14 * (size/2.) * (size/2.);

    // Picture
    QPainter picpainter;
    picpainter.begin(&picture);
    
    // Also width must be odd otherwise there are discrepancies visible in canvas!
    int lw = (int)(2*floor((double)pen.width()/2)+1); // -> lw > 0
    pen.setWidth(lw);
    picpainter.setPen ( pen );
    picpainter.setBrush( brush);

    QRect box;
    if ( name == "circle" ) 
    {
	picpainter.drawEllipse(0, 0, size, size);
    } 
    else if ( name == "rectangle" ) 
    {
	size = (int) (2*floor(sqrt(area)/2.) + 1);
	picpainter.drawRect(0, 0, size, size);
    } 
    else if ( name == "diamond" ) 
    {
	half = (int) ( sqrt(area/2.) );
	Q3PointArray pa(4);
	pa.setPoint ( 0, 0, half);
	pa.setPoint ( 1, half, 2*half);
	pa.setPoint ( 2, 2*half, half);
	pa.setPoint ( 3, half, 0);
	picpainter.drawPolygon ( pa );
    }
    // Warning! if pen width > 0 picpainter.drawLine(x1,y1,x2,y2) will draw only (x1,y1,x2-1,y2-1) !
    // It is impossible to draw lines as rectangles because line width scaling would not work
    // (QPicture is scaled later in QgsVectorLayer)
    //  -> reset boundingRect for cross, cross2
    else if ( name == "cross" ) 
    {
	int add;
	if ( qtBug ) {
	    add = 1;  // lw always > 0
	} else {
	    add = 0;
	}
	
	picpainter.drawLine(0, half, size-1+add, half); // horizontal
	picpainter.drawLine(half, 0, half, size-1+add); // vertical
	box.setRect ( 0, 0, size, size );
    }
    else if ( name == "cross2" ) 
    {
	half = (int) floor( s/2/sqrt(2.0));
	size = 2*half + 1;
	
	int add;
	if ( qtBug ) {
	    add = 1;  // lw always > 0
	} else {
	    add = 0;
	}
	
	int addwidth = (int) ( 0.5 * lw ); // width correction, cca lw/2 * cos(45)
	
	picpainter.drawLine( 0, 0, size-1+add, size-1+add);
	picpainter.drawLine( 0, size-1, size-1+add, 0-add);

        box.setRect ( -addwidth, -addwidth, size + 2*addwidth, size + 2*addwidth );	
    }
    picpainter.end();

    if ( name == "cross" || name == "cross2" ) {
        picture.setBoundingRect ( box ); 
    }

    // If oversampling > 1 create pixmap 
    
    // Hardcoded symbols are not oversampled at present, because the problem is especially 
    // with SVG markers (I am not sure why)
    /*
    if ( oversampling > 1 ) {
	QRect br = picture.boundingRect();
	QPixmap pixmap ( oversampling * br.width(), oversampling * br.height() );

	// Find bg color (must differ from line and fill)
	QColor transparent;
	for ( int i = 0; i < 255; i++ ) {
	    if ( pen.color().red() != i &&  brush.color().red() != i ) {
		transparent = QColor ( i, 0, 0 );
		break;
	    }
	}
	
	pixmap.fill( transparent );
	QPainter pixpainter;
	pixpainter.begin(&pixmap);
	pixpainter.scale ( oversampling, oversampling );
	pixpainter.drawPicture ( -br.x(), -br.y(), picture );
	pixpainter.end();

	QImage img = pixmap.convertToImage();
	img.setAlphaBuffer(true);
	for ( int i = 0; i < img.width(); i++ ) {
	    for ( int j = 0; j < img.height(); j++ ) {
		QRgb pixel = img.pixel(i, j);
		int alpha = 255;
		if ( qRed(pixel) == transparent.red() ) {
		    alpha = 0;
		}
		img.setPixel ( i, j, qRgba(qRed(pixel), qGreen(pixel), qBlue(pixel), alpha) );
	    }
	}
	img = img.smoothScale( br.width(), br.height());
	pixmap.convertFromImage ( img );

        picpainter.begin(&picture);
	picpainter.drawPixmap ( 0, 0, pixmap );
	picpainter.end();
    } 
    */

    return picture;
}

