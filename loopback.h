#ifndef PPPOAT_LOOPBACK_H
#define PPPOAT_LOOPBACK_H

int mod_loop(int argc, char **argv, int rd, int wr);

#define LOOP_FIFO_RD "/var/run/pppoat_loop_rd"
#define LOOP_FIFO_WR "/var/run/pppoat_loop_wr"
#define LOOP_BUFSIZ 2048

#endif
