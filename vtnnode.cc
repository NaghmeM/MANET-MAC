//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2003 Ahmet Sekercioglu
// Copyright (C) 2003-2009 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include <algorithm>
#include <math.h>
#include<iostream>
#include<fstream>
#include <stdlib.h>
#include "inet\common\ModuleAccess.h"
#include "inet\mobility\contract\IMobility.h"



#include "reqiotext_m.h"


#include "Aodvn.h"

using namespace std;


const short num_node=10;
const short num_color=10;
double node_inf[num_node][9];//x//y//token//num1_hope//num2_hope//node_lev//nid//priority//color
//struct NAFT
//{
//    double unitdisk
//};
short net_inf[num_node][num_node];
double rang=400;
short max_pro=5;
short max_ser=20;
short min_ser=3;
short max_tok=255;
double delay_time=0.1;
short cur_ser=10;
short num_pack=0;


using namespace omnetpp;


class vntnode : public cSimpleModule
{
    double num_test;
    simsignal_t num_msg;
    short num_reap;
    double stv;


  protected:
    virtual Sdnmsg *generateMessage(short reciv);
    virtual void forwardMessage(Sdnmsg *msg);
    virtual void initialize() ;
    virtual void handleMessage(cMessage *msg);
    virtual void find_loc(short);
    virtual void find1_hope();
    virtual void find2_hope();
    virtual void node_level();
    virtual void ser_load();
    virtual bool isValid(short,short[],short);
    virtual bool graphColoring(short[],short,double **);
    virtual short * checkSolution(double **);
    virtual double ** sort_ar(double[][9],short,short,short);
    virtual bool state_node(short);
    virtual bool state_color(short,short);

};

Define_Module(vntnode);

void vntnode::initialize()
{
    num_reap=0;
    cSimulation *sim=getSimulation();
    cModule *nod=sim->getModule(this->getId());
    cModule *node=inet::getContainingNode(nod);
    short id_node=node->getId()-2;
    Sdnmsg *cp=new Sdnmsg();
    ser_load();
    for (short i=0;i<num_node;i++)
    {
       node_inf[i][6]=i;
    }
    send(cp,"gat$o",0);
}
void vntnode::handleMessage(cMessage *msg)
{
    num_msg=registerSignal("num_msg");
    num_test++;
    cSimulation *sim=getSimulation();
    cModule *nod=sim->getModule(this->getId());
    cModule *node=inet::getContainingNode(nod);
    short id_node=node->getId()-2;

    //stv=simTime().dbl();
    //emit(num_msg,(clock()-stv)/10000);
    //stv=clock();
    emit(num_msg,(double) num_pack);
    Sdnmsg *ttmsg = check_and_cast<Sdnmsg *>(msg);
    forwardMessage(ttmsg);
}
Sdnmsg *vntnode::generateMessage(short reciv)
{

    short src =getId();
    //EV<<src<<"=SRC\n";
    short dest = reciv;

    char msgname[20];
    sprintf(msgname, "sdn-%d-to-%d", src, dest);

    Sdnmsg *msg=new Sdnmsg(msgname);
    msg->setSource(src);
    msg->setDestination(dest);
    return msg;
}
void vntnode::forwardMessage(Sdnmsg *msg)
{
    cSimulation *sim=getSimulation();
    cModule *nod=sim->getModule(this->getId());
    cModule *node=inet::getContainingNode(nod);
    short id_node=node->getId()-2;
    short msg_state=msg->getMsg_state();
    if(msg_state)//start net
    {
        msg->setMsg_state(0);
    }
    find_loc(id_node);
    find1_hope();
    find2_hope();
    node_level();

    /*for (short i=0;i<num_node;i++)
    {
        EV<<'\n';
        for (short j=0;j<9;j++)
        {
            EV<<" "<<node_inf[i][j];
        }
    }*/
    double ** tmp=sort_ar(node_inf,num_node,5,3);
    /*EV<<"then sort"<<'\n';
    for (short i=0;i<num_node;i++)
    {
        EV<<'\n';
        for (short j=0;j<9;j++)
        {
            EV<<" "<<tmp[i][j];;
        }
    }*/
    short *per_send=checkSolution(tmp);
    for (short i=0;i<num_node;i++)
    {
        node_inf[(short)tmp[i][6]][8]=per_send[i];
        //EV<<"ind="<<tmp[i][6]<<"  per="<<per_send[i]<<'\n';
    }
    cGate *ga=gate("gat$o" ,0);
    short id_dest=ga->getPathEndGate()->getOwnerModule()->getParentModule()->getId()-2;

    if(net_inf[id_node][id_dest]==num_node+1)
    {
        if(state_node(id_node) || state_color(id_dest, id_node))
        {
            node_inf[id_node][7]=0;
            cur_ser--;
            num_pack++;
            send(msg,"gat$o" ,0);
            if(cur_ser<0)
                ser_load();
        }
        else
        {
            scheduleAfter(delay_time, msg);
        }
    }
    else
    {
        scheduleAfter(delay_time, msg);
    }
    //EV<<" sender_id="<<ga->getId()<<" dest_mod="<<id_dest<<'\n';
}
void vntnode::find_loc(short idn)
{
    cSimulation *sim=getSimulation();
    cModule *nod=sim->getModule(this->getId());
    cModule *node=inet::getContainingNode(nod);
    inet::IMobility  *mod = check_and_cast<inet::IMobility *>(node->getSubmodule("mobility"));
    inet::Coord pos = mod->getCurrentPosition();

    node_inf[idn][0]= pos.getX();
    node_inf[idn][1]=pos.getY();
}
void vntnode::find1_hope()
{
    for (short i=0;i<num_node;i++)
    {
        for (short j=0;j<num_node;j++)
        {
            net_inf[i][j]=0;
        }
    }
    for (short i=0;i<num_node;i++)
    {
        short num1_hope=0;
        for (short j=0;j<num_node;j++)
        {
            double dist=sqrt(pow((node_inf[i][0]-node_inf[j][0]),2)+pow((node_inf[i][1]-node_inf[j][1]),2));
            if(dist<=rang && i!=j)
            {
                net_inf[i][j]=num_node+1;
                num1_hope++;
            }
        }
        node_inf[i][3]=num1_hope;
    }
}
void vntnode::find2_hope()
{
    for (short i=0;i<num_node;i++)
    {
        short num2_hope=0;
        for (short j=0;j<num_node;j++)
        {
            if (net_inf[i][j]==num_node+1)
            {
                for (short k=0;k<num_node;k++)
                {
                    if(net_inf[j][k]==num_node+1 && net_inf[i][k]==0 && i!=k)
                    {
                        net_inf[i][k]=j+1;
                        num2_hope++;
                    }
                }
            }
        }
        node_inf[i][4]=num2_hope;
    }
}
void vntnode::node_level()
{
    for (short i=0;i<num_node;i++)
    {
        node_inf[i][5]=node_inf[i][3]*2+node_inf[i][4];
    }
}
void vntnode::ser_load()
{
    cur_ser=10;
    num_pack=0;
    for (short i=0;i<num_node;i++)
    {
        node_inf[i][7]=intuniform(1, max_pro);
        node_inf[i][2]=intuniform(0, max_tok);
    }
}
bool vntnode::isValid(short v,short color[num_color],short c)
{
    for (int i = 0; i < num_node; i++)
       if (net_inf[v][i]==num_node+1 && c == color[i])
          return false;
    return true;
}
bool vntnode::graphColoring(short color[num_color],short vertex,double ** node_tmp)
{
    if (vertex == num_node)    //when all vertices are considered
       return true;

    for (int col = 1; col <= num_color; col++)
    {
       if (isValid( (short)node_tmp[vertex][6],color, col)) //check whether color col is valid or not
       {
          color[(short) node_tmp[vertex][6]] = col;
          if (graphColoring (color, vertex+1,node_tmp) == true)    //go for additional vertices
             return true;/**/

          color[(short)node_tmp[vertex][6]] = 0;
       }
    }
    return false; //when no colors can be assigned
}
short * vntnode::checkSolution(double ** node_tmp )
{
    short *color = new short[num_node];    //make color matrix for each vertex

    for (int i = 0; i < num_node; i++) //initially set to 0
       color[i] = 0;

    if (graphColoring(color, 0,node_tmp) == false)  //for vertex 0 check graph coloring
    {
       EV << "Solution does not exist.";
       for (int i = 0; i < num_node; i++) //initially set to 0
          color[i] = 0;
       return color;
    }
    return color;
}
double ** vntnode::sort_ar(double ary[][9],short num_row,short col_sort,short col_sort2)//if need 1 factor to sort, col_sort2=-1
{
    short num_col=sizeof(ary[0])/sizeof (ary[0][0]) ;
    double **ar= new double * [num_row];
    for ( short i = 0; i < num_row; i++ ) ar[i] = new double [num_col];
    for (short i=0;i<num_row;i++)
    {
        for (short j=0;j<num_col;j++)
        {
            ar[i][j]=ary[i][j];
        }
    }

    for (short i=0;i<num_row;i++)
    {
        for (short j=0;j<num_row-1;j++)
        {
            if(ar[j][col_sort]<ar[j+1][col_sort])
            {
                double temp[num_col];
                for (short k=0;k<num_col;k++)
                {
                    temp[k]=ar[j][k];
                    ar[j][k]=ar[j+1][k];
                    ar[j+1][k]=temp[k];
                }
            }
            if (col_sort2>=0)
            {
                if(ar[j][col_sort]==ar[j+1][col_sort] && ar[j][col_sort]<ar[j+1][col_sort2])
                {
                    double temp[num_col];
                    for (short k=0;k<num_col;k++)
                    {
                        temp[k]=ar[j][k];
                        ar[j][k]=ar[j+1][k];
                        ar[j+1][k]=temp[k];
                    }
                }
            }
        }
    }
    return ar;
}
bool vntnode::state_node(short id_node)
{
        for(short i=0; i<num_node;i++)
        {
            if(net_inf[id_node][i]>0)
            {
                if(node_inf[id_node][7]<node_inf[i][7])
                    return false;
                else if(node_inf[id_node][7]==node_inf[i][7] && node_inf[id_node][2]>=node_inf[i][2])
                    return false;
            }
        }
    return true;
}
bool vntnode::state_color(short id_dest,short id_node)
{
   short max_per=-1;
   for(short i=0; i<num_node;i++)
   {
       if(net_inf[id_node][i]==num_node+1)
       {
            max_per=i;
            break;
       }
   }

   if( max_per==-1)
   {
       return false;
   }
   else
   {
       for(short i=0; i<num_node;i++)
       {
           if(net_inf[id_node][i]==num_node+1 && net_inf[i][7]>net_inf[max_per][7])
           {
                max_per=i;
           }
       }
       if(node_inf[id_dest][8]==node_inf[ max_per][8])
       {
           return true;
       }
       else
       {
           return false;
       }
   }

}
