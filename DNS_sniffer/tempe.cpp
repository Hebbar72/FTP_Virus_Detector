#include<iostream>
#include<list>
#include<unordered_map>
#include<vector>

using namespace std;

class LRU
{
    private:
        list<int> cache;
        int size;
        unordered_map<int, list<int>::iterator> map;
    
    public:
        LRU(int capacity)
        {
            size = capacity;
        }

        bool find(int val)
        {
            return map.find(val) != map.end();
        }

        void insert(int val)
        {
            if(map.find(val) == map.end())
            {
                if(map.size() >= size)
                {
                    map.erase(cache.back());
                    cache.pop_back();
                }

                cache.push_front(val);
                map[val] = cache.begin();
            }
            else
            {
                cache.erase(map[val]);
                cache.push_front(val);
                map[val] = cache.begin();
            }
        }
};

static class Heap
{
    private:
    int size;
    int capacity;
    vector<pair<int, string>> arr;

    public:
    Heap(int cap)
    {
        capacity = cap + 1;
        size = 0;
        arr.resize(capacity);
    }

    void push(pair<int, string> &x)
    {
        if(size == capacity)
        {
            perror("Cannot insert more to the heap");
            return;
        }
        arr[size] = x;
        int parent = (size++ - 1)/2;
        int loc = size - 1;
        while(parent >= 0)
        {
            if(arr[loc].first < arr[parent].first)
            {
                swap(arr[loc], arr[parent]);
                loc = parent;
                parent = (parent - 1)/2;
            }
            else    
                break;
        } 
    }

    void pop()
    {
        swap(arr[0], arr[size - 1]);
        int loc = 0;
        --size;
        while(loc < size/2)
        {
            int smallest = loc;
            int right = 2*loc + 2;
            int left = 2*loc + 1;

            if(left < size && arr[left].first < arr[smallest].first)
                smallest = left;
            if(right < size && arr[right].first < arr[smallest].first)
                smallest = right;
            
            if(smallest != loc)
            {
                swap(arr[loc], arr[smallest]);
                loc = smallest;
            }
            else
                break;
        }
    }

    void update(int loc)
    {
        while(loc < size/2)
        {
            int smallest = loc;
            int right = 2*loc + 2;
            int left = 2*loc + 1;
            arr[loc].first++;
            if(left < size && arr[left].first < arr[smallest].first)
                smallest = left;
            if(right < size && arr[right].first < arr[smallest].first)
                smallest = right;
            
            if(smallest != loc)
            {
                swap(arr[loc], arr[smallest]);
                loc = smallest;
            }
            else
                break;
        }
    }

    pair<int, string> top()
    {
        if(size)
            return arr[0];
        
        perror("The stack is empty");
        return {};
    }



};

int main()
{
    Heap heap(4);
    heap.push(5);
    heap.push(6);
    heap.push(4);
    heap.push(9);
    heap.push(2);
    cout << heap.top() << "\n";
    heap.pop();
    cout << heap.top() << "\n";
    heap.pop();while(loc < size/2)
        {
            int smallest = loc;
            int right = 2*loc + 2;
            int left = 2*loc + 1;

            if(left < size && arr[left].first < arr[smallest].first)
                smallest = left;
            if(right < size && arr[right].first < arr[smallest].first)
                smallest = right;
            
            if(smallest != loc)
            {
                swap(arr[loc], arr[smallest]);
                loc = smallest;
            }
            else
                break;
        }
    cout << heap.top() << "\n";
    heap.pop();
    cout << heap.top() << "\n";
    heap.pop();
    cout << heap.top() << "\n";
    heap.pop();

}