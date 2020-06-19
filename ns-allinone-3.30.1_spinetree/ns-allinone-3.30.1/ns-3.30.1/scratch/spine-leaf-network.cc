#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/ipv4-flow-probe.h"
#include "ns3/csma-module.h"
#include "ns3/bgp.h"
#include "ns3/log.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SpinLeafNetworkSimulation");

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
    // Allow the user to override any of the defaults and the above DefaultValue::Bind ()s at run-time, via command-line arguments
    CommandLine cmd;
    uint32_t m_NumSpines = 3;
    uint32_t m_NumLeafs = 6;
    uint32_t m_NumEndHosts = 10;

    double m_NetworkWidth = 1000.0;
    double m_NetworkHeight = 1000.0;
    double m_LayerHeight = m_NetworkHeight / 4.0;

    //Time simTime = MilliSeconds (1050);

    //cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
    cmd.AddValue("nSpines", "Number of Spine Nodes", m_NumSpines);
    cmd.AddValue("nLeafs", "Number of Leaf Nodes", m_NumLeafs);
    cmd.AddValue("nEndHosts", "Number of End Host Nodes", m_NumEndHosts);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1500)); //MTU
    double simTime = m_NumEndHosts + 1;

    NodeContainer spineNodes;
    NodeContainer leafNodes;
    NodeContainer endHostNodes;
	
    spineNodes.Create(m_NumSpines);
    leafNodes.Create(m_NumLeafs);
    endHostNodes.Create(m_NumEndHosts);

    NodeContainer totalNodes;
    totalNodes.Add(spineNodes);
    totalNodes.Add(leafNodes);
    totalNodes.Add(endHostNodes);

    std::vector<NodeContainer> m_NodeFairList;
    std::vector<NetDeviceContainer> m_DeviceFairList;
    std::vector<Ipv4InterfaceContainer> m_Ipv4InterfaceList;

    for (uint32_t i = 0; i < spineNodes.GetN(); i++)
    {
        for (uint32_t j = 0; j < leafNodes.GetN(); j++)
        {
            // push_back function increases the spineNodes size by one in the topology.
			m_NodeFairList.push_back(NodeContainer(spineNodes.Get(i), leafNodes.Get(j)));
        }
    }

    for (uint32_t i = 0; i < leafNodes.GetN(); i++)
    {
        for (uint32_t j = 0; j < endHostNodes.GetN(); j++)
        {
            // push_back function increases the leafNodes size by one in the topology.
			m_NodeFairList.push_back(NodeContainer(leafNodes.Get(i), endHostNodes.Get(j)));  
        }
    }

    // create the internet stack
    InternetStackHelper internet;
    internet.Install(totalNodes); // install internet stack to all nodes

    // We create the channels first without any IP addressing information
    NS_LOG_INFO("Create channels.");
    PointToPointHelper p2p;  // create the P2P Helper
    p2p.SetDeviceAttribute("DataRate", StringValue("1000Mbps")); // setting DataRate aka transmission speed per node 
    p2p.SetChannelAttribute("Delay", StringValue("1ms")); //setting queue delay

    for (uint32_t i = 0; i < m_NodeFairList.size(); i++)
    {
        // group all nodes as single object to ease ip allocation and link
		m_DeviceFairList.push_back(p2p.Install(m_NodeFairList[i]));
    }

    // set the mobility model
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    double m_DistSpine = m_NetworkWidth / (m_NumSpines + 1);
    for (uint32_t i = 0; i < m_NumSpines; i++)
    {
        positionAlloc->Add(Vector(m_DistSpine * (i + 1), m_LayerHeight * 1, 0.0));
    }

    double m_DistLeaf = m_NetworkWidth / (m_NumLeafs + 1);
    for (uint32_t i = 0; i < m_NumLeafs; i++)
    {
        positionAlloc->Add(Vector(m_DistLeaf * (i + 1), m_LayerHeight * 2, 0.0));
    }

    double m_DistEndHost = m_NetworkWidth / (m_NumEndHosts + 1);
    for (uint32_t i = 0; i < m_NumEndHosts; i++)
    {
        positionAlloc->Add(Vector(m_DistEndHost * (i + 1), m_LayerHeight * 3, 0.0));
    }

    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(totalNodes);

    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;  //set ipv4 for dhcp pool ip addresses

    char buf[255] = {0};
    for (uint32_t i = 0; i < m_DeviceFairList.size(); i++)
    {
        // create dhcp pool
		memset(buf, 0, 255);
        sprintf(buf, "10.%d.%d.0", (i + 1) / 255 + 1, i + 1);
        ipv4.SetBase(buf, "255.255.255.0");
        m_Ipv4InterfaceList.push_back(ipv4.Assign(m_DeviceFairList[i]));
    }

    for (uint32_t i = 0; i < spineNodes.GetN(); i++)
    {
        // create bgp router config to app1.
		Ptr<Bgp> bgp_router = CreateObject<Bgp>();
        bgp_router->SetAttribute("RouterID", Ipv4AddressValue(m_Ipv4InterfaceList[i * m_NumLeafs].GetAddress(0)));

        Peer bgp_router_peer;
        bgp_router_peer.peer_address = m_Ipv4InterfaceList[i * m_NumLeafs].GetAddress(1);
        bgp_router_peer.peer_asn = 65001;
        bgp_router_peer.local_asn = 65000;
        bgp_router_peer.passive = false;

        // add bgp peer config to app1.
        bgp_router->AddPeer(bgp_router_peer);
        spineNodes.Get(i)->AddApplication(bgp_router);
    }

    for (uint32_t i = 0; i < leafNodes.GetN(); i++)
    {
        Ptr<Bgp> bgp_router = CreateObject<Bgp>();
        bgp_router->SetAttribute("RouterID", Ipv4AddressValue(m_Ipv4InterfaceList[i * m_NumSpines].GetAddress(1)));

        Peer bgp_router_peer;
        bgp_router_peer.peer_address = m_Ipv4InterfaceList[i * m_NumSpines].GetAddress(1);
        bgp_router_peer.peer_asn = 65001;
        bgp_router_peer.local_asn = 65000;
        bgp_router_peer.passive = false;

        // add the peer config to app1.
        bgp_router->AddPeer(bgp_router_peer);
        leafNodes.Get(i)->AddApplication(bgp_router);
    }

    // Create router nodes, initialize routing database and set up the routing tables in the nodes.
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    // p2p.EnablePcapAll("spine-leaf");

    NS_LOG_INFO("Create Applications.");
 
    // Create OnOff applications to send TCP to the sink node from each leaf node except last leaf node
    MalwareSendHelper malwareHelper("ns3::TcpSocketFactory", Address());

    uint16_t port = 50000;
    Address sinkAddr(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", sinkAddr);
    ApplicationContainer sinkApp = packetSinkHelper.Install(totalNodes);
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simTime));

    for (uint32_t i = 0; i < endHostNodes.GetN(); i++)
    {
        ApplicationContainer clientApps;
        Ptr<Ipv4> ipv4 = endHostNodes.Get(i)->GetObject<Ipv4>();
        Ipv4Address ipAddr = ipv4->GetAddress(1, 0).GetLocal();
        AddressValue remoteAddress(InetSocketAddress(ipAddr, port));
        malwareHelper.SetAttribute("Remote", remoteAddress);
        clientApps.Add(malwareHelper.Install(endHostNodes.Get(0)));
        clientApps.Start(Seconds(i));
        clientApps.Stop(Seconds(simTime));
    }

    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/0/$ns3::PacketSink/Rx", MakeCallback(&SinkRx));

    // create animation file for NetAnim
    AnimationInterface anim("spine-leaf-network.xml");

    for (uint32_t i = 0; i < totalNodes.GetN(); i++)
    {
        // set the node size in NetAnim
        anim.UpdateNodeSize(totalNodes.Get(i)->GetId(), 20, 20);
    }

    // Flow Monitor Installation
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(Seconds(simTime + 1));
    Simulator::Run();
    NS_LOG_INFO("Done.");

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

    m_ThroughPut = (m_RxDataSize * 8 / m_DelayTime / 1024 / 1024)*-1 ;
   
    //std::cout << "Delay: " << m_DelayTime * 1000 << " ms" << std::endl;
    //std::cout << "TxPackets Size: " << m_TxPackets << " bytes" << std::endl;
    //std::cout << "RxPackets Size: " << m_RxPackets << " bytes" << std::endl;

    std::cout << "Number of Spines: " << m_NumSpines << std::endl;
    std::cout << "Number of Leafs: " << m_NumLeafs << std::endl;
    std::cout << "Number of hosts: " << m_NumEndHosts << std::endl;
    std::cout << "Throughput: " << m_ThroughPut << " Mbps" << std::endl;
    std::cout << "Average delay: " << (m_DelayTime / m_count) * 1000 << " ms" << std::endl;
    std::cout << "PDR: " << (m_RxPackets / m_TxPackets) * 100 << " %" << std::endl;
    std::cout << "Packet loss: " << m_TxPackets - m_RxPackets << " TCP packets" << std::endl;


    Simulator::Destroy();

    return 0;
}
