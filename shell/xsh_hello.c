/* xsh_hello.c - xsh_hello */

#include <xinu.h>

/*------------------------------------------------------------------------
 * xsh_hello - shell command to greet user based on string input
 *------------------------------------------------------------------------
 */
shellcmd xsh_hello(int nargs, char *args[])
{
    if(nargs < 2){
        printf("Too few arguments\n");    
        printf("Usage: xsh$ hello <string>\n");    
        return 1;
    }
    else if(nargs == 2){
        printf("Hello %s, Welcome to the world of Xinu!!\n", args[1]);
        return 0;
    }
    else{
        printf("Too many arguments\n");    
        printf("Usage: xsh$ hello <string>\n");
        return 1;
    }
}