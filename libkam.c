#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdint.h>
#include "kamN3247.h"

#define _CRT_SECURE_NO_WARNINGS //to disable unsafe warnings

//plugin description
static char *purpose = "Checks if file contains single-precision floating-point number in binary form";
static char *author = "Magaskin Kirill Anatolevich N3247";

#define OPT_FLOAT_IN_BINARY "float-bin"

static struct plugin_option opts[] = {
        {
            {OPT_FLOAT_IN_BINARY, 
			required_argument, 				//needs argument
			0, 0,			
			},				//0 to return val(number)
            "Pass float number to plugin"
        }
};
//using Knuth-Morris-Pratt algorithm as the most effective string-searching algorithm
int searchKMP(char *string, int N, char *substring, int M){
    int ret = -1;
    int R = 256;
    int **dfa = calloc(R, sizeof(int*));
    if(!dfa){
        goto END_SEARCH;
    }
    for(int i = 0; i<R; i++){
        dfa[i] = calloc(M, sizeof(int));
        if(!dfa[i]){
            goto END_SEARCH;
        }
    }
    dfa[(int)(substring[0])][0] = 1;
    for(int X=0, j=1; j<M; j++){
        for(int c=0; c<R; c++){
            dfa[c][j] = dfa[c][X];
        }
        dfa[(int)(substring[j])&0xff][j] = j+1;
        X = dfa[(int)(substring[j])&0xff][X];
    }
    int k, j;
    for(k=0, j=0; k<N && j<M; k++){
        j = dfa[(int)(string[k])&0xff][j];
    }
    ret = j==M ? k-M:N; //returns position of string from the beginning if we have substring or file length if there is nothing, -1 is error of search
    END_SEARCH:
    if(dfa){
        for(int i=0; i<R; i++){
            if(dfa[i]) free(dfa[i]);
        }
        free(dfa);
    }
    if(ret!=-1)
        return ret/8;//8 is here because we search in a binary format of string and 1 byte==8bits
    else
        return ret;
}

//transfers file bytes to binary string
char* byte_to_str_bin(char *input,int len){
    char *buf=calloc(len*8+1,sizeof(char));
    char n;
    for (int i=0;i<len;i++)
    {
        n=input[i];
        for(int j=0;j<8;j++)
        {
            if(n & 0x80000000)
                strncat(buf, "1", 2);
            else
                strncat(buf, "0", 2);//2 is here to stop warnings [-Wstringop-overflow=]
            n<<=1;
        }
    }
    return buf;
}

//transfers argument(float number) to binary string
char* num_to_bin(char* arg){
    int bit=0, i, j;
    float x=strtof(arg, NULL);
    unsigned char *gg = (unsigned char*)&x;
    unsigned char tmp;
    char *buf=calloc(32+1,sizeof(char));
    for (i=0;i<4;i++){
        tmp = *gg;
        for (j=0; j < 8; j++) {
            bit = (tmp & 1);
            if (bit)
                strncat(buf, "1", 2);
            else 
                strncat(buf, "0", 2);
            tmp >>= 1;
        }
    gg++;
    }
    char* output=calloc(32+1,sizeof(char));
    for (i=31;i>=0;i--){
        strncat(output,&buf[i],1);
	}
	if (buf){
		free(buf);
	}
    return output;
}

//reverses argument to little-endian format
char* to_little(char* str_big)
{
    char* str_lit=calloc(32+1,sizeof(char));
    strncat(str_lit,&str_big[24],8);
    strncat(str_lit,&str_big[16],8);
    strncat(str_lit,&str_big[8],8);
    strncat(str_lit,str_big,8);
    return str_lit;
}

//API FUNCTIONS
int plugin_get_info(struct plugin_info* ppi){//fills plugin info structure
    if(!ppi){
        fprintf(stderr, "ERROR: ppi pointer is null - cannot write info to plugin info\n");
        return -1;
    }
    ppi->plugin_purpose = purpose;
    ppi->plugin_author = author;
    ppi->sup_opts = opts;
    ppi->sup_opts_len = sizeof(opts)/sizeof(opts[0]);
    return 0;
}

int plugin_process_file(const char *fname, struct option in_opts[], size_t in_opts_len){
    char* float_num = NULL; //argument e.g 2.71
    char* float_bin_big = NULL;//argument in binary big-endian e.g 01000000001011010111000010100100
    char* float_bin_lit = NULL;//argument in binary little-endian e.g 10100100011100000010110101000000
    char* ptr_file = NULL;//pointer to file content
    char* ptr_fbin=NULL;//pointer to file content(binary)
    int retval=-1;//returns error by default
    int debug_mode = 0;//by default is disabled
	int stream=0;//to stop warning -Werror=maybe-uninitialized
    char *DEBUG = getenv("LAB1DEBUG");
    if(DEBUG){
        debug_mode = 1;//enables debug_mode
    }
    if (!fname || !in_opts) {
        errno = EINVAL;
        retval = -1;
        goto END;
    }
    if(in_opts_len==0){
        errno = EINVAL;
        retval = 1;
        goto END;
    }
    if(!in_opts){
        errno = EINVAL;
        retval = -1;
        goto END;
    }
    for(size_t i=0; i<in_opts_len; i++){
        if(strncmp(in_opts[i].name, "float-bin", 9)==0){
            float_num = (char *)in_opts[i].flag;
            break;
        }
    }
    if(float_num==NULL){
        fprintf(stderr, "ERROR: No option! Example: --float-bin 2.71\n");
        retval = 1;
        goto END;
    }

    float_bin_big= num_to_bin(float_num);
    float_bin_lit=to_little(float_bin_big);
    if(debug_mode) fprintf(stdout, "DEBUG: binary big-endian = %s; binary little-endian = %s\n", float_bin_big, float_bin_lit);

    //begin work with file
    stream=open(fname,O_RDONLY);
    if(stream<0){
        errno = ENOENT;
        retval = -1;
        goto END;
    }
	else if (DEBUG) fprintf(stdout,"DEBUG: File %s was opened\n", fname);
    struct stat statistics;
    int err=fstat(stream,&statistics);
    if(err < 0){
        retval = -1;
        goto END;
    }
    if(!statistics.st_size){
        retval = 1;
        goto END;
    }
    ptr_file = mmap(NULL, statistics.st_size, PROT_READ, MAP_SHARED, stream, 0);//Pages may be read and the mapping are visible to other processes.
    if(ptr_file == MAP_FAILED){
        fprintf(stderr, "ERROR: Mapping unsuccessful: %s\n", strerror(errno));
        return retval;
    }
    ptr_fbin = byte_to_str_bin(ptr_file,statistics.st_size);//convert to binary

    int found_big=searchKMP(ptr_fbin,statistics.st_size*8,float_bin_big,32);
    int found_lit=searchKMP(ptr_fbin,statistics.st_size*8,float_bin_lit,32);

    retval = found_big==statistics.st_size?1:0;
	if(debug_mode==1) fprintf(stdout,"DEBUG: retval is %d, statistics size is %ld\n",retval,statistics.st_size);
	
    if(debug_mode==1 && retval==0) fprintf(stdout,"DEBUG: found %s (big-endian) at %d\n",float_bin_big,found_big);
	
    if (retval!=0){
        retval = found_lit==statistics.st_size?1:0;
        if(debug_mode==1 && retval==0)  fprintf(stdout,"DEBUG: found %s (little-endian) at %d\n",float_bin_lit,found_lit);
        }
    if(debug_mode==1 && retval==1)  fprintf(stdout,"DEBUG: nothing found here!\n");
    //exiting plugin and file in right way
    END:
    if(stream){
        close(stream);
        if(debug_mode) fprintf(stdout, "DEBUG: stream was closed!\n");
    }
    if(ptr_file){
        err = munmap(ptr_file, statistics.st_size);
        if(err<0) fprintf(stderr, "ERROR: UnMapping unsuccessful! Errno = %s\n", strerror(errno));
    }
    if(ptr_fbin){
        free(ptr_fbin);
        if (debug_mode) fprintf(stdout, "DEBUG: ptr_fbin was freed\n");
    }
    if(float_bin_big){
        free(float_bin_big);
        if(debug_mode) fprintf(stdout, "DEBUG: float_bin_big was freed\n");
    }
    if(float_bin_lit){
        free(float_bin_lit);
        if(debug_mode) fprintf(stdout, "DEBUG: float_bin_lit was freed\n");
    }
    return retval;//return 0 means plugin found smth in file,1 means nothing found or you didn't specified file, -1 is an error
}
