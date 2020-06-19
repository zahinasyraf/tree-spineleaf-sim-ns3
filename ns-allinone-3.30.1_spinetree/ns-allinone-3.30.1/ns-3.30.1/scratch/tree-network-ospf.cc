
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-probe.h"
#include "ns3/log.h"
#include "ns3/ipv4-ospf-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TreeNetworkSimulation");

void SinkRx(Ptr<const Packet> p, const Address &from)
{
    Ptr<Packet> packet = p->Copy();
    SeqTsHeader seqTs;
    packet->RemoveHeader(seqTs);

    if (seqTs.GetTs() > 0)
    {
        NS_LOG_UNCOND("TraceDelay: RX " << packet->GetSize() << " bytes from " << InetSocketAddress::ConvertFrom(from).GetIpv4() << " Uid: " << packet->GetUid() << " TXtime: " << seqTs.GetTs().GetSeconds() << " RXtime: " << Simulator::Now().GetSeconds());
    }
}

int main(int argc, char *argv[])
{
    uint32_t nLevels = 4;   // Number of levels in the tree
    uint32_t nBranches = 6; // Number of branches

    CommandLine cmd;
    cmd.AddValue("nLevels", "Number of levels in the tree", nLevels);
    cmd.AddValue("nBranches", "Fan out", nBranches);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1500)); //MTU

    NS_LOG_INFO("Build tree topology.");
    PointToPointHelper pointToPointHelper;
    pointToPointHelper.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
    pointToPointHelper.SetChannelAttribute("Delay", StringValue("1ms"));
    Ptr<PointToPointTreeHelper> tree = CreateObject<PointToPointTreeHelper>(nLevels, nBranches, pointToPointHelper);
    double simTime = tree->GetNLeaves();

    NS_LOG_INFO("Install internet stack on all nodes.");

    InternetStackHelper stack;
    tree->InstallStack(stack);

    NS_LOG_INFO("Assign IP Addresses.");
    tree->AssignIpv4Address(Ipv4Address("10.0.0.0"), Ipv4Mask("255.0.0.0"));

    NS_LOG_INFO("Create applications.");

    MalwareSendHelper malwareHelper("ns3::TcpSocketFactory", Address());
    uint16_t port = 50000;
    Address sinkAddr(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", sinkAddr);
    for (uint32_t i = 0; i < nLevels; i++)
    {
        ApplicationContainer sinkApp = packetSinkHelper.Install(tree->GetAllNodesForLevel(i));
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(simTime));
    }

    for (uint32_t i = 0; i < tree->GetNLeaves(); i++)
    {
        ApplicationContainer clientApps;
        Ptr<Ipv4> ipv4 = tree->GetLeaf(i)->GetObject<Ipv4>();
        Ipv4Address ipAddr = ipv4->GetAddress(1, 0).GetLocal();
        AddressValue remoteAddress(InetSocketAddress(ipAddr, port));
        malwareHelper.SetAttribute("Remote", remoteAddress);
        clientApps.Add(malwareHelper.Install(tree->GetNode(0, 0)));
        clientApps.Start(Seconds(i));
        clientApps.Stop(Seconds(simTime));
    }

    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/0/$ns3::PacketSink/Rx", MakeCallback(&SinkRx));

    // pointToPointHelper.EnablePcapAll("tree");
    Ipv4OSPFRoutingHelper::PopulateRoutingTables();

    // Set the bounding box for animation
    tree->BoundingBox(0, 0, 1000, 1000);

    // Create the animation object and configure for specified output
    AnimationInterface anim("tree-simulation.xml");

    // Flow Monitor
    FlowMonitorHelper flowmon;

    // install the flow monitor
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(Seconds(simTime + 1));
    Simulator::Run();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    uint32_t m_TxPackets = 0, m_TxDataSize = 0, m_RxPackets = 0, m_RxDataSize = 0, m_count = 0;
    double m_ThroughPut = 0.0, m_DelayTime = 0.0, m_JitterTime = 0.0;
    for (std::map<FlowId, FlowMonitor::FlowStats>::iterator it = stats.begin(); it != stats.end(); it++)
    {
        if (it->second.rxPackets > 1)
        {
            m_TxPackets += it->second.txPackets;
            m_TxDataSize += it->second.txBytes;
            m_RxPackets += it->second.rxPackets;
            m_RxDataSize += it->second.rxBytes;

            m_DelayTime += it->second.delaySum.GetSeconds() / (it->second.rxPackets);
            m_JitterTime += it->second.jitterSum.GetSeconds() / (it->second.rxPackets - 1);
            m_count++;
        }
    }

    m_ThroughPut = m_RxDataSize * 8 / m_DelayTime / 1024 / 1024;

    std::cout << "Throughput: " << m_ThroughPut << " Mbps" << std::endl;
    std::cout << "Mean delay: " << m_DelayTime / m_count *1000 << " ms" << std::endl;
    std::cout << "PDR: " << m_RxPackets * 100.0 / m_TxPackets << " %" << std::endl;

    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}
