#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <ntrack_rbf.h>
#include <ntrack_log.h>

#define SHM_NAME "/dev/mem"

void hexdump(FILE *fp, const void *data, int size)
{
    /* dumps size bytes of *data to stdout. Looks like:
     * [0000] 75 6E 6B 6E 6F 77 6E 20
     *                  30 FF 00 00 00 00 39 00 unknown 0.....9.
     * (in a single line of course)
     */

    const unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};
    for(n=1;n<=size;n++) {
        if (n%16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
               (unsigned int)(p-(const unsigned char *)data));
        }
            
        c = *p;
        if (c<'!' || c>'}') {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02x ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

        if(n%16 == 0) { 
            /* line completed */
            fprintf(fp, "[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if(n%8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        fprintf(fp, "[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
    }
}

int main(int argc, char *argv[])
{
	cpu_set_t set;
	rbf_t *rbp;
	void *base_addr, *p;
	uint32_t size = (RBF_NODE_SIZE) * 1024 + sizeof(rbf_hdr_t);

	CPU_ZERO(&set);

	int fd = open(SHM_NAME, O_RDWR);
	if(fd == -1) {
		nt_error("open shm.\n");
		exit(EXIT_FAILURE);
	}

	base_addr = mmap(0, 4<<20, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 16<<20);
	if (!base_addr) {
		perror("mem map.\n");
		exit(EXIT_FAILURE);
	}

	// memset(base_addr, 0xff, size);
	rbp = rbf_init(base_addr, size);
	if(!rbp) {
		perror("rbp init\n");
		exit(EXIT_FAILURE);
	}
	
	CPU_SET(0, &set);
	if(sched_setaffinity(getpid(), sizeof(set), &set) == -1) {
		perror("set p affinity.\n");
		exit(EXIT_FAILURE);
	}
	// fprintf(stderr, "%d->%d core: %d\n", getppid(), getpid(), sched_getcpu());

	while(1) {
		p = rbf_get_data(rbp);
		if (!p) {
			fprintf(stderr, "read empty.\n");
			sleep(1);
			continue;
		}
		nt_dump(p, 64, "node");
		rbf_release_data(rbp);
	}

	return 0;
}
