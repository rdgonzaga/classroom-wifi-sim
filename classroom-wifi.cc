/*
 * Copyright (c) 2016 SEBASTIEN DERONNE
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sebastien Deronne <sebastien.deronne@gmail.com>
 */

// Based on ns-3.48's examples/wireless/wifi-he-network.cc, modified by NSCOM03 Group 5.
// One AP, nStations students, one flow each. Appends results to --csv and --perStudentCsv.

#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/he-phy.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/neighbor-cache-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/uinteger.h"
#include "ns3/vht-phy.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-static-setup-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <limits>
#include <sstream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("classroom-wifi");

// append, writing the header if the file is new
std::ofstream
OpenCsv(const std::string& path, const std::string& header)
{
    auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent);
    }
    // different columns: move the old file aside instead of mixing them
    if (std::filesystem::is_regular_file(path) && std::filesystem::file_size(path) > 0)
    {
        std::ifstream existing(path);
        std::string firstLine;
        std::getline(existing, firstLine);
        existing.close();
        if (firstLine != header)
        {
            auto p = std::filesystem::path(path);
            auto old = p.parent_path() / (p.stem().string() + "-old" + p.extension().string());
            for (int i = 2; std::filesystem::exists(old); i++)
            {
                old = p.parent_path() /
                      (p.stem().string() + "-old" + std::to_string(i) + p.extension().string());
            }
            std::filesystem::rename(p, old);
            std::cerr << "Note: " << path << " had different columns, moved it to " << old
                      << '\n';
        }
    }
    // only regular files get a header, so /dev/null works
    bool isNew = !std::filesystem::exists(path) || (std::filesystem::is_regular_file(path) &&
                                                     std::filesystem::file_size(path) == 0);
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
    bool ofdma{false};
    bool ulOfdma{false};
    std::string phyModel{"Yans"};
    Time accessReqInterval{"3ms"};
    bool udp{true};
    bool downlink{true};
    Time simulationTime{"10s"};
    std::string layout{"circle"};
    double distance{1.0};
    double farDistance{0.0};
    // grid layout: 7 m x 9 m room, 3 m ceiling, 8 x 7 = 56 seats
    double roomWidth{7.0};
    double roomDepth{9.0};
    std::size_t rows{8};
    std::size_t cols{7};
    double apZ{3.0};
    bool rtsCts{false};
    std::size_t nStations{1};
    double demand{0.0};
    int mcs{5};
    int channelWidth{20};
    int guardInterval{800};
    uint32_t payloadSize{700}; // fits a 26-tone RU at MCS 0
    std::string csvPath{"results/runs.csv"};
    std::string perStudentCsvPath{"results/per-student.csv"};

    CommandLine cmd(__FILE__);
    cmd.AddValue("standard", "Wi-Fi standard: ax (Wi-Fi 6) or ac (Wi-Fi 5)", standard);
    cmd.AddValue("rateManager",
                 "Rate control: constant (fixed --mcs), minstrel or ideal",
                 rateManager);
    cmd.AddValue("ofdma", "Enable DL OFDMA (802.11ax only)", ofdma);
    cmd.AddValue("ulOfdma",
                 "Also enable UL OFDMA (802.11ax only, needs guardInterval >= 1600)",
                 ulOfdma);
    cmd.AddValue("muSchedAccessReqInterval",
                 "With UL OFDMA, how often the AP asks for channel access to send triggers",
                 accessReqInterval);
    cmd.AddValue("phyModel",
                 "PHY model: Yans or Spectrum. OFDMA always uses Spectrum",
                 phyModel);
    cmd.AddValue("nStations", "Number of students (stations)", nStations);
    cmd.AddValue("layout",
                 "Student placement: circle (around the AP) or grid (classroom seats)",
                 layout);
    cmd.AddValue("distance",
                 "Circle layout: distance in meters between each student and the AP",
                 distance);
    cmd.AddValue("farDistance",
                 "If set, one student is placed this many meters from the AP (horizontally) "
                 "instead",
                 farDistance);
    cmd.AddValue("roomWidth", "Grid layout: room width in meters", roomWidth);
    cmd.AddValue("roomDepth", "Grid layout: room depth in meters (front to back)", roomDepth);
    cmd.AddValue("rows", "Grid layout: rows of seats", rows);
    cmd.AddValue("cols", "Grid layout: seats per row", cols);
    cmd.AddValue("apZ", "Grid layout: AP height in meters (on the ceiling)", apZ);
    cmd.AddValue("rtsCts", "Use RTS/CTS before every data frame if set to 1", rtsCts);
    cmd.AddValue("demand",
                 "Offered load per student in Mbit/s (application payload). 0 means saturated",
                 demand);
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
    if (phyModel != "Yans" && phyModel != "Spectrum")
    {
        NS_FATAL_ERROR("Invalid PHY model: " << phyModel << " (must be Yans or Spectrum)");
    }
    if (ulOfdma)
    {
        ofdma = true;
    }
    if (ofdma && !isAx)
    {
        NS_FATAL_ERROR("OFDMA is only available in 802.11ax");
    }
    if (ulOfdma && guardInterval < 1600)
    {
        NS_FATAL_ERROR("UL OFDMA needs a guard interval of 1600 or 3200 ns");
    }
    if (ofdma)
    {
        phyModel = "Spectrum";
    }
    if (nStations < 1)
    {
        NS_FATAL_ERROR("Need at least one station");
    }
    if (farDistance < 0)
    {
        NS_FATAL_ERROR("farDistance can't be negative");
    }
    if (demand < 0)
    {
        NS_FATAL_ERROR("demand can't be negative");
    }
    if (layout != "circle" && layout != "grid")
    {
        NS_FATAL_ERROR("Invalid layout: " << layout << " (must be circle or grid)");
    }
    // teacher's area at the front
    const double frontClearance = 2.0;
    if (layout == "grid")
    {
        if (rows < 1 || cols < 1)
        {
            NS_FATAL_ERROR("Need at least one row and one column");
        }
        if (nStations > rows * cols)
        {
            NS_FATAL_ERROR(nStations << " students don't fit in " << rows << " x " << cols
                                     << " = " << rows * cols << " seats");
        }
        if (roomWidth <= 0 || roomDepth <= frontClearance || apZ < 0)
        {
            NS_FATAL_ERROR("Room must be wider than 0 m, deeper than " << frontClearance
                                                                         << " m, and apZ >= 0");
        }
    }

    auto getMode = [isAx](int m) { return isAx ? HePhy::GetHeMcs(m) : VhtPhy::GetVhtMcs(m); };
    const MHz_u width{static_cast<double>(channelWidth)};
    if (!getMode(mcs).IsAllowed(width, 1))
    {
        NS_FATAL_ERROR("MCS " << mcs << " is not allowed at " << channelWidth << " MHz for 802.11"
                              << standard);
    }

    // saturated load = PHY rate / nStations, at the top MCS if the rate manager can go above --mcs
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
    const auto maxLoad =
        demand > 0 ? static_cast<uint64_t>(demand * 1e6) : phyRate / nStations;

    if (!udp)
    {
        Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));
    }

    // FqCoDel's default 1024 queues let two students share one (each gets half). queues are
    // created lazily, so the full hash range is free
    Config::SetDefault("ns3::FqCoDelQueueDisc::Flows", UintegerValue(4294967295U));

    if (ofdma)
    {
        Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                           EnumValue(WifiAcknowledgment::DL_MU_AGGREGATE_TF));
    }

    if (rtsCts)
    {
        // threshold 0: every frame gets RTS/CTS
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(0));
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

    if (isAx)
    {
        wifi.ConfigHeOptions("GuardInterval", TimeValue(NanoSeconds(guardInterval)));
    }
    else
    {
        wifi.ConfigHtOptions("ShortGuardIntervalSupported", BooleanValue(guardInterval == 400));
    }

    Ssid ssid = Ssid("classroom-ap");

    if (phyModel == "Spectrum")
    {
        auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();

        auto lossModel = CreateObject<LogDistancePropagationLossModel>();
        spectrumChannel->AddPropagationLossModel(lossModel);

        SpectrumWifiPhyHelper phy;
        phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy.SetChannel(spectrumChannel);

        mac.SetType("ns3::StaWifiMac",
                    "Ssid",
                    SsidValue(ssid),
                    "MpduBufferSize",
                    UintegerValue(64));
        phy.Set("ChannelSettings", StringValue(channelStr));
        staDevices = wifi.Install(phy, mac, wifiStaNodes);

        if (ofdma)
        {
            // in uplink the AP needs AccessReqInterval to get the channel for triggers
            mac.SetMultiUserScheduler("ns3::RrMultiUserScheduler",
                                      "EnableUlOfdma",
                                      BooleanValue(ulOfdma),
                                      "EnableBsrp",
                                      BooleanValue(ulOfdma),
                                      "AccessReqInterval",
                                      TimeValue(ulOfdma ? accessReqInterval : Time{0}));
        }
        mac.SetType("ns3::ApWifiMac",
                    "EnableBeaconJitter",
                    BooleanValue(false),
                    "BeaconGeneration",
                    BooleanValue(false),
                    "Ssid",
                    SsidValue(ssid));
        apDevice = wifi.Install(phy, mac, wifiApNode);
    }
    else
    {
        auto channel = YansWifiChannelHelper::Default();
        YansWifiPhyHelper phy;
        phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy.SetChannel(channel.Create());

        mac.SetType("ns3::StaWifiMac",
                    "Ssid",
                    SsidValue(ssid),
                    "MpduBufferSize",
                    UintegerValue(64));
        phy.Set("ChannelSettings", StringValue(channelStr));
        staDevices = wifi.Install(phy, mac, wifiStaNodes);

        mac.SetType("ns3::ApWifiMac",
                    "EnableBeaconJitter",
                    BooleanValue(false),
                    "Ssid",
                    SsidValue(ssid));
        apDevice = wifi.Install(phy, mac, wifiApNode);
    }

    int64_t streamNumber = 150;
    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);

    Vector apPos;
    std::vector<Vector> staPos(nStations);
    if (layout == "circle")
    {
        apPos = Vector(0.0, 0.0, 0.0);
        for (std::size_t i = 0; i < nStations; i++)
        {
            double d = (farDistance > 0 && i == nStations - 1) ? farDistance : distance;
            double angle = 2 * M_PI * i / nStations;
            staPos[i] = Vector(d * std::cos(angle), d * std::sin(angle), 0.0);
        }
    }
    else
    {
        // AP on the ceiling at the center, seats filled front to back at desk height
        const double deskZ = 0.75;
        apPos = Vector(roomWidth / 2, roomDepth / 2, apZ);
        const double colSpacing = roomWidth / cols;
        const double rowSpacing = (roomDepth - frontClearance) / rows;
        for (std::size_t i = 0; i < nStations; i++)
        {
            std::size_t row = i / cols;
            std::size_t col = i % cols;
            staPos[i] = Vector((col + 0.5) * colSpacing,
                               frontClearance + (row + 0.5) * rowSpacing,
                               deskZ);
        }
        // far student is outside the room, e.g. the hallway
        if (farDistance > 0)
        {
            staPos[nStations - 1] = Vector(apPos.x, apPos.y + farDistance, deskZ);
        }
    }

    // 3D distance, as the loss model sees it
    std::vector<double> staDistances(nStations);
    for (std::size_t i = 0; i < nStations; i++)
    {
        staDistances[i] = CalculateDistance(staPos[i], apPos);
    }

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(apPos);
    for (const auto& pos : staPos)
    {
        positionAlloc->Add(pos);
    }
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    auto apDev = DynamicCast<WifiNetDevice>(apDevice.Get(0));
    NS_ASSERT(apDev);
    WifiStaticSetupHelper::SetStaticAssociation(apDev, staDevices);
    WifiStaticSetupHelper::SetStaticBlockAck(apDev, staDevices, {0});
    auto clientAppStartTime = MilliSeconds(1);

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

    NeighborCacheHelper nbCache;
    nbCache.PopulateNeighborCache();

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

    // throughput counts until the apps stop; the drain lets queued packets arrive or drop
    // so delay and loss are complete
    const Time drainTime = Seconds(1);
    FlowMonitor::FlowStatsContainer statsAtStop;
    Simulator::Schedule(simulationTime + clientAppStartTime,
                        [&]() { statsAtStop = monitor->GetFlowStats(); });
    Simulator::Stop(simulationTime + clientAppStartTime + drainTime);
    Simulator::Run();

    // strip IP + UDP/TCP headers to get goodput
    auto classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    const uint32_t headerBytes = udp ? 28 : 52;
    const Ipv4Address apAddress = apNodeInterface.GetAddress(0);
    std::vector<double> perStudent(nStations, 0.0);
    std::vector<double> delaySumMs(nStations, 0.0);
    std::vector<uint64_t> txPackets(nStations, 0);
    std::vector<uint64_t> rxPackets(nStations, 0);

    for (const auto& [flowId, stats] : statsAtStop)
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

    monitor->CheckForLostPackets();
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
                delaySumMs[i] += stats.delaySum.GetSeconds() * 1000;
                txPackets[i] += stats.txPackets;
                rxPackets[i] += stats.rxPackets;
            }
        }
    }

    Simulator::Destroy();

    // a student that received nothing has NaN delay
    const double nan = std::numeric_limits<double>::quiet_NaN();
    std::vector<double> delayMs(nStations, nan);
    std::vector<double> lossPct(nStations, 0.0);
    for (std::size_t i = 0; i < nStations; i++)
    {
        if (rxPackets[i] > 0)
        {
            delayMs[i] = delaySumMs[i] / rxPackets[i];
        }
        if (txPackets[i] > 0)
        {
            lossPct[i] = 100.0 * (txPackets[i] - rxPackets[i]) / txPackets[i];
        }
    }

    double total = 0;
    double sumSquares = 0;
    double minThroughput = perStudent[0];
    double maxThroughput = perStudent[0];
    for (double x : perStudent)
    {
        total += x;
        sumSquares += x * x;
        minThroughput = std::min(minThroughput, x);
        maxThroughput = std::max(maxThroughput, x);
    }
    double mean = total / nStations;
    // Jain's index: 1 = fair, 1/n = one student gets everything
    double jain = sumSquares > 0 ? (total * total) / (nStations * sumSquares) : 0;
    double minMaxRatio = maxThroughput > 0 ? minThroughput / maxThroughput : 0;

    double delaySum = 0;
    std::size_t delayCount = 0;
    double worstDelay = nan;
    double lossSum = 0;
    for (std::size_t i = 0; i < nStations; i++)
    {
        lossSum += lossPct[i];
        if (!std::isnan(delayMs[i]))
        {
            delaySum += delayMs[i];
            delayCount++;
            if (std::isnan(worstDelay) || delayMs[i] > worstDelay)
            {
                worstDelay = delayMs[i];
            }
        }
    }
    double meanDelay = delayCount > 0 ? delaySum / delayCount : nan;
    double meanLoss = lossSum / nStations;
    double meanSatisfaction = demand > 0 ? mean / demand : 0;
    double minSatisfaction = demand > 0 ? minThroughput / demand : 0;

    const std::string direction = downlink ? "downlink" : "uplink";
    const uint64_t run = RngSeedManager::GetRun();
    const std::string ofdmaStr = ulOfdma ? "dl+ul" : (ofdma ? "dl" : "off");

    std::cout << "802.11" << standard << ", " << nStations << " students, " << direction
              << ", rate manager: " << rateManager;
    if (rateManager == "constant")
    {
        std::cout << " (MCS " << mcs << ")";
    }
    std::cout << ", " << channelWidth << " MHz, " << guardInterval << " ns GI, "
              << (udp ? "UDP" : "TCP") << ", OFDMA " << ofdmaStr << ", " << phyModel << ", ";
    if (demand > 0)
    {
        std::cout << "demand " << demand << " Mbit/s, ";
    }
    else
    {
        std::cout << "saturated, ";
    }
    std::cout << layout << " layout, " << (rtsCts ? "RTS/CTS on, " : "")
              << simulationTime.As(Time::S) << ", run " << run << '\n';
    for (std::size_t i = 0; i < nStations; i++)
    {
        std::ostringstream dist;
        dist << std::fixed << std::setprecision(2) << staDistances[i];
        std::cout << "  Student " << i + 1 << " (" << dist.str() << " m): " << perStudent[i]
                  << " Mbit/s, delay " << delayMs[i] << " ms, loss " << lossPct[i] << " %\n";
    }
    std::cout << "Total throughput: " << total << " Mbit/s\n"
              << "Mean per student: " << mean << " Mbit/s\n"
              << "Min per student: " << minThroughput << " Mbit/s\n"
              << "Max per student: " << maxThroughput << " Mbit/s\n"
              << "Jain's fairness index: " << jain << '\n'
              << "Min / max ratio: " << minMaxRatio << '\n'
              << "Mean delay: " << meanDelay << " ms\n"
              << "Worst student's delay: " << worstDelay << " ms\n"
              << "Mean loss: " << meanLoss << " %" << std::endl;
    if (demand > 0)
    {
        std::cout << "Delivered / demand (mean): " << meanSatisfaction << '\n'
                  << "Delivered / demand (worst student): " << minSatisfaction << std::endl;
    }

    // add new columns at the end
    auto csv = OpenCsv(csvPath,
                       "standard,nStations,direction,rateManager,farDistance,totalMbps,meanMbps,"
                       "minMbps,jain,mcs,channelWidth,guardInterval,distance,udp,simulationTime,run,"
                       "ofdma,phyModel,demand,meanSatisfaction,minSatisfaction,layout,rtsCts,"
                       "maxMbps,minMaxRatio,meanDelayMs,worstDelayMs,meanLossPct");
    csv << standard << ',' << nStations << ',' << direction << ',' << rateManager << ','
        << farDistance << ',' << total << ',' << mean << ',' << minThroughput << ',' << jain << ','
        << mcs << ',' << channelWidth << ',' << guardInterval << ',' << distance << ',' << udp << ','
        << simulationTime.GetSeconds() << ',' << run << ',' << ofdmaStr << ',' << phyModel << ','
        << demand << ',' << meanSatisfaction << ',' << minSatisfaction << ',' << layout << ','
        << rtsCts << ',' << maxThroughput << ',' << minMaxRatio << ',' << meanDelay << ','
        << worstDelay << ',' << meanLoss << '\n';

    auto perStudentCsv = OpenCsv(perStudentCsvPath,
                                 "standard,nStations,direction,rateManager,farDistance,mcs,ofdma,"
                                 "run,student,studentDistance,throughputMbps,demand,satisfaction,"
                                 "layout,delayMs,lossPct");
    for (std::size_t i = 0; i < nStations; i++)
    {
        perStudentCsv << standard << ',' << nStations << ',' << direction << ',' << rateManager
                      << ',' << farDistance << ',' << mcs << ',' << ofdmaStr << ',' << run << ','
                      << i + 1 << ',' << staDistances[i] << ',' << perStudent[i] << ',' << demand
                      << ',' << (demand > 0 ? perStudent[i] / demand : 0) << ',' << layout << ','
                      << delayMs[i] << ',' << lossPct[i] << '\n';
    }
    return 0;
}
