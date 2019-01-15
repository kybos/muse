//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrledit.h,v 1.4.2.1 2008/05/21 00:28:53 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __CTRL_EDIT_H__
#define __CTRL_EDIT_H__

#include <QWidget>

#include "ctrlcanvas.h"
#include "song.h"
// REMOVE Tim. citem. Added.
#include "event_tag_list.h"

namespace MusECore {
class Xml;
}

#define CTRL_PANEL_FIXED_WIDTH 40

namespace MusEGui {

class MidiEditor;
class CtrlView;
class CtrlPanel;

//---------------------------------------------------------
//   CtrlEdit
//---------------------------------------------------------

class CtrlEdit : public QWidget {
      Q_OBJECT
      CtrlCanvas* canvas;
      CtrlPanel* panel;

   private slots:
      void destroy();

   public slots:
      void setTool(int tool);
      void setXPos(int val)           { canvas->setXPos(val); }
      void setXMag(int val)           { canvas->setXMag(val); }
      void setCanvasWidth(int w);
      void setController(int n);
      void curPartHasChanged(MusECore::Part*);
      
   signals:
      void timeChanged(unsigned);
      void destroyedCtrl(CtrlEdit*);
      void enterCanvas();
      void yposChanged(int);
      void redirectWheelEvent(QWheelEvent*);

   public:
      CtrlEdit(QWidget*, MidiEditor* e, int xmag,
         bool expand = false, const char* name = 0);
      void readStatus(MusECore::Xml&);
      void writeStatus(int, MusECore::Xml&);
      void setController(const QString& name);
      bool itemsAreSelected() const { if(!canvas) return false; return canvas->itemsAreSelected(); }
// REMOVE Tim. citem. Added.
//       // Adds all selected items to the given list. Does not clear the list first.
//       // Checks for duplicates, employing the 'tagged' features.
//       //void getAllSelectedItems(CItemSet& list) const { if(!canvas) return; canvas->getAllSelectedItems(list); }
//       // Tags all selected item objects. Checks for duplicates, employing the 'tagged' features.
//       void tagItems(bool tagAllItems = false, bool tagAllParts = false, bool range = false,
//         const MusECore::Pos& p0 = MusECore::Pos(),
//         const MusECore::Pos& p1 = MusECore::Pos()) const {
//         if(canvas) canvas->tagItems (tagAllItems, tagAllParts, range, p0, p1); }
      // Appends given tag list with item objects according to options. Avoids duplicate events or clone events.
      // Special: We 'abuse' a controller event's length, normally 0, to indicate visual item length.
      void tagItems(MusECore::TagEventList* tag_list, const MusECore::EventTagOptionsStruct& options) const
      { if(canvas) canvas->tagItems(tag_list, options); }
      };

      
typedef std::list<CtrlEdit*> CtrlEditList;
typedef CtrlEditList::iterator iCtrlEdit;
typedef CtrlEditList::const_iterator ciCtrlEdit;
      
} // namespace MusEGui

#endif

