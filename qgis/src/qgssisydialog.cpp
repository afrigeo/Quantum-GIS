/***************************************************************************
                         qgssisydialog.cpp  -  description
                             -------------------
    begin                : Oct 2003
    copyright            : (C) 2003 by Marco Hugentobler
    email                : mhugent@geo.unizh.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */
#include "qgssisydialog.h"
#include "qpushbutton.h"
#include "qspinbox.h"
#include "qcolordialog.h"
#include "qgspatterndialog.h"
#include "qgssymbologyutils.h"
#include "qgslinestyledialog.h"
#include <iostream>
#include "qgsvectorlayer.h"
#include "qpixmap.h"
#include "qgslegenditem.h"
#include "qgsrenderitem.h"
#include "qgssinglesymrenderer.h"
#include "qgsvectorlayerproperties.h"
#include <qlineedit.h>
#include <qtoolbutton.h>
#include <qlabel.h>

QgsSiSyDialog::QgsSiSyDialog():QgsSiSyDialogBase(), mVectorLayer(0)
{
#ifdef QGISDEBUG
    qWarning("constructor QgsSiSyDialog");
#endif
}

QgsSiSyDialog::QgsSiSyDialog(QgsVectorLayer * layer):QgsSiSyDialogBase(), mVectorLayer(layer)
{
#ifdef QGISDEBUG
    qWarning("constructor QgsSiSyDialog");
#endif
    
    if (layer)
    {
	QgsSingleSymRenderer *renderer;
	
	//initial settings, use the buffer of the propertiesDialog if possible. If this is not possible, use the renderer of the vectorlayer directly
	if (mVectorLayer->propertiesDialog())
        {
	    renderer = dynamic_cast < QgsSingleSymRenderer * >(layer->propertiesDialog()->getBufferRenderer());
	} 
	else
        {
	    renderer = dynamic_cast < QgsSingleSymRenderer * >(layer->renderer());
        }

  if (renderer)
  {
    outlinewidthspinbox->setValue(renderer->item()->getSymbol()->pen().width());
    outlinewidthspinbox->setMinValue(1);//set line width 1 as minimum to avoid confusion between line width 0 and no pen line style
    lblFillColor->setPaletteBackgroundColor(renderer->item()->getSymbol()->brush().color());
    patternbutton->setName(QgsSymbologyUtils::brushStyle2Char(renderer->item()->getSymbol()->brush().style()));
    patternbutton->setPixmap(QgsSymbologyUtils::char2PatternPixmap(patternbutton->name()));
    stylebutton->setName(QgsSymbologyUtils::penStyle2Char(renderer->item()->getSymbol()->pen().style()));
    stylebutton->setPixmap(QgsSymbologyUtils::char2LinePixmap(stylebutton->name()));
    lblOutlineColor->setPaletteBackgroundColor(renderer->item()->getSymbol()->pen().color());
  }else
	{
	    qWarning("Warning, typecast failed in qgssisydialog.cpp, line 54 or 58");
	}

	if (mVectorLayer && mVectorLayer->vectorType() == QGis::Line)
        {
	    lblFillColor->unsetPalette();
	    btnFillColor->setEnabled(false);
	    patternbutton->setText("");
	    patternbutton->setEnabled(false);
        }
	//do the signal/slot connections
	QObject::connect(btnOutlineColor, SIGNAL(clicked()), this, SLOT(selectOutlineColor()));
	QObject::connect(stylebutton, SIGNAL(clicked()), this, SLOT(selectOutlineStyle()));
	QObject::connect(btnFillColor, SIGNAL(clicked()), this, SLOT(selectFillColor()));
	QObject::connect(patternbutton, SIGNAL(clicked()), this, SLOT(selectFillPattern()));
    } 
    else
    {
	qWarning("Warning, layer is a null pointer in QgsSiSyDialog::QgsSiSyDialog(QgsVectorLayer)");
    }
}

QgsSiSyDialog::~QgsSiSyDialog()
{
#ifdef QGISDEBUG
    qWarning("destructor QgsSiSyDialog");
#endif
}

void QgsSiSyDialog::selectOutlineColor()
{
    lblOutlineColor->setPaletteBackgroundColor(QColorDialog::getColor(QColor(black),this));
    setActiveWindow();
}

void QgsSiSyDialog::selectOutlineStyle()
{
    QgsLineStyleDialog linestyledialog;
    if (linestyledialog.exec() == QDialog::Accepted)
    {
	stylebutton->setName(QgsSymbologyUtils::penStyle2QString(linestyledialog.style()).ascii());
	stylebutton->setPixmap(QgsSymbologyUtils::qString2LinePixmap(QString::fromAscii(stylebutton->name())));
    }
    setActiveWindow();
}

void QgsSiSyDialog::selectFillColor()
{
    lblFillColor->setPaletteBackgroundColor(QColorDialog::getColor(QColor(black),this));
    setActiveWindow();
}

void QgsSiSyDialog::selectFillPattern()
{
    QgsPatternDialog patterndialog;
    if (patterndialog.exec() == QDialog::Accepted)
    {
	patternbutton->setName(QgsSymbologyUtils::brushStyle2Char(patterndialog.pattern()));
	patternbutton->setPixmap(QgsSymbologyUtils::brushStyle2Pixmap(patterndialog.pattern()));
    }
    setActiveWindow();
}

void QgsSiSyDialog::apply()
{
    //query the values of the widgets and set the symbology of the vector layer
    QgsSymbol* sy = new QgsSymbol();
    sy->brush().setColor(lblFillColor->paletteBackgroundColor());
    sy->brush().setStyle(QgsSymbologyUtils::char2BrushStyle(patternbutton->name()));
    sy->pen().setStyle(QgsSymbologyUtils::char2PenStyle(stylebutton->name()));
    sy->pen().setWidth(outlinewidthspinbox->value());
    sy->pen().setColor(lblOutlineColor->paletteBackgroundColor());
    QgsRenderItem* ri = new QgsRenderItem(sy, "blabla", "blabla");
    
    QgsSingleSymRenderer *renderer = dynamic_cast < QgsSingleSymRenderer * >(mVectorLayer->renderer());
    
    if (renderer)
    {
	renderer->addItem(ri);
    } 
    else
    {
	qWarning("typecast failed in QgsSiSyDialog::apply()");
	return;
    }
    
    //add a pixmap to the legend item
    
    //font tor the legend text
    QFont f("arial", 10, QFont::Normal);
    QFontMetrics fm(f);
    
    QPixmap *pix = mVectorLayer->legendPixmap();

    QString name;
    if (mVectorLayer->propertiesDialog())
    {
	name = mVectorLayer->propertiesDialog()->displayName();
    } 
    else
    {
	name = "";
    }

    int width = 40 + fm.width(name);
    int height = (fm.height() + 10 > 35) ? fm.height() + 10 : 35;
    pix->resize(width, height);
    pix->fill();

    QPainter p(pix);
    p.setPen(sy->pen());
    p.setBrush(sy->brush());
    //paint differently in case of point, lines, polygones
    switch (mVectorLayer->vectorType())
    {
	case QGis::Polygon:
	    p.drawRect(10, pix->height() - 25, 20, 15);
	    break;
	case QGis::Line:
	    p.drawLine(10, pix->height() - 25, 25, pix->height() - 10);
	    break;
	case QGis::Point:
	    p.drawRect(20, pix->height() - 17, 5, 5);
    }

    p.setPen(Qt::black);
    p.setFont(f);
    p.drawText(35, pix->height() - 10, name);

    mVectorLayer->updateItemPixmap();
    
    if (mVectorLayer->propertiesDialog())
    {
	mVectorLayer->propertiesDialog()->setRendererDirty(false);
    }
    //repaint the map canvas
    mVectorLayer->triggerRepaint();
}
