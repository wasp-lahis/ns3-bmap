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
 */

#include "ns3/lorawan-mac-helper.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/lora-net-device.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("LorawanMacHelper");

LorawanMacHelper::LorawanMacHelper () : m_region (LorawanMacHelper::EU)
{
}

void
LorawanMacHelper::Set (std::string name, const AttributeValue &v)
{
  m_mac.Set (name, v);
}

void
LorawanMacHelper::SetDeviceType (enum DeviceType dt)
{
  NS_LOG_FUNCTION (this << dt);
  switch (dt)
    {
    case GW:
      m_mac.SetTypeId ("ns3::GatewayLorawanMac");
      break;
    case ED_A:
      m_mac.SetTypeId ("ns3::ClassAEndDeviceLorawanMac");
      break;
    }
  m_deviceType = dt;
}

void
LorawanMacHelper::SetAddressGenerator (Ptr<LoraDeviceAddressGenerator> addrGen)
{
  NS_LOG_FUNCTION (this);

  m_addrGen = addrGen;
}

void
LorawanMacHelper::SetRegion (enum LorawanMacHelper::Regions region)
{
  m_region = region;
}

Ptr<LorawanMac>
LorawanMacHelper::Create (Ptr<Node> node, Ptr<NetDevice> device) const
{
  Ptr<LorawanMac> mac = m_mac.Create<LorawanMac> ();
  mac->SetDevice (device);

  // If we are operating on an end device, add an address to it
  if (m_deviceType == ED_A && m_addrGen != 0)
    {
      mac->GetObject<ClassAEndDeviceLorawanMac> ()->SetDeviceAddress (m_addrGen->NextAddress ());
    }

  // Add a basic list of channels based on the region where the device is
  // operating
  if (m_deviceType == ED_A)
    {
      Ptr<ClassAEndDeviceLorawanMac> edMac = mac->GetObject<ClassAEndDeviceLorawanMac> ();
      switch (m_region)
        {
          case LorawanMacHelper::EU: {
            ConfigureForEuRegion (edMac);
            break;
          }
          case LorawanMacHelper::SingleChannel: {
            ConfigureForSingleChannelRegion (edMac);
            break;
          }
          case LorawanMacHelper::ALOHA: {
            ConfigureForAlohaRegion (edMac);
            break;
          }
          case LorawanMacHelper::Australia: {
            // std:: cout << "!!ed Austrália!!!\n";
            ConfigureForAuRegion (edMac);
            break;
          }          
          default: {
            NS_LOG_ERROR ("This region isn't supported yet!");
            break;
          }
        }
    }
  else
    {
      Ptr<GatewayLorawanMac> gwMac = mac->GetObject<GatewayLorawanMac> ();
      switch (m_region)
        {
          case LorawanMacHelper::EU: {
            ConfigureForEuRegion (gwMac);
            break;
          }
          case LorawanMacHelper::SingleChannel: {
            ConfigureForSingleChannelRegion (gwMac);
            break;
          }
          case LorawanMacHelper::ALOHA: {
            ConfigureForAlohaRegion (gwMac);
            break;
          }
          case LorawanMacHelper::Australia: {
            // std:: cout << "!!gw Austrália!!!\n";
            ConfigureForAuRegion (gwMac);
            break;
          }
          default: {
            NS_LOG_ERROR ("This region isn't supported yet!");
            break;
          }
        }
    }
  return mac;
}

void
LorawanMacHelper::ConfigureForAlohaRegion (Ptr<ClassAEndDeviceLorawanMac> edMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  ApplyCommonAlohaConfigurations (edMac);

  /////////////////////////////////////////////////////
  // TxPower -> Transmission power in dBm conversion //
  /////////////////////////////////////////////////////
  edMac->SetTxDbmForTxPower (std::vector<double>{16, 14, 12, 10, 8, 6, 4, 2});

  ////////////////////////////////////////////////////////////
  // Matrix to know which DataRate the GW will respond with //
  ////////////////////////////////////////////////////////////
  LorawanMac::ReplyDataRateMatrix matrix = {{{{0, 0, 0, 0, 0, 0}},
                                             {{1, 0, 0, 0, 0, 0}},
                                             {{2, 1, 0, 0, 0, 0}},
                                             {{3, 2, 1, 0, 0, 0}},
                                             {{4, 3, 2, 1, 0, 0}},
                                             {{5, 4, 3, 2, 1, 0}},
                                             {{6, 5, 4, 3, 2, 1}},
                                             {{7, 6, 5, 4, 3, 2}}}};
  edMac->SetReplyDataRateMatrix (matrix);

  /////////////////////
  // Preamble length //
  /////////////////////
  edMac->SetNPreambleSymbols (8);

  //////////////////////////////////////
  // Second receive window parameters //
  //////////////////////////////////////
  edMac->SetSecondReceiveWindowDataRate (0);
  edMac->SetSecondReceiveWindowFrequency (869.525);
}

void
LorawanMacHelper::ConfigureForAlohaRegion (Ptr<GatewayLorawanMac> gwMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  ///////////////////////////////
  // ReceivePath configuration //
  ///////////////////////////////
  Ptr<GatewayLoraPhy> gwPhy =
      gwMac->GetDevice ()->GetObject<LoraNetDevice> ()->GetPhy ()->GetObject<GatewayLoraPhy> ();

  ApplyCommonAlohaConfigurations (gwMac);

  if (gwPhy) // If cast is successful, there's a GatewayLoraPhy
    {
      NS_LOG_DEBUG ("Resetting reception paths");
      gwPhy->ResetReceptionPaths ();

      int receptionPaths = 0;
      int maxReceptionPaths = 1;
      while (receptionPaths < maxReceptionPaths)
        {
          gwPhy->GetObject<GatewayLoraPhy> ()->AddReceptionPath ();
          receptionPaths++;
        }
      gwPhy->AddFrequency (868.1);
    }
}

void
LorawanMacHelper::ApplyCommonAlohaConfigurations (Ptr<LorawanMac> lorawanMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  //////////////
  // SubBands //
  //////////////

  LogicalLoraChannelHelper channelHelper;
  channelHelper.AddSubBand (868, 868.6, 1, 14); 

  //////////////////////
  // Default channels //
  //////////////////////
  Ptr<LogicalLoraChannel> lc1 = CreateObject<LogicalLoraChannel> (868.1, 0, 5);
  channelHelper.AddChannel (lc1);

  lorawanMac->SetLogicalLoraChannelHelper (channelHelper);

  ///////////////////////////////////////////////
  // DataRate -> SF, DataRate -> Bandwidth     //
  // and DataRate -> MaxAppPayload conversions //
  ///////////////////////////////////////////////
  lorawanMac->SetSfForDataRate (std::vector<uint8_t>{12, 11, 10, 9, 8, 7, 7});
  lorawanMac->SetBandwidthForDataRate (
      std::vector<double>{125000, 125000, 125000, 125000, 125000, 125000, 250000});
  lorawanMac->SetMaxAppPayloadForDataRate (
      std::vector<uint32_t>{59, 59, 59, 123, 230, 230, 230, 230});
}

void
LorawanMacHelper::ConfigureForEuRegion (Ptr<ClassAEndDeviceLorawanMac> edMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  ApplyCommonEuConfigurations (edMac);

  /////////////////////////////////////////////////////
  // TxPower -> Transmission power in dBm conversion //
  /////////////////////////////////////////////////////
  edMac->SetTxDbmForTxPower (std::vector<double>{16, 14, 12, 10, 8, 6, 4, 2});

  ////////////////////////////////////////////////////////////
  // Matrix to know which DataRate the GW will respond with //
  ////////////////////////////////////////////////////////////
  LorawanMac::ReplyDataRateMatrix matrix = {{{{0, 0, 0, 0, 0, 0}},
                                             {{1, 0, 0, 0, 0, 0}},
                                             {{2, 1, 0, 0, 0, 0}},
                                             {{3, 2, 1, 0, 0, 0}},
                                             {{4, 3, 2, 1, 0, 0}},
                                             {{5, 4, 3, 2, 1, 0}},
                                             {{6, 5, 4, 3, 2, 1}},
                                             {{7, 6, 5, 4, 3, 2}}}};
  edMac->SetReplyDataRateMatrix (matrix);

  /////////////////////
  // Preamble length //
  /////////////////////
  edMac->SetNPreambleSymbols (8);

  //////////////////////////////////////
  // Second receive window parameters //
  //////////////////////////////////////
  edMac->SetSecondReceiveWindowDataRate (0);
  edMac->SetSecondReceiveWindowFrequency (869.525);
}

void
LorawanMacHelper::ConfigureForEuRegion (Ptr<GatewayLorawanMac> gwMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  ///////////////////////////////
  // ReceivePath configuration //
  ///////////////////////////////
  Ptr<GatewayLoraPhy> gwPhy =
      gwMac->GetDevice ()->GetObject<LoraNetDevice> ()->GetPhy ()->GetObject<GatewayLoraPhy> ();

  ApplyCommonEuConfigurations (gwMac);

  if (gwPhy) // If cast is successful, there's a GatewayLoraPhy
    {
      NS_LOG_DEBUG ("Resetting reception paths");
      gwPhy->ResetReceptionPaths ();

      std::vector<double> frequencies;
      frequencies.push_back (868.1);
      frequencies.push_back (868.3);
      frequencies.push_back (868.5);

      for (auto &f : frequencies)
        {
          gwPhy->AddFrequency (f);
        }

      int receptionPaths = 0;
      int maxReceptionPaths = 8;
      while (receptionPaths < maxReceptionPaths)
        {
          gwPhy->GetObject<GatewayLoraPhy> ()->AddReceptionPath ();
          receptionPaths++;
        }
    }
}

void
LorawanMacHelper::ApplyCommonEuConfigurations (Ptr<LorawanMac> lorawanMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  //////////////
  // SubBands //
  //////////////

  LogicalLoraChannelHelper channelHelper;
  channelHelper.AddSubBand (868, 868.6, 0.01, 20);  // Lahis 14->20
  channelHelper.AddSubBand (868.7, 869.2, 0.001, 20); // Lahis 14->20
  channelHelper.AddSubBand (869.4, 869.65, 0.1, 27);

  //////////////////////
  // Default channels //
  //////////////////////
  Ptr<LogicalLoraChannel> lc1 = CreateObject<LogicalLoraChannel> (868.1, 0, 5);
  Ptr<LogicalLoraChannel> lc2 = CreateObject<LogicalLoraChannel> (868.3, 0, 5);
  Ptr<LogicalLoraChannel> lc3 = CreateObject<LogicalLoraChannel> (868.5, 0, 5);
  channelHelper.AddChannel (lc1);
  channelHelper.AddChannel (lc2);
  channelHelper.AddChannel (lc3);

  lorawanMac->SetLogicalLoraChannelHelper (channelHelper);

  ///////////////////////////////////////////////
  // DataRate -> SF, DataRate -> Bandwidth     //
  // and DataRate -> MaxAppPayload conversions //
  ///////////////////////////////////////////////
  lorawanMac->SetSfForDataRate (std::vector<uint8_t>{12, 11, 10, 9, 8, 7, 7});
  lorawanMac->SetBandwidthForDataRate (
      std::vector<double>{125000, 125000, 125000, 125000, 125000, 125000, 250000});
  lorawanMac->SetMaxAppPayloadForDataRate (
      std::vector<uint32_t>{59, 59, 59, 123, 230, 230, 230, 230});
}

///////////////////////////////

void
LorawanMacHelper::ConfigureForSingleChannelRegion (Ptr<ClassAEndDeviceLorawanMac> edMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  ApplyCommonSingleChannelConfigurations (edMac);

  /////////////////////////////////////////////////////
  // TxPower -> Transmission power in dBm conversion //
  /////////////////////////////////////////////////////
  edMac->SetTxDbmForTxPower (std::vector<double>{16, 14, 12, 10, 8, 6, 4, 2});

  ////////////////////////////////////////////////////////////
  // Matrix to know which DataRate the GW will respond with //
  ////////////////////////////////////////////////////////////
  LorawanMac::ReplyDataRateMatrix matrix = {{{{0, 0, 0, 0, 0, 0}},
                                             {{1, 0, 0, 0, 0, 0}},
                                             {{2, 1, 0, 0, 0, 0}},
                                             {{3, 2, 1, 0, 0, 0}},
                                             {{4, 3, 2, 1, 0, 0}},
                                             {{5, 4, 3, 2, 1, 0}},
                                             {{6, 5, 4, 3, 2, 1}},
                                             {{7, 6, 5, 4, 3, 2}}}};
  edMac->SetReplyDataRateMatrix (matrix);

  /////////////////////
  // Preamble length //
  /////////////////////
  edMac->SetNPreambleSymbols (8);

  //////////////////////////////////////
  // Second receive window parameters //
  //////////////////////////////////////
  edMac->SetSecondReceiveWindowDataRate (0);
  edMac->SetSecondReceiveWindowFrequency (869.525);
}

void
LorawanMacHelper::ConfigureForSingleChannelRegion (Ptr<GatewayLorawanMac> gwMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  ///////////////////////////////
  // ReceivePath configuration //
  ///////////////////////////////
  Ptr<GatewayLoraPhy> gwPhy =
      gwMac->GetDevice ()->GetObject<LoraNetDevice> ()->GetPhy ()->GetObject<GatewayLoraPhy> ();

  ApplyCommonEuConfigurations (gwMac);

  if (gwPhy) // If cast is successful, there's a GatewayLoraPhy
    {
      NS_LOG_DEBUG ("Resetting reception paths");
      gwPhy->ResetReceptionPaths ();

      std::vector<double> frequencies;
      frequencies.push_back (868.1);

      for (auto &f : frequencies)
        {
          gwPhy->AddFrequency (f);
        }

      int receptionPaths = 0;
      int maxReceptionPaths = 8;
      while (receptionPaths < maxReceptionPaths)
        {
          gwPhy->GetObject<GatewayLoraPhy> ()->AddReceptionPath ();
          receptionPaths++;
        }
    }
}

void
LorawanMacHelper::ApplyCommonSingleChannelConfigurations (Ptr<LorawanMac> lorawanMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  //////////////
  // SubBands //
  //////////////

  LogicalLoraChannelHelper channelHelper;
  channelHelper.AddSubBand (868, 868.6, 0.01, 20); // Lahis14 ->20
  channelHelper.AddSubBand (868.7, 869.2, 0.001, 20);
  channelHelper.AddSubBand (869.4, 869.65, 0.1, 27);

  //////////////////////
  // Default channels //
  //////////////////////
  Ptr<LogicalLoraChannel> lc1 = CreateObject<LogicalLoraChannel> (868.1, 0, 5);
  channelHelper.AddChannel (lc1);

  lorawanMac->SetLogicalLoraChannelHelper (channelHelper);

  ///////////////////////////////////////////////
  // DataRate -> SF, DataRate -> Bandwidth     //
  // and DataRate -> MaxAppPayload conversions //
  ///////////////////////////////////////////////
  lorawanMac->SetSfForDataRate (std::vector<uint8_t>{12, 11, 10, 9, 8, 7, 7});
  lorawanMac->SetBandwidthForDataRate (
      std::vector<double>{125000, 125000, 125000, 125000, 125000, 125000, 250000});
  lorawanMac->SetMaxAppPayloadForDataRate (
      std::vector<uint32_t>{59, 59, 59, 123, 230, 230, 230, 230});
}


void
LorawanMacHelper::ConfigureForAuRegion (Ptr<ClassAEndDeviceLorawanMac> edMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  ApplyCommonAuConfigurations (edMac);

  /////////////////////////////////////////////////////
  // TxPower -> Transmission power in dBm conversion //
  /////////////////////////////////////////////////////
  // Values based on:
  // RP002-1.0.3 LoRaWAN® Regional Parameters 2021
  // Table 43 : AU915-928 TX power table
  edMac->SetTxDbmForTxPower (std::vector<double>{30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2});

  ////////////////////////////////////////////////////////////
  // Matrix to know which DataRate the GW will respond with //
  ////////////////////////////////////////////////////////////
  // Values based on:
  // RP002-1.0.3 LoRaWAN® Regional Parameters 2021
  // Table 47 : AU915-928 downlink RX1 data rate mapping
  LorawanMac::ReplyDataRateMatrix matrix = {{{{8,  8,  8,  8,  8,  8}},
                                             {{9,  8,  8,  8,  8,  8}},
                                             {{10, 9,  8,  8,  8,  8}},
                                             {{11, 10, 9,  8,  8,  8}},
                                             {{12, 11, 10, 9,  8,  8}},
                                             {{13, 12, 11, 10, 9,  8}},
                                             {{13, 13, 12, 11, 10, 9}},
                                             {{9,  8,  8,  8,  8,  8}}}};
  edMac->SetReplyDataRateMatrix (matrix);

  /////////////////////
  // Preamble length //
  /////////////////////
  // Values based on:
  // RP002-1.0.3 LoRaWAN® Regional Parameters 2021
  //  4.1.2 LoRa settings
  edMac->SetNPreambleSymbols (8);

  //////////////////////////////////////
  // Second receive window parameters //
  //////////////////////////////////////
  // Values based on:
  // RP002-1.0.3 LoRaWAN® Regional Parameters 2021
  // 2.8.7 AU915-928 Receive windows
  edMac->SetSecondReceiveWindowDataRate (8);
  edMac->SetSecondReceiveWindowFrequency (923.3);
}

void
LorawanMacHelper::ConfigureForAuRegion (Ptr<GatewayLorawanMac> gwMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  ///////////////////////////////
  // ReceivePath configuration //
  ///////////////////////////////
  Ptr<GatewayLoraPhy> gwPhy =
      gwMac->GetDevice ()->GetObject<LoraNetDevice> ()->GetPhy ()->GetObject<GatewayLoraPhy> ();

  ApplyCommonAuConfigurations (gwMac);

  if (gwPhy) // If cast is successful, there's a GatewayLoraPhy
    {
      NS_LOG_DEBUG ("Resetting reception paths");
      gwPhy->ResetReceptionPaths ();

      std::vector<double> frequencies;

      // Values based on:
      // RP002-1.0.3 LoRaWAN® Regional Parameters 2021
      // 2.8.2 AU915-928 Band Channel Frequencies
      // for (double gwch0_63 = 915.2; gwch0_63<=927.8+0.2; gwch0_63+=0.2){
      //   frequencies.push_back (gwch0_63);
      // }
      for (double gwch64_71 = 915.9; gwch64_71<=927.1+1.6; gwch64_71+=1.6){
        frequencies.push_back (gwch64_71);
      }


      for (auto &f : frequencies)
        {
          gwPhy->AddFrequency (f);
        }

      int receptionPaths = 0;
      int maxReceptionPaths = 8;
      while (receptionPaths < maxReceptionPaths)
        {
          gwPhy->GetObject<GatewayLoraPhy> ()->AddReceptionPath ();
          receptionPaths++;
        }
    }
}

void
LorawanMacHelper::ApplyCommonAuConfigurations (Ptr<LorawanMac> lorawanMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  //////////////
  // SubBands //
  //////////////

  LogicalLoraChannelHelper channelHelper;
  channelHelper.AddSubBand (915, 928, 1, 30);

  //////////////////////
  // Default channels //
  //////////////////////
  // Values based on:
  // RP002-1.0.3 LoRaWAN® Regional Parameters 2021
  // 2.8.2 AU915-928 Band Channel Frequencies
  // for (double gwch0_63 = 915.2; gwch0_63<=927.8+0.2; gwch0_63+=0.2){
  //   Ptr<LogicalLoraChannel> lc1 = CreateObject<LogicalLoraChannel> (gwch0_63, 0, 5);
  //   channelHelper.AddChannel (lc1);
  // }
  for (double gwch64_71 = 915.9; gwch64_71<=927.1+1.6; gwch64_71+=1.6){
    Ptr<LogicalLoraChannel> lc1 = CreateObject<LogicalLoraChannel> (gwch64_71, 6, 6);
    channelHelper.AddChannel (lc1);
  }
 
  
  lorawanMac->SetLogicalLoraChannelHelper (channelHelper);

  ///////////////////////////////////////////////
  // DataRate -> SF, DataRate -> Bandwidth     //
  // and DataRate -> MaxAppPayload conversions //
  ///////////////////////////////////////////////
  // Values based on:
  // RP002-1.0.3 LoRaWAN® Regional Parameters 2021
  // Table 45: AU915-928 maximum payload size (repeater compatible)
  lorawanMac->SetSfForDataRate (std::vector<uint8_t>{12, 11, 10, 9, 8, 7, 8, 0, 12, 11, 10, 9, 8, 7});
  lorawanMac->SetBandwidthForDataRate (
      std::vector<double>{125000, 125000, 125000, 125000, 125000, 125000, 500000, 0, 500000, 500000, 500000, 500000, 500000, 500000 });
  lorawanMac->SetMaxAppPayloadForDataRate (
      std::vector<uint32_t>{59, 59, 59, 123, 230, 230, 230, 58, 61, 137, 230, 230, 230, 230});
}

///////////////////////////////


std::vector<int>
LorawanMacHelper::SetSpreadingFactorsUp (NodeContainer endDevices, NodeContainer gateways,
                                         Ptr<LoraChannel> channel)
{
  NS_LOG_FUNCTION_NOARGS ();

  std::vector<int> sfQuantity (7, 0);
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<ClassAEndDeviceLorawanMac> mac =
          loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();
      NS_ASSERT (mac != 0);

      // Try computing the distance from each gateway and find the best one
      Ptr<Node> bestGateway = gateways.Get (0);
      Ptr<MobilityModel> bestGatewayPosition = bestGateway->GetObject<MobilityModel> ();

      // Assume devices transmit at 14 dBm
      double highestRxPower = channel->GetRxPower (20, position, bestGatewayPosition); // Lahis 14->20

      for (NodeContainer::Iterator currentGw = gateways.Begin () + 1; currentGw != gateways.End ();
           ++currentGw)
        {
          // Compute the power received from the current gateway
          Ptr<Node> curr = *currentGw;
          Ptr<MobilityModel> currPosition = curr->GetObject<MobilityModel> ();
          double currentRxPower = channel->GetRxPower (20, position, currPosition); // Lahis dBm 14->20

          if (currentRxPower > highestRxPower)
            {
              bestGateway = curr;
              bestGatewayPosition = curr->GetObject<MobilityModel> ();
              highestRxPower = currentRxPower;
            }
        }

      // NS_LOG_DEBUG ("Rx Power: " << highestRxPower);
      double rxPower = highestRxPower;

      // Get the ED sensitivity
      Ptr<EndDeviceLoraPhy> edPhy = loraNetDevice->GetPhy ()->GetObject<EndDeviceLoraPhy> ();
      const double *edSensitivity = edPhy->sensitivity;

      if (rxPower > *edSensitivity)
        {
          mac->SetDataRate (5);
          sfQuantity[0] = sfQuantity[0] + 1;
        }
      else if (rxPower > *(edSensitivity + 1))
        {
          mac->SetDataRate (4);
          sfQuantity[1] = sfQuantity[1] + 1;
        }
      else if (rxPower > *(edSensitivity + 2))
        {
          mac->SetDataRate (3);
          sfQuantity[2] = sfQuantity[2] + 1;
        }
      else if (rxPower > *(edSensitivity + 3))
        {
          mac->SetDataRate (2);
          sfQuantity[3] = sfQuantity[3] + 1;
        }
      else if (rxPower > *(edSensitivity + 4))
        {
          mac->SetDataRate (1);
          sfQuantity[4] = sfQuantity[4] + 1;
        }
      else if (rxPower > *(edSensitivity + 5))
        {
          mac->SetDataRate (0);
          sfQuantity[5] = sfQuantity[5] + 1;
        }
      else // Device is out of range. Assign SF12.
        {
          // NS_LOG_DEBUG ("Device out of range");
          mac->SetDataRate (0);
          sfQuantity[6] = sfQuantity[6] + 1;
          // NS_LOG_DEBUG ("sfQuantity[6] = " << sfQuantity[6]);
        }

      /*

      // Get the Gw sensitivity
      Ptr<NetDevice> gatewayNetDevice = bestGateway->GetDevice (0);
      Ptr<LoraNetDevice> gatewayLoraNetDevice = gatewayNetDevice->GetObject<LoraNetDevice> ();
      Ptr<GatewayLoraPhy> gatewayPhy = gatewayLoraNetDevice->GetPhy ()->GetObject<GatewayLoraPhy> ();
      const double *gwSensitivity = gatewayPhy->sensitivity;

      if(rxPower > *gwSensitivity)
        {
          mac->SetDataRate (5);
          sfQuantity[0] = sfQuantity[0] + 1;

        }
      else if (rxPower > *(gwSensitivity+1))
        {
          mac->SetDataRate (4);
          sfQuantity[1] = sfQuantity[1] + 1;

        }
      else if (rxPower > *(gwSensitivity+2))
        {
          mac->SetDataRate (3);
          sfQuantity[2] = sfQuantity[2] + 1;

        }
      else if (rxPower > *(gwSensitivity+3))
        {
          mac->SetDataRate (2);
          sfQuantity[3] = sfQuantity[3] + 1;
        }
      else if (rxPower > *(gwSensitivity+4))
        {
          mac->SetDataRate (1);
          sfQuantity[4] = sfQuantity[4] + 1;
        }
      else if (rxPower > *(gwSensitivity+5))
        {
          mac->SetDataRate (0);
          sfQuantity[5] = sfQuantity[5] + 1;

        }
      else // Device is out of range. Assign SF12.
        {
          mac->SetDataRate (0);
          sfQuantity[6] = sfQuantity[6] + 1;

        }
        */

    } // end loop on nodes

  return sfQuantity;

} //  end function

std::vector<int>
LorawanMacHelper::SetSpreadingFactorsGivenDistribution (NodeContainer endDevices,
                                                        NodeContainer gateways,
                                                        std::vector<double> distribution)
{
  NS_LOG_FUNCTION_NOARGS ();

  std::vector<int> sfQuantity (7, 0);
  Ptr<UniformRandomVariable> uniformRV = CreateObject<UniformRandomVariable> ();
  std::vector<double> cumdistr (6);
  cumdistr[0] = distribution[0];
  for (int i = 1; i < 7; ++i) {
      cumdistr[i] = distribution[i] + cumdistr[i - 1];
  }

  NS_LOG_DEBUG ("Distribution: " << distribution[0] << " " << distribution[1] << " "
                                 << distribution[2] << " " << distribution[3] << " "
                                 << distribution[4] << " " << distribution[5]);
  NS_LOG_DEBUG ("Cumulative distribution: " << cumdistr[0] << " " << cumdistr[1] << " "
                                            << cumdistr[2] << " " << cumdistr[3] << " "
                                            << cumdistr[4] << " " << cumdistr[5]);
  
  // Debug cumdistr
  // for (int i = 0; i < 7; ++i) {
  //     std::cout << "cumdistr[" << i << "]: " << cumdistr[i] << '\n';
  // }

  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<ClassAEndDeviceLorawanMac> mac =
          loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();
      NS_ASSERT (mac != 0);

      double prob = uniformRV->GetValue (0, 1);
      // Debug prob
      // std::cout << "prob:" << prob << '\n';

      // NS_LOG_DEBUG ("Probability: " << prob);
      if (prob < cumdistr[0])
        {
          mac->SetDataRate (5);
          sfQuantity[0] = sfQuantity[0] + 1;
        }
      else if (prob > cumdistr[0] && prob < cumdistr[1])
        {
          mac->SetDataRate (4);
          sfQuantity[1] = sfQuantity[1] + 1;
        }
      else if (prob > cumdistr[1] && prob < cumdistr[2])
        {
          mac->SetDataRate (3);
          sfQuantity[2] = sfQuantity[2] + 1;
        }
      else if (prob > cumdistr[2] && prob < cumdistr[3])
        {
          mac->SetDataRate (2);
          sfQuantity[3] = sfQuantity[3] + 1;
        }
      else if (prob > cumdistr[3] && prob < cumdistr[4])
        {
          mac->SetDataRate (1);
          sfQuantity[4] = sfQuantity[4] + 1;
        }
      else
        {
          // std::cout << "SF 12\n";
          mac->SetDataRate (0);
          sfQuantity[5] = sfQuantity[5] + 1;
        }

    } // end loop on nodes

  return sfQuantity;

} //  end function

std::vector<int>
LorawanMacHelper::SetSpreadingFactorsAuGivenDistribution (NodeContainer endDevices,
                                                        NodeContainer gateways,
                                                        std::vector<double> distribution)
{
  NS_LOG_FUNCTION_NOARGS ();

  std::vector<int> sfQuantity (14, 0); // DR0-DR13
  Ptr<UniformRandomVariable> uniformRV = CreateObject<UniformRandomVariable> ();
  std::vector<double> cumdistr (14);
  cumdistr[0] = distribution[0];
  for (int i = 1; i < 14; ++i)
    {
      cumdistr[i] = distribution[i] + cumdistr[i - 1];
    }

  // Debug cumdistr
  // só estou usando do SF7 - SF12
  // for (int i = 0; i < 5; ++i)
  // {
  //     std::cout << "cumdistr[" << i << "]: " << cumdistr[i] << '\n';
  // }


  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<ClassAEndDeviceLorawanMac> mac =
          loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();
      NS_ASSERT (mac != 0);

      double prob = uniformRV->GetValue (0, 1);
      
      // debug prob
      // std::cout << "prob:" << prob << '\n';

      // NS_LOG_DEBUG ("Probability: " << prob);
      if (prob < cumdistr[0])
        {
          mac->SetDataRate (5); //SF7
          sfQuantity[0] = sfQuantity[0] + 1;
        }
      else if (prob > cumdistr[0] && prob < cumdistr[1])
        {
          mac->SetDataRate (4); //SF8
          sfQuantity[1] = sfQuantity[1] + 1;
        }
      else if (prob > cumdistr[1] && prob < cumdistr[2])
        {
          mac->SetDataRate (3); //SF9
          sfQuantity[2] = sfQuantity[2] + 1;
        }
      else
        {
          mac->SetDataRate (2);//SF10
          sfQuantity[3] = sfQuantity[3] + 1;
        }
      /*else if (prob > cumdistr[3] && prob < cumdistr[4])
        {
          mac->SetDataRate (1);
          sfQuantity[4] = sfQuantity[4] + 1;
        }
      else
        {
          mac->SetDataRate (0);
          sfQuantity[5] = sfQuantity[5] + 1;
        }*/
        

    } // end loop on nodes

  return sfQuantity;

} //  end function

int
LorawanMacHelper::SetSpreadingFactorsGivenDistributionManually (NodeContainer endDevices,
                                                        NodeContainer gateways, int id, int SF)
{
  NS_LOG_FUNCTION_NOARGS ();

  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
  {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<ClassAEndDeviceLorawanMac> mac =
          loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();
      NS_ASSERT (mac != 0);

      int nodeId = object->GetId();

      if (nodeId == id)
      {
        if (SF == 5) // sf7
        {
          mac->SetDataRate (5);
          return 5;
        }
        else if (SF == 4)
          {
            mac->SetDataRate (4);
            return 4;
          }
        else if (SF == 3)
          {
            mac->SetDataRate (3);
            return 3;
          }
        else if (SF == 2)
          {
            mac->SetDataRate (2);
            return 2;
          }
        else if (SF == 1)
          {
            mac->SetDataRate (1);
            return 1;
          }
        else if (SF == 0)
          {
            mac->SetDataRate (0);
            return 0;
          }
      }
      
  } // end loop on nodes
  return -1;
} //  end function
} // namespace lorawan
} // namespace ns3