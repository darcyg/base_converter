/*************************************************************************************************************
$Author: Dbabitsky $
$Revision: 13 $
$Date: 10/28/03 4:00p $
**************************************************************************************************************/
#include "stdafx.h"

#if defined(USE_BOOST_PROGRESS)
#include <boost/progress.hpp>
#endif

std::string& widen(std::string& str, unsigned int len, char fillchar){
    if (str.length()<len){
        std::ostringstream oss; 
        oss<<std::setw(len)<<std::setfill(fillchar)<<str;
        str=oss.str();
    }
    return str;
}

char* increment_base36(char* pStr){
    for(size_t i=strlen(pStr)-1; i>=0;i--){
        char c=pStr[i];
        c=(c=='9')?'A':(c=='Z')?'0':c+1;
        pStr[i]=c;
        if(c!='0') break;
    }
    return pStr;
}   

std::string& increment_base36(std::string& str){
    for(size_t i=str.length()-1; i>=0;i--){
        char c=str[i];
        c=(c=='9')?'A':(c=='Z')?'0':c+1;
        str[i]=c;
        if(c!='0') break;
    }

    return str;
}

namespace dmitry{//strupr fn does not exist in sun workshop
    char * strupr (char * string){
        for (char* cp=string; cp && *cp; ++cp){
            if ('a' <= *cp && *cp <= 'z')
                *cp += 'A' - 'a';
        }
        return(string);
    }
}

char* reverse(char* s){
    if (!s)return s;
    
    for(size_t i=0, j=strlen(s)-1; i<j; i++,j--)    {
        s[i]^=s[j]; //triple XOR Trick for not using a temp
        s[j]^=s[i]; 
        s[i]^=s[j];
    }
    return s;
}

void reverse2(char s[]) {
    size_t  i, j;
    for (i=0, j=strlen(s)-1; i<j;i++, j--)    {
        char c = s[i];
        s[i] = s [j];
        s[j] = c;
    }
}


/**Performance numbers for 0xFFFFFF iterations(PII 450 MHz, VS7,full optimize)
        
        base_converter 94
        ****************KIb1=16777215
        12 s

        base_converter 36
        ***************9ZLDR=16777215
        17 s

        base_converter 2
        111111111111111111111111=16777215
        56 s
*/



static const char BASE_CHARSET[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~abcdefghijklmnopqrstuvwxyz";
static int INDEX['~'+1]={0};//~ is the last char of those in our BASE_CHARSET in the ASCII table.


namespace version3{

//our charset is not in the same order as ASCII table, that's why we need to index it by the value of every char
static int init(){
    for(int i=0;i<sizeof(BASE_CHARSET)-1;i++){ 
        assert('!'<=BASE_CHARSET[i] && BASE_CHARSET[i]<sizeof(INDEX) && "BASE_CHARSET contains symbol outside of printable range(see ASCII table)");
        INDEX[BASE_CHARSET[i]]=i;
    }
    return 1;
}

//convert null-terminated string to a number using base
//~15% faster than version2, which is ~100% faster than version1
int  a2i(char *pStr, int base){
    static int init_once=init();

    if(base<=INDEX['Z']) dmitry::strupr(pStr);  //if base is <36 assume customer wants upper-case only. _strupr fn does not exist in sun workshop

    int n=0;
    for(int i=0; pStr[i]; i++){
        char c=pStr[i];
        assert('!' <=c && c<='~' && "string contains symbol not in base-94 charset");
        n = n * base + INDEX[c];
	}
	return n;
}
//convert number to a string
char*  i2a(char *pStr, int cch, int number, int base){
    if(!pStr || cch<=0) return pStr;

    int i = 0;
    do{
        int remainder = number % base;
		pStr[i++] = BASE_CHARSET[remainder];
        number /= base;
    }while(number && i<cch);

    return  reverse(pStr);//strrev does not exist in Sun compiler
}
}//version 3

namespace version2{
    static const char BASE_CHARSET[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~abcdefghijklmnopqrstuvwxyz";
    static const int OFFSET[6]={                  10,                       36,             51,    58,    64, 68};

    //(from 1.5 to 2x faster depending on base-the higher base the faster)
    int  a2i(char *pStr, int cch, int base){
        if(base<=36) dmitry::strupr(pStr);  //if base is <36 assume customer wants upper-case only. _strupr fn does not exist in sun workshop

        int n=0;
        for(int i = 0; i< cch && pStr[i]; i++){
            char c=pStr[i];
            int index=('0'<=c && c<='9')? c-'0':
                      ('A'<=c && c<='Z')? c-'A'+OFFSET[0]:
                      ('!'<=c && c<='/')? c-'!'+OFFSET[1]:
                      (':'<=c && c<='@')? c-':'+OFFSET[2]:
                      ('['<=c && c<='`')? c-'['+OFFSET[3]:
                      ('{'<=c && c<='~')? c-'{'+OFFSET[4]:
                      ('a'<=c && c<='z')? c-'a'+OFFSET[5]:-1;
            assert(index>=0 && "string contains symbol not in a base-94 charset");
            n = n * base + index;
	    }
	    return n;
    }
    //this avoids reversing the string(takes 1/2 of the time of i2a), but it's not faster anyway
    char*  i2a(char *pStr, int cch, int number, int base){
        if(!pStr || cch<=0) return pStr;

        char szReverse[100]={0};
        int i = sizeof(szReverse)-2,end=i;//-2, not -1, essential for debug builds
        do{
            int remainder = number % base;
		    number /= base;
		    szReverse[i--] = BASE_CHARSET[remainder];
        }while(number && i>0);

        return (char*) memcpy(pStr,&szReverse[++i],end-i+1);
    }
}

namespace version1{
    int  a2i(char *pStr, int cch, int base){
        if(base<=36) dmitry::strupr(pStr);  //if base is <36 assume customer wants upper-case only. _strupr fn does not exist in sun workshop

        int n=0;
        for(int i = 0; i< cch && pStr[i]; i++){
		    const char* pCh = strchr(BASE_CHARSET, pStr[i]);//find the char in the array
            assert(pCh && "string contains symbol not in a base-94 charset");

			int index = int(pCh - BASE_CHARSET);
            n = n * base + index;
	    }
	    return n;
    }
    char*  i2a(char *pStr, int cch, int number, int base){
        if(!pStr) return pStr;
        if(0==number) {*pStr='0';return pStr;}

        int index = 0;
        while(number && index<cch)	{
            int remainder = number % base;
		    number /= base;
		    pStr[index++] = BASE_CHARSET[remainder];
	    }

        return  reverse(pStr);//strrev does not exist in Sun compiler
    }
}
namespace version=version3;

int main(int argc, char* argv[]){
    using namespace std;
//    boost::progress_timer t; 

    int base(0);
    if(argc<2 || (base=atoi(argv[1]))< 2 || base >sizeof(BASE_CHARSET)-1) 
        return printf("usage: %s base(must be between 2-%d)\n",argv[0],sizeof(BASE_CHARSET)-1);

    bool dbg=(argc>2 && *argv[2]=='d');

    char s[100]={'0'};
    //int i=sizeof(Data),j=_tcslen(Data); //6-5

    const int MAX_ITERS=0xFFFFFF;
#if defined(USE_BOOST_PROGRESS)
    boost::progress_display progress(MAX_ITERS);
#endif    
    //char z[]="00000";
    std::string z="";
    widen(z,5,'0');

    for (unsigned int i=0;i<MAX_ITERS;i++){
        int n=version::a2i(s, base);                 //convert to number//_tcslen(s)        
#if defined(USE_BOOST_PROGRESS)
        if(dbg) 
            ++progress;
        else    
#endif
            cout<<setiosflags(ios::right)<<setw(20)<<setfill(' ')<<s<<"="<<n<<" incr(z)="<<z<<endl;

        n++;                                            
        increment_base36(z);
        memset(s,0,sizeof(s));
        if(0==version::i2a(s,sizeof(s),n, base))break;           //convert back to string
    }

    cout<<setiosflags(ios::right)<<setw(20)<<setfill('*')<<s<<"="<<MAX_ITERS<<" z="<<z<<endl<<"\a";
	return 0;
}


/**
    char foo[]="ABC", bar[3]="ABC";
    sizeof(foo)=4,sizeof(bar)=3;
    strlen(foo)=strlen(bar)=3
    foo[2]=bar[2]='C'
*/

/*
    #if 0
    for(char* p=pStr;number!=0 && p-pStr<cch;p++){
        int remainder = number % base;
		number /= base;
		*p = BASE_CHARSET[remainder];
    }
    #endif

    void init2(){
        for (char c='!';c<sizeof(INDEX);c++){
            char* pCh = strchr(BASE_CHARSET, c);
            assert(pCh && "our charset does not contain this symbol");
            INDEX[c]=int(pCh - BASE_CHARSET);
        }
    }
            


#include <math.h>
int  a2i2(TCHAR *pStr, int cch, int base){
    dmitry::_strupr(pStr);                    //convert it to upper case. fn does not exist in sun workshop

    long n=0,d;
    for(long i = 0; i< cch; i++)	{
        d=pStr[i]-55;
        n+= d * pow(base,i);
    }

    return n;
}
*/


