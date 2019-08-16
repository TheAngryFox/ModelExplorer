#include <Explorer.h>

int main (int argc, char **argv)
{
    string fileName;
    set<string> flags;
    if(argc>1) 
    {
		bool has_filename = false;
	  	if(argv[argc-1][0]!='-') 
		{
			has_filename = true;
	  		fileName = argv[argc-1];
		}
      	for(int i=0;i<(argc-((has_filename) ? 2 : 1));i++) flags.insert(argv[i+1]);
    }


    al_init_primitives_addon();
    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    if(!al_init()) fprintf(stderr, "failed to initialize allegro!\n");

    Explorer expl(fileName,flags,0.9,0.9);
    expl.run();

    printf("\n\nThe execution was successful!\n\n");

    return 0;
}
