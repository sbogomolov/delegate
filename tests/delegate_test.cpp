#include <delegate/delegate.h>

#include <gtest/gtest.h>

#include <utility>
#include <vector>

namespace delegate {

struct DelegateTestClass {
    int Method(int, float) { return 1; }
    int MethodSameSignature(int, float) { return 15; }
    void MethodNoReturn(int& ret, float) { ret = 2; }
    int MethodNoArgs() { return 3; }

    int MethodConst() const { return 4; }
    int MethodVolatile() volatile { return 5; }
    int MethodConstVolatile() const volatile { return 6; }

    int MethodLvalueRef() & { return 16; }
    int MethodConstLvalueRef() const & { return 17; }
    int MethodVolatileLvalueRef() volatile & { return 18; }
    int MethodConstVolatileLvalueRef() const volatile & { return 19; }
    int MethodLvalueRefNoexcept() & noexcept { return 20; }
    int MethodRvalueRef() && { return 21; }

    int MethodNoexcept() noexcept { return 7; }
    int MethodConstNoexcept() const noexcept { return 8; }
    int MethodVolatileNoexcept() volatile noexcept { return 9; }
    int MethodConstVolatileNoexcept() const volatile noexcept { return 10; }

    static int StaticMethod(int, float) { return 11; }
    static void StaticMethodNoReturn(int& ret, float) { ret = 12; }
    static int StaticMethodNoArgs() { return 13; }

    static int StaticMethodNoexcept() noexcept { return 14; }
};

struct DelegateForwardingTestNonMovable {
    DelegateForwardingTestNonMovable() = default;
    DelegateForwardingTestNonMovable(const DelegateForwardingTestNonMovable&) = delete;
    DelegateForwardingTestNonMovable& operator=(const DelegateForwardingTestNonMovable&) = delete;
    DelegateForwardingTestNonMovable(DelegateForwardingTestNonMovable&&) = delete;
    DelegateForwardingTestNonMovable& operator=(DelegateForwardingTestNonMovable&&) = delete;
};

struct DelegateForwardingTestSink {
    const DelegateForwardingTestNonMovable* stored_ptr = nullptr;

    void ConstLvalueRef(const DelegateForwardingTestNonMovable& value) {
        stored_ptr = &value;
    }

    void LvalueRef(DelegateForwardingTestNonMovable& value) {
        stored_ptr = &value;
    }

    void RvalueRef(DelegateForwardingTestNonMovable&& value) {
        stored_ptr = &value;
    }

    void PointerByValue(const DelegateForwardingTestNonMovable* value) {
        stored_ptr = value;
    }
};

struct DelegateCompatTestClass {
    int value = 20;

    int Add(int x) { return value + x; }
    int& ValueRef() { return value; }
};

// Compile-time probe for bind-site rejection: inside a concept the FromMethod constraint failure is a
// substitution failure, so the probe evaluates to false instead of hard-erroring as a bare
// requires-expression in a non-template context would.
template <typename DelegateT, typename T, auto method>
concept CanBindMethod = requires(T* object) { DelegateT::template FromMethod<T, method>(object); };

struct DelegateInheritanceTestBase {
    int BaseMethod(int x) { return x + 100; }
};

struct DelegateInheritanceTestDerived : DelegateInheritanceTestBase {};

TEST(DelegateTest, InstanceMethods) {
    DelegateTestClass test;

    auto delegate = CreateDelegate<&DelegateTestClass::Method>(&test);
    EXPECT_EQ(delegate(1, 2.3f), 1);

    auto delegate_no_ret = CreateDelegate<&DelegateTestClass::MethodNoReturn>(&test);
    int ret;
    delegate_no_ret(ret, 2.3f);
    EXPECT_EQ(ret, 2);

    auto delegate_no_args = CreateDelegate<&DelegateTestClass::MethodNoArgs>(&test);
    EXPECT_EQ(delegate_no_args(), 3);
}

TEST(DelegateTest, ConstVolatileMethods) {
    DelegateTestClass test;

    auto delegate_c = CreateDelegate<&DelegateTestClass::MethodConst>(&test);
    EXPECT_EQ(delegate_c(), 4);

    auto delegate_v = CreateDelegate<&DelegateTestClass::MethodVolatile>(&test);
    EXPECT_EQ(delegate_v(), 5);

    auto delegate_cv = CreateDelegate<&DelegateTestClass::MethodConstVolatile>(&test);
    EXPECT_EQ(delegate_cv(), 6);
}

TEST(DelegateTest, ConstVolatileObjects) {
    // Qualified methods must bind through pointers to equally qualified objects — the delegate
    // erases the qualifier at bind time and MethodStub restores it before the call.
    const DelegateTestClass const_test{};
    auto delegate_c = CreateDelegate<&DelegateTestClass::MethodConst>(&const_test);
    EXPECT_EQ(delegate_c(), 4);

    volatile DelegateTestClass volatile_test{};
    auto delegate_v = CreateDelegate<&DelegateTestClass::MethodVolatile>(&volatile_test);
    EXPECT_EQ(delegate_v(), 5);

    const volatile DelegateTestClass cv_test{};
    auto delegate_cv = CreateDelegate<&DelegateTestClass::MethodConstVolatile>(&cv_test);
    EXPECT_EQ(delegate_cv(), 6);

    auto delegate_c_n = CreateDelegate<&DelegateTestClass::MethodConstNoexcept>(&const_test);
    EXPECT_EQ(delegate_c_n(), 8);
}

TEST(DelegateTest, NoexceptMethods) {
    DelegateTestClass test;

    auto delegate_n = CreateDelegate<&DelegateTestClass::MethodNoexcept>(&test);
    EXPECT_EQ(delegate_n(), 7);

    auto delegate_c_n = CreateDelegate<&DelegateTestClass::MethodConstNoexcept>(&test);
    EXPECT_EQ(delegate_c_n(), 8);

    auto delegate_v_n = CreateDelegate<&DelegateTestClass::MethodVolatileNoexcept>(&test);
    EXPECT_EQ(delegate_v_n(), 9);

    auto delegate_cv_n = CreateDelegate<&DelegateTestClass::MethodConstVolatileNoexcept>(&test);
    EXPECT_EQ(delegate_cv_n(), 10);
}

TEST(DelegateTest, LvalueRefQualifiedMethods) {
    DelegateTestClass test;

    auto delegate_l = CreateDelegate<&DelegateTestClass::MethodLvalueRef>(&test);
    EXPECT_EQ(delegate_l(), 16);

    auto delegate_cl = CreateDelegate<&DelegateTestClass::MethodConstLvalueRef>(&test);
    EXPECT_EQ(delegate_cl(), 17);

    auto delegate_vl = CreateDelegate<&DelegateTestClass::MethodVolatileLvalueRef>(&test);
    EXPECT_EQ(delegate_vl(), 18);

    auto delegate_cvl = CreateDelegate<&DelegateTestClass::MethodConstVolatileLvalueRef>(&test);
    EXPECT_EQ(delegate_cvl(), 19);

    auto delegate_l_n = CreateDelegate<&DelegateTestClass::MethodLvalueRefNoexcept>(&test);
    EXPECT_EQ(delegate_l_n(), 20);

    // &&-qualified methods stay unbindable: the delegate always calls through an lvalue object, so
    // consume-once methods would double-move on a second call (std::function and C++26 function_ref
    // reject them too).
    static_assert(!CanBindMethod<Delegate<int()>, DelegateTestClass, &DelegateTestClass::MethodRvalueRef>);
}

TEST(DelegateTest, StaticMethods) {
    auto static_delegate = CreateDelegate<&DelegateTestClass::StaticMethod>();
    EXPECT_EQ(static_delegate(1, 2.3f), 11);

    auto static_delegate_no_ret = CreateDelegate<&DelegateTestClass::StaticMethodNoReturn>();
    int ret;
    static_delegate_no_ret(ret, 2.3f);
    EXPECT_EQ(ret, 12);

    auto static_delegate_no_args = CreateDelegate<&DelegateTestClass::StaticMethodNoArgs>();
    EXPECT_EQ(static_delegate_no_args(), 13);
}

TEST(DelegateTest, StaticNoexceptMethods) {
    auto static_delegate_n = CreateDelegate<&DelegateTestClass::StaticMethodNoexcept>();
    EXPECT_EQ(static_delegate_n(), 14);
}

TEST(DelegateTest, ForwardsConstLvalueReference) {
    DelegateForwardingTestSink sink;
    DelegateForwardingTestNonMovable value;

    auto delegate = CreateDelegate<&DelegateForwardingTestSink::ConstLvalueRef>(&sink);
    delegate(value);

    EXPECT_EQ(sink.stored_ptr, &value);
}

TEST(DelegateTest, ForwardsLvalueReference) {
    DelegateForwardingTestSink sink;
    DelegateForwardingTestNonMovable value;

    auto delegate = CreateDelegate<&DelegateForwardingTestSink::LvalueRef>(&sink);
    delegate(value);

    EXPECT_EQ(sink.stored_ptr, &value);
}

TEST(DelegateTest, ForwardsRvalueReference) {
    DelegateForwardingTestSink sink;
    DelegateForwardingTestNonMovable value;

    auto delegate = CreateDelegate<&DelegateForwardingTestSink::RvalueRef>(&sink);
    delegate(std::move(value));

    EXPECT_EQ(sink.stored_ptr, &value);
}

TEST(DelegateTest, PassesPointerByValue) {
    DelegateForwardingTestSink sink;
    DelegateForwardingTestNonMovable value;

    auto delegate = CreateDelegate<&DelegateForwardingTestSink::PointerByValue>(&sink);
    delegate(&value);

    EXPECT_EQ(sink.stored_ptr, &value);
}

TEST(DelegateTest, DefaultConstructedIsInvalid) {
    const Delegate<int(int, float)> empty;
    EXPECT_FALSE(empty.IsValid());

    DelegateTestClass test;
    const auto bound = CreateDelegate<&DelegateTestClass::Method>(&test);
    EXPECT_TRUE(bound.IsValid());
}

TEST(DelegateTest, BindingNullObjectYieldsInvalidDelegate) {
    // A method cannot be called on a null object, so binding one produces the same invalid
    // delegate as default construction — IsValid() stays a truthful guard.
    DelegateTestClass* null_object = nullptr;

    const auto delegate = CreateDelegate<&DelegateTestClass::Method>(null_object);

    EXPECT_FALSE(delegate.IsValid());
}

TEST(DelegateTest, BindingNullTargetYieldsInvalidDelegate) {
    // Null method and function constants also bind to the invalid delegate. (These checks are
    // runtime, not static_asserts: GCC cannot constant-evaluate the comparisons under
    // -fsanitize=undefined.)
    DelegateTestClass test;
    const auto null_method =
        Delegate<int(int, float)>::FromMethod<DelegateTestClass,
                                              static_cast<int (DelegateTestClass::*)(int, float)>(
                                                  nullptr)>(&test);

    EXPECT_FALSE(null_method.IsValid());

    const auto null_function = Delegate<int(int, float)>::FromStatic<nullptr>();

    EXPECT_FALSE(null_function.IsValid());
}

TEST(DelegateTest, AssignedAfterDefaultConstructionIsCallable) {
    DelegateTestClass test;

    // The "bind later" case the default constructor exists for: hold an empty delegate, assign a
    // target afterwards, then call it.
    Delegate<int(int, float)> delegate;
    ASSERT_FALSE(delegate.IsValid());

    delegate = CreateDelegate<&DelegateTestClass::Method>(&test);

    EXPECT_TRUE(delegate.IsValid());
    EXPECT_EQ(delegate(1, 2.3f), 1);
}

TEST(DelegateTest, EqualityComparesBoundTarget) {
    DelegateTestClass test;
    DelegateTestClass other;

    const auto bound = CreateDelegate<&DelegateTestClass::Method>(&test);

    // Same object, same method: equal.
    EXPECT_EQ(bound, CreateDelegate<&DelegateTestClass::Method>(&test));
    // Different object: unequal.
    EXPECT_NE(bound, CreateDelegate<&DelegateTestClass::Method>(&other));
    // Same object, different method of the same signature: unequal.
    EXPECT_NE(bound, CreateDelegate<&DelegateTestClass::MethodSameSignature>(&test));
    // Bound vs invalid, and invalid vs invalid.
    EXPECT_NE(bound, (Delegate<int(int, float)>{}));
    EXPECT_EQ((Delegate<int(int, float)>{}), (Delegate<int(int, float)>{}));
    // Function delegates compare by bound function.
    EXPECT_EQ(CreateDelegate<&DelegateTestClass::StaticMethod>(),
              CreateDelegate<&DelegateTestClass::StaticMethod>());
    EXPECT_NE(CreateDelegate<&DelegateTestClass::StaticMethod>(), bound);
}

TEST(DelegateTest, EqualityEnablesEraseByValue) {
    // The observer-pattern round-trip: register delegates, later remove the one you registered.
    DelegateTestClass subscriber_a;
    DelegateTestClass subscriber_b;
    std::vector<Delegate<int(int, float)>> subscribers{
        CreateDelegate<&DelegateTestClass::Method>(&subscriber_a),
        CreateDelegate<&DelegateTestClass::Method>(&subscriber_b),
    };

    std::erase(subscribers, CreateDelegate<&DelegateTestClass::Method>(&subscriber_a));

    ASSERT_EQ(subscribers.size(), 1u);
    EXPECT_EQ(subscribers.front(), CreateDelegate<&DelegateTestClass::Method>(&subscriber_b));
}

TEST(DelegateTest, MethodSignatureCompatibility) {
    DelegateCompatTestClass test;

    // Argument and return values convert exactly as in a direct call.
    auto widened =
        Delegate<long(short)>::FromMethod<DelegateCompatTestClass, &DelegateCompatTestClass::Add>(&test);
    EXPECT_EQ(widened(static_cast<short>(2)), 22L);

    // A reference return binds directly: same object through a more-qualified reference.
    auto ref =
        Delegate<const int&()>::FromMethod<DelegateCompatTestClass, &DelegateCompatTestClass::ValueRef>(
            &test);
    EXPECT_EQ(&ref(), &test.value);

    // Compatible binds satisfy the probe...
    static_assert(CanBindMethod<Delegate<int(int)>, DelegateCompatTestClass, &DelegateCompatTestClass::Add>);
    static_assert(
        CanBindMethod<Delegate<long(short)>, DelegateCompatTestClass, &DelegateCompatTestClass::Add>);
    // ...while incompatible ones are rejected at the bind site by IsCompatibleMethod:
    // a non-const method on a const object,
    static_assert(
        !CanBindMethod<Delegate<int(int)>, const DelegateCompatTestClass, &DelegateCompatTestClass::Add>);
    // a reference delegate over a by-value return (the reference would dangle),
    static_assert(!CanBindMethod<Delegate<const int&(int)>, DelegateCompatTestClass,
                                 &DelegateCompatTestClass::Add>);
    // a reference delegate over a reference to a different type (would bind a temporary),
    static_assert(!CanBindMethod<Delegate<const long&()>, DelegateCompatTestClass,
                                 &DelegateCompatTestClass::ValueRef>);
    // a void delegate over a value-returning method (the stub cannot discard a return),
    static_assert(
        !CanBindMethod<Delegate<void(int)>, DelegateCompatTestClass, &DelegateCompatTestClass::Add>);
    // and a plain arity mismatch.
    static_assert(!CanBindMethod<Delegate<int()>, DelegateCompatTestClass, &DelegateCompatTestClass::Add>);
}

TEST(DelegateTest, BindsInheritedMethodOnDerivedInstance) {
    // &Derived::BaseMethod on an inherited method is a Base member pointer; binding it on a Derived* must
    // still work — FromMethod accepts an instance whose type derives from the method's class.
    DelegateInheritanceTestDerived derived;

    auto delegate = CreateDelegate<&DelegateInheritanceTestDerived::BaseMethod>(&derived);

    EXPECT_EQ(delegate(5), 105);
}

}  // namespace delegate
