/* This script simulates a complex scenario with multiple GWs, EDs and buildings.
 * - Unicamp Waste Collection Scenario
 * - Okumura model (Channel Model)
 *
 * The metrics of interest for the network of this script are:
 * - Packet Loss Rate 
 * - Packet Error Rate 
 * - Packet Delivery Rate 
 * - Average Delay per SF
 * - Total Average delay
 * 
 * The metrics of interest for the PHY layer of this script are:
 * - PHY metrics (native)
 * - RSSI coverage
 * 
 * 
 * The metrics of interest for power consumption are:
 * - NEC
 * 
 * Authors: Lahis Almeida e Marianna Campos
 * Based on:
 * - https://github.com/GaiaFL/NS-3_LoraWan
 * - https://gitlab.com/serzagit/QoS-802.11ah-NS3/-/blob/master/scratch/test/NodeEntry.cc
 * 
 * 
 *  
 * RUN example:
 * $ cd NS3_BASE_DIR
 * $ ./waf --run "simulation-coletores-conteiners-cenario --simu_repeat=1 --channel_model=okumura"
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

// energy model
#include "ns3/basic-energy-source-helper.h"
#include "ns3/lora-radio-energy-model-helper.h"

// mobilty
#include "ns3/csv-reader.h"
#include "ns3/int64x64-128.h"

// namespaces
using namespace ns3;
using namespace lorawan;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("lorawan-unicamp-3d");

// -------- Setup Variables and Structures --------

struct device{
    double SF;
};

struct spf{
    double S;
    double R;
    Time delay;
};

struct unicamp_battery_bins{ // armazenará dataset de coletores de pilhas e baterias
    double id;
    string name;
    double x;
    double y;
    double z;
    double elevation;
    double lat;
    double lng;
    double delta; // Elevacao do predio - elevacao media da regiao do mapa
    double elev_min;
};

struct unicamp_conteiner_bins{ // armazenará dataset de conteiners n reciclaveis
    double x;
    double y;
    double z;
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
vector<unicamp_battery_bins> unicamp_battery_bins_dataset;
vector<unicamp_conteiner_bins> unicamp_conteiner_bins_dataset;

// Channel model
std::string channel_model = "";

// Network settings
int nDevices = 0; // sera sobrescrito
int nGateways = 1;
int payloadSize = 11;   // bytes: id - 6 bytes, level - 4 bytes, batery - 1 byte
Time appPeriod = Hours(0.5); // 30 em 30 min
// Time appPeriod = Minutes(1); // 1 em 1 min
// Time appPeriod = Seconds(1); 

uint8_t txEndDevice = 20; // Dbm
double regionalFrequency = 915e6; // frequency band AU 915 MHz
// double regionalFrequency = 868e6; // frequency band EU 868 MHz

// Simulation settings
Time simulationTime = Hours(1); // 1 mes   
// Time simulationTime = Minutes(1); // 1 em 1 min 
// Time simulationTime = Seconds(10);  

int nSimulationRepeat = 0;

// Input dataset file names
string nodes_battery_dataset = "coletores_pos_dataset_elev.csv"; //  Nodes positions dataset
string nodes_conteiner_dataset = "conteiners_dataset.csv"; //  Nodes positions dataset

// Output file names
string exp_name = ""; // experiment name
string output_results_path = "./simulation_results/"; // results folder
string rssi_result_file = ""; // rssi results
string net_position_file = ""; // device position by SF results
string net_result_file = ""; // network metrics file (pdr e per)
string delay_result_file = ""; // delay result file
string phy_result_file = ""; // phy result file
string energy_result_file = "";

long double count_send_pkts = 0.;
long double count_receiv_pkts = 0.;

/* -----------------------------------------------------------------------------
*			MAIN
* ------------------------------------------------------------------------------
*/


// -------- Functions --------

void initialize_structs(){
  deviceList.resize(nDevices);
  spreadFList.resize(SF_QTD);
  distances.resize(SF_QTD);
}

void cleaning_structs(){
  pacote_sf.clear();
  pacote_ds.clear();
  pacote_dr.clear();
  deviceList.clear();
  spreadFList.clear();
  distances.clear();

  unicamp_battery_bins_dataset.clear();
  unicamp_conteiner_bins_dataset.clear();
}

// Count Sent Packet per SF
void PacketTraceDevice(Ptr<Packet const> pacote){
    uint32_t id =  Simulator::GetContext ();
    pacote_sf.insert({pacote->GetUid(), deviceList[id].SF});
    Time sendTime = Simulator::Now ();
    pacote_ds.insert({pacote->GetUid(), sendTime});
    spreadFList[deviceList[id].SF].S++;   
    count_send_pkts = count_send_pkts +1;
    cout << "Num of Packets sent: " << count_send_pkts << " - ";
}

// Count Received Packet per SF
void PacketTraceGW(Ptr<Packet const> pacote){
    u_int64_t pkid = pacote->GetUid();
    int sf = pacote_sf.at(pkid);
    Time receivedTime = Simulator :: Now ();
    Time sent = pacote_ds.at(pkid);
    pacote_dr.insert({pkid, receivedTime - sent});
    spreadFList[sf].R++;
    count_receiv_pkts = count_receiv_pkts + 1;
    cout << "receive: " << count_receiv_pkts <<endl;
}

void GetEnergyRemaining(EnergySourceContainer sources, double interval){
  // open log file for output
  ofstream os;
  string logFile = output_results_path + energy_result_file;
  
  os.open (logFile.c_str (), std::ofstream::out | std::ofstream::app);
  // std::cout << "\nEnergy Remaining: " << sources.Get(0)->GetRemainingEnergy() << " " << std::endl;
  os << (Simulator::Now()).GetSeconds() << "," << sources.Get(0)->GetRemainingEnergy() << std::endl;
  os.close();

  Simulator::Schedule(Hours(interval), &GetEnergyRemaining, sources, interval);
}

// write in an output file the position of devices, distance from gateway and positions (x,y) per SF
void GetDevicePositionsPerSF(NodeContainer endDevices, NodeContainer gateways,  Ptr<PropagationDelayModel> delay, double interval){    
    // open log file for output
    string sf;
    ofstream os;
    string logFile = output_results_path + net_position_file;
    os.open (logFile.c_str (), std::ofstream::out);
        
    for(NodeContainer::Iterator gw = gateways.Begin (); gw != gateways.End (); ++gw){
        uint32_t gwId = (*gw)->GetId(); 
        Ptr<MobilityModel> mobModelG = (*gw)->GetObject<MobilityModel>();
        Vector3D posgw = mobModelG->GetPosition();
        
        for (NodeContainer::Iterator node = endDevices.Begin (); node != endDevices.End (); ++node){
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
    Simulator::Schedule(Seconds(interval), &GetDevicePositionsPerSF, endDevices, gateways, delay, interval);
}

void GetGWRSSI(NodeContainer endDevices, NodeContainer gateways,Ptr<LoraChannel> channel){
  ofstream os_rssi_file;
  string logFile = output_results_path + rssi_result_file;
  os_rssi_file.open (logFile.c_str (), std::ofstream::out | std::ofstream::app);

  for(NodeContainer::Iterator gw = gateways.Begin (); gw != gateways.End (); ++gw){
      uint32_t gwId = (*gw)->GetId(); 
      Ptr<MobilityModel> mobModelG = (*gw)->GetObject<MobilityModel>();
      
      for (NodeContainer::Iterator node = endDevices.Begin (); node != endDevices.End (); ++node){
        Ptr<MobilityModel> mobModel = (*node)->GetObject<MobilityModel>();
        double position = mobModel->GetDistanceFrom(mobModelG);  
        uint32_t nodeId = (*node)->GetId();
      
        // std:: cout << "\nRX power for GW " << gwId << " receive from node "<< nodeId << ": ";
        // std:: cout << channel->GetRxPower(20, mobModel, mobModelG) << " - distance from GW "
        // << position << std::endl ;

        os_rssi_file << gwId << "," << nodeId << "," << channel->GetRxPower(txEndDevice, mobModel, mobModelG) << "," <<  position << "\n" ; 
      }
  } 
  os_rssi_file.close();
}

// https://www.nsnam.org/doxygen/classns3_1_1_csv_reader.html
void read_battery_bin_dataset(const std::string &filepath){

  CsvReader csv (filepath);

  while (csv.FetchNextRow ()) {
      // Ignore blank lines
      if (csv.IsBlankRow ()){
          cout << "Blank Line!" << endl;
          continue;
      }

      // colunms: id, name, x, y, z e elevation
      string name;
      double id, x, y, z, elevation, lat, lng, delta, elev_min;
      
      bool ok = csv.GetValue (0, id);
      ok |= csv.GetValue (1, name);
      ok |= csv.GetValue (2, x);
      ok |= csv.GetValue (3, y);
      ok |= csv.GetValue (4, z);
      ok |= csv.GetValue (5, elevation);
      ok |= csv.GetValue (6, lat);
      ok |= csv.GetValue (7, lng);
      ok |= csv.GetValue (8, delta);
      ok |= csv.GetValue (9, elev_min);
      
      if (!ok) {
        // Handle error, then
        // cout << "Read Line Error!" << endl;
        continue;
      }
      else {
        unicamp_battery_bins_dataset.push_back({
          id,  
          name,  
          x,  
          y,  
          z,
          elevation,
          lat,
          lng,
          delta,
          elev_min   
        });
      }
    }  // while FetchNextRow
    
    // delete first row
    unicamp_battery_bins_dataset.erase(unicamp_battery_bins_dataset.begin());
    
    // Show info
    // int aux = 0;
    // cout << "Total Rows: "<< unicamp_battery_bins_dataset.size() << endl;
    // for ( vector<unicamp_battery_bins>::iterator i = unicamp_battery_bins_dataset.begin(); i!= unicamp_battery_bins_dataset.end(); ++i){
    //       cout << aux<<", "<<i->id << ", " << i->name << ", " << i->x << ", " << i->y << ", " << i->z << ", " << i->elevation << ", " << i->delta<< ", " << i->elev_min<< std::endl;
    //       aux++;
    // }
    
}

void read_conteiner_bin_dataset(const std::string &filepath){

  CsvReader csv (filepath);

  while (csv.FetchNextRow ()) {
      // Ignore blank lines
      if (csv.IsBlankRow ()){
          cout << "Blank Line!" << endl;
          continue;
      }

      // colunms: id, name, x, y, z e elevation
      string name;
      double x, y, z;
      
      bool ok = csv.GetValue (0, x);
      ok |= csv.GetValue (1, y);
      ok |= csv.GetValue (2, z);
      
      if (!ok) {
        // Handle error, then
        // cout << "Read Line Error!" << endl;
        continue;
      }
      else {
        unicamp_conteiner_bins_dataset.push_back({
          x,  
          y,  
          z
        });
      }
    }  // while FetchNextRow
    
    // delete first row
    // unicamp_conteiner_bins_dataset.erase(unicamp_conteiner_bins_dataset.begin());
    
    // Show info
    // int aux = 0;
    // cout << "Total Rows: "<< unicamp_conteiner_bins_dataset.size() << endl;
    // for ( vector<unicamp_conteiner_bins>::iterator i = unicamp_conteiner_bins_dataset.begin(); i!= unicamp_conteiner_bins_dataset.end(); ++i){
    //       cout << i->x << ", " << i->y << ", " << i->z << std::endl;
    //       aux++;
    // }
    
}

// Simulation Code
LoraPacketTracker& runSimulation(){

  // Channel Propagation Model
  Ptr<LogDistancePropagationLossModel> logDistLoss;  
  Ptr<OkumuraHataPropagationLossModel> okumuraloss;
  Ptr<PropagationLossModel> final_loss;

  if (channel_model == "log-distance"){
    logDistLoss = CreateObject<LogDistancePropagationLossModel> ();
    logDistLoss->SetPathLossExponent (3.28128); // 915 Mhz, h = (1.5 + 43.41880713641183) = 44.92, R=1m
    logDistLoss->SetReference (1.0, 14.0116);
    final_loss = logDistLoss;
  }
  else if (channel_model == "okumura"){
    Ptr<OkumuraHataPropagationLossModel> okumuraLoss = CreateObject<OkumuraHataPropagationLossModel>();
    okumuraLoss->SetAttribute("Frequency", DoubleValue(regionalFrequency));
    okumuraLoss->SetAttribute("Environment", EnumValue (SubUrbanEnvironment));
    okumuraLoss->SetAttribute("CitySize", EnumValue (SmallCity));
    final_loss = okumuraLoss;
  }

  // Create channel
  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();
  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (final_loss, delay);
  
  // Helpers
  LoraPhyHelper phyHelper = LoraPhyHelper (); // Create the LoraPhyHelper
  phyHelper.SetChannel (channel);
  LorawanMacHelper macHelper = LorawanMacHelper (); // Create the LorawanMacHelper
  LoraHelper helper = LoraHelper ();   // Create the LoraHelper
  helper.EnablePacketTracking ();
 
  // positioning nodes     
  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  for ( vector<unicamp_battery_bins>::iterator i = unicamp_battery_bins_dataset.begin(); i!= unicamp_battery_bins_dataset.end(); ++i){
    allocator->Add (Vector (i->x, i->y, (i->z)));
  }
  for ( vector<unicamp_conteiner_bins>::iterator i = unicamp_conteiner_bins_dataset.begin(); i!= unicamp_conteiner_bins_dataset.end(); ++i){
    allocator->Add (Vector (i->x, i->y, (i->z)));
  }
  cout << "Number of Position Allocated: " << allocator->GetSize() << endl;
  
  MobilityHelper mobility; 
  mobility.SetPositionAllocator(allocator);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); // ED does not move 

  NodeContainer endDevices;
  endDevices.Create (nDevices);
  mobility.Install (endDevices);

  // Make it so that nodes are at a certain height > 0
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j){
      Ptr<MobilityModel> m = (*j)->GetObject<MobilityModel> ();
      Vector position = m->GetPosition ();
      position.z = 1.5;
      m->SetPosition (position);
  }

  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LorawanMacHelper::ED_A);
  macHelper.SetRegion(LorawanMacHelper::Australia);
  NetDeviceContainer endDevicesNetDevices = helper.Install (phyHelper, macHelper, endDevices);
  
  // Gateway
  NodeContainer gateways;
  gateways.Create (nGateways);
  Ptr<ListPositionAllocator> positionAllocGw = CreateObject<ListPositionAllocator> ();
  positionAllocGw->Add (Vector (1694.975, 2141.471, (1.5 + 43.41880713641183) ));    // z - altura antena + (elevacao museu - elevacao do mapa)
  mobility.SetPositionAllocator (positionAllocGw);
  mobility.Install(gateways);

  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LorawanMacHelper::GW);
  macHelper.SetRegion (LorawanMacHelper::Australia);
  helper.Install (phyHelper, macHelper, gateways);
      
  // Set SF automatically based on position and RX power
  vector<int> sf = macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);

  // Set SF manually based on position and RX power - AU 915 MHz
  // vector<double> distribution(6, 0); // só estou usando do SF7 - SF12
  // // distribution[0] = 1; // distribution[0] - DR5 - S7
  // distribution[3] = 1; // distribution[3] - DR2 - S10 = 125khz
  // vector<int> sf = macHelper.SetSpreadingFactorsAuGivenDistribution (endDevices, gateways, distribution);

  // Print ED distribution by SF
  cout << "\n- Qtd de EDs por SF:  [ SF7 SF8 SF9 SF10 SF11 SF12 ] = [ ";
  for ( vector<int>::const_iterator i = sf.begin(); i != sf.end(); ++i)
    cout << *i << ' ';
  cout << "] \n";
  
  // Connect trace sources
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j){
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

  for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); ++j){
      Ptr<Node> node = *j;
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<LorawanMac> mac = loraNetDevice->GetMac()->GetObject<LorawanMac>();
      mac->TraceConnectWithoutContext("ReceivedPacket", MakeCallback(&PacketTraceGW));
  }

  // NetworkServer
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

  /************************
   * Install Energy Model *
   ************************/
  BasicEnergySourceHelper basicSourceHelper;
  LoraRadioEnergyModelHelper radioEnergyHelper;

  // configure energy source
  // values based on SX1272/73 datasheet - Table 6 Power Consumption Specification
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (10000)); // Energy in J
  basicSourceHelper.Set ("BasicEnergySupplyVoltageV", DoubleValue (3.3)); // Volts
  radioEnergyHelper.Set ("StandbyCurrentA", DoubleValue (0.0014)); // Ampere
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.028)); // Ampere
  radioEnergyHelper.Set ("SleepCurrentA", DoubleValue (0.0000015)); // Ampere
  radioEnergyHelper.Set ("RxCurrentA", DoubleValue (0.0112)); // Ampere
  radioEnergyHelper.SetTxCurrentModel ("ns3::ConstantLoraTxCurrentModel","TxCurrent", DoubleValue (0.028)); // Ampere

  // install source on EDs' nodes
  EnergySourceContainer sources = basicSourceHelper.Install (endDevices);
  
  // install device model
  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install(endDevicesNetDevices, sources);

  // make simulation result random
  Ptr<RandomVariableStream> rv = CreateObjectWithAttributes<UniformRandomVariable> (
      "Min", DoubleValue (0), "Max", DoubleValue (10));

  ApplicationContainer appContainer = appHelper.Install (endDevices);

  // Start simulation
  appContainer.Start (Seconds (0));
  Simulator::Stop (appStopTime);
  Simulator::Schedule(Seconds(0.00), &GetDevicePositionsPerSF, endDevices, gateways, delay, 3.0); // seconds
  Simulator::Schedule(Hours(0.00), &GetEnergyRemaining, sources, 1); // hour

  Simulator::Run ();
  Simulator::Destroy ();

  // GET RX POWER - LoRa Coverage
  GetGWRSSI(endDevices, gateways,channel);

  // Pacotes enviados e recebidos
  return helper.GetPacketTracker ();
  // helper.DoPrintDeviceStatus(endDevices, gateways, "resultados.txt");
}

void getSimulationResults(LoraPacketTracker& tracker){

  Time appStopTime = simulationTime;
  int iterator = nDevices; 

  // Metricas da Camada Física
  ofstream phy_file;
  string logFile = output_results_path + phy_result_file;
  phy_file.open (logFile.c_str (), std::ofstream::out | std::ofstream::app); 
  cout << "\n- Evaluate the performance at PHY level of a specific gateway: \n";
  for (int gw = 0; gw != nGateways; ++gw){
      vector <int> output = tracker.CountPhyPacketsPerGw(Seconds(0), appStopTime, iterator);
      cout << "GwID " << gw << "\nReceived: " << output.at(1) << "\nInterfered: " << output.at(2)
      << "\nNoMoreReceivers: " << output.at(3) << "\nUnderSensitivity: " << output.at(4) << "\nLost: " << output.at(5) << "\n";

      phy_file << gw << "," << output.at(1) << "," << output.at(2) << "," << output.at(3) << "," << output.at(4) << "," << output.at(5) << "\n";
  }
  phy_file.close();

  // Metricas da Rede completa
  cout << "\n- Evaluate the global performance at MAC level of the whole network: \n";
  string s =  tracker.CountMacPacketsGlobally (Seconds (0), appStopTime);
  stringstream ss(s);
  string item;
  vector< string> splittedStrings;
  while ( getline(ss, item, ' ')) {
    splittedStrings.push_back(item);
  }

  //pdr: https://www.sciencedirect.com/topics/computer-science/packet-delivery-ratio
  double sent =  stod(splittedStrings[0]);
  double receiv = stod(splittedStrings[1]);
  double PER = ( sent - receiv )/receiv;
  double PLR = ( sent - receiv )/sent;
  double PDR = receiv/sent;
  cout <<  "Nº of Pkts Sent and Received: " << sent << ' ' << receiv << "\n";
  cout << "Packet error rate: " << PER << "\n";
  cout << "Packet loss rate: " << PLR << "\n";
  cout << "Packet delivery rate: " << PDR << "\n";

  ofstream network_file;
  logFile = output_results_path + net_result_file;
  network_file.open (logFile.c_str (), std::ofstream::out | std::ofstream::app);
  network_file << sent << "," << receiv << "," << PER << "," << PLR << "," << PDR << "\n";
  network_file.close();
  
  // Somatorio de delays por per SF 
  for(auto i = pacote_dr.begin(); i != pacote_dr.end(); i++){
    int SF = pacote_sf[i->first];
    spreadFList[SF].delay += i->second;
  }

  ofstream delay_file;
  logFile = output_results_path + delay_result_file;
  delay_file.open (logFile.c_str (), std::ofstream::out | std::ofstream::app);

  cout << "\n- Nº of Pkts sent, received, Average delay per SF (ms), Average delay per SF (ns) & Somatorio de delays per SF:\n";
  for ( vector<spf>::iterator i = spreadFList.begin(); i!= spreadFList.end(); ++i){
    if(i->delay != Time(0)){
      cout << i->S << " " << i->R <<  " " << (i->delay/i->R).GetMilliSeconds()  << " " << i->delay/i->R  << " " << i->delay << "\n";
      delay_file << i->S << "," << i->R << "," << (i->delay/i->R).GetMilliSeconds() << "," << i->delay/i->R << "," << i->delay << "\n";
      i->S = i->R  = 0;
      i->delay = Time(0);
    }
    else{  
      cout << i->S << " " << i->R << "\n";
      i->S = i->R = 0;
      i->delay = Time(0);
    }  
  }
  delay_file.close();
}

int main (int argc, char *argv[])
{
      CommandLine cmd;
      cmd.AddValue ("simu_repeat", "Number of Simulation Repeat", nSimulationRepeat);
      cmd.AddValue ("channel_model", "Channel Model", channel_model);
      cmd.Parse (argc, argv);
     
      // Set up logging
      LogComponentEnable ("lorawan-unicamp-3d", LOG_LEVEL_ALL);
      // LogComponentEnable("LoraChannel", LOG_LEVEL_INFO); // tx, rx info

      // change result files name by experiment name
      rssi_result_file = "rssi_results.txt";
      net_position_file = "network_position.txt";
      net_result_file = "net_results.txt";
      delay_result_file = "delay_results.txt";
      phy_result_file = "phy_results.txt";
      energy_result_file = "energy_results.txt"; 

      for(int n = 0; n < nSimulationRepeat; n++){
        // generate a different seed for each simulation 
        srand(time(0));
        int seed = rand();
        RngSeedManager::SetSeed (seed); 
        RngSeedManager::SetRun (7); 

        cout << "\n[SEED "<< n << "]: " << seed << "\n";  
        
        //load nodes dataset  
        read_battery_bin_dataset(nodes_battery_dataset); 
        read_conteiner_bin_dataset(nodes_conteiner_dataset);
        nDevices = unicamp_battery_bins_dataset.size() + unicamp_conteiner_bins_dataset.size();
        cout << "unicamp_battery_bins_dataset:" << unicamp_battery_bins_dataset.size() << endl;
        cout << "unicamp_conteiner_bins_dataset:" << unicamp_conteiner_bins_dataset.size() << endl;
        cout << "nDevices:" << nDevices << endl;
     
        initialize_structs(); // Structs Inicialization
        LoraPacketTracker& tracker = runSimulation(); // run simulation
        getSimulationResults(tracker); // calculate results
        cleaning_structs(); // Structs Cleaning

        count_send_pkts = 0;
        count_receiv_pkts = 0;
      }

      return 0;
}

