/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 University of Saskatchewan
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
 * Author: Peggy Anderson <peggy.anderson@usask.ca>
 */

#ifndef CLASS_C_END_DEVICE_LORAWAN_MAC_H
#define CLASS_C_END_DEVICE_LORAWAN_MAC_H

#include "ns3/lorawan-mac.h"
#include "ns3/end-device-lorawan-mac.h"

namespace ns3 {
namespace lorawan {

/**
 * Class representing the MAC layer of a Class C LoRaWAN device.
 */
class ClassCEndDeviceLorawanMac : public EndDeviceLorawanMac
{
public:
  static TypeId GetTypeId (void);

  ClassCEndDeviceLorawanMac ();
  virtual ~ClassCEndDeviceLorawanMac ();

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

  void ScheduleEvents (Time RxcClose, Time Rx1Open, Time Rx2Open);

  void OpenContinuousReceiveWindow (void);

  void CloseContinuousReceiveWindow (void);

  /**
   * Perform the actions that are required after a packet send.
   *
   * This function handles opening of the first receive window.
   */
  virtual void TxFinished (Ptr<const Packet> packet);

  virtual void ResetReceiveWindows (EndDeviceLorawanMac::ClassCReceiveWindows rw);
  
  /**
   * Cancel all receive windows
   */
  virtual void CancelReceiveWindows (void);

protected:
  /**
   * The event of the first receive window opening.
   *
   * This Event is used to cancel the first window in case the first occurence of the
   * second receive window is successful.
   */
  EventId m_firstReceiveWindow;

  /**
   * The event of the continuous receive window opening.
   *
   * This Event is used to cancel the receive window in case the first occurence of the
   * second receive window or the first receive window is successful.
   */
  EventId m_continuousReceiveWindow1;
  EventId m_continuousReceiveWindow2;
  EventId m_continuousReceiveWindow3;

  /**
     * The event of the closing the continuous receive window.
     *
     * This Event will be canceled if there's a successful reception of a packet
     * in RX1.
     */
  EventId m_closeContinuousWindow;

  Time m_rx1DelayRemaining;
  Time m_rx2DelayRemaining;

}; /* ClassCEndDeviceLorawanMac */

} /* namespace lorawan */
} /* namespace ns3 */

#endif /* CLASS_C_END_DEVICE_LORAWAN_MAC_H */
