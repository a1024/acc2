#include			"include/math.h"
#include			"include/stdlib.h"
int					maximum(int a, int b){return a>b?a:b;}
int					minimum(int a, int b){return a<b?a:b;}
int					clamp(int lo, int x, int hi)
{
	if(x<lo)
		x=lo;
	if(x>hi)
		x=hi;
	return x;
}
int					mod(int x, int n)
{
	x%=n;
	x+=n&-(x<0);
	return x;
}
int					first_set_bit(unsigned long long n)//idx of LSB
{
	int sh=((n&((1ULL<<32)-1))==0)<<5,	idx =sh;	n>>=sh;
		sh=((n&((1   <<16)-1))==0)<<4,	idx+=sh;	n>>=sh;
		sh=((n&((1   << 8)-1))==0)<<3,	idx+=sh;	n>>=sh;
		sh=((n&((1   << 4)-1))==0)<<2,	idx+=sh;	n>>=sh;
		sh=((n&((1   << 2)-1))==0)<<1,	idx+=sh;	n>>=sh;
		sh= (n&((1   << 1)-1))==0,		idx+=sh;
	return idx;
}
int					first_set_bit16(unsigned short n)//idx of LSB
{
	int sh=((n&((1<<8)-1))==0)<<3,	idx =sh;	n>>=sh;
		sh=((n&((1<<4)-1))==0)<<2,	idx+=sh;	n>>=sh;
		sh=((n&((1<<2)-1))==0)<<1,	idx+=sh;	n>>=sh;
		sh= (n&((1<<1)-1))==0,		idx+=sh;
	return idx;
}
int					floor_log2(unsigned long long n)//idx of MSB
{
	int sh=(n>=1ULL	<<32)<<5,	logn =sh; n>>=sh;
		sh=(n>=1	<<16)<<4;	logn+=sh, n>>=sh;
		sh=(n>=1	<< 8)<<3;	logn+=sh, n>>=sh;
		sh=(n>=1	<< 4)<<2;	logn+=sh, n>>=sh;
		sh=(n>=1	<< 2)<<1;	logn+=sh, n>>=sh;
		sh= n>=1	<< 1;		logn+=sh;
	return logn;
}
int					floor_log10(double x)
{
	static const double pmask[]=//positive powers
	{
		1, 10,		//10^2^0
		1, 100,		//10^2^1
		1, 1e4,		//10^2^2
		1, 1e8,		//10^2^3
		1, 1e16,	//10^2^4
		1, 1e32,	//10^2^5
		1, 1e64,	//10^2^6
		1, 1e128,	//10^2^7
		1, 1e256	//10^2^8
	};
	static const double nmask[]=//negative powers
	{
		1, 0.1,		//1/10^2^0
		1, 0.01,	//1/10^2^1
		1, 1e-4,	//1/10^2^2
		1, 1e-8,	//1/10^2^3
		1, 1e-16,	//1/10^2^4
		1, 1e-32,	//1/10^2^5
		1, 1e-64,	//1/10^2^6
		1, 1e-128,	//1/10^2^7
		1, 1e-256	//1/10^2^8
	};
	int logn, sh;
	if(x<=0)
		return 0x80000000;
	if(x>=1)
	{
		logn=0;
		sh=(x>=pmask[17])<<8;	logn+=sh, x*=nmask[16+(sh!=0)];
		sh=(x>=pmask[15])<<7;	logn+=sh, x*=nmask[14+(sh!=0)];
		sh=(x>=pmask[13])<<6;	logn+=sh, x*=nmask[12+(sh!=0)];
		sh=(x>=pmask[11])<<5;	logn+=sh, x*=nmask[10+(sh!=0)];
		sh=(x>=pmask[9])<<4;	logn+=sh, x*=nmask[8+(sh!=0)];
		sh=(x>=pmask[7])<<3;	logn+=sh, x*=nmask[6+(sh!=0)];
		sh=(x>=pmask[5])<<2;	logn+=sh, x*=nmask[4+(sh!=0)];
		sh=(x>=pmask[3])<<1;	logn+=sh, x*=nmask[2+(sh!=0)];
		sh= x>=pmask[1];		logn+=sh;
		return logn;
	}
	logn=-1;
	sh=(x<nmask[17])<<8;	logn-=sh;	x*=pmask[16+(sh!=0)];
	sh=(x<nmask[15])<<7;	logn-=sh;	x*=pmask[14+(sh!=0)];
	sh=(x<nmask[13])<<6;	logn-=sh;	x*=pmask[12+(sh!=0)];
	sh=(x<nmask[11])<<5;	logn-=sh;	x*=pmask[10+(sh!=0)];
	sh=(x<nmask[9])<<4;		logn-=sh;	x*=pmask[8+(sh!=0)];
	sh=(x<nmask[7])<<3;		logn-=sh;	x*=pmask[6+(sh!=0)];
	sh=(x<nmask[5])<<2;		logn-=sh;	x*=pmask[4+(sh!=0)];
	sh=(x<nmask[3])<<1;		logn-=sh;	x*=pmask[2+(sh!=0)];
	sh= x<nmask[1];			logn-=sh;
	return logn;
}
double				power(double x, int y)
{
	double mask[]={1, 0}, product=1;
	if(y<0)
		mask[1]=1/x, y=-y;
	else
		mask[1]=x;
	for(;;)
	{
		product*=mask[y&1], y>>=1;	//67.7
		if(!y)
			return product;
		mask[1]*=mask[1];
	}
	return product;
}
double				_10pow(int n)
{
	static double *mask=0;
	int k;
//	const double _ln10=log(10.);
	if(!mask)
	{
		mask=(double*)malloc(616*sizeof(double));
		for(k=-308;k<308;++k)		//23.0
			mask[k+308]=power(10., k);
		//	mask[k+308]=exp(k*_ln10);//inaccurate
	}
	if(n<-308)
		return 0;
	if(n>307)
		return _HUGE;
	return mask[n+308];
}