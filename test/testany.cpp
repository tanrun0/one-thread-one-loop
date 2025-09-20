#include <iostream>
#include <any>


// any 是可以存储任意类型的一个类
// 本质是: 通过在类里面设置一个父类指针，然后子类继承父类，在子类里面存不同类型的数据
// 从而实现，类中无序关心数据类型，只需要保存一个父类指针就可以访问不同类型


// 常用成员函数
// has_value(): 检查是否持有值
// type(): 返回所存储值的类型信息
// reset(): 销毁存储的值，使其变为空状态
// emplace<T>(): 直接在 std::any 中构造一个 T 类型的对象
// 非成员函数
// std::any_cast<T>(): 提取存储的值，如果类型不匹配会抛出 bad_any_cast 异常
int main()
{

    std::any a;
    a = 10;
    // type() 成员函数返回一个 std::type_info 对象的引用
    std::cout << a.type().name() << std:: endl; 
    // 必须显式实例化，且类型要一致 (注意要根据返回对象来传递参数)
    int * pa = std::any_cast<int>(&a); // 返回 any 中存储的数据的指针的
    std::cout << *pa << std:: endl; 
    int aa = std::any_cast<int>(a); // 返回any中存储的数据
    std::cout << aa << std:: endl; 

    return 0;
    
}