#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<list>
#include<iostream>
#include<time.h>


using namespace std;

struct temp
{
    int x;
    char z;
    int y;
    char temp[0];
    // char a;
};

// class helper
// {
//     int a;
//     public:
//     helper(int y) (a:y) 
//     {

//     }
// };

class pub1 
{
    public:
    
    int help = 1;
};

class pub2 : public pub1
{
    public:
    pub2()
    {
        pub1 x = pub1();
    }
    int h = 2;
};

int main(int argc , char *argv[])
{
    // int *temp = new int(1);
    // char *temp1 = (char*)temp;
    // for(int i = 0;i < 4;i++)
    //     printf("%d\n", *(temp1 + i));
    
    // char arr_string[][20] = {"hello", "ow", "are", "you"};
    // arr_string[0][0] = 'a';
    // printf("%c\n", arr_string[0][0]);

    // char x[] = {1, 0, 0, 0};
    // int* hel = (int*)x;
    // printf("%d\n", *hel);
    // char helper[] = "helper";
    // printf("%d\n", sizeof(temp));
    // char helper[1] = "";
    // strcpy(helper, "ma");
    // malloc(4);

    // int * temp = (int*)malloc(sizeof(int));

    // fork();
    // (*temp)++;
    // printf("%d\n", temp);
    // return 0;
    // helper x = helper(5);
    // char* helper[] = {"helper"};
    // printf("%c", **argv++);
    // printf("%c", **argv);
    // char c[100], d[100];
    // scanf("%s %s\n", c, d);
    // strcat(c, d);
    // printf("%s", c);
    // void* t = nullptr;

    // struct temp* ptr = (struct temp*)malloc(sizeof(temp) + 10);
    // strcpy(ptr->temp, "hel");
    // printf("%s\n", ptr->temp);
    // printf("%d\n", sizeof(*ptr));
    // int *hhh = (int*)malloc(4);
    
    // pub1* temp = new pub2();
    // printf("%d\n", ((pub2*)temp)->h);

    time_t ti = time(NULL);
    cout << ti << "\n";
}