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
 * RUN example:
 * $ cd NS3_BASE_DIR
 * $ ./waf --run "simulation-coletores-cenario --exp_name=coletores_unicamp --simu_repeat=1"
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

// mobilty
#include "ns3/csv-reader.h"

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
    int index;
    double id;
    string name;
    double x;
    double y;
    double z;
    double elevation;
};

// Instantiate of data structures
uint8_t SF_QTD = 6; // AU 915 MHz and EU 868 MHz

vector<device> deviceList;
vector<spf> spreadFList;
map <uint64_t, int> pacote_sf;
map <uint64_t, Time> pacote_ds; // send
map <uint64_t, Time> pacote_dr; // receive
vector<double> distances;

// dataset structs
vector<unicamp_trash_bins> unicamp_trash_bins_dataset;

// Network settings
int nDevices = 0; // sera sobrescrito
int nGateways = 1;
int payloadSize = 7;   // bytes: id - 2 bytes, level - 4 bytes, batery - 1 byte
Time appPeriod = Hours(24); // h

uint8_t txEndDevice = 20; // Dbm
double regionalFrequency = 915e6; // frequency band AU 915 MHz
// double regionalFrequency = 868e6; // frequency band EU 868 MHz

// Simulation settings
Time simulationTime = Hours(24 * 7); // hour 
int nSimulationRepeat = 0;

// Input dataset file names
string nodes_dataset_input = "coletores_pos_dataset_elev.csv"; //  Nodes positions dataset

// Output file names
string exp_name = ""; // experiment name
string output_results_path = "";
string network_result_file = "";
string per_result_file = "";

int count_send_pkts = 0;
int count_receiv_pkts = 0;

/* -----------------------------------------------------------------------------
*			MAIN
* ------------------------------------------------------------------------------
*/


// -------- Functions --------

// Count Sent Packet per SF
void PacketTraceDevice(Ptr<Packet const> pacote){
    // uint32_t id =  Simulator::GetContext ();
    // pacote_sf.insert({pacote->GetUid(), deviceList[id].SF});
    // Time sendTime = Simulator::Now ();
    // pacote_ds.insert({pacote->GetUid(), sendTime});
    // spreadFList[deviceList[id].SF].S++;   
    count_send_pkts = count_send_pkts +1;
}

// Count Received Packet per SF
void PacketTraceGW(Ptr<Packet const> pacote){
    // u_int64_t pkid = pacote->GetUid();
    // int sf = pacote_sf.at(pkid);
    // Time receivedTime = Simulator :: Now ();
    // Time sent = pacote_ds.at(pkid);
    // pacote_dr.insert({pkid, receivedTime - sent});
    // spreadFList[sf].R++;
    count_receiv_pkts = count_receiv_pkts + 1;
}


// Print and write in an output file the position of devices, distance from gateway and positions (x,y) per SF
void Print(NodeContainer endDevices, NodeContainer gateways,  Ptr<PropagationDelayModel> delay, double interval){    
    string logFile = output_results_path + network_result_file;
    
    // open log file for output
    string sf;
    ofstream os;
    os.open (logFile.c_str (), std::ofstream::out | std::ofstream::app);
        
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
          // if(deviceList[nodeId].SF == 0){
          //   sf = "12";
          // }else if(deviceList[nodeId].SF == 1){
          //   sf = "11";
          // }else if(deviceList[nodeId].SF == 2){
          //   sf = "10";
          // }else if(deviceList[nodeId].SF == 3){
          //   sf = "9";
          // }else if(deviceList[nodeId].SF == 4){
          //   sf = "8";
          // }else{
          //   sf = "7";
          // }

          // id ED, x,y,z ED, SF, id GW, x,y,z GW, distance
          // os << nodeId << "," << pos.x << "," << pos.y << "," << pos.z << "," << sf << "," ;
          // os << gwId << "," << posgw.x << "," << posgw.y << "," << posgw.z << ",";
          // os << position << "\n" ; 

          os << nodeId << "," << pos.x << "," << pos.y << "," << pos.z << "," ;
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

      // colunms: index, id, name, x, y, z e elevation
      int index;
      string name;
      double id, x, y, z, elevation;
      
      bool ok = csv.GetValue (0, index);
      ok |= csv.GetValue (1, id);
      ok |= csv.GetValue (2, name);
      ok |= csv.GetValue (3, x);
      ok |= csv.GetValue (4, y);
      ok |= csv.GetValue (5, z);
      ok |= csv.GetValue (6, elevation);
      
      if (!ok) {
        // Handle error, then
        // cout << "Read Line Error!" << endl;
        continue;
      }
      else {
        unicamp_trash_bins_dataset.push_back({
          index,  
          id,  
          name,  
          x,  
          y,  
          z,
          elevation   
        });
      }
    }  // while FetchNextRow
    
    // delete first row
    unicamp_trash_bins_dataset.erase(unicamp_trash_bins_dataset.begin());
    
    // Show info
    // cout << "Total Rows: "<< unicamp_trash_bins_dataset.size() << endl;
    // for ( vector<unicamp_trash_bins>::iterator i = unicamp_trash_bins_dataset.begin(); i!= unicamp_trash_bins_dataset.end(); ++i){
    //       cout << i->index << ", " << i->id << ", " << i->name << ", " << i->x << ", " << i->y << ", " << i->z << ", " << i->elevation<< std::endl;
    // }
    
}

// Simulation Code
LoraPacketTracker& runSimulation(){

  // Structs Inicialization
  deviceList.resize(nDevices);
  spreadFList.resize(SF_QTD);
  distances.resize(SF_QTD);

  // Channel Propagation Models Cenarios
  Ptr<OkumuraHataPropagationLossModel> okumuraLoss = CreateObject<OkumuraHataPropagationLossModel>();
  okumuraLoss->SetAttribute("Frequency", DoubleValue(regionalFrequency));
  okumuraLoss->SetAttribute("Environment", EnumValue (SubUrbanEnvironment));
  okumuraLoss->SetAttribute("CitySize", EnumValue (SmallCity));
  
  //Create channel
  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();
  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (okumuraLoss, delay);
  
  //Helpers
  // Create the LoraPhyHelper
  LoraPhyHelper phyHelper = LoraPhyHelper ();
  phyHelper.SetChannel (channel);
  // Create the LorawanMacHelper
  LorawanMacHelper macHelper = LorawanMacHelper ();
  // Create the LoraHelper
  LoraHelper helper = LoraHelper ();
  helper.EnablePacketTracking ();

  //load nodes dataset  
  read_nodes_dataset(nodes_dataset_input); 
  NodeContainer endDevices;
  nDevices = unicamp_trash_bins_dataset.size();
  endDevices.Create (nDevices);
  
  // positioning nodes     
  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  for ( vector<unicamp_trash_bins>::iterator i = unicamp_trash_bins_dataset.begin(); i!= unicamp_trash_bins_dataset.end(); ++i){
    // cout <<"[DEBUG]: " << i->x << "," << i->y << "," << (i->z + i->elevation) << endl;
    allocator->Add (Vector (i->x, i->y, (i->z)));
  }

  MobilityHelper mobility; 
  mobility.SetPositionAllocator(allocator);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); // ED does not move 
  mobility.Install (endDevices);

  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LorawanMacHelper::ED_A);
  // macHelper.SetRegion(LorawanMacHelper::EU);
  macHelper.SetRegion(LorawanMacHelper::Australia);
  helper.Install (phyHelper, macHelper, endDevices);
  
  //Gateway
  NodeContainer gateways;
  gateways.Create (nGateways);
  Ptr<ListPositionAllocator> positionAllocGw = CreateObject<ListPositionAllocator> ();
  positionAllocGw->Add (Vector (1694.975, 2141.471, (1.5)));    // z - altura antena + elevacao museu
  mobility.SetPositionAllocator (positionAllocGw);
  mobility.Install(gateways);

  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LorawanMacHelper::GW);
  // macHelper.SetRegion (LorawanMacHelper::EU);
  macHelper.SetRegion (LorawanMacHelper::Australia);
  helper.Install (phyHelper, macHelper, gateways);

      
  // Set SF automatically based on position and RX power
  // vector<int> sf = macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);

  // Set SF manually based on position and RX power - AU 915 MHz
  vector<double> distribution(6, 0); // só estou usando do SF7 - SF12
  // // distribution[0] = 1; // distribution[0] - DR5 - S7
  distribution[3] = 1; // distribution[3] - DR2 - S10 = 125khz
  vector<int> sf = macHelper.SetSpreadingFactorsAuGivenDistribution (endDevices, gateways, distribution);

  // Print ED distribution by SF
  cout << "\n- Qtd de EDs por SF:  [ SF7 SF8 SF9 SF10 SF11 SF12 ] = [ ";
  for ( vector<int>::const_iterator i = sf.begin(); i != sf.end(); ++i)
    cout << *i << ' ';
  cout << "] \n";
  

  // Connect trace sources
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
  {
      Ptr<Node> node = *j;
      // uint32_t id =  node->GetId();
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();
      // uint8_t DR =  mac->GetDataRate();
      // cout << "[DEBUG]: " <<  mac->GetDataRate() << "\n";
      // if (unsigned (DR) == 5){
      //     deviceList[id].SF = 5;
      // }else if (unsigned(DR) == 4){
      //     deviceList[id].SF = 4;
      // }else if (unsigned(DR) == 3){
      //     deviceList[id].SF = 3;
      // }else if (unsigned(DR) == 2){
          // deviceList[id].SF = 2;
      // }else if (unsigned(DR) == 1){
      //     deviceList[id].SF = 1;
      // }else{
      //     deviceList[id].SF = 0;
      // }
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

  Time appStopTime = simulationTime;
  PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
  appHelper.SetPeriod (appPeriod);
  appHelper.SetPacketSize (payloadSize);

  // make simulation result random
  Ptr<RandomVariableStream> rv = CreateObjectWithAttributes<UniformRandomVariable> (
      "Min", DoubleValue (0), "Max", DoubleValue (10));

  ApplicationContainer appContainer = appHelper.Install (endDevices);

  // Start simulation
  Simulator::Stop (appStopTime);

  Simulator::Run ();
  Simulator::Destroy ();


  for(NodeContainer::Iterator gw = gateways.Begin (); gw != gateways.End (); ++gw){
      uint32_t gwId = (*gw)->GetId(); 
      Ptr<MobilityModel> mobModelG = (*gw)->GetObject<MobilityModel>();
      
      for (NodeContainer::Iterator node = endDevices.Begin (); node != endDevices.End (); ++node){
        Ptr<MobilityModel> mobModel = (*node)->GetObject<MobilityModel>();
        double position = mobModel->GetDistanceFrom(mobModelG);  
        uint32_t nodeId = (*node)->GetId();
      
        std:: cout << "RX power for GW " << gwId << " receive from node "<< nodeId << ": ";
        std:: cout << channel->GetRxPower(txEndDevice, mobModel, mobModelG) << " - distance from GW "
        << position << std::endl ;
      }
  }

  // Pacotes enviados e recebidos
  return helper.GetPacketTracker ();
  // helper.DoPrintDeviceStatus(endDevices, gateways, "resultados.txt");

}

void getSimulationResults(LoraPacketTracker& tracker){

  Time appStopTime = simulationTime;

  int iterator = nDevices; 
  cout << "\n- Evaluate the performance at PHY level of a specific gateway: \n";
  for (int gw = 0; gw != nGateways; ++gw){
      vector <int> output = tracker.CountPhyPacketsPerGw(Seconds(0), appStopTime, iterator);
      cout << "GwID " << gw << "\nReceived: " << output.at(1) << "\nInterfered: " << output.at(2)
      << "\nNoMoreReceivers: " << output.at(3) << "\nUnderSensitivity: " << output.at(4) << "\nLost: " << output.at(5)
      << "\n" << "\n";
  }

  cout << "\n- Evaluate the global performance at MAC level of the whole network: \n";
  string s =  tracker.CountMacPacketsGlobally (Seconds (0), appStopTime);
  stringstream ss(s);
  string item;
  vector< string> splittedStrings;
  while ( getline(ss, item, ' ')) {
    splittedStrings.push_back(item);
  }
  double sent =  stod(splittedStrings[0]);
  double receiv = stod(splittedStrings[1]);
  double PER = ( sent - receiv )/receiv;
  cout <<  "Nº of Pkts Sent and Received: " << sent << ' ' << receiv << "\n";
  cout << "Packet error rate: " << PER << "\n";

  // ofstream os_per_file;
  // string logFile = output_results_path + per_result_file;
  // os_per_file.open (logFile.c_str (), std::ofstream::out | std::ofstream::app);
  // os_per_file << sent << "," << receiv << "," << PER << "\n";
  // os_per_file.close();
  
  // // Somatorio de delays por per SF (?)
  // for(auto i = pacote_dr.begin(); i != pacote_dr.end(); i++){
  //   int SF = pacote_sf[i->first];
  //   spreadFList[SF].delay += i->second;
  // }

  // cout << "\n- Nº of Pkts sent, received, Average delay per SF & Delay per SF:\n";
  // for ( vector<spf>::iterator i = spreadFList.begin(); i!= spreadFList.end(); ++i){
  //   if(i->delay != Time(0)){
  //       cout << i->S << " " << i->R <<  " " << (i->delay/i->R).GetMilliSeconds()  << " " << i->delay/i->R  << " " << i->delay << "\n";
  //     i->S = i->R  = 0;
  //     i->delay = Time(0);
  //   }
  //   else{  
  //       cout << i->S << " " << i->R << "\n";
  //     i->S = i->R = 0;
  //     i->delay = Time(0);
  //   }  
  // }


  cout << "count_send_pkts: "   << count_send_pkts   << std::endl ;
  cout << "count_receiv_pkts: " << count_receiv_pkts <<  std::endl ;

  // Structs Cleaning
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
      cmd.AddValue ("exp_name", "Experiment name", exp_name);
      cmd.AddValue ("simu_repeat", "Number of Simulation Repeat", nSimulationRepeat);
      cmd.Parse (argc, argv);
     
      // Set up logging
      LogComponentEnable ("lorawan-unicamp-3d", LOG_LEVEL_ALL);
      // LogComponentEnable("LoraChannel", LOG_LEVEL_INFO); // tx, rx info

      output_results_path = "./simulation_results/";
      network_result_file = "network_results_" + exp_name + ".txt";
      per_result_file = "PER_results_" + exp_name + ".txt";

      for(int n = 0; n < nSimulationRepeat; n++){
      
        // generate a different seed for each simulation 
        srand(time(0));
        int seed = rand();
        RngSeedManager::SetSeed (seed); // Changes seed from default of 1 to seed
        RngSeedManager::SetRun (7); // Changes run number from default of 1 to 7

        cout << "\n[SEED "<< n << "]: " << seed << "\n";         
        LoraPacketTracker& tracker = runSimulation();
        getSimulationResults(tracker);
      }

      return 0;
}
