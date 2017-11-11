#ifndef _MUIC_COAGENT_
#define _MUIC_COAGENT_

/*
  * B[7:0] : cmd
  * B[15:8] : parameter
  *
  */

enum coagent_cmd {
	COA_INIT = 0,
	COA_AFC,
};	

enum coagent_param {
	AFC_START = 0,
	AFC_STOP,
};	

#define COAGENT_CMD(n) (n & 0xff)
#define COAGENT_PARAM(n) ((n >> 8) & 0xff)

extern int coagent_in(unsigned int *pbuf);
extern int coagent_out(unsigned int *pbuf);
extern bool coagent_alive(void);
#endif
