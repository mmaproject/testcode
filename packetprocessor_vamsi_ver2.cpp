//we are at ip protocol capinfo

//#define __STDC_FORMAT_MACROS

//#ifdef HAVE_CONFIG_H
//#include "config.h"
//#endif /* HAVE_CONFIG_H */

#include "caputils/caputils.h"
#include "caputils/stream.h"
#include "caputils/filter.h"
#include "caputils/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <netdb.h>
#include <cstdlib>
#include <map>
#include <vector>
#include <string.h>
#include <cstring>
#include <string>
#include <iostream>
#include <qd/qd_real.h>
#include <math.h>
#include <iomanip>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/moment.hpp>
//#include <boost/accumulators/statistics/.hpp>

#include <boost/accumulators/statistics/variance.hpp>
using namespace boost::accumulators;
#define PICODIVIDER (double)1.0e12
#define STPBRIDGES 0x0026
#define CDPVTP 0x016E

using namespace std;

static int keep_running = 1;
static int print_content = 0;
static int print_date = 0;
static int max_packets = 0;
static const char* iface = NULL;
static struct timeval timeout = {1,0};
static const char* program_name = NULL;
static long ipv4 = 0;
//static long tcp = 0;
//static long udp = 0;
static long ipv6 = 0;
static long arp = 0;
static long stp = 0;
static long cdpvtp = 0;
static long other = 0;
static long ipproto[255] = {0,}; 
static long mp;
map <string,long> udp_packet_Count;
map <string,long> tcp_packet_Count;
map <string,vector <int> > udp_packet_Sizes;
map <string,vector <int> > tcp_packet_Sizes;
map <string,vector <qd_real> > udppacket_InterArrivaltime;
map <string,vector <qd_real> > tcppacket_InterArrivaltime;


void handle_sigint(int signum){
	if ( keep_running == 0 ){
		fprintf(stderr, "\rGot SIGINT again, terminating.\n");
		abort();
	}
	fprintf(stderr, "\rAborting capture.\n");
	keep_running = 0;
}

static void print_distribution(){
	printf("PACKETDISTRIBUTION;");

	long ipother = 0;
	if ( ipv4 > 0 ){
		printf("IPv4;");
		for ( int i = 0; i < 255; i++ ){
			if ( ipproto[i] == 0 ){
				continue;
			}

			struct protoent* protoent = getprotobynumber(i);

			if ( !protoent ){
				ipother += ipproto[i];
				continue;
			}

			printf("%s(%ld);", protoent->p_name, ipproto[i]);
		}
		if ( ipother > 0 ){
			printf("other(%ld);", other);
		}
		printf("");
	}
	if ( ipv6 > 0 ){
		printf("IPv6;%ld;", ipv6);
	}
	if ( arp > 0 ){
		printf("ARP;%ld;", arp);
	}
	if ( stp > 0 ){
		printf("STP:%ld:", stp);
	}
	if ( cdpvtp > 0 ){
		printf("cdpvtp:%ld:", cdpvtp);
	}
	if ( other > 0 ){
		printf("Other:%ld:", other);
	}
cout <<"\n";
}

static void print_udpinterarrivaltime ()
{

	//cout << "UDP Inter-arrival STATISTICS ARE: \n";
	//cout <<"the number of UDP socket pairs encountered are: " <<udp_packet_Sizes.size() <<"\n";
	for (map < string, vector<qd_real> > :: const_iterator bt = udppacket_InterArrivaltime.begin(); bt != udppacket_InterArrivaltime.end();++ bt)
	{
		accumulator_set<double, stats<tag::mean, tag::moment<2>, tag :: min, tag :: max, tag :: variance > > acc;
		cout << "INTERARRIVALTIME;UDP;"<<bt->first <<";"<<bt->second.size() <<";";
		if (bt->second.size() > 1) {
			qd_real temp_two = bt->second[0];
			for (int x = 1; x< bt->second.size();x++)
			{
				qd_real diffTime=bt->second[x]-temp_two;
				//cout << setiosflags(ios::fixed) << setprecision(12)<< to_double(bt->second[x])<<"\t"<<to_double(diffTime) << endl;
				if (to_double(diffTime) > 0)
				{
					acc(to_double(diffTime));
					//cout <<"\nAccumulating " << setiosflags(ios::fixed) << setprecision(12)<<to_double(diffTime)<<"\n";
				}
				temp_two = bt->second[x];
				//cout << bt->second[x] <<"-";
			}
			cout <<setiosflags(ios::fixed) <<setprecision(12)<< (mean(acc)) << ";";
			cout << setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::moment<2>(acc)) << ";";
			cout <<setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::min(acc)) << ";";
			cout <<setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::max(acc)) << ";";
			cout <<setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::variance(acc)) << ";";
		}
		else {
			cout <<"0;0;0;0;0;";
		}
		cout <<"\n";
	}
	//cout <<"The numbers of packets per socket pair are: \n";



}

static void print_tcpinterarrivaltime ()
{

//cout << "tcp Inter-arrival STATISTICS ARE: \n";
//qd_real temp_two, diffTime;

//cout <<"the number of tcp socket pairs encountered are: " <<tcp_packet_Sizes.size() <<"\n";
	for (map < string, vector<qd_real> > :: const_iterator bt = tcppacket_InterArrivaltime.begin(); bt != tcppacket_InterArrivaltime.end();++ bt)
	{
		//cout << bt->first  <<"\n";
		accumulator_set<double, stats<tag::mean, tag::moment<2>, tag :: min, tag :: max, tag :: variance > > acc;
		cout << "INTERARRIVALTIME;TCP;"<<bt->first <<";"<<bt->second.size() <<";";
		if (bt->second.size() > 1) {

			qd_real temp_two = bt->second[0];
			for (int x = 1; x< bt->second.size();x++)
			{
				qd_real diffTime=bt->second[x]-temp_two;
				//cout << setiosflags(ios::fixed) << setprecision(12)<< to_double(bt->second[x])<<"\t"<<to_double(diffTime) << endl;
				if (to_double(diffTime) > 0)
				{
					acc(to_double(diffTime));
					//cout <<"\nAccumulating " << setiosflags(ios::fixed) << setprecision(12)<<to_double(diffTime)<<"\n";
				}
				temp_two = bt->second[x];
			//cout << bt->second[x] <<"-";
			}
			//cout <<"\n";
			
			//cout <<"The numbers of packets per socket pair are: \n";
			cout <<setiosflags(ios::fixed) <<setprecision(12)<< (mean(acc)) << ";";
			cout << setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::moment<2>(acc)) << ";";
			cout <<setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::min(acc)) << ";";
			cout <<setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::max(acc)) << ";";
			cout <<setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::variance(acc)) << ";";
		}
		else {
			cout <<"0;0;0;0;0;";
		}
		cout <<"\n";	

	}

}
static void print_tcp(FILE* dst, const struct ip* ip, const struct tcphdr* tcp, qd_real timestamp, int payload_size){
        tcp++;
	
        char tcpsourcebuffer[40];
        char tcpdestinationbuffer[40];
        string tcpbuffer;
	//fprintf(dst, "] %s:%d ",inet_ntoa(ip->ip_src),(u_int16_t)ntohs(tcp->source));
	//fprintf(dst, "--> %s:%d",inet_ntoa(ip->ip_dst),(u_int16_t)ntohs(tcp->dest));
         int temp;
         sprintf(tcpsourcebuffer,"%s;%d",inet_ntoa(ip->ip_src),(u_int16_t)ntohs(tcp->source));
         sprintf (tcpdestinationbuffer, ";%s;%d",inet_ntoa(ip->ip_dst),(u_int16_t)ntohs(tcp->dest));
	 
         tcpbuffer.append(tcpsourcebuffer);
         tcpbuffer.append(tcpdestinationbuffer);
         if ( tcp_packet_Count.find(tcpbuffer) == tcp_packet_Count.end() ){
		tcp_packet_Count[tcpbuffer] = 0;
	}

         tcp_packet_Sizes[tcpbuffer].push_back(payload_size);
	 tcppacket_InterArrivaltime[tcpbuffer].push_back(timestamp);
         tcp_packet_Count [tcpbuffer]++;
       // cout <<" \nhello world "  <<tcpbuffer << "\n"; 
	//fprintf(dst, "\n");
}

static void print_udp(FILE* dst, const struct ip* ip, const struct udphdr* udp, qd_real timestamp, int payload_size){
         udp++;
        char udpsourcebuffer[80];
        char udpdestinationbuffer[80];
        string udpbuffer;
	//fprintf(dst, "UDP(HDR[8]DATA[%d]):\t %s:%d ",(u_int16_t)(ntohs(udp->len)-8),inet_ntoa(ip->ip_src),(u_int16_t)ntohs(udp->source));
	//fprintf(dst, " -> %s:%d",inet_ntoa(ip->ip_dst),(u_int16_t)ntohs(udp->dest));
       // int temp;

         sprintf(udpsourcebuffer,"%s;%d",inet_ntoa(ip->ip_src),(u_int16_t)ntohs(udp->source));
         sprintf(udpdestinationbuffer,";%s;%d",inet_ntoa(ip->ip_dst),(u_int16_t)ntohs(udp->dest));
         udpbuffer.append(udpsourcebuffer);
         udpbuffer.append(udpdestinationbuffer);

	if ( udp_packet_Count.find(udpbuffer) == udp_packet_Count.end() ){
		udp_packet_Count[udpbuffer] = 0;
	}

         udp_packet_Sizes[udpbuffer].push_back(payload_size);
	 udppacket_InterArrivaltime[udpbuffer].push_back(timestamp);
         udp_packet_Count [udpbuffer]++;
	//fprintf(dst, "\n");
}

static void print_icmp(FILE* dst, const struct ip* ip, const struct icmphdr* icmp){
	/*
        fprintf(dst, "ICMP:\t %s ",inet_ntoa(ip->ip_src));
	fprintf(dst, " --> %s ",inet_ntoa(ip->ip_dst));
	fprintf(dst, "Type %d , code %d", icmp->type, icmp->code);
	if( icmp->type==0 && icmp->code==0){
		fprintf(dst, " echo reply: SEQNR = %d ", icmp->un.echo.sequence);
	}
	if( icmp->type==8 && icmp->code==0){
		fprintf(dst, " echo reqest: SEQNR = %d ", icmp->un.echo.sequence);
	}
	fprintf(dst, "\n"); */
}

static void print_ipv4(FILE* dst, const struct ip* ip, qd_real timestamp, int payload_size){
        ipproto[ip->ip_p]++;
	void* payload = ((char*)ip) + 4*ip->ip_hl;
      /*	fprintf(dst, "IPv4(HDR[%d])[", 4*ip->ip_hl);
	fprintf(dst, "Len=%d:",(u_int16_t)ntohs(ip->ip_len));
	fprintf(dst, "ID=%d:",(u_int16_t)ntohs(ip->ip_id));
	fprintf(dst, "TTL=%d:",(u_int8_t)ip->ip_ttl);
	fprintf(dst, "Chk=%d:",(u_int16_t)ntohs(ip->ip_sum));
       
	if(ntohs(ip->ip_off) & IP_DF) {
		fprintf(dst, "DF");
	}
	if(ntohs(ip->ip_off) & IP_MF) {
		fprintf(dst, "MF");
	}

	fprintf(dst, " Tos:%0x]:\t",(u_int8_t)ip->ip_tos);
        */
	switch( ip->ip_p ) {
	case IPPROTO_TCP:
		print_tcp(dst, ip, (const struct tcphdr*)payload , timestamp ,  payload_size);
		break;

	case IPPROTO_UDP:
		print_udp(dst, ip, (const struct udphdr*)payload,  timestamp ,  payload_size);
		break;

	case IPPROTO_ICMP:
		print_icmp(dst, ip, (const struct icmphdr*)payload);
		break;

	default:
		fprintf(dst, "Unknown transport protocol: %d \n", ip->ip_p);
		break;
	}
}

static void print_ieee8023(FILE* dst, const struct llc_pdu_sn* llc){
	fprintf(dst,"dsap=%02x ssap=%02x ctrl1 = %02x ctrl2 = %02x\n", llc->dsap, llc->ssap, llc->ctrl_1, llc->ctrl_2);
}

static void print_eth(FILE* dst, const struct ethhdr* eth, qd_real timestamp, int payload_size){
	void* payload = ((char*)eth) + sizeof(struct ethhdr);
	uint16_t h_proto = ntohs(eth->h_proto);
	uint16_t vlan_tci;


 begin:

	if(h_proto<0x05DC){
	/*	fprintf(dst, "IEEE802.3 ");
		fprintf(dst, "  %02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x ",
		        eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5],
		        eth->h_dest[0],  eth->h_dest[1],  eth->h_dest[2],  eth->h_dest[3],  eth->h_dest[4],  eth->h_dest[5]);
		print_ieee8023(dst,(struct llc_pdu_sn*)payload);
      */
	} else {
		switch ( h_proto ){
		case ETHERTYPE_VLAN:
			vlan_tci = ((uint16_t*)payload)[0];
			h_proto = ntohs(((uint16_t*)payload)[0]);
			payload = ((char*)eth) + sizeof(struct ethhdr);
			fprintf(dst, "802.1Q vlan# %d: ", 0x0FFF&ntohs(vlan_tci));
			goto begin;

		case ETHERTYPE_IP:
			print_ipv4(dst, (struct ip*)payload ,timestamp, payload_size);
                        
                        ipv4++;
			break;

		case ETHERTYPE_IPV6:
			//printf("ipv6\n");
                        ipv6++;
			break;

		case ETHERTYPE_ARP:
			//printf("arp\n");
                        arp++;
			break;

		case 0x0810:
			//fprintf(dst, "MP packet\n");
                        mp++;
			break;

		case STPBRIDGES:
			//fprintf(dst, "STP(0x%04x): (spanning-tree for bridges)\n", h_proto);
                        stp++;
			break;

		case CDPVTP:
			//fprintf(dst, "CDP(0x%04x): (CISCO Discovery Protocol)\n", h_proto);
                        cdpvtp++;
			break;

		default:
                        other++;
			//fprintf(dst, "Unknown ethernet protocol (0x%04x),  ", h_proto);
			//fprintf(dst, " %02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x \n",
			   //     eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5],
			     //   eth->h_dest[0],  eth->h_dest[1],  eth->h_dest[2],  eth->h_dest[3],  eth->h_dest[4],  eth->h_dest[5]);
			break;
		}
	}
}

static struct option long_options[]= {
	{"content",  no_argument,       0, 'c'},
	{"packets",  required_argument, 0, 'p'},
	{"iface",    required_argument, 0, 'i'},
	{"timeout",  required_argument, 0, 't'},
	{"calender", no_argument,       0, 'd'},
	{"help",     no_argument,       0, 'h'},
	{0, 0, 0, 0} /* sentinel */
};

static void show_usage(void){
//	printf("%s-" VERSION "\n", program_name);
	printf("(C) 2004 Patrik Arlos <patrik.arlos@bth.se>\n");
	printf("(C) 2012 David Sveningsson <david.sveningsson@bth.se>\n");
	printf("Usage: %s [OPTIONS] STREAM\n", program_name);
	printf("  -c, --content        Write full package content as hexdump. [default=no]\n"
	       "  -i, --iface          For ethernet-based streams, this is the interface to listen\n"
	       "                       on. For other streams it is ignored.\n"
	       "  -p, --packets=N      Stop after N packets.\n"
	       "  -t, --timeout=N      Wait for N ms while buffer fills [default: 1000ms].\n"
	       "  -d, --calender       Show timestamps in human-readable format.\n"
		" This tool prints PACKETSIZES;PROTOCOL;IP_ADDRESS( Source); SOURCE PORT; IP_ADDRESSES( Destination) ; DESTINATION PORT ; Number of packets in Stream; MEAN;MOMENT (2);MIN;MAX;VARIANCE \n"
		"FUNCTIONS SUPPORTED ARE PACKETSIZES ; INTERARRIVALTIME;PACKETDISTRIBUTION;\n"
	       "  -h, --help           This text.\n\n");
	filter_from_argv_usage();
	cout <<" This tool prints FUNCTION;PROTOCOL;IP_ADDRESS( Source); SOURCE PORT; IP_ADDRESSES( Destination) ; DESTINATION PORT ; Number of packets in Stream; MEAN;MOMENT (2);MIN;MAX;VARIANCE \n FUNCTIONS SUPPORTED ARE:\n a.PACKETSIZES\n b.INTERARRIVALTIME\n c.PACKETDISTRIBUTION\n";
}

static void show_statistics () {

// PACKETSIZE[FUNCTION];PROTOCOL;STREAM_ID;COUNT;MEAN;MOMENT;MIN;MAX;VARIANCE...
// STREAM_ID;PROTOCOL;
//UDP,PACKETSIZES:SO
//cout << "UDP STATISTICS ARE: \n";
//cout <<"the number of UDP socket pairs encountered are: " <<udp_packet_Sizes.size() <<"\n";
for (map < string, vector<int> > :: const_iterator bt = udp_packet_Sizes.begin(); bt != udp_packet_Sizes.end();++ bt)
{
//cout <<"" <<udp_packet_Sizes.size() <<"\n";
cout <<"PACKETSIZES;UDP;"<< bt->first  <<";"<<bt->second.size()<<";";
//initialise
 accumulator_set<double, stats<tag::mean, tag::moment<2>, tag :: min, tag :: max, tag :: variance > > acc;
for (int x = 0; x< bt->second.size();x++)
{
acc(bt->second[x]);
}
cout <<setiosflags(ios::fixed) <<setprecision(12)<< (mean(acc)) << ";";
cout << setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::moment<2>(acc)) << ";";
cout <<setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::min(acc)) << ";";
cout <<setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::max(acc)) << ";";
cout <<setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::variance(acc)) << ";";
cout <<"\n";
}
//cout <<"The numbers of packets per socket pair are: \n";
/*for (map < string, long> :: const_iterator it = udp_packet_Count.begin(); it != udp_packet_Count.end();++ it)
{
cout << it->first << " -->" <<it->second <<"\n";

}
*/
//cout << "TCP STATISTICS ARE: \n";
//cout <<"the number of TCP socket pairs encountered are:\n";
for (map < string, vector<int> > :: const_iterator tbt = tcp_packet_Sizes.begin(); tbt != tcp_packet_Sizes.end();++ tbt)
{
cout << "PACKETSIZES;TCP;" << tbt->first  << ";"<<tbt->second.size()<<";";
 accumulator_set<double, stats<tag::mean, tag::moment<2>, tag :: min, tag :: max, tag :: variance  > > acc;
for (int x = 0; x< tbt->second.size();x++)
{
acc( tbt->second[x]);
}
cout <<setiosflags(ios::fixed) <<setprecision(12)<< (mean(acc)) << ";";
cout << setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::moment<2>(acc)) << ";";
cout <<setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::min(acc)) << ";";
cout <<setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::max(acc)) << ";";
cout <<setiosflags(ios::fixed) <<setprecision(12)<< (boost::accumulators::variance(acc)) << ";";
//cout << "Min: " << min(acc) << endl;
//cout << "Max: " << max(acc) << endl;
cout <<"\n";
}
//cout <<"The numbers of packets per socket pair are: \n";
/*for (map < string, long> :: const_iterator tit = tcp_packet_Count.begin(); tit != tcp_packet_Count.end();++ tit)
{
cout << tit->first << " -->" <<tit->second <<"\n";

}
*/
}
int main(int argc, char **argv){
  /* extract program name from path. e.g. /path/to/MArCd -> MArCd */
  const char* separator = strrchr(argv[0], '/');
  if ( separator ){
    program_name = separator + 1;
  } else {
    program_name = argv[0];
  }

	struct filter filter;
	if ( filter_from_argv(&argc, argv, &filter) != 0 ){
		return 0; /* error already shown */
	}

	filter_print(&filter, stderr, 0);

	int op, option_index = -1;
	while ( (op = getopt_long(argc, argv, "hcdi:p:t:", long_options, &option_index)) != -1 ){
		switch (op){
		case 0:   /* long opt */
		case '?': /* unknown opt */
			break;

		case 'd':
			print_date = 1;
			break;

		case 'p':
			max_packets = atoi(optarg);
			break;

		case 't':
			{
				int tmp = atoi(optarg);
				timeout.tv_sec  = tmp / 1000;
				timeout.tv_usec = tmp % 1000 * 1000;
			}
			break;

		case 'c':
			print_content = 1;
			break;

		case 'i':
			iface = optarg;
			break;

		case 'h':
			show_usage();
			return 0;

		default:
			printf ("?? getopt returned character code 0%o ??\n", op);
		}
	}

	int ret;

	/* Open stream(s) */
	struct stream* stream;
	if ( (ret=stream_from_getopt(&stream, argv, optind, argc, iface, "-", program_name, 0)) != 0 ) {
		return ret; /* Error already shown */
	}
	const stream_stat_t* stat = stream_get_stat(stream);
	stream_print_info(stream, stderr);

	/* handle C-c */
	signal(SIGINT, handle_sigint);

	while ( keep_running ) {
		/* A short timeout is used to allow the application to "breathe", i.e
		 * terminate if SIGINT was received. */
		struct timeval tv = timeout;

		/* Read the next packet */
		cap_head* cp;
		ret = stream_read(stream, &cp, &filter, &tv);
		if ( ret == EAGAIN ){
			continue; /* timeout */
		} else if ( ret != 0 ){
			break; /* shutdown or error */
		} 
                qd_real pkt1;
                pkt1=(qd_real)(double)cp->ts.tv_sec+(qd_real)(double)(cp->ts.tv_psec/PICODIVIDER);
		time_t time = (time_t)cp->ts.tv_sec;
		//fprintf(stdout, "[%4"PRIu64"]:%.4s:%.8s:", stat->matched, cp->nic, cp->mampid);
		if( print_date == 0 ) {
			//fprintf(stdout, "%u.", cp->ts.tv_sec);
		} else {
			static char timeStr[25];
			struct tm tm = *gmtime(&time);
			strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tm);
		//	fprintf(stdout, "%s.", timeStr);
		}

		//fprintf(stdout, "%012"PRId64":LINK(%4d):CAPLEN(%4d):", cp->ts.tv_psec, cp->len, cp->caplen);
                int packetlength = cp->len;
		print_eth(stdout, (struct ethhdr*)cp->payload , pkt1, packetlength);

		if ( max_packets > 0 && stat->matched >= max_packets) {
			/* Read enough pkts lets break. */
			printf("read enought packages\n");
			break;
		}
	}

	/* if ret == -1 the stream was closed properly (e.g EOF or TCP shutdown)
	 * In addition EINTR should not give any errors because it is implied when the
	 * user presses C-c */
	if ( ret > 0 && ret != EINTR ){
		fprintf(stderr, "stream_read() returned 0x%08X: %s\n", ret, caputils_error_string(ret));
	}

	/* Write stats */
	//fprintf(stderr, "%"PRIu64" packets received.\n", stat->read);
	//fprintf(stderr, "%"PRIu64" packets matched filter.\n", stat->matched);

	/* Release resources */
	stream_close(stream);
	filter_close(&filter);
       show_statistics();
       print_tcpinterarrivaltime ();
	 print_udpinterarrivaltime ();	
         print_distribution();

	return 0;
}




