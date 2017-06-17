/* 
 * vim: ts=8 sw=8
 * calc.c - Floating-point calculator V1.00 (C) Richard K. Lloyd 1992-2002
 * Compile using gcc -O -s -o calc calc.c -lm 
 */

#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>

#include <readline/readline.h>
#include <readline/history.h>

#define VERSION "1.00"
#define MAXFACTOR  148  /* Get that lipstick out ! */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double drand48(); /* Aaargh ! You shouldn't need this !! */
extern int optind;
extern char *getenv();
extern char *strrchr();
void level_0();

char *	expr_line;
char tempstr[BUFSIZ + 1];
int exp_len,hex_format;
int exp_error;
char err_string[BUFSIZ + 1];
char expression[BUFSIZ + 1],answer_str[BUFSIZ + 1];
int scan,equals,interact,goteof,needCR;
double facts[MAXFACTOR];
double operand;
char progname[BUFSIZ + 1];
char *envstr;

void basename(slash,fullname)
char *slash,*fullname;
{
   char *slashptr;
   if ((slashptr=strrchr(fullname,'/'))==NULL)
      strcpy(slash,fullname);
   else
      strcpy(slash,&slashptr[1]);
}

char *substr(mainstr,startpos,length)
char *mainstr;
int startpos;
int length;
{
   int mainsize=strlen(mainstr);
   int newpos=0;
   int oldpos=startpos-1;
   if (startpos<1 || startpos>mainsize || length<1) return("");
   while (oldpos<mainsize && newpos<length)
   {
      tempstr[newpos++]=mainstr[oldpos++];
   }
   tempstr[newpos]='\0';
   return(tempstr);
}

int instr(mainstr,slicechar)
char *mainstr;
char slicechar;
{
   int mainlen=strlen(mainstr);
   int slicepos=0;
   int found=FALSE;
   while (slicepos<mainlen && !found)
   {
      found=(mainstr[slicepos++]==slicechar);
   }
   if (found) return(slicepos); else return(0);
}

void randomize(newseed)
long newseed;
{
   srand48(newseed);
}

double randval(upper)
long upper;
{
   if (upper<0) { randomize(-upper); return(upper); }
   if (upper<=1) return(drand48());
   return((long)(drand48()*upper+1));
}

void strip_expression()
{
   int copy1=0;
   int copy2=0;
   exp_len=strlen(expr_line);
   while (copy2<exp_len)
   {
      if (expr_line[copy2]==' ')
         copy2++;
      else
         expr_line[copy1++]=toupper(expr_line[copy2++]);
   }
   exp_len=copy1;
   expr_line[exp_len]='\0';
}

void crash(error_message)
char *error_message;
{
   if (!exp_error)
   {
      strcpy(err_string,error_message);
      exp_error=TRUE;
   }
}

int search(operator)
char *operator;
{
   int op_len=strlen(operator);
   int found_it=FALSE;
   if ((scan+op_len>exp_len) || exp_error) return(FALSE);
   if ((found_it=(strcmp(substr(expression,scan+1,op_len),operator)==0)))
      scan+=op_len;
   return(found_it);
}

char next()
{
   if (scan>=exp_len || exp_error) return('\0');
   return(expression[scan++]);
}

double get_number()
{
   char the_number[255],the_var[255],conv_str[63];
   int num_len=0;
   int OK=TRUE;
   int got_exp= -1;
   int got_hex=FALSE;
   int var_len=0;
   double answer;
   unsigned int answer_int;
   int status;
   char *envptr;
   if (search("PI")) return(M_PI);
   while (scan<exp_len && OK && !exp_error)
   {
      switch (expression[scan])
      {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
         if (var_len)
            the_var[var_len++]=next();
         else
            the_number[num_len++]=next();
         break;
      case '.':
         if (got_hex || var_len) OK=FALSE; else the_number[num_len++]=next();
         break;
      case '$':
      case '&':
         if (got_hex || num_len || var_len)
            OK=FALSE;
         else
         {
            scan++; got_hex=TRUE;
            the_number[0]='0'; the_number[1]='x';
            num_len+=2;
         }   
         break;
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'F':
         if (got_hex) the_number[num_len++]=next();
         else
            if (num_len) OK=FALSE; else the_var[var_len++]=next();
         break;
      case 'G':
      case 'H':
      case 'I':
      case 'J':
      case 'K':
      case 'L':
      case 'M':
      case 'N':
      case 'O':
      case 'P':
      case 'Q':
      case 'R':
      case 'S':
      case 'T':
      case 'U':
      case 'V':
      case 'W':
      case 'X':
      case 'Y':
      case 'Z':
      case '_':
         if (num_len) OK=FALSE; else the_var[var_len++]=next();
         break;
      case 'E':
         if (var_len) the_var[var_len++]=next();
         else
            if (num_len)
            {
               the_number[num_len++]=next();
               if (!got_hex) got_exp=scan;
            }
            else OK=FALSE;
         break;
      case '+':
      case '-':
         if (scan==got_exp) the_number[num_len++]=next(); else OK=FALSE;
      default:
         OK=FALSE;
      }
   }
   the_number[num_len]='\0'; the_var[var_len]='\0';
   if (got_hex)
   {
      strcpy(conv_str,"Bad hexadecimal constant");
      status=(sscanf(the_number,"%x",&answer_int)<0);
      answer=(double)answer_int;
#ifdef DEBUG
      printf("String = %s, Value = %f\n",the_number,answer);
#endif
   }
   else
   {
      status=FALSE;
      if (var_len)
      {
         sprintf(conv_str,"Bad symbol (%s)",the_var);
#ifdef DEBUG
         printf("Reading %d char var (%s)\n",strlen(the_var),the_var);
         printf("Printf claims A=%s\n",getenv(the_var));
#endif
         envptr=getenv(the_var);
         if (envptr==NULL)
            status=TRUE;
         else
         {
            strcpy(the_number,envptr);
#ifdef DEBUG
            printf("Translation of %s = %s\n",the_var,the_number);
#endif
         }
      }
      else strcpy(conv_str,"Bad decimal constant");
      if (!status) status=(sscanf(the_number,"%lf",&answer)<0);
   }
   if (status) crash(conv_str);
   return(answer);
}

void bracket()
{
   level_0();
   if (next()!=')') crash("Missing bracket");
}

double compute_sin(angle)
double angle;
{
   return(sin(M_PI*angle/180));
}

double compute_cos(angle)
double angle;
{
   return(cos(M_PI*angle/180));
}

void eval_operand()
{
   if (search("INT"))
   {
      eval_operand();
      if (operand>INT_MAX || operand<-INT_MAX)
         crash("Floating-point number too large to be truncated");
      else
         operand=(long)operand;
   }
   else
   if (search("SQRT"))
   {
      eval_operand();
      if (operand<0) crash("Negative square root"); else operand=sqrt(operand);
   }
   else
   if (search("SIN"))
   {
      eval_operand(); operand=compute_sin(operand);
   }
   else
   if (search("COS"))
   {
      eval_operand(); operand=compute_cos(operand);
   }
   else
   if (search("TAN"))
   {
      eval_operand();
      if (compute_cos(operand)==0) crash("Bad tangent");
         else operand=compute_sin(operand)/compute_cos(operand);
   }
   else
   if (search("ASN"))
   {
      eval_operand();
      if (abs((int)operand)>1) crash("Bad arcsine");
         else operand=asin(operand);
   }
   else
   if (search("ACS"))
   {
      eval_operand();
      if (abs((int)operand)>1) crash("Bad arccosine");
         else operand=acos(operand);
   }
   else
   if (search("EXP"))
   {
      eval_operand(); operand=exp(operand);
   }
   else
   if (search("LN"))
   {
      eval_operand();
      if (operand<=0) crash("Natural log error");
         else operand=log(operand);
   }
   else
   if (search("LOG"))
   {
      eval_operand();
      if (operand<=0) crash("Base 10 log error");
         else operand=log10(operand);
   }
   else
   if (search("ATN"))
   {
      eval_operand(); operand=atan(operand);
   }
   else
   if (search("RND"))
   {
      eval_operand();
      operand=randval((long)operand);
   }
   else
   if (search("TIME")) operand=time(0);
   else
   if (search("FACT"))
   {
      eval_operand();
      if (operand<0 || operand>=MAXFACTOR)
         crash("Out of bounds factorial value");
      else
         operand=facts[(long)operand];
   }
   else
   if (search("-"))
   {
      eval_operand(); operand= -operand;
   }
   else
   if (search("+")) eval_operand();
   else
   if (search("(")) bracket(); else operand=get_number();
}

void level_4()
{
   double temp_operand;
   eval_operand();
   while (search("^"))
   {
      if (operand<=0)
         crash("Cannot raise zero or less to the power of a value");
      else
      {
         temp_operand=operand;
         eval_operand();
         if (!exp_error) operand=pow(temp_operand,operand);
      }
   }
}

void level_3()
{
   double temp_operand;
   level_4();
   while (search("/"))
   {
      temp_operand=operand;
      level_4();
      if (operand==0) crash("Division by zero");
      else
      {
         if (search("%")) operand=operand/100;
         operand=temp_operand/operand;
      }
   }
}

void level_2()
{
   double temp_operand;
   level_3();
   while (search("*"))
   {
      temp_operand=operand;
      level_2();
      if (search("%")) operand=operand/100;
      operand=temp_operand*operand;
   }
}

void level_1()
{
   double temp_operand;
   level_2();
   while (search("-"))
   {
      temp_operand=operand;
      level_2();
      if (search("%")) operand=temp_operand*operand/100;
      operand=temp_operand-operand;
   }
}

void level_0()
{
    double temp_operand;
    level_1();
    if (search("+"))
    {
       temp_operand=operand;
       level_0();
       if (search("%")) operand=temp_operand*operand/100;
       operand+=temp_operand;
    }
}

void eval_statement()
{
   char signstr[10];
   int ans_len;
   scan=0; exp_error=FALSE; level_0();
   if (scan<=exp_len-1) crash("Missing operand");
   if (equals==0 && interact && !exp_error) printf("%s",expression);
   if (exp_error)
      fprintf(stderr,"%s contains an error : %s\n",expression,err_string);
   else
   {
      if (hex_format)
      {
         if (operand<0) strcpy(signstr,"-$"); else strcpy(signstr,"$");
         sprintf(answer_str,"%s%x",signstr,(unsigned int)operand);
      }
      else
      {
         sprintf(answer_str,"%.14lf",operand);
         ans_len=strlen(answer_str)-1;
         while (answer_str[ans_len]=='0')
         {
            ans_len--;
         }
         if (answer_str[ans_len]=='.') ans_len--;
         answer_str[ans_len+1]='\0';
      }
      if (equals==0 && !exp_error)
      {
         if (interact) printf(" = ");
         printf("%s\n",answer_str);
      }
   }
}

void show_help()
{
   printf("%s is a BASIC-style expression evaluator which understands the following\n",progname);
   printf("operators (all trigonometrical functions use degrees):\n");
   printf("+,-,*,/,^   Addition, subtraction, multiplication, division and power.\n");
   printf("()          Brackets (overrides operator precedence).\n");
   if (!interact)
   printf("            Be careful with * on the command line - use \\* to avoid expansion.\n");
   printf("%%           Percentage (works in combination with +,-,* and /).\n");
   printf("~           Print result as a hexadecimal number.\n");
   printf("$ or &      Hexadecimal constant follows.\n");
   if (interact)
   printf("!command    Issue a shell command.\n");
   printf("ACS expr    Arccosine.\n");
   printf("ASN expr    Arcsine.\n");
   printf("ATN expr    Arctangent.\n");
   printf("COS expr    Cosine.\n");
   printf("EXP expr    Exponential power.\n");
   printf("FACT expr   Factorial (expr must be between 0 and %d).\n",MAXFACTOR-1);
   printf("LN expr     Natural log (expr must be positive).\n");
   printf("LOG expr    Log to base 10 (expr must be positive).\n");
   printf("RND expr    Random number generator.\n");
   printf("            If expr<0, then expr is used to seed the random number generator.\n");
   printf("            If 0<=expr<=1, then returns a real number between 0 and 1.\n");
   printf("            If expr>1, then returns an integer between 1 and expr.\n");
   printf("SIN expr    Sine.\n");
   printf("SQRT expr   Square root.\n");
   printf("TAN expr    Tangent.\n");
   printf("TIME        Returns the number of seconds since 1st January 1970.\n");
}

void split_line()
{
   char lhs[255];
   int comma;
   int line_len=strlen(expr_line);
   while (line_len)
   {
      exp_len=instr(expr_line,',')-1;
      if (exp_len<0) exp_len=line_len;
      comma=exp_len;
      hex_format=(expr_line[0]=='~');
      if (hex_format)
      {
         exp_len-=1;
         strcpy(expression,substr(expr_line,2,exp_len));
      }
      else strcpy(expression,substr(expr_line,1,exp_len));
      equals=instr(expression,'=');
      if (equals)
      {
         if (equals==1)
         {
            printf("Warning ! Discarded result ! "); exp_len-=1;
            strcpy(expression,substr(expression,2,exp_len));
            equals=0;
         }
         else
         {
            exp_len-=equals;
            strcpy(lhs,substr(expression,1,equals-1));
            strcpy(expression,substr(expression,equals+1,exp_len));
         }
      }
      if (interact &&
         (strcmp(expression,"QUIT")==0 || strcmp(expression,"EXIT")==0))
      {
         goteof=TRUE; line_len=0;
      }
      else
      if (interact && strcmp(expression,"HELP")==0)
      {
         line_len=0; show_help();
      }
      else
      {
         eval_statement();
         if (equals)
         {
#ifdef VMS
/* Put something in here to assign to a DCL symbol */
#else
/* This next b*****d bit took me AGES to work out ! If you do multiple putenv
   stuff, you have to MALLOC the strings, otherwise they get overwritten. I
   think the putenv man page should make this very clear indeed (instead of
   some vague waffle about "automatic variables") */
            envstr=malloc(255);
            sprintf(envstr,"%s=%s",lhs,answer_str);
            putenv(envstr);
#ifdef DEBUG
            printf("Env str = %s, New value of %s = %s\n",envstr,lhs,getenv(lhs));
#endif
#endif
         }
         strcpy(expr_line,substr(expr_line,comma+2,line_len-comma-1));
         line_len=strlen(expr_line);
      }
   }
}

void init()
{
   int i;
   facts[0]=1;
   for (i=1;i<MAXFACTOR;i++)
   {
      facts[i]=facts[i-1]*i;
   }
   randomize(time(0)); /* Equivalent to RND(-TIME) */
   needCR=FALSE;
}

void tidy_up()
{
   signal(SIGINT, SIG_IGN);
   signal(SIGQUIT, SIG_IGN);
}

void aborted()
{
   printf("\n");
   tidy_up();
   exit(1);
}

void 
interactive(
	void
)
{
	char	prompt[ BUFSIZ + 1 ];

	snprintf( prompt, sizeof( prompt ), "%s> ", progname );
	signal(SIGINT, aborted);
	signal(SIGQUIT, aborted);
	goteof=FALSE;
#if	0
	printf("%s V%s (C) Richard K. Lloyd 1992-2002. Type HELP for further info.\n",progname,VERSION);
	printf("Now in interactive mode. Type EXIT or QUIT to exit.\n");
#endif	/* NOPE */

	while (!goteof) {
#ifdef DEBUG
		printf("A=%s,",getenv("A"));
#endif
		if( expr_line )	{
			free( expr_line );
			expr_line = NULL;
		}
		expr_line = readline( prompt );
		goteof = (expr_line == NULL);
		if (goteof)	{
			printf("\n");
		} else if (expr_line[0]=='!') {
			system(substr(expr_line,2,strlen(expr_line)-1));
		} else {
			add_history( expr_line );
			strip_expression();
			split_line();
		}
	}
	tidy_up();
}

void 
batch_mode(
	void
)
{
	while( (expr_line = readline( NULL )) != NULL )	{
		strip_expression();
		split_line();
	}
}

void 
command_line(
	int		argc,
	char * *	argv
)
{
	char		cmd[ BUFSIZ + 1 ];

	cmd[0] = '\0';
	while( optind < argc )	{
		char * const	optarg = argv[ optind++ ];

		strcat( cmd, optarg );
	}
	interact=FALSE;
	expr_line = cmd;
	strip_expression();
	split_line();
}

void usage()
{
   fprintf(stderr,"Syntax: %s [options] [expression[,expression...]\n",progname);
   fprintf(stderr,"All spaces are stripped from the expression prior to evaluation.\n");
   fprintf(stderr,"If no expression is supplied, then an interactive mode is entered.\n");
   fprintf(stderr,"Options currently supported with %s %s are as follows:\n",progname,VERSION);
   fprintf(stderr,"-? : Displays this syntax message.\n");
   fprintf(stderr,"-h : Displays the interactive help.\n");
   fprintf(stderr,"-s : Reads from standard input without entering interactive mode.\n");
   fprintf(stderr,"-v : Displays version information.\n");
   exit(1);
}

void get_options(argc,argv)
int argc;
char **argv;
{
   int param;
   while ((param=getopt(argc,argv,"hsv?")) != EOF)
   {
       switch (param)
       {
          case 'h':
             show_help();
             exit(0);
             break;
          case 's':
             interact=FALSE;
             break;
          case 'v':
             printf("%s version %s\n",progname,VERSION);
             exit(0);
             break;
          case '?':
             usage(); /* Bad option */
             break;
       }
   }
}

int main(argc,argv)
int argc;
char **argv;
{
	init();
	basename(progname,argv[0]);
	get_options(argc,argv);
	if (optind==argc) {
		if (interact)	{
			interact = isatty( fileno( stdin ) );
			interactive();
		} else	{
			batch_mode();
		}
	} else	{
		command_line(argc,argv);
	}
	return(0);
}
