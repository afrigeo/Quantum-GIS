/***************************************************************************
    qgsmapserverexport.cpp - Export QGIS MapCanvas to MapServer
     --------------------------------------
    Date                 : 8-Nov-2003
    Copyright            : (C) 2003 by Gary E.Sherman
    email                : sherman at mrcc.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include <iostream>
#include <fstream>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qmessagebox.h>
#include <qcolor.h>
#include <qregexp.h>
#include <qstring.h>
#include "qgsmaplayer.h"
#ifdef POSTGRESQL
#include "qgsdatabaselayer.h"
#endif
#include "qgsshapefilelayer.h"
#include "qgsmapcanvas.h"
#include "qgsrect.h"
#include "qgsmapserverexport.h"
#include "qgshelpviewer.h"
#include "qgis.h"

// constructor
QgsMapserverExport::QgsMapserverExport(QgsMapCanvas *_map,  
	QWidget* parent, const char* name, bool modal, WFlags fl )
		: QgsMapserverExportBase(parent, name, modal, fl),
			map(_map)

{
}

// Default destructor
QgsMapserverExport::~QgsMapserverExport()
{
}
// Get the base name for the map file
QString QgsMapserverExport::baseName(){
QFileInfo fi(txtMapFilePath->text());
return fi.baseName(true);
}
// Write the map file
bool QgsMapserverExport::write(){
	
	//QMessageBox::information(0,"Full Path",fullPath);
	int okToSave = 0;
	// Check for file and prompt for overwrite if it exists
	if(QFile::exists(txtMapFilePath->text())){
		okToSave = QMessageBox::warning(0,"Overwrite File?",txtMapFilePath->text() + 
			" exists. \nDo you want to overwrite it?", "Yes", "No");
	}
	if(okToSave == 0){
	// write the project information to the selected file
		writeMapFile();
		return true;
	}else{
		return false;
	}
}


void QgsMapserverExport::setFileName(QString fn){
	fullPath = fn;
	}
QString QgsMapserverExport::fullPathName(){
	return fullPath;
	}
void QgsMapserverExport::writeMapFile(){
	// write the map file, making massive assumptions about default values
	std::cout << "Opening map file " << txtMapFilePath->text() << std::endl;
	std::ofstream mapFile(txtMapFilePath->text());
	if(!mapFile.fail()){
		mapFile << "# Map file generated by QGIS version " << QGis::qgisVersion << std::endl;
		mapFile << "# Edit this file to customize for your interface" << std::endl;
		mapFile << "# Not all sections are complete. See comments for details." << std::endl;
		if(!chkExpLayersOnly->isChecked()){
		// header
		mapFile << "NAME " << txtMapName->text() << std::endl; 
		mapFile << "STATUS ON" << std::endl;
		mapFile << "\n# Map image size. Change size as desired" << std::endl;
		mapFile << "SIZE " << txtMapWidth->text() << " " << txtMapHeight->text() << std::endl;
		// web interface definition - this is minimal!
		mapFile << "#" << std::endl;
		mapFile << "# Start of web interface definition. Only the TEMPLATE parameter" << std::endl;
		mapFile << "# must be specified to display a map. See Mapserver documentation" << std::endl;
		mapFile << "#" << std::endl;
		mapFile << "WEB" << std::endl;
		// if no header is supplied, write the header line but comment it out
		if(txtWebHeader->text().isEmpty()){
			mapFile << "  # HEADER" << std::endl;
		}else{
			// header provided - write it
			mapFile << "  HEADER " << txtWebHeader->text() << std::endl;
		}
		// if no template provided, write the template line but comment it out
		if(txtWebTemplate->text().isEmpty()){
			mapFile << "  # TEMPLATE" << std::endl;
		}else{
			// template provided - write it
			mapFile << "  TEMPLATE " << txtWebTemplate->text() << std::endl;
		}
		// if no footer provided, write the footer line but comment it out
		if(txtWebFooter->text().isEmpty()){
			mapFile << "  # FOOTER" << std::endl;
		}else{
			mapFile << "  FOOTER " << txtWebFooter->text() << std::endl;
		}
		// write min and maxscale
		mapFile << "  MINSCALE " << txtMinScale->text() << std::endl;
		mapFile << "  MAXSCALE " << txtMaxScale->text() << std::endl;
		// end of web section
		mapFile << "END" << std::endl;
		
		// extent
		mapFile << "\n# Extent based on full extent of QGIS view" << std::endl;
		mapFile << "EXTENT ";
		QgsRect extent = map->extent();
		mapFile << extent.xMin() << " " << extent.yMin() << " ";
		mapFile << extent.xMax() << " " << extent.yMax() << std::endl;
		// units
		mapFile << "UNITS " << cmbMapUnits->currentText() << std::endl;
		// image info
		mapFile << "IMAGECOLOR 255 255 255" << std::endl;
		mapFile << "IMAGETYPE " << cmbMapImageType->currentText() << std::endl;
		// projection information TODO: support projections :)
		mapFile << "# Projection definition" << std::endl;
		mapFile << "# Projections are not currenlty supported. If desired, add your own" << std::endl;
		mapFile << "# projection information based on Mapserver documentation." << std::endl;
		mapFile << "#" << std::endl;
		
	}else{
		mapFile << " # This file contains layer definitions only and is not a complete" << std::endl;
		mapFile << " # Mapserver map file." << std::endl;
	}
		
		// write layer definitions 
		for(int i = 0; i < map->layerCount(); i++){
			bool isPolygon =false;
			bool isLine = false;
			QgsMapLayer *lyr = map->getZpos(i);
			mapFile << "LAYER" << std::endl;
			QString name = lyr->name().lower();
			// MapServer NAME must be < 20 char and unique
			name.replace(QRegExp(" "),"_");
			name.replace(QRegExp("\\."),"_");
			name.replace(QRegExp("\\("), "_");
			name.replace(QRegExp("\\)"), "_");
			mapFile << "  NAME " << name << std::endl;
			// feature type
			mapFile << "  TYPE "; 
			switch (lyr->featureType()) {
				case QGis::WKBPoint:
				case QGis::WKBMultiPoint:
                  mapFile << "POINT";
                  break;
				case QGis::WKBLineString:
				  case QGis::WKBMultiLineString:
                  mapFile << "LINE";
				  isLine = true;
                  break;
				case QGis::WKBPolygon:
				case QGis::WKBMultiPolygon:
                 mapFile << "POLYGON";
				 isPolygon = true;
                  break;
			}
			mapFile << std::endl;
			
			// set visibility (STATUS)
			mapFile << "  STATUS ";
			if(lyr->visible()){
				mapFile << "ON";
			}else{
				mapFile << "OFF";
			}
			mapFile << std::endl;
			
			// data source (DATA)
			// Data source spec depends on layer type
			switch(lyr->type()){
				case QgsMapLayer::VECTOR:
					mapFile << "  DATA " << lyr->source() << std::endl;
					break;
#ifdef POSTGRESQL
				case QgsMapLayer::DATABASE:
					QgsDatabaseLayer *dblyr = (QgsDatabaseLayer *)lyr;
					mapFile << "  CONNECTION \"" << lyr->source() << "\"" 
						<< std::endl;
					mapFile << "  CONNECTIONTYPE postgis" << std::endl;
					mapFile << "  DATA \"" << dblyr->geometryColumnName() <<
						" from " << dblyr->geometryTableName() << "\"" 
						<< std::endl;
					break;
#endif
			}
			// create a simple class entry based on layer color
			QgsSymbol *sym = lyr->symbol();
			mapFile << "  CLASS" << std::endl;
			QgsLegend *lgd = map->getLegend();
			//QListViewItem *li = lgd->currentItem();
            //    return li->text(0);
			mapFile << "    NAME \"" << lyr->name() << "\"" << std::endl;
			mapFile << "    # TEMPLATE" << std::endl;
			QColor fillColor = sym->fillColor();
			if(!isLine){
			mapFile << "    COLOR " << fillColor.red() << " " <<
				fillColor.green() << " " << fillColor.blue() << std::endl;
			}
			if(isPolygon || isLine){
				QColor outlineColor = sym->color();
				mapFile << "    OUTLINECOLOR " << outlineColor.red() << " "	
					<< outlineColor.green() << " " << outlineColor.blue() 
					<< std::endl;
			}
			mapFile << "  END" << std::endl;
			mapFile << "END" << std::endl;
		}
		if(!chkExpLayersOnly->isChecked()){
			mapFile << "END # Map File";
		}
		mapFile.close();
	}else{
	}
}
void QgsMapserverExport::showHelp(){
	//QMessageBox::information(this, "Help","Help");
	QgsHelpViewer *hv = new QgsHelpViewer(this);
	hv->setModal(false);
	hv->setCaption("QGIS Help - Mapserver Export");
	hv->show();
}
