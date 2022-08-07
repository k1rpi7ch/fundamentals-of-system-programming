#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <dlfcn.h>
#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include "kamN3247.h"

typedef int (*ppf_func_t)(const char*, struct option*, size_t);
typedef int (*pgi_func_t)(struct plugin_info*); 

//to save plugin parameters
struct parameters {
    struct option *all_opts; 
    size_t all_opts_len;
    struct option *opts_plug; // to plugin
    size_t opts_plug_len;
    ppf_func_t funct;
    pgi_func_t info;
};

//to go through files and apply plugin
int dir_check (const char* cur_dir, int length, struct parameters plugin_param[], bool is_or, bool is_not) {
    DIR* work_dir = opendir(cur_dir);
    if (work_dir == NULL) {
        fprintf(stderr, "ERROR: No such directory %s\n", cur_dir);
        return -1;
    }
    struct dirent* list_dir;
    list_dir = readdir(work_dir);
    while (list_dir != NULL) {
        if (strcmp(list_dir->d_name, ".") && strcmp(list_dir->d_name, "..")) { //checks that it isn't parent or working directory
            size_t path_len = strlen(cur_dir) + strlen(list_dir->d_name) + 2;
            char* path = malloc(path_len);
            snprintf(path, path_len, "%s/%s", cur_dir, list_dir->d_name);

            if (list_dir->d_type == DT_REG) {
                int is_correct = 0; // counts accepted plugins
                int plugins_used = 0; //counts all plugins
                for (int i = 0; i < length; i++) {
                    if(plugin_param[i].opts_plug_len > 0) {
                        plugins_used++;
                        int frez = plugin_param[i].funct(path, plugin_param[i].opts_plug, plugin_param[i].opts_plug_len);
                        if (frez == 0) is_correct++;
                        if (frez < 0) {
                            free(path);
                            return -1;//to tree
                        }
                    }
                }
                if (is_correct == plugins_used && is_or == 0 && is_not == 0) printf("Found: %s\n", list_dir->d_name); // AND
                else if (is_correct > 0 && is_or == 1 && is_not == 0) printf("Found: %s\n", list_dir->d_name); // OR
                else if (is_correct < plugins_used && is_or == 0 && is_not == 1) printf("Found: %s\n", list_dir->d_name); // NOT_AND
                else if (is_correct == 0 && is_or == 1 && is_not == 1) printf("Found: %s\n", list_dir->d_name); // NOT_OR
            }
            if (list_dir->d_type == DT_DIR) {
                int ares = dir_check(path, length, plugin_param, is_or, is_not);
                if (ares != 0) {
                    free(path);
                    return -1;
                }
            }
            free(path);
        }
        list_dir = readdir(work_dir);
    }
    closedir(work_dir);
    return 0;
}


int p_option (const char* cur_dir, void* dl[], int len) {
    DIR* work_dir = opendir(cur_dir);
    if (work_dir == NULL) {
        fprintf(stderr, " ERROR: No directory %s\n", cur_dir);
        return -1;
    }
    struct dirent* list_dir;
    list_dir = readdir(work_dir);
    int count = 0;
    while (list_dir != NULL && count < len) {
        int nlen = strlen(list_dir->d_name);
        if ((list_dir->d_type == DT_REG) && nlen > 3
            && (list_dir->d_name[nlen-1] == 'o') && (list_dir->d_name[nlen-2] == 's') && (list_dir->d_name[nlen-3] == '.')) {
            size_t filename_len = strlen(cur_dir) + strlen(list_dir->d_name) + 2;
            char* filename = malloc(filename_len);
            sprintf(filename, "%s/%s", cur_dir, list_dir->d_name);
            dl[count] = dlopen(filename, RTLD_LAZY);
            if (dl[count] == NULL) {
                fprintf(stderr, " ERROR: Failed to dlopen %s\n%s\n", list_dir->d_name, dlerror());
                return -1;
            }
            else {
                count++;
            }
            free(filename);
        }
        list_dir = readdir(work_dir);
    }
    closedir(work_dir);
    return 0;
}

//plugin counter
int count_plug (const char* cur_dir, int* len) {
    DIR* work_dir = opendir(cur_dir);
    if (work_dir == NULL) {
        fprintf(stderr, " ERROR: No directory %s\n", cur_dir);
        return -1;
    }
    struct dirent* list_dir;
    list_dir = readdir(work_dir);
    *len = 0;
    while (list_dir != NULL) {
        int nlen = strlen(list_dir->d_name);
        if ((list_dir->d_type == DT_REG) && (list_dir->d_name[nlen-1] == 'o') && (list_dir->d_name[nlen-2] == 's') && (list_dir->d_name[nlen-3] == '.')) {
            (*len)++;
        }
        list_dir = readdir(work_dir);
    }
    closedir(work_dir);
    return 0;
}

void usage(){
    fprintf(stdout, ""
           "Usage:\n"
           "./lab1kamN3247 ./libabcNXXXX.so [options_for_lib] /path/to/directory\n\n"
           "For more info use: ./lab1kamN3247 -h\n"
    );
}


int main(int argc, char *argv[]) {
    struct parameters* plugin_param = 0;
    char *firstfn = 0;
    opterr = 0;
	void** dl=NULL;
// Minimum number of arguments is 2:
// $ program_name lib_name --opt1 number
    if (argc < 2) {        
        usage();
        return 0;
    }

	char** save_argv = calloc(argc, sizeof(char*));
	if(!save_argv) {
		fprintf(stderr, "ERROR: could not allocate for argv copy\n");
		goto END;
	}
    int len = 0;

    if (count_plug(".", &len)) {
        fprintf (stderr, "ERROR: unable to count in current dir\n");
    }
    dl = calloc (len, sizeof(void*));
    if (p_option(".", dl, len)) {
        fprintf (stderr, "ERROR: unable to dlopen libs in a current dir\n");
        goto END;
    }
//here is new code begins (new getopt) 
    
    //short opts except -v and -h
    bool is_or = 0;
    bool is_not = 0;

    int shopt;
    memcpy(save_argv, argv, argc * sizeof(char*));
    while ((shopt = getopt(argc, save_argv, "AONP:")) != -1) {
        switch (shopt) {
            case 'A':
                is_or=0;
                break;
            case 'O':
                is_or = 1;
                break;
            case 'N':
                is_not = 1;
                break;
            case 'P':
                for (int i = 0; i < len; i++) {
                    dlclose (dl[i]);
                }
                free (dl);
                dl = 0;
                if (count_plug(optarg, &len)) {
                    fprintf (stderr, "ERROR: unable to count *.so files\n");
                    goto END;
                }

                dl = calloc (len, sizeof(void*));
                if (p_option(optarg, dl, len)) {
                    fprintf (stderr, "ERROR: unable to process -P\n");
                    goto END;
                }
                break;
        }
    }
	
	plugin_param = calloc(len, sizeof(struct parameters));
    for (int i = 0; i < len; i++){
        plugin_param[i].info = dlsym(dl[i], "plugin_get_info");
        if (!plugin_param[i].info) {
            fprintf(stderr, "ERROR: dlsym() failed for pgi: %s\n", dlerror());
            goto END;
        }
        struct plugin_info pi = {0};
        if (plugin_param[i].info(&pi) < 0) {
            fprintf(stderr, "ERROR: plugin_get_info() failed\n");
            goto END;
        }
        if (pi.sup_opts_len == 0) {
            fprintf(stderr, "ERROR: no options for plugin\n");
            goto END;
        }
        plugin_param[i].funct = dlsym(dl[i], "plugin_process_file");
        if (!plugin_param[i].funct) {
            fprintf(stderr, "ERROR: dlsym() failed for ppf: %s\n", dlerror());
            goto END;
        }
        plugin_param[i].all_opts_len = pi.sup_opts_len;
        plugin_param[i].all_opts = calloc(pi.sup_opts_len + 1, sizeof(struct option));
        if (!plugin_param[i].all_opts) {
            fprintf(stderr, "ERROR: calloc() failed: %s\n", strerror(errno));
            goto END;
        }
        for (size_t j = 0; j < pi.sup_opts_len; j++) {
            memcpy(&plugin_param[i].all_opts[j], &pi.sup_opts[j].opt, sizeof(struct option));
        }
        plugin_param[i].opts_plug_len = 0;
        plugin_param[i].opts_plug = calloc(pi.sup_opts_len, sizeof(struct option));
        if (!plugin_param[i].opts_plug) {
            fprintf(stderr, "ERROR: calloc() failed: %s\n", strerror(errno));
            goto END;
        }
    }
	
 //here is work with -v and -h options
	optind=1;
    memcpy(save_argv, argv, argc * sizeof(char*));
    while ((shopt = getopt(argc, save_argv, "vh")) != -1) {
        switch (shopt) {
            case 'v':
                for (int i = 0; i < len; i++) {
                    struct plugin_info pi = {0};
                    if (plugin_param[i].info(&pi) < 0) {
                        fprintf(stderr, "ERROR: plugin_get_info() failed\n");
                        goto END;
                    }
                    printf("Plugin purpose:\t\t%s\n", pi.plugin_purpose);
                    printf("Plugin author:\t\t%s\n\n", pi.plugin_author);
                }
                goto END;
            case 'h':
			fprintf(stdout,"Supported options for the programm:\n"
									   " -P <dir>                          Full path to plugin directory\n"
									   " -A                                AND operator\n"
									   " -O                                OR operator\n"
									   " -N                                NOT operator\n"
									   " -v                                Show info about plugins\n"
									   " -h                                Help info(show this msg)\n\n");
                for (int i = 0; i < len; i++) {
                    struct plugin_info pi = {0};
                    if (plugin_param[i].info(&pi) < 0) {
                        fprintf(stderr, "ERROR: plugin_get_info() failed\n");
                        goto END;
                    }
                    printf("Supported options for the plugin %d: ", i + 1);
                    if (pi.sup_opts_len > 0) {
                        printf("\n");
                        for (size_t j = 0; j < pi.sup_opts_len; j++) {
                            printf("\t--%s\t\t%s\n", pi.sup_opts[j].opt.name, pi.sup_opts[j].opt_descr);
                        }
                    } else {
                        printf("none (!?)\n");
                    }
                    printf("\n");
                }
                goto END;
        }
    }
// Directory to work with
    firstfn = strdup(argv[argc-1]);
	//work with long opts
    for (int i = 0; i < len; i++) {
		optind=1;
        memcpy(save_argv, argv, argc * sizeof(char*));
        while (1) {
            int opt_ind = 0;
            int ret = getopt_long(argc, save_argv, "", plugin_param[i].all_opts, &opt_ind);
            if (ret == -1) {
                break;
            }
            if(ret != 0) {
                continue;
            }
            // Check options we got up to this moment
            if ((size_t) plugin_param[i].opts_plug_len == plugin_param[i].all_opts_len) {
                fprintf(stderr, "ERROR: too many options!\n");
                goto END;
            }
            // Add this option to array of options in plugin_process_file()
            memcpy(plugin_param[i].opts_plug + plugin_param[i].opts_plug_len, plugin_param[i].all_opts + opt_ind, sizeof(struct option));
            if ((plugin_param[i].all_opts + opt_ind)->has_arg) {
                (plugin_param[i].opts_plug + plugin_param[i].opts_plug_len)->flag = (int *)strdup(optarg);
            }
            plugin_param[i].opts_plug_len++;
        }
    }


    if (getenv("LAB1DEBUG")) {
        for (int i = 0; i < len; i++) {
            for (size_t j = 0; j < plugin_param[i].opts_plug_len; j++) {
                fprintf(stderr, "DEBUG: passing option '%s' with arg '%s'\n",
                        plugin_param[i].opts_plug[j].name,
                        (char *)(plugin_param[i].opts_plug[j].flag));
            }
        }
    }
    

    // Call plugin_process_file()
    errno = 0;
    int r = dir_check(firstfn, len, plugin_param, is_or, is_not);
    fprintf(stdout, "dir_check() returned %d\n", r);
    if (r < 0) {
        fprintf(stdout, "Error information: %s\n", strerror(errno));
    }

    //exiting the program in right way
    END:
    if (plugin_param) {
        for (int i = 0; i < len; i++) {
            for(size_t j = 0; j < plugin_param[i].opts_plug_len; j++) {
                if(plugin_param[i].opts_plug[j].flag) free(plugin_param[i].opts_plug[j].flag);
            }
            if (plugin_param[i].opts_plug) free(plugin_param[i].opts_plug);
            if (plugin_param[i].all_opts) free(plugin_param[i].all_opts);
        }
        free(plugin_param);
    }
    if(save_argv) free(save_argv);
    if (firstfn) free(firstfn);
    if (dl) {
        for (int i = 0; i < len; i++) {
            if(dl[i]) dlclose(dl[i]);
        }
        free (dl);
    }

    return 0;
}