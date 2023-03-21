/*
 * This script simulates a complex scenario with multiple gateways and end
 * devices. The metric of interest for this script is the throughput of the
 * network.
 */

#include "ns3/end-device-lora-phy.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/class-a-end-device-lorawan-mac.h"
#include "ns3/class-c-end-device-lorawan-mac.h"
#include "ns3/gateway-lorawan-mac.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/lora-helper.h"
#include "ns3/node-container.h"
#include "ns3/mobility-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/command-line.h"
#include "ns3/network-server-helper.h"
#include "ns3/correlated-shadowing-propagation-loss-model.h"
#include "ns3/building-penetration-loss.h"
#include "ns3/building-allocator.h"
#include "ns3/buildings-helper.h"
#include "ns3/forwarder-helper.h"
#include <algorithm>
#include <ctime>
#include "ns3/basic-energy-source-helper.h"
#include "ns3/lora-radio-energy-model-helper.h"

#include "ns3/udp-client-server-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include <iostream>
#include <string>
#include <map>
#include <sstream>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE ("ComplexLorawanNetworkExample");

// Network settings
int nDevices = 64;
int nGateways = 1;
double radius = 1000;
double simulationTime = 86400;

// Channel model
bool realisticChannelModel = true;

int appPeriodSeconds = 50;

// Output control
bool print = true;

int packetSize = 8;
bool isCEndDevice = false;
std::string filename = "lorawan_input";
int seed = 1;
std::string positionFile = "";
std::string transmitInputFile = "";
int interfererDevices = 50;
int interfererPacketSize = 64;

struct Device {
    double x, y;
};


int
main (int argc, char *argv[])
{

  CommandLine cmd;
  cmd.AddValue ("nDevices", "Number of end devices to include in the simulation", nDevices);
  cmd.AddValue ("nGateways", "Number of gateways to include in the simulation", nGateways);
  cmd.AddValue ("radius", "The radius of the area to simulate", radius);
  cmd.AddValue ("simulationTime", "The time for which to simulate", simulationTime);
  cmd.AddValue ("realisticChannelModel", "Run using realistic channel model", realisticChannelModel);
  cmd.AddValue ("packetSize", "Size of the packet to use", packetSize);
  cmd.AddValue ("appPeriod",
                "The period in seconds to be used by periodically transmitting applications",
                appPeriodSeconds);
  cmd.AddValue ("print", "Whether or not to print various informations", print);
  cmd.AddValue ("isCEndDevice", "Use Class C end device", isCEndDevice);
  cmd.AddValue ("filename", "File to store input for lorawan", filename);
  cmd.AddValue ("positionFile", "positionFile", positionFile);
  cmd.AddValue ("transmitInputFile", "transmitInputFile", transmitInputFile);
  cmd.AddValue ("interfererDevices", "interfererDevices", interfererDevices);
  cmd.AddValue ("interfererPacketSize", "interfererPacketSize", interfererPacketSize);
  cmd.Parse (argc, argv);

  // Set up logging
  // LogComponentEnable ("LoraNetDevice", LOG_LEVEL_ALL);
  // LogComponentEnable ("ComplexLorawanNetworkExample", LOG_LEVEL_ALL);
  // LogComponentEnable ("CorrelatedShadowingPropagationLossModel", LOG_LEVEL_ALL);
  // LogComponentEnable ("BuildingPenetrationLoss", LOG_LEVEL_ALL);

  LogComponentEnable ("LoraPacketTracker", LOG_LEVEL_ALL);
  // LogComponentEnable("EndDeviceStatus", LOG_LEVEL_ALL);

  // LogComponentEnable("LoraChannel", LOG_LEVEL_INFO);
  // LogComponentEnable("LoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable("EndDeviceLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable("GatewayLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable("SimpleGatewayLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable("SimpleEndDeviceLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraInterferenceHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("LorawanMac", LOG_LEVEL_ALL);
  // LogComponentEnable("EndDeviceLorawanMac", LOG_LEVEL_ALL);
  // LogComponentEnable("ClassAEndDeviceLorawanMac", LOG_LEVEL_ALL);
  // LogComponentEnable("GatewayLorawanMac", LOG_LEVEL_ALL);
  // LogComponentEnable("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("LogicalLoraChannel", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraPhyHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("LorawanMacHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("PeriodicSenderHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("PeriodicSender", LOG_LEVEL_ALL);  
  // LogComponentEnable("LorawanMacHeader", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraFrameHeader", LOG_LEVEL_ALL);
  // LogComponentEnable("NetworkScheduler", LOG_LEVEL_ALL);
  // LogComponentEnable("NetworkServer", LOG_LEVEL_ALL);
  // LogComponentEnable("NetworkStatus", LOG_LEVEL_ALL);
  // LogComponentEnable("NetworkController", LOG_LEVEL_ALL);

  /***********
   *  Setup  *
   ***********/

  std::ifstream inputFile(transmitInputFile);
	std::map<std::string, std::vector<double>> transmissionTimes;
	int device;
  std::string line;
	while (std::getline(inputFile, line)) {
		std::istringstream iss(line);
  		std::vector<std::string> tokens;
		std::string token;
		while (std::getline(iss, token, ' ')) {
			tokens.push_back(token);
		}

    std::string str = tokens[2];
    size_t pos = str.find("s");

    std::string num_str = str.substr(0, pos);
    double time = std::stod(num_str);

    pos = tokens[3].find("=");
    std::string ip = tokens[3].substr(pos+1);
    if(transmissionTimes.find(ip) == transmissionTimes.end()) {
      transmissionTimes[ip] = std::vector<double>();
    }
    transmissionTimes[ip].push_back(time);
  	// iss >> time >> std::ignore >> std::ignore >> std::ignore >> device;
		// if(tokens[tokens.size() -2].compare("gateway") != 0)
		// 	transmissionTimes[stoi(tokens.back())].push_back(stod(tokens.front()));
	}
	std::map<int, std::vector<double>> transmissionTimesInterferer;
  int i = 0;
	for (const auto& [device, times] : transmissionTimes) {
    transmissionTimesInterferer[i] = times;
    i++;
  }

	inputFile.close();

	std::vector<Device> devices;
    std::ifstream infile(positionFile);
    while (getline(infile, line)) {
        int pos = line.find_first_of(",");
        int end = line.find_first_of(":", pos);

        int device_num = stoi(line.substr(7, pos));
		// cout << device_num;
        double x_pos = stod(line.substr(pos+13, end-pos-13));

        pos = end;
        end = line.find_first_of(":", pos+1);

        double y_pos = stod(line.substr(pos+1, end-pos-1));

        Device d;
        d.x = x_pos;
        d.y = y_pos;
        devices.push_back(d);
    }
	infile.close();

  // Create the time value from the period
  Time appPeriod = Seconds (appPeriodSeconds);

  // Mobility
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", "rho", DoubleValue (radius),
                                 "X", DoubleValue (0.0), "Y", DoubleValue (0.0));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  /************************
   *  Create the channel  *
   ************************/

  // Create the lora channel object
  Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
  loss->SetPathLossExponent (3.76);
  loss->SetReference (1, 7.7);

  if (realisticChannelModel)
    {
      // Create the correlated shadowing component
      Ptr<CorrelatedShadowingPropagationLossModel> shadowing =
          CreateObject<CorrelatedShadowingPropagationLossModel> ();

      // Aggregate shadowing to the logdistance loss
      loss->SetNext (shadowing);
      // Add the effect to the channel propagation loss
      Ptr<BuildingPenetrationLoss> buildingLoss = CreateObject<BuildingPenetrationLoss> ();

      shadowing->SetNext (buildingLoss);
    }

  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();

  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (loss, delay);

  /************************
   *  Create the helpers  *
   ************************/

  // Create the LoraPhyHelper
  LoraPhyHelper phyHelper = LoraPhyHelper ();
  phyHelper.SetChannel (channel);

  // Create the LorawanMacHelper
  LorawanMacHelper macHelper = LorawanMacHelper ();
  // Create the LoraHelper
  LoraHelper helper = LoraHelper ();
  helper.EnablePacketTracking (); // Output filename
  // helper.EnableSimulationTimePrinting ();

  //Create the NetworkServerHelper
  NetworkServerHelper nsHelper = NetworkServerHelper ();

  //Create the ForwarderHelper
  ForwarderHelper forHelper = ForwarderHelper ();

  /************************
   *  Create End Devices  *
   ************************/

  // Create a set of nodes
  NodeContainer endDevices;
  endDevices.Create (nDevices);

  // Assign a mobility model to each node
  mobility.Install (endDevices);

  // Make it so that nodes are at a certain height > 0
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
    {
      Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
      Vector position = mobility->GetPosition ();
      position.z = 1.2;
      mobility->SetPosition (position);
    }

  // Create the LoraNetDevices of the end devices
  uint8_t nwkId = 54;
  uint32_t nwkAddr = 1864;
  Ptr<LoraDeviceAddressGenerator> addrGen =
      CreateObject<LoraDeviceAddressGenerator> (nwkId, nwkAddr);

  // Create the LoraNetDevices of the end devices
  macHelper.SetAddressGenerator (addrGen);
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  if(isCEndDevice) {
    NS_LOG_INFO ("Using Class C end device"); 
    macHelper.SetDeviceType (LorawanMacHelper::ED_C);
  }
  else {
    NS_LOG_INFO ("Using Class A end device"); 
    macHelper.SetDeviceType (LorawanMacHelper::ED_A);
  }

  NetDeviceContainer endDevicesNetDevices = helper.Install (phyHelper, macHelper, endDevices);

  // Now end devices are connected to the channel

  // Connect trace sources
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
    {
      Ptr<Node> node = *j;
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<LoraPhy> phy = loraNetDevice->GetPhy ();
    }

  /*********************
   *  Create Gateways  *
   *********************/

  // Create the gateway nodes (allocate them uniformely on the disc)
  NodeContainer gateways;
  gateways.Create (nGateways);

  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  // Make it so that nodes are at a certain height > 0
  allocator->Add (Vector (0.0, 0.0, 15.0));
  mobility.SetPositionAllocator (allocator);
  mobility.Install (gateways);

  // Create a netdevice for each gateway
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LorawanMacHelper::GW);
  helper.Install (phyHelper, macHelper, gateways);

  /**********************
   *  Handle buildings  *
   **********************/

  double xLength = 130;
  double deltaX = 32;
  double yLength = 64;
  double deltaY = 17;
  int gridWidth = 2 * radius / (xLength + deltaX);
  int gridHeight = 2 * radius / (yLength + deltaY);
  if (realisticChannelModel == false)
    {
      gridWidth = 0;
      gridHeight = 0;
    }
  Ptr<GridBuildingAllocator> gridBuildingAllocator;
  gridBuildingAllocator = CreateObject<GridBuildingAllocator> ();
  gridBuildingAllocator->SetAttribute ("GridWidth", UintegerValue (gridWidth));
  gridBuildingAllocator->SetAttribute ("LengthX", DoubleValue (xLength));
  gridBuildingAllocator->SetAttribute ("LengthY", DoubleValue (yLength));
  gridBuildingAllocator->SetAttribute ("DeltaX", DoubleValue (deltaX));
  gridBuildingAllocator->SetAttribute ("DeltaY", DoubleValue (deltaY));
  gridBuildingAllocator->SetAttribute ("Height", DoubleValue (6));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsX", UintegerValue (2));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsY", UintegerValue (4));
  gridBuildingAllocator->SetBuildingAttribute ("NFloors", UintegerValue (2));
  gridBuildingAllocator->SetAttribute (
      "MinX", DoubleValue (-gridWidth * (xLength + deltaX) / 2 + deltaX / 2));
  gridBuildingAllocator->SetAttribute (
      "MinY", DoubleValue (-gridHeight * (yLength + deltaY) / 2 + deltaY / 2));
  BuildingContainer bContainer = gridBuildingAllocator->Create (gridWidth * gridHeight);

  BuildingsHelper::Install (endDevices);
  BuildingsHelper::Install (gateways);

  // Print the buildings
  if (print)
    {
      std::ofstream myfile;
      myfile.open ("buildings.txt");
      std::vector<Ptr<Building>>::const_iterator it;
      int j = 1;
      for (it = bContainer.Begin (); it != bContainer.End (); ++it, ++j)
        {
          Box boundaries = (*it)->GetBoundaries ();
          myfile << "set object " << j << " rect from " << boundaries.xMin << "," << boundaries.yMin
                 << " to " << boundaries.xMax << "," << boundaries.yMax << std::endl;
        }
      myfile.close ();
    }

  /**********************************************
   *  Set up the end device's spreading factor  *
   **********************************************/

  macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);

  NS_LOG_DEBUG ("Completed configuration");

  /*********************************************
   *  Install applications on the end devices  *
   *********************************************/

  Time appStopTime = Seconds (simulationTime);
  PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
  appHelper.SetPeriod (Seconds (appPeriodSeconds));
  appHelper.SetPacketSize(packetSize);
  appHelper.SetPacketSizeRandomVariable(0);
  ApplicationContainer appContainer = appHelper.Install(endDevices);

  appContainer.Start (Seconds (0));
  appContainer.Stop (appStopTime);

    /************************
   * Install Energy Model *
   ************************/

  BasicEnergySourceHelper basicSourceHelper;
  LoraRadioEnergyModelHelper radioEnergyHelper;

  // configure energy source
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (10000)); // Energy in J
  basicSourceHelper.Set ("BasicEnergySupplyVoltageV", DoubleValue (3.3));

  radioEnergyHelper.Set ("StandbyCurrentA", DoubleValue (0.0014));
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.028));
  radioEnergyHelper.Set ("SleepCurrentA", DoubleValue (0.0000015));
  radioEnergyHelper.Set ("RxCurrentA", DoubleValue (0.0112));

  radioEnergyHelper.SetTxCurrentModel ("ns3::ConstantLoraTxCurrentModel",
                                       "TxCurrent", DoubleValue (0.028));

  // install source on EDs' nodes
  EnergySourceContainer sources = basicSourceHelper.Install (endDevices);
  Names::Add ("/Names/EnergySource", sources.Get (0));

  // install device model
  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install
      (endDevicesNetDevices, sources);

    

  /**************************
   *  Create Network Server  *
   ***************************/

  // Create the NS node
  NodeContainer networkServer;
  networkServer.Create (1);

  // Create a NS for the network
  nsHelper.SetEndDevices (endDevices);
  nsHelper.SetGateways (gateways);
  nsHelper.Install (networkServer);

  //Create a forwarder for each gateway
  forHelper.Install (gateways);

  /**********************************************
   *  Interferer setup  *
   **********************************************/

  LoraHelper helperI = LoraHelper ();
  helperI.EnablePacketTracking (); // Output filename

  LorawanMacHelper macHelperI = LorawanMacHelper ();

  Ptr<LoraChannel> channelI = CreateObject<LoraChannel> (loss, delay);

  LoraPhyHelper phyHelperI = LoraPhyHelper ();
  phyHelperI.SetChannel (channel);
  phyHelperI.SetDeviceType (LoraPhyHelper::ED);

  NodeContainer interfererNodes;
  interfererNodes.Create(transmissionTimes.size());
  double xpos = 0.0;
  double ypos = 0.0;
	MobilityHelper mobilityInterferer;
	Ptr<ListPositionAllocator> positionAllocInterferer = CreateObject<ListPositionAllocator>();
	for (int i = 0; i < transmissionTimes.size(); i++) {
    Vector pos(devices[i].x, devices[i].y, 0);
		positionAllocInterferer->Add(pos);
  }
	mobilityInterferer.SetPositionAllocator(positionAllocInterferer);
	mobilityInterferer.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobilityInterferer.Install(interfererNodes);

  nwkId = 1;
  nwkAddr = 130;
  addrGen =
      CreateObject<LoraDeviceAddressGenerator> (nwkId, nwkAddr);
  macHelperI.SetAddressGenerator (addrGen);
  phyHelperI.SetDeviceType (LoraPhyHelper::ED);
  macHelperI.SetDeviceType (LorawanMacHelper::ED_A);
  NetDeviceContainer interfererDevice = helperI.InstallI (phyHelperI, macHelperI, interfererNodes);

  for (NodeContainer::Iterator j = interfererNodes.Begin (); j != interfererNodes.End (); ++j)
    {
      Ptr<Node> node = *j;
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<LoraPhy> phy = loraNetDevice->GetPhy ();
    }

  NodeContainer interfererServer;
  interfererServer.Create(1);
  Ptr<ListPositionAllocator> allocatorI = CreateObject<ListPositionAllocator> ();
  // Make it so that nodes are at a certain height > 0
  allocatorI->Add (Vector (10.0, 10.0, 0.0));
  mobility.SetPositionAllocator (allocatorI);
  mobility.Install (interfererServer);
  phyHelperI.SetDeviceType (LoraPhyHelper::GW);
  macHelperI.SetDeviceType (LorawanMacHelper::GW);
  helperI.InstallI (phyHelperI, macHelperI, interfererServer);

  macHelperI.SetSpreadingFactorsUp (interfererNodes, interfererServer, channel);

  Time appStopTimeI = Seconds (simulationTime);
  PeriodicSenderHelper appHelperI = PeriodicSenderHelper ();
  appHelperI.SetPacketSize(interfererPacketSize);
  appHelperI.SetPacketSizeRandomVariable(0);
  // appHelperI.SetPeriod (MilliSeconds (20));
  // ApplicationContainer appContainerI = appHelperI.InstallI(interfererNodes);
  // appContainerI.Start (Seconds (0));
	for (const auto& [device, times] : transmissionTimesInterferer) {
		if(times.size() > 1) {
      double sum = 0.0;
      for (int i = 0; i < times.size()-1; i++) {
          sum += times[i+1] - times[i];
      }
      double average = sum / (times.size()-1);
      // appHelperI.SetPeriod (MilliSeconds (20));
      appHelperI.SetPeriod (MilliSeconds (average*1000));
		}
		else {
      appHelperI.SetPeriod (MilliSeconds (1000));
		}
		// client.SetAttribute("Interval", TimeValue(MilliSeconds(20)));
		std::cout << device << " " << interfererNodes.GetN() << "\n";
    ApplicationContainer appContainerI = appHelperI.InstallI(interfererNodes.Get(device));
    appContainerI.Start (Seconds (times[0]));
	}
  ////////////////
  // Simulation //
  ////////////////

  Simulator::Stop (appStopTime);

  NS_LOG_INFO ("Running simulation...");
  Simulator::Run ();
  int counter = 0;
  // for (DeviceEnergyModelContainer::Iterator iter = deviceModels.Begin (); iter != deviceModels.End (); iter ++)
  // {
  //   counter += 1;
  //   double energyConsumed = (*iter)->GetTotalEnergyConsumption ();
  //   NS_LOG_UNCOND ("End of simulation (" << Simulator::Now ().GetSeconds ()
  //                   << "s) Total energy consumed by node " << counter << " = " << energyConsumed << "J");
  // }


  Simulator::Destroy ();

  ///////////////////////////
  // Print results to file //
  ///////////////////////////
  NS_LOG_INFO ("Computing performance metrics...");

  LoraPacketTracker &tracker = helper.GetPacketTracker ();
  std::cout << tracker.CountMacPacketsGlobally (Seconds (0), appStopTime + Hours (1)) << std::endl;
  
  LoraPacketTracker &trackerI = helperI.GetPacketTracker ();
  std::cout << trackerI.CountMacPacketsGlobally (Seconds (0), appStopTime + Hours (1)) << std::endl;


	// if (true) {

  //    std::ofstream file(filename);
	// 	uint32_t i = 0;
	// 	while (i < nDevices) {
	// 		Ptr<MobilityModel> mobility = endDevices.Get(i)->GetObject<
	// 				MobilityModel>();
	// 		Vector position = mobility->GetPosition();
	// 		file << "Device " << i << ", " << "position = " << position
	// 				<< std::endl;
	// 		i++;
	// 	}

  //     file.close();
	// }


  return 0;
}

// eliminate the checking before transmission
// 1 gateway independent of the other one