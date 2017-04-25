#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<netinet/udp.h>
#include<netinet/tcp.h>
#include<netinet/ip.h>
#include<netinet/if_ether.h>
#include<net/ethernet.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include<sys/types.h>
#include<unistd.h>

FILE *outpt;
struct sockaddr_in source,dest;
int tcp=0,udp=0,tot=0,i,j;

void PacketProc(unsigned char* buff, int size);
void EthHeaderPrint(unsigned char* buff, int Size);
void IPHeaderPrint(unsigned char* buff, int Size);
void PacketTCPPrint(unsigned char* buff, int Size);
void PacketUDPPrint(unsigned char *buff , int Size);
void dataPrint (unsigned char* data , int sz);

void PacketProc(unsigned char* buff, int size)
{
    struct iphdr *iph = (struct iphdr*)(buff + sizeof(struct ethhdr));
    ++tot;
    int prot=iph->protocol;
    if(prot==6){
        tcp++;
        PacketTCPPrint(buff , size);
    }
    else if(prot==17){
        udp++;
        PacketUDPPrint(buff , size);
    }
    else{
    }
    printf("UDP : %d   TCP : %d   tot : %d\r", udp , tcp, tot);
}

void EthHeaderPrint(unsigned char* buff, int Size)
{
    struct ethhdr *eth = (struct ethhdr *)buff;
    fprintf(outpt , "\n");
    fprintf(outpt , "Ethernet Header Detail - \n");
    fprintf(outpt , "   Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_dest[0] , eth->h_dest[1] , eth->h_dest[2] , eth->h_dest[3] , eth->h_dest[4] , eth->h_dest[5] );
    fprintf(outpt , "   Source Address      : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_source[0] , eth->h_source[1] , eth->h_source[2] , eth->h_source[3] , eth->h_source[4] , eth->h_source[5] );
}

void IPHeaderPrint(unsigned char* buff, int Size)
{
    EthHeaderPrint(buff , Size);
    unsigned short ipln;
    struct iphdr *iph = (struct iphdr *)(buff  + sizeof(struct ethhdr) );
    ipln =iph->ihl*4;
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = iph->saddr;
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;
    fprintf(outpt , "\n");
    fprintf(outpt , "IP Header\n");
    fprintf(outpt , "   IP Version        : %d\n",(unsigned int)iph->version);
    fprintf(outpt , "   Identification    : %d\n",ntohs(iph->id));
    fprintf(outpt , "   Source IP        : %s\n",inet_ntoa(source.sin_addr));
    fprintf(outpt , "   Destination IP   : %s\n",inet_ntoa(dest.sin_addr));
}

void PacketTCPPrint(unsigned char* buff, int Size)
{
    unsigned short ipln;
    struct iphdr *iph = (struct iphdr *)( buff  + sizeof(struct ethhdr) );
    ipln = iph->ihl*4;
    struct tcphdr *tcph=(struct tcphdr*)(buff + ipln + sizeof(struct ethhdr));
    int header_size =  sizeof(struct ethhdr) + ipln + tcph->doff*4;
    fprintf(outpt , "\n\n------------------------------TCP----------------------------\n");

    IPHeaderPrint(buff,Size);

    fprintf(outpt , "\n");
    fprintf(outpt , "TCP Header\n");
    fprintf(outpt , "   Source Port      : %u\n",ntohs(tcph->source));
    fprintf(outpt , "   Destination Port : %u\n",ntohs(tcph->dest));


    fprintf(outpt , "                        DATA                          ");
    fprintf(outpt , "\n");

    fprintf(outpt , "IP Header\n");
    dataPrint(buff,ipln);

    fprintf(outpt , "TCP Header\n");
    dataPrint(buff+ipln,tcph->doff*4);

    fprintf(outpt , "Data Payload\n");
    dataPrint(buff + header_size , Size - header_size );

    fprintf(outpt , "\n****************************************************************");
}

void PacketUDPPrint(unsigned char *buff , int Size)
{

    unsigned short ipln;

    struct iphdr *iph = (struct iphdr *)(buff +  sizeof(struct ethhdr));
    ipln = iph->ihl*4;

    struct udphdr *udph = (struct udphdr*)(buff + ipln  + sizeof(struct ethhdr));

    int header_size =  sizeof(struct ethhdr) + ipln + sizeof udph;

    fprintf(outpt , "\n\n---------------------------UDP Packet-----------------------------\n");

    IPHeaderPrint(buff,Size);

    fprintf(outpt , "\nUDP Header\n");
    fprintf(outpt , "   Source Port      : %d\n" , ntohs(udph->source));
    fprintf(outpt , "   Destination Port : %d\n" , ntohs(udph->dest));


    fprintf(outpt , "\n");
    fprintf(outpt , "IP Header\n");
    dataPrint(buff , ipln);

    fprintf(outpt , "UDP Header\n");
    dataPrint(buff+ipln , sizeof udph);

    fprintf(outpt , "Data Payload\n");


    dataPrint(buff + header_size , Size - header_size);

    fprintf(outpt , "\n******************************************************************");
}



void dataPrint (unsigned char* data , int sz)
{
    int i , j;
    for(i=0 ; i < sz ; i++)
    {
        if( i!=0 && i%16==0)
        {
            fprintf(outpt , "         ");
            for(j=i-16 ; j<i ; j++)
            {
                if(data[j]>=32 && data[j]<=128)
                    fprintf(outpt , "%c",(unsigned char)data[j]);

                else fprintf(outpt , ".");
            }
            fprintf(outpt , "\n");
        }

        if(i%16==0) fprintf(outpt , "   ");
            fprintf(outpt , " %02X",(unsigned int)data[i]);

        if( i==sz-1)
        {
            for(j=0;j<15-i%16;j++)
            {
              fprintf(outpt , "   ");
            }

            fprintf(outpt , "         ");

            for(j=i-i%16 ; j<=i ; j++)
            {
                if(data[j]>=32 && data[j]<=128)
                {
                  fprintf(outpt , "%c",(unsigned char)data[j]);
                }
                else
                {
                  fprintf(outpt , ".");
                }
            }

            fprintf(outpt ,  "\n" );
        }
    }
}
int main()
{
    int sadrs , ds;
    struct sockaddr s_addr;

    unsigned char *buff = (unsigned char *) malloc(65536);

    outpt=fopen("packetLog.txt","w");
    if(outpt==NULL)
    {
        printf("Not able to create txt file.");
    }
    printf("The Analyzer is starting ...\n");

    int s_raw = socket( AF_PACKET , SOCK_RAW , htons(ETH_P_ALL)) ;

    if(s_raw < 0)
    {
        printf("Socket Error");
        return 1;
    }
    int hh=0;
    while(1)
    {   if(hh>1024)break;
        sadrs = sizeof s_addr;
        ds = recvfrom(s_raw , buff , 65536 , 0 , &s_addr , (socklen_t*)&sadrs);
        if(ds <0 )
        {
            printf("Failed to get packets\n");
            return 1;
        }

        hh++;
        PacketProc(buff , ds);
    }
    close(s_raw);
    printf("Done");
    return 0;
}
