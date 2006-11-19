//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2006 by Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "splitlayergui.h"
#include "splitlayer.h"
#include "midi.h"
#include "midievent.h"

//---------------------------------------------------------
//   SplitLayer
//---------------------------------------------------------

SplitLayer::SplitLayer(const char* name, const MempiHost* h)
   : Mempi(name, h)
      {
      gui = 0;
      }

//---------------------------------------------------------
//   SplitLayer
//---------------------------------------------------------

SplitLayer::~SplitLayer()
      {
      if (gui)
            delete gui;
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool SplitLayer::init()
      {
      data.startVelo[0]   = 0;
      data.endVelo[0]     = 128;
      data.startPitch[0]  = 0;
      data.endPitch[0]    = 128;
      data.pitchOffset[0] = 0;
      data.veloOffset[0]  = 0;
      for (int i = 1; i < MIDI_CHANNELS; ++i) {
            data.startVelo[i]   = 0;
            data.endVelo[i]     = 0;
            data.startPitch[i]  = 0;
            data.endPitch[i]    = 0;
            data.pitchOffset[i] = 0;
            data.veloOffset[i]  = 0;
            }
      memset(notes, 0, 128 * sizeof(int));
      learnMode = false;
      gui = new SplitLayerGui(this, 0);
      gui->hide();
      gui->setWindowTitle(QString(name()));
      gui->init();
      return false;
      }

//---------------------------------------------------------
//   getGeometry
//---------------------------------------------------------

void SplitLayer::getGeometry(int* x, int* y, int* w, int* h) const
      {
      QPoint pos(gui->pos());
      QSize size(gui->size());
      *x = pos.x();
      *y = pos.y();
      *w = size.width();
      *h = size.height();
      }

//---------------------------------------------------------
//   setGeometry
//---------------------------------------------------------

void SplitLayer::setGeometry(int x, int y, int w, int h)
      {
      gui->resize(QSize(w, h));
      gui->move(QPoint(x, y));
      }

//---------------------------------------------------------
//   process
//---------------------------------------------------------

void SplitLayer::process(unsigned, unsigned, MidiEventList* il, MidiEventList* ol)
      {
      for (iMidiEvent i = il->begin(); i != il->end(); ++i) {
            if (i->type() != ME_NOTEON && i->type() != ME_NOTEOFF) {
                  ol->insert(*i);
                  continue;
                  }
            int pitch = i->dataA();
            int velo  = i->dataB();
            if (learnMode) {
                  if (learnStartPitch)
                        data.startPitch[learnChannel] = pitch;
                  else
                        data.endPitch[learnChannel] = pitch;
                  learnMode = false;
                  gui->sendResetLearnMode();
                  return;
                  }
            if (i->type() == ME_NOTEON && velo) {
                  for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
                        MidiEvent event(*i);
                        if (pitch >= data.startPitch[ch] 
                           && pitch <= data.endPitch[ch]
                           && velo >= data.startVelo[ch]
                           && velo <= data.endVelo[ch]) {
                              notes[pitch] |= (1 << ch);
                              event.setChannel(ch);
                              int p = pitch;
                              int v = velo;
                              p += data.pitchOffset[ch];
                              if (p > 127)
                                    p = 127;
                              else if (p < 0)
                                    p = 0;
                              v += data.veloOffset[ch];
                              if (v > 127)
                                    v = 127;
                              else if (v < 0)
                                    v = 0;
                              event.setA(p);
                              event.setB(v);
                              ol->insert(event);
                              }
                        }
                  }
            else {
                  for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
                        if (notes[pitch] & (1 << ch)) {
                              MidiEvent event(*i);
                              event.setChannel(ch);
                              int p = pitch + data.pitchOffset[ch];
                              if (p < 0)
                                    p = 0;
                              else if (p > 127)
                                    p = 127;
                              event.setA(p);
                              ol->insert(event);
                              }
                        }
                  notes[pitch] = 0;
                  }
            }
      }

//---------------------------------------------------------
//   getInitData
//---------------------------------------------------------

void SplitLayer::getInitData(int* n, const unsigned char** p) const
      {
      *n = sizeof(data);
      *p = (unsigned char*)&data;
      }

//---------------------------------------------------------
//   setInitData
//---------------------------------------------------------

void SplitLayer::setInitData(int n, const unsigned char* p)
      {
      memcpy((void*)&data, p, n);
      if (gui)
            gui->init();
      }

//---------------------------------------------------------
//   inst
//---------------------------------------------------------

static Mempi* instantiate(const char* name, const MempiHost* h)
      {
      return new SplitLayer(name, h);
      }

extern "C" {
      static MEMPI descriptor = {
            "SplitLayer",
            "MusE Midi Splits and Layers",
            "0.1",      // version string
            MEMPI_FILTER,
            MEMPI_MAJOR_VERSION, MEMPI_MINOR_VERSION,
            instantiate
            };

      const MEMPI* mempi_descriptor() { return &descriptor; }
      }

