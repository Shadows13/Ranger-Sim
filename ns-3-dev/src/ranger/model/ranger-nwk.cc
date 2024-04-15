/*
 * Copyright (c) 2011 The Boeing Company
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
 * Author:
 *  Gary Pei <guangyu.pei@boeing.com>
 *  Sascha Alexander Jopen <jopen@cs.uni-bonn.de>
 */
#include "ranger-nwk.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("RangerNwk");
NS_OBJECT_ENSURE_REGISTERED(RangerNwk);

TypeId
RangerNwk::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RangerNwk").SetParent<Object>().SetGroupName("Ranger");
    return tid;
}

RangerNwk::RangerNwk()
{
}


RangerNwk::~RangerNwk()
{
}



} // namespace ns3