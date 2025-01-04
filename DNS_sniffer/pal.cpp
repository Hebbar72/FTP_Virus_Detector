#include<iostream>
#include<queue>
#include<algorithm>
#include<vector>

using namespace std;

double median(int val)
{
    static priority_queue<int> lower;
    static priority_queue<int, vector<int>, greater<int>> higher;

    lower.push(val);
    if(lower.size() - higher.size() == 2)
    {
        int temp = lower.top();
        lower.pop();
        higher.push(temp);
    }

    if(lower.size() == higher.size())
        return (double(lower.top()) + higher.top())/2.0;
    else    
        return lower.top();
}

int main(int argn, char* args[])
{
    int size = atoi(args[1]);
    vector<int> input(size);

    for(int i = 0;i < size;i++)
        cin >> input[i];
    
    vector<double> ans(size);

    cout << "**********************\n";
    for(int i = 0;i < size;i++)
    {
        ans[i] = median(input[i]);
        cout << ans[i] << "\n";
    }

    cout << "completed\n";

}