package multiprotocol_gateway;

import inet.linklayer.contract.IEtherMac;
import fico4omnet.linklayer.can.CanPort;

// Central Gateway connecting heterogeneous bus networks to Automotive Ethernet backbone.
// Emits queue-depth, queuing-delay, drop-count, and uplink-utilisation signals.
simple MultiGateway
{
    parameters:
        @display("i=device/router");
        int numCanPorts    = default(1);
        int numCanFdPorts  = default(1);
        int numLinPorts    = default(1);
        int numEthPorts    = default(1);

        bool enableFragmentation = default(true);
        bool enableAggregation   = default(true);

        double processingDelay @unit(s) = default(0.001s);
        int    maxQueueSize            = default(100);

        // ── Statistics signals ─────────────────────────────────────────────
        @signal[canInputQueueLength](type=long);
        @signal[canFdInputQueueLength](type=long);
        @signal[linInputQueueLength](type=long);
        @signal[canQueuingDelay](type=double);
        @signal[canFdQueuingDelay](type=double);
        @signal[linQueuingDelay](type=double);
        @signal[canE2EDelay](type=double);
        @signal[canFdE2EDelay](type=double);
        @signal[linE2EDelay](type=double);
        @signal[ethE2EDelay](type=double);
        @signal[inputDropCount](type=long);
        @signal[processingDelay](type=double);
        @signal[uplinkUtil](type=double);

        @statistic[canInputQueueLength](title="CAN input queue length";
            source=canInputQueueLength; record=mean,max,vector;
            interpolationmode=sample-hold; unit=frames);
        @statistic[canFdInputQueueLength](title="CAN-FD input queue length";
            source=canFdInputQueueLength; record=mean,max,vector;
            interpolationmode=sample-hold; unit=frames);
        @statistic[linInputQueueLength](title="LIN input queue length";
            source=linInputQueueLength; record=mean,max,vector;
            interpolationmode=sample-hold; unit=frames);
        @statistic[canQueuingDelay](title="CAN gateway queuing delay";
            source=canQueuingDelay; record=mean,max,vector; unit=us);
        @statistic[canFdQueuingDelay](title="CAN-FD gateway queuing delay";
            source=canFdQueuingDelay; record=mean,max,vector; unit=us);
        @statistic[linQueuingDelay](title="LIN gateway queuing delay";
            source=linQueuingDelay; record=mean,max,vector; unit=us);
        @statistic[canE2EDelay](title="CAN e2e delay (ECU to GW Ethernet departure)";
            source=canE2EDelay; record=mean,max,min,vector; unit=us);
        @statistic[canFdE2EDelay](title="CAN-FD e2e delay (ECU to GW Ethernet departure)";
            source=canFdE2EDelay; record=mean,max,min,vector; unit=us);
        @statistic[linE2EDelay](title="LIN e2e delay (ECU to GW Ethernet departure)";
            source=linE2EDelay; record=mean,max,min,vector; unit=us);
        @statistic[ethE2EDelay](title="ETH e2e delay (node send to GW receive)";
            source=ethE2EDelay; record=mean,max,min,vector; unit=us);
        @statistic[inputDropCount](title="Gateway input drop count";
            source=inputDropCount; record=last,vector);
        @statistic[processingDelay](title="Gateway processing delay";
            source=processingDelay; record=mean,vector; unit=us);
        @statistic[uplinkUtil](title="Gateway uplink utilisation";
            source=uplinkUtil; record=mean,timeavg; unit=percent);

    gates:
        inout canPort[numCanPorts];
        inout canFdPort[numCanFdPorts];
        inout linPort[numLinPorts];
        inout ethg[numEthPorts];
}
