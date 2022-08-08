#ifndef _PLUGIN_API_H
#define _PLUGIN_API_H

#include <getopt.h>
#include <stddef.h> //for size_t, NULL offsetof() etc.

#define _XOPEN_SOURCE 500 //  POSIX 1995 for nftw()
#include <ftw.h> //for file tree traversal

#define PLUGIN_API_VERSION  1

/*
    A structure describing the option supported by the plugin.
*/
struct plugin_option {
    /* Option in the format supported by getopt_long (man 3 getopt_long). */
    struct option opt;
    /* Description of the option provided by the plugin. */
    const char *opt_descr;
};

/*
    Structure containing information about the plugin.
*/
struct plugin_info {
    /* Plugin purpose */
    const char *plugin_purpose;
    /* Plugin author, i.e. "John Doe" */
    const char *plugin_author;
    /* Length of options list */
    size_t sup_opts_len;
    /* List of options, supported by plugin */
    struct plugin_option *sup_opts;
};


int plugin_get_info(struct plugin_info* ppi);
/*
    plugin_get_info()
    
    A function that allows you to get information about the plugin.

    Arguments:
    ppi is the address of the structure that the plugin fills in with information.

    Return value:
    0 - if successful,
    < 0 - in case of failure (in this case it is impossible to continue working with this plugin).
*/



int plugin_process_file(const char *fname,
        struct option in_opts[],
        size_t in_opts_len);
/*
    plugin_process_file()
    
    A function that allows you to find out whether the file meets the specified criteria.
    
    Arguments:
        fname is the path to the file (full or relative), which is checked for
    compliance with the criteria set using the in_opts array.

        in_opts - a list of options (search criteria) that are passed to the plugin.
            struct option {
               const char *name;
               int         has_arg;
               int        *flag;
               int         val;
            };
            The name field is used to transmit the name of the option, the flag field is used to transmit
            option values (as a string). If the option has an argument, the has_arg field
            set to a non-zero value. The val field is not used.
           
        in_opts_len - the length of the options list.
                    
    Return value:
          0 - the file meets the specified criteria,
        > 0 - the file does NOT meet the specified criteria,
        < 0 - an error occurred during operation
        
    In case an error has occurred, the errno variable should be set 
    to the appropriate value.
*/
        
#endif
