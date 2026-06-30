#define BUILDER_IMPLEMENTATION

//#include "/home/rami/usr/include/make.h"
#include "make.h"

int main(int argc, char **argv) {
    setGlobalCompiler("gcc");
    setBuildDir("build");

    check_and_rebuild_self(argc, argv, "make.c");
    BObject math_lib = {
        .name = "my_lib",
        .type = B_SHARED,
        .files = (const i8*[]){
            "lib.c",
            null
        }

    };

    BObject main_app2 = {
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
    BObject main_app = {
        .name = "main",
        .type = B_EXEC,
        .flags = (const i8*[]){"-std=c11", "-ggdb", null},
        .files = (const i8*[]){
            "main.c",     
            null
        },
		.librs  =(const i8 *[]){"array" , null}, 
		.object =(BObject *[] ){&math_lib ,null},
    };
    objectAdd(&main_app);
    objectAdd(&main_app2);


	system("./build/main.elf");
    return 0;
}


