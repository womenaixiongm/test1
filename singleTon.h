//
// Created by c00467120 on 2021/4/29.
//

#ifndef UNTITLED_SINGLETON_H
#define UNTITLED_SINGLETON_H


template<typename T>
class SingleTon{
public:
    static T* GetSingleTon()
    {
        static T value;
        return & value;
    }
private:
    SingleTon();
    SingleTon(const T& value);
};
#endif //UNTITLED_SINGLETON_H

