/****************************************************************************
**
** Copyright (C) 2007-2009 Kevin Clague. All rights reserved.
** Copyright (C) 2015 - 2025 Trevor SANDY. All rights reserved.
** Copyright (C) 2026 DoubleEagle. All rights reserved.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "arrowitem.h"
#include <QPainterPath>
#include <QLineF>
#include <QPen>
#include <QBrush>

ArrowItem::ArrowItem(InsertMeta insertMeta, QGraphicsItem *parent)
  : QGraphicsPathItem(parent),
    meta(insertMeta)
{
  setArrowPath();

  // Black shaft and filled head, 2 logical px (scales with page resolution,
  // consistent with how other LPub3D pen widths behave during export).
  QPen pen(QColor(0x00, 0x00, 0x00));
  pen.setWidthF(2.0);
  pen.setCapStyle(Qt::SquareCap);
  pen.setJoinStyle(Qt::MiterJoin);
  setPen(pen);
  setBrush(QColor(0x00, 0x00, 0x00));

  setZValue(INSERTPIXMAP_ZVALUE_DEFAULT);
  margin.setValues(0.0, 0.0);

  QSizeF itemSize = boundingRect().size();
  size[XX] = qRound(itemSize.width());
  size[YY] = qRound(itemSize.height());
}

void ArrowItem::setArrowPath()
{
  InsertData d = meta.value();

  QPointF head = d.arrowHead;
  QPointF tail = d.arrowTail;
  QPointF tip  = d.haftingTip;
  qreal   depth = d.haftingDepth;

  // Guard against degenerate/empty geometry so a bare command still draws.
  if (QLineF(head, tail).length() < 0.5) {
      head = QPointF(20.0, 0.0);
      tail = QPointF(-20.0, 0.0);
  }

  // NOTE: QLineF(head, tail) means dir points from the head (arrow tip) back
  // toward the tail, i.e. along the shaft pointing away from the arrow tip.
  QLineF shaft(head, tail);
  qreal  shaftLen = shaft.length();
  QPointF dir = shaftLen > 0.0 ? QPointF(shaft.dx() / shaftLen, shaft.dy() / shaftLen)
                               : QPointF(1.0, 0.0);
  QPointF perp(-dir.y(), dir.x());

  // Base centre of the head triangle: where the shaft meets the head.
  QPointF baseCentre;
  qreal tipDist = QLineF(head, tip).length();
  bool tipValid = !tip.isNull() && tipDist > 0.5 && tipDist < shaftLen;
  if (depth > 0.0) {
      // Head triangle base sits BEHIND the tip, back along the shaft toward the
      // tail.  dir points from head toward tail, so head + dir*depth is the
      // point `depth` units back from the arrow tip along the shaft.
      baseCentre = head + dir * depth;
  } else if (tipValid) {
      baseCentre = tip;
  } else {
      baseCentre = head + dir * 10.0;
  }


  // Half width of the head triangle (matches RotateIcon arrowTipHeight = 4).
  qreal headHalfWidth = 4.0;

  QPainterPath path;
  path.moveTo(tail);
  path.lineTo(baseCentre);
  path.moveTo(head);
  path.lineTo(baseCentre + perp * headHalfWidth);
  path.lineTo(baseCentre - perp * headHalfWidth);
  path.closeSubpath();

  if (qEnvironmentVariableIsSet("LPUB_STEP_BADGE_DEBUG"))
      fprintf(stderr, "INSERTARROW_GEOM head=(%.1f,%.1f) tail=(%.1f,%.1f) depth=%.1f tip=(%.1f,%.1f) base=(%.1f,%.1f) rect=(%.1f,%.1f,%.1f,%.1f)\n",
              head.x(), head.y(), tail.x(), tail.y(), depth, tip.x(), tip.y(),
              baseCentre.x(), baseCentre.y(),
              path.boundingRect().x(), path.boundingRect().y(),
              path.boundingRect().width(), path.boundingRect().height());

  // Normalise to item-local coordinates with the bounding box at (0,0).
  QRectF br = path.boundingRect();
  path.translate(-br.left(), -br.top());
  setPath(path);
}
