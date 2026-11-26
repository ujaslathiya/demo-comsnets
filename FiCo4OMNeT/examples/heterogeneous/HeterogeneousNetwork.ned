package fico4omnet.examples.heterogeneous;

import fico4omnet.bus.can.CanBus;
import fico4omnet.nodes.can.CanNode;
import fico4omnet.nodes.can.CanNodeBridge;
import fico4omnet.nodes.can.CanGateway;

network HeterogeneousNetwork
{
    parameters:
        @display("bgb=900,850,white");
    submodules:
        // ===================================
        // BUSES
        // ===================================
        busStd: CanBus {
            parameters:
                @display("p=300,300");
                numGates = 3;
        }
        
        busFd: CanBus {
            parameters:
                @display("p=600,300");
                numGates = 3;
        }
        
        busStd2: CanBus {
            @display("p=100,600");
        }
        
        busFd2: CanBus {
            @display("p=800,600");
        }
        
        // ===================================
        // GATEWAY
        // ===================================
        gateway: CanGateway {
            @display("p=450,300");
        }
        
        // ===================================
        // NODES
        // ============================================================
        
        // Standard CAN Nodes
        canStdNode1: CanNode {
            @display("p=150,200;i=device/device_s,green");
        }
        
        // Bridge Node (Visual Wrapper for 2 nodes)
        canStdNode2: CanNodeBridge {
            @display("p=150,450;i=device/device_s,green");
        }
        
        canStdNode3: CanNode {
            @display("p=150,700;i=device/device_s,green");
        }
        
        // CAN-FD Nodes
        canFdNode1: CanNode {
            @display("p=750,200;i=device/device_s,gold");
        }
        
        // Bridge Node (Visual Wrapper for 2 nodes)
        canFdNode2: CanNodeBridge {
            @display("p=750,450;i=device/device_s,gold");
        }
        
        canFdNode3: CanNode {
            @display("p=750,700;i=device/device_s,gold");
        }
        
    connections:
        // Main Bus
        busStd.gate[0] <--> canStdNode1.gate;
        busStd.gate[1] <--> canStdNode2.gate[0];
        busStd.gate[2] <--> gateway.gateStd;
        
        busFd.gate[0] <--> canFdNode1.gate;
        busFd.gate[1] <--> canFdNode2.gate[0];
        busFd.gate[2] <--> gateway.gateFd;
        
        // Extension Bus
        busStd2.gate[0] <--> canStdNode2.gate[1];
        busStd2.gate[1] <--> canStdNode3.gate;
        
        busFd2.gate[0] <--> canFdNode2.gate[1];
        busFd2.gate[1] <--> canFdNode3.gate;
}
