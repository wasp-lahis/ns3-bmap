/* This script simulates a complex scenario with multiple EDs and Aplications.
 * - Unicamp Waste Collection Scenario ()
 * - 
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
 * Authors: Lahis Almeida e Marianna Campos
 * Based on:
 * - https://github.com/GaiaFL/NS-3_LoraWan
 * - https://gitlab.com/serzagit/QoS-802.11ah-NS3/-/blob/master/scratch/test/NodeEntry.cc
 * 
 * 
 *  
 * RUN example:
 * $ cd NS3_BASE_DIR
 * $ conda activate ns3-buildings
 * $ ./waf --run "simulation-many-lorawan-applications_with_random_nodes --simu_repeat=1 --channel_model=okumura&nakagami --n_devices_without_dataset=0"
 */


/* -----------------------------------------------------------------------------
*     HEADERS
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
// #include <ns3/spectrum-module.h>
#include <ns3/okumura-hata-propagation-loss-model.h>
#include "ns3/correlated-shadowing-propagation-loss-model.h"
// #include "ns3/flow-monitor-helper.h"

// energy-harvester
#include "ns3/energy-module.h"
#include "ns3/basic-energy-harvester.h"
// basic energy model
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
    double elevation_norm;
};

struct unicamp_conteiner_bins{ // armazenará dataset de conteiners n reciclaveis
    double x;
    double y;
    double z;
    double lat;
    double lng;
    double elevation;
    double elevation_norm;
};

struct unicamp_smart_meters{ // armazenará dataset de conteiners n reciclaveis
    double x;
    double y;
    double z;
    double lat;
    double lng;
    double elevation;
    double elevation_norm;
};

// Instantiate of data structures
uint8_t SF_QTD = 6; // AU 915 MHz and EU 868 MHz

vector<device> deviceList;
vector<device> deviceList_aux;
vector<spf> spreadFList;
map <uint64_t, int> pacote_sf;
map <uint64_t, Time> pacote_ds; // send
map <uint64_t, Time> pacote_dr; // receive
vector<double> distances;

// dataset structs
vector<unicamp_battery_bins> unicamp_battery_bins_dataset;
vector<unicamp_conteiner_bins> unicamp_conteiner_bins_dataset;
vector<unicamp_smart_meters> unicamp_smart_meters_dataset;

// Channel model
std::string channel_model = "";

// Network settings
int nDevices = 0; // sera sobrescrito
int nDevices_without_dataset = 0;
int nGateways = 2;

uint8_t txEndDevice = 20; // Dbm
double regionalFrequency = 915e6; // frequency band AU 915 MHz
// double regionalFrequency = 868e6; // frequency band EU 868 MHz

// Simulation settings
int nSimulationRepeat = 0;
int nSimulation = 0;
Time simulationTime = Hours(24); // 1 semana

// Input dataset file names
string nodes_battery_dataset = "coletores_pos_dataset_elev_norm.csv"; //  Nodes positions dataset
string nodes_conteiner_dataset = "conteiners_dataset_elev_norm.csv"; //  Nodes positions dataset
string nodes_smart_meter_dataset = "medidores_inteligentes_dataset_elev_norm.csv"; //  Nodes positions dataset

// Output file names
string exp_name = ""; // experiment name
string output_results_path = "./teste/"; // results folder
string rssi_result_file = ""; // rssi results
string net_position_file = ""; // device position by SF results
string net_result_file = ""; // network metrics file (pdr e per)
string delay_result_file = ""; // delay result file
string phy_result_file = ""; // phy result file

long double count_send_pkts = 0.;
long double count_receiv_pkts = 0.;

/* -----------------------------------------------------------------------------
*     MAIN
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
  unicamp_smart_meters_dataset.clear();
}

// Count Sent Packet per SF
void PacketTraceDevice(Ptr<Packet const> pacote){
    uint32_t id =  Simulator::GetContext ();
    pacote_sf.insert({pacote->GetUid(), deviceList[id].SF});
    Time sendTime = Simulator::Now ();
    pacote_ds.insert({pacote->GetUid(), sendTime});
    spreadFList[deviceList[id].SF].S++;   
    count_send_pkts = count_send_pkts +1;
    cout <<"\n[SIMU " << nSimulation << "] "<<"Num of Packets sent: " << count_send_pkts << " - ";
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
    cout << "receive: " << count_receiv_pkts;
}

// write in an output file the position of devices, distance from gateway and positions (x,y) per SF
void GetDevicePositionsPerSF(NodeContainer endDevices, NodeContainer gateways){    
    // open log file for output
    string sf;
    ofstream os;
    string logFile = output_results_path + net_position_file;
    os.open (logFile.c_str (), std::ofstream::out| std::ofstream::app);
        
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
      double id, x, y, z, elevation, lat, lng, elevation_norm;
      
      bool ok = csv.GetValue (0, id);
      ok |= csv.GetValue (1, name);
      ok |= csv.GetValue (2, x);
      ok |= csv.GetValue (3, y);
      ok |= csv.GetValue (4, z);
      ok |= csv.GetValue (5, elevation);
      ok |= csv.GetValue (6, lat);
      ok |= csv.GetValue (7, lng);
      ok |= csv.GetValue (8, elevation_norm);
     
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
          elevation_norm
        });
      }
    }  // while FetchNextRow
    
    // delete first row
    unicamp_battery_bins_dataset.erase(unicamp_battery_bins_dataset.begin());
    
    // Show info
    // int aux = 0;
    // cout << "Total Rows: "<< unicamp_battery_bins_dataset.size() << endl;
    // for ( vector<unicamp_battery_bins>::iterator i = unicamp_battery_bins_dataset.begin(); i!= unicamp_battery_bins_dataset.end(); ++i){
    //       cout << aux <<", "<<i->id << ", " << i->name << ", ";
    //       cout << i->x << ", " << i->y << ", " << i->z << ", ";
    //       cout << i->elevation << ", " << i->lat<< ", " << i->lng;
    //       cout <<  ", " << i->elevation_norm  << std::endl;
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
      double x, y, z, lat, lng, elevation, elevation_norm;
      
      bool ok = csv.GetValue (0, x);
      ok |= csv.GetValue (1, y);
      ok |= csv.GetValue (2, z);
      ok |= csv.GetValue (3, lat);
      ok |= csv.GetValue (4, lng);
      ok |= csv.GetValue (5, elevation);
      ok |= csv.GetValue (6, elevation_norm);
      
      if (!ok) {
        // Handle error, then
        // cout << "Read Line Error!" << endl;
        continue;
      }
      else {
        unicamp_conteiner_bins_dataset.push_back({
          x,  
          y,  
          z,
          lat,
          lng,
          elevation,
          elevation_norm
        });
      }
    }  // while FetchNextRow
    
    // delete first row
    // unicamp_conteiner_bins_dataset.erase(unicamp_conteiner_bins_dataset.begin());
    
    // Show info
    // int aux = 0;
    // cout << "Total Rows: "<< unicamp_conteiner_bins_dataset.size() << endl;
    // for ( vector<unicamp_conteiner_bins>::iterator i = unicamp_conteiner_bins_dataset.begin(); i!= unicamp_conteiner_bins_dataset.end(); ++i){
    //       cout << i->x << ", " << i->y << ", " << i->z << ", ";
    //       cout << i->lat << ", " << i->lng << ", " << i->elevation;
    //       cout << ", " << i->elevation_norm << std::endl;
    //       aux++;
    // }
    
}

void read_smart_meter_dataset(const std::string &filepath){

  CsvReader csv (filepath);

  while (csv.FetchNextRow ()) {
      // Ignore blank lines
      if (csv.IsBlankRow ()){
          cout << "Blank Line!" << endl;
          continue;
      }

      // colunms: id, name, x, y, z e elevation
      string name;
      double x, y, z, lat, lng, elevation, elevation_norm;
      
      bool ok = csv.GetValue (0, x);
      ok |= csv.GetValue (1, y);
      ok |= csv.GetValue (2, z);
      ok |= csv.GetValue (3, lat);
      ok |= csv.GetValue (4, lng);
      ok |= csv.GetValue (5, elevation);
      ok |= csv.GetValue (6, elevation_norm);
      
      if (!ok) {
        // Handle error, then
        // cout << "Read Line Error!" << endl;
        continue;
      }
      else {
        unicamp_smart_meters_dataset.push_back({
          x,  
          y,  
          z,
          lat,
          lng,
          elevation,
          elevation_norm
        });
      }
    }  // while FetchNextRow
    
    // delete first row
    // unicamp_conteiner_bins_dataset.erase(unicamp_conteiner_bins_dataset.begin());
    
    // Show info
    // int aux = 0;
    // cout << "Total Rows: "<< unicamp_smart_meters_dataset.size() << endl;
    // for ( vector<unicamp_smart_meters>::iterator i = unicamp_smart_meters_dataset.begin(); i!= unicamp_smart_meters_dataset.end(); ++i){
    //   cout << i->x << ", " << i->y << ", " << i->z << ", ";
    //   cout << i->lat << ", " << i->lng << ", " << i->elevation;
    //   cout << ", " << i->elevation_norm << std::endl;
    //   aux++;
    // }
    
}

Ptr<ListPositionAllocator> SetNodePositions(int nDevices_random){
  // Positioning nodes from datasets     
  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  for ( vector<unicamp_battery_bins>::iterator i = unicamp_battery_bins_dataset.begin(); i!= unicamp_battery_bins_dataset.end(); ++i){
    allocator->Add (Vector (i->x, i->y, (i->z)));
    // allocator->Add (Vector (i->x, i->y, (i->z + i->elevation_norm)));
  }
  for ( vector<unicamp_conteiner_bins>::iterator i = unicamp_conteiner_bins_dataset.begin(); i!= unicamp_conteiner_bins_dataset.end(); ++i){
    allocator->Add (Vector (i->x, i->y, i->z));
  }
  for ( vector<unicamp_smart_meters>::iterator i = unicamp_smart_meters_dataset.begin(); i!= unicamp_smart_meters_dataset.end(); ++i){
    allocator->Add (Vector (i->x, i->y, i->z));
  }
  
  // Positioning nodes that does not have a dataset
  // o mapa da Unicamp Normalizado tem ~2508x2439 metros
  Ptr<UniformRandomVariable> random_x = CreateObject<UniformRandomVariable> ();
  Ptr<UniformRandomVariable> random_y = CreateObject<UniformRandomVariable> ();
  random_x->SetAttribute("Min", DoubleValue(0));
  random_x->SetAttribute("Max", DoubleValue(2508));
  random_y->SetAttribute("Min", DoubleValue(0));
  random_y->SetAttribute("Max", DoubleValue(2440));
  
  // nDevices_w_dataset * 2 -> 2 applications
  // Total of extra nodes: nDevices_w_dataset * 2
  int num_nodes_from_datasets = allocator->GetSize();
  for(int i = num_nodes_from_datasets; i < (num_nodes_from_datasets + nDevices_random); i++ ){
      allocator->Add (Vector (random_x->GetValue(), random_y->GetValue(), 1.5));
  }

  cout << "[INFO] Number of Position Allocated: " << allocator->GetSize() << endl;
  
  return allocator;
}

Ptr<PropagationLossModel> SetChannelPropagation(std::string channel_propag_select){

  // Channel Propagation Model
  Ptr<LogDistancePropagationLossModel> logDistLoss;  
  Ptr<OkumuraHataPropagationLossModel> okumuraloss;
  Ptr<CorrelatedShadowingPropagationLossModel> shadowing;
  Ptr<PropagationLossModel> final_loss;

  if (channel_propag_select == "log-distance"){
    logDistLoss = CreateObject<LogDistancePropagationLossModel> ();
    
    // com elevação normalizada h = (1.5 + 9.05882353) = 10.5588, f=915 Mhz, R=1m
    logDistLoss->SetPathLossExponent (3.831);
    logDistLoss->SetReference (1.0, 8.8347);

    // com elevação h = (1.5 + 43.41880713641183) = 44.92, f=915 Mhz, R=1m
    // logDistLoss->SetPathLossExponent (3.28128);
    // logDistLoss->SetReference (1.0, 14.0116);

    // sem elevação h= 1.5, f= 915, R=1m
    // logDistLoss->SetPathLossExponent (3.976); 
    // logDistLoss->SetReference (1.0, 19.7402);

    final_loss = logDistLoss;
  }
  else if (channel_propag_select == "correlated-shadowing"){
    logDistLoss = CreateObject<LogDistancePropagationLossModel> ();
    
    // com elevação normalizada h = (1.5 + 9.05882353) = 10.5588, f=915 Mhz, R=1m
    logDistLoss->SetPathLossExponent (3.831);
    logDistLoss->SetReference (1.0, 8.8347);

    // com elevação h = (1.5 + 43.41880713641183) = 44.92, f=915 Mhz, R=1m
    // logDistLoss->SetPathLossExponent (3.28128);
    // logDistLoss->SetReference (1.0, 14.0116);

    // sem elevação h= 1.5, f= 915MHz,  R=1m
    // logDistLoss->SetPathLossExponent (3.976); 
    // logDistLoss->SetReference (1.0, 19.7402);

    // Create the correlated shadowing component
    shadowing = CreateObject<CorrelatedShadowingPropagationLossModel> ();
    shadowing->SetAttribute("CorrelationDistance",  DoubleValue(110.0));
    logDistLoss->SetNext (shadowing); // Aggregate shadowing to the logdistance loss
    final_loss = logDistLoss;
  }
  else if (channel_propag_select == "okumura"){
    Ptr<OkumuraHataPropagationLossModel> okumuraLoss = CreateObject<OkumuraHataPropagationLossModel>();
    okumuraLoss->SetAttribute("Frequency", DoubleValue(regionalFrequency));
    okumuraLoss->SetAttribute("Environment", EnumValue (SubUrbanEnvironment));
    okumuraLoss->SetAttribute("CitySize", EnumValue (SmallCity));
    final_loss = okumuraLoss;
  }
  else if (channel_propag_select == "okumura&nakagami"){
    Ptr<NakagamiPropagationLossModel> nakagami = CreateObject<NakagamiPropagationLossModel>();
    nakagami->SetAttribute("m0", DoubleValue(1));
    nakagami->SetAttribute("m1",DoubleValue(1));
    nakagami->SetAttribute("m2",DoubleValue(1));
    Ptr<OkumuraHataPropagationLossModel> okumuraLoss = CreateObject<OkumuraHataPropagationLossModel>();
    okumuraLoss->SetAttribute("Frequency",DoubleValue(regionalFrequency));
    // okumuraLoss->SetAttribute("Environment", EnumValue (SubUrbanEnvironment));
    // okumuraLoss->SetAttribute("CitySize", EnumValue (SmallCity));
    okumuraLoss->SetNext(nakagami);
    okumuraLoss->Initialize();
    final_loss = okumuraLoss;
  }

  return final_loss;
}

// Simulation Code
LoraPacketTracker& runSimulation(int numDevices, int numRandomDevices, Ptr<ListPositionAllocator> nodePositionAllocator, Ptr<LoraChannel> channel){

  Ptr<LoraChannel> channel_propag = channel;

  // Craete PHY and MAC Helpers
  LoraPhyHelper phyHelper = LoraPhyHelper (); // Create the LoraPhyHelper
  phyHelper.SetChannel (channel_propag);
  LorawanMacHelper macHelper = LorawanMacHelper (); // Create the LorawanMacHelper
  LoraHelper helper = LoraHelper ();   // Create the LoraHelper
  helper.EnablePacketTracking ();

  // Node devices Mobility
  MobilityHelper mobility; 
  mobility.SetPositionAllocator(nodePositionAllocator);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); // ED does not move 
  
  // Node devices containers
  NodeContainer endDevices;
  endDevices.Create (numDevices);
  mobility.Install (endDevices);

  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LorawanMacHelper::ED_A);
  macHelper.SetRegion(LorawanMacHelper::Australia);
  NetDeviceContainer endDevicesNetDevices = helper.Install (phyHelper, macHelper, endDevices);
  
  // Gateway device container
  NodeContainer gateways;
  gateways.Create (nGateways);
  Ptr<ListPositionAllocator> positionAllocGw = CreateObject<ListPositionAllocator> ();
  
  // Posição do Museu da Unicamp
  // positionAllocGw->Add (Vector (1694.975, 2141.471, (1.5 + 9.05882353))); // z - altura antena + elevacao museu normalizada
  // positionAllocGw->Add (Vector (1694.975, 2141.471, (1.5 + 43.41880713641183) ));    // z - altura antena + (elevacao museu - elevacao do mapa)
  positionAllocGw->Add (Vector (1694.975, 2141.471, 1.5 )); //Museu
  positionAllocGw->Add (Vector (1197.962, 330.562, 1.5 )); //Medicina
  mobility.SetPositionAllocator (positionAllocGw);
  mobility.Install(gateways);

  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LorawanMacHelper::GW);
  macHelper.SetRegion (LorawanMacHelper::Australia);
  helper.Install (phyHelper, macHelper, gateways);
      
  // Set SF automatically based on position and RX power
  vector<int> sf;
  if (nSimulation == 0){
    vector<int> sf_up = macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel_propag);
    sf = sf_up;

    for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j){
      Ptr<Node> node = *j;
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();
      uint8_t DR = mac->GetDataRate();
      if (unsigned (DR) < 2){
        mac->SetDataRate (2);
        sf[3] = sf[3] + 1;
      }
    }
    sf[4] = sf[5] = sf[6] = 0;  
  }
  else{
    std::vector<int> sf_manually (4, 0); // DR0-DR13
    int sf_result_per_node = 0;
    // 563 numero total de nodes - dps deixar inteligente
    for(int i = 0; i < 563; i++){
      sf_result_per_node = macHelper.SetSpreadingFactorsGivenDistributionManually (endDevices, gateways, i, deviceList_aux[i].SF);

      if (sf_result_per_node == 5) sf_manually[0] = sf_manually[0] + 1;
      else if (sf_result_per_node == 4) sf_manually[1] = sf_manually[1] + 1;
      else if (sf_result_per_node == 3) sf_manually[2] = sf_manually[2] + 1;
      else sf_manually[3] = sf_manually[3] + 1;
    }
    sf = sf_manually;
    
  }
  // Set SF manually based on position and RX power - AU 915 MHz
  // vector<double> distribution(6, 0); // só estou usando do SF7 - SF12
  // distribution[0] = 167; // distribution[0] - DR5 - S7
  // distribution[1] = 68; // distribution[1] - DR4 - S8
  // distribution[2] = 86; // distribution[2] - DR3 - S9
  // distribution[3] = 55; // distribution[3] - DR2 - S10 = 125khz
  // distribution[4] = 36; // distribution[3] - DR1 - S11 = 125khz
  // distribution[5] = 68; // distribution[3] - DR0 - S12 = 125khz
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
      }else{
          deviceList[id].SF = 2;
      }/*else if (unsigned(DR) == 1){
          deviceList[id].SF = 1;
      }else{
          deviceList[id].SF = 0;
      }*/
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

  // Set simulation duration
  Time appStopTime = simulationTime;

  // ---- Creation of packet intervals and payload sizes for each type of node application:
  //  - batteries, containers, smart meters, air_monitoring, indoor/outdoor, localization
  PeriodicSenderHelper appHelper_battery = PeriodicSenderHelper ();
  appHelper_battery.SetPeriod (Hours(56)); // 7 dias -> 1176 horas, 1176/3 => 3x na semana
  //appHelper_battery.SetPeriod (Hours(24)); 
  appHelper_battery.SetPacketSize (5); // bytes

  PeriodicSenderHelper appHelper_container = PeriodicSenderHelper ();
  appHelper_container.SetPeriod (Hours(6)); // 4x dia
  //appHelper_container.SetPeriod (Hours(24)); // 4x dia
  appHelper_container.SetPacketSize (31);

  PeriodicSenderHelper appHelper_smart_meter = PeriodicSenderHelper ();
  appHelper_smart_meter.SetPeriod (Minutes(15));
  //appHelper_smart_meter.SetPeriod (Hours(24));
  appHelper_smart_meter.SetPacketSize (49); // 3 correntes, 3 angulos, 3 tensoes, 3 fpotencia, 1 frequencia

  PeriodicSenderHelper appHelper_air_monitoring = PeriodicSenderHelper ();
  appHelper_air_monitoring.SetPeriod (Seconds(10));
  //appHelper_air_monitoring.SetPeriod (Hours(24));
  appHelper_air_monitoring.SetPacketSize (20);

  PeriodicSenderHelper appHelper_localization = PeriodicSenderHelper ();
  appHelper_localization.SetPeriod (Hours(24));
  appHelper_localization.SetPacketSize (32);
  // ----

  // Separation of nodes into:
  //  - baterry, container, smart meter,
  //  air monitoring, indoor/outdoor, localization type
  NodeContainer endDevices_pilhas;
  NodeContainer endDevices_conteiners;
  NodeContainer endDevices_smart_meter;
  NodeContainer endDevices_air_monitoring;
  NodeContainer endDevices_localization;
  
  int size_max = int(unicamp_battery_bins_dataset.size());
  for(int i = 0; i < size_max; i++){
    endDevices_pilhas.Add(endDevices.Get(i));
  }
  for(int i = size_max; i < (size_max + int(unicamp_conteiner_bins_dataset.size())); i++){
    endDevices_conteiners.Add(endDevices.Get(i));
  }
  size_max = size_max + int(unicamp_conteiner_bins_dataset.size());
  for(int i = size_max; i < (size_max + int(unicamp_smart_meters_dataset.size())); i++){
    endDevices_smart_meter.Add(endDevices.Get(i));
  }
  size_max = size_max + int(unicamp_smart_meters_dataset.size());
  //cout << "\n[INFO] Size max: " << size_max << "\n" << endl ;

  // Random nodes
  for(int i = size_max; i < (size_max + numRandomDevices); i++){
    endDevices_air_monitoring.Add(endDevices.Get(i));
  }
  size_max = size_max + numRandomDevices;
  for(int i = size_max; i < (size_max + numRandomDevices); i++){
    endDevices_localization.Add(endDevices.Get(i));
  }
  cout << "\n[INFO] Size max: " << size_max + numRandomDevices << "\n" << endl ;


  // Creating follow applications:
  //  - baterry, container, smart meter, air monitoring, indoor/outdoor, localization type
  ApplicationContainer appContainer_battery = appHelper_battery.Install (endDevices_pilhas);
  ApplicationContainer appContainer_container = appHelper_container.Install (endDevices_conteiners);
  ApplicationContainer appContainer_smart_meter = appHelper_smart_meter.Install (endDevices_smart_meter);
  ApplicationContainer appContainer_air_monitoring = appHelper_air_monitoring.Install (endDevices_air_monitoring);
  ApplicationContainer appContainer_localization = appHelper_localization.Install (endDevices_localization);
  
  //Addition of application subgroups in the final application of the simulation 
  ApplicationContainer appContainer;
  appContainer.Add(appContainer_battery);
  appContainer.Add(appContainer_container);
  appContainer.Add(appContainer_smart_meter);
  appContainer.Add(appContainer_air_monitoring);
  appContainer.Add(appContainer_localization);

  //Verification of packet interval per node
  /*for (ApplicationContainer::Iterator j = appContainer.Begin (); j != appContainer.End (); ++j){
    Ptr<Application> app = *j;
    Ptr<Node> node = app->GetNode();
    Ptr<PeriodicSender> t = app->GetObject<PeriodicSender>();
    cout << "Nó "<< node->GetId() << ": " << t->GetInterval().GetHours() << " Horas\n"; 
  }*/

  // make simulation result random
  Ptr<RandomVariableStream> rv = CreateObjectWithAttributes<UniformRandomVariable> (
      "Min", DoubleValue (0), "Max", DoubleValue (10));

  // Start simulation
  appContainer.Start (Seconds (0));
  Simulator::Stop (appStopTime);
  Simulator::Run ();
  Simulator::Destroy ();

  // Get Device Positionn/SF
  if (nSimulation == 0){
    GetDevicePositionsPerSF(endDevices, gateways);
    deviceList_aux = deviceList;
    //cout << "\n+++++++++++++++ oi\n";
  }

  // GET RX POWER - LoRa Coverage
  GetGWRSSI(endDevices, gateways, channel_propag);

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
  cout << "\n\n- Evaluate the performance at PHY level of a specific gateway: \n";
  for (int gw = 0; gw != nGateways; ++gw){
      vector <int> output = tracker.CountPhyPacketsPerGw(Seconds(0), appStopTime, iterator);
      cout << "GwID " << gw << "\nReceived: " << output.at(1) << "\nInterfered: " << output.at(2)
      << "\nNoMoreReceivers: " << output.at(3) << "\nUnderSensitivity: " << output.at(4) << "\nLost: " << output.at(5) << "\n";

      phy_file << gw << "," << output.at(1) << "," << output.at(2) << "," << output.at(3) << "," << output.at(4) << "," << output.at(5) << "\n";
      iterator += 1;
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
      cmd.AddValue ("n_devices_without_dataset", "Number of nodes without dataset. This number will be multiple by 3", nDevices_without_dataset);
      cmd.Parse (argc, argv);
     
      // Set up logging
      LogComponentEnable ("lorawan-unicamp-3d", LOG_LEVEL_ALL);
      // LogComponentEnable("LoraChannel", LOG_LEVEL_INFO); // tx, rx info
      //LogComponentEnable("LoraPacketTracker", LOG_LEVEL_INFO); // tx, rx info

      // change result files name by experiment name
      rssi_result_file = "rssi_results.txt";
      net_position_file = "network_position.txt";
      net_result_file = "net_results.txt";
      delay_result_file = "delay_results.txt";
      phy_result_file = "phy_results.txt";

      // Load node datasets  
      read_battery_bin_dataset(nodes_battery_dataset); 
      read_conteiner_bin_dataset(nodes_conteiner_dataset);
      read_smart_meter_dataset(nodes_smart_meter_dataset);
      nDevices = unicamp_battery_bins_dataset.size() + unicamp_conteiner_bins_dataset.size() + unicamp_smart_meters_dataset.size();
      nDevices_without_dataset = nDevices_without_dataset * 2;

      // Set position nodes
      Ptr<ListPositionAllocator> nodePositionAllocator = SetNodePositions(nDevices_without_dataset);
      
      // Set propagation loss
      Ptr<PropagationLossModel> propagation_model = SetChannelPropagation(channel_model);
     
      for(nSimulation = 0; nSimulation < nSimulationRepeat; nSimulation++){
        // generate a different seed for each simulation 
        srand(time(0));
        int seed = rand();
        RngSeedManager::SetSeed (seed); 
        RngSeedManager::SetRun (7); 
 
        nDevices += nDevices_without_dataset;
        // Multiplicado por 2, pois são 2 aplicações sem dataset e 
        // cada uma delas terá a qtd de nodes setada pela flag 'nDevices_without_dataset'. As aplicações sao:
        //    - Monitoramento de ar
        //    - Indoor e outdoor application

        cout << "\n[INFO]: Simulation Seed: " << seed;  
        cout << "\n[INFO]: Loading node datasets... ";  
        cout << "\n[INFO] unicamp_battery_bins_dataset: " << unicamp_battery_bins_dataset.size() << endl;
        cout << "[INFO] unicamp_conteiner_bins_dataset: " << unicamp_conteiner_bins_dataset.size() << endl;
        cout << "[INFO] unicamp_smart_meter_dataset: " << unicamp_smart_meters_dataset.size() << endl;
        cout << "[INFO] number of devices without dataset: " << nDevices_without_dataset << endl;
        cout << "[INFO] Total number of devices: " << nDevices << endl;

        initialize_structs(); // Structs Inicialization

        Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();
        Ptr<LoraChannel> channel = CreateObject<LoraChannel> (propagation_model, delay);        
        LoraPacketTracker& tracker = runSimulation(nDevices, (int)nDevices_without_dataset/2, nodePositionAllocator, channel); // run simulation
        
        getSimulationResults(tracker); // calculate results
        
        // --------- Clean all values ---------
        // clean structs
        cleaning_structs(); 

        // clean Tracker
        tracker.~LoraPacketTracker();

        // clean variables
        count_send_pkts = 0;
        count_receiv_pkts = 0;
        nDevices = 0;
        nDevices_without_dataset = nDevices_without_dataset;

        // Load datasets Again
        read_battery_bin_dataset(nodes_battery_dataset); 
        read_conteiner_bin_dataset(nodes_conteiner_dataset);
        read_smart_meter_dataset(nodes_smart_meter_dataset);
        nDevices = unicamp_battery_bins_dataset.size() + unicamp_conteiner_bins_dataset.size() + unicamp_smart_meters_dataset.size();
      }

      return 0;
}

