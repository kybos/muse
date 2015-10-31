//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: route.h,v 1.5.2.1 2008/05/21 00:28:52 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011, 2015 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __ROUTE_H__
#define __ROUTE_H__

#include <QMetaType>

#include <vector>
#include "globaldefs.h"

#define ROUTE_PERSISTENT_NAME_SIZE 256    // Size of char array Route::persistentName, including the terminating null character.

class QString;

namespace MusECore {

class Track;
class MidiDevice;
class Xml;
class PendingOperationList;

//---------------------------------------------------------
//   Route
//---------------------------------------------------------

class Route {
  public:
      enum RouteType { TRACK_ROUTE=0, JACK_ROUTE=1, MIDI_DEVICE_ROUTE=2, MIDI_PORT_ROUTE=3 }; 
      
      union {
            Track* track;
            MidiDevice* device;      
            void* jackPort;
	    void* voidPointer;
            };
      
      int midiPort;              // Midi port number. Best not to put this in the union to avoid problems?
      
      //snd_seq_addr_t alsaAdr;  // TODO
      
      // Midi channel, or starting audio source channel (of the owner of this route).
      // Normally zero for mono or stereo audio tracks, higher for multi-channel audio tracks.
      int channel;                             
      // Number of (audio) channels being routed. 
      int channels;
      // Starting (audio) destination channel of the remote object (pointed to by the union or port etc.).
      int remoteChannel;
      
      RouteType type;

      // Always same as the port name. When connection disappears, this holds on to the name.
      char persistentJackPortName[ROUTE_PERSISTENT_NAME_SIZE]; 
      
      Route(void* t, int ch=-1);
      Route(Track* t, int ch = -1, int chans = -1);
      Route(MidiDevice* d, int ch = -1);  
      Route(int port, int ch = -1);         
      Route(const QString&, bool dst, int ch, int rtype = -1);
      Route();
      Route(const Route&); // Copy constructor
      // Useful for generic complete construction.
      Route(RouteType type_, int midi_port_num_, void* void_pointer_, int channel_, int channels_, int remote_channel_, const char* name_);
      
      // Create string name representation.
      // preferred_name_or_alias (mainly for Jack routes): -1: No preference 0: Prefer canonical name 1: Prefer 1st alias 2: Prefer 2nd alias.
      QString name(int preferred_name_or_alias = -1) const;
      // Fill and return str char name representation.
      // preferred_name_or_alias (mainly for Jack routes): -1: No preference 0: Prefer canonical name 1: Prefer 1st alias 2: Prefer 2nd alias.
      char* name(char* str, int str_size, int preferred_name_or_alias = -1) const;
      bool operator==(const Route&) const;
      // If the routes support channels, if the given route's channel is -1 meaning all channels, compare matches ONLY if this channel == -1, 
      //  and if the given route's channel is >= 0, compare matches on ANY channel. Useful for example finding router treeview items.
      bool compare(const Route&) const;
      bool exists() const;
      Route& operator=(const Route&);
      bool isValid() const {
            return ((type == TRACK_ROUTE) && (track != 0)) || 
                   (type == JACK_ROUTE) ||  // For persistent Jack routes: A NULL jackPort is actually valid.
                   ((type == MIDI_DEVICE_ROUTE) && (device != 0)) ||
                   ((type == MIDI_PORT_ROUTE) && (midiPort >= 0) && (midiPort < MIDI_PORTS));   
            }
      void read(Xml& xml);
      void dump() const;
      };

//---------------------------------------------------------
//   RouteList
//---------------------------------------------------------

class RouteList : public std::vector <Route> {
  public:  
      iterator find(const Route& r)             { return std::find(begin(), end(), r); }
      const_iterator find(const Route& r) const { return std::find(begin(), end(), r); }
      bool contains(const Route& r) const         { return std::find(begin(), end(), r) != end(); }
      void removeRoute(const Route& r)          { iterator i = std::find(begin(), end(), r);  if(i != end()) erase(i); }
      };

typedef RouteList::iterator iRoute;
typedef RouteList::const_iterator ciRoute;

extern void addRoute(Route src, Route dst);
extern void removeRoute(Route src, Route dst);
extern void removeAllRoutes(Route src, Route dst);
extern Route name2route(const QString&, bool dst, int rtype = -1);
// Returns true if the routes are found and they are connected.
extern bool routeCanDisconnect(const Route& src, const Route& dst);
// Returns true if the routes are found and they are not connected and CAN be connected.
extern bool routeCanConnect(const Route& src, const Route& dst);
// Returns true if the routes are found and they CAN be connected (even if they are already connected).
// If check_types_only is true, it only compares route types.
// Otherwise other parameters such as channels are also compared.
extern bool routesCompatible(const Route& src, const Route& dst, bool check_types_only = false);

} // namespace MusECore

// Allow Routes to be a QVariant
Q_DECLARE_METATYPE(MusECore::Route)

#endif
