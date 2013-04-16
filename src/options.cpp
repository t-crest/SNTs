#include "options.h"

using namespace std;

string get_stat_name(int argc, char *argv[]) 
{
	string ret;
	ret += "stat";

	for (int i = 1; i < argc; i++) {
		string tmp = string(argv[i]);
		for (int i = 0; i < tmp.size(); i++) if (tmp[i] == '/' || tmp[i] =='\\') tmp[i] = '_';
		ret += tmp;
	}
	ret += "__";

	{
		char buff[30];
		time_t now = time(NULL);
		strftime(buff, 30, "%Y-%m-%d_%H:%M:%S", localtime(&now));	
		ret += string(buff);
	}
	ret += "__";
	
	std::random_device get_rand;
	ret += ::lex_cast<string>(get_rand());
	
	return ret;
}

options::options(int argc, char *argv[])
// Option defaults:
:	input_platform(""),
	input_com(""),
	output_file(""),
	metaheuristic(ERR),
	draw(false),
	meta_inital(ERR),
	save_best(true), // normally, we want to remember the best globally solution
	cal_stats(false),
	run_for(0),
	beta_percent(-1.0)
//	stat_file()
//	stat_file(get_stat_name(argc, argv).c_str(), fstream::out)
{
	bool output = false;
	
	static struct option long_options[] =
	{
		{"meta",          required_argument, 0, 'm'},
		{"initial",       required_argument, 0, 'i'},
		{"platform",      required_argument, 0, 'p'},
		{"communication", required_argument, 0, 'c'},
		{"schedule",      required_argument, 0, 's'},
		{"time",          required_argument, 0, 't'},
		{"cal-stats",	  no_argument,		 0, 'a'},
		{"draw",          no_argument,       0, 'd'},
		{"quick",         no_argument,       0, 'q'},
		{"beta",          required_argument, 0, 'b'},
		{"help",          no_argument,       0, 'h'},
		{0,0,0,0}
	};
	int option_index = 0;
	
	/* Set options as specified by user */
	for (int c; (c = getopt_long(argc, argv, "m:i:p:c:s:t:adqb:h", long_options, &option_index)) != -1;) {
		switch (c) {
			case  0 :	/* The option sets a flag */					break;
			case 'm':	metaheuristic = parse_meta_t(string(optarg));	break; // m for chosen metaheuristic
			case 'i':	meta_inital = parse_init_t(string(optarg));		break; // i for initial sol
			case 'p':	input_platform = optarg;						break; // p for xml input file describing the platform
			case 'c':	input_com = optarg;								break; // c for xml input file describing the communicaiton
			case 's':   output_file = optarg; output = true;			break; // s for specifying the output schedule
			case 't':	run_for = ::lex_cast<time_t>(string(optarg));	break; // t for run time, in seconds
			case 'a':	stat_file.open(get_stat_name(argc, argv).c_str(), fstream::out);
						cal_stats = true;								break; // a for calculation of bounds
			case 'd':	draw = true;									break; // d for draw
			case 'q':	save_best = false;								break; // q for quick
			case 'b':	beta_percent = ::lex_cast<float>(string(optarg)); break; // b for beta_percent
			case 'h':   print_help();									break; // h for the help menu
			default:	ensure(false, "Unknown flag " << c << ".");
		}
	}
	
	if (argc < 2){ // If no comand line options are given, the help menu is printet.
		print_help();
	}
	
	/* Some input validation */
	ensure(input_platform.size() > 0, "Empty file name given for platform specification.");
	ensure(metaheuristic != ERR, "Metaheuristic must be set to GRASP or ALNS, etc.");
	if (meta_inital != ERR)
		ensure(metaheuristic == ALNS, "ALNS-inital given, but we don't run ALNS");
	if (metaheuristic == ALNS)
		ensure(meta_inital != ERR, "ALNS specified, but no inital solution specified");
	
	const bool both_alns = (metaheuristic == ALNS && meta_inital == ALNS);
	ensure(!both_alns, "Can not use ALNS as initial solution for ALNS");

	if (metaheuristic == GRASP || metaheuristic == ALNS)
		ensure(run_for != 0, "Not specified how long the metaheuristic should run");

	if (metaheuristic == GRASP)
		ensure(0.0 <= beta_percent && beta_percent <= 1.0, "Beta not from 0.0 to 1.0");
	
	if (output){
		ensure(output_file.size() > 0, "Empty output directory given.");
		string extension = output_file.substr(output_file.find_last_of(".") + 1);
		ensure((extension == "xml") || (extension == "XML"),"File extension of output file was not .xml or .XML");
	}
}

options::meta_t options::parse_meta_t(string str) 
{
	meta_t ret = ERR;
	if		(str=="RANDOM")		ret=RANDOM;
	else if	(str=="BAD_RANDOM")	ret=BAD_RANDOM;
	else if (str=="GREEDY")		ret=GREEDY;
	else if (str=="rGREEDY")	ret=rGREEDY;
	else if (str=="CROSS")		ret=CROSS;
	else if (str=="GRASP")		ret=GRASP;		
	else if (str=="ALNS")		ret=ALNS;
	return ret;
}

options::meta_t options::parse_init_t(string str) 
{
	meta_t ret = ERR;
	if		(str=="RANDOM")		ret=RANDOM;
	else if	(str=="BAD_RANDOM")	ret=BAD_RANDOM;
	else if (str=="GREEDY")		ret=GREEDY;
	else if (str=="rGREEDY")	ret=rGREEDY;
	else if (str=="CROSS")		ret=CROSS;
	return ret;
}

void options::print_help()
{
	cout << endl << "Help menu for SNTs" << endl;
	cout << "\tMandatory options:" << endl; 
	print_option('m',"meta","Choose the Metaheuristic to apply schedule [GRASP, ALNS].");
	print_option('p',"platform","The file containging a specification of the platform.");
	print_option('i',"initial","Choose the initial solutionused by the Metaheuristics [RANDOM, BAD_RANDOM, GREEDY, rGREEDY].");
	print_option('t',"time","The time in seconds for which the metaheuristic should run.");
	cout << endl;
	cout << "\tOptional options:" << endl;
	print_option('c',"communication","The file containing the scheduling problem. If not specified the scheduler assumes all-to-all communication.");
	print_option('a',"cal-stats","If specified the scheduler will output statistical data such as link utilization and lower bound on the schedule");
	print_option('d',"draw","If specified the network is drawn in an SVG file.");
	print_option('q',"quick","If specified the scheulder will make a dry run, does not save the result.");
	print_option('b',"beta","Specify the beta value, only applicable when using the GRASP metaheuristic.");
	print_option('s',"schedule","Specify the output directory for the generated XML schedule.");
	print_option('h',"help","Shows the help menu, I guess you know that.");
	cout << endl;
	
	delete this;
	return exit(0);
	
}

void options::print_option(char opt, string text){
	cout << "\t-" << opt << "\t\t" << text << endl;
}

void options::print_option(char opt, string long_opt, string text){
	if(long_opt.length() > 10) {
		cout << "\t-" << opt << " --" << long_opt << endl;
		cout << "\t\t\t" << text << endl;
	} else {
		cout << "\t-" << opt << " --" << long_opt << "\t" << text << endl << endl;
	}
}

options::~options()
{
	this->stat_file.close();
}


namespace global {
	options* opts;
}


