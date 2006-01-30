/*
This module_helper program will be installed setuid and allows
the user to add and remove a whitelist of modules necessary to
run EMC.  Without a scheme like this, we have to rely on sudo
to start AND EXIT our program, and that may require the user to
enter a password.  Prompting for a password to exit a program
is bad.  If the user cancels at that phase of the run, it's also
bad since we leave realtime modules inserted and he'll probably
end up being forced to reboot.

In summary, I don't like this any more than you do, but I can't
think of a better way.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* module name, between last / and ., must be one of these */

char *module_whitelist[] = {
    "rtai_math", "rtai_sem", "rtai_shm", "rtai_fifos", "rtai_up", 
    "rtai_hal", "adeos",

    "blocks", "classicladder_rt", "debounce", "encoder", "encoder_ratio",
    "extint", "fifotask", "freqgen", "hal_ax5214h", "hal_evoreg",
    "hal_lib", "hal_m5i20", "hal_motenc", "hal_parport", "hal_ppmc",
    "hal_skeleton", "hal_stg", "hal_tiro", "master", "motmod", "pid",
    "rtapi", "scope_rt", "shmemtask", "siggen", "slave", "stepgen",
    "supply", "threads", "timedelay", "timertask", 

    NULL
};

/* module path must start with this. */

char *path_whitelist[] = {
    "/lib/modules", "/usr/realtime",
    NULL
};

void error(int argc, char **argv) {
    int i;
    char *prog = argv[0];

    /* drop root privs permanently */
    setuid(getuid());

    fprintf(stderr, "%s: Invalid usage with args:", argv[0]);
    for(i=1; i<argc; i++) {
        fprintf(stderr, " %s", argv[i]);
    }
    fprintf(stderr, "\n\nUsage: %s insert /path/to/module [param1=value1 ...]\n", prog);
    fprintf(stderr, "\nwhere module is one of:\n");
    for(i=0; module_whitelist[i]; i++) {
        fprintf(stderr, "\t%s\n", module_whitelist[i]);
    }
    fprintf(stderr, "\nand the path starts with one of:\n");
    for(i=0; path_whitelist[i]; i++) {
        fprintf(stderr, "\t%s\n", path_whitelist[i]);
    }

    fprintf(stderr, "OR\n\n%s remove module\n\nwhere module is one of"
                    " the modules listed above.\n\n", prog);
    exit(1);
}

int main(int argc, char **argv) {
    char *mod;
    char *last_slash;
    int i;
    int inserting = 0;
    int module_ok = 0, path_ok = 0;
    char **exec_argv;

    /* drop root privs temporarily */
    seteuid(getuid());

    if(argc < 3) error(argc, argv);
    if(strcmp(argv[1], "insert") && strcmp(argv[1], "remove")) error(argc, argv);
    exec_argv = malloc(argc * sizeof(char *));

    if(!strcmp(argv[1], "insert"))
        inserting = 1;

    mod = argv[2];

    if(inserting) {
        last_slash = strrchr(mod, '/');

        if(!last_slash || strstr(mod, "..")) error(argc, argv);

        for(i=0; module_whitelist[i]; i++)
            if(!strncmp(last_slash+1, module_whitelist[i], 
                        strlen(module_whitelist[i])))
                module_ok = 1;

        for(i=0; path_whitelist[i]; i++)
            if(!strncmp(mod, path_whitelist[i], strlen(path_whitelist[i])))
                path_ok = 1;

        if(!path_ok || !module_ok) error(argc, argv);
        exec_argv[0] = "/sbin/insmod";
        exec_argv[1] = mod;

        for(i=3; i<argc; i++) {
            exec_argv[i-1] = argv[i];
        }
        exec_argv[argc-1] = NULL;
    } else {
        for(i=0; module_whitelist[i]; i++)
            if(!strcmp(module_whitelist[i], mod)) 
                module_ok = 1;

        if(!module_ok) error(argc, argv);

        exec_argv[0] = "/sbin/rmmod";
        exec_argv[1] = mod;
        exec_argv[2] = NULL;
    }

    /* reinstate root privs */
    seteuid(0);
    execve(exec_argv[0], exec_argv, NULL);

    perror("execv failed");
    return 1;
}
