static long long read_int(const char *str)
{
	long long val=0;
	for(;;)
	{
		unsigned char c=*str-'0';
		if(c<10)
		{
			val*=10;
			val+=c;
		}
		else
			break;
	}
	return val;
}
int main(int argc, char **argv)
{
	int sum=0;
	for(int k=0;k<argc;++k)
		sum+=read_int(argv[k]);
	return sum;
}
