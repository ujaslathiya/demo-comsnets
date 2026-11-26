//Copyright (c) 2014, CoRE Research Group, Hamburg University of Applied Sciences
//All rights reserved.
//Modified for CAN-FD support

package fico4omnet.bus.can;

import fico4omnet.bus.BusPort;
import fico4omnet.bus.can.CanBusLogic;

//
// Central unit of the CAN-Network
//
// Forwards data-frames and error-frames to all participating nodes of the network
// Modified to support CAN-FD with dual bitrates
//
// @author Stefan Buschmann
//
module CanBus
{
    parameters:
        @display("i=block/network");
        
        //bandwidth of the bus (nominal/arbitration phase for CAN-FD)
        double bandwidth @unit(bps) = default(1Mbps);
        
        //CAN-FD data phase bitrate (for BRS mode)
        double dataBitrate @unit(bps) = default(5Mbps);
        
        //Version of CAN-Protocol. 2.0A = 11 Bits identifier, 2.0B = 29 Bits identifier
        string version @enum("2.0A", "2.0B") = default("2.0A");
        
        //Value for the percentage distribution for bit stuffing. Valid values 0 to 1.
        double bitStuffingPercentage = default(0);

        //Number of nodes connected to this bus
        int numGates = default(2);

    gates:
        inout gate[numGates];

    submodules:
        busPort: BusPort {
            @display("p=163,74");
            gates:
                phygate[parent.numGates];
        }

        canBusLogic: CanBusLogic {
            @display("p=52,74");
        }

    connections allowunconnected:
        busPort.innerGate <--> canBusLogic.gate;
        for i=0..numGates-1 {
            busPort.phygate[i] <--> gate[i];
        }
}
