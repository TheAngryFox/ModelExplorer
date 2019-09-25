#include <ETracer.h>

int main (int argc, char **argv)
{
    string fileName;
    vector<string> flags;
	bool help_case = false;
    if(argc>1) 
    {
		for(size_t i=0;i<argc;i++)
		{
			if(string(argv[i])=="--help") 
			{
				printf("\n ErrorTracer argument information:"
						"\n-time: \tRecord the execution time of the ErrorTracer algorithms.\n"
						"-o: \tSpecify output file. Should be followed by the folder\n\tor file name "
						"to which the error tracking output of the \n\tErrorTracer should be saved. "
						"If this flag is not specified, \n\tthe output will be saved to the same folder "
						"as the executable \n\titself, and it will get the name \"ErroTracer_results_test.xml\".\n\n");
				help_case = true;		
				break;
			} 
		}

		fileName = argv[argc-1];
		for(int i=0;i<(argc-2);i++) 
		{
			flags.push_back(argv[i+1]);
		}
    }

	if(!help_case)
	{
		int result;
		ETracer expl(fileName,flags,result);

		if(result==0) printf("\n\nThe execution of ErrorTracer was successful!\n\n");	
		return result;
	}
	else return 0;
}
