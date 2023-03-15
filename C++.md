# 语言基础
C和C++的区别：

* C是面向过程的语言（尽管可以自己实现虚函数表来支持多态），C++是面向对象的语言
* C++在C的基础上添加了更多特性，比如继承、封装、重载、模板、异常、RAII等
* C差不多是C++的子集，但是从C99开始就不完全兼容了


## 面向对象
面向对象是一种程序设计思想，把数据及对数据的操作方法放在一起，作为一个相互依存的整体——对象。对同类对象抽象出其共性，形成类。类中的大多数数据，只能用本类的方法进行处理。类通过一个简单的外部接口与外界发生关系，对象与对象之间通过消息进行通信

特点：

* 封装：把不需要让外界知道的实现细节隐藏起来。能够减少耦合，使类内部的实现可以自由修改
* 继承：派生类可以复用基类的属性和方法，并进行扩展。能复用基类的代码，减少冗余，但是会使派生类和基类更加耦合
* 多态：同一个接口可以具有不同的实现。可以通过基类的指针调用派生类的方法，不需要知道具体是什么类

C++的多态是通过虚表实现的。每个类有自己的虚表，里面存储了虚函数的地址。每个对象实例会存储虚表指针，调用虚函数时要去虚表里找函数地址。有虚函数的类的对象才是多态对象，才可以使用`typeid`、`dynamic_cast`这样的RTTI特性。而非多态对象，值的解释方式由使用对象的表达式所确定，这在编译期就已经决定了

```c++
// 非多态对象
struct B {
    void f() {
        cout << "调用了B" << endl;
    }
};

struct D : B {
    void f() {
        cout << "调用了D" << endl;
    }
};

D d;
D* pd = &d;
B* pb = &d;
pd->f(); // 调用了D
pb->f(); // 调用了B
cout << typeid(*pd).name() << endl; // 1D
cout << typeid(*pb).name() << endl; // 1B


// 多态对象
struct B {
    virtual void f() {
        cout << "调用了B" << endl;
    }
};

struct D : B {
    virtual void f() {
        cout << "调用了D" << endl;
    }
};

D d;
D* pd = &d;
B* pb = &d;
pd->f(); // 调用了D
pb->f(); // 调用了D
cout << typeid(*pd).name() << endl; // 1D
cout << typeid(*pb).name() << endl; // 1D
```

在构造函数、析构函数中调用虚函数不会调用到派生类的实现，因为此时派生类还未初始化或已销毁

如果要使用继承，基类的析构函数必须是虚函数，否则通过指向基类的指针删除对象会引发未定义行为。一般的表现是，调用了基类的析构函数，但不会调用派生类的析构函数

```c++
struct B {
    ~B() {
        cout << "B析构" << endl;
    }

    virtual void f() {}
};

struct D : B {
    ~D() {
        cout << "D析构" << endl;
    }
};

B* pb = new D();
delete pb; // 只有B析构，没有D析构
```

基类的可访问性：

* `public`继承：基类的所有`public`成员作为派生类的`public`成员，基类的所有`protected`成员作为派生类的`protected`成员，且外部可以把派生类指针转成基类指针
* `protected`继承：基类的所有`public`、`protected`成员作为派生类的`protected`成员，且外部不能把派生类指针转成基类指针
* `private`继承：基类的所有`public`、`protected`成员作为派生类的`private`成员，且外部不能把派生类指针转成基类指针

`struct`和`class`的区别：`struct`成员、基类的可访问性默认是`public`的，而`class`默认是`private`的。其实留着`struct`就是为了兼容C语言


## 内存
这节内容可能有实现定义的，以下结论来自GCC 9.4.0 x86_64 Linux平台，C++20标准

### new和malloc
区别和联系：

* `new`可以指[new表达式](https://zh.cppreference.com/w/cpp/language/new)或[new操作符](https://zh.cppreference.com/w/cpp/memory/new/operator_new)，而`malloc`是一个函数
* 最重要的区别是，`new`表达式会初始化（调用构造函数），而`malloc`不会初始化分配的内存
* `new`表达式会调用`new`操作符分配空间然后再初始化。`new`操作符底层会调用`malloc`分配空间（标准没规定，但目前所有实现都是这么做的）
* `new`操作符可以被重载替换，可以对特定的类使用不同的实现，而`malloc`不能
* `new`表达式的结果是指定类型的指针，而`malloc`的结果是`void*`

Linux下malloc的实现：

* 小于128kB的内存分配使用`brk`，它会增加进程的堆区大小，等到进程访问时触发缺页异常，然后才分配物理内存
* 大于或等于128kB的内存分配使用`mmap`，它会在堆和栈之间找一块空闲内存分配

### 对齐
每个对象类型都具有对齐要求，它是一个2的整数幂，表示这个类型的不同实例所能放置的连续相邻地址之间的字节数。一般地，基础类型的对齐要求就是它的大小。比如`uint32_t`对齐要求为4，则它的变量地址必须能被4整除

为什么要有对齐要求：因为有的CPU读取不能被对齐要求整除的地址时，可能分成多次读取，速度更慢；另外有的CPU直接禁止读取未对齐的地址

以下讨论不考虑`#pragma pack`，因为这是实现定义的（万恶的MSVC引入的），参考[GCC文档](https://gcc.gnu.org/onlinedocs/gcc/Structure-Layout-Pragmas.html)。只需知道用它可以设置出更小的（更宽松的）对齐要求

可以用`alignof`查询对齐要求，用`alignas`设置更大的（更严格的）对齐要求，但不能设置更小的

类的对齐要求是它的非静态成员（包括虚表指针）的对齐要求中的最大值。因为只要最大的对齐要求满足了，较小的对齐要求也能满足。为了使所有非静态成员都满足对齐要求，会在一些成员后面插入填充字节，使得所有成员的偏移量能被对齐要求整除

```c++
struct S1 {
    uint8_t a; // 偏移量0，大小1
    // 为了使b的偏移量能被8整除，这里要填充7
    uint64_t b; // 偏移量8，大小8
    uint32_t c; // 偏移量16，大小4
    // S的对齐要求是成员对齐要求的最大值，即8，这里要填充4
};
static_assert(alignof(S1) == 8);
static_assert(sizeof(S1) == 24);

struct S2 {
    virtual void f();

    // 虚表指针，偏移量0，大小8
    uint32_t a; // 偏移量8，大小4
    // S的对齐要求是8，这里要填充4
};
static_assert(alignof(S2) == 8);
static_assert(sizeof(S2) == 16);

struct S3 {
    struct alignas(16) InnerS {} a; // 加了alignas，偏移量0，大小16
    uint32_t b; // 偏移量16，大小4
    // S的对齐要求是16，这里要填充12
};
static_assert(alignof(S3) == 16);
static_assert(sizeof(S3) == 32);
```

### 类的内存布局
#### 没有虚表
一个经典的菱形继承：

```c++
struct A {
    int a = 1; // 4
};

struct B : A {
    // 4 基类A
    int b = 2; // 4
};

struct C : A {
    // 4 基类A
    int c = 3; // 4
};

struct D : B, C {
    // 8 基类B，其中
    //     4 基类A，其中：
    //         4 a
    //     4 b
    // 8 基类C，其中
    //     4 基类A，其中：
    //         4 a
    //     4 c
    int d = 4; // 4
};
static_assert(sizeof(D) == 20);

D d;
// d.a = 1; // 错误，a有歧义，可能是B的基类A里的a或者C的基类A里的a
d.B::a = 1;
```

可以看到A在D中有2个副本，为了解决菱形继承的问题，C++又引入了虚继承。虚继承表示这个基类的偏移量是编译期不能确定的，需要运行时去虚表找

#### 两个基类都用虚继承
有虚函数或者虚继承都会引入虚表指针，但是只有虚继承没有虚函数的对象也不是多态对象

```c++
struct A {
    int a = 1; // 4
};

struct B : virtual A {
    // 8 B的虚表指针（虚表里存了基类A的偏移量）
    int b = 2; // 4
    // 4 基类A，其中：
    //     4 a
};
static_assert(sizeof(B) == 16);

// C的情况同B
struct C : virtual A {
    int c = 3; // 4
};

struct D : B, C {
    // 12 基类B，其中：
    //     8 B、D共用的虚表指针（指向D的虚表中的成员；虚表里存了基类A的偏移量）
    //     4 b
    // 4 填充，为了使基类C的偏移量能被8整除
    // 12 基类C，其中：
    //     8 C的虚表指针（指向D的虚表中的成员；虚表里存了基类A的偏移量）
    //     4 c
    int d = 4; // 4
    // 4 基类A，其中：
    //     4 a
    // 4 填充
};
static_assert(sizeof(D) == 40);

D d;
d.a = 1; // a现在只有一个，无歧义
```

虚继承的类一定放在末尾，而且是最先初始化的，因为它的信息只有最终派生类知道

#### 只有一个基类用了虚继承
如果一个基类用了虚继承，另一个没用，还是会导致A有两个副本。下面A还添加了虚函数，导致很复杂的情况

```c++
struct A {
    virtual void f() {}

    // 8 虚表指针
    int a = 1; // 4
    // 4 填充
};
static_assert(sizeof(A) == 16);

struct B : virtual A {
    // 8 B的虚表指针（虚表里存了基类A的偏移量）
    int b = 2; // 4
    // 4 填充
    // 12 基类A（不算末尾填充），其中：
    //     8 A的虚表指针（指向B的虚表中的成员）
    //     4 a
    // 4 填充
};
static_assert(sizeof(B) == 32);

struct C : A {
    // 12 基类A（不算末尾填充），其中：
    //     8 A、C共用的虚表指针（指向C的虚表中的成员）
    //     4 a
    int c = 3; // 4
};
static_assert(sizeof(C) == 16);

struct D : B, C {
    // 12 基类B（不算末尾填充），其中：
    //     8 B、D共用的虚表指针（指向D的虚表中的成员；虚表里存了基类A的偏移量）
    //     4 b
    // 4 填充
    // 16 基类C，其中：
    //     12 基类A（不算末尾填充），其中：
    //         8 A、C共用的虚表指针（指向D的虚表中的成员）
    //         4 a
    //     4 c
    int d = 4; // 4
    // 4 填充
    // 12 基类B的基类A，其中：
    //     8 A的虚表指针（指向D的虚表中的成员）
    //     4 a
    // 4 填充
};
static_assert(sizeof(D) == 56);
```


## 引用
[参考](https://zh.cppreference.com/w/cpp/language/reference)

引用是其他变量的别名，对引用赋值相当于对它指向的变量赋值。引用不必占用存储空间，但是编译器一般会实现为指针，所以一般会占用空间。引用不是对象，所以不存在引用的数组，不存在指向引用的指针，不存在引用的引用

### 左值引用、右值引用
* 非const的左值引用只能绑定到左值，不能绑定到右值；const左值引用可以绑定到左值或右值
* 右值引用只能绑定到右值，不能绑定到左值
* 右值引用和左值引用功能上一样，都可以通过引用修改它指向的变量。声明一个右值引用变量没什么意义，应该用在函数重载决议里提供移动语义才有意义

```c++
string s = "a";

// string&& rr1 = s; // 错误，右值引用不能绑定到左值
string&& rr2 = s + s; // 右值引用可以绑定到右值，它指向的临时变量的生存期会延长到引用的作用域结尾
rr2 = "b"; // 可以通过右值引用修改它指向的变量

// string& lr1 = move(s); // 错误，非const的左值引用不能绑定到右值
const string& lr2 = s + s; // const左值引用可以绑定到右值，它指向的临时变量的生存期会延长到引用的作用域结尾

string s2 = move(s); // 优先调用右值引用的重载（移动构造函数），如果没有才会调用const左值引用的重载（拷贝构造函数）
```

引用折叠：

* 通过模板或者类型别名可以构成引用的引用，此时引用会折叠，只会变成一个引用
* 规则是，右值引用的右值引用变成右值引用；如果其中一个是左值引用，则变成左值引用

### 转发引用
* 转发引用是一种特殊的引用，能绑定到左值和右值。绑定到左值时会变成左值引用，绑定到右值时会变成右值引用。一般用于模板中，配合`forward`转发实参，调用正确的下层函数
* 简单判断方法：如果`&&`左边是自动推导的，且没有cv限定，基本上就是转发引用

```c++
template<class T>
int f(T&& x) { // x 是转发引用
    return g(forward<T>(x));
}

int i = 0;
f(i); // 实参是左值，调用 f<int&>(int&)，forward<int&>(x) 是左值引用
f(0); // 实参是右值，调用 f<int>(int&&)，forward<int>(x) 是右值引用
f(move(i)); // 同上

auto&& vec = foo(); // foo() 可以是左值或右值，vec 是转发引用
```

### move和forward
* `move`就是强转成右值引用
* `forward`一般用于模板中，保持转发引用参数的引用类型，调用正确的下层函数重载

```c++
template<class T>
int f(T&& x) { // x 是转发引用
    return g(forward<T>(x)); // 这里 “<T>” 不能省略，因为无法自动推导
    // 如果不用forward，因为 “x”表达式 是一个左值，即使调用f时用了右值引用，调用g时用的还是左值引用的重载
}
```


## 值类别
参考：
* [cppreference](https://zh.cppreference.com/w/cpp/language/value_category)
* [一个容易理解的教程](https://riptutorial.com/cplusplus/topic/763/value-categories)

值类别是针对表达式的，不会说“某个变量的值类别是什么”，但是可以说“变量名构成的表达式值类别是什么”

作用：

* 值类别确定表达式的两个重要属性：
    1. 表达式是否具有标识，即引用到某个具有变量名的对象
    2. 从表达式的值隐式移动是否合法，或者说在用作函数参数时是否能绑定到右值引用
* 值类别主要用来影响函数的重载决议

基本类别：

* 左值：其值可确定某个标识，但不能被隐式移动
* 纯右值：没有标识的值，临时的值
* 亡值：将亡（expiring）的值，其值可确定某个标识，但是表示它的资源能够被重新使用，可以被隐式移动

混合类别：

* 泛左值 = 左值 + 亡值
    * 其值可确定某个标识
* 右值 = 纯右值 + 亡值
    * 不能被`&`取址
    * 不能被赋值
    * 可以用来初始化右值引用和const左值引用

### 常见例子
```c++
struct X { int n; };
X x{};
X&& rr = x;
string s = "a";
void f();

// 左值：
x // 变量名（无论什么类型）
rr // 类型是右值引用的变量名
f // 函数名
// 返回左值引用的函数调用或运算符
s.front()
s += "a"
*&x // 解指针
"a" // 字符串字面量

// 纯右值：
// 除了字符串字面量以外的字面量
1
true
nullptr
// 返回非引用的函数调用或运算符
s.substr()
s + s
1 + 1
X{1} // 临时对象
[]{} // Lambda表达式

// 亡值：
move(x) // 返回右值引用的函数调用或运算符
(X&&)x // 直接强转成右值引用
X{1}.n // 右值对象的成员
```

### 判断方法
用`decltype`的表达式版本

* 左值表达式产生左值引用
* 纯右值表达式产生非引用
* 亡值表达式产生右值引用

```c++
int i = 0;
static_assert(is_same_v<decltype((i)), int&>); // decltype内加一个额外的括号是为了强制使用它的表达式版本
static_assert(is_same_v<decltype((1)), int>);
static_assert(is_same_v<decltype((move(i))), int&&>);
```

## 类型转换
* `const_cast`：转换到不同的cv限定，可以用来去掉`const`
* `static_cast`：功能和C风格转换差不多，但是会加上限制，更安全
* `reinterpret_cast`：不改变内存中的值，只是让编译器将表达式视为新类型
* `dynamic_cast`：沿继承层级安全地转换。会检查转换是否是安全的，如果不安全，返回空指针或者抛异常。需要RTTI支持，只有多态对象才能使用

C风格的转换不推荐使用了，因为它包含`const_cast`、`static_cast`、`reinterpret_cast`的功能，可能有歧义

```c++
const int i = 0;
// const_cast<float>(i); // 禁止转换类型
// static_cast<int&>(i); // 禁止去掉const
// static_cast<float&>(i); // 禁止转换到无关的类型
static_cast<float>(i); // 可以，创建了i的拷贝
reinterpret_cast<void*>(i); // 可以，指针指向的地址就是i的值
const_cast<float&>(reinterpret_cast<const float&>(i)) = 1.23f; // 可以，对i的地址以float编码赋值
cout << const_cast<volatile int&>(i) << endl; // 用volatile强制重新读取i的值，i变成了1067282596


struct B {
    virtual void f() {} // 只有多态类型能用dynamic_cast
};

struct D : B {};

D* pd = dynamic_cast<D*>(new B()); // B对象的指针转换成D指针是不安全的，返回nullptr
B* pb = dynamic_cast<B*>(new D()); // D对象的指针转换成B指针是安全的
```


## 关键词
### const/volatile/constexpr
`const`：

* 用于变量时表示不可修改，编译器会阻止修改它。编译器可以假设它的值是不变的，这样只需要从内存中读取一次
* 用于类成员函数时指定`*this`引用必须是`const`的

`volatile`：

* 和`const`相反，表示变量可能随时被修改，每次访问它必须从内存重新读取，禁止优化
* 用于类成员函数时指定`*this`引用必须是`volatile`的
* `volatile`不能代替原子操作

`const`和`volatile`可以同时使用，表示编译时禁止修改，运行时每次访问它必须从内存重新读取

`constexpr`：

* 用于变量时表示编译期可以计算出来的真常量
* 用于函数时表示有可能编译期计算，但不是强制的。C++20的`consteval`才是强制编译期计算
* 另外C++17添加了`if constexpr`，用于编译期分支判断，可以简化模板元编程


### static和extern
`static`：

* 用于变量时指定静态（或线程）存储期，即用于局部变量时，离开作用域也不会销毁
* 指定变量或函数具有内部链接，即只能在当前翻译单元访问，其他翻译单元不可访问。可以用于避免同名函数重定义。这个作用也可以用匿名命名空间实现
* 在类里面，表示成员变量或函数不绑定到类实例，使用时不需要`this`指针

`extern`：

* 指定变量或函数具有外部链接，即它的定义可能在其他翻译单元，当前翻译单元可以不定义。一般用于变量，因为不带初始化器的变量声明会被当成定义，用`extern`可以消歧义，让它被当做声明
* 另一种用法是`extern "C"`，指定C语言链接。禁止对函数名称修饰，使得C、C++函数可以互相调用

`static`和`extern`是矛盾的，不可以同时用

### inline
* 本意是建议编译器在函数调用的地方展开函数体，而不生成一个函数调用
* 现在表示允许在不同的翻译单元重定义，链接时会只保留一个定义。如果不同的翻译单元有不同的定义，那么行为未定义
* C++17后允许对变量使用
* `inline`的函数、变量必须在使用它的每个翻译单元定义，一般会直接定义在头文件
* 定义在类定义内的函数是隐式`inline`的

### virtual
* `virtual`表示编译期不能确定，需要运行时动态查找
* 用于虚函数时表示调用时应该从虚表查找函数地址
* 用于虚继承时表示这个基类的偏移量是不确定的，需要从虚表查找

### auto和decltype
`auto`：

* 用于变量时，指定要从它的初始化器自动推导出它的类型
* 用于函数返回值时，指定要从`return`自动推导出它的类型，或者使用尾随返回类型声明
* C++14可以把Lambda表达式的参数声明为`auto`类型，相当于创建了一个模板函数

`decltype`：

* 用于计算实体或表达式的类型和值类别
* C++14添加了`decltype(auto)`，指定变量类型为`decltype(初始化器表达式)`。它不可以加`const`、`&`这样的修饰

```c++
auto i1 = 1; // int
auto& i2 = i1; // int&
auto i3 = move(i1); // int，auto是不会管值类别的
decltype(i1) i4 = i1; // int，decltype的实体版本结果就是实体的类型（包括引用）
decltype((i1)) i5 = i1; // int&，decltype的表达式版本会根据值类型决定引用类型，“i1”是左值，结果是左值引用
decltype((move(i1))) i6 = move(i1); // int&&，“move(i1)”是亡值，结果是右值引用
decltype(auto) i7 = move(i1); // 同上

// 推导返回值类型为int
auto f() {
    return 1;
}

// 尾随返回类型声明可用于返回类型由参数决定的情况
template<class T, class U>
auto add(T a, U b) -> decltype(a + b) {
    return a + b;
}

// C++14起，用于Lambda表达式参数时，相当于创建了一个模板函数
auto f = [](auto a, auto b) { return a + b; };
```


# STL容器
## array
数组的包装类，添加了容器的各种接口（`begin`、`size`等），而且`at`函数会检查越界，更安全

数据结构：数组

访问：

* `operator[]`、`at`：返回指定下标的元素引用，`at`会检查越界
* `front`、`back`：返回首、尾的元素引用


## vector
长度可变的数组，需要扩容时会分配新的内存，然后把旧的元素移动或拷贝过去。因为重分配后元素的地址会变，所以最好不要长期持有元素的指针。GCC的扩容策略是每次变成原来的2倍，而MSVC是1.5倍

数据结构：数组

访问：

* `operator[]`、`at`：返回指定下标的元素引用，`at`会检查越界
* `front`、`back`：返回首、尾的元素引用

修改：

* `push_back`、`emplace_back`：`push_back`将元素拷贝或移动添加到末尾；`emplace_back`是直接在末尾调用构造函数，参数是构造函数的参数
* `pop_back`：移除末尾元素
* `insert`、`emplace`：在迭代器指定的位置插入元素，`emplace`是直接调用构造函数
* `erase`：移除迭代器指定的元素


## list/forward_list
数据结构：`list`是双向链表，`forward_list`是单向链表

### list
访问：

* `front`、`back`：返回首、尾的元素引用

修改：

* `push_back`、`emplace_back`：添加到末尾
* `pop_back`：移除末尾元素
* `push_front`、`emplace_front`：添加到头部
* `pop_front`：移除头部元素
* `insert`、`emplace`：在迭代器指定的位置插入元素
* `erase`：移除迭代器指定的元素

### forward_list
访问：

* `front`：返回首个元素引用

修改：

* `push_front`、`emplace_front`：添加到头部
* `pop_front`：移除头部元素
* `insert_after`、`emplace_after`：在迭代器指定的位置后面插入元素
* `erase_after`：移除迭代器下一个指定的元素


## map/multimap/unordered_map/unordered_multimap
键值映射，默认（有序）版使用二叉搜索树实现，unordered版使用哈希表实现，multi版允许有多个相同键的元素。元素类型是`pair`，因为要有键和值

数据结构：`map`、`multimap`是红黑树，`unordered_map`、`unordered_multimap`是哈希表 + 链表

访问：

* （非multi版限定）`operator[]`、`at`：返回指定键的元素引用，`at`会检查越界。`operator[]`是非`const`限定的，因为键不存在时会自动创建
* `count`：返回有指定键的元素数量
* `find`：返回有指定键的元素的迭代器。既需要判断是否存在，又需要访问、修改时，可以使用这个

修改：

* `insert`、`emplace`：插入元素。对于非multi版，如果键已经存在，则插入失败
* `erase`：移除迭代器或键指定的元素


## set/multiset/unordered_set/unordered_multiset
集合，默认（有序）版使用二叉搜索树实现，unordered版使用哈希表实现，multi版允许有多个相同的元素

数据结构：`set`、`multiset`是红黑树，`unordered_set`、`unordered_multiset`是哈希表 + 链表

访问：

* `count`：返回指定元素数量
* `find`：返回指定元素的迭代器

修改：

* `insert`、`emplace`：插入元素。对于非multi版，如果键已经存在，则插入失败
* `erase`：移除迭代器或值指定的元素


## deque
双端队列，扩缩容比`vector`更快，因为不用移动元素到新的位置

数据结构：分段的数组。内部有一个数组存放每个分段的指针，分段里存的才是元素

访问：

* `operator[]`、`at`：返回指定下标的元素引用，`at`会检查越界
* `front`、`back`：返回首、尾的元素引用

修改：

* `push_back`、`emplace_back`：添加到末尾
* `pop_back`：移除末尾元素
* `push_front`、`emplace_front`：添加到头部
* `pop_front`：移除头部元素
* `insert`、`emplace`：在迭代器指定的位置插入元素
* `erase`：移除迭代器指定的元素


## queue
队列，先进先出

数据结构：默认是`deque`

访问：

* `front`、`back`：返回首、尾的元素引用

修改：

* `push`、`emplace`：添加到末尾
* `pop`：移除头部元素


## stack
栈，先进后出

数据结构：默认是`deque`

访问：

* `top`：返回栈顶元素引用

修改：

* `push`、`emplace`：添加到栈顶
* `pop`：移除栈顶元素


## priority_queue
优先队列。默认是最大的排在前面，如果想改成最小的，可以使用 `priority_queue<int, vector<int>, greater<int>>` 这样的参数。因为标准定义`priority_queue`优先输出最大的元素，所以输出顺序和比较的大小相反

数据结构：默认是基于`vector`的二叉堆

访问：

* `top`：返回队首元素引用

修改：

* `push`、`emplace`：添加到队列里面并排序
* `pop`：移除队首元素并排序


# 多线程
## 线程管理
`thread`类用来创建、管理线程

因为RAII的语义，在析构`thread`之前，必须保证它代表的线程已经结束，否则会直接调用`terminate`。也就是要保证在析构之前：

* 不关联到任何线程，即刚被默认构造或者被移动了
* 调用了`join`，等待关联的线程结束
* 调用了`detach`，使关联的线程和`thread`对象分离，之后线程就不归C++的对象管了

```c++
void f(int arg) {
    cout << arg << endl;
}

// 创建一个线程然后立即分离
thread([]{
    cout << "在线程中" << endl;
}).detach();

vector<thread> threads;
for (int i = 0; i < 5; i++) {
    threads.emplace_back(f, i);
}
for (thread& t : threads) {
    t.join(); // 等待线程结束
}
```


## 线程同步
### thread_local
为了充分利用多线程的性能，最好的方法就是不共享内存。`thread_local`关键词指定变量具有线程存储期，不同的线程访问同一个`thread_local`变量会访问到不同的实例，每个线程首次访问这个变量时才初始化

原理是每个线程有自己的本地存储（TLS），里面存了一个指针，指向一个存储所有`thread_local`变量的结构，每个线程访问时去自己的TLS找

```c++
struct S {
    S() {
        cout << "构造" << endl;
    }

    ~S() {
        cout << "析构" << endl;
    }

    int i = 0;
};

thread_local S s;

cout << s.i << endl;

thread([]{
    cout << "线程启动" << endl;
    s.i = 1;
    cout << s.i << endl;
    cout << "线程结束" << endl;
}).join();

cout << s.i << endl;

// 输出：
// 构造         // 主线程第一次访问
// 0
// 线程启动
// 构造         // 线程第一次访问
// 1            // 修改了线程中变量的值
// 线程结束
// 析构         // 线程结束，析构
// 0            // 不影响主线程变量的值
// 析构         // 主线程结束，析构
```

### 原子操作
原子操作基于CPU提供的比较并交换（CAS）操作，CPU会保证这个过程不被打断

```c++
atomic<int> val = 0;
auto f = [&]{
    for (int j = 0; j < 1000000; j++) {
        val++;
    }
};

vector<thread> threads;
for (int i = 0; i < 5; i++) {
    threads.emplace_back(f);
}
for (thread& t : threads) {
    t.join();
}

cout << val << endl; // 一定是5000000
```

可以用`compare_exchange_weak`、`compare_exchange_strong`直接进行CAS，weak版本可能会虚假地失败，但是性能更好。这个函数会比较expected和变量值是否相等，如果相等则把变量值设置为desired，否则把expected设置为变量当前的值。这是一种乐观锁，如果线程竞争很少时性能比悲观锁好，但如果竞争很多则会经常失败和重试，浪费CPU

```c++
auto f = [&]{
    for (int j = 0; j < 1000000; j++) {
        int old_val = val;
        int new_val = 0;
        do {
            // 这里可以进行一堆复杂操作
            new_val = old_val + 1;
            // 如果失败了再用新的值计算new_val，再CAS，直到成功
        } while (!val.compare_exchange_weak(old_val, new_val));
    }
};
```

CAS还会有ABA问题，即变量的值由A变成B，再变成A，此时CAS会成功。一种解决方法是加上版本，比如原来32位整数改用64位整数，高32位表示版本，CAS的时候把版本和值一起修改

### 互斥量和锁
`mutex`同一时间只能有一个线程锁定，其它线程再次调用`lock`会阻塞，直到锁释放，这样保证了同时只有一个线程能访问资源。一般不会直接调用`mutex`的`lock`，而是通过RAII来管理锁

`lock_guard`和`unique_lock`都是用RAII来管理锁的类，区别是`unique_lock`更加灵活。它提供了所有权语义，可以提前解锁，可以转移所有权。性能上差不多，只是`unique_lock`多了几个if判断而已

```c++
mutex mu;

void f() {
    // 这里是不用锁定的代码...
    {
        lock_guard lock(mu);
        // 或者 unique_lock lock(mu);
        // C++17之前要写成 lock_guard<mutex>

        // 这里同时只能有一个线程执行，可以安全操作线程共享的资源了
    }
    // 这里是不用锁定的代码...
}
```

`shared_mutex`是读写锁（共享、独占锁），可以使只读的线程不会互相阻塞，而读和写、写和写会互相阻塞。只读的线程用`shared_lock`来管理锁，需要写的线程用`unique_lock`

```c++
shared_mutex mu;

{
    unique_lock lock(mu);
    // 写的代码...
}

{
    shared_lock lock(mu);
    // 只读的代码...
}
```

C++20还提供了信号量`counting_semaphore`，允许同时有多个线程访问，但数量不能超过一定量。它不是用来保证线程安全的，而是用来控制并发数的。也可以用于发送提醒，它有比条件变量更好的性能

### 条件变量
`condition_variable`用于阻塞线程，直到其它线程修改了一些共享变量（条件），并通知唤醒线程。它要配合锁使用，这个锁是用来保护共享变量的，而不是保护条件变量本身

要等待条件的线程：

1. 获得锁
2. 检查条件
3. 如果条件不成立，调用`wait`，会释放锁并阻塞等待通知
4. 被唤醒后会获得锁，这时可能是被虚假唤醒的，要再次检查条件，如果条件不成立则再次`wait`

要修改条件并发通知的线程：

1. 获得锁
2. 修改条件
3. 发送通知（这一步不需要持有锁）

虚假唤醒的原因：

* 可能是`wait`在系统调用的时候收到了信号，系统调用被打断了
* 可能一次唤醒了多个线程，第一个线程先拿到锁，修改了条件，然后第二个线程拿到锁时发现条件不成立了
* 还跟操作系统底层实现有关，[参考](https://linux.die.net/man/3/pthread_cond_signal)

```c++
bool flag = false;
mutex flag_mu;
condition_variable flag_cv;

thread t([&]{
    unique_lock lock(flag_mu);
    // 传入一个可调用对象，会自动判断条件成立，避免虚假唤醒
    flag_cv.wait(lock, [&]{ return flag; });

    // 做一些处理...
    cout << "条件成立" << endl;
    flag = false;
});

// 先不获取锁，让线程走到wait
this_thread::sleep_for(1s);
{
    unique_lock lock(flag_mu);
    flag = true;
}
cout << "准备通知" << endl;
flag_cv.notify_one();

t.join();
```

### Future
`future`表示一个现在可能没准备好，未来会准备好的值。`future`一般由`promise`创建，`promise`可以用来设置值或异常

```c++
void add(int a, int b, promise<int> p) {
    this_thread::sleep_for(1s);
    p.set_value(a + b);
}

promise<int> p;
future fu = p.get_future();

thread t(add, 1, 2, move(p));

// fu.wait(); // 可以先wait等待值准备好
int res = fu.get(); // 获取值，如果没准备好则阻塞直到准备好
cout << res << endl; // 3

t.join();
```

`packaged_task`封装可调用对象的一次调用，可以通过`future`获取它的返回值

```c++
int add(int a, int b) {
    this_thread::sleep_for(1s);
    return a + b;
}

// 封装add。task也是个可调用对象，调用后给future设置值
packaged_task<int(int, int)> task(add);
future fu = task.get_future();

thread t(move(task), 1, 2);

int res = fu.get();
cout << res << endl; // 3

t.join();
```


# 新标准常用特性
## C++11
### 语法
* [基于范围的for循环](https://zh.cppreference.com/w/cpp/language/range-for)：用来方便迭代数组和任意容器

    ```c++
    vector<int> vec;
    for (int i : vec) {} // 拷贝成员到i
    for (int& i : vec) {} // 绑定成员的引用到i
    for (auto&& i : vec) {} // 转发引用
    ```

* [类型别名](https://zh.cppreference.com/w/cpp/language/type_alias)：和`typedef`差不多，但是用于复杂类型时可读性更好。另外还可以定义模板的别名

    ```c++
    using Func = void (*)(int, float);

    template<class T>
    using IntMap = map<int, T>;
    static_assert(is_same_v<IntMap<float>, map<int, float>>);
    ```

* [模板形参包](https://zh.cppreference.com/w/cpp/language/parameter_pack)：模板实参长度可变。不过这时候的包展开只能变成逗号分隔的模式，C++17的折叠表达式才支持更多运算符。要展开成复杂的模式暂时只能用模板递归的方法

    ```c++
    template<class... Types>
    struct Tuple {};
    Tuple<int, float> t;

    template<class... Ts>
    void f(Ts... args) {
        g(&args...); // “&args...” 是包展开，“&args” 是它的模式
    }
    f(1, 2.0);
    ```

### 类型
* [右值引用](https://zh.cppreference.com/w/cpp/language/reference)：用于提供移动语义，消除没必要的深拷贝
* [nullptr](https://zh.cppreference.com/w/cpp/language/nullptr)：提供空指针字面量，取代以前的`NULL`宏，避免重载决议时的歧义
* [constexpr](https://zh.cppreference.com/w/cpp/language/constexpr)：用于变量、函数表示可以在编译期计算
* [auto](https://zh.cppreference.com/w/cpp/language/auto)、[decltype](https://zh.cppreference.com/w/cpp/language/decltype)：自动推导类型和获取实体、表达式的类型

### 类
* [非静态成员默认初始化器](https://zh.cppreference.com/w/cpp/language/data_members#.E6.88.90.E5.91.98.E5.88.9D.E5.A7.8B.E5.8C.96)：指定成员初始化器列表中省略这个成员时应该使用的初始化器

    ```C++
    struct S {
        int i = 0;
        string s{"a"};
        S() : i(1) {} // i以1初始化，s以"a"初始化
    };
    ```

* [移动构造函数](https://zh.cppreference.com/w/cpp/language/move_constructor)、[移动赋值运算符](https://zh.cppreference.com/w/cpp/language/move_assignment)：配合右值引用用来消除没必要的深拷贝
* [列表初始化](https://zh.cppreference.com/w/cpp/language/list_initialization)：用于容器，在初始化时指定容器内容

    ```c++
    vector<int> v{1, 2, 3};
    map<int, int> m{{1, 1}, {2, 2}};
    ```

* [default](https://zh.cppreference.com/w/cpp/language/function#.E5.87.BD.E6.95.B0.E5.AE.9A.E4.B9.89)、[delete](https://zh.cppreference.com/w/cpp/language/function#.E5.BC.83.E7.BD.AE.E5.87.BD.E6.95.B0)：`default`用于某些特殊成员函数使用默认生成的定义，`delete`用于禁止使用成员函数。比如定义了其他构造函数时，编译器不会自动生成默认构造函数，这时可以使用`default`显式定义；`delete`可以用于禁止拷贝构造、赋值
* [override](https://zh.cppreference.com/w/cpp/language/override)、[final](https://zh.cppreference.com/w/cpp/language/final)：`override`用于虚函数，指定它必须覆盖基类的函数，这是为了避免基类函数改名了而派生类没有改；`final`用于虚函数时指定它不能被派生类覆盖，或者用于类指定它不能被继承

### 函数
* [Lambda表达式](https://zh.cppreference.com/w/cpp/language/lambda)：创建一个匿名的可调用对象，它可以以值或引用捕获所在作用域中的变量

    ```c++
    int i = 0;
    auto change_i = [&](int new_val) -> int { // 默认按引用捕获
        int old_val = i;
        i = new_val; // 修改i会影响原变量
        return old_val;
    };
    change_i(1);
    // i == 1

    int a = 0, b = 0, c = 0;
    auto f1 = [&, b] {}; // 默认按引用捕获，a、c按引用捕获，但是b按值捕获
    auto f2 = [=, &b] {}; // 默认按值捕获，a、c按值捕获，但是b按引用捕获
    auto f3 = [a, &b] {}; // 不指定默认捕获，a按值捕获，b按引用捕获，c不捕获
    ```

* [bind](https://zh.cppreference.com/w/cpp/utility/functional/bind)：固定一个可调用对象的部分参数，生成一个新的可调用对象。可以用于成员函数，固定`this`参数。`bind`只是顺便从boost库拿来的，C++14后，Lambda表达式可以完全取代`bind`，而且优化更好

    ```c++
    int add(int a, int b) {
        return a + b;
    }

    auto f1 = bind(add, placeholders::_1, 1); // a传入参数1，b固定传入1
    // f1(2) == 3

    string s = "abc";
    auto f2 = bind(&string::size, &s); // this参数固定传入&s
    // f2() == 3
    ```

* [function](https://zh.cppreference.com/w/cpp/utility/functional/function)：表示任何可调用对象，包括函数、Lambda表达式、定义了`()`操作符的对象

    ```c++
    int g(int x) {
        return x + 1;
    }

    int h(int x, int y) {
        return x + y;
    }

    function<int(int)> f;
    f = g;
    f = [&](int x){ return x + 1; };
    f = bind(h, placeholders::_1, 1);
    // f(2) == 3
    ```

### 多线程
* [thread](https://zh.cppreference.com/w/cpp/thread)：提供了创建线程、线程同步、原子操作等多线程支持
* [thread_local](https://zh.cppreference.com/w/cpp/language/storage_duration#.E5.AD.98.E5.82.A8.E6.9C.9F)：指定变量具有线程存储期。不同的线程访问同一个`thread_local`变量会访问到不同的实例
* C++11开始才有了多线程的概念，这意味着下面这个单例模式以前不是线程安全的，而现在编译器会自动加锁检查是否初始化

    ```c++
    static C& getInstance() {
        static C instance; // 初次调用时初始化
        return instance;
    }
    ```

### 标准库
* [智能指针](https://zh.cppreference.com/w/cpp/memory/unique_ptr)：使用RAII管理的指针，并且与裸指针相比，添加了所有权语义。裸指针表示没有所有权；`unique_ptr`独占所有权，不需要引用计数；`shared_ptr`表示共享所有权，只有引用计数变成0才销毁对象。为了防止循环引用，又添加了`weak_ptr`，它不直接持有对象指针，而是持有控制块的指针，需要使用对象时临时获取一个`shared_ptr`，对象已经销毁的情况下会获取失败
* [正则表达式](https://zh.cppreference.com/w/cpp/regex)：匹配、搜索、替换特定模式的字符串
* [unordered_map](https://zh.cppreference.com/w/cpp/container/unordered_map)、[unordered_set](https://zh.cppreference.com/w/cpp/container/unordered_set)：哈希表版的`map`和`set`
* [tuple](https://zh.cppreference.com/w/cpp/utility/tuple)：可存放不同类型的容器，一般用于返回多个值，或者作为`map`的键类型
* [array](https://zh.cppreference.com/w/cpp/container/array)：数组的包装类，添加了容器的各种接口，而且`at`函数会检查越界


## C++14
* [变量模板](https://zh.cppreference.com/w/cpp/language/variable_template)：用于简化模板元编程，原来的`is_same<>::value`相当于`is_same_v<>`
* [泛型Lambda](https://zh.cppreference.com/w/cpp/language/lambda)：Lambda表达式可以把参数类型声明为`auto`，相当于创建了一个模板函数
* [函数的返回类型推导](https://zh.cppreference.com/w/cpp/language/function#.E8.BF.94.E5.9B.9E.E7.B1.BB.E5.9E.8B.E6.8E.A8.E5.AF.BC_.28C.2B.2B14_.E8.B5.B7.29)：函数返回类型可以声明为`auto`，会从`return`语句推导


## C++17
### 变量
* [inline变量](https://zh.cppreference.com/w/cpp/language/inline)：`inline`可用于变量，可以在不同翻译单元中多次定义一个变量。要求所有定义相同，否则是未定义行为
* [结构化绑定](https://zh.cppreference.com/w/cpp/language/structured_binding)：类似于引用，可以创建别名，区别是结构化绑定的类型不必是引用类型，而且结构化绑定可以用于位域

    ```c++
    // 用于数组
    int arr[]{1, 2};
    auto [a, b] = arr; // 把arr拷贝了，修改a、b不会影响arr的值
    auto& [c, d]{arr}; // 修改c、d会影响arr的值
    auto&& [e, f]{arr}; // 同上，这里是转发引用，e、f被折叠成左值引用了

    // 用于元组式类型
    tuple<int, int> t{1, 2};
    auto&& [e, f]{t};

    // 用于结构体
    struct S {
        int x : 16; // 支持位域
        int y;
    };
    S s{1, 2};
    auto&& [e, f]{s};

    // 常见的用法，迭代map
    map<int, int> m{{1, 1}};
    for (auto&& [key, value] : m) {
        // key是const限定的，不可修改；value可以修改
    }
    ```

### 模板
* [类模板实参推导](https://zh.cppreference.com/w/cpp/language/class_template_argument_deduction)：从初始化器类型推导缺失的模板实参

    ```c++
    tuple t{1, 2.0}; // 推导出 tuple<int, double>
    unique_lock lock{mu}; // 推导出 unique_lock<mutex>
    ```

* [折叠表达式](https://zh.cppreference.com/w/cpp/language/fold)：用来简化模板元编程，取代递归处理形参包

    ```c++
    template<class... Args>
    void printer(Args&&... args)
    {
        (std::cout << ... << args) << '\n'; // 二元左折叠
    }

    template<class T, class... Args>
    void push_back_vec(std::vector<T>& v, Args&&... args)
    {
        static_assert((std::is_constructible_v<T, Args&> && ...)); // 一元右折叠
        (v.push_back(args), ...); // 一元右折叠
    }
    ```

* [if constexpr](https://zh.cppreference.com/w/cpp/language/if)：编译期if，用来简化模板元编程

### 标准库
* [optional](https://zh.cppreference.com/w/cpp/utility/optional)：表示可以存在或不存在的值，一般用于返回值，取代`nullptr`。和`unique_ptr`区别是，`optional`不用分配额外内存，值就是`optional`内存的一部分
* [any](https://zh.cppreference.com/w/cpp/utility/any)：表示任意类型的值，可以取代`void*`，而且更安全，因为它会保存值的类型信息，并在转换时检查是否可以转换
* [variant](https://zh.cppreference.com/w/cpp/utility/variant)：表示可能是某几个类型的值，可以取代`union`。它同样会保存和检查类型，而且比起`union`，可以持有拥有复杂构造函数的对象
* [shared_mutex](https://zh.cppreference.com/w/cpp/thread/shared_mutex)：并发读写锁，读者不会互相阻塞
* [string_view](https://zh.cppreference.com/w/cpp/header/string_view)：`string`的优化版，不拥有内存，只持有指针和长度，构造和拷贝更快。可以取代`const string&`
* [文件系统库](https://zh.cppreference.com/w/cpp/header/filesystem)：提供跨平台的文件系统操作，以前只能使用平台特定的API


## C++20
* [协程](https://zh.cppreference.com/w/cpp/language/coroutines)：可暂停执行和恢复的函数，可以用来简化异步编程、实现生成器。C++标准提供的是无栈协程
* [模块](https://zh.cppreference.com/w/cpp/language/modules)：用来取代头文件，在多个翻译单元间共享声明和定义。去掉头文件后编译会更快，因为头文件在每个翻译单元都要重复处理
* [约束和概念](https://zh.cppreference.com/w/cpp/language/constraints)：用来简化模板元编程，并且使报错信息更容易理解（万恶的SFINAE）
* [三路比较运算符<=>](https://zh.cppreference.com/w/cpp/language/operator_comparison#.E4.B8.89.E8.B7.AF.E6.AF.94.E8.BE.83)：类似于`strcmp`，调用一次即可分辨出大于小于还是等于
