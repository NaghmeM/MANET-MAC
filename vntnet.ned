

import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;
import inet.common.geometry.common.SimpleGeographicCoordinateSystem;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;


simple vntnode
{
    parameters:

        @signal[num_msg](type=double);
        @statistic[num_msg](title=" num msg"; record=vector?,count?);
        volatile int posX = intuniform (0,1000);
        volatile int posY = intuniform (0,1000);

    gates:
        inout gat[];

}
module vt extends MobileHost
{
    parameters:
        int ownIndex;
        @display("i=misc/smartphone");
    gates:
        inout gate[];
    submodules:
        vtn[sizeof(gate)]: vntnode {
            @display("p=$posX,$posY");
        }
    connections allowunconnected:

        for i=0..sizeof(gate)-1 {
            vtn[i].gat++ <--> gate++;
        }
        for i=0..sizeof(gate)-1, for j=i+1..sizeof(gate)-1 {
            vtn[i].gat++ <--> vtn[j].gat++;
        }
}

//
// TODO auto-generated type
//
network Vntnet
{
    parameters:
        int num_node;

        @display("bgb=1000,1000");


        bool hasVisualizer = default(false);

    types:
        channel Channel extends ned.DelayChannel
        {
            delay = 100ms;
            @display("ls=black,0");
        }
    submodules:

        vtn_node[num_node]: vt {
            parameters:
                @display("i=misc/smartphone","p=$posX,$posY");
                ownIndex = index;
        }


        visualizer: IntegratedCanvasVisualizer {
            parameters:
                @display("p=1000,1000");
        }
        coordinateSystem: SimpleGeographicCoordinateSystem {
            parameters:
                @display("p=1000,1000");
        }

    connections allowunconnected:

        for i=0..num_node-1, for j=i+1..num_node-1 {
            vtn_node[i].gate++ <--> Channel <--> vtn_node[j].gate++;
        }

}

