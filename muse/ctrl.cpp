//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ctrl.cpp,v 1.1.2.4 2009/06/10 00:34:59 terminator356 Exp $
//
//    controller handling for mixer automation
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

// Turn on debugging messages
//#define _CTRL_DEBUG_

#include <QLocale>
#include <QColor>

#include <math.h>

#include "gconfig.h"
#include "fastlog.h"
#include "globals.h"
#include "ctrl.h"
#include "midictrl.h"
#include "xml.h"

namespace MusECore {

void CtrlList::initColor(int i)
{
  QColor collist[] = { Qt::red, Qt::yellow, Qt::blue , Qt::black, Qt::white, Qt::green };

  if (i < 6)
    _displayColor = collist[i%6];
  else
    _displayColor = Qt::green;
  _visible = false;
}

//---------------------------------------------------------
//   midi2AudioCtrlValue
//   Apply mapper if it is non-null
//---------------------------------------------------------

double midi2AudioCtrlValue(const CtrlList* audio_ctrl_list, const MidiAudioCtrlStruct* /*mapper*/, int midi_ctlnum, int midi_val)
{
  double fmin, fmax;
  audio_ctrl_list->range(&fmin, &fmax);
  double frng = fmax - fmin;             // The audio control range.
  
  MidiController::ControllerType t = midiControllerType(midi_ctlnum);
  CtrlValueType aud_t = audio_ctrl_list->valueType();
  
  #ifdef _CTRL_DEBUG_
  printf("midi2AudioCtrlValue: midi_ctlnum:%d val:%d fmin:%f fmax:%f\n", midi_ctlnum, midi_val, fmin, fmax);  
  #endif  
  
  int ctlmn = 0;
  int ctlmx = 127;
  
  int bval = midi_val;
  switch(t) 
  {
    case MidiController::RPN:
    case MidiController::NRPN:
    case MidiController::Controller7:
      ctlmn = 0;
      ctlmx = 127;
    break;
    case MidiController::Controller14:
    case MidiController::RPN14:
    case MidiController::NRPN14:
      ctlmn = 0;
      ctlmx = 16383;
    break;
    case MidiController::Program:
      ctlmn = 0;
      ctlmx = 0xffffff;
    break;
    case MidiController::Pitch:
      ctlmn = -8192;
      ctlmx = 8191;
      bval += 8192;
    break;
    case MidiController::Velo:        // cannot happen
    default:
      break;
  }

  double fictlrng = double(ctlmx - ctlmn);   // Float version of the integer midi range.
  double normval = double(bval) / fictlrng;  // Float version of the normalized midi value.

  // ----------  TODO: Do stuff with the mapper, if supplied.
  
  if(aud_t == VAL_LOG)
  {
    // FIXME: Although this should be correct, some sliders show "---" at top end, some don't. 
    // Possibly because of use of fast_log10 in value(), and in sliders and automation IIRC.
    fmin = 20.0*log10(fmin);
    fmax = 20.0*log10(fmax);
    frng = fmax - fmin;
    double ret = exp10((normval * frng + fmin) / 20.0);
    #ifdef _CTRL_DEBUG_
    printf("midi2AudioCtrlValue: is VAL_LOG normval:%f frng:%f returning:%f\n", normval, frng, ret);          
    #endif
    return ret;
  }

  if(aud_t == VAL_LINEAR)
  {
    double ret = normval * frng + fmin;
    #ifdef _CTRL_DEBUG_
    printf("midi2AudioCtrlValue: is VAL_LINEAR normval:%f frng:%f returning:%f\n", normval, frng, ret);       
    #endif
    return ret;
  }

  if(aud_t == VAL_INT)
  {
    double ret = int(normval * frng + fmin);
    #ifdef _CTRL_DEBUG_
    printf("midi2AudioCtrlValue: is VAL_INT returning:%f\n", ret);   
    #endif
    return ret;  
  }
  
  if(aud_t == VAL_BOOL) 
  {
    #ifdef _CTRL_DEBUG_
    printf("midi2AudioCtrlValue: is VAL_BOOL\n");  
    #endif
    //if(midi_val > ((ctlmx - ctlmn)/2 + ctlmn))
    if((normval * frng + fmin) > (frng/2.0 + fmin))
      return fmax;
    else
      return fmin;
  }
  
  printf("midi2AudioCtrlValue: unknown audio controller type:%d\n", aud_t);
  return 0.0;
}      

//---------------------------------------------------------
// Midi to audio controller stuff
//---------------------------------------------------------

MidiAudioCtrlStruct* MidiAudioCtrlPortMap::add_ctrl_struct(int midi_port, int midi_chan, int midi_ctrl_num, int audio_ctrl_id)
{
  iMidiAudioCtrlPortMap imacp = find(midi_port);
  if(imacp == end())
    imacp = insert(std::pair<int, MidiAudioCtrlChanMap >(midi_port, MidiAudioCtrlChanMap() )).first;
  
  iMidiAudioCtrlChanMap imacc = imacp->second.find(midi_chan);
  if(imacc == imacp->second.end())
    imacc = imacp->second.insert(std::pair<int, MidiAudioCtrlMap >(midi_chan, MidiAudioCtrlMap() )).first;
  
  iMidiAudioCtrlMap imac = imacc->second.find(midi_ctrl_num);
  if(imac == imacc->second.end())
    imac = imacc->second.insert(std::pair<int, MidiAudioCtrlStructMap >(midi_ctrl_num, MidiAudioCtrlStructMap() )).first;

  iMidiAudioCtrlStructMap imacs = imac->second.find(audio_ctrl_id);
  if(imacs == imac->second.end())
    imacs = imac->second.insert(std::pair<int, MidiAudioCtrlStruct >(audio_ctrl_id, MidiAudioCtrlStruct() )).first;
  
  return &imacs->second;
}

MidiAudioCtrlStructMap* MidiAudioCtrlPortMap::find_ctrl_map(int midi_port, int midi_chan, int midi_ctrl_num)
{
  iMidiAudioCtrlPortMap imacp = find(midi_port);
  if(imacp == end())
    return NULL;
  
  iMidiAudioCtrlChanMap imacc = imacp->second.find(midi_chan);
  if(imacc == imacp->second.end())
    return NULL;
  
  iMidiAudioCtrlMap imac = imacc->second.find(midi_ctrl_num);
  if(imac == imacc->second.end())
    return NULL;

  return &imac->second;  
}

MidiAudioCtrlStruct* MidiAudioCtrlPortMap::find_ctrl_struct(int midi_port, int midi_chan, int midi_ctrl_num, int audio_ctrl_id)
{
  iMidiAudioCtrlPortMap imacp = find(midi_port);
  if(imacp == end())
    return NULL;
  
  iMidiAudioCtrlChanMap imacc = imacp->second.find(midi_chan);
  if(imacc == imacp->second.end())
    return NULL;
  
  iMidiAudioCtrlMap imac = imacc->second.find(midi_ctrl_num);
  if(imac == imacc->second.end())
    return NULL;

  iMidiAudioCtrlStructMap imacs = imac->second.find(audio_ctrl_id);
  if(imacs == imac->second.end())
    return NULL;
  
  return &imacs->second;  
}

void MidiAudioCtrlPortMap::erase_ctrl_struct(int midi_port, int midi_chan, int midi_ctrl_num, int audio_ctrl_id)
{
  iMidiAudioCtrlPortMap imacp = find(midi_port);
  if(imacp == end())
    return;
  
  iMidiAudioCtrlChanMap imacc = imacp->second.find(midi_chan);
  if(imacc == imacp->second.end())
    return;
  
  iMidiAudioCtrlMap imac = imacc->second.find(midi_ctrl_num);
  if(imac == imacc->second.end())
    return;

  iMidiAudioCtrlStructMap imacs = imac->second.find(audio_ctrl_id);
  if(imacs == imac->second.end())
    return;

  imac->second.erase(imacs);  
  
  if(imac->second.empty())
    imacc->second.erase(imac);
  
  if(imacc->second.empty())
    imacp->second.erase(imacc);
  
  if(imacp->second.empty())
    erase(imacp);
}

void MidiAudioCtrlPortMap::find_audio_ctrl_structs(int audio_ctrl_id, AudioMidiCtrlStructMap* amcs)
{
  for(iMidiAudioCtrlPortMap         imacp = begin();               imacp != end();               ++imacp)
  {
    for(iMidiAudioCtrlChanMap       imacc = imacp->second.begin(); imacc != imacp->second.end(); ++imacc)
    {
      for(iMidiAudioCtrlMap         imac  = imacc->second.begin(); imac != imacc->second.end();  ++imac)
      {
        for(iMidiAudioCtrlStructMap imacs = imac->second.begin();  imacs != imac->second.end();  ++imacs)
        {
          if(imacs->first == audio_ctrl_id)
          {
            //iAudioMidiCtrlStructMap iamcs = 
              amcs->insert(std::pair<int, AudioMidiCtrlStruct>
                    (audio_ctrl_id, AudioMidiCtrlStruct(imacp->first, imacc->first, imac->first) ));
          }
        }
      }
    }
  }
  
}


//---------------------------------------------------------
//   CtrlList
//---------------------------------------------------------

CtrlList::CtrlList()
      {
      _id      = 0;
      _default = 0.0;
      _curVal  = 0.0;
      _mode    = INTERPOLATE;
      _dontShow = false;
      _visible = false;
      initColor(0);
      }

CtrlList::CtrlList(int id)
      {
      _id      = id;
      _default = 0.0;
      _curVal  = 0.0;
      _mode    = INTERPOLATE;
      _dontShow = false;
      _visible = false;
      initColor(id);
      }

CtrlList::CtrlList(int id, QString name, double min, double max, CtrlValueType v, bool dontShow)
{
      _id      = id;
      _default = 0.0;
      _curVal  = 0.0;
      _mode    = INTERPOLATE;
      _name    = name;
      _min     = min;
      _max     = max;
      _valueType = v;
      _dontShow = dontShow;
      _visible = false;
      initColor(id);
}

//---------------------------------------------------------
//   assign
//---------------------------------------------------------

void CtrlList::assign(const CtrlList& l, int flags)
{
  if(flags & ASSIGN_PROPERTIES)
  {
    _id            = l._id;
    _default       = l._default;
    _curVal        = l._curVal;
    _mode          = l._mode;
    _name          = l._name;
    _min           = l._min;
    _max           = l._max;
    _valueType     = l._valueType;
    _dontShow      = l._dontShow;
    _displayColor  = l._displayColor;
    _visible       = l._visible;
  }
  
  if(flags & ASSIGN_VALUES)
  {
    *this = l; // Let the vector assign values.
  }
}

//---------------------------------------------------------
//   value
//---------------------------------------------------------

double CtrlList::value(int frame) const
{
      if(empty()) 
        return _curVal;

      double rv;
      ciCtrl i = upper_bound(frame); // get the index after current frame

      if (i == end()) { // if we are past all items just return the last value
            --i;
            rv = i->second.val;
            }
      else if(_mode == DISCRETE)
      {
        if(i == begin())
        {
            rv = i->second.val;
        }  
        else
        {  
          --i;
          rv = i->second.val;
        }  
      }
      else {
        if (i == begin()) {
            rv = i->second.val;
        }
        else {
            int frame2 = i->second.frame;
            double val2 = i->second.val;
            --i;
            int frame1 = i->second.frame;
            double val1   = i->second.val;

            if (_valueType == VAL_LOG) {
              val1 = 20.0*fast_log10(val1);
              if (val1 < MusEGlobal::config.minSlider)
                val1=MusEGlobal::config.minSlider;
              val2 = 20.0*fast_log10(val2);
              if (val2 < MusEGlobal::config.minSlider)
                val2=MusEGlobal::config.minSlider;
            }

            frame -= frame1;
            val2  -= val1;
            frame2 -= frame1;
            val1 += (double(frame) * val2)/double(frame2);
    
            if (_valueType == VAL_LOG) {
              val1 = exp10(val1/20.0);
            }

            rv = val1;
          }
      }
      return rv;
}

//---------------------------------------------------------
//   curVal
//   returns the static 'manual' value
//---------------------------------------------------------
double CtrlList::curVal() const
{ 
  return _curVal;
}

//---------------------------------------------------------
//   setCurVal
//   Sets the static 'manual' value
//---------------------------------------------------------
void CtrlList::setCurVal(double val)
{
  _curVal = val;
}

//---------------------------------------------------------
//   add
//   Add, or replace, an event at time frame having value val. 
//---------------------------------------------------------

void CtrlList::add(int frame, double val)
      {
      iCtrl e = find(frame);
      if (e != end())
            e->second.val = val;
      else
            insert(std::pair<const int, CtrlVal> (frame, CtrlVal(frame, val)));
      }

//---------------------------------------------------------
//   del
//---------------------------------------------------------

void CtrlList::del(int frame)
      {
      iCtrl e = find(frame);
      if (e == end())
            return;
      
      erase(e);
      }

//---------------------------------------------------------
//   updateCurValues
//   Set the current static 'manual' value (non-automation value) 
//    from the automation value at the given time.
//---------------------------------------------------------

void CtrlList::updateCurValue(int frame)
{
  _curVal = value(frame);
}
      
//---------------------------------------------------------
//   read
//---------------------------------------------------------

void CtrlList::read(Xml& xml)
      {
      QLocale loc = QLocale::c();
      bool ok;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Attribut:
                        if (tag == "id")
                        {
                              _id = loc.toInt(xml.s2(), &ok);
                              if(!ok)
                                printf("CtrlList::read failed reading _id string: %s\n", xml.s2().toLatin1().constData());
                        }
                        else if (tag == "cur")
                        {
                              _curVal = loc.toDouble(xml.s2(), &ok);
                              if(!ok)
                                printf("CtrlList::read failed reading _curVal string: %s\n", xml.s2().toLatin1().constData());
                        }        
                        else if (tag == "visible")
                        {
                              _visible = loc.toInt(xml.s2(), &ok);
                              if(!ok)
                                printf("CtrlList::read failed reading _visible string: %s\n", xml.s2().toLatin1().constData());
                        }
                        else if (tag == "color")
                        {
#if QT_VERSION >= 0x040700
                              ok = _displayColor.isValidColor(xml.s2());
                              if (!ok) {
                                printf("CtrlList::read failed reading color string: %s\n", xml.s2().toLatin1().constData());
                                break;
                              }
#endif
                              _displayColor.setNamedColor(xml.s2());
                        }
                        else
                              printf("unknown tag %s\n", tag.toLatin1().constData());
                        break;
                  case Xml::Text:
                        {
                          int len = tag.length();
                          int frame;
                          double val;
  
                          int i = 0;
                          for(;;) 
                          {
                                while(i < len && (tag[i] == ',' || tag[i] == ' ' || tag[i] == '\n'))
                                  ++i;
                                if(i == len)
                                      break;
                                
                                QString fs;
                                while(i < len && tag[i] != ' ')
                                {
                                  fs.append(tag[i]); 
                                  ++i;
                                }
                                if(i == len)
                                      break;
                                
                                frame = loc.toInt(fs, &ok);
                                if(!ok)
                                {
                                  printf("CtrlList::read failed reading frame string: %s\n", fs.toLatin1().constData());
                                  break;
                                }
                                  
                                while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
                                  ++i;
                                if(i == len)
                                      break;
                                
                                QString vs;
                                while(i < len && tag[i] != ' ' && tag[i] != ',')
                                {
                                  vs.append(tag[i]); 
                                  ++i;
                                }
                                
                                val = loc.toDouble(vs, &ok);
                                if(!ok)
                                {
                                  printf("CtrlList::read failed reading value string: %s\n", vs.toLatin1().constData());
                                  break;
                                }
                                  
                                add(frame, val);
                                
                                if(i == len)
                                      break;
                          }
                        }
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "controller")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void CtrlListList::add(CtrlList* vl)
      {
      insert(std::pair<const int, CtrlList*>(vl->id(), vl));
      }

//---------------------------------------------------------
//   value
//---------------------------------------------------------

double CtrlListList::value(int ctrlId, int frame, bool cur_val_only) const
      {
      ciCtrlList cl = find(ctrlId);
      if (cl == end())
            return 0.0;

      if(cur_val_only)
        return cl->second->curVal();
      
      return cl->second->value(frame);  
      }

//---------------------------------------------------------
//   updateCurValues
//   Set the current 'manual' values (non-automation values) 
//    from the automation values at the given time.
//   This is typically called right after a track's automation type changes
//    to OFF, so that the manual value becomes the last automation value.
//   There are some interesting advantages to having completely independent 
//    'manual' and automation values, but the jumping around when switching to OFF
//    becomes disconcerting.
//---------------------------------------------------------

void CtrlListList::updateCurValues(int frame)
{
  for(ciCtrlList cl = begin(); cl != end(); ++cl)
    cl->second->updateCurValue(frame);
}
      
} // namespace MusECore