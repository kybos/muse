//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: rack.h,v 1.5.2.3 2006/09/24 19:32:31 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#ifndef __RACK_H__
#define __RACK_H__

#include <QListWidget>

class QDragEnterEvent;
class QDragLeaveEvent;
class QDropEvent;
class QMouseEvent;

class AudioTrack;
class Xml;

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

class EffectRack : public QListWidget {
      Q_OBJECT
    
    
      AudioTrack* track;
      int itemheight;
      
      virtual QSize minimumSizeHint() const;
      virtual QSize sizeHint() const;
      
      void startDrag(int idx);
      void initPlugin(Xml xml, int idx);
      QPoint dragPos;
      void savePreset(int idx);
      void choosePlugin(QListWidgetItem* item, bool replace = false);

   private slots:
      void menuRequested(QListWidgetItem*);
      void doubleClicked(QListWidgetItem*);
      void songChanged(int);
      void updateContents();

   protected:
      void dropEvent(QDropEvent *event);
      void dragEnterEvent(QDragEnterEvent *event);
      void mousePressEvent(QMouseEvent *event);
      void mouseMoveEvent(QMouseEvent *event);

      QStringList mimeTypes() const;
      Qt::DropActions supportedDropActions () const;
   
   public:
      EffectRack(QWidget*, AudioTrack* t);
      ~EffectRack();
      
      AudioTrack* getTrack() { return track; } 
      QPoint getDragPos() { return dragPos; }
      };

#endif
