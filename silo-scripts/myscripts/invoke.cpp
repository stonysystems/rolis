//
// Created by weihshen on 3/25/21.
//
#include <iostream>
#include <unordered_map>

using namespace std;

class abstract_invoke_wrapper {
public:
    virtual void output() { std::cout << "invoke in parent\n" ; };
    virtual void set_i(int) = 0 ;
} ;

class invoke_wrapper : public abstract_invoke_wrapper {

public:
    int i = 1 ;

    void output() {
        std::cout << "invoke inside wrapper: " << i << "\n" ;
    }

    void set_i(int ii) {
        i = ii ;
    }
};

void modify(abstract_invoke_wrapper * ww) {
    ww->set_i(10) ;
}

invoke_wrapper * vv () {
    static invoke_wrapper * ret = new invoke_wrapper ;
    std::cout << "1: " << &ret << std::endl;
    return ret ;
}

void test_same_two_pointers() {
    // all different returns have different addresses but point to the same data
    invoke_wrapper *v0 = vv() ;
    v0->i += 1 ;
    std::cout << "2: " << &v0 << std::endl;
    invoke_wrapper *v1 = vv() ;
    std::cout << "3: " << &v1 << std::endl;
    v1->i += 1 ;
    invoke_wrapper *v2 = vv() ;
    v2->i += 1 ;
    std::cout << "4: " << &v2 << std::endl;
    std::cout << v2->i << std::endl;
    // https://stackoverflow.com/questions/9974596/how-to-check-whether-two-pointers-point-to-the-same-object-or-not
    std::cout << ((void*)v0 == (void*)v2) << std::endl;
}

void test_reference_object() {
    abstract_invoke_wrapper *w = new invoke_wrapper ;
    w->output() ;
    modify(w) ;
    w->output() ;
}

int main() {
    test_reference_object() ;
    //test_same_two_pointers() ;
}