// Functions to remember in the new libcap utilities

// To Open a stream:
// int stream_open (struct stream ** stptr, const stream_addr_t* addr, const char * nic, const int port )

// general usage : first obtain a stream address stream_addr_t input;

//comments
#define pdebug() printf("########## %s %d \n",_FILE_,_LINE_);
#define __STDC_FORMAT_MACROS
//#define debug_sample
//#define debug

#include <caputils/caputils.h>
#include <caputils/stream.h>
#include <caputils/filter.h>
#include <conserver/server.h>

#include <stdio.h>
#include <inttypes.h>
#include <net/if_arp.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <math.h>
#include <qd/qd_real.h>
#include <signal.h>
#include <string>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <cerrno>
using namespace std;

#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 2
#define PICODIVIDER (double)1.0e12
#define STPBRIDGES 0x0026
#define CDPVTP 0x016E

#ifndef ETHERTYPE_IPV6 /* libc might not provide this if it is missing ipv6 support */
#define ETHERTYPE_IPV6 0x86dd
#endif /* ETHERTYPE_IPV6 */

static void pktArrival (qd_real tArr, int pktSize, double linkCapacity,qd_real *BITS, qd_real *ST, int bins, double tSample);
static double sampleBINS (qd_real *BITS, qd_real * ST, int bins, double tSample, double linkCapacity);
static int payLoadExtraction (int level, const cap_head* caphead);

static int fractionalPDU = 1;
static const char* program_name;
static int keep_running = 1;

static struct option long_options [] = {
	{"pkts",            required_argument, 0, 's'},
	{"samplefrequency", required_argument, 0 ,'m'},
	{"triggerpoint",    required_argument, 0, 'n'},
	{"no-fraction",     no_argument,       0, 'z'},
	{"level",           required_argument, 0 ,'q'},
	{"link",	          required_argument, 0, 'l'},
	{"help",            required_argument, 0, 'h'},
	{"iface",           required_argument, 0, 'i'},
	{"verbose",         required_argument, 0, 'v'},
	{"listen",          required_argument, 0, 'g'},
	{"port",            required_argument, 0, 'p'},
	{0, 0, 0, 0}
};

static void show_usage(const char* program_name){
	printf("(C) 2004 Patrik Arlos <patrik.arlos@bth.se>\n");
	printf("(C) 2012 Vamsi Krishna Konakalla\n");
	printf("usage: %s [OPTION]... INPUT\n", program_name);
	printf ("  -m, --samplefrequency  Sample frequency in Hz default [1 Hz]\n"
	        "  -n, --triggerpoint	    If enabled Sampling will start 1/(2*fs) s prior to the first packet.\n"
	        "   	  		              otherwise it shall start floor( 1/(2*fs)) s  prior to the first packet.\n"
	        "  -q, --level 		        Level to calculate bitrate {physical (default), link, network, transport and application}\n"
	        "                         At level N , payload of particular layer is only considered, use filters to select particular streams.\n"
	        "                         To calculate the bitrate at physical , use physical layer, Consider for Network layer use [-q network]\n"
	        "                         It shall contain transport protocol header + payload\n"
	        "                           - link: all bits captured at physical level, i.e link + network + transport + application\n"
	        "                           - network: payload field at link layer , network + transport + application\n"
	        "                           - transport: payload at network  layer, transport + application\n"
	        "                           - application: The payload field at transport leve , ie.application\n"
	        "                         Default is link\n"
	        "      --no-fraction      No fractional PDU\n"
	        "  -s, --pkts=N           Stop after N packets\n"
	        "  -v, --verbose 	        Enable verbose output\n"
	        "  -l, --link             link capacity in Mbps [Default 100 Mbps]"
	        "  -i, --iface=IFACE      MA interface\n"
	        "  -g, --listen=IP        Listen IP [Default: 0.0.0.0]\n"
	        "  -p, --port=PORT        Listen port [Default: 8073]\n"
	        "  -h, --help 		        This help text\n\n");
	filter_from_argv_usage();
}

int main (int argc , char **argv) {
	/* extract program name from path. e.g. /path/to/MArCd -> MArCd */
	const char* separator = strrchr(argv[0], '/');
	if ( separator ){
		program_name = separator + 1;
	} else {
		program_name = argv[0];
	}

	int level = 0; // Bits Per Second calculation variables [BPScv]
	double linkCapacity = 100e6; // initializing as 10 Mbps
	double sampleFrequency = 1.0;
	int triggerPoint = 0; //When do we do the sampling
	FILE* verbose = NULL;
	uint64_t sampleCounter = 0;
	qd_real tSample = 1.0/sampleFrequency;
	qd_real nextSample,lastEvent, pktArrivalTime; // when does the next sample occur, sample interval time
	uint64_t dropCount = 0; // number of packets that have been dropped
	qd_real timeOffset;
	const char* listen_ip = "0.0.0.0";
	int listen_port = 8073;
	double linkC = 100.0;
	int ret = 0;
	uint64_t max_packets = 0;
	struct filter myFilter; // filter to filter arguments
	const char* iface = NULL;

	if ((filter_from_argv(&argc,argv, &myFilter)) != 0) {
		fprintf (stderr, "could not create filter ");
		exit(1);
	}

	if ( argc < 2 ) {
		printf ("use %s -h or --help for help \n" , argv[0]);
		exit (1);
	}

	int op, option_index = -1;
	while ( (op=getopt_long(argc, argv, "hvl:i:m:n:q:p:g:p:", long_options, &option_index)) != -1 ){
		switch (op) {
		case '?': /* error */
			return 1;

		case 0: /* longopt with flag set */
			break;

		case 'm' : /* --samplefrequency */
			sampleFrequency = atof (optarg);
			tSample = 1/(double) sampleFrequency;
			break;

		case 'n': /* --triggerpoint */
			triggerPoint = 1;
			break;

		case 'v': /* --verbose */
			verbose = stderr;
			break ;

		case 'q': /* --level */
			if (strcmp (optarg, "link") == 0)
				level = 0;
			else if ( strcmp (optarg, "network" ) == 0)
				level = 1;
			else if (strcmp (optarg ,"transport") == 0)
				level = 2;
			else if (strcmp (optarg , "application") == 0)
				level = 3;
			else {
				fprintf(stderr, "unrecognised level arg %s \n", optarg);
				exit(1);
			}
			break;

		case 'l': /* --link */
			linkC = atof (optarg);
			cout << " Link Capacity input = " << linkC << " bps\n";
			break;

		case 'z': /* --no-fraction */
			fractionalPDU = 0;
			break;

		case 's': /* --pkts */
			max_packets = atoi (optarg);
			break;

		case 'i' : /* --iface */
			iface = optarg;
			break;

		case 'g': /* --listen */
			listen_ip = optarg;
			break;

		case 'p': /* --port */
			listen_port = atoi(optarg);
			break;

		case 'h': /* --help */
			show_usage(argv[0]);
			exit (0);
			break;

		default:
			if ( option_index >= 0 ){
				fprintf(stderr, "flag --%s declared but not handled\n", long_options[option_index].name);
			} else {
				fprintf(stderr, "flag -%c declared but not handled\n", op);
			}
			abort();
		}
	}

	/* Verbose output is discarded by default */
	if ( !verbose ){
		verbose = fopen("/dev/null", "r");
	}

	/* Initialize sample bins */
	const size_t noBins = (ceil ((double)(1514*8)/(double) linkCapacity/to_double(tSample))) + 2 +sampleFrequency ;  // +1 to account for edge values, second +1 to account for n+1 samples.
	qd_real* BINS = new qd_real [noBins];
	qd_real* ST =  new qd_real [noBins];

	/* Show settings */
	fprintf(verbose, "Longest transfer time = %f\n", (double)1514*8 /(double) linkCapacity);
	fprintf(verbose, "tT/tStamp = %f\n", to_double(1514*8 /(double) linkCapacity/tSample));
	fprintf(verbose, "we need %zd bins\n", noBins);
	fprintf(verbose, "Allocating memory buffer\n");
	fprintf(verbose, "sampleFrequency = %fHz %f\n", sampleFrequency, to_double(tSample));
	fprintf(verbose, "LinkCapacity = %fMbps\n", linkCapacity/1e6);

	/* Open source stream */
	stream_t inStream; // stream to read from
	if ( stream_from_getopt(&inStream, argv, optind, argc, iface, NULL, program_name,0) != 0 ){
		return 1; /* error already shown */
	}

	/* Get stats handle */
	const stream_stat_t* stats = stream_get_stat(inStream);

	/* Display stream information (version, mampid, etc) */
	stream_print_info(inStream, verbose);

	/* Initialize conserver */
	conserver_t srv;
	dataset_t ds;
	if ( (ret=conserver_init(&srv, listen_ip, listen_port)) != 0 ){
		fprintf(stderr, "Failed to initialize conserver (%d): %s\n", ret, conserver_get_error());
		return 1;
	}
	if ( (ret=conserver_add(srv, &ds, "bitrate")) != 0 ){
		fprintf(stderr, "Failed to create dataset (%d): %s\n", ret, conserver_get_error());
		return 1;
	}

	// begin packet processing

	///////////////////// HERE ONWARS LOOK INTOOOOO
	//Begin Packet processing
	fprintf(verbose, "Going to read first\n");

	struct cap_header *caphead;
	if ( (ret=stream_read (inStream, &caphead, &myFilter, NULL)) != 0 ){
		fprintf(stderr, "stream_read() returned %d: %s\n", ret, caputils_error_string(ret));
		return 1;
	}

	fprintf(verbose, "Read initial packet\n");

	pktArrivalTime=(qd_real)(double)caphead->ts.tv_sec+(qd_real)(double)caphead->ts.tv_psec/PICODIVIDER;
	timeOffset=floor(pktArrivalTime);
        cout<< "Packet offset is  "<< setiosflags(ios::fixed) << setprecision(12) << to_double(timeOffset) << "\n"; //vkk
	pktArrivalTime-=timeOffset;
        cout << "Packet arrival time is " << to_double(pktArrivalTime)<< "\n"; //vkk

	if(triggerPoint==1) {
		nextSample=pktArrivalTime+tSample/2.0; // Set the first sample to occur 0.5Ts from the arrival of this packet.
	} else {
		nextSample=floor(pktArrivalTime+tSample/2.0);
	}

	fprintf(verbose,"First packet arrives at %f.\n",to_double(pktArrivalTime+timeOffset));
	fprintf(verbose,"                        %lu.%06llu \n",caphead->ts.tv_sec,caphead->ts.tv_psec);
	

	for( unsigned int i = 0; i < noBins; i++ ){
		ST[i]=nextSample;
		BINS[i]=0.0;
		nextSample-=tSample; // Since these will have taken place.
	}

	if ( (ret=stream_read (inStream, &caphead, &myFilter, NULL)) != 0 ){
		fprintf(stderr, "stream_read() returned %d: %s\n", ret, caputils_error_string(ret));
		return 1;
	}

	fprintf(verbose, "Read secondary packet\n");
	static int readPktCounter=1;
	while ( keep_running ){
		int payLoadSize = 0;
		if(pktArrivalTime<ST[0]) { /* Packet arrived first */
			//  cout << "pkt["<< pktCount << "]";
			if(pktArrivalTime<ST[1]) { /* Packet should have arrived earlier, dropping (pktArrival CAN handle this partially */
				//	  cout << caphead->nic << ":" << caphead->ts.tv_sec << "." << caphead->ts.tv_psec << "\t";
				//	  cout << "Dropping packet!!!" << endl;
				dropCount++;
				printf("\n[%d] dropping pkt:%f st[0]: %f st[1]: %f, timeOffSet = %f\n", readPktCounter,to_double(pktArrivalTime), to_double(ST[0]), to_double(ST[1]),to_double(timeOffset));
				printf("(dropping)Ppkt arrived:%lu.%09llu \n", caphead->ts.tv_sec,caphead->ts.tv_psec);
			} else {                   /* Normal packet treatment */
				/* Begin by extracting the interesting information. */
				//    printf("%s:%f.%f:LINK(%4d): \t",caphead->nic,caphead->ts.tv_sec,caphead->ts.tv_psec, caphead->len);
				//     cout << caphead->nic << ":" << caphead->ts.tv_sec << "." << caphead->ts.tv_psec << ":LINK(" << caphead->len << ") \t";
				payLoadSize = payLoadExtraction(level, caphead);
				pktArrival(pktArrivalTime,payLoadSize,linkCapacity, BINS,ST, noBins,to_double(tSample));
				lastEvent=pktArrivalTime;
			}  // End Normal packet treatment.

			if ( max_packets > 0 && ( stats->matched + 1 ) > max_packets) {
				/* Read enough pkts lets break. */
				break;
			}

			/* MA packets arrive at least once per second if there is data (and no packets if there wasn't any) */
			struct timeval tv = {1,0};
			ret = stream_read (inStream, &caphead, &myFilter, NULL);
			if ( ret == -1 ){
				keep_running = 0; /* finished */
				continue;
			} else if ( ret == EAGAIN ){ /* timeout */
			  printf("WTF ! ! ? ? ! ? ! ? ! ? ! ? ! ? ! ? ! ? ! ? !\n");
				goto do_sample; /* f#ing spagetti, sorry */
			} else if ( ret != 0 ){ /* error */
				fprintf(stderr, "stream_read() returned %d: %s\n", ret, caputils_error_string(ret));
				return 1;
			}
			readPktCounter++;
			//usleep(300);

			pktArrivalTime=(qd_real)(double)caphead->ts.tv_sec+(qd_real)(double)caphead->ts.tv_psec/PICODIVIDER;
			pktArrivalTime-=timeOffset;
#ifdef debug			
			printf("\n%d pktArrival:%f st[0]: %f st[1]: %f (nic = %s)\n",readPktCounter, to_double(pktArrivalTime), to_double(ST[0]), to_double(ST[1]), caphead->nic);
#endif 
		} else { /* Sample should occur */
#ifdef debug
		  printf("SAMPLE ME? %f < %f \n", to_double(pktArrivalTime),to_double(ST[0]));
#endif
		  do_sample:
			lastEvent=ST[noBins-1];
			const double sampleValue=sampleBINS(BINS,ST,noBins,to_double(tSample),linkCapacity);
			//cout << "[" << sampleCounter <<"]  " << setiosflags(ios::fixed) << setprecision(12) << to_double(lastEvent+timeOffset)<< ": " << sampleValue << " bps " << endl;
			//cout << setiosflags(ios::fixed) << setprecision(6) << to_double(lastEvent+timeOffset)<< ":" << sampleValue << endl;

			char msg[64];
			const int bytes = snprintf(msg, 64, "%.6f:%.6f", to_double(lastEvent+timeOffset), sampleValue);

			if(sampleValue>0.0){
			  fprintf(stdout, "%s\n", msg);
			}
			if(sampleValue>linkCapacity){
			  abort();
			}
			fflush(stdout);
			conserver_push(&ds, msg, bytes);
			sampleCounter++;
		}
	}//End Packet processing
#ifdef debug
	cout << "Terminated loop. Cleaning up." << endl;
#endif
	for( unsigned int i = 0; i < noBins + 2; i++ ){
		lastEvent=ST[noBins-1];
		const double sampleValue = sampleBINS(BINS,ST,noBins,to_double(tSample),linkCapacity);
		//      cout << "[" << sampleCounter <<"]" << setiosflags(ios::fixed) << setprecision(12) << (double)(lastEvent+timeOffset) << ": " << sampleValue << endl;
		if(sampleValue>0.0){
		  cout << setiosflags(ios::fixed) << setprecision(6) << to_double(lastEvent+timeOffset) << ":" << sampleValue << endl;
		}
		if(sampleValue>linkCapacity){
			  abort();
		}
		sampleCounter++;
	}

	delete(ST);
	delete(BINS);



	fprintf(verbose, "There was a total of %"PRIu64" pkts that matched the filter.\n", stats->matched);

	stream_close(inStream);
	filter_close(&myFilter);

	return 0;
}


/* ------------------------SUB-routines-------------------------------*/
/* ****************************************************************** */
/*
  Created:      2002-12-16 ??:??, Patrik.Carlsson@bth.se
  Latest edit:  2003-02-20 10:30, Patrik.Carlsson@bth.se

  Function:
  void pktArrival(*)
  Return value:
  None

  Arguments (In Order):
  Arrival time of Packet.
  Size of Packet/Payload, this is used to calculate when the packet/payload started.
  Currently this only works when the payload is at the end!
  I.e If p indicated payload, H packet headers/or other uninteresting info.
  HHHHpppppp OK
  Hppppppppp OK
  HHHpppHHHH NOT OK
  ppppppHHHH NOT OK
  Link Capacity in bps.
  Array that contains the number of bits that arrived in a sample interval, defined in ST.
  Array that contains the sample times, ST[0] is the NEXT sample, ST[1] the most recent.
  ST[N] is the sample that is to be "releases"
  Number of samples/bins, i.e. N in the definition above.
  Inter sample time, or 1/fS.

  Description:
  In order to correctly estimate where a packets bits arrive, this to sample correctly, we calculate when the first packet/payload
  bit arrived. Once we have the packets 'real' arrival time(tStart), and termination time (tArr). We can overlay this time span
  on the sample time line, the bits of the packet are then 'dropped' down into the corresponding bin. Since we allow arbitrary
  sample frequency (double sets the limit), we can zoom down to sample intervals smaller than the transfer time of a bit. However
  at this level of detail, the time stamp of the packet is _crucial_. Depending on its inaccuracy there might be more than one bit
  in an interval. This routine DOES NOT account for this, it MAY report if there is more bits than possible in a interval.




*/
static void pktArrival(qd_real tArr, int pktSize, double linkCapacity,qd_real *BITS,qd_real *STy,int bins, double tSample){
	qd_real tTransfer, tStart;              // Transfer time of packet, start time of packet
	int j;                                  // Yee, can it be a index variable..
	qd_real bits;                           // Temporary variable, holds number of bits in a, or parts of, sample interval

#ifdef debug
	printf("pktArrival() %d bytes ",pktSize);
#endif
	tTransfer=(pktSize*8.0)/linkCapacity;   // Estimate the transfer wire transfer time
	tStart=tArr-tTransfer;                  // Estimate when the packet started arriving //vkk edit tStart = tArr;
	//	tArr=tArr+tTransfer;

#ifdef debug
	cout << "  [tStart " << setiosflags(ios::fixed) << setprecision(12) << to_double(tStart) << " -- tTrfs " << to_double(tTransfer) << "  ---- " << (tArr) << " tT/tS " << to_double(tTransfer)/(tSample) << endl;
	cout << " STy[0] = " << setiosflags(ios::fixed) << setprecision(12) << to_double(STy[0]) << endl;
	cout << " STy[1] = " << setiosflags(ios::fixed) << setprecision(12) << to_double(STy[1]) << endl;
 
#endif
	if(fractionalPDU==0){        // Do not consider fractional pdus, just wack them in the array.
		BITS[0]+=pktSize*8;
	} else {       // Normal treatemant
	  

		if(tArr<STy[0] && tArr>STy[1]) {        // Pkt arrives in the "next" sample.. NORMAL behaviour.
#ifdef debug
			printf("<<PKT arrived Correctly>>");
#endif
			if(tArr>STy[1]) {                  // Packet arrives completely next sample interval, completely since we also know tArr<STy[0]
#ifdef debug
				printf("bin 0 %d bits (ONE).\n",pktSize*8);
#endif
				BITS[0]+=pktSize*8;
			} else {
				if(tStart>STy[2] ){               // Packet started arriving in previous sample interval, so its split inbetween the 'previous' and 'next' intervals.
#ifdef debug
					cout << "bin 0 +" << (tArr-STy[1])*linkCapacity << " bin 1 += " << (STy[1]-tStart)*linkCapacity << " (TWO)" << endl;
#endif
					BITS[0]+=(tArr-STy[1])*linkCapacity;    // Calculate how many bits that will arrive in the next interval.
					BITS[1]+=(STy[1]-tStart)*linkCapacity;  // and how many bits that arrived in the previous.
				} else {                                  // Packet spans more than 3 intervals
					if(ceil(to_double(tTransfer/tSample))>=bins) {  // Calculate how many bins that the packet needs to be completely measured.
						cout << "  [tStart " << tStart << " -- tTrfs " << tTransfer << "  ---- " << tArr << " tT/tS " << tTransfer/tSample << endl;
						printf("Error: pkt to large...\n");      // It needs more than we got, ERROR!! Increase the noBins in the init..
						return;                                  // Each extra bin, allows us to handle packets that are
					}                                            // offset by tSample seconds. Thus 10 extra bins, allows pkts to be offset 10tSample

					bits=pktSize*8.0-(tArr-STy[1])*linkCapacity; // How many bits will remain of the packet after the once that go to
					// next interval has been removed?
					BITS[0]+=(tArr-STy[1])*linkCapacity;         // How many bits will end up in the next interval ?
					j=1;
#ifdef debug
					cout << "bin 0 +" << (tArr-STy[1])*linkCapacity << "( "<< tArr << " - " << STy[1]<< " = " << tArr-STy[1] << ") BIN[" << BITS[0]<< "]"<<endl;
#endif
					while(bits>tSample*linkCapacity){            // While bits>tSample*linkCapacity=MaxIntervalBits of the packet keep filling the intervals.
#ifdef debug                                           // We use MIB to not endup with negative bits in the last interval.
						cout << "bin " << j << "  BIN["<< BITS[j] << "] " << endl;
#endif
						BITS[j]+=tSample*linkCapacity;            // add B bits in interval j
						bits-=tSample*linkCapacity;               // calculate how many bits that remain
#ifdef debug
						cout << " += " << tSample*linkCapacity << " (" << STy[j+1] << " -- "<< STy[j] << " ) BIN[" << BITS[j] << "]" << endl;
#endif
						j++;
					}
#ifdef debug
					cout << "Lbin " << j << " BIN[ " << BITS[j] << "] "<< endl;//The remaining bits goes here. The range of bits should be 0<= bits <MIB
#endif
					if(bits>0.0) {
						BITS[j]+=bits;
					}
					if(bits<0.0) {                                // Just a precausion, and probably an ERROR!
						BITS[j-1]-=(-1.0)*bits;
						cout << "  [tStart " << tStart << " -- tTrfs " << tTransfer << "  ---- " << tArr << " tT/tS " << tTransfer/tSample << endl;
						printf("ERROR: negative bits in the last bin. Multiple bins coverd!!");
						return;
					}
#ifdef debug
					cout << " += " << bits << " ("<< STy[j+1] << " -- "<< STy[j] << " ) BIN[ " << BITS[j] << "]" << endl;

#endif
				}
			}
		} else if(STy[2]<tArr && tArr<STy[1]) {                    // Packet should have arrived in the previous sample..
			printf("**PKT MISSED IT sample**\n");            // This is the almost the same routines as above, only adjusted to account for the
			// shifted arrival time of the packet.
			if(ceil(to_double(tTransfer/tSample))>=bins-1) {
				printf("Error: Pkt missed by one interval and pkt to large...\n");
				return;
			}

			if(tStart>STy[2]) {                             // Packet arived completely in the previous interval
#ifdef debug
				printf("Full pkt of %d bits in bin 1.\n",pktSize*8);
#endif
				BITS[1]+=pktSize*8;
			} else {
				if(tStart>STy[3] ){                               // Pkt split from previous sample and this
					BITS[1]+=(tArr-STy[2])*linkCapacity;
					BITS[2]+=(STy[2]-tStart)*linkCapacity;
				} else {                                          // Pkt spans more than 3 intervals
					BITS[1]+=(tArr-STy[2])*linkCapacity;
					j=2;
					bits=pktSize*8.0-(tArr-STy[2])*linkCapacity;    // How many bits will remain of the packet after the once that go to
					while(bits>tSample*linkCapacity && j<bins-1){
						BITS[j]+=tSample*linkCapacity;
						bits-=tSample*linkCapacity;                   // calculate how many bits that remain
						j++;
					}
					if(bits>0.0) {
						BITS[j]+=bits;
					}
					if(bits<0.0) {                                  // Just a precausion, and probably an ERROR!
						BITS[j-1]-=(-1.0)*bits;
						cout << "  [tStart " << tStart << " -- tTrfs " << tTransfer << "  ---- " << tArr << " tT/tS " << tTransfer/tSample << endl;
						printf("ERROR: negative bits in the last bin. Multiple bins coverd(Shifted pkt)!!");
						return;
					}
				}
			}
		} else if(tArr<STy[2]) {                               // Packet is off by two samples.. BIG ERRROR!!
#ifdef debug
			cout << "ERROR: packet (" << tArr <<") arrived " << (STy[1]-tArr)/tSample << " samples to late.\n" << endl;
#endif
		} else {
		  cout << "WTF?" << endl;
		}
	}
#ifdef debug
	printf("<-pktArrival()  ");
#endif
  return;
}

/*
  Created:      2002-12-16 ??:??, Patrik.Carlsson@bth.se
  Latest edit:  2003-02-20 10:30, Patrik.Carlsson@bth.se

  Function:
  void sample(*)
  Return value:
  bitrate estimate at sample ST[N]

  Arguments (In Order):
  Array that contains the number of bits that arrived in a sample interval, defined in ST.
  Array that contains the sample times, ST[0] is the NEXT sample, ST[1] the most recent.
  ST[N] is the sample that is to be "releases"
  Number of samples/bins, i.e. N in the definition above.
  Inter sample time, or 1/fS.
  Link Capacity in bps.

  Description:
  Based on the sample intervals filled in by pktArrival() this routine takes the interval that happend 'earliest' ST[N], and
  calculates the bitrate obtained in this interval. The result, bitEst, is the returned value. Then it shifts both arrays
  this to create a new sample interval at ST[0].



*/


static double sampleBINS(qd_real *BITS, qd_real *STy, int bins,double tSample, double linkCapacity) {
  double bitEst;
  int i;
  qd_real tSample2=tSample;
  bitEst=to_double(BITS[bins-1]/tSample2);    // Calculate for the bit rate estimate for the last bin, which cannot get any more samples.

#ifdef debug_sample
  printf("sample()\t at %f \n",to_double(STy[0]));
#endif
  if(bitEst>linkCapacity ||  bitEst<0.0) {
    printf("########bit estimation of %f Mbps\n", bitEst/1e6);
  }
#ifdef debug_bps_estm
  printf("S[%12.12f]bps = %f / %f = %f\n",STy[bins-1],BITS[bins-1],tSample,bitEst);
#endif
  for(i=bins-1;i>0;i--){        // Shift the bins; bin[i]=bin[i-1]
    STy[i]=STy[i-1];
    BITS[i]=BITS[i-1];
  }
  BITS[0]=0.0;                    // Initialize the first bin to zero, and the arrival time of the next sample.
  STy[0]=STy[1]+tSample;

#ifdef debug_sample
  printf("--bins--\n");
  for(i=0;i<bins;i++){
    cout << "[" << i << "]" << setiosflags(ios::fixed) << setprecision(12) << to_double(STy[i]) << "=  " << to_double(BITS[i]) << endl;
  }

  cout << " next at " << setiosflags(ios::fixed) << setprecision(12) << to_double(STy[0]) << "<-sample()" << endl;
#endif


  return bitEst;

}

/*
  Created:      2003-02-20 12:40, Patrik.Carlsson@bth.se
  Latest edit:  2003-02-20 10:30, Patrik.Carlsson@bth.se

  Function:
  int payLoadExtraction(*)
  Return value:
  Payload at given level.

  Arguments (In Order):
  Desired level, 0 physical, 1 link, 2 network, 3 transport
  Description:
  Returns the number of bytes a packet contains at level L



*/
//caphead is new one, data is old.

static int payLoadExtraction(int level, const cap_head* caphead) {
	// payload size at physical (ether+network+transport+app)
	if ( level==0 ) {
		return caphead->len;
	};

	// payload size at link  (network+transport+app)
	if ( level==1 ) {
		return caphead->len - sizeof(struct ethhdr);
	};

	const struct ethhdr *ether = caphead->ethhdr;
	const struct ip* ip_hdr = NULL;
	struct tcphdr* tcp = NULL;
	struct udphdr* udp = NULL;
	size_t vlan_offset = 0;

	switch(ntohs(ether->h_proto)) {
	case ETHERTYPE_IP:/* Packet contains an IP, PASS TWO! */
		ip_hdr = (struct ip*)(caphead->payload + sizeof(cap_header) + sizeof(struct ethhdr));
	  ipv4:

		// payload size at network  (transport+app)
		if ( level==2 ) {
			return ntohs(ip_hdr->ip_len)-4*ip_hdr->ip_hl;
		};

		switch(ip_hdr->ip_p) { /* Test what transport protocol is present */
		case IPPROTO_TCP: /* TCP */
			tcp = (struct tcphdr*)(caphead->payload + sizeof(cap_header) + sizeof(struct ethhdr) + vlan_offset + 4*ip_hdr->ip_hl);
			if(level==3) return ntohs(ip_hdr->ip_len)-4*tcp->doff-4*ip_hdr->ip_hl;  // payload size at transport  (app)
			break;
		case IPPROTO_UDP: /* UDP */
			udp = (struct udphdr*)(caphead->payload + sizeof(cap_header) + sizeof(struct ethhdr) + vlan_offset + 4*ip_hdr->ip_hl);
			if(level==3) return ntohs(udp->len)-8;                     // payload size at transport  (app)
			break;
		default:
			fprintf(stderr, "Unknown IP transport protocol: %d\n", ip_hdr->ip_p);
			return 0; /* there is no way to know the actual payload size here */
		}
		break;

	case ETHERTYPE_VLAN:
		ip_hdr = (struct ip*)(caphead->payload + sizeof(cap_header) + sizeof(struct ether_vlan_header));
		vlan_offset = 4;
		goto ipv4;

	case ETHERTYPE_IPV6:
		fprintf(stderr, "IPv6 not handled, ignored\n");
		return 0;

	case ETHERTYPE_ARP:
		fprintf(stderr, "ARP not handled, ignored\n");
		return 0;

	case STPBRIDGES:
		fprintf(stderr, "STP not handled, ignored\n");
		return 0;

	case CDPVTP:
		fprintf(stderr, "CDPVTP not handled, ignored\n");
		return 0;

	default:      /* Packet contains unknown link . */
		fprintf(stderr, "Unknown ETHERTYPE 0x%0x \n", ntohs(ether->h_proto));
		return 0; /* there is no way to know the actual payload size here, a zero will ignore it in the calculation */
	}

	fprintf(stderr, "packet wasn't handled by payLoadExtraction, ignored\n");
	return 0;
}
