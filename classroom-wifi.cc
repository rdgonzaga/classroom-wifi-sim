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
#include "ns3/random-variable-stream.h"
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

#include <algorithm>
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
    std::string devices{"identical"};
    double laptopProb{1.0};
    int apStreams{0};
    int staStreams{1};
    double laptopDemand{3.0};
    double phoneDemand{1.0};

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
    cmd.AddValue("devices",
                 "identical (one STA per student) or mixed (a 1x1 phone per student, plus a 2x2 "
                 "laptop with probability --laptopProb)",
                 devices);
    cmd.AddValue("laptopProb", "Mixed devices: chance that a student also has a laptop", laptopProb);
    cmd.AddValue("apStreams",
                 "AP antennas / spatial streams. 0 = 1 for identical, 2 for mixed",
                 apStreams);
    cmd.AddValue("staStreams",
                 "Identical devices: antennas / spatial streams per STA",
                 staStreams);
    cmd.AddValue("laptopDemand",
                 "Mixed devices: offered load per laptop in Mbit/s (both 0 = saturated)",
                 laptopDemand);
    cmd.AddValue("phoneDemand",
                 "Mixed devices: offered load per phone in Mbit/s (both 0 = saturated)",
                 phoneDemand);
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
    if (devices != "identical" && devices != "mixed")
    {
        NS_FATAL_ERROR("Invalid devices: " << devices << " (must be identical or mixed)");
    }
    const bool mixed = (devices == "mixed");
    if (apStreams == 0)
    {
        apStreams = mixed ? 2 : 1;
    }
    if (apStreams < 1 || apStreams > 4 || staStreams < 1 || staStreams > 4)
    {
        NS_FATAL_ERROR("apStreams and staStreams must be 1-4");
    }
    if (laptopProb < 0 || laptopProb > 1)
    {
        NS_FATAL_ERROR("laptopProb must be between 0 and 1");
    }
    if (mixed && demand > 0)
    {
        NS_FATAL_ERROR("With --devices=mixed use --laptopDemand and --phoneDemand, not --demand");
    }
    if (mixed && (laptopDemand < 0 || phoneDemand < 0 || (laptopDemand > 0) != (phoneDemand > 0)))
    {
        NS_FATAL_ERROR("laptopDemand and phoneDemand must both be > 0, or both 0 (saturated)");
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
    for (int nss : {1, apStreams, staStreams})
    {
        if (!getMode(mcs).IsAllowed(width, nss))
        {
            NS_FATAL_ERROR("MCS " << mcs << " is not allowed at " << channelWidth
                                  << " MHz with " << nss << " streams for 802.11" << standard);
        }
    }

    // one entry per STA. identical: one per student. mixed: laptop (if drawn) then phone
    struct Device
    {
        std::size_t student;
        std::string type; // sta, laptop or phone
        int streams;
        double demand; // Mbit/s, 0 = saturated
    };

    std::vector<Device> devs;
    if (!mixed)
    {
        for (std::size_t i = 0; i < nStations; i++)
        {
            devs.push_back({i, "sta", staStreams, demand});
        }
    }
    else
    {
        // own stream, so the draw depends only on --RngSeed / --RngRun
        auto laptopDraw = CreateObject<UniformRandomVariable>();
        laptopDraw->SetStream(100);
        for (std::size_t i = 0; i < nStations; i++)
        {
            if (laptopDraw->GetValue() < laptopProb)
            {
                devs.push_back({i, "laptop", 2, laptopDemand});
            }
            devs.push_back({i, "phone", 1, phoneDemand});
        }
    }
    const std::size_t nDevices = devs.size();
    std::size_t nLaptops = 0;
    std::size_t nPhones = 0;
    bool equalDemand = true;
    for (const auto& d : devs)
    {
        nLaptops += (d.type == "laptop");
        nPhones += (d.type == "phone");
        equalDemand = equalDemand && (d.demand == devs[0].demand);
    }
    const bool fixedDemand = devs[0].demand > 0;

    // saturated load = PHY rate at apStreams / nDevices, at the top MCS if the rate manager can
    // go above --mcs
    int loadMcs = mcs;
    if (rateManager != "constant")
    {
        loadMcs = maxMcs;
        while (!getMode(loadMcs).IsAllowed(width, apStreams))
        {
            loadMcs--;
        }
    }
    const uint64_t phyRate =
        isAx ? HePhy::GetDataRate(loadMcs, width, NanoSeconds(guardInterval), apStreams)
             : VhtPhy::GetDataRate(loadMcs, width, NanoSeconds(guardInterval), apStreams);
    std::vector<uint64_t> maxLoad(nDevices);
    for (std::size_t i = 0; i < nDevices; i++)
    {
        maxLoad[i] =
            devs[i].demand > 0 ? static_cast<uint64_t>(devs[i].demand * 1e6) : phyRate / nDevices;
    }

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
    wifiStaNodes.Create(nDevices);
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

    auto setStreams = [](WifiPhyHelper& phy, int n) {
        phy.Set("Antennas", UintegerValue(n));
        phy.Set("MaxSupportedTxSpatialStreams", UintegerValue(n));
        phy.Set("MaxSupportedRxSpatialStreams", UintegerValue(n));
    };
    // all STAs alike: one Install, as before. otherwise one at a time with their own streams
    auto installStas = [&](WifiPhyHelper& phy) {
        bool sameStreams = true;
        for (const auto& d : devs)
        {
            sameStreams = sameStreams && (d.streams == devs[0].streams);
        }
        if (sameStreams)
        {
            setStreams(phy, devs[0].streams);
            staDevices = wifi.Install(phy, mac, wifiStaNodes);
        }
        else
        {
            for (std::size_t i = 0; i < nDevices; i++)
            {
                setStreams(phy, devs[i].streams);
                staDevices.Add(wifi.Install(phy, mac, wifiStaNodes.Get(i)));
            }
        }
        setStreams(phy, apStreams);
    };

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
        installStas(phy);

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
        installStas(phy);

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
    std::vector<Vector> seatPos(nStations);
    if (layout == "circle")
    {
        apPos = Vector(0.0, 0.0, 0.0);
        for (std::size_t i = 0; i < nStations; i++)
        {
            double d = (farDistance > 0 && i == nStations - 1) ? farDistance : distance;
            double angle = 2 * M_PI * i / nStations;
            seatPos[i] = Vector(d * std::cos(angle), d * std::sin(angle), 0.0);
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
            seatPos[i] = Vector((col + 0.5) * colSpacing,
                               frontClearance + (row + 0.5) * rowSpacing,
                               deskZ);
        }
        // far student is outside the room, e.g. the hallway
        if (farDistance > 0)
        {
            seatPos[nStations - 1] = Vector(apPos.x, apPos.y + farDistance, deskZ);
        }
    }

    // laptop (or the single STA) at the seat, phone 0.3 m beside it
    std::vector<Vector> staPos(nDevices);
    for (std::size_t i = 0; i < nDevices; i++)
    {
        staPos[i] = seatPos[devs[i].student];
        if (devs[i].type == "phone")
        {
            staPos[i].x += 0.3;
        }
    }

    // 3D distance, as the loss model sees it
    std::vector<double> staDistances(nDevices);
    for (std::size_t i = 0; i < nDevices; i++)
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
    for (std::size_t i = 0; i < nDevices; i++)
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

        for (std::size_t i = 0; i < nDevices; i++)
        {
            UdpClientHelper client(serverInterfaces.GetAddress(i), port);
            client.SetAttribute("MaxPackets", UintegerValue(4294967295U));
            client.SetAttribute("Interval", TimeValue(Seconds(payloadSize * 8.0 / maxLoad[i])));
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

        for (std::size_t i = 0; i < nDevices; i++)
        {
            OnOffHelper onoff("ns3::TcpSocketFactory", Ipv4Address::GetAny());
            onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
            onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
            onoff.SetAttribute("PacketSize", UintegerValue(payloadSize));
            onoff.SetAttribute("DataRate", DataRateValue(maxLoad[i]));
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
    std::vector<double> staMbps(nDevices, 0.0);
    std::vector<double> delaySumMs(nDevices, 0.0);
    std::vector<uint64_t> txPackets(nDevices, 0);
    std::vector<uint64_t> rxPackets(nDevices, 0);

    // which STA a flow carries data for; nDevices if none (data direction only, so TCP ACK
    // flows are skipped)
    auto deviceOf = [&](FlowId flowId) {
        auto flow = classifier->FindFlow(flowId);
        for (std::size_t i = 0; i < nDevices; i++)
        {
            auto staAddress = staNodeInterfaces.GetAddress(i);
            if (downlink ? (flow.sourceAddress == apAddress && flow.destinationAddress == staAddress)
                         : (flow.sourceAddress == staAddress && flow.destinationAddress == apAddress))
            {
                return i;
            }
        }
        return nDevices;
    };

    for (const auto& [flowId, stats] : statsAtStop)
    {
        std::size_t i = deviceOf(flowId);
        if (i < nDevices)
        {
            double goodBytes = static_cast<double>(stats.rxBytes) - stats.rxPackets * headerBytes;
            staMbps[i] += goodBytes * 8 / simulationTime.GetMicroSeconds();
        }
    }

    monitor->CheckForLostPackets();
    for (const auto& [flowId, stats] : monitor->GetFlowStats())
    {
        std::size_t i = deviceOf(flowId);
        if (i < nDevices)
        {
            delaySumMs[i] += stats.delaySum.GetSeconds() * 1000;
            txPackets[i] += stats.txPackets;
            rxPackets[i] += stats.rxPackets;
        }
    }

    Simulator::Destroy();

    // a student that received nothing has NaN delay
    const double nan = std::numeric_limits<double>::quiet_NaN();
    std::vector<double> delayMs(nDevices, nan);
    std::vector<double> lossPct(nDevices, 0.0);
    for (std::size_t i = 0; i < nDevices; i++)
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
    double minThroughput = staMbps[0];
    double maxThroughput = staMbps[0];
    for (double x : staMbps)
    {
        total += x;
        sumSquares += x * x;
        minThroughput = std::min(minThroughput, x);
        maxThroughput = std::max(maxThroughput, x);
    }
    double mean = total / nDevices;

    // satisfaction = delivered / demand, per STA
    std::vector<double> satisfaction(nDevices, 0.0);
    double satTotal = 0;
    double satSquares = 0;
    double demandSum = 0;
    double worstLaptopSat = nan;
    double worstPhoneSat = nan;
    for (std::size_t i = 0; i < nDevices; i++)
    {
        demandSum += devs[i].demand;
        if (devs[i].demand > 0)
        {
            satisfaction[i] = staMbps[i] / devs[i].demand;
        }
        satTotal += satisfaction[i];
        satSquares += satisfaction[i] * satisfaction[i];
        double& worst = devs[i].type == "laptop" ? worstLaptopSat : worstPhoneSat;
        if (fixedDemand && devs[i].type != "sta" && (std::isnan(worst) || satisfaction[i] < worst))
        {
            worst = satisfaction[i];
        }
    }
    const double meanDemand = demandSum / nDevices;

    // Jain within one device type on raw throughput, since the overall index mixes 2x2 laptops
    // and 1x1 phones
    auto jainOf = [&](const std::string& type) {
        double s = 0;
        double sq = 0;
        std::size_t n = 0;
        for (std::size_t i = 0; i < nDevices; i++)
        {
            if (devs[i].type == type)
            {
                s += staMbps[i];
                sq += staMbps[i] * staMbps[i];
                n++;
            }
        }
        return (n > 0 && sq > 0) ? (s * s) / (n * sq) : nan;
    };
    const double jainLaptops = jainOf("laptop");
    const double jainPhones = jainOf("phone");

    // Jain's index: 1 = fair, 1/n = one STA gets everything. with unequal demands it's taken
    // on satisfaction (same value as on throughput when demands are equal)
    double jain = sumSquares > 0 ? (total * total) / (nDevices * sumSquares) : 0;
    if (!equalDemand)
    {
        jain = satSquares > 0 ? (satTotal * satTotal) / (nDevices * satSquares) : 0;
    }
    double minMaxRatio = maxThroughput > 0 ? minThroughput / maxThroughput : 0;

    double delaySum = 0;
    std::size_t delayCount = 0;
    double worstDelay = nan;
    double lossSum = 0;
    for (std::size_t i = 0; i < nDevices; i++)
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
    double meanLoss = lossSum / nDevices;
    double meanSatisfaction = 0;
    double minSatisfaction = 0;
    if (fixedDemand && equalDemand)
    {
        meanSatisfaction = mean / devs[0].demand;
        minSatisfaction = minThroughput / devs[0].demand;
    }
    else if (fixedDemand)
    {
        meanSatisfaction = satTotal / nDevices;
        minSatisfaction = *std::min_element(satisfaction.begin(), satisfaction.end());
    }

    const std::string direction = downlink ? "downlink" : "uplink";
    const uint64_t run = RngSeedManager::GetRun();
    const std::string ofdmaStr = ulOfdma ? "dl+ul" : (ofdma ? "dl" : "off");

    std::cout << "802.11" << standard << ", " << nStations << " students, ";
    if (mixed)
    {
        std::cout << nDevices << " STAs (" << nLaptops << " laptops 2x2, " << nPhones
                  << " phones 1x1), AP " << apStreams << "x" << apStreams << ", ";
    }
    else if (apStreams != 1 || staStreams != 1)
    {
        std::cout << "STAs " << staStreams << "x" << staStreams << ", AP " << apStreams << "x"
                  << apStreams << ", ";
    }
    std::cout << direction << ", rate manager: " << rateManager;
    if (rateManager == "constant")
    {
        std::cout << " (MCS " << mcs << ")";
    }
    std::cout << ", " << channelWidth << " MHz, " << guardInterval << " ns GI, "
              << (udp ? "UDP" : "TCP") << ", OFDMA " << ofdmaStr << ", " << phyModel << ", ";
    if (mixed && fixedDemand)
    {
        std::cout << "demand " << laptopDemand << " (laptop) / " << phoneDemand
                  << " (phone) Mbit/s, ";
    }
    else if (demand > 0)
    {
        std::cout << "demand " << demand << " Mbit/s, ";
    }
    else
    {
        std::cout << "saturated, ";
    }
    std::cout << layout << " layout, " << (rtsCts ? "RTS/CTS on, " : "")
              << simulationTime.As(Time::S) << ", run " << run << '\n';
    // sweep.sh drops lines starting with "  Student"
    for (std::size_t i = 0; i < nDevices; i++)
    {
        std::ostringstream dist;
        dist << std::fixed << std::setprecision(2) << staDistances[i];
        std::cout << "  Student " << devs[i].student + 1
                  << (mixed ? " " + devs[i].type : std::string{}) << " (" << dist.str()
                  << " m): " << staMbps[i] << " Mbit/s, delay " << delayMs[i] << " ms, loss "
                  << lossPct[i] << " %\n";
    }
    const std::string per = mixed ? "STA" : "student";
    std::cout << "Total throughput: " << total << " Mbit/s\n"
              << "Mean per " << per << ": " << mean << " Mbit/s\n"
              << "Min per " << per << ": " << minThroughput << " Mbit/s\n"
              << "Max per " << per << ": " << maxThroughput << " Mbit/s\n"
              << "Jain's fairness index: " << jain << (equalDemand ? "" : " (on delivered / demand)")
              << '\n'
              << "Min / max ratio: " << minMaxRatio << '\n'
              << "Mean delay: " << meanDelay << " ms\n"
              << "Worst " << per << "'s delay: " << worstDelay << " ms\n"
              << "Mean loss: " << meanLoss << " %" << std::endl;
    if (fixedDemand)
    {
        std::cout << "Delivered / demand (mean): " << meanSatisfaction << '\n'
                  << "Delivered / demand (worst " << per << "): " << minSatisfaction << std::endl;
        if (mixed)
        {
            std::cout << "Delivered / demand (worst laptop): " << worstLaptopSat << '\n'
                      << "Delivered / demand (worst phone): " << worstPhoneSat << std::endl;
        }
    }
    if (mixed)
    {
        // mean throughput per type, to show the 2x2 vs 1x1 gap
        double laptopSum = 0;
        double phoneSum = 0;
        for (std::size_t i = 0; i < nDevices; i++)
        {
            (devs[i].type == "laptop" ? laptopSum : phoneSum) += staMbps[i];
        }
        std::cout << "Mean per laptop: " << (nLaptops ? laptopSum / nLaptops : nan) << " Mbit/s\n"
                  << "Mean per phone: " << (nPhones ? phoneSum / nPhones : nan) << " Mbit/s\n"
                  << "Jain's index, laptops only: " << jainLaptops << '\n'
                  << "Jain's index, phones only: " << jainPhones << std::endl;
    }

    // add new columns at the end
    auto csv = OpenCsv(csvPath,
                       "standard,nStations,direction,rateManager,farDistance,totalMbps,meanMbps,"
                       "minMbps,jain,mcs,channelWidth,guardInterval,distance,udp,simulationTime,run,"
                       "ofdma,phyModel,demand,meanSatisfaction,minSatisfaction,layout,rtsCts,"
                       "maxMbps,minMaxRatio,meanDelayMs,worstDelayMs,meanLossPct,devices,"
                       "apStreams,staStreams,laptopProb,nDevices,laptops,phones,laptopDemand,"
                       "phoneDemand,worstLaptopSatisfaction,worstPhoneSatisfaction,jainLaptops,"
                       "jainPhones");
    // demand: mean per-STA demand (= --demand for identical devices)
    csv << standard << ',' << nStations << ',' << direction << ',' << rateManager << ','
        << farDistance << ',' << total << ',' << mean << ',' << minThroughput << ',' << jain << ','
        << mcs << ',' << channelWidth << ',' << guardInterval << ',' << distance << ',' << udp << ','
        << simulationTime.GetSeconds() << ',' << run << ',' << ofdmaStr << ',' << phyModel << ','
        << (mixed ? meanDemand : demand) << ',' << meanSatisfaction << ',' << minSatisfaction << ','
        << layout << ',' << rtsCts << ',' << maxThroughput << ',' << minMaxRatio << ','
        << meanDelay << ',' << worstDelay << ',' << meanLoss << ',' << devices << ',' << apStreams
        << ',' << staStreams << ',' << laptopProb << ',' << nDevices << ',' << nLaptops << ','
        << nPhones << ',' << (mixed ? laptopDemand : 0) << ',' << (mixed ? phoneDemand : 0) << ','
        << worstLaptopSat << ',' << worstPhoneSat << ',' << jainLaptops << ',' << jainPhones
        << '\n';

    // one row per STA; student = STA number, studentId = whose device it is
    auto perStudentCsv = OpenCsv(perStudentCsvPath,
                                 "standard,nStations,direction,rateManager,farDistance,mcs,ofdma,"
                                 "run,student,studentDistance,throughputMbps,demand,satisfaction,"
                                 "layout,delayMs,lossPct,devices,studentId,deviceType,streams");
    for (std::size_t i = 0; i < nDevices; i++)
    {
        perStudentCsv << standard << ',' << nStations << ',' << direction << ',' << rateManager
                      << ',' << farDistance << ',' << mcs << ',' << ofdmaStr << ',' << run << ','
                      << i + 1 << ',' << staDistances[i] << ',' << staMbps[i] << ','
                      << devs[i].demand << ',' << satisfaction[i] << ',' << layout << ','
                      << delayMs[i] << ',' << lossPct[i] << ',' << devices << ','
                      << devs[i].student + 1 << ',' << devs[i].type << ',' << devs[i].streams
                      << '\n';
    }
    return 0;
}
