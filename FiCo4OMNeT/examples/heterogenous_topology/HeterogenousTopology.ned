package fico4omnet.examples.heterogenous_topology;

import fico4omnet.bus.can.CanBus;
import fico4omnet.nodes.can.CanNode;
import fico4omnet.nodes.can.CanGateway;

network HeterogenousTopology
{
    parameters:
        @display("bgb=1200,600,white");
    submodules:
        // ===================================
        // BUSES
        // ===================================
        // 1. Legacy Domain Bus (Left)
        busLegacy: CanBus {
            @display("p=250,300;is=12;t=Classical CAN,b");
        }
        
        // 2. Backbone Bus (Center - High Speed)
        // Simulating the "Ethernet/TSN Backbone" as a high-speed link
        busBackbone: CanBus {
            @display("p=550,300;is=12;t=CAN-FD,b");
        }
        
        // 3. Actuator Domain Bus (Right)
        busActuator: CanBus {
            @display("p=850,300;is=12;t=CAN-FD,b");
        }
        
        // ===================================
        // GATEWAYS
        // ===================================
        gateway1: CanGateway {
            @display("p=400,300;i=device/router,red,is=80"); // Red as in diagram
        }
        
        gateway2: CanGateway {
            @display("p=700,300;i=device/router,red,is=80"); // Red as in diagram
        }
        
        // ===================================
        // NODES
        // ===================================
        // Sensor Node (Legacy CAN)
        sensorNode: CanNode {
            @display("p=100,300;i=device/device_s,black,is=80");
        }
        
        // Actuator Node (CAN-FD)
        actuatorNode: CanNode {
            @display("p=1000,300;i=device/device_s,black,is=80");
        }
        
    connections:
        // Left Domain: Sensor -> Legacy Bus -> Gateway1 (Std Port)
        sensorNode.gate <--> busLegacy.gate[0];
        busLegacy.gate[1] <--> gateway1.gateStd;
        
        // Backbone: Gateway1 (FD Port) -> Backbone Bus -> Gateway2 (FD Port)
        gateway1.gateFd <--> busBackbone.gate[0];
        busBackbone.gate[1] <--> gateway2.gateFd;
        
        // Right Domain: Gateway2 (Std Port) -> Actuator Bus -> Actuator
        // Note: Using gateStd on Gateway2 to connect to the right side, 
        // as the FD port is used for the high-speed backbone.
        gateway2.gateStd <--> busActuator.gate[0];
        busActuator.gate[1] <--> actuatorNode.gate;
}
