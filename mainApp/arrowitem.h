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

/****************************************************************************
 *
 * This file implements a graphics item used to render the LPub INSERT ARROW
 * metacommand.  The upstream parser/emitter supports the ARROW insert type
 * (meta.cpp InsertMeta::parse/format) but never implemented its rendering:
 * formatpage.cpp treated InsertData::InsertArrow as an empty case.
 *
 * Geometry semantics (no upstream implementation exists; derived from the
 * field names in InsertData):
 *   arrowHead     - tip of the arrow head triangle
 *   arrowTail     - far end of the arrow shaft
 *   haftingTip    - point on the shaft where the head triangle base meets it
 *   haftingDepth  - depth of the head triangle measured along the shaft
 *
 * The item is placed on the page using the standard Placement machinery,
 * exactly like InsertPixmapItem (see InsertPicture case in formatpage.cpp).
 *
 ***************************************************************************/

#ifndef ARROWITEM_H
#define ARROWITEM_H

#include <QGraphicsPathItem>
#include "placement.h"
#include "metatypes.h"
#include "meta.h"
#include "declarations.h"

class ArrowItem : public QGraphicsPathItem, public MetaItem, public Placement
{
  public:
    ArrowItem(InsertMeta insertMeta, QGraphicsItem *parent = nullptr);

    InsertMeta meta;

    void setArrowPath();
};

#endif
