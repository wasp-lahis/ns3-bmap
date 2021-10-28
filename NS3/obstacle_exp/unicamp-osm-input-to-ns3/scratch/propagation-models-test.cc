/* This script simulates a complex scenario with multiple GWs, EDs and buildings.
 * - A 3D Obstacle Shadowing Model is used in LoRa Channel

 * The metrics of interest for the network of this script are:
 * - Throughput 
 * - Packet Loss Rate (TO DO)
 * - Packet Error Rate (TO DO)
 * - Packet Delivery Rate 
 * - Average Delay per SF
 * - Total Average delay
 * 
 * Authors: Lahis Almeida e Marianna Campos
 * Based on:
 * - https://github.com/GaiaFL/NS-3_LoraWan
 * - https://gitlab.com/serzagit/QoS-802.11ah-NS3/-/blob/master/scratch/test/NodeEntry.cc
 * - https://github.com/mromanelli9/master-thesis/tree/barichello
 * 
 * RUN:
 * $ cd NS3_BASE_DIR
 * $ ./waf --run "propagation-models-test --cenario=1"
 * ou
 * $ ./waf --run "propagation-models-test --cenario=2"
 * ou
 * $ ./waf --run "propagation-models-test --cenario=3"
 */


/* -----------------------------------------------------------------------------
*			HEADERS
* ------------------------------------------------------------------------------
*/

// LoraWAN Module header files
#include "ns3/log.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/lora-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/forwarder-helper.h" 
#include "ns3/network-server-helper.h"
#include "ns3/periodic-sender-helper.h"
#include <iostream>
#include <fstream>
#include <ns3/spectrum-module.h>
#include <ns3/okumura-hata-propagation-loss-model.h>
#include "ns3/flow-monitor-helper.h"
#include "ns3/correlated-shadowing-propagation-loss-model.h"

// Box obstacles
#include "ns3/building-penetration-loss.h"
#include "ns3/building-allocator.h"
#include "ns3/buildings-helper.h"

// obstacle polygons model
#include "ns3/topology.h"
#include "ns3/obstacle-shadowing-propagation-loss-model.h"

// mobilty
#include "ns3/csv-reader.h"
#include "ns3/mobility-module.h"

// namespaces
using namespace ns3;
using namespace lorawan;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("lorawan-unicamp-3d");

// -------- Setup Variables and Structures --------

struct device{
    int SF;
};

struct spf{
    int S;
    int R;
    Time delay;
};

struct unicamp_trash_bins{
    string osm_id;
    string name;
    string type;
    double x;
    double y;
    double z;
};


// Instantiate of data structures
uint8_t SF_QTD = 6;
vector<device> deviceList;
vector<spf> spreadFList;
map <uint64_t, int> pacote_sf;
map <uint64_t, Time> pacote_ds; // send
map <uint64_t, Time> pacote_dr; // receive
vector<double> distances;
vector<unicamp_trash_bins> unicamp_trash_bins_dataset;

// Network settings
int nDevices = 30; // sera sobrescrito
int nGateways = 1;
double radius = 2000;    // Radio coverage
int payloadSize = 51;   // bytes
int appPeriodSeconds = 2; // seconds
double regionalFrequency = 868e6; // frequency band EU

// Simulation settings
int simulationTime = 60; // sec
int nSimulationRepeat = 1;

// Channel model
int cenario = 0;

// Input dataset file names
string nodes_pos_dataset = "coletores_pos_dataset.csv"; //  Nodes positions dataset
string building_dataset = "predios_unicamp_dataset.xml"; // Unicamp buildings dataset

// Output file names
string network_file = "network_results.txt";

/* -----------------------------------------------------------------------------
*			MAIN
* ------------------------------------------------------------------------------
*/


// -------- Functions --------

// Count Sent Packet per SF
void PacketTraceDevice(Ptr<Packet const> pacote){
    uint32_t id =  Simulator::GetContext ();
    pacote_sf.insert({pacote->GetUid(), deviceList[id].SF});
    Time sendTime = Simulator::Now ();
    pacote_ds.insert({pacote->GetUid(), sendTime});
    spreadFList[deviceList[id].SF].S++;   
}

// Count Received Packet per SF
void PacketTraceGW(Ptr<Packet const> pacote){
    u_int64_t pkid = pacote->GetUid();
    int sf = pacote_sf.at(pkid);
    Time receivedTime = Simulator :: Now ();
    Time sent = pacote_ds.at(pkid);
    pacote_dr.insert({pkid, receivedTime - sent});
    spreadFList[sf].R++;
}

// Print and write in an output file the position of devices, distance from gateway and positions (x,y) per SF
void Print(NodeContainer endDevices, NodeContainer gateways,  Ptr<PropagationDelayModel> delay, double interval){    
    string logFile = network_file;
    
    // open log file for output
    string sf;
    ofstream os;
    os.open (logFile.c_str ());
        
    for(NodeContainer::Iterator gw = gateways.Begin (); gw != gateways.End (); ++gw){
        uint32_t gwId = (*gw)->GetId(); 
        Ptr<MobilityModel> mobModelG = (*gw)->GetObject<MobilityModel>();
        Vector3D posgw = mobModelG->GetPosition();
        
        for (NodeContainer::Iterator node = endDevices.Begin (); node != endDevices.End (); ++node)
        {
          Ptr<MobilityModel> mobModel = (*node)->GetObject<MobilityModel>();
          Vector3D pos = mobModel->GetPosition();
          double position = mobModel->GetDistanceFrom(mobModelG);  
          uint32_t nodeId = (*node)->GetId();
        
          // Prints position and velocities
          if(deviceList[nodeId].SF == 0){
            sf = "12";
          }else if(deviceList[nodeId].SF == 1){
            sf = "11";
          }else if(deviceList[nodeId].SF == 2){
            sf = "10";
          }else if(deviceList[nodeId].SF == 3){
            sf = "9";
          }else if(deviceList[nodeId].SF == 4){
            sf = "8";
          }else{
            sf = "7";
          }

          // id ED, x,y,z ED, SF, id GW, x,y,z GW, distance
          os << nodeId << "," << pos.x << "," << pos.y << "," << pos.z << "," << sf << "," ;
          os << gwId << "," << posgw.x << "," << posgw.y << "," << posgw.z << ",";
          os << position << "\n" ; 
        }
     }

    os.close();
    Simulator::Schedule(Seconds(interval), &Print, endDevices, gateways, delay, interval);
}


// https://www.nsnam.org/doxygen/classns3_1_1_csv_reader.html
void read_nodes_dataset(const std::string &filepath){

  CsvReader csv (filepath);

  while (csv.FetchNextRow ()) {
      // Ignore blank lines
      if (csv.IsBlankRow ()){
          cout << "Blank Line!" << endl;
          continue;
      }

      // colunms: osm_id, name,	type,	x, y e z
      string osm_id, name,	type;
      double x, y, z;
      
      bool ok = csv.GetValue (0, osm_id);
      ok |= csv.GetValue (1, name);
      ok |= csv.GetValue (2, type);
      ok |= csv.GetValue (3, x);
      ok |= csv.GetValue (4, y);
      ok |= csv.GetValue (5, z);
      
      if (!ok) {
        // Handle error, then
        cout << "Read Line Error!" << endl;
        continue;
      }
      else {
        unicamp_trash_bins_dataset.push_back({
          osm_id,  
          name,  
          type,  
          x,  
          y,  
          z   
        });
      }
    }  // while FetchNextRow
    
    // delete first row
    unicamp_trash_bins_dataset.erase(unicamp_trash_bins_dataset.begin());
    
    // Show info
    cout << "Total Rows: "<< unicamp_trash_bins_dataset.size() << endl;
    for ( vector<unicamp_trash_bins>::iterator i = unicamp_trash_bins_dataset.begin(); i!= unicamp_trash_bins_dataset.end(); ++i){
          cout << i->osm_id << ", " << i->name << ", " << i->x << ", " << i->y << ", " << i->z << std::endl;
    }
    
}

// Simulation Code
void runSimulation(){

  // Inicialization
  deviceList.resize(nDevices);
  spreadFList.resize(SF_QTD);
  distances.resize(SF_QTD);

  // Channel Propagation Models Cenarios
  Ptr<LogDistancePropagationLossModel> logDistLoss;  
  Ptr<OkumuraHataPropagationLossModel> okumuraloss;
  Ptr<PropagationLossModel> final_loss;

  // PropagationLossModel final_loss;
  if (cenario == 1){
    // Create the lora channel object
    logDistLoss = CreateObject<LogDistancePropagationLossModel> ();
    logDistLoss->SetPathLossExponent (3.76);
    logDistLoss->SetReference (1, 7.7);

    final_loss = logDistLoss;
  }

  if (cenario == 2){
    
    // Loading Building Dataset
    NS_LOG_INFO ("Loading buildings file \"" << building_dataset << "\".");
    Topology::LoadBuildings (building_dataset);
    
    logDistLoss = CreateObject<LogDistancePropagationLossModel> ();
    logDistLoss->SetPathLossExponent (3.76);
    logDistLoss->SetReference (1, 7.7);

    cout << "[INFO] Obstacle Mode is used!" <<endl;
    Ptr<ObstacleShadowingPropagationLossModel> obstacle3DLoss = CreateObject<ObstacleShadowingPropagationLossModel>();
    obstacle3DLoss->SetAttribute("Radius", DoubleValue (1000.0));
    logDistLoss->SetNext (obstacle3DLoss);

    final_loss = logDistLoss;
  }

  if (cenario == 3){
    // Ptr<NakagamiPropagationLossModel> nakagami = CreateObject<NakagamiPropagationLossModel>();
    // nakagami->SetAttribute("m0", DoubleValue(1));
    // nakagami->SetAttribute("m1",DoubleValue(1));
    // nakagami->SetAttribute("m2",DoubleValue(1));
    // final_loss = nakagami;
    
    // okumuraloss->SetNext(nakagami);
    Ptr<OkumuraHataPropagationLossModel> okumuraLoss = CreateObject<OkumuraHataPropagationLossModel>();
    okumuraLoss->SetAttribute("Frequency", DoubleValue(regionalFrequency));
    okumuraLoss->SetAttribute("Environment", EnumValue (SubUrbanEnvironment));
    okumuraLoss->SetAttribute("CitySize", EnumValue (SmallCity));
    final_loss = okumuraLoss;
  }

  //Create channel
  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();
  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (final_loss, delay);

  //Helpers
  // Create the LoraPhyHelper
  LoraPhyHelper phyHelper = LoraPhyHelper ();
  phyHelper.SetChannel (channel);
  // Create the LorawanMacHelper
  LorawanMacHelper macHelper = LorawanMacHelper ();
  // Create the LoraHelper
  LoraHelper helper = LoraHelper ();
  helper.EnablePacketTracking ();


  MobilityHelper mobility;
  read_nodes_dataset(nodes_pos_dataset);

  // positioning nodes (trash bins)
  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();

  for ( vector<unicamp_trash_bins>::iterator i = unicamp_trash_bins_dataset.begin(); i!= unicamp_trash_bins_dataset.end(); ++i){
    allocator->Add (Vector (i->x, i->y, i->z));
  }

  mobility.SetPositionAllocator(allocator);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); // ED does not move
  
 
  NodeContainer endDevices;
  nDevices = unicamp_trash_bins_dataset.size();
  endDevices.Create (nDevices);
  mobility.Install (endDevices);
       
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LorawanMacHelper::ED_A);
  helper.Install (phyHelper, macHelper, endDevices);
  
  //Gateway
  NodeContainer gateways;
  gateways.Create (nGateways);
  Ptr<ListPositionAllocator> positionAllocGw = CreateObject<ListPositionAllocator> ();
  positionAllocGw->Add (Vector (90, 50, 4));
  mobility.SetPositionAllocator (positionAllocGw);
  mobility.Install(gateways);

  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LorawanMacHelper::GW);
  macHelper.SetRegion (LorawanMacHelper::EU);
  helper.Install (phyHelper, macHelper, gateways);

      
  // spreadFListingFactor
  vector<int> sf = macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);
  cout << "\n- Qtd de EDs por SF: \n [ SF7 SF8 SF9 SF10 SF11 SF12 ] = [ ";
  for ( vector<int>::const_iterator i = sf.begin(); i != sf.end(); ++i)
    cout << *i << ' ';
  cout << "] \n";

  // Connect trace sources
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
  {
      Ptr<Node> node = *j;
      uint32_t id =  node->GetId();
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();
      uint8_t DR =  mac->GetDataRate();
      if (unsigned (DR) == 5){
          deviceList[id].SF = 5;
      }else if (unsigned(DR) == 4){
          deviceList[id].SF = 4;
      }else if (unsigned(DR) == 3){
          deviceList[id].SF = 3;
      }else if (unsigned(DR) == 2){
          deviceList[id].SF = 2;
      }else if (unsigned(DR) == 1){
          deviceList[id].SF = 1;
      }else{
          deviceList[id].SF = 0;
      }
              
      mac->TraceConnectWithoutContext("SentNewPacket", MakeCallback(&PacketTraceDevice));
  }

  for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); ++j)
  {
      Ptr<Node> node = *j;
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<LorawanMac> mac = loraNetDevice->GetMac()->GetObject<LorawanMac>();
      mac->TraceConnectWithoutContext("ReceivedPacket", MakeCallback(&PacketTraceGW));
  }

  //NetworkServer
  NodeContainer networkServers;
  networkServers.Create (1);
  NetworkServerHelper networkServerHelper;
  networkServerHelper.SetGateways (gateways);
  networkServerHelper.SetEndDevices (endDevices);
  networkServerHelper.Install (networkServers);
     
  // Install the Forwarder application on the gateways
  ForwarderHelper forwarderHelper;
  forwarderHelper.Install (gateways);

  // Time appStopTime = Hours(24);
  Time appStopTime = Seconds (simulationTime);
  PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
  // appHelper.SetPeriod (Hours (appPeriodSeconds));
  appHelper.SetPeriod (Seconds (appPeriodSeconds));//
  appHelper.SetPacketSize (payloadSize);

  ApplicationContainer appContainer = appHelper.Install (endDevices);

  // Start simulation
  Simulator::Stop (appStopTime);
  Simulator::Schedule(Seconds(0.00), &Print, endDevices, gateways, delay, 10.0);

  Simulator::Run ();
  Simulator::Destroy ();

  // Pacotes enviados e recebidos
  LoraPacketTracker &tracker = helper.GetPacketTracker ();
  // helper.DoPrintDeviceStatus(endDevices, gateways, "resultados.txt");

  cout << "- Evaluate the global performance at MAC level of the whole network: \n";
  string s =  tracker.CountMacPacketsGlobally (Seconds (0), appStopTime);
  stringstream ss(s);
  string item;
  vector< string> splittedStrings;
  while ( getline(ss, item, ' '))
  {
    splittedStrings.push_back(item);
  }
  double sent =  stod(splittedStrings[0]);
  double receiv =  stod(splittedStrings[1]);
  double PER = ( sent - receiv )/receiv;
  cout <<  "Nº of Pkts Sent And Received: ";
  cout << sent << ' ' << receiv << "\n";

  cout << "Packet error rate: ";
  cout << PER << "\n";

  
  // Somatorio de delays por per SF (?)
  for(auto i = pacote_dr.begin(); i != pacote_dr.end(); i++){
    int SF = pacote_sf[i->first];
    spreadFList[SF].delay += i->second;
  }

  cout << "\n- Nº of Pkts sent, received, Average delay per SF & Delay per SF:\n";
  for ( vector<spf>::iterator i = spreadFList.begin(); i!= spreadFList.end(); ++i){
    if(i->delay != Time(0)){
        cout << i->S << " " << i->R <<  " " << (i->delay/i->R).GetMilliSeconds()  << " " << i->delay/i->R  << " " << i->delay << "\n";
      i->S = i->R  = 0;
      i->delay = Time(0);
    }
    else{  
        cout << i->S << " " << i->R << "\n";
      i->S = i->R = 0;
      i->delay = Time(0);
    }  
  }


  // Get RSSI for each node to GW
  // see LoraChannel::Send function in lora-channel.cc to understanding:
  // LogComponentEnable("LoraChannel", LOG_LEVEL_INFO) and GetRxPower() 
  for(NodeContainer::Iterator gw = gateways.Begin (); gw != gateways.End (); ++gw){
      uint32_t gwId = (*gw)->GetId(); 
      Ptr<MobilityModel> mobModelG = (*gw)->GetObject<MobilityModel>();
      // Vector3D posgw = mobModelG->GetPosition();
      
      for (NodeContainer::Iterator node = endDevices.Begin (); node != endDevices.End (); ++node)
      {
        Ptr<MobilityModel> mobModel = (*node)->GetObject<MobilityModel>();
        // Vector3D pos = mobModel->GetPosition();
        double position = mobModel->GetDistanceFrom(mobModelG);  
        uint32_t nodeId = (*node)->GetId();
      
        std:: cout << "RX power for GW " << gwId << " receive from node "<< nodeId << ": ";
        std:: cout << channel->GetRxPower(14.0,mobModel,mobModelG) << " - distance from GW "
        << position << std::endl ;
      }
  }

  // Cleaning
  pacote_sf.clear();
  pacote_ds.clear();
  pacote_dr.clear();
  deviceList.clear();
  spreadFList.clear();
  distances.clear();

}


int main (int argc, char *argv[])
{

      CommandLine cmd;
      cmd.AddValue ("cenario", "Number of end devices to include in the simulation", cenario);
      cmd.Parse (argc, argv);
     
      // Set up logging
      LogComponentEnable ("lorawan-unicamp-3d", LOG_LEVEL_ALL);

      cout << "[OBSTACLE] cenario: " << cenario << endl;          

      for(int n = 0; n < nSimulationRepeat; n++){
      
        // generate a different seed for each simulation 
        // srand(time(0));
        // int seed = rand();
        // RngSeedManager::SetSeed (seed); // Changes seed from default of 1 to seed
        // RngSeedManager::SetRun (7); // Changes run number from default of 1 to 7
        runSimulation();
      }

      return 0;
}
