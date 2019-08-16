#include <ETracer.h>

int main (int argc, char **argv)
{
    string fileName;
    vector<string> flags;
    if(argc>1) 
    {
      fileName = argv[argc-1];
      for(int i=0;i<(argc-2);i++) 
	  {
		  flags.push_back(argv[i+1]);
	  }
    }

	int result;
    ETracer expl(fileName,flags,result);

    if(result==0) printf("\n\nThe execution of ErrorTracer was successful!\n\n");	
	return result;
}
