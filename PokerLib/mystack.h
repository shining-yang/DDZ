//
// mystack.h
//
// 简单的栈类模板，类成员使用指针而非静态数组。
//

template <class T>
class MyStack
{
private:
    T*      container;
    int     top;
    int     size;

public:
    MyStack(void);
    MyStack(int sz);
    MyStack(const MyStack<T>& s);
    ~MyStack(void);
    
    MyStack<T>& operator=(const MyStack<T>& s);

    void Push(const T& elem);
    T Pop(void);

    T Top(void) const;
    void ClearStack(void);

    bool IsEmpty(void);
    bool IsFull(void);
};


const int MYSTACK_DEF_SZ = 64;


template <class T>
MyStack<T>::MyStack(void) : top(-1), size(MYSTACK_DEF_SZ)
{
    container = new T[MYSTACK_DEF_SZ];
    if (container == NULL) {
        exit(0);
    }
}

template <class T>
MyStack<T>::MyStack(int sz) : top(-1), size(sz)
{
    container = new T[size];
    if (container == NULL) {
        exit(0);
    }
}

template <class T>
MyStack<T>::MyStack(const MyStack<T>& s)
{
    top = s.top;
    size = s.size;
    container = new T[size];
    if (container == NULL) {
        exit(0);
    }
    memcpy(container, s.container, size);
}

template <class T>
MyStack<T>::~MyStack(void)
{
    delete[] container;
}

template <class T>
MyStack<T>& MyStack<T>::operator=(const MyStack<T>& s)
{
    if (&s != this) {
        if (size < s.size) {
            delete[] container;
        }

        size = s.size;
        top = s.top;
        container = new T[size];
        if (container == NULL) {
            exit(0);
        }
        memcpy(container, s.container, size);
    }

    return *this;
}

template <class T>
void MyStack<T>::ClearStack(void)
{
    top = -1;
}

template <class T>
bool MyStack<T>::IsEmpty(void)
{
    return top == -1 ? true : false;
}

template <class T>
bool MyStack<T>::IsFull(void)
{
    return (top == size - 1) ? true : false;
}

template <class T>
void MyStack<T>::Push(const T& elem)
{
    if (top == size - 1) {
        // printf("Stack is OVERFLOW.\n");
        exit(0);
    }

    container[++top] = elem;
}

template <class T>
T MyStack<T>::Pop(void)
{
    T elem;
    if (top == -1) {
        // printf("Stack is EMPTY.\n");
        exit(0);
    }

    elem = container[top];
    top--;

    return elem;
}

template <class T>
T MyStack<T>::Top(void) const
{
    if (top == -1) {
        // printf("Stack is EMPTY.\n");
        exit(0);
    }

    return container[top];
}