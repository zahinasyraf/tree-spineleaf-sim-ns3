/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

// Define an object to create a dumbbell topology.

#ifndef POINT_TO_POINT_STAR_HELPER_H
#define POINT_TO_POINT_STAR_HELPER_H

#include <string>

#include "point-to-point-helper.h"
#include "ipv4-address-helper.h"
#include "internet-stack-helper.h"
#include "ns3/object.h"
#include "ns3/ptr.h"

namespace ns3 {

  /**
   * \defgroup pointtopointlayout Point-to-Point Layout Helpers
   *
   */

  /**
   * \ingroup pointtopointlayout
   *
   * \brief A helper to make it easier to create a star topology
   * with PointToPoint links
   */
  
  /**
   * \brief A helper to make it easier to create a star topology
   * with PointToPoint links
   */
class PointToPointStarHelper : public Object
{
public:

  /**
   * Create a PointToPointStarHelper in order to easily create
   * star topologies using p2p links
   *
   * \param numSpokes the number of links attached to 
   *        the hub node, creating a total of 
   *        numSpokes + 1 nodes
   *
   * \param p2pHelper the link helper for p2p links, 
   *        used to link nodes together
   */
  PointToPointStarHelper (uint32_t numSpokes, 
                          PointToPointHelper p2pHelper);

  /**
   * Create a PointToPointStarHelper in order to easily create
   * star topologies using p2p links.This constructor takes an existent node "hub", 
   * creates the specified number of spoke nodes (numSpokes) around it and installs
   * point-to-point links between the hub and the spoke nodes
   *
   * \param hub the NodeContainer containing the already-created hub Node
   *
   * \param numSpokes the number of links attached to 
   *        the hub node, creating a total of 
   *        numSpokes + 1 nodes
   *
   * \param p2pHelper the link helper for p2p links, 
   *        used to link nodes together
   */
  PointToPointStarHelper (NodeContainer hub,uint32_t numSpokes,PointToPointHelper p2pHelper);

  /**
   * Create a PointToPointStarHelper (Empty constructor)
   */
  PointToPointStarHelper ();

  /**
   * Destructor
   */
  virtual ~PointToPointStarHelper ();

public:
  /**
   * \returns a node pointer to the hub node in the
   *          star, i.e., the center node
   */
  Ptr<Node> GetHub () const;

  /**
   * \param i an index into the spokes of the star
   *
   * \returns a node pointer to the node at the indexed spoke
   */
  Ptr<Node> GetSpokeNode (uint32_t i) const;

  /**
   * \returns a node Container containing the set of spoke nodes
   */
  NodeContainer GetSpokeNodes() const;

  /**
   * \brief  Get a NetDevice on the Hub Node
   * \param i index into the NetDeviceContainer on the Hub Node
   * \returns a Ptr to the i'th NetDevice on the Hub Node
   *
   */
  Ptr <NetDevice> GetHubNetDevice (uint32_t i) const;

  /**
   * \brief Get a NetDevice from a given Spoke Node
   * \param i index into the NetDeviceContainer containing the NetDevices
   *  on all Spoke Nodes 
   *
   * \returns a Ptr to the NetDevice on the i'th Spoke Node
   *
   */
  Ptr <NetDevice> GetSpokeNetDevice (uint32_t i) const;

  /**
   * \param i index into the hub interfaces
   *
   * \returns Ipv4Address according to indexed hub Interface
   */
  Ipv4Address GetHubIpv4Address (uint32_t i) const;

  /**
   * \param i index into the spoke interfaces
   *
   * \returns Ipv4Address according to indexed spoke Interface
   */
  Ipv4Address GetSpokeIpv4Address (uint32_t i) const;

  /**
   * \returns the total number of spokes in the star
   */
  uint32_t SpokeCount () const;

  /**
   * \param stack an InternetStackHelper which is used to install 
   *              on every node in the star
   */
  void InstallStack (InternetStackHelper stack);

  /**
   * Assigns Ipv4 addresses for the interfaces between the hub and 
   * all spoke nodes
   * \param address an Ipv4AddressHelper which is used to install 
   *                Ipv4 addresses on all the node interfaces in 
   *                the star
   */
  void AssignIpv4Addresses (Ipv4AddressHelper address);

  /**
   * Assigns Ipv4 addresses for the interfaces between the hub and
   * a give spoke node
   *
   * \param address an Ipv4AddressHelper which is used to install 
   *                Ipv4 addresses on all the node interfaces in 
   *                the star
   *
   * \param spoke_id Id of the spoke node. Spoke nodes are zero-indexed
   *
   */
  void AssignIpv4AddressForSingleSpoke (Ipv4AddressHelper address, uint32_t spoke_id);

  /**
   * Sets up the node canvas locations for every node in the star. 
   * This is needed for use with the animation Interface
   *
   * \param ulx upper left x value
   * \param uly upper left y value
   * \param lrx lower right x value
   * \param lry lower right y value
   */
  void BoundingBox (double ulx, double uly, double lrx, double lry);

   /**
   * \brief Get the type ID.
   * \return type ID
   */
  static TypeId GetTypeId ();

private:
  NodeContainer m_hub;
  NetDeviceContainer m_hubDevices;
  NodeContainer m_spokes;
  NetDeviceContainer m_spokeDevices;
  Ipv4InterfaceContainer m_hubInterfaces;
  Ipv4InterfaceContainer m_spokeInterfaces;
};

} // namespace ns3

#endif /* POINT_TO_POINT_STAR_HELPER_H */

