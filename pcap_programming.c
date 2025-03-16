#include <stdlib.h>
#include <stdio.h>
#include <pcap.h>
#include <arpa/inet.h>


/* Ethernet header */
struct ethheader {
  u_char  ether_dhost[6]; /* destination host address */
  u_char  ether_shost[6]; /* source host address */
  u_short ether_type;     /* protocol type (IP, ARP, RARP, etc) */
};

/* IP Header */
struct ipheader {
  unsigned char      iph_ihl:4, //IP header length
                     iph_ver:4; //IP version
  unsigned char      iph_tos; //Type of service
  unsigned short int iph_len; //IP Packet length (data + header)
  unsigned short int iph_ident; //Identification
  unsigned short int iph_flag:3, //Fragmentation flags
                     iph_offset:13; //Flags offset
  unsigned char      iph_ttl; //Time to Live
  unsigned char      iph_protocol; //Protocol type
  unsigned short int iph_chksum; //IP datagram checksum
  struct  in_addr    iph_sourceip; //Source IP address
  struct  in_addr    iph_destip;   //Destination IP address
};

/* TCP Header */
struct tcpheader {
  u_short tcp_sport;               /* source port */
  u_short tcp_dport;               /* destination port */
  u_int   tcp_seq;                 /* sequence number */
  u_int   tcp_ack;                 /* acknowledgement number */
  u_char  tcp_offx2;               /* data offset, rsvd */
#define TH_OFF(th)      (((th)->tcp_offx2 & 0xf0) >> 4)
  u_char  tcp_flags;
#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20
#define TH_ECE  0x40
#define TH_CWR  0x80
#define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
  u_short tcp_win;                 /* window */
  u_short tcp_sum;                 /* checksum */
  u_short tcp_urp;                 /* urgent pointer */
};

void print_mac(u_char host[6]){
  printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
          host[0], host[1], host[2], host[3], host[4], host[5]);
}

void got_packet(u_char *args, const struct pcap_pkthdr *header,
                              const u_char *packet)
{
  struct ethheader *eth = (struct ethheader *)packet;
  printf("eth    From: "); print_mac(eth->ether_shost);   
  printf("eth      To: "); print_mac(eth->ether_dhost); 

  if (ntohs(eth->ether_type) == 0x0800) { // 0x0800 is IP type
    struct ipheader * ip = (struct ipheader *)
                           (packet + sizeof(struct ethheader)); 
    printf("ip     From: %s\n", inet_ntoa(ip->iph_sourceip));   
    printf("ip       To: %s\n", inet_ntoa(ip->iph_destip));    

    struct tcpheader * tcp = (struct tcpheader *)
                            ((u_char *)ip + (ip->iph_ihl*4));                 
    printf("tcp    From: %d\n", ntohs(tcp->tcp_sport));   
    printf("tcp      To: %d\n", ntohs(tcp->tcp_dport));
    
    u_char *message = (u_char *)tcp + (TH_OFF(tcp))*4;
    int message_len = ntohs(ip->iph_len) - ((ip->iph_ihl * 4) + (TH_OFF(tcp) * 4));
    if(message_len > 64){
      //printf("message_len: %d\n", message_len);
        printf("message:\n");
        for (int i = 0; i < 64; i++)
          printf("%c", message[i]);
    }
    
    printf("\n-------------------\n");
  }
}

int main()
{
  pcap_t *handle;
  char errbuf[PCAP_ERRBUF_SIZE];
  struct bpf_program fp;
  char filter_exp[] = "tcp";
  bpf_u_int32 net;

  // Step 1: Open live pcap session on NIC with name enp0s3
  handle = pcap_open_live("enp0s3", BUFSIZ, 1, 1000, errbuf);

  // Step 2: Compile filter_exp into BPF psuedo-code
  pcap_compile(handle, &fp, filter_exp, 0, net);
  if (pcap_setfilter(handle, &fp) !=0) {
      pcap_perror(handle, "Error:");
      exit(EXIT_FAILURE);
  }

  // Step 3: Capture packets
  pcap_loop(handle, -1, got_packet, NULL);

  pcap_close(handle);   //Close the handle
  return 0;
}


