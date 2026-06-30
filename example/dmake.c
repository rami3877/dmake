#define BUILDER_IMPLEMENTATION
#include "../dmake.h"

int main(int argc, char **argv) {
    setGlobalCompiler("gcc");
    setBuildDir("build");
    check_and_rebuild_self(argc, argv, "dmake.c");
    BObject math_lib = {
        .name = "my_lib",
        .type = B_SHARED,
        .files = (const i8*[]){
            "lib.c",
            null
        }

    };

    BObject main_app = {
        .name = "main2",
        .type = B_EXEC,
        .flags = (const i8*[]){"-std=c11", "-ggdb", null},
        .files = (const i8*[]){
            "main.c",     
            null
        },
		.librs  =(const i8 *[]){"array" , null}, 
		.object =(BObject *[] ){&math_lib , null},
    };
    objectAdd(&main_app);



    return 0;
}


