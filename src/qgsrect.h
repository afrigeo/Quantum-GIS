/***************************************************************************
                          qgsrect.h  -  description
                             -------------------
    begin                : Sat Jun 22 2002
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSRECT_H
#define QGSRECT_H
#include <iostream>
#include <qstring.h>
class QgsPoint;

/*! \class QgsRect
 * \brief A rectangle specified with double values.
 *
 * QgsRect is used to store a rectangle when double values are required. 
 * Examples are storing a layer extent or the current view extent of a map
 */
class QgsRect{
 public:
    //! Constructor
    QgsRect(double xmin=0, double ymin=0, double xmax=0, double ymax=0);
    //! Construct a rectangle from two points. The rectangle is normalized after construction.
    QgsRect(QgsPoint p1, QgsPoint p2);
    //! Destructor
    ~QgsRect();
    //! Set the minimum x value
    void setXmin(double x);
    //! Set the maximum x value
    void setXmax(double x);
    //! Set the maximum y value
    void setYmin(double y);
    //! Set the maximum y value
    void setYmax(double y);
    //! Get the x maximum value (right side of rectangle)
    double xMax() const;
    //! Get the x maximum value (right side of rectangle)
    double xMin() const;
    //! Get the x minimum value (left side of rectangle)
    double yMax() const;
    //! Get the y maximum value (top side of rectangle)
    double yMin() const;
    //! Normalize the rectangle so it has non-negative width/height
    void normalize();
    //! Width of the rectangle
    double width() const;
    //! Height of the rectangle
    double height() const;
    //! Center point of the rectangle
    QgsPoint center() const;
    //! Scale the rectangle around its center point
    void scale(double, QgsPoint *c =0);
    //! Expand the rectangle to support zoom out scaling
    void expand(double, QgsPoint *c = 0);
     //! return the intersection with the given rectangle
    QgsRect intersect(QgsRect *rect);
    //! test if rectangle is empty
    bool isEmpty();
    //! returns string representation of form xmin,ymin xmax,ymax
    QString stringRep() const;
    /*! Comparison operator
      @return True if rectangles are equal
    */
    bool operator==(const QgsRect &r1);
    /*! Assignment operator
     * @param r1 QgsRect to assign from
     */
    QgsRect & operator=(const QgsRect &r1);
 private:
    
    double xmin;
    double ymin;
    double xmax;
    double ymax;
};

inline std::ostream& operator << (std::ostream& os, const QgsRect &r)
{
    return os << r.stringRep();
}
  
#endif // QGSRECT_H
