/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Padova
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Davide Magrin <magrinda@dei.unipd.it>
 *         Martina Capuzzo <capuzzom@dei.unipd.it>
 *
 * Modified by: Peggy Anderson <peggy.anderson@usask.ca>
 */

#ifndef CLASS_A_END_DEVICE_LORAWAN_MAC_H
#define CLASS_A_END_DEVICE_LORAWAN_MAC_H

#include "ns3/lorawan-mac.h"                // Packet
#include "ns3/end-device-lorawan-mac.h"     // EndDeviceLorawanMac
#include "ns3/lora-frame-header.h"          // RxParamSetupReq
// #include "ns3/random-variable-stream.h"
#include "ns3/lora-device-address.h"
// #include "ns3/traced-value.h"

namespace ns3 {
namespace lorawan {

/**
 * Class representing the MAC layer of a Class A LoRaWAN device.
 */
class ClassAEndDeviceLorawanMac : public EndDeviceLorawanMac
{
public:
  static TypeId GetTypeId (void);

  ClassAEndDeviceLorawanMac ();
  virtual ~ClassAEndDeviceLorawanMac ();

  //////////////////////////
  //  Receiving methods   //
  //////////////////////////

  /**
   * Receive a packet.
   *
   * This method is typically registered as a callback in the underlying PHY
   * layer so that it's called when a packet is going up the stack.
   *
   * \param packet the received packet.
   */
  virtual void Receive (Ptr<Packet const> packet);

  /**
   * Perform the actions that are required after a packet send.
   *
   * This function handles opening of the first receive window.
   */
  virtual void TxFinished (Ptr<const Packet> packet);

  /**
   * Cancel all receive windows
   */
  virtual void CancelReceiveWindows (void) {};

}; /* ClassAEndDeviceLorawanMac */
} /* namespace lorawan */
} /* namespace ns3 */
#endif /* CLASS_A_END_DEVICE_LORAWAN_MAC_H */
