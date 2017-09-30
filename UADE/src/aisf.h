#ifndef _UADE_AISF_H_
#define _UADE_AISF_H_

struct aisf_aud_state {
  unsigned int dat[4];
  unsigned int len[4];
  unsigned int per[4];
  unsigned int pt[4];
  unsigned int vol[4];
  unsigned int dma;
  unsigned char *mem;
};

struct aisf_cmd_b {
  unsigned int cycles;
  unsigned char cmd;
  unsigned char nr;
  unsigned char data;
};

struct aisf_cmd_w {
  unsigned int cycles;
  unsigned char cmd;
  unsigned char nr;
  unsigned short data;
};

struct aisf_cmd_l {
  unsigned int cycles;
  unsigned char cmd;
  unsigned char nr;
  unsigned int data;
};

#define AISF_END 0
#define AISF_DMA 1
#define AISF_DAT 2
#define AISF_LEN 3
#define AISF_PER 4
#define AISF_PTH 5
#define AISF_PTL 6
#define AISF_VOL 7
#define AISF_STRIKE 8

#endif
