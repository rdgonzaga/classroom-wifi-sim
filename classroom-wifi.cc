/*
 * Copyright (c) 2016 SEBASTIEN DERONNE
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sebastien Deronne <sebastien.deronne@gmail.com>
 */

// Based on ns-3.48's examples/wireless/wifi-he-network.cc. Modified by NSCOM03 Group 5 for
// the crowded classroom study.
//
// One AP and nStations students on 5 GHz, using 802.11ax or 802.11ac. Each student has one
// saturated UDP (or TCP) flow. Runs one configuration and appends the results to --csv and
// --perStudentCsv.
//
// Students sit in a circle at --distance meters from the AP. If --farDistance is set, the last
// student is placed that far away instead.

#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/he-phy.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/neighbor-cache-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/uinteger.h"
#include "ns3/vht-phy.h"
#include "ns3/wifi-static-setup-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("classroom-wifi");

// open for appending, write the header if the file is new
std::ofstream
OpenCsv(const std::string& path, const std::string& header)
{
    auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent);
    }
    bool isNew = !std::filesystem::exists(path) || std::filesystem::file_size(path) == 0;
    std::ofstream file(path, std::ios::app);
    if (!file)
    {
        NS_FATAL_ERROR("Could not open " << path);
    }
    if (isNew)
    {
        file << header << '\n';
    }
    return file;
}

int
main(int argc, char* argv[])
{
    std::string standard{"ax"};
    std::string rateManager{"constant"};
    bool udp{true};
    bool downlink{true};
    Time simulationTime{"10s"};
    double distance{1.0};    // meters
    double farDistance{0.0}; // meters, 0 means no far student
    std::size_t nStations{1};
    int mcs{5};
    int channelWidth{20};  // MHz
    int guardInterval{800}; // ns
    uint32_t payloadSize =
        700; // must fit in the max TX duration when transmitting at MCS 0 over an RU of 26 tones
    std::string csvPath{"results/runs.csv"};
    std::string perStudentCsvPath{"results/per-student.csv"};

    CommandLine cmd(__FILE__);
    cmd.AddValue("standard", "Wi-Fi standard: ax (Wi-Fi 6) or ac (Wi-Fi 5)", standard);
    cmd.AddValue("rateManager",
                 "Rate control: constant (fixed --mcs), minstrel or ideal",
                 rateManager);
    cmd.AddValue("nStations", "Number of students (stations)", nStations);
    cmd.AddValue("distance", "Distance in meters between each student and the AP", distance);
    cmd.AddValue("farDistance",
                 "If set, one student is placed this many meters from the AP instead",
                 farDistance);
    cmd.AddValue("simulationTime", "Simulation time", simulationTime);
    cmd.AddValue("udp", "UDP if set to 1, TCP otherwise", udp);
    cmd.AddValue("downlink",
                 "Generate downlink flows if set to 1, uplink flows otherwise",
                 downlink);
    cmd.AddValue("mcs",
                 "MCS used by the constant rate manager (ax: 0-11, ac: 0-9). Ignored by "
                 "minstrel/ideal",
                 mcs);
    cmd.AddValue("channelWidth", "Channel width in MHz (20, 40, 80 or 160)", channelWidth);
    cmd.AddValue("guardInterval",
                 "Guard interval in ns (ax: 800, 1600 or 3200; ac: 400 or 800)",
                 guardInterval);
    cmd.AddValue("payloadSize", "The application payload size in bytes", payloadSize);
    cmd.AddValue("csv", "CSV file to append the run summary to", csvPath);
    cmd.AddValue("perStudentCsv", "CSV file to append per-student throughput to", perStudentCsvPath);
    cmd.Parse(argc, argv);

    if (standard != "ax" && standard != "ac")
    {
        NS_FATAL_ERROR("Invalid standard: " << standard << " (must be ax or ac)");
    }
    const bool isAx = (standard == "ax");
    const int maxMcs = isAx ? 11 : 9;
    if (mcs < 0 || mcs > maxMcs)
    {
        NS_FATAL_ERROR("Invalid MCS " << mcs << " for 802.11" << standard << " (must be 0-"
                                      << maxMcs << ")");
    }
    if (channelWidth != 20 && channelWidth != 40 && channelWidth != 80 && channelWidth != 160)
    {
        NS_FATAL_ERROR("Invalid channel width: " << channelWidth << " MHz");
    }
    if (isAx && guardInterval != 800 && guardInterval != 1600 && guardInterval != 3200)
    {
        NS_FATAL_ERROR("802.11ax guard interval must be 800, 1600 or 3200 ns");
    }
    if (!isAx && guardInterval != 400 && guardInterval != 800)
    {
        NS_FATAL_ERROR("802.11ac guard interval must be 400 or 800 ns");
    }
    if (rateManager != "constant" && rateManager != "minstrel" && rateManager != "ideal")
    {
        NS_FATAL_ERROR("Invalid rate manager: " << rateManager
                                                << " (must be constant, minstrel or ideal)");
    }
    if (nStations < 1)
    {
        NS_FATAL_ERROR("Need at least one station");
    }
    if (farDistance < 0)
    {
        NS_FATAL_ERROR("farDistance can't be negative");
    }

    // e.g. VHT MCS 9 isn't allowed at 20 MHz with 1 stream
    auto getMode = [isAx](int m) { return isAx ? HePhy::GetHeMcs(m) : VhtPhy::GetVhtMcs(m); };
    const MHz_u width{static_cast<double>(channelWidth)};
    if (!getMode(mcs).IsAllowed(width, 1))
    {
        NS_FATAL_ERROR("MCS " << mcs << " is not allowed at " << channelWidth << " MHz for 802.11"
                              << standard);
    }

    // load per student = PHY rate / nStations like the original. minstrel/ideal can go above
    // --mcs, so use the highest MCS there so the channel stays saturated
    int loadMcs = mcs;
    if (rateManager != "constant")
    {
        loadMcs = maxMcs;
        while (!getMode(loadMcs).IsAllowed(width, 1))
        {
            loadMcs--;
        }
    }
    const uint64_t phyRate =
        isAx ? HePhy::GetDataRate(loadMcs, width, NanoSeconds(guardInterval), 1)
             : VhtPhy::GetDataRate(loadMcs, width, NanoSeconds(guardInterval), 1);
    const auto maxLoad = phyRate / nStations;

    if (!udp)
    {
        Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));
    }

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nStations);
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    NetDeviceContainer apDevice;
    NetDeviceContainer staDevices;
    WifiMacHelper mac;
    WifiHelper wifi;
    std::string channelStr = "{0, " + std::to_string(channelWidth) + ", BAND_5GHZ, 0}";

    wifi.SetStandard(isAx ? WIFI_STANDARD_80211ax : WIFI_STANDARD_80211ac);

    if (rateManager == "constant")
    {
        std::ostringstream ossDataMode;
        ossDataMode << (isAx ? "HeMcs" : "VhtMcs") << mcs;
        auto nonHtRefRateMbps =
            (isAx ? HePhy::GetNonHtReferenceRate(mcs) : VhtPhy::GetNonHtReferenceRate(mcs)) / 1e6;
        std::ostringstream ossControlMode;
        ossControlMode << "OfdmRate" << nonHtRefRateMbps << "Mbps";
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode",
                                     StringValue(ossDataMode.str()),
                                     "ControlMode",
                                     StringValue(ossControlMode.str()));
    }
    else if (rateManager == "minstrel")
    {
        wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");
    }
    else
    {
        wifi.SetRemoteStationManager("ns3::IdealWifiManager");
    }

    // Set guard interval (802.11ac only has 800 ns or short 400 ns)
    if (isAx)
    {
        wifi.ConfigHeOptions("GuardInterval", TimeValue(NanoSeconds(guardInterval)));
    }
    else
    {
        wifi.ConfigHtOptions("ShortGuardIntervalSupported", BooleanValue(guardInterval == 400));
    }

    Ssid ssid = Ssid("classroom-ap");

    auto channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.SetChannel(channel.Create());

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "MpduBufferSize", UintegerValue(64));
    phy.Set("ChannelSettings", StringValue(channelStr));
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac",
                "EnableBeaconJitter",
                BooleanValue(false),
                "Ssid",
                SsidValue(ssid));
    apDevice = wifi.Install(phy, mac, wifiApNode);

    int64_t streamNumber = 150;
    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);

    // mobility. AP at the origin, students in a circle around it
    std::vector<double> staDistances(nStations, distance);
    if (farDistance > 0)
    {
        staDistances[nStations - 1] = farDistance;
    }

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    for (std::size_t i = 0; i < nStations; i++)
    {
        double angle = 2 * M_PI * i / nStations;
        positionAlloc->Add(
            Vector(staDistances[i] * std::cos(angle), staDistances[i] * std::sin(angle), 0.0));
    }
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    /* static setup of association and BA agreements */
    auto apDev = DynamicCast<WifiNetDevice>(apDevice.Get(0));
    NS_ASSERT(apDev);
    WifiStaticSetupHelper::SetStaticAssociation(apDev, staDevices);
    WifiStaticSetupHelper::SetStaticBlockAck(apDev, staDevices, {0});
    auto clientAppStartTime = MilliSeconds(1);

    /* Internet stack*/
    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);
    streamNumber += stack.AssignStreams(wifiApNode, streamNumber);
    streamNumber += stack.AssignStreams(wifiStaNodes, streamNumber);

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterfaces;
    Ipv4InterfaceContainer apNodeInterface;

    staNodeInterfaces = address.Assign(staDevices);
    apNodeInterface = address.Assign(apDevice);

    /* static setup of ARP cache */
    NeighborCacheHelper nbCache;
    nbCache.PopulateNeighborCache();

    /* Setting applications */
    ApplicationContainer serverApp;
    auto serverNodes = downlink ? std::ref(wifiStaNodes) : std::ref(wifiApNode);
    Ipv4InterfaceContainer serverInterfaces;
    NodeContainer clientNodes;
    for (std::size_t i = 0; i < nStations; i++)
    {
        serverInterfaces.Add(downlink ? staNodeInterfaces.Get(i) : apNodeInterface.Get(0));
        clientNodes.Add(downlink ? wifiApNode.Get(0) : wifiStaNodes.Get(i));
    }

    if (udp)
    {
        // UDP flow
        uint16_t port = 9;
        UdpServerHelper server(port);
        serverApp = server.Install(serverNodes.get());
        streamNumber += server.AssignStreams(serverNodes.get(), streamNumber);

        serverApp.Start(Seconds(0));
        serverApp.Stop(simulationTime + clientAppStartTime);
        const auto packetInterval = payloadSize * 8.0 / maxLoad;

        for (std::size_t i = 0; i < nStations; i++)
        {
            UdpClientHelper client(serverInterfaces.GetAddress(i), port);
            client.SetAttribute("MaxPackets", UintegerValue(4294967295U));
            client.SetAttribute("Interval", TimeValue(Seconds(packetInterval)));
            client.SetAttribute("PacketSize", UintegerValue(payloadSize));
            ApplicationContainer clientApp = client.Install(clientNodes.Get(i));
            streamNumber += client.AssignStreams(clientNodes.Get(i), streamNumber);

            clientApp.Start(clientAppStartTime);
            clientApp.Stop(simulationTime + clientAppStartTime);
        }
    }
    else
    {
        // TCP flow
        uint16_t port = 50000;
        Address localAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
        PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", localAddress);
        serverApp = packetSinkHelper.Install(serverNodes.get());
        streamNumber += packetSinkHelper.AssignStreams(serverNodes.get(), streamNumber);

        serverApp.Start(Seconds(0));
        serverApp.Stop(simulationTime + clientAppStartTime);

        for (std::size_t i = 0; i < nStations; i++)
        {
            OnOffHelper onoff("ns3::TcpSocketFactory", Ipv4Address::GetAny());
            onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
            onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
            onoff.SetAttribute("PacketSize", UintegerValue(payloadSize));
            onoff.SetAttribute("DataRate", DataRateValue(maxLoad));
            AddressValue remoteAddress(InetSocketAddress(serverInterfaces.GetAddress(i), port));
            onoff.SetAttribute("Remote", remoteAddress);
            ApplicationContainer clientApp = onoff.Install(clientNodes.Get(i));
            streamNumber += onoff.AssignStreams(clientNodes.Get(i), streamNumber);

            clientApp.Start(clientAppStartTime);
            clientApp.Stop(simulationTime + clientAppStartTime);
        }
    }

    Simulator::Schedule(Seconds(0), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);

    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    Simulator::Stop(simulationTime + clientAppStartTime);
    Simulator::Run();

    // FlowMonitor counts whole IP packets, so subtract the headers to get the same goodput the
    // original printed (UDP: 20 + 8, TCP: 20 + 20 + 12 timestamp option)
    monitor->CheckForLostPackets();
    auto classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    const uint32_t headerBytes = udp ? 28 : 52;
    const Ipv4Address apAddress = apNodeInterface.GetAddress(0);
    std::vector<double> perStudent(nStations, 0.0); // Mbit/s

    for (const auto& [flowId, stats] : monitor->GetFlowStats())
    {
        auto flow = classifier->FindFlow(flowId);
        for (std::size_t i = 0; i < nStations; i++)
        {
            auto staAddress = staNodeInterfaces.GetAddress(i);
            // data direction only, so TCP ACK flows are skipped
            bool isThisStudent =
                downlink ? (flow.sourceAddress == apAddress && flow.destinationAddress == staAddress)
                         : (flow.sourceAddress == staAddress && flow.destinationAddress == apAddress);
            if (isThisStudent)
            {
                double goodBytes =
                    static_cast<double>(stats.rxBytes) - stats.rxPackets * headerBytes;
                perStudent[i] += goodBytes * 8 / simulationTime.GetMicroSeconds();
            }
        }
    }

    Simulator::Destroy();

    double total = 0;
    double sumSquares = 0;
    double minThroughput = perStudent[0];
    for (double x : perStudent)
    {
        total += x;
        sumSquares += x * x;
        minThroughput = std::min(minThroughput, x);
    }
    double mean = total / nStations;
    // Jain's index: 1 = perfectly fair, 1/n = one student gets everything
    double jain = sumSquares > 0 ? (total * total) / (nStations * sumSquares) : 0;

    const std::string direction = downlink ? "downlink" : "uplink";
    const uint64_t run = RngSeedManager::GetRun();

    std::cout << "802.11" << standard << ", " << nStations << " students, " << direction
              << ", rate manager: " << rateManager;
    if (rateManager == "constant")
    {
        std::cout << " (MCS " << mcs << ")";
    }
    std::cout << ", " << channelWidth << " MHz, " << guardInterval << " ns GI, "
              << (udp ? "UDP" : "TCP") << ", " << simulationTime.As(Time::S) << ", run " << run
              << '\n';
    for (std::size_t i = 0; i < nStations; i++)
    {
        std::cout << "  Student " << i + 1 << " (" << staDistances[i]
                  << " m): " << perStudent[i] << " Mbit/s\n";
    }
    std::cout << "Total throughput: " << total << " Mbit/s\n"
              << "Mean per student: " << mean << " Mbit/s\n"
              << "Min per student: " << minThroughput << " Mbit/s\n"
              << "Jain's fairness index: " << jain << std::endl;

    auto csv = OpenCsv(csvPath,
                       "standard,nStations,direction,rateManager,farDistance,totalMbps,meanMbps,"
                       "minMbps,jain,mcs,channelWidth,guardInterval,distance,udp,simulationTime,run");
    csv << standard << ',' << nStations << ',' << direction << ',' << rateManager << ','
        << farDistance << ',' << total << ',' << mean << ',' << minThroughput << ',' << jain << ','
        << mcs << ',' << channelWidth << ',' << guardInterval << ',' << distance << ',' << udp << ','
        << simulationTime.GetSeconds() << ',' << run << '\n';

    auto perStudentCsv = OpenCsv(perStudentCsvPath,
                                 "standard,nStations,direction,rateManager,farDistance,mcs,run,"
                                 "student,studentDistance,throughputMbps");
    for (std::size_t i = 0; i < nStations; i++)
    {
        perStudentCsv << standard << ',' << nStations << ',' << direction << ',' << rateManager
                      << ',' << farDistance << ',' << mcs << ',' << run << ',' << i + 1 << ','
                      << staDistances[i] << ',' << perStudent[i] << '\n';
    }

    return 0;
}
