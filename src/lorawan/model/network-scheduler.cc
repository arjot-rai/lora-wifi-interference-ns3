#include "network-scheduler.h"

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("NetworkScheduler");

NS_OBJECT_ENSURE_REGISTERED (NetworkScheduler);

TypeId
NetworkScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetworkScheduler")
    .SetParent<Object> ()
    .AddConstructor<NetworkScheduler> ()
    .AddTraceSource ("ReceiveWindowOpened",
                     "Trace source that is fired when a receive window "
                     "opportunity happens.",
                     MakeTraceSourceAccessor (&NetworkScheduler::m_receiveWindowOpened),
                     "ns3::Packet::TracedCallback")
    .SetGroupName ("lorawan");
  return tid;
}

NetworkScheduler::NetworkScheduler ()
{
  m_recieveWindowOpportunity = EventId ();
  m_recieveWindowOpportunity.Cancel ();
}

NetworkScheduler::NetworkScheduler (Ptr<NetworkStatus> status,
                                    Ptr<NetworkController> controller) :
  m_status (status),
  m_controller (controller)
{
}

NetworkScheduler::~NetworkScheduler ()
{
}

void
NetworkScheduler::OnReceivedPacket (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (packet);

  // Get the current packet's frame counter
  Ptr<Packet> packetCopy = packet->Copy ();
  LorawanMacHeader receivedMacHdr;
  packetCopy->RemoveHeader (receivedMacHdr);
  LoraFrameHeader receivedFrameHdr;
  receivedFrameHdr.SetAsUplink ();
  packetCopy->RemoveHeader (receivedFrameHdr);
  uint8_t currentFrameCounter = receivedFrameHdr.GetFCnt ();

  // Get the saved packet's frame counter
  Ptr<const Packet> savedPacket = m_status->GetEndDeviceStatus
      (packet)->GetLastReceivedPacketInfo ().packet;
  if (savedPacket)
    {
      Ptr<Packet> savedPacketCopy = savedPacket->Copy ();
      LorawanMacHeader savedMacHdr;
      savedPacketCopy->RemoveHeader (savedMacHdr);
      LoraFrameHeader savedFrameHdr;
      savedFrameHdr.SetAsUplink ();
      savedPacketCopy->RemoveHeader (savedFrameHdr);
      uint8_t savedFrameCounter = savedFrameHdr.GetFCnt ();

      if (currentFrameCounter == savedFrameCounter)
        {
          NS_LOG_DEBUG ("Packet was already received by another gateway.");
          return;
        }
    }

  // It's possible that we already received the same packet from another
  // gateway.
  // - Extract the address
  LoraDeviceAddress deviceAddress = receivedFrameHdr.GetAddress ();


  double delayInSeconds = 1.0;
  int window = 1;

  // Get the device's MAC layer
  Ptr<EndDeviceLorawanMac> edLorawanMac = m_status->GetEndDeviceStatus
      (packet)->GetMac ();
  NS_ASSERT (edLorawanMac != 0);

  if (edLorawanMac->GetDeviceClass () == EndDeviceLorawanMac::CLASS_C)
    {     
      window = 2;
    }

  // Schedule OnReceiveWindowOpportunity event
  m_recieveWindowOpportunity =  Simulator::Schedule (Seconds (delayInSeconds),
                    &NetworkScheduler::OnReceiveWindowOpportunity,
                    this,
                    deviceAddress,
                    window, 
                    edLorawanMac);     // This will be the first receive window
}

void
NetworkScheduler::OnReceiveWindowOpportunity (LoraDeviceAddress deviceAddress, int window, Ptr<EndDeviceLorawanMac> edLorawanMac)
{
  NS_LOG_FUNCTION (deviceAddress);

  NS_LOG_DEBUG ("Opening receive window number " << window << " for device "
                                                 << deviceAddress);

  double delayInSeconds = 1.0;
  NS_ASSERT (edLorawanMac != 0);

  // if (edLorawanMac->GetDeviceClass () == EndDeviceLorawanMac::CLASS_C)
  //   {     
  //     delayInSeconds = 0.001;
  //   }

  // Check whether we can send a reply to the device, again by using
  // NetworkStatus
  Address gwAddress = m_status->GetBestGatewayForDevice (deviceAddress, window);

  NS_LOG_DEBUG ("Found available gateway with address: " << gwAddress);

  if (gwAddress == Address () && window == 1)
    {
      if (edLorawanMac->GetDeviceClass () == EndDeviceLorawanMac::CLASS_A)
        {
          NS_LOG_DEBUG ("No suitable gateway found.");

          // No suitable GW was found
          // Schedule OnReceiveWindowOpportunity event
          Simulator::Schedule (Seconds (delayInSeconds),
                              &NetworkScheduler::OnReceiveWindowOpportunity,
                              this,
                              deviceAddress,
                              2,
                              edLorawanMac);     // This will be the second receive window
        }
      else  // Only other option at this time are Class C devices
        {
          // No suitable GW was found
          // Simply give up.
          NS_LOG_INFO ("Giving up on reply: no suitable gateway was found " <<
                      "on the second receive window");

          // Reset the reply
          // XXX Should we reset it here or keep it for the next opportunity?
          m_status->GetEndDeviceStatus (deviceAddress)->InitializeReply ();
        }
    }
  else if (gwAddress == Address () && window == 2)
    {
      if (edLorawanMac->GetDeviceClass () == EndDeviceLorawanMac::CLASS_A)
        {
          // No suitable GW was found
          // Simply give up.
          NS_LOG_INFO ("Giving up on reply: no suitable gateway was found " <<
                      "on the second receive window");

          // Reset the reply
          // XXX Should we reset it here or keep it for the next opportunity?
          m_status->GetEndDeviceStatus (deviceAddress)->InitializeReply ();
        }
      else // Only other option at this time are Class C devices
        {
          NS_LOG_DEBUG ("No suitable gateway found.");

          // No suitable GW was found
          // Schedule OnReceiveWindowOpportunity event
          Simulator::Schedule (Seconds (delayInSeconds),
                                &NetworkScheduler::OnReceiveWindowOpportunity,
                                this,
                                deviceAddress,
                                1,
                                edLorawanMac);     // This will be the first receive window
        }
    }
  else
    {
      // A gateway was found
      m_controller->BeforeSendingReply (m_status->GetEndDeviceStatus
                                          (deviceAddress));

      // Check whether this device needs a response by querying m_status
      bool needsReply = m_status->NeedsReply (deviceAddress);

      if (needsReply)
        {
          NS_LOG_INFO ("A reply is needed");

          // Send the reply through that gateway
          m_status->SendThroughGateway (m_status->GetReplyForDevice
                                          (deviceAddress, window),
                                        gwAddress);

          // Reset the reply
          m_status->GetEndDeviceStatus (deviceAddress)->InitializeReply ();
        }
    }
}

Ptr<NetworkStatus>
NetworkScheduler::GetNetworkStatus (void)
{
  return m_status;
}

}
}
