#include <glob.h>   // glob(), globfree()
#include <grp.h>    // getgrnam()
#include <libgen.h> // basename()
#include <stdio.h>  // printf()
#include <stdlib.h> // getenv(), malloc(), free()
#include <string.h> // strcmp()
 
#include <sys/mount.h> // mount(), umount()
#include <sys/types.h> // getgrnam()
 
#define GROUP "sftponly"
#define HOME "/home"
#define SKEL "/etc/skel.sftponly"
#define SOURCE "/data/media"
 
//  clang pam_exec.c -o pam_exec && PAM_TYPE=open_session PAM_USER=user ./pam_exec
 
int main (int argc, char **argv, char **envp) {
    // defined by pam_exec.so
    char *pam_type = getenv("PAM_TYPE");
    char *pam_user = getenv("PAM_USER");
 
    // get users in GROUP
    struct group *record = getgrnam(GROUP);
    char **users = record->gr_mem;
 
    // if user in GROUP
    while (*users) {
        if (strcmp(*users++, pam_user) == 0) {
            // store glob result of SKEL directory
            glob_t globbuf;
            glob(SKEL "/*", 0, NULL, &globbuf);
            char **match = globbuf.gl_pathv;
 
            while (*match) {
                char *path = basename(*match++);
                char *src = malloc(strlen(SOURCE) + strlen(path) + 2);
                char *dest = malloc(strlen(HOME) + strlen(pam_user) + strlen(path) + 3);
 
                sprintf(src, "%s/%s", SOURCE, path);
                sprintf(dest, "%s/%s/%s", HOME, pam_user, path);
 
                if (strcmp(pam_type, "open_session") == 0) {
                    // read+write bind mount
                    mount(src, dest, NULL, MS_BIND, NULL);
                    if ((strcmp(path, "dev") != 0) && (strcmp(path, "Dropbox") != 0)) {
                        // remount read-only except for "dev" and "Dropbox" which retain r+w
                        mount(NULL, dest, NULL, MS_BIND|MS_REMOUNT|MS_RDONLY, NULL);
                    }
                } else if (strcmp(pam_type, "close_session") == 0) {
                        // pam_exec is also called when the session is terminated
                        umount(dest);
                }
                free(src);
                free(dest);
            }
            globfree(&globbuf);
            break; // return after first match
        }
    }
    return 0;
}
