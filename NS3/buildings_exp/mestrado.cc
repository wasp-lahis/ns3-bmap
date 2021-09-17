/* This script simulates a complex scenario with multiple GWs, EDs and buildings.
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
 */

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
#include "ns3/building-penetration-loss.h"
#include "ns3/building-allocator.h"
#include "ns3/buildings-helper.h"

#include "ns3/mobility-module.h"

using namespace ns3;
using namespace lorawan;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("Cenario I");

// -------- Setup Variables and Structures --------

struct device{
    int SF;
};

struct spf{
    int S;
    int R;
    Time delay;
};

// Instantiate of data structures
uint8_t SF_QTD = 6;
vector<device> deviceList;
vector<spf> spreadFList;
map <uint64_t, int> pacote_sf;
map <uint64_t, Time> pacote_ds; // send
map <uint64_t, Time> pacote_dr; // receive
vector<double> distances;

// Network settings
int nDevices = 50;
int nGateways = 1;
double radius = 500;    // Radio coverage
int payloadSize = 51;   // bytes
int appPeriodSeconds = 120; // seconds
double regionalFrequency = 868e6; // frequency band EU

// Output control
bool print = true;

// Simulation settings
int simulationTime = 24; // hours
int nSimulationRepeat = 1;

// Channel model
bool realisticChannelModel = true;

// Output file names
string building_file = "buildings_dimensions.txt";
string network_file = "network_results.txt";


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

// Simulation Code
void simulationCode(int nSimulation){

  
  // /* Debug
  //Chamadas de funções da classe com endereços da memória dos componentes, tamanho do pacote e alguns tempos computados (?)
  //LogComponentEnable("LoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraChannel", LOG_LEVEL_INFO); // tx, rx info
  //Só chamada de funções da classe EndDevice
  //LogComponentEnable("EndDeviceLoraPhy", LOG_LEVEL_ALL);
  //Chamadas de funções com endereços da memórias como parâmetros;
  //LogComponentEnable("GatewayLoraPhy", LOG_LEVEL_ALL);
  //Avisa se encontrou interferência, em qual canal, a energia, bem como intervalo de tempo
  //LogComponentEnable("LoraInterferenceHelper", LOG_LEVEL_ALL);
  //Chamada de função com endereço e algum ID (?)
  //LogComponentEnable("LorawanMac", LOG_LEVEL_ALL);
  //Chamada de funções da classe + frequência do canal atual e tempo de espera pra tal canal
  //LogComponentEnable("EndDeviceLorawanMac", LOG_LEVEL_ALL);
  //Chama da funções da classe + frequência, quanto tipos de transmissões faltam, se é mensagem confirmada, (replyDataRate, m_rx1DrOffset e m_dataRate (?)) 
  //LogComponentEnable("ClassAEndDeviceLorawanMac", LOG_LEVEL_ALL);
  //Chamada de funções da classe com endereços da memória como parâmetros
  //LogComponentEnable("GatewayLorawanMac", LOG_LEVEL_ALL);
  //Chamada de funções da classe com endereços da memória, tempo da mensagem no ar, m_aggregatedDutyCycle(?), tempo atual e quando a próxima transmissão vai ser permitida na sub-banda; 
  //LogComponentEnable("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
  //Chamada de funções da classe com endereços da memória
  //LogComponentEnable("LogicalLoraChannel", LOG_LEVEL_ALL);
  //Avisa quando cria as camadas PHY e MAC, bem como cada dispositivo e gateway e suas posições
  //LogComponentEnable("LoraHelper", LOG_LEVEL_ALL);
  //Chamada de funções da classe com endereços da memória
  //LogComponentEnable("LoraPhyHelper", LOG_LEVEL_ALL);
  //Chamada de funções da classe com endereços da memória
  //LogComponentEnable("LorawanMacHelper", LOG_LEVEL_ALL);
  //Chamada de funções da classe com endereços da memória e o intervalo de tempo da aplicação criada
  //LogComponentEnable("PeriodicSenderHelper", LOG_LEVEL_ALL);
  //Chamada de funções da classe com endereços da memória, avisa do id do dispositivo (evento) e que enviou o pacote
  //LogComponentEnable("PeriodicSender", LOG_LEVEL_ALL);
  //Chamada de funções da classe
  //LogComponentEnable("LorawanMacHeader", LOG_LEVEL_ALL);
  //Chamada de funções da classe e algumas variáveis (ACK, fpending, foptslen, fcnt)
  //LogComponentEnable("LoraFrameHeader", LOG_LEVEL_ALL);
  //Chamada de funções da classe com endereços da memória, avisa a abertura de janela do dispositivo e se encontrou gateway disponível 
  //LogComponentEnable("NetworkScheduler", LOG_LEVEL_ALL);
  //id do dispositivo + propagação: txPower (transmissor), rxPower (receptor), distância acho q do gateway, delay da mensagem
  //LogComponentEnable ("LoraChannel", LOG_LEVEL_INFO);
  //Funções da classe e endereços dos componentes passados como parâmetros
  //LogComponentEnable("NetworkServer", LOG_LEVEL_ALL);
  //Chamada de funções da classe com endereços da memória
  //LogComponentEnable("NetworkStatus", LOG_LEVEL_ALL);
  //Chamada de funções da classe com endereços da memória
  //LogComponentEnable("NetworkController", LOG_LEVEL_ALL);
  //Pega endereço de pacote e mostra de qual dispositivo foi enviado e se foi recebido pelo gateway
  //LogComponentEnable("LoraPacketTracker", LOG_LEVEL_ALL);
  //*/

  // Inicialization
  deviceList.resize(nDevices);
  spreadFList.resize(SF_QTD);
  distances.resize(SF_QTD);
      
  // Create Propagation Loss
  Ptr<NakagamiPropagationLossModel> nakagami = CreateObject<NakagamiPropagationLossModel>();
  nakagami->SetAttribute("m0", DoubleValue(1));
  nakagami->SetAttribute("m1",DoubleValue(1));
  nakagami->SetAttribute("m2",DoubleValue(1));
  Ptr<OkumuraHataPropagationLossModel> loss = CreateObject<OkumuraHataPropagationLossModel>();
  loss->SetAttribute("Frequency", DoubleValue(regionalFrequency));
  loss->SetNext(nakagami);
  loss->Initialize();

  if (realisticChannelModel){
    // Create the correlated shadowing component
    Ptr<CorrelatedShadowingPropagationLossModel> shadowing =
        CreateObject<CorrelatedShadowingPropagationLossModel> ();

    // Aggregate shadowing to the logdistance loss
    loss->SetNext (shadowing);

    // Add the effect to the channel propagation loss
    Ptr<BuildingPenetrationLoss> buildingLoss = CreateObject<BuildingPenetrationLoss> ();

    shadowing->SetNext (buildingLoss);
  }

  //Create channel
  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();
  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (loss, delay);

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
  std::string mobility_file = "ns2mobility.tcl";
  
  // using built-in ns-2 mobility helper
  // Ns2MobilityHelper sumo_trace(mobility_file);

  // random EDs positions distributions
  // see https://www.nsnam.org/wiki/MobilityHelper
  mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", "rho", DoubleValue (radius),
                                      "X", DoubleValue (0.0), "Y", DoubleValue (0.0));

  // especific positions distributions
  // Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  // for(uint16_t i = 0; i < nDevices; i++){
    
    
  //   allocator->Add (Vector (0 + i*4, 0 + i*2, 0));

  // }
  // mobility.SetPositionAllocator(allocator);

  // ED does not move
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  
  //EndDevices
  NodeContainer endDevices;
  endDevices.Create (nDevices);
     
  mobility.Install (endDevices);
  
  // Make it so that nodes are at a certain height > 0
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j){
      Ptr<MobilityModel> m = (*j)->GetObject<MobilityModel> ();
      Vector position = m->GetPosition ();
      position.z = 1.0;
      //  cout << position <<   endl;
      m->SetPosition (position);
  }
      
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LorawanMacHelper::ED_A);
  helper.Install (phyHelper, macHelper, endDevices);
  
  //Gateway
  NodeContainer gateways;
  gateways.Create (nGateways);
  Ptr<ListPositionAllocator> positionAllocGw = CreateObject<ListPositionAllocator> ();
  positionAllocGw->Add (Vector (0.0, 0.0, 50.0));
  mobility.SetPositionAllocator (positionAllocGw);
  mobility.Install(gateways);

  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LorawanMacHelper::GW);
  macHelper.SetRegion (LorawanMacHelper::EU);
  helper.Install (phyHelper, macHelper, gateways);

  /**********************
   *  Handle buildings  *
   **********************/

  double xLength = 130; // tamanho do predio no eixo X
  double deltaX = 32; // distancia entre predios no eixo X
  double yLength = 64; // tamanho do predio no eixo Y
  double deltaY = 17; // distancia entre predios no eixo Y
  int gridWidth = 2 * radius / (xLength + deltaX); // n de colunas de predios
  int gridHeight = 2 * radius / (yLength + deltaY); // n de linhas de predios
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
  if (print){
    ofstream myfile;
    myfile.open (building_file);
    vector<Ptr<Building>>::const_iterator it;

    int j = 1;
    for (it = bContainer.Begin (); it != bContainer.End (); ++it, ++j){
        Box boundaries = (*it)->GetBoundaries ();
        
        // print building boundaries
        // cout << "boundaries:" << boundaries << "\n" 
        // << "x:" << boundaries.xMin << "," << boundaries.xMax << "\n" 
        // << "y:" << boundaries.yMin << "," << boundaries.yMax << "\n" 
        // << "z:" << boundaries.zMin << "," << boundaries.zMax << endl;        
        
        // write building boundaries in a file
        myfile << j << "," << boundaries.xMin << "," << boundaries.yMin
                << "," << boundaries.xMax << "," << boundaries.yMax << "," 
                << boundaries.zMin << "," << boundaries.zMax << endl;
    }
    myfile.close ();
  }
      
  //spreadFListingFactor
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
  Time appStopTime = Hours (simulationTime);
  PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
  // appHelper.SetPeriod (Hours (appPeriodSeconds));
  appHelper.SetPeriod (Seconds (appPeriodSeconds));//
  appHelper.SetPacketSize (payloadSize);
  Ptr<RandomVariableStream> rv = CreateObjectWithAttributes<UniformRandomVariable> (
      "Min", DoubleValue (0), "Max", DoubleValue (10));
  ApplicationContainer appContainer = appHelper.Install (endDevices);

  // Start simulation
  Simulator::Stop (appStopTime);
  Simulator::Schedule(Seconds(0.00), &Print, endDevices, gateways, delay, 10.0);

  Simulator::Run ();
  Simulator::Destroy ();

  // Pacotes enviados e recebidos
  LoraPacketTracker &tracker = helper.GetPacketTracker ();
  // helper.DoPrintDeviceStatus(endDevices, gateways, "resultados.txt");
  int iterator = nDevices;

    cout << "\n- Evaluate the performance at PHY level of a specific gateway: \n";
  for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); ++j){
      vector <int> output = tracker.CountPhyPacketsPerGw(Seconds(0), appStopTime, iterator);
      cout << "GwID " << iterator << "\nReceived: " << output.at(1) << "\nInterfered: " << output.at(2)
      << "\nNoMoreReceivers: " << output.at(3) << "\nUnderSensitivity: " << output.at(4) << "\nLost: " << output.at(5)
      << "\n" << "\n";
      iterator += 1;
  }

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


int main (int argc, char *argv[]){

  CommandLine cmd;
  cmd.AddValue ("nDevices", "Number of end devices to include in the simulation", nDevices);
  cmd.AddValue ("radius", "The radius of the area to simulate", radius);
  cmd.AddValue ("simulationTime", "The time for which to simulate", simulationTime);
  cmd.AddValue ("appPeriod",
                "The period in seconds to be used by periodically transmitting applications",
                appPeriodSeconds);
  cmd.AddValue ("print", "Whether or not to print various informations", print);
  cmd.AddValue ("nSimulationRepeat", "Number of times to run the simulation", nSimulationRepeat);
  cmd.Parse (argc, argv);

   // Set up logging
  LogComponentEnable ("Cenario I", LOG_LEVEL_ALL);

  for(int n = 0; n < nSimulationRepeat; n++){
      
      // generate a different seed for each simulation 
      srand(time(0));
      int seed = rand();
      RngSeedManager::SetSeed (seed); // Changes seed from default of 1 to seed
      RngSeedManager::SetRun (7); // Changes run number from default of 1 to 7
      
      // all simulation network code 
      simulationCode(n);

      // TODO: obtain and calculated metrics
      // TODO: generate a csv with results
  }


  return 0;
}