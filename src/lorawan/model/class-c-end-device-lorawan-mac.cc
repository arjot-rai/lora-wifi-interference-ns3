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

#include "ns3/class-c-end-device-lorawan-mac.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/log.h"

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("ClassCEndDeviceLorawanMac");

NS_OBJECT_ENSURE_REGISTERED (ClassCEndDeviceLorawanMac);

TypeId
ClassCEndDeviceLorawanMac::GetTypeId (void)
{
static TypeId tid = TypeId ("ns3::ClassCEndDeviceLorawanMac")
  .SetParent<EndDeviceLorawanMac> ()
  .SetGroupName ("lorawan")
  .AddTraceSource ("NumberOfContinuousReceiveWindowCalls",
                   "Count how many times a continuous receive window "
                   "has been opened.",
                    MakeTraceSourceAccessor
                      (&EndDeviceLorawanMac::m_numContinuousReceiveWindows),
                    "ns3::TracedValueCallback::Int")
  .AddConstructor<ClassCEndDeviceLorawanMac> ();
return tid;
}

ClassCEndDeviceLorawanMac::ClassCEndDeviceLorawanMac () 
  : m_rx1DelayRemaining (Seconds (0.0)),
    m_rx2DelayRemaining (Seconds (0.0))
{
  NS_LOG_FUNCTION (this);

  SetDeviceClass(EndDeviceLorawanMac::CLASS_C);
  //m_deviceClass = EndDeviceLorawanMac::CLASS_C;

  m_firstReceiveWindow = EventId ();
  m_firstReceiveWindow.Cancel ();
  m_continuousReceiveWindow1 = EventId ();
  m_continuousReceiveWindow1.Cancel ();
  m_continuousReceiveWindow2 = EventId ();
  m_continuousReceiveWindow2.Cancel ();
  m_continuousReceiveWindow3 = EventId ();
  m_continuousReceiveWindow3.Cancel ();
  m_closeContinuousWindow = EventId ();
  m_closeContinuousWindow.Cancel ();

  // this->ScheduleEvents(Seconds(0) - Seconds(1), m_receiveDelay1, m_receiveDelay2);

  // Schedule the opening of the first occurence of the continuous receive window
  SetDeviceCurrentReceiveWindow(EndDeviceLorawanMac::RXC);
  m_continuousReceiveWindow1 = Simulator::ScheduleNow (&ClassCEndDeviceLorawanMac::OpenContinuousReceiveWindow, 
                                                 this);
}

ClassCEndDeviceLorawanMac::~ClassCEndDeviceLorawanMac ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

//////////////////////////
//  Receiving methods   //
//////////////////////////

void
ClassCEndDeviceLorawanMac::Receive (Ptr<Packet const> packet)
{
  NS_LOG_FUNCTION (this << packet);

  // Work on a copy of the packet
  Ptr<Packet> packetCopy = packet->Copy ();

  // Remove the Mac Header to get some information
  LorawanMacHeader mHdr;
  packetCopy->RemoveHeader (mHdr);

  NS_LOG_DEBUG ("Mac Header: " << mHdr);

  // Only keep analyzing the packet if it's downlink
  if (!mHdr.IsUplink ())
    {
      NS_LOG_INFO ("Found a downlink packet.");

      // Remove the Frame Header
      LoraFrameHeader fHdr;
      fHdr.SetAsDownlink ();
      packetCopy->RemoveHeader (fHdr);

      NS_LOG_DEBUG ("Frame Header: " << fHdr);

      // Determine whether this packet is for us
      bool messageForUs = (m_address == fHdr.GetAddress ());

      Time Rx1DelayRemaining = Time(0);
      Time Rx2DelayRemaining = Time(0);
      // Time RxcCloseDelayRemaining = Time(0);

      // Keep track of time remaining before the next events are to execute
      Rx1DelayRemaining = Simulator::GetDelayLeft (m_firstReceiveWindow);
      Rx2DelayRemaining = Simulator::GetDelayLeft (m_secondReceiveWindow);

      if (m_rx1DelayRemaining != Time (Seconds (0.0)) || m_rx2DelayRemaining != Time (Seconds (0.0)))
        {
          Rx1DelayRemaining = m_rx1DelayRemaining;
          Rx2DelayRemaining = m_rx2DelayRemaining;
          NS_LOG_DEBUG (Rx1DelayRemaining << " " << Rx2DelayRemaining);
        }

      if (messageForUs)
        {
          NS_LOG_INFO ("The message is for us!");

          // If it exists, check which receive window the packet was
          // received in.  
          // Note: A scheduled event is marked as expired as soon as it starts
          if (GetDeviceCurrentReceiveWindow () == EndDeviceLorawanMac::RXC)
          // if (m_continuousReceiveWindow1.IsExpired () && 
          //     !m_firstReceiveWindow.IsExpired () &&
          //     !m_secondReceiveWindow.IsExpired ())
            {
              NS_LOG_DEBUG ("Packet was received before RX1");
              
              Simulator::Cancel (m_closeContinuousWindow);
              Simulator::Cancel (m_firstReceiveWindow);
              Simulator::Cancel (m_continuousReceiveWindow2);
              Simulator::Cancel (m_secondReceiveWindow);
              Simulator::Cancel (m_continuousReceiveWindow3);
              
              // Reschedule cancelled events
              this->ScheduleEvents(Rx1DelayRemaining, Rx1DelayRemaining, Rx2DelayRemaining);
            }
          else if (GetDeviceCurrentReceiveWindow () == EndDeviceLorawanMac::RX1)
          // else if (!m_continuousReceiveWindow2.IsExpired () && 
          //          m_firstReceiveWindow.IsExpired () &&
          //          !m_secondReceiveWindow.IsExpired ())
            {
              NS_LOG_DEBUG ("Packet was received in RX1");

              Simulator::Cancel (m_closeFirstWindow);
              Simulator::Cancel (m_continuousReceiveWindow2);
              Simulator::Cancel (m_secondReceiveWindow);
              Simulator::Cancel (m_continuousReceiveWindow3);

              // Reschedule cancelled events
              this->ScheduleEvents(Seconds(0) - Seconds(1), Rx1DelayRemaining, Rx2DelayRemaining);
            }    
          else if (GetDeviceCurrentReceiveWindow () == EndDeviceLorawanMac::RXC2)
          // else if (m_continuousReceiveWindow2.IsExpired () && 
          //          m_firstReceiveWindow.IsExpired () &&
          //          !m_secondReceiveWindow.IsExpired ())
            {
              NS_LOG_DEBUG ("Packet was received after RX1 but before RX2");

              Simulator::Cancel (m_closeContinuousWindow);
              Simulator::Cancel (m_secondReceiveWindow);
              Simulator::Cancel (m_continuousReceiveWindow3);

              // Reschedule cancelled events
              this->ScheduleEvents(Rx2DelayRemaining, Seconds(0) - Seconds(1), Rx2DelayRemaining);
            }    
          else if (GetDeviceCurrentReceiveWindow () == EndDeviceLorawanMac::RX2)
          // else if (!m_continuousReceiveWindow3.IsExpired () &&
                  //  m_firstReceiveWindow.IsExpired () &&
                  //  m_secondReceiveWindow.IsExpired ())
            {
              NS_LOG_DEBUG ("Packet was received in RX2");

              Simulator::Cancel (m_closeSecondWindow);
              Simulator::Cancel (m_continuousReceiveWindow3);

              // Reschedule cancelled events
              this->ScheduleEvents(Seconds(0) - Seconds(1), Rx1DelayRemaining, Rx2DelayRemaining);
            }
          else 
            {
              NS_LOG_DEBUG ("Packet was received in the final RXC");
            }
          

          // Parse the MAC commands
          ParseCommands (fHdr);

          // TODO Pass the packet up to the NetDevice

          // Call the trace source
          m_receivedPacket (packet);
        }
      else
        {
          NS_LOG_DEBUG ("The message is intended for another recipient.");

          // In this case, we are either receiving in the first receive window
          // and finishing reception inside the second one, or receiving a
          // packet in the second receive window and finding out, after the
          // fact, that the packet is not for us. In either case, if we no
          // longer have any retransmissions left, we declare failure.
          if (m_retxParams.waitingAck && m_secondReceiveWindow.IsExpired ())
            {
              if (m_retxParams.retxLeft == 0)
                {
                  uint8_t txs = m_maxNumbTx - (m_retxParams.retxLeft);
                  m_requiredTxCallback (txs, false, m_retxParams.firstAttempt, m_retxParams.packet);
                  NS_LOG_DEBUG ("Failure: no more retransmissions left. Used " << unsigned(txs) << " transmissions.");

                  // Reset retransmission parameters
                  resetRetransmissionParameters ();
                }
              else       // Reschedule
                {
                  this->Send (m_retxParams.packet);
                  NS_LOG_INFO ("We have " << unsigned(m_retxParams.retxLeft) << " retransmissions left: rescheduling transmission.");
                }
            }
        }
    }
  else if (m_retxParams.waitingAck && m_secondReceiveWindow.IsExpired ())
    {
      NS_LOG_INFO ("The packet we are receiving is in uplink.");
      if (m_retxParams.retxLeft > 0)
        {
          this->Send (m_retxParams.packet);
          NS_LOG_INFO ("We have " << unsigned(m_retxParams.retxLeft) << " retransmissions left: rescheduling transmission.");
        }
      else
        {
          uint8_t txs = m_maxNumbTx - (m_retxParams.retxLeft);
          m_requiredTxCallback (txs, false, m_retxParams.firstAttempt, m_retxParams.packet);
          NS_LOG_DEBUG ("Failure: no more retransmissions left. Used " << unsigned(txs) << " transmissions.");

          // Reset retransmission parameters
          resetRetransmissionParameters ();
        }
    } 
}

void
ClassCEndDeviceLorawanMac::ScheduleEvents (Time RxcClose, Time Rx1Open, Time Rx2Open)
{
  NS_LOG_FUNCTION (this);
  double tSymRx1 = pow (2, GetSfFromDataRate (GetFirstReceiveWindowDataRate ())) / GetBandwidthFromDataRate ( GetFirstReceiveWindowDataRate ());
  Time firstReceiveWindowDuration = Seconds (m_receiveWindowDurationInSymbols*tSymRx1);
  double tSymRx2 = pow (2, GetSfFromDataRate (GetSecondReceiveWindowDataRate ())) / GetBandwidthFromDataRate ( GetSecondReceiveWindowDataRate ());
  Time secondReceiveWindowDuration = Seconds (m_receiveWindowDurationInSymbols*tSymRx2);            

  if (RxcClose.IsPositive ())
    {
      m_closeContinuousWindow = Simulator::Schedule (RxcClose - Seconds (0.000000001),
                                                     &ClassCEndDeviceLorawanMac::CloseContinuousReceiveWindow,
                                                     this);
    }
  
  if (Rx1Open.IsStrictlyPositive ())    // Packet is received before RX1 (RXC1)
    {
      m_firstReceiveWindow = Simulator::Schedule (Rx1Open,
                                                  &ClassCEndDeviceLorawanMac::OpenFirstReceiveWindow, 
                                                  this);

      m_continuousReceiveWindow2 = Simulator::Schedule (Rx1Open + firstReceiveWindowDuration + Seconds (0.000000001),
                                                        &ClassCEndDeviceLorawanMac::OpenContinuousReceiveWindow,
                                                        this);
    }
  else if (Rx1Open.IsZero () && !Rx2Open.IsZero ())     // Packet is received in RX1
    {
      m_closeFirstWindow = Simulator::ScheduleNow (&ClassCEndDeviceLorawanMac::CloseFirstReceiveWindow,
                                                   this);

      // m_secondReceiveWindow might need to be scheduled first...
      // TODO: figure it out
      m_continuousReceiveWindow2 = Simulator::Schedule (Seconds (0.000000001),
                                                        &ClassCEndDeviceLorawanMac::OpenContinuousReceiveWindow,
                                                        this);
    }
  
  if (Rx2Open.IsStrictlyPositive ())
    {
      m_secondReceiveWindow = Simulator::Schedule (Rx2Open,
                                                   &ClassCEndDeviceLorawanMac::OpenSecondReceiveWindow,
                                                   this);

      m_continuousReceiveWindow3 = Simulator::Schedule (Rx2Open + secondReceiveWindowDuration + Seconds (0.000000001),
                                                        &ClassCEndDeviceLorawanMac::OpenContinuousReceiveWindow,
                                                        this);
    }
  else if (Rx2Open.IsZero ())
    {
      m_closeSecondWindow = Simulator::ScheduleNow (&ClassCEndDeviceLorawanMac::CloseSecondReceiveWindow,
                                                    this);

      m_continuousReceiveWindow3 = Simulator::Schedule (Seconds (0.000000001), 
                                                        &ClassCEndDeviceLorawanMac::OpenContinuousReceiveWindow,
                                                        this);      
    }
}

void
ClassCEndDeviceLorawanMac::OpenContinuousReceiveWindow (void)
{ 
  NS_LOG_FUNCTION (this);

  // This is where if windows are being reset... This will be set to RXC3?
  if (GetDeviceCurrentReceiveWindow () != EndDeviceLorawanMac::RXC)
    {
      SetDeviceCurrentReceiveWindow(EndDeviceLorawanMac::RXC3);
    }

  if (GetDeviceCurrentReceiveWindow () == EndDeviceLorawanMac::RXC &&
      m_firstReceiveWindow.IsExpired () )
    {
      this->ScheduleEvents(Seconds(0) - Seconds(1), m_receiveDelay1, m_receiveDelay2);
    }

  Time RxDelay = Time(0);

  NS_LOG_DEBUG(m_numContinuousReceiveWindows);
  Simulator::ScheduleNow (&ClassCEndDeviceLorawanMac::OpenSecondReceiveWindow,
                          this);

  m_numContinuousReceiveWindows += 1;

  if (m_numContinuousReceiveWindows == 1)
    {
      // Close RXC when RX1 opens
      RxDelay = Simulator::GetDelayLeft (m_firstReceiveWindow) - Seconds (0.000000001);
      
      if (RxDelay.IsNegative ())
        {
          // This should only be true when the device is being initialized
          RxDelay = Seconds (1) - Seconds (0.000000001);
          NS_LOG_DEBUG ("RxDelay: " << RxDelay);
        }

      m_closeContinuousWindow = Simulator::Schedule (RxDelay,
                                                     &ClassCEndDeviceLorawanMac::CloseContinuousReceiveWindow, 
                                                     this);
    }
  else if (m_numContinuousReceiveWindows == 2)
    {
      SetDeviceCurrentReceiveWindow(EndDeviceLorawanMac::RXC2);

      // Close RXC when RX2 opens
      RxDelay = Simulator::GetDelayLeft (m_secondReceiveWindow);
      m_closeContinuousWindow = Simulator::Schedule (RxDelay - Seconds (0.000000001),
                                                     &ClassCEndDeviceLorawanMac::CloseContinuousReceiveWindow, 
                                                     this);
    }
}

void
ClassCEndDeviceLorawanMac::CloseContinuousReceiveWindow (void)
{
  NS_LOG_FUNCTION (this);

  Simulator::ScheduleNow (&ClassCEndDeviceLorawanMac::CloseSecondReceiveWindow,
                          this);
}

void
ClassCEndDeviceLorawanMac::TxFinished (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  NS_LOG_DEBUG ("TxFinished");

  this->ScheduleEvents(Seconds(0) - Seconds(1), m_receiveDelay1, m_receiveDelay2);

  // Schedule the opening of the first occurence of the continuous receive window
  SetDeviceCurrentReceiveWindow(EndDeviceLorawanMac::RXC);
  m_numContinuousReceiveWindows = 0;
  m_continuousReceiveWindow1 = Simulator::ScheduleNow (&ClassCEndDeviceLorawanMac::OpenContinuousReceiveWindow,
                                                       this);  
}

void
ClassCEndDeviceLorawanMac::ResetReceiveWindows (enum ClassCReceiveWindows rw)
{
  NS_LOG_FUNCTION (this << rw);
  // Time RxcCloseDelayRemaining = Time(0);

  // Keep track of time remaining before the next events are to execute
  m_rx1DelayRemaining = Simulator::GetDelayLeft (m_firstReceiveWindow);
  m_rx2DelayRemaining = Simulator::GetDelayLeft (m_secondReceiveWindow);
  
  // if (rw == EndDeviceLorawanMac::RXC)
  //   {
  //     CancelReceiveWindows ();
  //   }
  // else if (rw == EndDeviceLorawanMac::RX1)
  //   {
  //     CancelReceiveWindows ();
  //   }
  // else if (rw == EndDeviceLorawanMac::RXC2)
  //   {
  //     CancelReceiveWindows ();
  //   }
  // else if (rw == EndDeviceLorawanMac::RX2)
  //   {
  //     CancelReceiveWindows ();
  //   }
  CancelReceiveWindows ();
}

void 
ClassCEndDeviceLorawanMac::CancelReceiveWindows (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG ("Cancelling receive windows for " << this);
  Simulator::Cancel (m_closeContinuousWindow);
  Simulator::Cancel (m_firstReceiveWindow);
  Simulator::Cancel (m_continuousReceiveWindow2);
  Simulator::Cancel (m_secondReceiveWindow);
  Simulator::Cancel (m_continuousReceiveWindow3);
}

} /* namespace lorawan */
} /* namespace ns3 */
